
// FilemanDlg.h : header file
//

#pragma once
#include "afxcmn.h"
#include "utils\WinUtil.h"
#include "utils\StringUtil.h"
#include "MltiTree.h"
#include <string>
#include <vector>
#include <algorithm>
#include "afxpropertygridctrl.h"
#include "afxwin.h"
#include "EditorSettings.h"

#define USE_STD_THREAD

#ifdef USE_STD_THREAD
	#include <thread>
	#include <mutex>
	using std::thread;
	using std::mutex;
	using std::lock_guard;
	using namespace std::this_thread;
	using namespace std::chrono;
#else
	#include "tinyThread\tinythread.h"
	using tthread::thread;
	using tthread::mutex;
	using tthread::lock_guard;
	using namespace tthread::this_thread;
	using namespace tthread::chrono;
#endif



class Settings;
namespace crim
{
	struct _CMScript;
	typedef _CMScript* CMScript;
}
namespace lua_tinker
{
	struct table;
}

class CMyEdit : public CEdit
{

public:
	virtual BOOL PreTranslateMessage( MSG* pMsg );
	DECLARE_MESSAGE_MAP()
	afx_msg void OnDropFiles(HDROP hDropInfo);
};

class CFilemanDlg : public CDialogEx
{
// Construction
public:
	CFilemanDlg(CWnd* pParent = NULL);	// standard constructor
	~CFilemanDlg();

// Dialog Data
	enum { IDD = IDD_FILEMAN_DIALOG };
	void ClearLogText();
	void CompileRunScript( bool compileOnly = false, bool switchTab = false );
	void AppendLogText( const CString& text );
	void PushLogText( const CString& text );
	void AutoRunScript( bool enable = true );
	void Select( lua_tinker::table& pathList );
	void ReverseSelect( lua_tinker::table& pathList );
	void UpdateTree();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
private:
	std::vector< Settings* > GetCurrentSelSettingItems();
	void UpdateTreeItemState( Settings* item );
	void UpdateTreeItemState( const std::vector< Settings* >& items );
	void UpdateTreeItemStateAll();
	void UpdatePropertiesFromTreeSelection();
	void MarkDirty( BOOL flag = TRUE );
	void CheckSaving();
	void OpenFile( const CString& fileName );
	void NewProject( const CString& dirName );
	void FilterTree( const WinUtil::DirTreeNode& data );
	void UpdateTab();
	CString GetCurrentOpenPathName();
	CString GetCurrentOpenPathRootName();
	void SetCurrentOpenPathName( const CString& path );
	void KillFileSystemMonitorThread();
	void HandleDirectoryChanged();
private:
	static BOOL	SearchFileFromRoot( const String& rootPath, WinUtil::DirTreeNode& out, const String* ignoreList = NULL );
	static BOOL	CreateTree( WinUtil::DirTreeNode& data, CTreeCtrl& tree );
	static BOOL UpdateTree( WinUtil::DirTreeNode& data, CTreeCtrl& tree );
	static BOOL	ResetTree( WinUtil::DirTreeNode& data, CTreeCtrl& tree, CMFCPropertyGridCtrl& ui );
	static BOOL MergeDir( WinUtil::DirTreeNode& out, const WinUtil::DirTreeNode& src, const WinUtil::DirTreeNode& dst );
	static BOOL SaveDirTreeNodeToXML( const char* fileName, const WinUtil::DirTreeNode& root );
	static BOOL LoadDirTreeFromXML( const char* fileName, WinUtil::DirTreeNode& root );
private:
	static void UpdateProperty( const Settings& settings, CMFCPropertyGridCtrl& control,
		std::vector< String >* excludePropNames = NULL,
		std::vector< String >* memberOwnerNames = NULL );
// Implementation
protected:
	HICON					m_hIcon;
	HACCEL					m_hAccel;
	CFont*					m_pEdFont;
	CMultiTree				m_FileTreeCtrl;
	CMultiTree				m_FilterTreeCtrl;
	WinUtil::DirTreeNode	m_FileSettingTree;
	CMFCPropertyGridCtrl	m_PropGridCtrl;
	CImageList				m_ImageList;
	CString					m_CurrentOpenFileName;
	CString					m_TitleName;
	UINT					m_TreeSelHackTimerId;
	BOOL					m_TreeSelUpdate;
	int						m_FilterSourceUpdate;
	BOOL					m_Dirty;
	BOOL					m_PostLog;
	CMyEdit					m_FilterEd;
	CEdit					m_OutputWindow;
	CTabCtrl				m_Tab;
	CString					m_SourceCode;
	CString					m_LogText;
	crim::CMScript			m_FilterScript;
	bool					m_AutoRunScript;
	thread*					m_FileSystemMonitor;
	volatile bool			m_ThreadExit;
	volatile long			m_FileSystemChangedCount;
	CString					m_CurrentOpenPathName;
	CString					m_CurrentOpenPathRootName;
	mutex					m_CurrentOpenPathNameMutex;
	EditorSettings*			m_SettingsEditor;
protected:
	// Generated message map functions
	virtual BOOL OnInitDialog();
	virtual BOOL DestroyWindow();
	virtual BOOL PreTranslateMessage( MSG* pMsg );
	virtual void OnOK();

	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:	
	afx_msg void OnSysCommand( UINT nID, LPARAM lParam );
	afx_msg void OnPaint();
	afx_msg void OnFileOpen();
	afx_msg void OnTvnSelchangingFileTree( NMHDR* pNMHDR, LRESULT* pResult );
	afx_msg LRESULT OnPropertyChanged( WPARAM wParam, LPARAM lParam );
	afx_msg void OnTvnSelchangedFileTree(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnFileSave();
	afx_msg void OnNMRClickFileTree(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTreeItemReset();
	afx_msg void OnTreerightclickResetchildren();
	afx_msg void OnTreerightclickResetall();
	afx_msg void OnTreerightclickApplytochildren();
	afx_msg void OnTreerightclickApplyall();
	afx_msg void OnTreerightclickMarkasresolved();
	afx_msg void OnTreerightclickResolveall();
	afx_msg void OnTreerightclickMarkasunresolved();
	afx_msg void OnTreerightclickUnresolveall();
	afx_msg void OnTreerightclickResolvechildren();
	afx_msg void OnTreerightclickUnresolvechildren();
	afx_msg void OnFileImport();
	afx_msg void OnTreerightclickDelete();
	afx_msg void OnTreeeditDelete();
	afx_msg void OnFileClose();
	afx_msg void OnFileExit();
	afx_msg void OnFileSaveas();
	afx_msg void OnClose();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnTcnSelchangeTab(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnEnChangeFilterEd();
	afx_msg void OnHelpAbout();
	afx_msg void OnFileLoadscript();
	afx_msg void OnRunScript();
	afx_msg void OnFileSavescript();
	afx_msg void OnFileAutorun();
	afx_msg void OnUpdateFileAutorun(CCmdUI *pCmdUI);
	afx_msg void OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);
	afx_msg void OnUpdateFileSave(CCmdUI *pCmdUI);
	afx_msg void OnUpdateFileClose(CCmdUI *pCmdUI);
	afx_msg void OnUpdateFileSaveas(CCmdUI *pCmdUI);
	afx_msg void OnTvnSelchangedFilterTree(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg void OnToolsSettings();
	afx_msg void OnFileTouchRun();
	afx_msg void OnTreerightclickOpen();
	afx_msg void OnFileSortfilelist();
};
