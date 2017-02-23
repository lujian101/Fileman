#include "LuaFunctor.h"
#include <map>
#include <string>


LuaStateHolder::LuaStateHolder()
{
	int a=0;
}

LuaStateHolder::~LuaStateHolder()
{
	int a=0;
}
static int _LuaStateHolderDestroyer( lua_State *L )
{
	( ( LuaStateHolder* )lua_touserdata( L, 1 ) )->~LuaStateHolder();
	return 0;
}

void LuaStateHolder::registerToLuaHook( lua_State* L )
{
	const char* name = "__LuaStateHolder";
	lua_tinker::class_name< LuaStateHolder >::name( name );
	lua_pushstring( L, name );
	lua_newtable( L );

	lua_pushstring( L, "__name" );
	lua_pushstring( L, name );
	lua_rawset( L, -3 );

	lua_pushstring( L, "__index" );
	lua_pushcclosure(L, lua_tinker::meta_get, 0 );
	lua_rawset( L, -3 );

	lua_pushstring( L, "__newindex" );
	lua_pushcclosure( L, lua_tinker::meta_set, 0 );
	lua_rawset( L, -3 );

	lua_pushstring( L, "__gc" );
	lua_pushcclosure( L, _LuaStateHolderDestroyer, 0 );
	lua_rawset( L, -3 );

	lua_settable( L, LUA_GLOBALSINDEX );

	lua_tinker::class_con< LuaStateHolder >( L, lua_tinker::constructor< LuaStateHolder > );
}

bool LuaStateHolder::isValid( lua_State* L )
{
	L;
	return false;
}

struct LuaFunctionPool
{
	typedef std::map< std::string, LuaFunctor > FuncTable;
	FuncTable funcs;
};

static inline LuaFunctionPool& _getGP() {
	static LuaFunctionPool lfp;
	return lfp;
}

LuaFunctor::LuaFunctor() : L( NULL ), func( 0 ), ref( NULL )
{
}

LuaFunctor::~LuaFunctor()
{
	unbind();
}

bool LuaFunctor::bind( lua_State* L, int func )
{
	if ( L && func ) {
		lua_rawgeti( L, LUA_REGISTRYINDEX, func );
		if ( lua_isfunction( L, -1 ) ) {
			unbind();
			this->L = L;
			this->func = func;
			this->ref = new int( 1 );
			return true;
		}
	}
	return false;
}

void LuaFunctor::unbind()
{
	if ( ref ) {
		--( *ref );
		if ( *ref == 0 ) {
			lua_unref( L, func );
			delete ref;	
		}
		ref = NULL;
		L = NULL;
		func = NULL;
	}
}

LuaFunctor::LuaFunctor( const LuaFunctor& o )
{
	if ( o.ref ) {
		L = o.L;
		func = o.func;
		ref = o.ref;
		if ( ref ) {
			++( *ref );
		}
	} else {
		L = NULL;
		ref = NULL;
		func = NULL;
	}
}

LuaFunctor& LuaFunctor::operator = ( const LuaFunctor& o )
{
	if ( this != &o ) {
		unbind();
		if ( o.ref ) {
			L = o.L;
			func = o.func;
			ref = o.ref;
			if ( ref ) {
				++( *ref );
			}
		}
	}
	return *this;
}


bool LuaFunctor::pushFunctionByName( const char* funcName, lua_State* L )
{
	LuaFunctionPool::FuncTable::iterator it = _getGP().funcs.find( funcName );
	if ( it != _getGP().funcs.end() ) {
		return pushFunction( it->second.func, L ? L : it->second.L );
	}
	return true;
}

bool LuaFunctor::pushFunction( LFun func, lua_State* L )
{
	lua_rawgeti( L, LUA_REGISTRYINDEX, func );
	if ( !lua_isfunction( L, -1 ) ) {
		lua_pop( L, 1 );
		return false;
	}
	return true;
}

int LuaFunctor::rawCall( int argn )
{
	int ret = 0;
	if ( pushFunction( func, L ) ) {
		if ( argn > 0 ) {
			lua_insert( L, -( argn + 1 ) );
		}
		int error = lua_pcall( L, argn, 1, 0 );
		if ( error ) {
			printf( "%s", lua_tostring( L, -1 ) );
			lua_settop( L, 0 );
			return ret;
		}
		if ( lua_isnumber( L, -1 ) ) {
			ret = (int)lua_tointeger( L, -1 );
		} else if ( lua_isboolean( L, -1 ) ) {
			ret = lua_toboolean( L, -1 );
		}
		lua_pop( L, 1 );
		return ret;
	} else {
		return ret;
	}
}

void LuaFunctor::registerToLuaHook( lua_State* L )
{
	lua_pushcfunction( L, registerCallback );
	lua_setglobal( L, "registerCallback" );
}

int LuaFunctor::registerCallback( lua_State* L )
{
	const char* s = lua_tostring( L, 1 );
	if ( lua_isfunction( L, 2 ) ) {
		LuaStateHolder* holder = NULL;
		lua_getglobal( L, "__LuaStateHolder__" );
		if ( !lua_isnil( L, -1 ) ) {
			holder = lua_tinker::pop< LuaStateHolder* >( L );	
		}
		if ( !holder ) {
			lua_tinker::constructor< LuaStateHolder >( L );
			lua_setglobal( L, "__LuaStateHolder__" );
		}
		
		lua_pushvalue( L, 2 );
		int funcRef = luaL_ref( L, LUA_REGISTRYINDEX );
		LuaFunctor f;
		f.bind( L, funcRef );
		_getGP().funcs[ s ] = f;
	}
	lua_pushboolean( L, 1 );
	return 1;
}

int LuaFunctor::rawCall( const char* name, int argn )
{
	LuaFunctionPool::FuncTable::iterator it = _getGP().funcs.find( name );
	if ( it != _getGP().funcs.end() ) {
		return it->second.rawCall( argn );
	} else {
		return 0;
	}
}

//EOF
