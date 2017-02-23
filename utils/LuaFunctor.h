#ifndef __LUA_FUNCTOR_H__
#define __LUA_FUNCTOR_H__

#include "lua_tinker.h"

class LuaStateHolder
{
public:
	LuaStateHolder();
	~LuaStateHolder();
	static bool isValid( lua_State* L );
	static void registerToLuaHook( lua_State* L );
};

class LuaFunctor {
	typedef int LFun;
public:
	LuaStateHolder holder;
	lua_State* L;
	LFun func;
	int* ref;
public:
	LuaFunctor();
	~LuaFunctor();
	LuaFunctor( const LuaFunctor& o );
	LuaFunctor& operator = ( const LuaFunctor& o );
	bool bind( lua_State* L, LFun luaFunc );
	void unbind();
	int rawCall( int argn );
public:
	static void registerToLuaHook( lua_State* L );
	static int registerCallback( lua_State* L );
	static int rawCall( const char* name, int argn );
	static bool pushFunction( LFun func, lua_State* L );
	static bool pushFunctionByName( const char* funcName, lua_State* L = NULL );
};




#endif
//EOF
