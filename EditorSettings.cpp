// EditorSettings.cpp : implementation file
//

#include "stdafx.h"
#include "Fileman.h"
#include "EditorSettings.h"
#include "afxdialogex.h"
#include "FilemanDlg.h"
#include <vector>
#include <memory>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utils\Rflext.h"
const int DefaultRecurPropDepth = -1;

bool g_RecurProps = false;
bool g_AddModify = false;
bool g_OverwriteMultiEdit = false;
int g_RecurPropDepth = -1;
std::map<std::string, rflext::PropDefExt*> g_globalPropSettings;
std::map<std::string, rflext::PropDefExt*> g_globalSettings;
std::map<std::string, bool> g_globalPropSettings_Show;

typedef std::function< void ( const rflx::ValueData&, CMFCPropertyGridProperty* ui )> PropChangedCallback;

static void _bindvc( rflext::PropDefExt& propDef, PropChangedCallback callback ) {
	auto f = new PropChangedCallback( callback );
	propDef.editorData = ( const char* )f;
}

static void _onValueChanged( rflext::PropDefExt& propDef, const rflx::ValueData& newVal, CMFCPropertyGridProperty* ui ) {
	auto f = ( PropChangedCallback* )propDef.editorData;
	( *f )( newVal, ui );
}

static void _appendLog( const char* s ) {
	if ( !s || *s == '\0' ) {
		return;
	}
	( ( CFilemanDlg* )theApp.GetMainWnd() )->AppendLogText( s );
}

static void _updateTree() {
	( ( CFilemanDlg* )theApp.GetMainWnd() )->UpdateTree();
}

static void _log( const CString& str ) {
	( ( CFilemanDlg* )theApp.GetMainWnd() )->AppendLogText( str );
}

template< typename T >
static void _setval( CMFCPropertyGridProperty* ui, T val ) {
	_variant_t src = val;
	_variant_t dst;
	VariantChangeType( &dst, &src, 0, ui->GetValue().vt );
	ui->SetValue( dst );
}

extern std::vector< rflext::PropDefExt* > _getAllPropDefsFromLib();
extern std::vector<String> g_globalIgnoreTable;
extern rflx::ValueData _convert_COleVariant( const COleVariant& val );
extern  CMFCPropertyGridProperty* _createMFCPropFromType(
	const char* name, rflx::ValueDataType type, const rflx::ValueData* value = NULL,
	const char* desc = NULL, const char* ed = NULL );

class PropSettingItem : public rflx::RflxObject
{
public:
	RFLX_IMP_BASE_CLASS( PropSettingItem );
	static std::vector< PropSettingItem* > settings;
	PropSettingItem() : propDef( NULL ){}
	virtual ~PropSettingItem() {}
	void bindValue( rflext::PropDefExt* value ) { propDef = value; }
public:
	virtual void onValueChanged( const rflx::ValueData& newValue, CMFCPropertyGridProperty* ui ) {
		_onValueChanged( *propDef, newValue, ui );
	}
protected:
	rflext::PropDefExt* propDef;
};

class SerializedSettings : public rflx::RflxObject {
public:
	RFLX_IMP_BASE_CLASS( SerializedSettings );
	bool loaded;
	std::map<std::string, std::string> global_settings;
	std::map<std::string, std::string> property_settings;
	SerializedSettings() {
		loaded = false;
	}
	RFLX_BEGIN_PROPERTY_MAP
		RFLX_IMP_PROPERTY_NAME( global_settings, "GlobalSettings" )
		RFLX_IMP_PROPERTY_NAME( property_settings, "PropertySettings" )
	RFLX_END_PROPERTY_MAP
};

RFLX_IMP_AUTO_REGISTER( PropSettingItem );
RFLX_IMP_AUTO_REGISTER( SerializedSettings );

std::vector<PropSettingItem*> PropSettingItem::settings;
std::auto_ptr<SerializedSettings> g_serializedSettings;
static const char* FilemanSettingsFile = "settings.jexon";

template< typename T >
static T _readGlobalSettingsValue( const std::string& name, const T* defaultValue = NULL ) {
	if ( g_serializedSettings.get() != NULL ) {
		auto it = g_serializedSettings->global_settings.find( name );
		if ( it != g_serializedSettings->global_settings.end() ) {
			T r;
			rflx::ValueData v;
			v.fromString( rflx::DataTrait<T>::type_value, it->second );
			if ( v.extract( r ) ) {
				return r;
			}
		}
	}
	return defaultValue ? *defaultValue : T();
}

static int _initGlobalSettings()
{
	printf( "_initGlobalPropSettings" );
	{
		rflext::PropDefExt& propDef = *( new rflext::PropDefExt() );
		propDef.nameHolder = "OverwriteMultiEdit";
		g_OverwriteMultiEdit = _readGlobalSettingsValue<bool>( "OverwriteMultiEdit" );
		propDef.defaultValHolder = g_OverwriteMultiEdit;
		propDef.descriptionHolder = "Allow modify multi-props with different values";
		propDef.bind( false );
		_bindvc( propDef,
			[ &propDef ]( const rflx::ValueData& newValue, CMFCPropertyGridProperty* ui ) {
				CString str;
				std::string val;
				newValue.toString( val );
				newValue.extract( g_OverwriteMultiEdit );
				str.Format( "GlobalSettings: %s = %s", propDef.name, val.c_str() );
				g_serializedSettings->global_settings[propDef.name] = val;
				_log( str );
			}
		);
		g_globalSettings.insert( std::make_pair( propDef.name, &propDef ) );
	}
	{
		rflext::PropDefExt& propDef = *( new rflext::PropDefExt() );
		propDef.nameHolder = "AddModify";
		g_AddModify = _readGlobalSettingsValue<bool>( "AddModify" );
		propDef.defaultValHolder = g_AddModify;
		propDef.descriptionHolder = "Enable append mode for adding value to the exists valus";
		propDef.bind( false );
		_bindvc( propDef,
			[ &propDef ]( const rflx::ValueData& newValue, CMFCPropertyGridProperty* ui ) {
				CString str;
				std::string val;
				newValue.toString( val );
				newValue.extract( g_AddModify );
				str.Format( "GlobalSettings: %s = %s", propDef.name, val.c_str() );
				g_serializedSettings->global_settings[propDef.name] = val;
				_log( str );
			}
		);
		g_globalSettings.insert( std::make_pair( propDef.name, &propDef ) );
	}
	{
		rflext::PropDefExt& propDef = *( new rflext::PropDefExt() );
		propDef.nameHolder = "RecurProps";
		g_RecurProps = _readGlobalSettingsValue<bool>( "RecurProps" );
		propDef.defaultValHolder = g_RecurProps;
		propDef.descriptionHolder = "Set property recursively";
		propDef.bind( false );
		_bindvc( propDef,
			[ &propDef ]( const rflx::ValueData& newValue, CMFCPropertyGridProperty* ui ) {
				CString str;
				std::string val;
				newValue.toString( val );
				newValue.extract( g_RecurProps );
				str.Format( "GlobalSettings: %s = %s", propDef.name, val.c_str() );
				g_serializedSettings->global_settings[propDef.name] = val;
				_log( str );
			}
		);
		g_globalSettings.insert( std::make_pair( propDef.name, &propDef ) );
	}	
	{
		rflext::PropDefExt& propDef = *( new rflext::PropDefExt() );
		propDef.nameHolder = "RecurPropDepth";
		int defaultValue = -1;
		g_RecurPropDepth = _readGlobalSettingsValue<int>( "RecurPropDepth", &defaultValue );
		propDef.defaultValHolder = g_RecurPropDepth;
		propDef.descriptionHolder = "Set property recursive depth, -1 means all";
		propDef.bind( false );
		_bindvc( propDef,
			[ &propDef ]( const rflx::ValueData& newValue, CMFCPropertyGridProperty* ui ) {
				int _val = 0;
				newValue.extract( _val );
				if ( _val < 0 ) {
					_val = DefaultRecurPropDepth;
					_setval( ui, _val );
				}			
				g_RecurPropDepth = _val;
				CString str;
				str.Format( "GlobalSettings: %s = %d", propDef.name, _val );
				std::string val;
				newValue.toString( val );
				g_serializedSettings->global_settings[propDef.name] = val;
				_log( str );
			}
		);
		g_globalSettings.insert( std::make_pair( propDef.name, &propDef ) );
	}
	{
		rflext::PropDefExt& propDef = *( new rflext::PropDefExt() );
		propDef.nameHolder = "Show";
		propDef.defaultValHolder = false;
		propDef.descriptionHolder = "Show this property at head of tree item";
		propDef.bind( false );
		//g_globalSettings
		if ( g_serializedSettings.get() != NULL ) {
			auto it = g_serializedSettings->property_settings.begin();
			while ( it != g_serializedSettings->property_settings.end() ) {
				const auto& value = it->second;
				std::vector<std::string> s;
				StringUtil::split( s, it->first, "_" );
				if ( s.size() == 2 ) {
					if ( s[1] == "Show" ) {
						rflx::ValueData val;
						val.fromString( rflx::vdt_bool, value );
						bool v;
						if ( val.extract( v ) ) {
							g_globalPropSettings_Show[ s[0] ] = v;
						}
					}
				}
				++it;			
			}
		}

		_bindvc( propDef,
			[ &propDef ]( const rflx::ValueData& newValue, CMFCPropertyGridProperty* ui ) {
				CString str;
				std::string val;
				newValue.toString( val );
				bool _val;
				newValue.extract( _val );
				str.Format( "GlobalPropSettings: %s = %s", propDef.name, val.c_str() );
				if ( ui->GetParent()->GetName() ) {
					std::string propName = ui->GetParent()->GetName();
					g_globalPropSettings_Show[ propName ] = _val;
					g_serializedSettings->property_settings[ propName + "_" + propDef.name ] = val;
				}
				_log( str );
				_updateTree();
			}
		);
		g_globalPropSettings.insert( std::make_pair( propDef.name, &propDef ) );
	}	
	return 0;
}

static void _uninitGlobalSettings()
{	
	for ( auto i : g_globalPropSettings ) {
		delete( ( PropChangedCallback* )i.second->editorData );
		delete i.second;
	}
	g_globalPropSettings.clear();
	for ( auto i : g_globalSettings ) {
		delete( ( PropChangedCallback* )i.second->editorData );
		delete i.second;
	}
	g_globalSettings.clear();
	for ( auto i : PropSettingItem::settings ) {
		delete i;
	}
	PropSettingItem::settings.clear();
}

struct _GlobalSettingsInit {
	_GlobalSettingsInit() {
		rflx::initialize();
		TCHAR fullPath[ MAX_PATH ] = { 0 };
		GetModuleFileName( GetModuleHandle( NULL ), fullPath, MAX_PATH );
		String fileName, exePath;
		StringUtil::splitFilename( fullPath, fileName, exePath );
		auto path = exePath + FilemanSettingsFile;
		auto reader = rflext::createSerializer( rflext::ISerializer::Format::ser_fmt_dtree, rflext::ISerializer::Usage::ser_usage_read );
		auto p = new SerializedSettings();
		if ( reader->fromFile( path ) ) {
			( *reader ) >> ( *p );
			p->loaded = true;
			g_serializedSettings.reset( p );
		} else {
			delete p;
			g_serializedSettings.reset( new SerializedSettings() );
		}
		delete reader;
		_initGlobalSettings();
	}
	~_GlobalSettingsInit() {
		_uninitGlobalSettings();
		auto p = g_serializedSettings.release();
		if ( p != NULL ) {
			TCHAR fullPath[ MAX_PATH ] = { 0 };
			GetModuleFileName( GetModuleHandle( NULL ), fullPath, MAX_PATH );
			String fileName, exePath;
			StringUtil::splitFilename( fullPath, fileName, exePath );
			auto path = exePath + FilemanSettingsFile;
			auto writer = rflext::createSerializer( rflext::ISerializer::Format::ser_fmt_dtree, rflext::ISerializer::Usage::ser_usage_write );
			( *writer ) << (*p);
			writer->toFile( path );
			delete writer;
			delete p;
		}
		rflx::unInitialize();
	}
}___GlobalSettingsInit__;

IMPLEMENT_DYNAMIC(EditorSettings, CDialog)

EditorSettings::EditorSettings(CWnd* pParent /*=NULL*/)
	: CDialog(EditorSettings::IDD, pParent)
{
}

EditorSettings::~EditorSettings()
{
}

void EditorSettings::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDITORSETTINGS, m_edSettings);
}


BEGIN_MESSAGE_MAP(EditorSettings, CDialog)
	ON_WM_CLOSE()
	ON_WM_SIZE()
	ON_REGISTERED_MESSAGE(AFX_WM_PROPERTY_CHANGED, &EditorSettings::OnPropertyChanged)
END_MESSAGE_MAP()


// EditorSettings message handlers
void EditorSettings::OnOK()
{
}

void EditorSettings::OnClose()
{
	// TODO: Add your message handler code here and/or call default
	if ( IsWindowVisible() ) {
		ShowWindow( SW_HIDE );
	}
}

void EditorSettings::OnSize(UINT nType, int cx, int cy)
{
	CRect rt;
	GetClientRect( rt );
	m_edSettings.MoveWindow( rt.left, rt.top, rt.Width(), rt.Height() );
	CDialog::OnSize( nType, cx, cy );
}

BOOL EditorSettings::DestroyWindow()
{
	return CDialog::DestroyWindow();
}

static void _createSettingItems( std::map<std::string, rflext::PropDefExt*>& allprops , CMFCPropertyGridProperty* propGroup )
{
	rflx::ValueData temp;
	for ( auto p : allprops ) {
		auto def = p.second;
		auto value = &def->valueHolder;
		if ( strcmp( def->name, "Show" ) == 0 ) {
			value = &temp;
			auto it = g_globalPropSettings_Show.find( propGroup->GetName() );
			if ( it != g_globalPropSettings_Show.end() ) {
				temp = it->second;
			}
		}
		auto sub = _createMFCPropFromType( def->name, def->type, value, def->description, NULL );
		PropSettingItem* propItem = new PropSettingItem();
		PropSettingItem::settings.push_back( propItem );
		propItem->bindValue( def );
		sub->SetData( (DWORD_PTR)propItem );
		propGroup->AddSubItem( sub );
	}
}

BOOL EditorSettings::OnInitDialog()
{
	CDialog::OnInitDialog();	
	HICON hIcon = LoadIcon( AfxGetInstanceHandle(), MAKEINTRESOURCE( IDR_MAINFRAME ) );
	SetIcon( hIcon, FALSE );
	auto propDefs = _getAllPropDefsFromLib();
	std::map< std::string, rflext::PropDefExt* > propdefs;
	CString errorInfo;
	for ( auto p : propDefs ) {
		auto it = propdefs.find( p->name );
		if ( it == propdefs.end() ) {
			std::string s;
			propdefs.insert( std::make_pair( p->name, p ) );
		} else {
			if ( it->second->type != p->type ) {
				errorInfo.Empty();
				errorInfo.Format( "multiple mis-matched prop found: %s", p->name );
				_appendLog( errorInfo.GetBuffer( 0 ) );
			}
		}
	}
	if ( !propdefs.empty() ) {
		auto gs = new CMFCPropertyGridProperty( "GlobalSettings" );
		auto ignoreList = new CMFCPropertyGridProperty( "IgnoreList" );
		for ( auto s : g_globalIgnoreTable ) {
			auto p = new CMFCPropertyGridProperty( s.c_str(), ( _variant_t )s.c_str(), "" );
			p->AllowEdit( FALSE );
			ignoreList->AddSubItem( p );
		}
		gs->AddSubItem( ignoreList );
		_createSettingItems( g_globalSettings, gs );

		m_edSettings.AddProperty( gs );
		auto group = new CMFCPropertyGridProperty( "Properties" );
		for ( auto p : propdefs ) {
			auto def = p.second;
			auto item = new CMFCPropertyGridProperty( def->name );
			_createSettingItems( g_globalPropSettings, item );
			group->AddSubItem( item );
		}
		group->Expand( FALSE );
		m_edSettings.AddProperty( group );
	}
	return TRUE;
}

LRESULT EditorSettings::OnPropertyChanged( WPARAM wParam, LPARAM lParam )
{	
	CMFCPropertyGridProperty* propUi = ( CMFCPropertyGridProperty* )lParam;
	if ( propUi ) {
		auto callbacker = ( PropSettingItem* )propUi->GetData();
		if ( callbacker != NULL ) {
			rflx::ValueData newVal = _convert_COleVariant( propUi->GetValue() );
			callbacker->onValueChanged( newVal, propUi );
		}
	}	
	return 0L;
}
