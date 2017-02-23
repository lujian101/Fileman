#include "stdafx.h"
#include "Fileman.h"
#include "FilemanDlg.h"
#include "afxdialogex.h"
#include "utils\Rflext.h"
#include "utils\Misc.h"
#include "tinyXml\tinyxml.h"
#include "utils\ScriptApi.h"
#include <deque>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace rflx;
using namespace rflext;
using namespace crim;

#define TREE_SEL_INTERVAL				100
#define TREE_ED_INTERVAL				500
#define FILESYSTEM_WATCHDOG_INTERVAL	1000

#define TREE_SEL_TIMER_ID	0
#define TREE_SEL_ED_ID		1
#define WATCHDOG_TIMER_ID	2

#define TREE_ICON_FOLDER	5
#define TREE_ICON_FILE		0
#define TREE_ICON_UNKNOWN	7
#define TREE_ICON_DELETED	8

// DirTreeNode->Settings
// Settings->DirTreeNode
// Settings->HTREEITEM
// HTREEITEM->Settings
#define DELETE_TREE_NODE_DATA( treeNode ) treeNode.clear( _DirTreeNode_UserData_Free )

extern bool g_RecurProps;
extern bool g_OverwriteMultiEdit;
extern int g_RecurPropDepth;
extern std::map<std::string, rflext::PropDefExt*> g_globalPropSettings;
extern std::map<std::string, bool> g_globalPropSettings_Show;
extern std::map<std::string, rflext::PropDefExt*> g_globalSettings;

rflext::OTLib AppOTLib = NULL;
std::vector<String> g_globalIgnoreTable;
static std::vector<String> g_ignoreTable;

#include "_Settings.h"
#include "_Script.h"
#include "_UI.h"
#include "_File.h"
#include "_Misc.h"
#include "_Tree.h"
#include "_Props.h"

CFilemanDlg::CFilemanDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CFilemanDlg::IDD, pParent)
	, m_SourceCode(_T(""))
	, m_LogText(_T(""))
	, m_FileSystemMonitor( NULL )
{
	m_hIcon = AfxGetApp()->LoadIcon( IDR_MAINFRAME );
	m_hAccel = NULL;
	m_TreeSelHackTimerId = -1;
	m_TreeSelUpdate = FALSE;
	m_FilterSourceUpdate = 0;
	m_Dirty = FALSE;
	m_PostLog = FALSE;
	m_FilterScript = NULL;
	m_pEdFont = NULL;
	m_ThreadExit = false;
	m_FileSystemChangedCount = 0;
	m_AutoRunScript = true;
	m_SettingsEditor = NULL;
	AppOTLib = rflext::createOTLib( "FilemanSettingDefine" );

	crim::cmInit();
	// overwrite stdio print from lua
	lua_pushcfunction( crim::_internal::_current_lua_context(), _luaB_print );
	lua_setglobal( crim::_internal::_current_lua_context(), "print" );

	crim::cmRegisterClass< Settings >( "Settings" );
	crim::cmRegisterClassMethod< Settings >( "getTypeName", &Settings::_getTypeName );
	crim::cmRegisterClassMethod< Settings >( "getBool", &Settings::_getPropVal_bool );
	crim::cmRegisterClassMethod< Settings >( "getInt", &Settings::_getPropVal_int );
	crim::cmRegisterClassMethod< Settings >( "getReal", &Settings::_getPropVal_real );
	crim::cmRegisterClassMethod< Settings >( "getStr", &Settings::_getPropVal_string );
	crim::cmRegisterClassMethod< Settings >( "hasProp", &Settings::_hasProp );
	crim::cmRegisterClassMethod< Settings >( "getName", &Settings::_getName );
	crim::cmRegisterClassMethod< Settings >( "getPath", &Settings::_getPath );
	crim::cmRegisterClassMethod< Settings >( "getPathName", &Settings::_getPathName );
	crim::cmRegisterClassMethod< Settings >( "isDir", &Settings::isDir );
	crim::cmRegisterClassMethod< Settings >( "isFile", &Settings::isFile );
	crim::cmRegisterClassMethod< Settings >( "isDir", &Settings::isDir );
	crim::cmRegisterClassMethod< Settings >( "isDeleted", &Settings::isDeleted );
	crim::cmRegisterClassMethod< Settings >( "isResolved", &Settings::isResolved );
	crim::cmRegisterClassMethod< Settings >( "isValid", &Settings::isValid );

	crim::cmRegisterGlobalMethod( "help", _helpFunc );
	crim::cmRegisterGlobalMethod( "clearLog", _clearLog );
	crim::cmRegisterGlobalMethod( "appendLog", _appendLog );
	crim::cmRegisterGlobalMethod( "autoRun", _autoRunScript );
	crim::cmRegisterGlobalMethod( "select", _select );
	crim::cmRegisterGlobalMethod( "revselect", _revselect );

	typedef void ( *fp_PrintOnConsole )( const char* msg );
	extern fp_PrintOnConsole PrintOnConsole_Hook;
	PrintOnConsole_Hook = _appendLog;
}

CFilemanDlg::~CFilemanDlg()
{
	if ( m_SettingsEditor != NULL ) {
		delete m_SettingsEditor;
		m_SettingsEditor = NULL;
	}
	CheckSaving();
	if ( m_FilterScript ) {
		crim::cmClose( m_FilterScript );
		m_FilterScript = NULL;
	}	
	KillFileSystemMonitorThread();
	crim::cmUninit();
	rflext::destroyOTLib( AppOTLib );
}

void CFilemanDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_FILE_TREE, m_FileTreeCtrl);
	DDX_Control(pDX, IDC_MFCPROPERTYGRID1, m_PropGridCtrl);
	DDX_Control(pDX, IDC_FILTER_ED, m_FilterEd);
	DDX_Control(pDX, IDC_TAB, m_Tab);
	DDX_Control(pDX, IDC_FILTER_TREE, m_FilterTreeCtrl);
	DDX_Control(pDX, IDC_FILTER_OUTPUT, m_OutputWindow);
	DDX_Text(pDX, IDC_FILTER_OUTPUT, m_LogText);
}

BEGIN_MESSAGE_MAP(CFilemanDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_COMMAND(ID_FILE_OPEN, &CFilemanDlg::OnFileOpen)
	ON_NOTIFY(TVN_SELCHANGING, IDC_FILE_TREE, &CFilemanDlg::OnTvnSelchangingFileTree)
	ON_REGISTERED_MESSAGE(AFX_WM_PROPERTY_CHANGED, &CFilemanDlg::OnPropertyChanged)
	ON_NOTIFY(TVN_SELCHANGED, IDC_FILE_TREE, &CFilemanDlg::OnTvnSelchangedFileTree)
	ON_WM_TIMER()
	ON_COMMAND(ID_FILE_SAVE, &CFilemanDlg::OnFileSave)
	ON_NOTIFY(NM_RCLICK, IDC_FILE_TREE, &CFilemanDlg::OnNMRClickFileTree)
	ON_COMMAND(ID_TREE_ITEM_RESET, &CFilemanDlg::OnTreeItemReset)
	ON_COMMAND(ID_TREERIGHTCLICK_RESETCHILDREN, &CFilemanDlg::OnTreerightclickResetchildren)
	ON_COMMAND(ID_TREERIGHTCLICK_RESETALL, &CFilemanDlg::OnTreerightclickResetall)
	ON_COMMAND(ID_TREERIGHTCLICK_APPLYTOCHILDREN, &CFilemanDlg::OnTreerightclickApplytochildren)
	ON_COMMAND(ID_TREERIGHTCLICK_APPLYALL, &CFilemanDlg::OnTreerightclickApplyall)
	ON_COMMAND(ID_TREERIGHTCLICK_MARKASRESOLVED, &CFilemanDlg::OnTreerightclickMarkasresolved)
	ON_COMMAND(ID_TREERIGHTCLICK_RESOLVEALL, &CFilemanDlg::OnTreerightclickResolveall)
	ON_COMMAND(ID_TREERIGHTCLICK_MARKASUNRESOLVED, &CFilemanDlg::OnTreerightclickMarkasunresolved)
	ON_COMMAND(ID_TREERIGHTCLICK_UNRESOLVEALL, &CFilemanDlg::OnTreerightclickUnresolveall)
	ON_COMMAND(ID_TREERIGHTCLICK_RESOLVECHILDREN, &CFilemanDlg::OnTreerightclickResolvechildren)
	ON_COMMAND(ID_TREERIGHTCLICK_UNRESOLVECHILDREN, &CFilemanDlg::OnTreerightclickUnresolvechildren)
	ON_COMMAND(ID_FILE_IMPORT, &CFilemanDlg::OnFileImport)
	ON_COMMAND(ID_TREERIGHTCLICK_DELETE, &CFilemanDlg::OnTreerightclickDelete)
	ON_COMMAND(ID_TREEEDIT_DELETE, &CFilemanDlg::OnTreeeditDelete)
	ON_COMMAND(ID_FILE_CLOSE, &CFilemanDlg::OnFileClose)
	ON_COMMAND(ID_FILE_EXIT, &CFilemanDlg::OnFileExit)
	ON_COMMAND(ID_FILE_SAVEAS, &CFilemanDlg::OnFileSaveas)
	ON_WM_CLOSE()
	ON_WM_SIZE()
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB, &CFilemanDlg::OnTcnSelchangeTab)
	ON_EN_CHANGE(IDC_FILTER_ED, &CFilemanDlg::OnEnChangeFilterEd)
	ON_COMMAND(ID_HELP_ABOUT, &CFilemanDlg::OnHelpAbout)
	ON_COMMAND(ID_FILE_LOADSCRIPT, &CFilemanDlg::OnFileLoadscript)
	ON_COMMAND(ID_RUN_SCRIPT, &CFilemanDlg::OnRunScript)
	ON_COMMAND(ID_FILE_SAVESCRIPT, &CFilemanDlg::OnFileSavescript)
	ON_COMMAND(ID_FILE_AUTORUN, &CFilemanDlg::OnFileAutorun)
	ON_UPDATE_COMMAND_UI(ID_FILE_AUTORUN, &CFilemanDlg::OnUpdateFileAutorun)
	ON_WM_INITMENUPOPUP()
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE, &CFilemanDlg::OnUpdateFileSave)
	ON_UPDATE_COMMAND_UI(ID_FILE_CLOSE, &CFilemanDlg::OnUpdateFileClose)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVEAS, &CFilemanDlg::OnUpdateFileSaveas)
	ON_NOTIFY(TVN_SELCHANGED, IDC_FILTER_TREE, &CFilemanDlg::OnTvnSelchangedFilterTree)
	ON_WM_DROPFILES()
	ON_COMMAND(ID_TOOLS_SETTINGS, &CFilemanDlg::OnToolsSettings)
	ON_COMMAND(ID_FILE_TOUCHRUN, &CFilemanDlg::OnFileTouchRun)
	ON_COMMAND(ID_TREERIGHTCLICK_OPEN, &CFilemanDlg::OnTreerightclickOpen)
	ON_COMMAND(ID_FILE_SORTFILELIST, &CFilemanDlg::OnFileSortfilelist)
END_MESSAGE_MAP()

void CFilemanDlg::OnFileAutorun()
{
	m_AutoRunScript = !m_AutoRunScript;
	if ( m_AutoRunScript ) {
		CompileRunScript();
	}
}

void CFilemanDlg::OnUpdateFileAutorun(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck( m_AutoRunScript );
}

void CFilemanDlg::OnUpdateFileSave(CCmdUI *pCmdUI)
{
	pCmdUI->Enable( !m_CurrentOpenFileName.IsEmpty() );
}

void CFilemanDlg::OnUpdateFileClose(CCmdUI *pCmdUI)
{
	pCmdUI->Enable( !m_CurrentOpenFileName.IsEmpty() || !GetCurrentOpenPathName().IsEmpty() );
}

void CFilemanDlg::OnUpdateFileSaveas(CCmdUI *pCmdUI)
{
	pCmdUI->Enable( !m_CurrentOpenFileName.IsEmpty() || !GetCurrentOpenPathName().IsEmpty() );
}

void CFilemanDlg::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu)
{
	ASSERT(pPopupMenu != NULL); 
	// Check the enabled state of various menu items. 

	CCmdUI state; 
	state.m_pMenu = pPopupMenu; 
	ASSERT(state.m_pOther == NULL); 
	ASSERT(state.m_pParentMenu == NULL); 

	// Determine if menu is popup in top-level menu and set m_pOther to 
	// it if so (m_pParentMenu == NULL indicates that it is secondary popup). 
	HMENU hParentMenu; 
	if (AfxGetThreadState()->m_hTrackingMenu == pPopupMenu->m_hMenu) 
		state.m_pParentMenu = pPopupMenu;   // Parent == child for tracking popup. 
	else if ((hParentMenu = ::GetMenu(m_hWnd)) != NULL) 
	{ 
		CWnd* pParent = this; 
		// Child windows don't have menus--need to go to the top! 
		if (pParent != NULL && 
			(hParentMenu = ::GetMenu(pParent->m_hWnd)) != NULL) 
		{ 
			int nIndexMax = ::GetMenuItemCount(hParentMenu); 
			for (int nIndex = 0; nIndex < nIndexMax; nIndex++) 
			{ 
				if (::GetSubMenu(hParentMenu, nIndex) == pPopupMenu->m_hMenu) 
				{ 
					// When popup is found, m_pParentMenu is containing menu. 
					state.m_pParentMenu = CMenu::FromHandle(hParentMenu); 
					break; 
				} 
			} 
		} 
	} 

	state.m_nIndexMax = pPopupMenu->GetMenuItemCount(); 
	for (state.m_nIndex = 0; state.m_nIndex < state.m_nIndexMax; 
		state.m_nIndex++) 
	{ 
		state.m_nID = pPopupMenu->GetMenuItemID(state.m_nIndex); 
		if (state.m_nID == 0) 
			continue; // Menu separator or invalid cmd - ignore it. 

		ASSERT(state.m_pOther == NULL); 
		ASSERT(state.m_pMenu != NULL); 
		if (state.m_nID == (UINT)-1) 
		{ 
			// Possibly a popup menu, route to first item of that popup. 
			state.m_pSubMenu = pPopupMenu->GetSubMenu(state.m_nIndex); 
			if (state.m_pSubMenu == NULL || 
				(state.m_nID = state.m_pSubMenu->GetMenuItemID(0)) == 0 || 
				state.m_nID == (UINT)-1) 
			{ 
				continue;     // First item of popup can't be routed to. 
			} 
			state.DoUpdate(this, TRUE);   // Popups are never auto disabled. 
		} 
		else 
		{ 
			// Normal menu item. 
			// Auto enable/disable if frame window has m_bAutoMenuEnable 
			// set and command is _not_ a system command. 
			state.m_pSubMenu = NULL; 
			state.DoUpdate(this, FALSE); 
		} 

		// Adjust for menu deletions and additions. 
		UINT nCount = pPopupMenu->GetMenuItemCount(); 
		if (nCount < state.m_nIndexMax) 
		{ 
			state.m_nIndex -= (state.m_nIndexMax - nCount); 
			while (state.m_nIndex < nCount && 
				pPopupMenu->GetMenuItemID(state.m_nIndex) == state.m_nID) 
			{ 
				state.m_nIndex++; 
			} 
		} 
		state.m_nIndexMax = nCount; 
	} 
}

BOOL CFilemanDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	GetWindowText( m_TitleName );
	
	UINT uiBmpId = IDB_IMAGE_LIST;
	CBitmap bmp;
	HANDLE hbmp = LoadImage( ::AfxGetInstanceHandle(), "res\\ImageList.bmp", IMAGE_BITMAP,
		0, 0, LR_LOADFROMFILE );
	if ( hbmp ) {
		bmp.Attach( hbmp );
	} else {
		bmp.LoadBitmap( uiBmpId );
	}
	if ( bmp.GetSafeHandle() ) {
		BITMAP bmpObj;
		bmp.GetBitmap( &bmpObj );
		UINT nFlags = ILC_MASK;
		nFlags |= ILC_COLOR24;
		m_ImageList.Create( 16, bmpObj.bmHeight, nFlags, 0, 0 );
		m_ImageList.Add( &bmp, RGB( 192, 192, 192 ) );
		m_FileTreeCtrl.SetImageList( &m_ImageList, TVSIL_NORMAL );
		m_FilterTreeCtrl.SetImageList( &m_ImageList, TVSIL_NORMAL );
	}
	m_hAccel = LoadAccelerators( AfxGetInstanceHandle(), MAKEINTRESOURCE( IDR_ACCELERATOR ) );

	m_FileTreeCtrl.SetMultiSelect( TRUE );
	TCHAR fullPath[ MAX_PATH ] = { 0 };
	GetModuleFileName( GetModuleHandle( NULL ), fullPath, MAX_PATH );
	String fileName, exePath;
	StringUtil::splitFilename( fullPath, fileName, exePath );
	auto path = exePath + FilemanSettingDefineFile;
	if ( !Misc::isFile( path.c_str() ) ) {
		GetCurrentDirectory( MAX_PATH, fullPath );
		path = StringUtil::standardisePath( fullPath ) + FilemanSettingDefineFile;
	}
	rflext::loadOTLibFromXML( AppOTLib, path.c_str() );

	auto gIgnoreListPath = exePath + GlobalIgnoreList;
	if ( !Misc::isFile( gIgnoreListPath.c_str() ) ) {
		GetCurrentDirectory( MAX_PATH, fullPath );
		gIgnoreListPath = StringUtil::standardisePath( fullPath ) + GlobalIgnoreList;
	}
	g_globalIgnoreTable.clear();
	if ( Misc::isFile( gIgnoreListPath.c_str() ) ) {
		Misc::ByteBuffer buf;
		Misc::readFileAll( gIgnoreListPath.c_str(), buf );
		if ( !buf.empty() ) {
			buf.push_back( '\0' );
			String content = String( &*buf.begin() );
			std::vector<String> segs;
			StringUtil::split( segs, content, ";,\t\n\r" );
			for ( auto s : segs ) {
				if ( !s.empty() ) {
					StringUtil::toLowerCase( s );
					g_globalIgnoreTable.push_back( s );
				}
			}
		}
	}

	m_PropGridCtrl.EnableHeaderCtrl( TRUE );
	m_PropGridCtrl.EnableDescriptionArea();
	m_PropGridCtrl.SetVSDotNetLook();
	m_PropGridCtrl.MarkModifiedProperties();
	RECT rt;
	m_PropGridCtrl.GetClientRect( &rt );
	m_PropGridCtrl.SendMessage( WM_SIZE, 0, MAKELPARAM( rt.right, rt.bottom ) );
	m_Tab.InsertItem( 0, "Editing" );
	m_Tab.InsertItem( 1, "Matching" );	
	m_Tab.SetCurSel( 0 );
	m_Tab.SetCurFocus( 0 );
	UpdateTab();
	{
		CString file = theApp.GetProfileStringA( "Fileman", "LastOpenedFile" );
		CString path = theApp.GetProfileStringA( "Fileman", "LastOpenedPath" );
		CString _rt = theApp.GetProfileStringA( "Fileman", "WindowRect" );
		CString source = theApp.GetProfileStringA( "Fileman", "FilterSource" );
		m_AutoRunScript = !!theApp.GetProfileIntA( "Fileman", "AutoRunScript", 0 );
		bool windowSized = false;
		if ( !_rt.IsEmpty() ) {
			CRect rt;
			sscanf_s( _rt.GetBuffer( 0 ), "%d,%d,%d,%d", &rt.left, &rt.top, &rt.right, &rt.bottom );
			if ( rt.Height() > 0 && rt.Width() > 0 ) {
				MoveWindow( rt.left, rt.top, rt.right - rt.left, rt.bottom - rt.top );
				windowSized = true;
			}
		}
		if ( windowSized ) {
			// force window update
			CRect rt;
			GetClientRect( rt );
			PostMessage( WM_SIZE, 0, ( rt.Height() << 16 ) | rt.Width() );
		}

		if ( file.GetLength() > 0 && Misc::isFile( file ) ) {
			OpenFile( file );
		} else if ( path.GetLength() > 0 && Misc::isDirectory( path ) ) {
			CString text( m_TitleName );
			text.Append( " " );
			text.Append( path );
			SetWindowText( text );
			SetCurrentOpenPathName( path );
			SearchFileFromRoot( path.GetBuffer(0), m_FileSettingTree );
			CreateTree( m_FileSettingTree, m_FileTreeCtrl );
			ResetTree( m_FileSettingTree, m_FileTreeCtrl, m_PropGridCtrl );
			if ( m_FileTreeCtrl.GetRootItem() ) {
				m_FileTreeCtrl.Expand( m_FileTreeCtrl.GetRootItem(), TVM_EXPAND | TVE_EXPANDPARTIAL );
			}
		}	
		if ( !source.IsEmpty() ) {
			m_FilterEd.SetWindowText( source );
			CompileRunScript( !m_AutoRunScript );
		}
	}
	SetTimer( TREE_SEL_ED_ID, TREE_ED_INTERVAL, NULL );
	SetTimer( WATCHDOG_TIMER_ID, FILESYSTEM_WATCHDOG_INTERVAL, NULL );

	m_pEdFont = new CFont();
	m_pEdFont->CreateFont( 18, 0, 0, 0, FW_NORMAL, false, false,
		0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		FIXED_PITCH | FF_MODERN, _T( "Consolas" ) );
	m_FilterEd.SetFont( m_pEdFont );
	m_OutputWindow.SetFont( m_pEdFont );
	ShowWindow( SW_HIDE );
	return TRUE;
}

void CFilemanDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

void CFilemanDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

HCURSOR CFilemanDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CFilemanDlg::DestroyWindow()
{
	if ( m_SettingsEditor != NULL ) {
		m_SettingsEditor->DestroyWindow();
		delete m_SettingsEditor;
		m_SettingsEditor = NULL;
	}
	CRect rt;
	theApp.WriteProfileStringA( "Fileman", "LastOpenedFile", m_CurrentOpenFileName );
	theApp.WriteProfileStringA( "Fileman", "LastOpenedPath", GetCurrentOpenPathName() );
	theApp.WriteProfileStringA( "Fileman", "WindowRect", "" );
	theApp.WriteProfileInt( "Fileman", "AutoRunScript", m_AutoRunScript );
	if ( ::IsWindow( GetSafeHwnd() ) ) {
		if ( IsZoomed() ) {
			rt.left = rt.top = 0;
			rt.right = ::GetSystemMetrics( SM_CXSCREEN );
			rt.bottom = ::GetSystemMetrics( SM_CYSCREEN );
		} else {
			GetWindowRect( rt );
		}
		if ( !IsIconic() ) {
			theApp.WriteProfileStringA( "Fileman", "WindowRect",
				[&rt](){
					CString s;
					s.Format( "%d,%d,%d,%d", rt.left, rt.top, rt.right, rt.bottom );
					return s;
				}()
			);
		}
	}
	m_FilterEd.GetWindowText( m_SourceCode );
	if ( !m_SourceCode.IsEmpty() ) {
		theApp.WriteProfileStringA( "Fileman", "FilterSource", m_SourceCode );
	}
	KillTimer( WATCHDOG_TIMER_ID );
	KillTimer( TREE_SEL_ED_ID );
	if ( m_TreeSelHackTimerId != -1 ) {
		KillTimer( m_TreeSelHackTimerId );
		m_TreeSelHackTimerId = -1;
	}
	KillFileSystemMonitorThread();
	if ( m_hAccel ) {
		::DestroyAcceleratorTable( m_hAccel );
		m_hAccel = NULL;
	}
	m_FileSettingTree.clear( _DirTreeNode_UserData_Free );
	if ( m_pEdFont ) {
		delete m_pEdFont;
		m_pEdFont = NULL;
	}
	return CDialogEx::DestroyWindow();
}

BOOL CFilemanDlg::PreTranslateMessage(MSG* pMsg)
{
	if ( pMsg->message == WM_KEYDOWN ) {
		switch ( pMsg->wParam ) {
		case VK_TAB:
			{
				if ( GetAsyncKeyState( VK_CONTROL ) & 0x8000 ) {
					int index = ( m_Tab.GetCurSel() + 1 ) % m_Tab.GetItemCount();
					m_Tab.SetCurFocus( index );
					m_Tab.SetCurSel(  index );
					return TRUE;
				}
			}
			break;
		}
	}
	if ( m_hAccel ) {   
		if ( ::TranslateAccelerator( m_hWnd, m_hAccel, pMsg ) ) {   
			return TRUE;   
		}   
	}
	return CDialogEx::PreTranslateMessage(pMsg);
}

void CFilemanDlg::NewProject( const CString& dirName )
{	
	CheckSaving();
	m_Dirty = TRUE;
	m_TreeSelUpdate = FALSE;
	String path = dirName;
	CString text( m_TitleName );
	text.Append( " " );
	text.Append( path.c_str() );
	SetWindowText( text );
	SetCurrentOpenPathName( path.c_str() );
	SearchFileFromRoot( path, m_FileSettingTree );
	m_FileSettingTree.sort();
	CreateTree( m_FileSettingTree, m_FileTreeCtrl );
	ResetTree( m_FileSettingTree, m_FileTreeCtrl, m_PropGridCtrl );
	if ( m_FileTreeCtrl.GetRootItem() ) {
		m_FileTreeCtrl.Expand( m_FileTreeCtrl.GetRootItem(), TVM_EXPAND | TVE_EXPANDPARTIAL );
	}
	FilterTree( m_FileSettingTree );
}

void CFilemanDlg::OnFileImport()
{	
	CheckSaving();
	m_Dirty = TRUE;
	m_TreeSelUpdate = FALSE;
	TCHAR path[ MAX_PATH ] = { 0 };
	if ( !WinUtil::getPathDlg( path, GetSafeHwnd(), TEXT( "Select the root directory" ) ) ) {
		return;
	}
	if ( !Misc::isDirectory( path ) ) {
		return;
	}
	CString text( m_TitleName );
	text.Append( " " );
	text.Append( path );
	SetWindowText( text );
	SetCurrentOpenPathName( path );
	SearchFileFromRoot( path, m_FileSettingTree );
	m_FileSettingTree.sort();
	CreateTree( m_FileSettingTree, m_FileTreeCtrl );
	ResetTree( m_FileSettingTree, m_FileTreeCtrl, m_PropGridCtrl );
	if ( m_FileTreeCtrl.GetRootItem() ) {
		m_FileTreeCtrl.Expand( m_FileTreeCtrl.GetRootItem(), TVM_EXPAND | TVE_EXPANDPARTIAL );
	}
	FilterTree( m_FileSettingTree );
	UpdateTreeItemStateAll();
}

CString CFilemanDlg::GetCurrentOpenPathName()
{
	lock_guard< mutex > guard( m_CurrentOpenPathNameMutex );
	return  m_CurrentOpenPathName;
}

CString CFilemanDlg::GetCurrentOpenPathRootName()
{
	lock_guard< mutex > guard( m_CurrentOpenPathNameMutex );
	return  m_CurrentOpenPathRootName;
}

void CFilemanDlg::KillFileSystemMonitorThread()
{
	if ( m_FileSystemMonitor ) {
		m_ThreadExit = true;
		if ( m_FileSystemMonitor->joinable() ) {
			m_FileSystemMonitor->join();
		}
		delete m_FileSystemMonitor;
		m_FileSystemMonitor = NULL;
	}
}

void CFilemanDlg::SetCurrentOpenPathName( const CString& path )
{
	lock_guard< mutex > guard( m_CurrentOpenPathNameMutex );
	if ( Misc::isDirectory( path ) ) {
		if ( m_CurrentOpenPathName != path || m_FileSystemMonitor == NULL ) {
			m_CurrentOpenPathName = path;
			m_CurrentOpenPathRootName = path;
			String _path( path );
			auto last = _path.length();
			char x = _path[last - 1];
			if ( _path[last - 1] == '/' || _path[last - 1] == '\\' ) {
				_path = _path.substr( 0, last - 1 );
			}
			String baseName, ext, basepath;
			StringUtil::splitFullFilename( _path, baseName, ext, basepath ); 
			m_CurrentOpenPathName = path;
			m_ThreadExit = false;
			m_FileSystemMonitor = new thread(
				[]( void* __this ) {
					CFilemanDlg* _this = ( CFilemanDlg* )__this;
					HANDLE hEvent = INVALID_HANDLE_VALUE;
					{
						CString path = _this->GetCurrentOpenPathName();
						hEvent = FindFirstChangeNotification( path, TRUE, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME );
					}
					if ( hEvent == INVALID_HANDLE_VALUE ) {
						return;
					}
					while ( !_this->m_ThreadExit ) {
						DWORD nObj = WaitForSingleObject( hEvent, 1000 );
						if ( nObj == WAIT_TIMEOUT ) {
							continue;
						} else if ( nObj == WAIT_OBJECT_0 ) {
							InterlockedIncrement( &_this->m_FileSystemChangedCount );
						}
						if ( FALSE == FindNextChangeNotification( hEvent ) ) {
							break;
						}
						sleep_for( milliseconds( 200 ) );
					}
					FindCloseChangeNotification( hEvent );
				},
				this
			);
		}
	} else {
		m_CurrentOpenPathName.Empty();
		KillFileSystemMonitorThread();
	}
}

void CFilemanDlg::OpenFile( const CString& _filePath )
{
	m_TreeSelUpdate = FALSE;
	String fileName = _filePath;
	String baseName, ext;
	std::replace( fileName.begin(), fileName.end(), '\\', '/' );
	StringUtil::splitBaseFilename( fileName, baseName, ext );
	if ( !Misc::isDirectory( baseName.c_str() ) ) {
		if ( IDYES == AfxMessageBox( "There is no associated directory beside this xml file! \r\n\
									 Do you want do open another one?", MB_YESNO | MB_ICONQUESTION ) ) {
			TCHAR path[ MAX_PATH ] = { 0 };
			if ( !WinUtil::getPathDlg( path, GetSafeHwnd(), TEXT( "Select the root directory" ) ) ) {
				return;
			}
			baseName = path;
			std::replace( baseName.begin(), baseName.end(), '\\', '/' );
		} else {
			return;
		}
	}
	// search directory
	WinUtil::DirTreeNode dst;
	SearchFileFromRoot( baseName, dst );
	// load data
	WinUtil::DirTreeNode data;
	if ( LoadDirTreeFromXML( _filePath, data ) ) {
		// merge data with real directory content
		WinUtil::DirTreeNode out;
		if ( MergeDir( out, data, dst ) ) {
			m_CurrentOpenFileName = _filePath;
			SetCurrentOpenPathName( baseName.c_str() );
			CString text( m_TitleName );
			text.Append( " " );
			text.Append( m_CurrentOpenFileName );
			SetWindowText( text );

			m_FileSettingTree.swap( out );
			m_FileSettingTree.sort();
			UpdateTree( m_FileSettingTree, m_FileTreeCtrl );
			UpdateTreeItemStateAll();
		} else {
			AppendLogText( "Merge xml with files failed!" );
		}
		DELETE_TREE_NODE_DATA( out );
	}
	DELETE_TREE_NODE_DATA( dst );
	DELETE_TREE_NODE_DATA( data );
}

void CFilemanDlg::OnFileOpen()
{
	CheckSaving();
	CFileDialog fd( TRUE, "xml", NULL, OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_FILEMUSTEXIST, "XML Files (*.xml)|*.xml|All Files (*.*)|*.*||" );
	fd.DoModal();
	if ( fd.GetPathName().GetLength() > 0 ) {
		OpenFile( fd.GetPathName() );
		FilterTree( m_FileSettingTree );
	}
}

void CFilemanDlg::MarkDirty( BOOL flag )
{
	CString text( m_TitleName );
	text.Append( " " );
	if ( !m_CurrentOpenFileName.IsEmpty() ) {
		text.Append( m_CurrentOpenFileName );
	} else {
		CString path = GetCurrentOpenPathName();
		if ( !path.IsEmpty() ) {
			text.Append( path );
		}
	}
	if ( !flag ) {
		m_Dirty = FALSE;
	} else {
		m_Dirty = TRUE;
		text.Append( "*" );
	}
	if ( ::IsWindow( GetSafeHwnd() ) ) {
		SetWindowText( text );
	}
}

void CFilemanDlg::OnFileClose()
{
	CheckSaving();
	m_CurrentOpenFileName.Empty();
	SetCurrentOpenPathName("");
	DELETE_TREE_NODE_DATA( m_FileSettingTree );
	m_FileTreeCtrl.DeleteAllItems();
	MarkDirty( FALSE );
}

void CFilemanDlg::OnFileExit()
{
	PostMessage( WM_CLOSE, 0, 0 );
}

void CFilemanDlg::CheckSaving()
{
	if ( m_Dirty && m_CurrentOpenFileName.GetLength() > 0 ) {
		if ( !IsWindow( GetSafeHwnd() ) || IDYES == AfxMessageBox( "Need save?", MB_YESNO ) ) {
			SaveDirTreeNodeToXML( m_CurrentOpenFileName, m_FileSettingTree );
		}
	}
	MarkDirty( FALSE );
}

void CFilemanDlg::OnFileSave()
{
	if ( m_Dirty ) {
		if ( m_CurrentOpenFileName.GetLength() > 0 ) {
			SaveDirTreeNodeToXML( m_CurrentOpenFileName, m_FileSettingTree );
		} else if ( GetCurrentOpenPathName().GetLength() > 0 ) {
			CString name( GetCurrentOpenPathName() );
			name.Append( ".xml" );
			if ( Misc::isFile( name ) ) {
				if ( IDNO == AfxMessageBox( "Do you really want to overwrite the exist one?", MB_YESNO ) ) {
					return;
				}
			}
			m_CurrentOpenFileName = name;
			SaveDirTreeNodeToXML( name, m_FileSettingTree );
		}
		MarkDirty( FALSE );
	}
}

void CFilemanDlg::OnFileSaveas()
{
	if ( m_FileSettingTree.curDir.empty() ) {
		return;
	}
	CFileDialog fd( FALSE, "xml", NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "XML Files (*.xml)|*.xml|All Files (*.*)|*.*||" );
	fd.DoModal();
	if ( !fd.GetPathName().IsEmpty() ) {
		if ( SaveDirTreeNodeToXML( fd.GetPathName(), m_FileSettingTree ) ) {
			if ( m_CurrentOpenFileName.IsEmpty() ) {
				m_CurrentOpenFileName = fd.GetPathName();
			}
			MarkDirty( FALSE );
		}
	}
}

void CFilemanDlg::OnClose()
{
	CheckSaving();
	CDialogEx::OnClose();
}

void CFilemanDlg::OnOK()
{
}

void CFilemanDlg::OnSize(UINT nType, int cx, int cy)
{
	CRect rt;
	GetClientRect( rt );
	double totalWidth = rt.right - 15;
	double totalHeight = rt.bottom - 10;
	double totalHeight2 = rt.bottom - 15;
	double treeWidth = totalWidth * ( 1 - 0.618 );
	double treeHeight = totalHeight;
	double propGridWidth = totalWidth * 0.618;
	double propGridHeight = totalHeight2 * 0.4;
	double edHeight = totalHeight2 * ( 1 - 0.4 );
	if ( ::IsWindow( m_FileTreeCtrl.GetSafeHwnd() ) ) {
		static bool firstIn = true;
		if ( firstIn ) {
			firstIn = false;
			ShowWindow( SW_SHOW );
		}
		m_Tab.MoveWindow( 5, 5, (int)treeWidth, (int)treeHeight );
		CRect yoff;
		m_Tab.GetItemRect( 0, &yoff );
		CRect bounder;
		m_Tab.AdjustRect( FALSE, &bounder );
		bounder.top += yoff.bottom;
		m_Tab.GetClientRect( rt );
		CRect innerRt( rt );
		innerRt.left += bounder.left;
		innerRt.top += bounder.top;
		innerRt.right += bounder.right;
		innerRt.bottom += bounder.bottom;
		m_FileTreeCtrl.MoveWindow( 5 + bounder.left, 5 + bounder.top, innerRt.Width(), innerRt.Height() );
		m_FilterTreeCtrl.MoveWindow( 5 + bounder.left, 5 + bounder.top, innerRt.Width(), innerRt.Height() );
	}
	if ( ::IsWindow( m_PropGridCtrl.GetSafeHwnd() ) ) {
		m_PropGridCtrl.MoveWindow( (int)treeWidth + 10, 5, (int)propGridWidth, (int)propGridHeight );
	}
	double edUpperHeight = ( edHeight - 5 ) * 0.618;
	double edLowerHeight = ( edHeight - 5 ) * ( 1 - 0.618 );
	if ( ::IsWindow( m_FilterEd.GetSafeHwnd() ) ) {
		m_FilterEd.MoveWindow( (int)treeWidth + 10, (int)propGridHeight + 10, (int)propGridWidth, (int)edUpperHeight );
	}
	if ( ::IsWindow( m_OutputWindow.GetSafeHwnd() ) ) {
		m_OutputWindow.MoveWindow( (int)treeWidth + 10, (int)( propGridHeight + 15 + edUpperHeight ), (int)propGridWidth, (int)edLowerHeight );
	}
	CDialogEx::OnSize(nType, cx, cy);
}

void CFilemanDlg::UpdateTab()
{	
	switch ( m_Tab.GetCurSel() ) {
	case 0:
		m_FileTreeCtrl.ShowWindow( SW_SHOW );
		m_FilterTreeCtrl.ShowWindow( SW_HIDE );
		break;
	case 1:
		m_FileTreeCtrl.ShowWindow( SW_HIDE );
		m_FilterTreeCtrl.ShowWindow( SW_SHOW );
		break;
	}
}

void CFilemanDlg::OnTcnSelchangeTab(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;
	UpdateTab();
}

void CFilemanDlg::OnTimer( UINT_PTR nIDEvent )
{
	if ( m_TreeSelHackTimerId == nIDEvent ) {
		KillTimer( m_TreeSelHackTimerId );
		m_TreeSelHackTimerId = -1;
		if ( m_TreeSelUpdate ) {
			m_TreeSelUpdate = FALSE;
			UpdatePropertiesFromTreeSelection();
		}
	} else if ( TREE_SEL_ED_ID == nIDEvent ) {
		if ( m_FilterSourceUpdate != -10000 ) {
			m_FilterSourceUpdate -= TREE_ED_INTERVAL;
			if ( m_FilterSourceUpdate < 0 ) {
				CompileRunScript( !m_AutoRunScript );
			}
		}
		crim::cmRun( TREE_ED_INTERVAL );
	} else if ( WATCHDOG_TIMER_ID == nIDEvent ) {
		if ( m_FileSystemChangedCount > 0 ) {
			HandleDirectoryChanged();
			m_FileSystemChangedCount = 0;
		}
	}
	CDialogEx::OnTimer( nIDEvent );
}

void CFilemanDlg::HandleDirectoryChanged()
{
	static bool isHandling = false;
	if ( isHandling ) {
		return;
	}
	isHandling = true;
	CString path = GetCurrentOpenPathName();
	if ( Misc::isDirectory( path ) ) {
		if ( IDYES == AfxMessageBox( "Workding directory content has been modified, do you want do update it now?", MB_YESNO | MB_ICONASTERISK ) ) {
			m_TreeSelUpdate = FALSE;
			// search directory
			WinUtil::DirTreeNode dst;
			SearchFileFromRoot( path.GetBuffer( 0 ), dst );
			// merge data with real directory content
			WinUtil::DirTreeNode out;
			if ( MergeDir( out, m_FileSettingTree, dst ) ) {
				m_FileSettingTree.swap( out );	
				m_FileSettingTree.sort();
				UpdateTree( m_FileSettingTree, m_FileTreeCtrl );
				UpdateTreeItemStateAll();
			} else {
				AfxMessageBox( "Update directory failed!", MB_ICONERROR );
			}
			DELETE_TREE_NODE_DATA( out );
			DELETE_TREE_NODE_DATA( dst );
		}
	} else {
		// you program should never get here because of the monitor holds this directory handle.
		AfxMessageBox( "Working directory is missing, you can't update directory now." );
	}
	isHandling = false;
}

void CFilemanDlg::OnDropFiles(HDROP hDropInfo)
{
	struct CursorGuard {
		CursorGuard( CWnd* w ) : wnd( w ) { wnd->BeginWaitCursor(); }
		~CursorGuard() { wnd->EndWaitCursor(); }
		CWnd* wnd;
	};
	CursorGuard _c( this );
	int fileNum = DragQueryFile( hDropInfo, 0xFFFFFFFF, NULL, 0 );
	for ( int i = 0; i < fileNum; ++i ) {
		char fileName[ MAX_PATH ] = { 0 };
		DragQueryFile( hDropInfo, i, fileName, MAX_PATH );
		if ( Misc::isFile( fileName ) ) {
			String fn( fileName );
			String baseName, ext, path;
			std::transform( ext.begin(), ext.end(), ext.begin(), ::tolower );
			StringUtil::splitFullFilename( fn, baseName, ext, path );
			if ( ext == "lua" ) {
				Misc::ByteBuffer buf;
				if ( Misc::readFileAll( fileName, buf ) ) {
					buf.push_back( '\0' );
					m_SourceCode = &*buf.begin();
					m_FilterEd.SetWindowText( &*buf.begin() );
					m_FilterEd.SetSel( 0, 0 );
				}
			} else if ( ext == "xml" ) {
				OpenFile( fileName );
			} else {
				continue;
			}
		} else if ( Misc::isDirectory( fileName ) ) {
			NewProject( fileName );
		} else {
			continue;
		}
		break;
	}	
	CDialogEx::OnDropFiles( hDropInfo );
}

void CFilemanDlg::OnToolsSettings()
{
	if ( m_SettingsEditor == NULL ) {
		m_SettingsEditor = new EditorSettings();
		m_SettingsEditor->Create( IDD_EDITORSETTINGS, GetDesktopWindow() );
		m_SettingsEditor->ShowWindow( SW_SHOW );
	} else {
		if ( m_SettingsEditor->IsIconic() ) {
			m_SettingsEditor->ShowWindow( SW_NORMAL );
		} else {
			if ( m_SettingsEditor->IsWindowVisible() ) {
				m_SettingsEditor->ShowWindow( SW_HIDE );
			} else {
				m_SettingsEditor->ShowWindow( SW_SHOW );
			}
		}
	}
}


void CFilemanDlg::OnFileSortfilelist()
{
	_TreeSortAllRecur( m_FileTreeCtrl, m_FileTreeCtrl.GetRootItem() );
}
