#include "Misc.h"
#ifdef GELIB_BUILD_FOR_WINDOWS
	#include <windows.h>
#endif
#include "ScriptApi.h"
#include <string>
#include <vector>
#include <map>
#include <assert.h>

const char* SCRIPT_ENTRY_NAME				= "main";
const char* SCRIPT_INITIALIZE_FUNC_NAME		= "Initialize";
const char* SCRIPT_USER_DATA_NAME			= "user_data";

typedef void (*fp_PrintOnConsole)( const char* msg );
fp_PrintOnConsole PrintOnConsole_Hook = NULL;

static void printOnConsole( const char* msg )
{
	if ( PrintOnConsole_Hook )
		PrintOnConsole_Hook( msg );
	else
		printf( msg );
}

static int lua_error_with_compile( lua_State* state )
{
	const char* szError = lua_tostring( state, -1 );
	if( szError){
		char errorInfo[ 1024 ] = {0};
		sprintf( errorInfo, "Compile error: %s\n", szError );
		__TracePrint( errorInfo );
	}
	return 0;
}

static int lua_error_with_running( lua_State* state )
{
	const char* szError = lua_tostring( state, -1 );
	if ( szError ){
		char errorInfo[ 1024 ] = {0};
		sprintf( errorInfo, "Running error: %s\n", szError );
		__TracePrint( errorInfo );		
	}
	return 0;
}



#define __PrintOnConsole( x ) printOnConsole( x )

namespace crim
{
	struct ScriptBank
	{
		void* userData;
		std::vector< CMScript > scripts;
		ScriptBank() : userData( NULL )
		{
		}
	};

	struct LuaThreadInfo
	{
		ScriptBank*				bank;
		lua_State*				self;
		int						stateRef;
		int						localMethodsRef;
		int						waitFrame;
		int						waitTime;
		int						runTime;
		bool					exit;
		std::string				sourceFile;
		std::string				commandLine;
		CMThreadLocalStore		tls;
		void reset() { waitFrame = waitTime = 0; }
		LuaThreadInfo( ) : bank( NULL ), self( NULL ), stateRef( 0 ), localMethodsRef( 0 ), waitFrame( 0 ), waitTime( 0 ), runTime( 0 ), exit( false ) { }
		void setInt( const char* name, int number ) { tls[ name ] = number; }
		int getInt( const char* name ) { return tls[ name ]; }
		void createLocalMethodsTable() {
			if ( self ) {
				if ( localMethodsRef ) {
					luaL_unref( self, LUA_REGISTRYINDEX, localMethodsRef );
				}
				lua_newtable( self );
				localMethodsRef = luaL_ref( self, LUA_REGISTRYINDEX );
			}
		}
		void destroyLocalMethodsTable() {
			if ( self && localMethodsRef ) {
				luaL_unref( self, LUA_REGISTRYINDEX, localMethodsRef );
				localMethodsRef = NULL;
			}
		}
	};

	typedef std::map< lua_State*, LuaThreadInfo* >	LuaNodeMap;
	typedef LuaNodeMap::iterator					LuaNodeMapIt;
	typedef std::vector< LuaThreadInfo* >			LuaNodeVector;
	typedef LuaNodeVector::iterator					LuaNodeVectorIt;

	struct LuaContext
	{
		lua_State*					pBaseState;
		std::string					szName;
		LuaNodeMap					Children;
		LuaThreadInfo*				currentThread;
		struct PostRunFileInfo {
			std::string szFileName;
			PostRunFileInfo( const std::string& fileName ) : szFileName( fileName ) { }
		};
		std::vector< PostRunFileInfo >	PostRunFileName;

		LuaContext() : pBaseState( NULL ), currentThread( NULL ) {}
	};

	static LuaContext	_Default_LuaContext;
	static LuaContext*	_Current_LuaContext = NULL;

	namespace _internal
	{
		inline LuaContext& _default_context()
		{
			return _Default_LuaContext;
		}
		inline LuaContext& _current_context()
		{
			assert( _Current_LuaContext );
			return *_Current_LuaContext;
		}

		lua_State* _current_lua_context()
		{
			assert( _Current_LuaContext );
			return _Current_LuaContext->pBaseState;
		}

		lua_State* _script_2_lua_state( CMScript script )
		{
			if ( !script )
				return _Current_LuaContext->pBaseState;
			LuaThreadInfo* node = reinterpret_cast< LuaThreadInfo* >( script );
			assert( node->stateRef != 0 );
			return node->self;
		}
	}
	using namespace _internal;

	static int LuaWaitFrame( lua_State* L )
	{
		lua_pushinteger( L, (int)L );
		lua_gettable( L, LUA_GLOBALSINDEX );
		LuaThreadInfo* node = NULL;
		if ( lua_islightuserdata( L, -1 ) ) {
			node = ( LuaThreadInfo* )lua_touserdata( L, -1 );
			lua_pop( L, 1 );
			if ( lua_isnumber( L, 1 ) ) {
				int frame = int( lua_tonumber( L, 1 ) );
				node->waitFrame += frame;
				return lua_yield( L, 0 );
			}
		}
		return 0;
	}

	static int LuaWaitTime( lua_State* L )
	{
		lua_pushinteger( L, (int)L );
		lua_gettable( L, LUA_GLOBALSINDEX );
		LuaThreadInfo* node = NULL;
		if ( lua_islightuserdata( L, -1 ) ) {
			node = ( LuaThreadInfo* )lua_touserdata( L, -1 );
			lua_pop( L, 1 );
			if ( lua_isnumber( L, 1 ) ) {
				int time = int( lua_tonumber( L, 1 ) );
				node->waitTime += time;
				return lua_yield( L, 0 );
			}
		}
		return 0;
	}

	static int LuaGetRunTime( lua_State* L )
	{
		lua_pushinteger(L, (int)L);
		lua_gettable( L, LUA_GLOBALSINDEX );
		LuaThreadInfo* node = NULL;
		if ( lua_islightuserdata( L, -1 ) ) {
			node = (LuaThreadInfo*)lua_touserdata( L, -1 );
			lua_pop( L, 1 );
			lua_pushinteger( L, node->runTime );
			return 1;
		}
		return 0;
	}

	static int LuaGetElapsedTime( lua_State* L )
	{
		assert( 0 && "LuaGetElapsedTime not implemented!" );
		//lua_pushinteger( L, (int)sys::getElapsedTimeMS() );
		return 1;
	}
		
	static int LuaGetTime( lua_State* L )
	{
		assert( 0 && "LuaGetTime not implemented!" );
		//lua_pushinteger( L, (int)sys::getTimeMS() );
		return 1;
	}
	static int LuaGetInt( lua_State* L )
	{
		lua_pushinteger( L, (int)L );
		lua_gettable( L, LUA_GLOBALSINDEX );
		LuaThreadInfo* node = NULL;
		if ( lua_islightuserdata( L, -1 ) ) {
			node = ( LuaThreadInfo* )lua_touserdata( L, -1 );
			lua_pop( L, 1 );
			if ( lua_isstring( L, 1 ) ) {
				const char* s = lua_tostring( L, 1 );
				int value = node->getInt( s );
				lua_pushinteger( L, value );
				return 1;
			}
		}
		return 0;
	}

	static int LuaSetInt( lua_State* L )
	{
		lua_pushinteger( L, (int)L );
		lua_gettable( L, LUA_GLOBALSINDEX );
		LuaThreadInfo* node = NULL;
		if ( lua_islightuserdata( L, -1 ) ) {
			node = ( LuaThreadInfo* )lua_touserdata( L, -1 );
			lua_pop( L, 1 );
			if ( lua_isstring( L, 1 ) ) {
				const char* s = lua_tostring( L, 1 );
				int value = lua_tointeger( L, 2 );
				node->setInt( s, value );
			}
		}
		return 0;
	}
	static int LuaRegisterCallback( lua_State* L )
	{
		lua_pushinteger( L, ( int )L );
		lua_gettable( L, LUA_GLOBALSINDEX );
		LuaThreadInfo* node = NULL;
		if ( lua_islightuserdata( L, -1 ) ) {
			node = ( LuaThreadInfo* )lua_touserdata( L, -1 );
			lua_pop( L, 1 ); // pop node pointer
		}
		if ( node && node->localMethodsRef ) {
			lua_rawgeti( L, LUA_REGISTRYINDEX, node->localMethodsRef );
			if ( lua_istable( L, -1 ) ) {
				lua_pushvalue( L, 1 ); // name
				lua_pushvalue( L, 2 ); // lua function
				lua_rawset( L, -3 ); // add to table
				lua_pop( L, 1 ); // pop table
			}
		}
		return 0;
	}

	static int LuaInvokeCallback( lua_State* L )
	{
		lua_pushinteger( L, ( int )L );
		lua_gettable( L, LUA_GLOBALSINDEX );
		LuaThreadInfo* node = NULL;
		if ( lua_islightuserdata( L, -1 ) ) {
			node = ( LuaThreadInfo* )lua_touserdata( L, -1 );
			lua_pop( L, 1 ); // pop node pointer
		}
		if ( node->localMethodsRef ) {
			if ( lua_isstring( L, 1 ) ) {
				int argn = lua_gettop( L ) - 1;
				const char* funcName = lua_tostring( L, 1 );
				lua_pushcclosure( node->self, lua_error_with_running, 0 );
				int errfunc = lua_gettop( node->self );
				lua_rawgeti( L, LUA_REGISTRYINDEX, node->localMethodsRef );
				if ( lua_istable( L, -1 ) ) {
					lua_getfield( L, -1, funcName );
					if ( lua_isfunction( L, -1 ) ) {
						int argBegin = 2;
						int argEnd = argBegin + argn;
						for ( int i = argBegin; i < argEnd; ++i )
							lua_pushvalue( L, i );
						if ( 0 == lua_pcall( node->self, argn, 0, errfunc ) ) {
							int s = lua_status( node->self );
							if ( LUA_YIELD == s || 0 == s ) {
								lua_pushboolean( L, 1 );
								return 1;
							}
						}
					}
				}
			}
		}
		lua_pushboolean( L, 0 );
		return 1;
	}

	static int LuaTerminate( lua_State* L )
	{
		lua_pushinteger(L, (int)L);
		lua_gettable( L, LUA_GLOBALSINDEX );
		LuaThreadInfo* node = NULL;
		if ( lua_islightuserdata( L, -1 ) ) {
			node = (LuaThreadInfo*)lua_touserdata( L, -1 );
			lua_pop( L, 1 );
			node->exit = true;
			if ( lua_isnumber( L, 1 ) ) {
				// got return value here
			}
			return lua_yield( L, 0 );
		}
		return 0;
	}

	template< typename T >
	static char* push_arg( char* cur, ... )
	{
		va_list va;
		va_start( va, cur );
		va_list start = va;
		T value = va_arg( va, T );
		size_t offset = (char*)va - (char*)start;
		memcpy( cur, start, offset );
		cur += offset;
		return cur;
	}

	static int LuaTracePrint( lua_State* L )
	{
		const char* format = NULL;
		if ( lua_isstring( L, 1 ) )
			format = lua_tostring( L, 1 );
		else
			return 0;
		int argNum = lua_gettop( L );
		if ( argNum > 1 ) {
			char buf[1024] = { 0 };
			char stack[1024] = { 0 };
			va_list args = stack;
			for ( int i = 2; i <= argNum; ++i ) {
				switch ( lua_type( L, i ) ) {
				case LUA_TBOOLEAN:
					args = push_arg< int >( (char*)args, lua_toboolean( L, i ) ? 1 : 0 );
					break;
				case LUA_TSTRING:
					args = push_arg< const char* >( (char*)args, lua_tostring( L, i ) );
					break;
				case LUA_TNUMBER:			
					args = push_arg< int >( (char*)args, (int)lua_tonumber( L, i ) );
					break;
				default:;
				}
			}
			vsnprintf( buf + strlen( buf ), 900, format, stack );
			__TracePrint( "[Script Trace]: %s\n", buf );
			__PrintOnConsole( buf );
		} else {
			__TracePrint( "[Script Trace]: %s\n", format );
			__PrintOnConsole( format );
		}
		return 0;
	}

	static int LuaTraceBreak( lua_State* L )
	{
		if ( lua_isstring( L, 1 ) ) {
			const char* format = NULL;
			format = lua_tostring( L, 1 );
			int argNum = lua_gettop( L );
			if ( argNum > 1 ) {
				char buf[1024];
				char stack[1024];
				va_list args = stack;
				for ( int i = 2; i <= argNum; ++i ) {
					switch ( lua_type( L, i ) ) {
				case LUA_TBOOLEAN:
					args = push_arg< int >( (char*)args, lua_toboolean( L, i ) ? 1 : 0 );
					break;
				case LUA_TSTRING:
					args = push_arg< const char* >( (char*)args, lua_tostring( L, i ) );
					break;
				case LUA_TNUMBER:
					args = push_arg< int >( (char*)args, (int)lua_tonumber( L, i ) );
					break;
				default:;
					}
				}
				strcpy( buf, "[Script]: " );
				vsnprintf( buf + strlen( buf ), 1024, format, stack );
				__TracePrint( buf );
			} else
				__TracePrint( "[Script]: %s\n", format );
		}
		__TracePrint( "[Script] break...\n" );
		for ( ;; ) {
#ifdef GELIB_BUILD_FOR_WINDOWS
			if ( GetAsyncKeyState( VK_F9 ) & 0x8000 )
				break;
#endif
			Misc::sleep(10);
		}
		__TracePrint( "[Script] continue...\n" );
		return 0;
	};

	static int LuaRunFile( lua_State* L )
	{
		lua_pushinteger(L, (int)L);
		lua_gettable( L, LUA_GLOBALSINDEX );
		LuaThreadInfo* node = NULL;
		if ( lua_islightuserdata( L, -1 ) ) {
			node = (LuaThreadInfo*)lua_touserdata( L, -1 );
			if ( lua_isstring( L, 1 ) ) {
				const char* szString = lua_tostring( L, 1 );
				if ( szString )
					_current_context().PostRunFileName.push_back( std::string( szString ) );
			}
		}
		return 0;
	}

	static int LuaGetSourceFile( lua_State* L )
	{
		lua_pushinteger(L, (int)L);
		lua_gettable( L, LUA_GLOBALSINDEX );
		LuaThreadInfo* node = NULL;
		if ( lua_islightuserdata( L, -1 ) ) {
			node = (LuaThreadInfo*)lua_touserdata( L, -1 );
			lua_pop( L, 1 );
			lua_pushstring( L, node->sourceFile.c_str() );
			return 1;
		}
		return 0;
	}

	static size_t memUsed = 0;

	static void *l_alloc (void *ud, void *ptr, size_t osize, size_t nsize) {
		(void)ud;
		(void)osize;
		if (nsize == 0) {
			memUsed -= osize;
			free(ptr);
			return NULL;
		}
		else
		{
			if ( nsize > osize )
				memUsed += nsize - osize;
			else
				memUsed -= osize - nsize;
			return realloc(ptr, nsize);
		}
	}


	static int panic (lua_State *L) {
		(void)L;  /* to avoid warnings */
		fprintf(stderr, "PANIC: unprotected error in call to Lua API (%s)\n",
			lua_tostring(L, -1));
		return 0;
	}

	void cmReportMemory()
	{
		__Trace( "Script memory used %f", memUsed / 1024.0f / 1024.0f );
	}

	static int loader_Luac( lua_State* L ) {
		const char* name = luaL_checkstring( L, 1 );
		std::string filename( name );
		filename.append( ".luac" );
		// load compiled file first
		if ( !Misc::isFile( filename.c_str() ) )
			filename.resize( filename.length() - 1 );	
		if ( luaL_loadfile( L, filename.c_str() ) != 0 ) {
			luaL_error( L, "error loading module " LUA_QS " from file " LUA_QS ":\n\t%s",
				lua_tostring( L, 1 ), filename.c_str(), lua_tostring( L, -1 ) );
		}
		return 1;
	}

	CMResultCode cmInit( )
	{
		_default_context().pBaseState = lua_newstate( l_alloc, NULL );
		if ( !_default_context().pBaseState )
			return SIMR_Failed;
		lua_atpanic( _default_context().pBaseState, &panic );
		luaL_openlibs( _default_context().pBaseState );
		lua_State* L = _default_context().pBaseState;
		lua_getfield( L, LUA_GLOBALSINDEX, "package" );
		if ( lua_istable( L, -1 ) ) {
			lua_getfield( L, -1, "loaders" );
			if ( !lua_istable( L, -1 ) )
				luaL_error( L, LUA_QL( "package.loaders" ) " must be a table" );
			else {
				int n = luaL_getn( L, -1 );
				lua_pushcfunction( L, loader_Luac );
				lua_rawseti( L, -2, n + 1 );
				lua_pop( L, 1 ); // pop package.loaders
			}
			lua_pop( L, 1 ); // pop package
		}
		_default_context().szName = "<default>";
		lua_gc( _default_context().pBaseState, LUA_GCSTOP, 0 );
		lua_pushcfunction( _default_context().pBaseState, LuaWaitFrame );
		lua_setglobal( _default_context().pBaseState, "WaitFrame" );
		lua_pushcfunction( _default_context().pBaseState, LuaWaitTime );
		lua_setglobal( _default_context().pBaseState, "WaitTime" );
		lua_pushcfunction( _default_context().pBaseState, LuaGetRunTime );
		lua_setglobal( _default_context().pBaseState, "GetRunTime" );
		lua_pushcfunction( _default_context().pBaseState, LuaGetElapsedTime );
		lua_setglobal( _default_context().pBaseState, "GetElapsedTime" );
		lua_pushcfunction( _default_context().pBaseState, LuaGetTime );
		lua_setglobal( _default_context().pBaseState, "GetTime" );
		lua_pushcfunction( _default_context().pBaseState, LuaRunFile );
		lua_setglobal( _default_context().pBaseState, "RunFile" );
		lua_pushcfunction( _default_context().pBaseState, LuaTracePrint );
		lua_setglobal( _default_context().pBaseState, "Trace" );
		lua_pushcfunction( _default_context().pBaseState, LuaTracePrint );
		lua_setglobal( _default_context().pBaseState, "trace" );
		lua_pushcfunction( _default_context().pBaseState, LuaTraceBreak );
		lua_setglobal( _default_context().pBaseState, "TraceBreak" );
		lua_pushcfunction( _default_context().pBaseState, LuaGetSourceFile );
		lua_setglobal( _default_context().pBaseState, "GetSourceFile" );
		lua_pushcfunction( _default_context().pBaseState, LuaTerminate );
		lua_setglobal( _default_context().pBaseState, "Terminate" );
		lua_pushcfunction( _default_context().pBaseState, LuaGetInt );
		lua_setglobal( _default_context().pBaseState, "ThreadGetInt" );
		lua_pushcfunction( _default_context().pBaseState, LuaSetInt );
		lua_setglobal( _default_context().pBaseState, "ThreadSetInt" );
		lua_pushcfunction( _default_context().pBaseState, LuaRegisterCallback );
		lua_setglobal( _default_context().pBaseState, "RegisterCallback" );
		lua_pushcfunction( _default_context().pBaseState, LuaInvokeCallback );
		lua_setglobal( _default_context().pBaseState, "InvokeCallback" );
		_Current_LuaContext = &_Default_LuaContext;
		lua_tinker::init( _default_context().pBaseState );
		//------------------------------------------------------------
		//------------------------------------------------------------
		return SIMR_OK;
	}

	CMResultCode cmExecute()
	{
		if ( !_default_context().pBaseState )
			return SIMR_Failed;
		std::string LuaCommLibPath = "Data/Scripts/ScriptApi.lua";
		if ( Misc::isFile( LuaCommLibPath.c_str() ) )
			lua_tinker::dofile( _default_context().pBaseState, LuaCommLibPath.c_str() );
		else {
			LuaCommLibPath.push_back( 'c' );
			if ( Misc::isFile( LuaCommLibPath.c_str() ) )
				lua_tinker::dofile( _default_context().pBaseState, LuaCommLibPath.c_str() );
		}
		lua_tinker::call< void >( _default_context().pBaseState, SCRIPT_INITIALIZE_FUNC_NAME );
		return SIMR_OK;
	}

	CMResultCode cmUninit()
	{
		if ( !_default_context().pBaseState )
			return SIMR_Failed;
		LuaNodeMapIt it, end;
		it = _default_context().Children.begin( );
		end = _default_context().Children.end( );
		while ( it != end ) {
			LuaThreadInfo* node = it->second;
			if ( node->self ) {
				node->destroyLocalMethodsTable();
				luaL_unref( _default_context().pBaseState, LUA_REGISTRYINDEX, node->stateRef );
				node->self = NULL;
			}
			delete it->second;
			++it;
		}
		_default_context().Children.clear();
		lua_close( _default_context().pBaseState );
		_default_context().pBaseState = NULL;
		return SIMR_OK;
	}

	CMContext cmCreateContext( const char* name )
	{
		LuaContext* context = new LuaContext;
		context->szName = name ? name : "<noname>";
		context->pBaseState = luaL_newstate();
		luaopen_base( context->pBaseState );
		luaopen_package( context->pBaseState );
		luaopen_string( context->pBaseState );
		lua_gc( context->pBaseState, LUA_GCSTOP, 0 );
		lua_pushcfunction( context->pBaseState, LuaWaitFrame );
		lua_setglobal( context->pBaseState, "WaitFrame" );
		lua_pushcfunction( context->pBaseState, LuaWaitTime );
		lua_setglobal( context->pBaseState, "WaitTime" );
		lua_pushcfunction( context->pBaseState, LuaGetRunTime );
		lua_setglobal( context->pBaseState, "GetRunTime" );
		lua_pushcfunction( context->pBaseState, LuaRunFile );
		lua_setglobal( context->pBaseState, "RunFile" );
		lua_pushcfunction( context->pBaseState, LuaTracePrint );
		lua_setglobal( context->pBaseState, "TracePrint" );
		lua_pushcfunction( context->pBaseState, LuaTerminate );
		lua_setglobal( context->pBaseState, "Terminate" );
		lua_pushcfunction( context->pBaseState, LuaTerminate );
		lua_setglobal( context->pBaseState, "Terminate" );
		lua_pushcfunction( context->pBaseState, LuaGetInt );
		lua_setglobal( context->pBaseState, "ThreadGetInt" );
		lua_pushcfunction( context->pBaseState, LuaSetInt );
		lua_setglobal( context->pBaseState, "ThreadSetInt" );
		lua_pushcfunction( context->pBaseState, LuaRegisterCallback );
		lua_setglobal( context->pBaseState, "RegisterCallback" );
		lua_pushcfunction( context->pBaseState, LuaInvokeCallback );
		lua_setglobal( context->pBaseState, "InvokeCallback" );
		return reinterpret_cast< CMContext >( context );
	}

	CMResultCode cmDestroyContext( CMContext _context )
	{
		LuaContext* context = reinterpret_cast< LuaContext* >( _context );
		if ( context ) {
			LuaNodeMapIt it, end;
			it = context->Children.begin( );
			end = context->Children.end( );
			while ( it != end ) {
				LuaThreadInfo* node = it->second;
				if ( node->self ) {
					node->destroyLocalMethodsTable();
					luaL_unref( context->pBaseState, LUA_REGISTRYINDEX, node->stateRef );
					node->self = NULL;
				}
				delete it->second;
				++it;
			}
			context->Children.clear();
			if ( context->pBaseState )
				lua_close( context->pBaseState );
			delete reinterpret_cast< LuaContext* >( context );
		}
		return SIMR_OK;
	}

	CMResultCode cmMakeCurrent( CMContext context )
	{
		if ( !context )
			_Current_LuaContext = &_Default_LuaContext;
		else
			_Current_LuaContext = reinterpret_cast< LuaContext* >( context );
		assert( _Current_LuaContext );
		return SIMR_OK;
	}

	CMContext cmGetCurrentContext()
	{
		return reinterpret_cast< CMContext >( _Current_LuaContext );
	}

	CMScript cmCreateThread()
	{
		lua_State* state = lua_newthread( _current_context().pBaseState );
		if ( !state )
			return NULL;
		LuaThreadInfo* node = new LuaThreadInfo;
		node->self = state;
		node->stateRef = luaL_ref( _current_context().pBaseState, LUA_REGISTRYINDEX );
		node->createLocalMethodsTable();
		lua_pushinteger(state, (int)state);
		lua_pushlightuserdata( state, node );
		lua_settable( state, LUA_GLOBALSINDEX );
		return reinterpret_cast< CMScript >( node );
	}

	CMScript cmCompileSource( const char* file )
	{
		lua_State* state = lua_newthread( _current_context().pBaseState );
		int stateRef = luaL_ref( _current_context().pBaseState, LUA_REGISTRYINDEX );
		
		if ( !state )
			return NULL;
		if ( 0 == luaL_loadfile( state, file ) ) {
			LuaThreadInfo* node = new LuaThreadInfo;
			node->self = state;
			node->sourceFile = file;
			node->stateRef = stateRef;
			node->createLocalMethodsTable();

			lua_pushinteger(state, (int)state);
			lua_pushlightuserdata( state, node );
			lua_settable( state, LUA_GLOBALSINDEX );

			lua_resume( node->self, 0 );
			return reinterpret_cast< CMScript >( node );
		}
		return NULL;
	}

	CMScript cmCompileSourceFromString( const char* str, CMRawFunction compileErrorHandle,
								 CMRawFunction runtimeErrorHandle )
	{		
		if ( !compileErrorHandle )
			compileErrorHandle = lua_error_with_compile;
		if ( !runtimeErrorHandle )
			runtimeErrorHandle = lua_error_with_running;
		lua_State* state = lua_newthread( _current_context().pBaseState );
		int stateRef = luaL_ref( _current_context().pBaseState, LUA_REGISTRYINDEX );
		if ( !state )
			return NULL;		
		lua_pushcclosure( state, runtimeErrorHandle, 0 );
		int errfunc = lua_gettop( state );
		if ( 0 == luaL_loadstring( state, str ) ) {
			LuaThreadInfo* node = new LuaThreadInfo;
			node->self = state;
			node->stateRef = stateRef;
			node->createLocalMethodsTable();

			lua_pushinteger( state, (int)state );
			lua_pushlightuserdata( state, node );
			lua_settable( state, LUA_GLOBALSINDEX );

			lua_resume( node->self, 0 );
			if ( 0 == lua_status( node->self ) ) {
			} else {
				if ( runtimeErrorHandle ) {
					runtimeErrorHandle( node->self );
				}
			}
			return reinterpret_cast< CMScript >( node );
		} else {
			if ( compileErrorHandle ) {
				compileErrorHandle( state );
			}
		}
		return NULL;
	}

	CMResultCode cmRunFile( const char* file, const char* entry, CMScript script, CMBank _bank, lua_CFunction _error_func_compile, lua_CFunction _error_func_running,
							void* userData, EntryParamsSender* entryParamsSender )
	{
		if ( !_error_func_compile )
			_error_func_compile = lua_error_with_compile;
		if ( !_error_func_running )
			_error_func_running = lua_error_with_running;
		LuaThreadInfo* node = reinterpret_cast< LuaThreadInfo* >( script );
		if ( node && lua_status( node->self ) == LUA_YIELD )
			return SIMR_Failed;
		if ( !node && !_bank ) {
			lua_State* state = lua_newthread( _current_context().pBaseState );
			if ( !state )
				return SIMR_Failed;
			node = new LuaThreadInfo;
			node->self = state;
			node->stateRef = luaL_ref( _current_context().pBaseState, LUA_REGISTRYINDEX );
			node->createLocalMethodsTable();
			lua_pushinteger( state, (int)state );
			lua_pushlightuserdata( state, node );
			lua_settable( state, LUA_GLOBALSINDEX );
			_current_context().Children[ state ] = node;
			script = reinterpret_cast< CMScript >( node );
		}

		lua_pushstring( node->self, SCRIPT_USER_DATA_NAME );
		lua_pushlightuserdata( node->self, userData );
		lua_settable( node->self, LUA_GLOBALSINDEX );

		CMResultCode hr = SIMR_OK;
		lua_pushcclosure( node->self, _error_func_running, 0 );
		int errfunc = lua_gettop( node->self );
		if ( 0 == luaL_loadfile( node->self, file ) ) {
			node->sourceFile = file;
			lua_resume( node->self, 0 );
			if ( 0 == lua_status( node->self ) ) {
				lua_getglobal( node->self, entry ? entry : SCRIPT_ENTRY_NAME );
				if ( lua_isfunction( node->self, -1 ) ) {
					int narg = 0;
					if ( entryParamsSender )
						narg = entryParamsSender->pushParams( node->self, entryParamsSender->userData );
					if ( 0 == lua_pcall( node->self, narg, 0, errfunc ) ) {
						int s = lua_status( node->self );
						if ( LUA_YIELD == s || 0 == s )
							hr = SIMR_OK;
					} else {
						if ( entryParamsSender )
							entryParamsSender->popParams( node->self, entryParamsSender->userData );
						hr = SIMR_Failed;
					}
				}
			} else {
				__Trace( "[ScriptApi::cmRunFile]: Entry function[%s] will not be called!", entry ? entry : SCRIPT_ENTRY_NAME );
			}
		} else {
			if ( _error_func_compile )
				_error_func_compile( node->self );
		}
		if ( hr != SIMR_OK ) {
			lua_pushinteger( node->self, (int)node->self );
			lua_pushnil( node->self );
			lua_settable( node->self, LUA_GLOBALSINDEX );

			node->destroyLocalMethodsTable();
			luaL_unref( node->self, LUA_REGISTRYINDEX, node->stateRef );
			node->stateRef = 0;
		}
		if ( _bank && node->stateRef != 0 ) {
			ScriptBank* bank = reinterpret_cast< ScriptBank* >( _bank );
			bank->scripts.push_back( script );
			node->bank = bank;
		}
		return hr;
	}

	CMResultCode cmRunString( const char* source, const char* entry, CMScript script, CMBank _bank, lua_CFunction _error_func_compile, 
							  lua_CFunction _error_func_running,
							  void *userData, EntryParamsSender* entryParamsSender )
	{
		if ( !_error_func_compile )
			_error_func_compile = lua_error_with_compile;
		if ( !_error_func_running )
			_error_func_running = lua_error_with_running;
		LuaThreadInfo* node = reinterpret_cast< LuaThreadInfo* >( script );
		if ( node && lua_status( node->self ) == LUA_YIELD )
			return SIMR_Failed;
		if ( !node && !_bank ) {
			lua_State* state = lua_newthread( _current_context().pBaseState );
			if ( !state )
				return SIMR_Failed;
			node = new LuaThreadInfo;
			node->self = state;
			node->stateRef = luaL_ref( _current_context().pBaseState, LUA_REGISTRYINDEX );
			node->createLocalMethodsTable();
			lua_pushinteger( state, (int)state );
			lua_pushlightuserdata( state, node );
			lua_settable( state, LUA_GLOBALSINDEX );
			_current_context().Children[ state ] = node;
			script = reinterpret_cast< CMScript >( node );
		}
		if ( userData ) {
			lua_pushstring( node->self, SCRIPT_USER_DATA_NAME );
			lua_pushlightuserdata( node->self, userData );
			lua_settable( node->self, LUA_GLOBALSINDEX );
		}

		CMResultCode hr = SIMR_OK;
		lua_pushcclosure( node->self, _error_func_running, 0 );
		int errfunc = lua_gettop( node->self );
		if ( 0 == luaL_loadstring( node->self, source ) ) {
			lua_resume( node->self, 0 );
			if ( 0 == lua_status( node->self ) ) {
				lua_getglobal( node->self, entry ? entry : SCRIPT_ENTRY_NAME );
				if ( lua_isfunction( node->self, -1 ) ) {
					int narg = 0;
					if ( entryParamsSender )
						narg = entryParamsSender->pushParams( node->self, entryParamsSender->userData );
					if ( 0 == lua_pcall( node->self, 0, 0, errfunc ) ) {
						int s = lua_status( node->self );
						if ( LUA_YIELD == s || 0 == s )
							hr = SIMR_OK;
					} else {
						if ( entryParamsSender )
							entryParamsSender->popParams( node->self, entryParamsSender->userData );
						hr = SIMR_Failed;
					}
				}
			} else {
				__Trace( "[ScriptApi::cmRunFile]: Entry function[%s] will not be called!", entry ? entry : SCRIPT_ENTRY_NAME );
			}
		} else if ( _error_func_compile ){
			if ( _error_func_compile )
				_error_func_compile( node->self );
		}
		if ( hr != SIMR_OK ) {
			lua_pushinteger(node->self, (int)node->self);
			lua_pushnil(node->self);
			lua_settable( node->self, LUA_GLOBALSINDEX );

			node->destroyLocalMethodsTable();
			luaL_unref( node->self, LUA_REGISTRYINDEX, node->stateRef );
			node->stateRef = 0;
		}
		if ( _bank && node->stateRef != 0 ) {
			ScriptBank* bank = reinterpret_cast< ScriptBank* >( _bank );
			bank->scripts.push_back( script );
			node->bank = bank;
		}
		return hr;
	}

	CMResultCode cmClose( CMScript script )
	{
		LuaThreadInfo* node = reinterpret_cast< LuaThreadInfo* >( script );
		if ( !node )
			return SIMR_OK;
		LuaNodeMapIt it = _current_context().Children.find( node->self );
		if ( _current_context().Children.end() != it ) {
			// call terminate handler if any
			// do not call lua_resume
			lua_State* L = node->self;
			lua_pushinteger( node->self, (int)node->self );
			lua_pushnil( node->self );
			lua_settable( node->self, LUA_GLOBALSINDEX );

			node->destroyLocalMethodsTable();
			luaL_unref( _current_context().pBaseState, LUA_REGISTRYINDEX, node->stateRef );
			node->self = NULL;
			node->stateRef = 0;
		} else {
			node->destroyLocalMethodsTable();
			luaL_unref( _current_context().pBaseState, LUA_REGISTRYINDEX, node->stateRef );
			node->self = NULL;
			node->stateRef = 0;
			if ( node->bank == NULL )
				delete node;
		}
		return SIMR_OK;
	}

	bool cmIsValid( CMScript script )
	{
		if ( !script )
			return false;
		LuaThreadInfo* node = reinterpret_cast< LuaThreadInfo* >( script );
		return node->self && lua_status( node->self ) == 0;
	}

	bool cmIsDead( CMScript script )
	{
		if ( !script )
			return true;
		LuaThreadInfo* node = reinterpret_cast< LuaThreadInfo* >( script );
		return !node->self || node->exit || node->self && lua_status( node->self ) == 0;
	}

	const char*	cmGetName( CMContext _context )
	{
		assert( _context );
		LuaContext* context = reinterpret_cast< LuaContext* >( _context );
		return context->szName.c_str();
	}

	const char* cmGetSourceFile( CMScript script )
	{
		if ( !script )
			return NULL;
		LuaThreadInfo* node = reinterpret_cast< LuaThreadInfo* >( script );
		return node->sourceFile.c_str();
	}

	const char* cmGetCommandLine( CMScript script )
	{		
		if ( !script )
			return NULL;
		LuaThreadInfo* node = reinterpret_cast< LuaThreadInfo* >( script );
		return node->commandLine.c_str();
	}
	
	bool cmSetCommandLine( CMScript script, const char* commandLine )
	{	
		if ( !script || !commandLine )
			return false;
		LuaThreadInfo* node = reinterpret_cast< LuaThreadInfo* >( script );
		node->commandLine = commandLine;
		return true;
	}
	
	const CMThreadLocalStore& cmGetThreadLocalStore( CMScript script )
	{
		assert( script );
		LuaThreadInfo* node = reinterpret_cast< LuaThreadInfo* >( script );
		return node->tls;
	}

	CMScript cmFindWithSourceName( CMBank bank, const char* sourceName )
	{
		ScriptBank* _tbank = reinterpret_cast< ScriptBank* >( bank );
		if ( bank ) {
			std::vector< CMScript >::iterator it = _tbank->scripts.begin();
			while ( it != _tbank->scripts.end() ) {
				CMScript& s = *it++;
				LuaThreadInfo* node = reinterpret_cast< LuaThreadInfo* >( s );
				if ( node->self != NULL && node->sourceFile == sourceName ) {
					return s;
				}
			}
		}
		return NULL;
	}

	bool cmSetThreadLocalStore( CMScript script, const CMThreadLocalStore& tls )
	{
		if ( !script ) return false;
		LuaThreadInfo* node = reinterpret_cast< LuaThreadInfo* >( script );
		node->tls = tls;
		return true;
	}

	const std::pair< CMScript, const char* >* cmEnumerateSource( CMBank bank, int* num )
	{
		assert( num );
		static std::vector< std::pair< CMScript, const char* > > out;
		out.clear();
		ScriptBank* _tbank = reinterpret_cast< ScriptBank* >( bank );
		if ( bank ) {
			std::vector< CMScript >::iterator it = _tbank->scripts.begin();
			while ( it != _tbank->scripts.end() ) {
				CMScript& s = *it++;
				LuaThreadInfo* node = reinterpret_cast< LuaThreadInfo* >( s );
				if ( node->self != NULL && !node->sourceFile.empty() ) {
					out.push_back( std::make_pair( s, node->sourceFile.c_str() ) );
				}
			}
		}
		*num = (int)out.size();
		return out.empty() ? NULL : &*out.begin();
	}

	bool cmIsYield( CMScript script )
	{
		if ( !script )
			return false;
		LuaThreadInfo* node = reinterpret_cast< LuaThreadInfo* >( script );
		return node->self && lua_status( node->self ) == LUA_YIELD;
	}

	CMResultCode cmResume( CMScript script )
	{	
		LuaThreadInfo* node = reinterpret_cast< LuaThreadInfo* >( script );
		if ( !node || !node->self )
			return SIMR_Failed;
		if ( lua_status( node->self ) == LUA_YIELD ) {
			lua_resume( node->self, 0 );
			if ( lua_status( node->self ) == 0 )
				return SIMR_OK;
		}
		return SIMR_Failed;
	}

	static void _runNode( LuaThreadInfo* node, int timeMS )
	{
		if ( node->stateRef != 0 ) {
			_current_context().currentThread = node;
			if ( node->exit || node->waitTime <= 0 && node->waitFrame <= 0 ) {
				int status = lua_status( node->self );
				if ( status != LUA_YIELD )
					node->exit = true; //< if you got here that means this thread has exit normally.
				if ( node->exit || LUA_YIELD != lua_resume( node->self, 0 ) ) {
					lua_pushinteger( node->self, (int)node->self );
					lua_pushnil( node->self );
					lua_settable( node->self, LUA_GLOBALSINDEX );

					node->destroyLocalMethodsTable();
					luaL_unref( _current_context().pBaseState, LUA_REGISTRYINDEX, node->stateRef );
					node->stateRef = 0;
					node->self = NULL;
				}
			} else {
				if ( node->waitFrame > 0 )
					--node->waitFrame;
				else if ( node->waitTime > 0 )
					node->waitTime -= timeMS;
			}
			node->runTime += timeMS;
			_current_context().currentThread = NULL;
		}
	}

	CMResultCode cmRunScript( CMScript script, int timeMS )
	{
		LuaThreadInfo* node = reinterpret_cast< LuaThreadInfo* >( script );
		if ( !node || node->exit )
			return SIMR_Failed;
		_runNode( node, timeMS );
		return SIMR_OK;
	}

	CMResultCode cmRunScript( CMBank bank, int timeMS )
	{
		CMResultCode rtv = SIMR_OK;
		ScriptBank* _tbank = reinterpret_cast< ScriptBank* >( bank );
		if ( bank ) {
			std::vector< CMScript >::iterator it = _tbank->scripts.begin();
			while ( it != _tbank->scripts.end() ) {
				CMScript& s = *it;
				if ( cmIsYield( s ) )
					rtv = cmRunScript( s, timeMS );
				LuaThreadInfo* node = reinterpret_cast< LuaThreadInfo* >( s );
				assert( node->bank );
				if ( node->self == NULL ) {
					delete node;
					it = _tbank->scripts.erase( it );
				} else {
					++it;
				}
			}
		}
		lua_gc( _current_context().pBaseState, LUA_GCSTEP, 1 );
		return rtv;
	}

	CMResultCode cmRun( int timeMS )
	{
		LuaNodeMapIt it = _current_context().Children.begin();
		LuaNodeMapIt end = _current_context().Children.end();
		while ( it != end ) {
			LuaThreadInfo* node = it->second;
			_runNode( node, timeMS );
			++it;
		}
		it = _current_context().Children.begin( );
		while ( it != _current_context().Children.end( ) ) {
			LuaThreadInfo* node = it->second;
			if ( node->stateRef == 0 ) {
				delete node;
				_current_context().Children.erase( it++ );
			} else
				++it;
		}
		if ( !_current_context().PostRunFileName.empty() ) {
			size_t n = _current_context().PostRunFileName.size();
			for ( size_t i = 0; i < n; ++i )
				if ( SIMR_OK != cmRunFile( _current_context().PostRunFileName[ i ].szFileName.c_str(), NULL, NULL, NULL, cmOnError, cmOnError) )
					__TracePrint( "cmRunFile failed: %s\n", _current_context().PostRunFileName[ i ].szFileName.c_str() );
			_current_context().PostRunFileName.clear();
		}
		lua_gc( _current_context().pBaseState, LUA_GCSTEP, 1 );
		return SIMR_OK;
	}

	CMBank cmCreateBank()
	{
		return reinterpret_cast< CMBank >( new ScriptBank );
	}

	CMResultCode cmCloseAll( CMBank _bank )
	{
		ScriptBank* bank = reinterpret_cast< ScriptBank* >( _bank );
		if ( bank ) {
			size_t n = bank->scripts.size();
			for ( size_t i = 0; i < n; ++i ) {
				if ( SIMR_OK != cmClose( bank->scripts[ i ] ) )
					__TracePrint( "cmCloseAll : cmClose failed!\n" );
				LuaThreadInfo* node = reinterpret_cast< LuaThreadInfo* >( bank->scripts[ i ] );
				delete node;
			}
			bank->scripts.clear();
		} else {
			LuaNodeMapIt it = _current_context().Children.begin();
			LuaNodeMapIt end = _current_context().Children.end();
			while ( it != end ) {
				LuaThreadInfo* node = it->second;
				cmClose( reinterpret_cast< CMScript >( node ) );
				++it;
			}
		}
		return SIMR_OK;
	}

	CMResultCode cmDestroyBank( CMBank _bank )
	{
		if ( !_bank ) return SIMR_OK;
		cmCloseAll( _bank );
		ScriptBank* bank = reinterpret_cast< ScriptBank* >( _bank );
		if ( bank )
			delete bank;
		return SIMR_OK;
	}

	CMScript cmGetCurrentRunningScript()
	{
		return reinterpret_cast< CMScript >( _current_context().currentThread );
	}

	bool cmRegisterRawFunction( const char* methodName, lua_CFunction method )
	{
		lua_pushcfunction( _internal::_current_lua_context(), method );
		lua_setglobal( _internal::_current_lua_context(), methodName );
		return true;
	}

	bool cmQueryThreadLocalStoreValue( const CMThreadLocalStore& lts, const char* name, int* value )
	{
		assert( value != NULL && name );
		CMThreadLocalStore::const_iterator it = lts.find( name );
		if ( it != lts.end() ) {
			*value = it->second;
			return true;
		}
		return false;
	}
	// copy follow error handle functions from lut_tinker
	static void cmPrintError( lua_State* L, const char* fmt, ... )
	{
		char text[4096];

		va_list args;
		va_start(args, fmt);
		vsnprintf(text, 4096, fmt, args);
		va_end(args);

		__TracePrint("%s\n", text);
	}

	static void cmCallStack( lua_State* L, int n )
	{
		lua_Debug ar;
		if(lua_getstack(L, n, &ar) == 1)
		{
			lua_getinfo(L, "nSlu", &ar);

			const char* indent;
			if(n == 0)
			{
				indent = "->\t";
				cmPrintError(L, "\t<call stack>");
			}
			else
			{
				indent = "\t";
			}

			if(ar.name)
				cmPrintError(L, "%s%s() : line %d [%s : line %d]", indent, ar.name, ar.currentline, ar.source, ar.linedefined);
			else
				cmPrintError(L, "%sunknown : line %d [%s : line %d]", indent, ar.currentline, ar.source, ar.linedefined);

			cmCallStack(L, n+1);
		}
	}

	int cmOnError( lua_State* L )
	{
		cmPrintError(L, "%s", lua_tostring(L, -1));

		cmCallStack(L, 0);

		return 0;
	}
}

//EOF
