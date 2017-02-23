
#define CONV_VAL( type, name ) \
	case type: {\
			return rflx::ValueData( name );\
		}\
		break;

#define PARSE_VAL( type ) \
	type val = type();\
	if ( value && !value->isNil() ) {\
		value->extract( val );\
	}

#define BASE_PARSE_VAL( type ) \
	case rflx::vdt_##type: {\
			PARSE_VAL( type );\
			ret = new MyPropertyGridProperty( name, ( _variant_t )val, desc );\
		}\
		break;


rflx::ValueData _convert_COleVariant( const COleVariant& val ) {
	switch ( val.vt ) {
		CONV_VAL( VT_BOOL, ( val.boolVal != 0 ) )
		CONV_VAL( VT_I1, val.cVal )
		CONV_VAL( VT_I2, val.iVal )
		CONV_VAL( VT_I4, val.lVal )
		CONV_VAL( VT_I8, val.llVal )
		CONV_VAL( VT_UI1, val.bVal )
		CONV_VAL( VT_UI2, val.uiVal )
		CONV_VAL( VT_UI4, val.ulVal )
		CONV_VAL( VT_UI8, val.ullVal )
		CONV_VAL( VT_INT, val.intVal )
		CONV_VAL( VT_UINT, val.uintVal )
		CONV_VAL( VT_R4, val.fltVal )
		CONV_VAL( VT_R8, val.dblVal )
	case VT_EMPTY:
		return rflx::ValueData();
		break;	
	case VT_BSTR:
		RFLX_TASSERT( ( rflx::TypeEqual< BSTR, wchar_t* >::value ) );
		return rflx::ValueData( ( const wchar_t* )val.pbstrVal );
		break;
	default:
		return rflx::ValueData();
	}
}

static BOOL _TextToVar( COleVariant& var, VARTYPE type, const CString& strText ) {
	CString strVal = strText;
	switch (type)
	{
	case VT_BSTR:
		var = (LPCTSTR) strVal;
		return TRUE;
	case VT_UI1:
		var = strVal.IsEmpty() ?(BYTE) 0 :(BYTE) strVal[0];
		return TRUE;
	case VT_I2:
		var = (short) _ttoi(strVal);
		return TRUE;
	case VT_INT:
	case VT_I4:
		var = _ttol(strVal);
		return TRUE;
	case VT_UI2:
		var.uiVal = unsigned short(_ttoi(strVal));
		return TRUE;
	case VT_UINT:
	case VT_UI4:
		var.ulVal = unsigned long(_ttol(strVal));
		return TRUE;
	case VT_R4:
		{
			float fVal = 0.;
			strVal.TrimLeft();
			strVal.TrimRight();
			if ( !strVal.IsEmpty() ) {
				_stscanf_s(strVal, _TEXT( "%f" ), &fVal);
			}
			var = fVal;
		}
		return TRUE;

	case VT_R8: {
			double dblVal = 0.;
			strVal.TrimLeft();
			strVal.TrimRight();

			if ( !strVal.IsEmpty() ) {
				_stscanf_s(strVal, _TEXT( "%lf" ), &dblVal);
			}
			var = dblVal;
		}
		return TRUE;

	case VT_BOOL:
		strVal.TrimRight();
		var = ( VARIANT_BOOL )(strVal == "True");
		return TRUE;
	}
	return FALSE;
}

class MyPropertyGridProperty : public CMFCPropertyGridProperty {
public:
	MyPropertyGridProperty( const CString& strGroupName, DWORD_PTR dwData = 0, BOOL bIsValueList = FALSE ) :
		CMFCPropertyGridProperty( strGroupName, dwData, bIsValueList ) {}

	MyPropertyGridProperty( const CString& strName, const COleVariant& varValue, LPCTSTR lpszDescr = NULL, DWORD_PTR dwData = 0,
		LPCTSTR lpszEditMask = NULL, LPCTSTR lpszEditTemplate = NULL, LPCTSTR lpszValidChars = NULL ) :
	CMFCPropertyGridProperty( strName, varValue, lpszDescr, dwData,
		lpszEditMask, lpszEditTemplate, lpszValidChars ) {}

	virtual BOOL OnUpdateValue() {
		ASSERT_VALID( this );
		ASSERT_VALID( m_pWndInPlace );
		ASSERT_VALID( m_pWndList );
		ASSERT( ::IsWindow(m_pWndInPlace->GetSafeHwnd()) );
		CString strText;
		m_pWndInPlace->GetWindowText( strText );
		BOOL bRes = FALSE;
		BOOL bIsChanged = FormatProperty() != strText;
		if ( m_bIsValueList ) {
			CString strDelimeter( _T( ',' ) );
			for ( int i = 0; !strText.IsEmpty() && i < GetSubItemsCount(); i++ ) {
				CString strItem = strText.SpanExcluding(strDelimeter);
				if ( strItem.GetLength() + 1 > strText.GetLength() ) {
					strText.Empty();
				} else {
					strText = strText.Mid( strItem.GetLength() + 1 );
				}
				strItem.TrimLeft();
				strItem.TrimRight();
				CMFCPropertyGridProperty* pSubItem = GetSubItem( i );
				ASSERT_VALID( pSubItem );
				COleVariant _val;
				_TextToVar( _val, pSubItem->GetOriginalValue().vt, strItem );
				pSubItem->SetValue( _val );
			}
			bRes = TRUE;
		} else {
			bRes = TextToVar( strText );
		}
		if ( bRes ) {
			m_pWndList->OnPropertyChanged( this );
		}
		return bRes;
	}
};

CMFCPropertyGridProperty* _createMFCPropFromType(
	const char* name, rflx::ValueDataType type, const rflx::ValueData* value = NULL,
	const char* desc = NULL, const char* ed = NULL )
{
	CMFCPropertyGridProperty* ret = NULL;
	switch ( type ) {
		BASE_PARSE_VAL( bool )
		BASE_PARSE_VAL( char )
		BASE_PARSE_VAL( uchar )
		BASE_PARSE_VAL( wchar )
		BASE_PARSE_VAL( int )
		BASE_PARSE_VAL( uint )
		BASE_PARSE_VAL( short )
		BASE_PARSE_VAL( ushort )
		BASE_PARSE_VAL( long )
		BASE_PARSE_VAL( ulong )
		BASE_PARSE_VAL( llong )
		BASE_PARSE_VAL( ullong )
		BASE_PARSE_VAL( float )
		BASE_PARSE_VAL( double )
	case rflx::vdt_string: {		
			PARSE_VAL( std::string );
			ret = new MyPropertyGridProperty( name, ( _variant_t )val.c_str(), desc );
		}
		break;
	case rflx::vdt_wstring: {		
			PARSE_VAL( std::wstring );
			ret = new MyPropertyGridProperty( name, ( _variant_t )val.c_str(), desc );
		}
		break;
	default:
		ret = new MyPropertyGridProperty( name, _variant_t(), desc );
	}
	if ( ed && *ed ) {
		String editorData( ed );
		std::vector< String > options;
		StringUtil::split( options, editorData, ",; " );
		for ( auto o : options ) {
			if ( !o.empty() ) {
				ret->AddOption( o.c_str() );
			}
		}
	}
	return ret;
}

static void _DirTreeNode_UnlinkTree( WinUtil::DirTreeNode& node )
{
	if ( node.userData ) {
		Settings* p = reinterpret_cast< Settings* >( node.userData );
		p->handle = NULL;
	}
	for ( size_t i = 0; i < node.fileNames.size(); ++i ) {
		Settings* p = reinterpret_cast< Settings* >( node.fileNames[i].userData );
		p->handle = NULL;
	}
	for ( size_t i = 0; i < node.subDirs.size(); ++i ) {
		_DirTreeNode_UnlinkTree( *node.subDirs[i] );
	}
}

static void _DirTreeNode_UserData_Free( void* userData, bool isDir )
{
	Settings* p = reinterpret_cast< Settings* >( userData );
	if ( p ) { 
		delete p;
	}
}

BEGIN_MESSAGE_MAP(CMyEdit, CEdit)
	ON_WM_DROPFILES()
END_MESSAGE_MAP()

BOOL CMyEdit::PreTranslateMessage( MSG* pMsg )
{
	if ( pMsg->message == WM_KEYDOWN ) {
		// get the char index of the caret position
		if ( pMsg->wParam == VK_TAB && ( GetAsyncKeyState( VK_CONTROL ) & 0x8000 ) == 0 ) {
			int nPos = LOWORD( CharFromPos( GetCaretPos() ) );
			// select zero chars
			SetSel( nPos, nPos );
			// then replace that selection with a TAB
			ReplaceSel( "    ", TRUE );
			// no need to do a msg translation, so quit. 
			// that way no further processing gets done
			return TRUE;
		} else if ( pMsg->wParam == 'A' && ( GetAsyncKeyState( VK_CONTROL ) & 0x8000 ) ) {
			// select all editor content
			SetSel( 0, -1 );
			return TRUE;
		}
	}
	return CEdit::PreTranslateMessage( pMsg );
}

void CMyEdit::OnDropFiles(HDROP hDropInfo)
{
	GetParent()->SendMessage( WM_DROPFILES, (WPARAM)hDropInfo, 0 );
}

template< typename T >
static void _walkTreeItems( CTreeCtrl& tree, HTREEITEM item, T op )
{
	op( item );
	auto child = tree.GetChildItem( item );
	while ( child ) {
		_walkTreeItems( tree, child, op );
		child = tree.GetNextSiblingItem( child );
	}
}

static bool _propVisible( const String& name ) {
	auto it = g_globalPropSettings_Show.find( name );
	if ( it != g_globalPropSettings_Show.end() ) {
		return it->second;
	}
	return false;
}

static String _getVisualString( const Settings& s ) {
	if ( g_globalPropSettings_Show.empty() ) {
		return String();
	}
	static CString buf;
	buf.Empty();
	bool hasPropsToShow = false;
	auto propObj = s.getPropObjForRead();
	if ( propObj != NULL ) {
		buf.AppendChar( '<' );
		while ( propObj ) {
			for ( auto p : propObj->getProps() ) {
				if ( buf.GetLength() > 10 ) {
					buf.Append( "..." );
					break;
				}
				if ( _propVisible( p.first ) ) {
					String s;
					if ( hasPropsToShow ) {
						buf.AppendChar( '_' );
					}
					if ( p.second->valueHolder.toString( s ) ) {
						if ( !s.empty() ) {
							buf.Append( s.c_str() );
							hasPropsToShow = true;
						}
					} else {
						buf.AppendChar( '?' );
						hasPropsToShow = true;
					}
				}
			}
			propObj = propObj->getBaseObj();
		}
		buf.AppendChar( '>' );
	}
	return hasPropsToShow ? buf.GetBuffer( 0 ) : String();
}