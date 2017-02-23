static const char* FilemanSettingDefineFile = "FilemanSettingDefine.xml";
static const char* GlobalIgnoreList = "GlobalIgnoreList.txt";
static const char* ScriptFunctionName_Main = "update";
static const char* ScriptFunctionName_Enter = "onEnter";
static const char* ScriptFunctionName_Exit = "onExit";
static const char* ScriptTableName_Context = "context";

static const char* HelpMessage = 
"global function:\r\n\
 void    help()\r\n\
 void    trace( fmt, ... )\r\n\
 void    clearLog()\r\n\
 void    appendLog()\r\n\
 \r\n\
Settings members :\r\n\
 string  getTypeName()\r\n\
 string  getName()\r\n\
 bool    getBool( name )\r\n\
 int     getInt( name )\r\n\
 real    getReal( name )\r\n\
 string  getStr( name )\r\n\
 string  getPath()\r\n\
 string  getPathName()\r\n\
 bool    hasProp( name )\r\n\
 bool    isDir()\r\n\
 bool    isFile()\r\n\
 bool    isDeleted()\r\n\
 bool    isResolved()\r\n\
 bool    isValid()\r\n\
 bool    select( pathlist )\r\n\
 bool    revselect( pathlist )\r\n\
\r\n\
sample:\r\n\
\r\n\
function main( s )\r\n\
    if s then\r\n\
        --all the directoies are visible\r\n\
        if s:getTypeName() == \"DirectorySettings\" then\r\n\
            return true\r\n\
        end\r\n\
        if not s:getBool(\"Build\") then\r\n\
            return true\r\n\
        end\r\n\
    end\r\n\
    return false\r\n\
end\r\n\
";

static void _clearLog()
{
	( ( CFilemanDlg* )theApp.GetMainWnd() )->ClearLogText();
}

static void _pushLog( const char* s )
{
	if ( !s || *s == '\0' ) {
		return;
	}
	( ( CFilemanDlg* )theApp.GetMainWnd() )->PushLogText( s );
}

static void _appendLog( const char* s )
{
	if ( !s || *s == '\0' ) {
		return;
	}
	( ( CFilemanDlg* )theApp.GetMainWnd() )->AppendLogText( s );
}

static void _AppendLog( const CString& s )
{
	if ( s.IsEmpty() ) {
		return;
	}
	( ( CFilemanDlg* )theApp.GetMainWnd() )->AppendLogText( s );
}

static void _helpFunc() {
	_clearLog();
	_appendLog( HelpMessage );
}
static std::string logBuf;
static int _luaB_print( lua_State* L ) {
	int n = lua_gettop(L);  /* number of arguments */
	int i;
	lua_getglobal(L, "tostring");
	logBuf.clear();
	for (i=1; i<=n; i++) {
		const char *s;
		lua_pushvalue(L, -1);  /* function to be called */
		lua_pushvalue(L, i);   /* value to print */
		lua_call(L, 1, 1);
		s = lua_tostring(L, -1);  /* get result */
		if (s == NULL)
			return luaL_error(L, LUA_QL("tostring") " must return a string to "
			LUA_QL("print"));
		if (i>1) logBuf.push_back('\t');
		logBuf.append( s );
		lua_pop(L, 1);  /* pop result */
	}
	logBuf.append( "\r\n" );
	_pushLog(logBuf.c_str());
	return 0;
}

static void _autoRunScript( bool flag )
{
	( ( CFilemanDlg* )theApp.GetMainWnd() )->AutoRunScript( flag );
}

static void _select( lua_tinker::table pathList ) {
	( ( CFilemanDlg* )theApp.GetMainWnd() )->Select( pathList );
}

static void _revselect( lua_tinker::table pathList ) {
	( ( CFilemanDlg* )theApp.GetMainWnd() )->ReverseSelect( pathList );
}

static void _compileRunScript()
{
	( ( CFilemanDlg* )theApp.GetMainWnd() )->CompileRunScript();
}

static bool _compileError = false;
static bool _runtimeError = false;

static auto lua_error_with_compile = []( lua_State* state )
{
	const char* e = lua_tostring( state, -1 );
	if ( e ) {
		char errorInfo[ 1024 ] = {0};
		sprintf_s( errorInfo, "Compile error: %s\n", e );
		__TracePrint( errorInfo );		
		if ( Misc::isDebuggerPresent() ) {
			__TracePrint( errorInfo );
		}
		_appendLog( errorInfo );
		_compileError = true;
	}
	return 0;
};

static auto lua_error_with_running = []( lua_State* state )
{
	const char* e = lua_tostring( state, -1 );
	if ( e ){
		char errorInfo[ 1024 ] = {0};
		sprintf_s( errorInfo, "Running error: %s\n", e );
		if ( Misc::isDebuggerPresent() ) {
			__TracePrint( errorInfo );
		}
		_appendLog( errorInfo );
		_runtimeError = true;
	}
	return 0;
};

template< typename RVal >
RVal _luaCall( lua_State* L, const char* name, bool allowError = false )
{
	lua_pushcclosure( L, lua_error_with_running, 0 );
	int errfunc = lua_gettop( L );
	lua_pushstring( L, name );
	lua_gettable( L, LUA_GLOBALSINDEX );
	if ( lua_isfunction( L, -1 ) ) {
		if ( lua_pcall( L, 0, 1, errfunc ) != 0 ) 	{
			lua_pop( L, 1 );
		}
	} else {
		if ( !allowError ) {
			char errorInfo[ 1024 ] = {0};
			sprintf_s( errorInfo, "Attempt to call global `%s' (not a function)", name );
			if ( Misc::isDebuggerPresent() ) {
				__TracePrint( errorInfo );
			}
			_appendLog( errorInfo );
			_runtimeError = true;
		}
	}
	lua_remove( L, -2 );
	return lua_tinker::pop< RVal >( L );
}

template<typename RVal, typename T1>
static RVal _luaCall( lua_State* L, const char* name, T1 arg, bool allowError = false )
{
	lua_pushcclosure( L, lua_error_with_running, 0 );
	int errfunc = lua_gettop( L );
	lua_pushstring( L, name );
	lua_gettable( L, LUA_GLOBALSINDEX );
	if ( lua_isfunction( L, -1 ) ) {
		lua_tinker::push( L, arg );
		if ( lua_pcall( L, 1, 1, errfunc ) != 0 ) {
			lua_pop( L, 1 );
		}
	} else {
		if ( !allowError ) {
			char errorInfo[ 1024 ] = {0};
			sprintf_s( errorInfo, "Attempt to call global `%s' (not a function)", name );
			if ( Misc::isDebuggerPresent() ) {
				__TracePrint( errorInfo );
			}
			_appendLog( errorInfo );
			_runtimeError = true;
		}
	}
	lua_remove( L, -2 );
	return 	lua_tinker::pop< RVal >( L );
}

static bool _callFilterFunc( crim::CMScript s, Settings* settings,
							const char* funName = ScriptFunctionName_Main,
							bool allowError = false ) {
	_runtimeError = false;
	bool ret = _luaCall< bool, Settings* >( crim::_internal::_script_2_lua_state( s ),
		funName, settings, allowError );
	if ( _runtimeError && !allowError ) {
		std::string msg( "lua runtime error: " );
		msg.append( settings->getName() );
		throw std::runtime_error( msg );
	}
	return ret;
}

static bool _callFunc( crim::CMScript s, const char* funName, bool allowError = false ) {
	_runtimeError = false;
	_luaCall< void >( crim::_internal::_script_2_lua_state( s ),
		funName, allowError );
	if ( _runtimeError && !allowError ) {
		std::string msg( "lua runtime error while call: " );
		msg.append( funName );
		return false;
	}
	return true;
}

struct _filterTreeRecur {
	CMultiTree& tree;
	crim::CMScript script;
	std::map< Settings*, bool > cache;
	_filterTreeRecur( crim::CMScript s, CMultiTree& _tree ) : script( s ), tree( _tree ) {
		assert( script );
	}
	bool touch( Settings* s ) {
		auto it = cache.find( s );
		if ( it == cache.end() ) {
			bool r = _callFilterFunc( script, s );
			cache.insert( std::make_pair( s, r ) );
			return r;
		} else {
			return it->second;
		}
	}

	bool nodeIsVisible( const WinUtil::DirTreeNode& data ) {
		bool childrenVisible = false;
		for ( auto f : data.fileNames ) {
			Settings* settings = reinterpret_cast< Settings* >( f.userData );
			if ( touch( settings ) ) {
				childrenVisible = true;
				break;
			}
		}
		if ( childrenVisible ) {
			return true;
		}
		bool subVisible = false;
		for ( int i = (int)data.subDirs.size() - 1; i >= 0; --i ) {
			if ( nodeIsVisible( *data.subDirs[i] ) ) {
				subVisible = true;
				break;
			}
		}
		if ( subVisible ) {
			return true;
		}
		Settings* settings = reinterpret_cast< Settings* >( data.userData );
		return touch( settings );
	}

	void operator()( const WinUtil::DirTreeNode& data, HTREEITEM parent ) {
		Settings* settings = reinterpret_cast< Settings* >( data.userData );
		if ( !nodeIsVisible( data ) ) {
			return ;
		}
		HTREEITEM cur = tree.InsertItem( data.curDir.c_str(), TREE_ICON_FOLDER, TREE_ICON_FOLDER, parent );
		tree.SetItemData( cur, ( DWORD_PTR )settings );
		for ( auto f : data.fileNames ) {
			Settings* settings = reinterpret_cast< Settings* >( f.userData );
			if ( touch( settings ) ) {
				HTREEITEM child = tree.InsertItem( f.name.c_str(), TREE_ICON_FILE, TREE_ICON_FILE, cur );
				tree.SetItemData( child, ( DWORD_PTR )settings );
			}
		}
		// simulate stack pop_back order
		for ( int i = (int)data.subDirs.size() - 1; i >= 0; --i ) {
			( *this )( *data.subDirs[i], cur );
		}
	}
};

static void _resetScriptEnv()
{
	lua_State* L = crim::_internal::_current_lua_context();
	if ( !L ) {
		return;
	}
	const char* funName[] = {
		ScriptFunctionName_Main,
		ScriptFunctionName_Enter,
		ScriptFunctionName_Exit,
	};
	for ( auto name : funName ) {
		lua_getglobal( L, name );
		if ( !lua_isnil( L, -1 ) ) {
			lua_pushnil( L );
			lua_setglobal( L, name );
		}
	}
	lua_getglobal( L, ScriptTableName_Context );
	if ( !lua_isnil( L, -1 ) ) {
		lua_pushnil( L );
		lua_setglobal( L, ScriptTableName_Context );
	}
	// clear stack
	lua_settop( L, 0 );
}


void CFilemanDlg::ClearLogText()
{
	m_OutputWindow.SetWindowText( "" );
	m_LogText.Empty();
}

void CFilemanDlg::PushLogText( const CString& text )
{	
	m_LogText.Append( text );
	if ( !m_PostLog ) {
		m_OutputWindow.SetWindowText( m_LogText );
	}
}

void CFilemanDlg::AppendLogText( const CString& text )
{	
	m_LogText.Append( text );
	m_LogText.Append( "\r\n" );
	if ( !m_PostLog ) {
		m_OutputWindow.SetWindowText( m_LogText );
		m_OutputWindow.SetSel( m_LogText.GetLength(), m_LogText.GetLength() );
	}
}

void CFilemanDlg::OnEnChangeFilterEd()
{
	m_FilterSourceUpdate = TREE_ED_INTERVAL;
}

void CFilemanDlg::AutoRunScript( bool enable )
{
	m_AutoRunScript = enable;
}

void CFilemanDlg::CompileRunScript( bool compileOnly, bool switchTab )
{
	int start, end;
	m_FilterEd.GetSel( start, end );
	m_FilterSourceUpdate = -10000;
	CWnd* ed = GetDlgItem( IDC_FILTER_ED );
	assert( ed );
	ed->GetWindowText( m_SourceCode );
	bool laseCompileError = _compileError;
	_compileError = false;
	_runtimeError = false;
	if ( m_FilterScript ) {
		crim::cmClose( m_FilterScript );
		m_FilterScript = NULL;
	}
	CString modSource = m_SourceCode;
	if ( m_SourceCode.GetLength() > 0 ) {
		_resetScriptEnv();
		m_FilterScript = cmCompileSourceFromString(
			modSource.GetBuffer( 0 ),
			lua_error_with_compile,
			lua_error_with_running );
		if ( !_compileError ) {
			this->AppendLogText( "Compile succeeded." );
		}
		if ( m_FilterScript ) {
			if ( !compileOnly ) {
				_callFunc( m_FilterScript, ScriptFunctionName_Enter, true );
				FilterTree( m_FileSettingTree );
				_callFunc( m_FilterScript, ScriptFunctionName_Exit, true );
				if ( switchTab ) {
					m_Tab.SetCurSel( 1 );
					m_Tab.SetCurFocus( 1 );
					UpdateTab();
				}
			}
		}
	}
	m_FilterEd.SetSel( start, end );
}

void CFilemanDlg::OnFileLoadscript()
{
	CFileDialog fd( TRUE, "lua", NULL, OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_FILEMUSTEXIST, "LUA Files (*.lua)|*.lua|TEXT Files (*.txt)|*.txt|All Files (*.*)|*.*||" );
	fd.DoModal();
	if ( !fd.GetPathName().IsEmpty() ) {
		Misc::ByteBuffer buf;
		Misc::readFileAll( fd.GetPathName(), buf );
		buf.push_back( '\0' );
		m_SourceCode = &*buf.begin();
		m_FilterEd.SetWindowText( &*buf.begin() );
		m_FilterEd.SetSel( 0, 0 );
	}
}

void CFilemanDlg::OnRunScript()
{
	ClearLogText();
	CompileRunScript( false, true );
}

void CFilemanDlg::OnFileSavescript()
{
	m_FilterEd.GetWindowText( m_SourceCode );
	if ( m_SourceCode.IsEmpty() ) {
		AppendLogText( "Empty source can't be saved!" );
	}
	CFileDialog fd( FALSE, "lua", NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "LUA Files (*.lua)|*.lua|TEXT Files (*.txt)|*.txt|All Files (*.*)|*.*||" );
	fd.DoModal();
	if ( !fd.GetPathName().IsEmpty() ) {
		if ( Misc::writeFileAll( fd.GetPathName(), m_SourceCode.GetBuffer(0), m_SourceCode.GetLength() ) ) {
			AppendLogText( "Save succeeded!" );
		} else {
			AppendLogText( "Save failed!" );
		}
	}
}

void CFilemanDlg::OnFileTouchRun()
{
	ClearLogText();
	m_FilterSourceUpdate = -10000;
	CWnd* ed = GetDlgItem( IDC_FILTER_ED );
	assert( ed );
	ed->GetWindowText( m_SourceCode );
	bool laseCompileError = _compileError;
	_compileError = false;
	_runtimeError = false;
	if ( m_FilterScript ) {
		crim::cmClose( m_FilterScript );
		m_FilterScript = NULL;
	}
	CString modSource = m_SourceCode;
	if ( m_SourceCode.GetLength() > 0 ) {
		_resetScriptEnv();
		m_FilterScript = cmCompileSourceFromString(
			modSource.GetBuffer( 0 ),
			lua_error_with_compile,
			lua_error_with_running );
		if ( !_compileError ) {
			this->AppendLogText( ">>> ok." );
		}
	}
}
