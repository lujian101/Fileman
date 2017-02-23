#ifndef __SCRIPT_API_H__
#define __SCRIPT_API_H__

#include "lua_tinker.h"
#include <map>
extern const char* SCRIPT_ENTRY_NAME;
extern const char* SCRIPT_EXIT_CALLBACK_NAME;
extern const char* SCRIPT_INITIALIZE_FUNC_NAME;
extern const char* SCRIPT_USER_DATA_NAME;

namespace _internal
{
	extern lua_State* _current_lua_context();
}

namespace crim
{
	struct _CMContext;
	struct _CMBank;
	struct _CMScript;

	typedef _CMContext* CMContext;
	typedef _CMBank* CMBank;
	typedef _CMScript* CMScript;
	typedef lua_CFunction CMRawFunction;
	typedef std::map< std::string, int > CMThreadLocalStore;

	namespace _internal
	{
		lua_State* _current_lua_context();
		lua_State* _script_2_lua_state( CMScript script );
	}

	template< typename TClass >
	bool cmRegisterClass( const char* name )
	{
		lua_tinker::class_add< TClass >( _internal::_current_lua_context(), name );
		return true;
	}

	template< typename TClass, typename TParentClass >
	bool cmRegisterClass( const char* name )
	{
		lua_tinker::class_add< TClass >( _internal::_current_lua_context(), name );
		lua_tinker::class_inh< TClass, TParentClass >( _internal::_current_lua_context() );
		return true;
	}

	template< typename TClass >
	bool cmRegisterClassConstructor( )
	{
		lua_tinker::class_con< TClass >( _internal::_current_lua_context(), lua_tinker::constructor< TClass > );
		return true;
	}

	template< typename TClass, typename TParam1 >
	bool cmRegisterClassConstructor( )
	{
		lua_tinker::class_con<TClass>( _internal::_current_lua_context(), lua_tinker::constructor< TClass, TParam1 > );
		return true;
	}

	template< typename TClass, typename TParam1, typename TParam2 >
	bool cmRegisterClassConstructor( )
	{
		lua_tinker::class_con< TClass >( _internal::_current_lua_context(), lua_tinker::constructor< TClass, TParam1, TParam2 > );
		return true;
	}

	template< typename TClass, typename TParam1, typename TParam2, typename TParam3 >
	bool cmRegisterClassConstructor( )
	{
		lua_tinker::class_con<TClass>( _internal::_current_lua_context(), lua_tinker::constructor< TClass, TParam1, TParam2, TParam3 > );
		return true;
	}

	template< typename TPropertyType >
	bool cmDeclGlobal( const char* proName, TPropertyType globalVariable )
	{
		lua_tinker::set( _internal::_current_lua_context(), proName, globalVariable );
		return true;
	}

	template< typename TPropertyType >
	bool cmRegisterProperty( const char* proName, TPropertyType* globalVariable )
	{
		lua_tinker::set( _internal::_current_lua_context(), proName, globalVariable );
		return true;
	}

	template< typename TPropertyType, typename TOwnerClass >
	bool cmRegisterProperty( const char* proName, TPropertyType TOwnerClass::*pVar )
	{
		lua_tinker::class_mem< TOwnerClass >( _internal::_current_lua_context(), proName, pVar );
		return true;
	}

	template< typename TClass, typename TMethodType >
	bool cmRegisterClassMethod( const char* methodName, TMethodType method )
	{
		lua_tinker::class_def< TClass >( _internal::_current_lua_context(), methodName, method );
		return true;
	}

	template< typename TMethodType >
	bool cmRegisterGlobalMethod( const char* methodName, TMethodType method )
	{
		lua_tinker::def( _internal::_current_lua_context(), methodName, method );
		return true;
	}

	template< typename RVal >
	RVal cmCall( CMScript s, const char* name )
	{
		return lua_tinker::call< RVal >( _internal::_script_2_lua_state(s), name );
	}

	template< typename RVal, typename T1 >
	RVal cmCall( CMScript s, const char* name, T1 arg1 )
	{
		return lua_tinker::call< RVal, T1 >( _internal::_script_2_lua_state(s), name, arg1 );
	}

	template< typename RVal, typename T1, typename T2 >
	RVal cmCall( CMScript s, const char* name, T1 arg1, T2 arg2 )
	{
		return lua_tinker::call< RVal, T1, T2 >( _internal::_script_2_lua_state(s), name, arg1, arg2 );
	}

	template< typename RVal, typename T1, typename T2, typename T3 >
	RVal cmCall( CMScript s, const char* name, T1 arg1, T2 arg2, T3 arg3 )
	{
		return lua_tinker::call< RVal, T1, T2, T3 >( _internal::_script_2_lua_state(s), name, arg1, arg2, arg3 );
	}

	int cmOnError( lua_State* L );

	enum CMResultCode
	{
		SIMR_OK = 0,
		SIMR_Failed,
	};

	struct EntryParamsSender
	{
		void* userData;
		int ( *pushParams )( lua_State*, void* );
		int ( *popParams )( lua_State*, void* );
		EntryParamsSender() : userData( NULL ), pushParams( NULL ), popParams( NULL ){}
	};

	CMResultCode	cmInit();

	CMResultCode	cmUninit();

	CMResultCode	cmExecute();
	// create new env
	CMContext		cmCreateContext( const char* name = NULL );

	CMResultCode	cmDestroyContext( CMContext context );

	// set current current to run script
	CMResultCode	cmMakeCurrent( CMContext context = NULL );

	CMContext		cmGetCurrentContext();

	// create a empty thread
	CMScript		cmCreateThread();

	// create a new thread and load source file to compile as a function package
	CMScript		cmCompileSource( const char* file );

	CMScript		cmCompileSourceFromString( const char* str, CMRawFunction compileErrorHandle = NULL,
								 CMRawFunction runtimeErrorHandle = NULL );

	// run file script in "Yield-able" way
	CMResultCode	cmRunFile( const char* file, const char* entry = NULL,
								 CMScript script = NULL, CMBank bank = NULL,
								 CMRawFunction compileErrorHandle = NULL,
								 CMRawFunction runtimeErrorHandle = NULL, 
								 void* userData = NULL, EntryParamsSender* entryParamsSender = NULL );

	// run string script in "Yield-able" way
	CMResultCode	cmRunString( const char* source, const char* entry = NULL,
								 CMScript script = NULL, CMBank bank = NULL,
								 CMRawFunction compileErrorHandle = NULL,
								 CMRawFunction runtimeErrorHandle = NULL,
								 void* userData = NULL, EntryParamsSender* entryParamsSender = NULL );

	CMResultCode	cmClose( CMScript script );

	CMResultCode	cmResume( CMScript script );

	// tick function
	CMResultCode	cmRun( int timeMS );

	CMResultCode	cmRunScript( CMScript script, int timeMS );

	CMResultCode	cmRunScript( CMBank bank, int timeMS );

	CMBank			cmCreateBank();

	CMResultCode	cmCloseAll( CMBank bank );

	CMResultCode	cmDestroyBank( CMBank bank );

	bool			cmIsValid( CMScript script );

	bool			cmIsDead( CMScript script );

	bool			cmIsYield( CMScript script );

	// debug info
	const char*		cmGetName( CMContext context );

	const char*		cmGetSourceFile( CMScript script );

	const char*		cmGetCommandLine( CMScript script );
	
	bool			cmSetCommandLine( CMScript script, const char* commandLine );

	const CMThreadLocalStore& cmGetThreadLocalStore( CMScript script );
	
	bool			cmSetThreadLocalStore( CMScript script, const CMThreadLocalStore& tls );

	const std::pair< CMScript, const char* >*	cmEnumerateSource( CMBank bank, int* num );

	CMScript		cmFindWithSourceName( CMBank bank, const char* sourceName );

	CMScript		cmGetCurrentRunningScript();

	bool			cmRegisterRawFunction( const char* methodName, CMRawFunction method );

	bool			cmQueryThreadLocalStoreValue( const CMThreadLocalStore& lts, const char* name, int* value );

	struct CMContextGuard {
		CMContextGuard( CMContext cur = NULL ) : old( NULL ) { if ( cur ) { old = cmGetCurrentContext(); cmMakeCurrent( cur ); } }
		~CMContextGuard() { if ( old ) cmMakeCurrent( old ); }
	private:
		CMContext old;
	};

	template< class T >
	int push_object( lua_State* L, void* _this )
	{
		T* ptr = ( T* )_this;
		GE_ASSERT( ptr );
		lua_tinker::push( L, ptr );
		return 1;
	}

	template< class T >
	int pop_object( lua_State* L, void* _this )
	{
		T* ptr = ( T* )_this;
		GE_ASSERT( ptr );
		T* _ptr = lua_tinker::pop< T* >( L );
		GE_ASSERT( ptr == _ptr );
		return 1;
	}

	/*
		example 1: function call

		cmInit();
		CMScript handle = crim::cmCompileSource( "test.lua" );
		int ret = crim::cmCall< int, const char* >( handle, "foo", "argName" );
		bool ret2 = crim::cmCall< bool >( handle, "foo1" );
		cmUninit();
	*/

	/*
		//example 1: script run
		//init time:
		cmInit();

		cmRunFile( "test.lua" );

		// run frame
		cmRun( elapsedTimeMS );

		// exit
		cmUninit();
	*/

}

#endif
