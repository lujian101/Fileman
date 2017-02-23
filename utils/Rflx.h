//---------------------------------------------------------------------------------------------------------------------
// A Tiny RTTI refection library
//
// Arthor : lujian
// Date of create: 2/22/2012
// email: lujian_niewa@163.com
//---------------------------------------------------------------------------------------------------------------------
#ifndef __RFLX_H__
#define __RFLX_H__

#include <stdarg.h>
#include <assert.h>
#include <string.h>
#include <string>

#define RFLX_STL_CONTAINER_ENABLE

#ifdef RFLX_STL_CONTAINER_ENABLE
	#define RFLX_STL_VECTOR_SUPPORT
	#define RFLX_STL_SET_SUPPORT
	#define RFLX_STL_MAP_SUPPORT
	#ifdef RFLX_STL_VECTOR_SUPPORT
		#include <vector>
	#endif	
	#ifdef RFLX_STL_SET_SUPPORT
		#include <set>
	#endif
	#ifdef RFLX_STL_MAP_SUPPORT
		#include <map>
	#endif
#endif

#ifdef _MSC_VER
	#pragma warning( push )
	#pragma warning( disable : 4996 )
	#pragma warning( disable : 4201 )
	#pragma warning( disable : 4100 ) //unreferenced formal parameter
#endif

#ifdef _MSC_VER
	#ifdef RLFX_DLL
		#if defined BUILD_LIB
			#define RFLX_API __declspec(dllexport)
		#else
			#define RFLX_API __declspec(dllimport)
		#endif
	#else
		#define RFLX_API
	#endif
#else
	#define RFLX_API __attribute__ ((visibility("default")))
#endif

#define RFLX_TASSERT( t )			{ rflx::SAssert< (t) >::dummy_func(); }
#define RFLX_TPRINT_NUM( t, n )		{ rflx::SAssertNumber< (n), (t) >::dummy_func(); }
#define RFLX_OFFSETOF( s, m )		( ( ( ptrdiff_t )&( (s*)0x1 )->m )- 0x1 )
#define RFLX_ARRAY_COUNT( a )		( sizeof( a ) / sizeof( a[0] ) )
#define RFLX_MALLOC					malloc
#define RFLX_FREE					free
#define RFLX_DASSERT				assert
#define RFLX_ERROR					rflx::getCurrentHook()->pfn_error
#define RFLX_WARNING				rflx::getCurrentHook()->pfn_warning

namespace rflx
{
	typedef wchar_t wchar;
	typedef unsigned char uchar;
	typedef unsigned short ushort;
	typedef unsigned int uint;
	typedef unsigned long ulong;
	typedef long long llong;
	typedef unsigned long long ullong;
	typedef char* pstr;
	typedef const char* pcstr;
	typedef wchar* pwstr;
	typedef const wchar* pcwstr;
	typedef void* vptr;
	typedef const void* vcptr;

	class Rflxable;
	class RflxObject;
	class RflxDynamicObject;
	class PropHandle;
	struct EnumInfo;
	struct EnumValue;
	struct ValueData;
	struct Message;
	struct PropDef;
	struct DVStack;
	struct ClassInfo;
	struct CustomDataHandler;
	struct _Class;
	struct _Context;
	typedef _Context* Ktex;
	typedef _Class* Klass;
	typedef void* MessageData;
	template< typename T > struct DataTypeTrait;
	template< typename T > struct PackageObject;
	template< typename T > struct ExtractObject;
	template< typename T > struct IsEnum;
	template< typename T, int N > class FArray;
	template< class TClass, class TBase > class RttiBind;
	
	const unsigned int	RFLX_INVLAID_PROPID = (unsigned int)~0;
	const bool			RFLX_CHAR_IS_SIGNED = ( (char)-1 == (signed char)-1 );
	const bool			RFLX_CHAR_IS_UNSIGNED = ( (char)-1 == (unsigned char)-1 );
}

template< typename T >
const rflx::EnumInfo* Rflx_GetEnumInfo();

namespace rflx
{
	enum ErrorCode
	{
		err_failed = 0x80000000,
		err_array_out_of_range,
		err_out_of_range,
		err_not_implemented,
		err_class_is_being_used,
		err_class_is_already_registered,
		err_class_not_found,
		err_class_is_abstract,
		err_static_class_can_not_be_unregistered,
		err_base_class_not_found,
		err_wrong_inheriting,
		err_invalid_params,
		err_invalid_data,
		err_property_not_found,
		err_file_unknown_type,
		err_file_not_exist,
		err_operation_invalid,
		err_object_not_found,
		err_data_type_mismatch,

		err_ok = 0,

		err_message_consumed,
	};

	enum ValueDataType {
		vdt_nil = 0,
		vdt_reflexable_begin,
			vdt_base_type_begin,
				vdt_bool,
				vdt_number_begin,
					vdt_integer_number_begin,
						vdt_signed_integer_number_begin,
							vdt_schar,
							vdt_schar_,
							vdt_short,
							vdt_int,
							vdt_long,
							vdt_llong,
						vdt_signed_integer_number_end,
						vdt_unsigned_integer_number_begin,
							vdt_wchar,
							vdt_uchar,
							vdt_uchar_,
							vdt_ushort,
							vdt_uint,
							vdt_ulong,
							vdt_ullong,
						vdt_unsigned_integer_number_end,
							vdt_char = RFLX_CHAR_IS_SIGNED ? vdt_schar_ : vdt_uchar_,
						vdt_unsigned_integer_number_end_ = vdt_unsigned_integer_number_end,
					vdt_integer_number_end,
					vdt_float_number_begin,
						vdt_float,
						vdt_double,
					vdt_float_number_end,
				vdt_number_end,
			vdt_base_type_end,
			vdt_string,
			vdt_wstring,
			vdt_enum,
			vdt_custom,
			vdt_pointer,
		vdt_reflexable_end,
		vdt_max_num,
	};	

	enum ValueType
	{
		vt_scalar,
		vt_container_begin,
			vt_c_array,
			vt_array,
			vt_set,
			vt_map,
		vt_container_end,
	};

	enum ClassTypeFlag
	{
		cif_none		= 0,
		cif_abstract	= 1 << 0,
		cif_final		= 1 << 1,
		cif_polymorphic	= 1 << 2,
		cif_default		= cif_none,
	};

	enum ClassInheritType
	{
		cih_default  = 0,
		cih_public	 = cih_default,
		cih_proteced = 1 << 0,
		cih_private  = 1 << 1,
	};
	
	enum PropTraitFlag
	{
		ptf_none			= 0,
		ptf_pointer			= 1 << 0,
		ptf_pointer_const	= 1 << 1,
		ptf_ref				= 1 << 2,
		ptf_ref_const		= 1 << 3,
		ptf_const			= 1 << 4,
		ptf_serializable	= 1 << 5, // not implemented yet
		ptf_enum			= 1 << 6,
		ptf_volatile		= 1 << 7,
		ptf_rflxable		= 1 << 8,  ///< simplest object just has propdefs and static typeinfo
		ptf_rflxobject		= 1 << 9,  ///< object has propdefs and dynmaic typeinfo
		ptf_rflxobject_d	= 1 << 10, ///< polymorphic object has propdefs and dynmaic typeinfo
		ptf_polymorphic		= 1 << 11,
		ptf_Max				= 1 << 16,
	};

	enum PropDefFlag
	{
		pdf_none	  = 0,
		pdf_read_only = 1 << 0,
		pdf_hide	  = 1 << 1,
	};

	enum PropOp
	{
		op_none,
		op_size,
		op_empty,
		op_clear,
		op_resize,
		op_reserve,
		op_pop_front,
		op_pop_back,
		op_push_back,
		op_push_front,
		op_front,
		op_back,
		op_insert,
		op_getref,
		op_at,
		op_index,
		op_erase,
	};

	enum MessageId {
		xm_none = 0,
	};

	struct Message {
		size_t cbSize;
		unsigned int msgId;
		bool handled;
		size_t cbData;
		void* data;
	};

	struct RFLX_API MessageEnableGuard {
		MessageEnableGuard( bool enable = true );
		~MessageEnableGuard( );
	private:
		bool old;
	};

#define RFLX_MESSAGE_ENABLE()	rflx::MessageEnableGuard __MessageEnableGuard__##__LINE__( true )
#define RFLX_MESSAGE_DISABLE()	rflx::MessageEnableGuard __MessageEnableGuard__##__LINE__( false )
#define RFLX_MESSAGE_GUARD( V )	rflx::MessageEnableGuard __MessageEnableGuard__##__LINE__( V )

	inline void createMessage( Message* msg, unsigned int id ) { memset( msg, 0x00, sizeof( *msg ) ); msg->cbSize = sizeof( *msg ); msg->msgId = id; }
	inline void messageExtra( Message* msg, MessageData data, size_t cbData ) { msg->data = data;  memset( data, 0x00, cbData ); msg->cbData = cbData; }

	// property operation functions
	typedef ErrorCode		( *pfn_get )( const PropDef* def, unsigned int index, void* userObject, ValueData* val );
	typedef void*			( *pfn_get_ref )( const PropDef* def, unsigned int index, void* userObject );
	typedef ErrorCode		( *pfn_set )( const PropDef* def, unsigned int index, void* userObject, const ValueData* val );
	typedef const PropDef*	( *pfn_get_propdef )( Klass* klass, unsigned int* count, RflxObject* object );
	typedef ErrorCode		( *pfn_operator )( const PropDef* def, void* userObject, PropOp op, va_list va );
	
	// object method functions
	typedef ErrorCode		( *pfn_create_instance )( RflxObject** object );
	typedef ErrorCode		( *pfn_destroy_instance )( RflxObject* object );
	typedef ErrorCode		( *pfn_message_filter )( Message* msg, void* _this );
	typedef ErrorCode		( *pfn_message_proc )( Message* msg, void* _this );
	typedef ErrorCode		( *pfn_object_proc )( Message* msg, void* _this );
	typedef void			( *pfn_initialize_hook )();
	typedef void			( *pfn_uninitialize_hook )();

	// struct to hold a member or global property information
	struct PropDef
	{
		unsigned int flags; ///< PropDefFlag
		unsigned int id; ///< index in the original array	
		uintptr_t offset; ///< memory offset in a class object
		const char* name;
		const char* description; ///< user comments
		const char* groupName;
		const char* userType; ///< user name
		ValueType kind; ///< scalar or array..
		ValueDataType type;
		ValueDataType type2; ///< compound type such as: map with key type 
		unsigned short traits_bits; ///< PropTraitFlag
		unsigned short traits_bits2; ///< PropTraitFlag2
		const CustomDataHandler* customHandler; ///< for user define data type
		const CustomDataHandler* customHandler2; ///< for user define data type
		const EnumInfo* enumInfo;
		const EnumInfo* enumInfo2;
		const ValueData* defaultVal;
		pfn_get get;
		pfn_get_ref get_ref;
		pfn_set set;
		pfn_get_propdef getDefs; ///< get perperty defs for embedded RflxObject
		pfn_operator op; ///< vary operation hanlde with command type and variables
		const char* editorData; ///< to store data for editor using, for example: you can put all combo-box items here "ComboBox = Option1 | Option2 | ..."
	};

	namespace _internal {
		struct DataTypeSizeName { size_t size; ValueDataType value; const char* name; const char* name2; };
		RFLX_API const DataTypeSizeName* _getValueDataTypeSizeNameTable();
		RFLX_API const EnumInfo* _findEnumInfo( const char* name );
		RFLX_API const EnumInfo* _addEnumInfo( const char* name, const EnumValue* values, unsigned int count );
		RFLX_API void _initObjectRtti( RflxObject* object, Klass klass );
		RFLX_API unsigned int _addClassInstanceCount( Klass klass );
		RFLX_API unsigned int _releaseClassInstanceCount( Klass klass );
		inline unsigned int _getClassFlags( Klass klass ) { return *( unsigned int* )klass; }
		// get the memory offset from outer object to RflxObject / Rflxable that is a non-polymorphic object
		inline size_t _getOuter2BaseOffset( Klass klass ) {
			// do not modify this!
			struct _raw{ unsigned int flags; size_t offset; };
			return ( ( _raw* )klass )->offset;
		}
		template< class TClass, class TBase > void _checkClassDeclaration() {
			RFLX_DASSERT( TClass::_name == TBase::_name );
		}
		int _strcmp( const char* src, const char* dst );
	}

	struct EnumValue {
		const char* name;
		long value;
		const char* desc;
	};

	struct RFLX_API EnumInfo {
	private:
		const char* name;
		EnumValue* data;
		unsigned int count;
		Ktex context;
		EnumInfo( const EnumInfo& );
		EnumInfo& operator = ( const EnumInfo& );
		EnumInfo( const char* _name, const EnumValue* _data, unsigned int _count );
		friend const EnumInfo* _internal::_addEnumInfo( const char*, const EnumValue*, unsigned int );
	public:
		~EnumInfo();
		Ktex getContext() const { return context; }
		// find a enum by a enum name
		bool getValue( const char* name, long* value = NULL, unsigned int* index = NULL ) const;
		// get index of enum declaration order by enum value.
		// return -1 if not found.
		unsigned int getValueIndex( long value ) const;
		const EnumValue* findItem( const char* name ) const;
		const EnumValue* findItemByValue( long value ) const;
		const EnumValue* getItemByIndex( unsigned int index ) const;
		const char* getName() const { return name; }
		unsigned int getCount() const { return count; }
		const EnumValue* getData() const { return data; }
	};

	template< typename T, bool IsEnumType = IsEnum< T >::value >
	struct GetEnumInfoHelper { static inline const EnumInfo* invoke() { return ::Rflx_GetEnumInfo< T >(); } };
	template< typename T >
	struct GetEnumInfoHelper< T, false > { static inline const EnumInfo* invoke() { return NULL; } };
	template< typename T >
	inline const EnumInfo* getEnumInfo() { return GetEnumInfoHelper< T >::invoke(); }
	
	struct ObjectMethods {
		pfn_object_proc	objectProc;
		pfn_create_instance	createInstance;
		pfn_destroy_instance destroyInstance;
		pfn_initialize_hook initializeHook;
		pfn_uninitialize_hook unInitializeHook; 
	};

	struct MsgFunc {
		MessageId id;
		struct MFP {
			// HACK the member function
			enum { MAX_MFP_SIZE = 16 };
			unsigned char raw[ MAX_MFP_SIZE ];
		};
		MFP func;
		unsigned int propFlags;
		size_t payloadSize;
	};	

	struct MsgFuncChain {
		const MsgFunc*	mf;
		MsgFuncChain*	next;
		Klass			klass;
		MsgFuncChain( const MsgFunc* _mf, Klass _k ) : mf( _mf ), next( NULL ), klass( _k ) {}
	};

	struct ClassInfo {
		const char* className;
		const char* baseClassName;
		unsigned int flags; // ClassTypeFlag
		size_t baseOffset; // no need init by user
		ObjectMethods methods;
		const PropDef* propDefs;
		unsigned int propCount;
		const CustomDataHandler* customDataHandler;
		const MsgFunc* msgMap;
	};

	struct PropPos {
		Klass owner;
		Klass outer;
		unsigned int id; // property index in owner class
		size_t offset; // offset to owner from outer class
		unsigned int index; // index array type object
		PropPos() : owner( NULL ), outer( NULL ), id( 0 ), offset( 0 ), index( 0 ) { }
		void clear() { memset( this, 0, sizeof( *this ) ); }
		operator bool() const { return owner != NULL; }
	};

	struct PropPosEx : PropPos {
		char* mbase;
		PropPosEx( const PropPos& o ) : mbase( NULL ) {
			owner = o.owner;
			id = o.id;
			offset = o.offset;
			index = o.index;
		}
		PropPosEx() : mbase( NULL ) {}
		void clear() { memset( this, 0, sizeof( *this ) ); }
		PropPosEx& operator = ( const PropPos& o ) {
			owner = o.owner;
			id = o.id;
			offset = o.offset;
			index = o.index;
			mbase = NULL;
			return *this;
		}
		operator bool() const { return owner != NULL && mbase; }
	};

	class RFLX_API PropHandle {
	public:
		PropHandle() : pos( NULL ) {}
		PropHandle& operator = ( const PropHandle& o );
		PropHandle( const PropHandle& o );
		PropHandle( const PropPosEx* p );
		PropHandle( const PropPos* p );
		~PropHandle();
		void clear();
		operator const PropPosEx* () const { if ( pos && pos->mbase ) return pos; else return NULL; }
		operator const PropPos* () const { return pos; }
		bool empty() const { return !pos || !pos->owner; }
		operator bool() const { return !empty(); }
	private:
		PropPosEx* pos;
	};

	inline unsigned int	makeClassFlag( ClassInheritType inheritType, ClassTypeFlag classType );
	inline void	setClassFlag( unsigned int& flag, ClassTypeFlag classType );
	inline void	setInheritType( unsigned int& flag, ClassInheritType inheritType );
	inline ClassInheritType getInheritType( unsigned int flag );
	inline bool	checkClassFlag( unsigned int flag, ClassTypeFlag classType );
	RFLX_API bool isUniqueName( const char* name );

	template< bool > struct SAssert;
	template<> struct SAssert< true > { static inline void dummy_func(){} };
	
	template< int __PrintNumber__, bool E > struct SAssertNumber;
	template< int __PrintNumber__ > struct SAssertNumber< __PrintNumber__, true > { static inline void dummy_func(){} };

	template< typename T1, typename T2 >
	struct TypeEqual { static const bool value = false; };
	template< typename T >
	struct TypeEqual< T, T > { static const bool value = true; };

	template< typename TClass >
	struct IsPtr { static const bool value = false; };
	template< typename TClass >
	struct IsPtr< TClass* > { static const bool value = true; };
	template< typename TClass >
	struct IsPtr< TClass* const > : IsPtr< TClass* >{};

	template< typename TClass >
	struct IsPtrConst { static const bool value = false; };
	template< typename TClass >
	struct IsPtrConst< TClass* const > { static const bool value = true; };

	template< typename TClass >
	struct IsConst { static const bool value = false; };
	template< typename TClass >
	struct IsConst< const TClass > { static const bool value = true; };

	template< typename TClass >
	struct IsRef { static const bool value = false; };
	template< typename TClass >
	struct IsRef< TClass& > { static const bool value = true; };

	template< typename TClass >
	struct IsVolatile { static const bool value = false; };
	template< typename TClass >
	struct IsVolatile< volatile TClass > { static const bool value = true; };

	template< typename TClass >
	struct IsCArray { static const bool value = false; typedef TClass type; };
	template< typename TClass >
	struct IsCArray< TClass[] > { static const bool value = true; typedef TClass type; };
	template< typename TClass, int N >
	struct IsCArray< TClass[N] > { static const bool value = true; typedef TClass type; };

	template< typename TClass >
	struct RemoveConst { typedef TClass type; };
	template< typename TClass >
	struct RemoveConst< const TClass > { typedef TClass type; };

	template< typename TClass >
	struct RemovePtr { typedef TClass type; };
	template< typename TClass >
	struct RemovePtr< TClass* > { typedef TClass type; };

	template< typename TClass >
	struct RemoveRef { typedef TClass type; };
	template< typename TClass >
	struct RemoveRef< TClass& > { typedef TClass type; };

	template< typename TClass >
	struct RemoveVolatile { typedef TClass type; };
	template< typename TClass >
	struct RemoveVolatile< volatile TClass > { typedef TClass type; };

	template< typename TClass >
	struct RemoveCV { typedef typename RemoveVolatile< typename RemoveConst< TClass >::type >::type type; };
	template< typename TClass >
	struct RemovePR { typedef typename RemovePtr< typename RemoveRef< TClass >::type >::type type; };

	template< typename TClass >
	struct RemoveQualifies { typedef typename RemoveCV< typename RemovePR< TClass >::type >::type type; };

	template< typename TClass >
	struct RemoveOwner { typedef TClass type; };
	template< typename TOwner, typename TClass >
	struct RemoveOwner< TClass TOwner::* > { typedef TClass type; };

	template< typename TClass >
	struct IsRefConst { static const bool value = IsRef< TClass >::value && IsConst< typename RemoveConst< TClass >::type >::value; };

	template< bool V, typename A, typename B >
	struct If { typedef A type; };
	template< typename A, typename B >
	struct If< false, A, B > { typedef B type; };

	template< typename TClass >
	struct IsBuildInType {
		typedef typename RemoveCV< TClass >::type type;
		static const bool value =
			TypeEqual< type, char >::value ||
			TypeEqual< type, signed char >::value ||
			TypeEqual< type, unsigned char >::value ||
			TypeEqual< type, short >::value ||
			TypeEqual< type, unsigned short >::value ||
			TypeEqual< type, int >::value ||
			TypeEqual< type, unsigned int >::value ||
			TypeEqual< type, long >::value ||
			TypeEqual< type, unsigned long >::value ||
			TypeEqual< type, long long >::value ||
			TypeEqual< type, unsigned long long >::value ||
			TypeEqual< type, float >::value ||
			TypeEqual< type, double >::value ||
			TypeEqual< type, wchar_t >::value ||
			TypeEqual< type, void >::value ||
			TypeEqual< type, bool >::value;
	};

	struct NullType{};
	struct Bool_True{ char dummy[1]; };
	struct Bool_False{ char dummy[2]; };
	template< bool B > struct BoolType { typedef Bool_False type; };
	template<> struct BoolType< true > { typedef Bool_True type; };

	template< typename THead, typename TTail = NullType >
	struct TypeList { typedef THead head; typedef TTail tail; };

	template< typename TList >
	struct TypeList_Length;

	template< typename TList, unsigned int index >
	struct TypeList_GetAt;

	template< typename TList, typename T >
	struct TypeList_Append;

	template<>
	struct TypeList_Length< NullType > { static const unsigned int value = 0; };

	template< typename THead, typename TTail >
	struct TypeList_Length< TypeList< THead, TTail > > { static const unsigned int value = 1 + TypeList_Length< TTail >::value; };
	
	template< typename THead, typename TTail >
	struct TypeList_GetAt< TypeList< THead, TTail >, 0 > { typedef THead type; };
	
	template< typename THead, typename TTail, unsigned int index >
	struct TypeList_GetAt< TypeList< THead, TTail >, index > { typedef typename TypeList_GetAt< TTail, index - 1 >::type type; };
	
	template<>
	struct TypeList_Append< NullType, NullType > { typedef NullType type; };
	
	template< typename T >
	struct TypeList_Append< NullType, T > { typedef TypeList< T, NullType > type; };

	template< typename THead, typename TTail, typename T >
	struct TypeList_Append< TypeList< THead, TTail >, T > { typedef TypeList< THead, typename TypeList_Append< TTail, T >::type > type; };

	#define RFLX_TYPE_LIST_1( T1 )											rflx::TypeList< T1, rflx::NullType >
	#define RFLX_TYPE_LIST_2( T1, T2 )										rflx::TypeList< T1, RFLX_TYPE_LIST_1( T2 ) >
	#define RFLX_TYPE_LIST_3( T1, T2, T3 )									rflx::TypeList< T1, RFLX_TYPE_LIST_2( T2, T3 ) >
	#define RFLX_TYPE_LIST_4( T1, T2, T3, T4 )								rflx::TypeList< T1, RFLX_TYPE_LIST_3( T2, T3, T4 ) >
	#define RFLX_TYPE_LIST_5( T1, T2, T3, T4, T5 )							rflx::TypeList< T1, RFLX_TYPE_LIST_4( T2, T3, T4, T5 ) >
	#define RFLX_TYPE_LIST_6( T1, T2, T3, T4, T5, T6 )						rflx::TypeList< T1, RFLX_TYPE_LIST_5( T2, T3, T4, T5, T6 ) >
	#define RFLX_TYPE_LIST_7( T1, T2, T3, T4, T5, T6, T7 )					rflx::TypeList< T1, RFLX_TYPE_LIST_6( T2, T3, T4, T5, T6, T7 ) >
	#define RFLX_TYPE_LIST_8( T1, T2, T3, T4, T5, T6, T7, T8 )				rflx::TypeList< T1, RFLX_TYPE_LIST_7( T2, T3, T4, T5, T6, T7, T8 ) >
	#define RFLX_TYPE_LIST_9( T1, T2, T3, T4, T5, T6, T7, T8, T9 )			rflx::TypeList< T1, RFLX_TYPE_LIST_8( T2, T3, T4, T5, T6, T7, T8, T9 ) >
	#define RFLX_TYPE_LIST_10( T1, T2, T3, T4, T5, T6, T7, T8, T9, T10 )	rflx::TypeList< T1, RFLX_TYPE_LIST_9( T2, T3, T4, T5, T6, T7, T8, T9, T10 ) >

	template< typename T, bool TValidTest = IsBuildInType< T >::value == false >
	class IsEnumHelper {
		struct _IntConvert { _IntConvert( int ); };
		static Bool_False _IntConvertTestFunc( ... );
		static Bool_True  _IntConvertTestFunc( _IntConvert );
		static Bool_False _PtrTestFunc( const volatile char* );
		static Bool_False _PtrTestFunc( const volatile signed char* );
		static Bool_False _PtrTestFunc( const volatile unsigned char* );
		static Bool_False _PtrTestFunc( const volatile short* );
		static Bool_False _PtrTestFunc( const volatile unsigned short* );
		static Bool_False _PtrTestFunc( const volatile int* );
		static Bool_False _PtrTestFunc( const volatile unsigned int* );
		static Bool_False _PtrTestFunc( const volatile long* );
		static Bool_False _PtrTestFunc( const volatile unsigned long* );
		static Bool_False _PtrTestFunc( const volatile long long* );
		static Bool_False _PtrTestFunc( const volatile unsigned long long* );
		static Bool_False _PtrTestFunc( const volatile double* );
		static Bool_False _PtrTestFunc( const volatile long double* );
		static Bool_False _PtrTestFunc( const volatile float* );
		static Bool_False _PtrTestFunc( const volatile bool* );
		static Bool_True  _PtrTestFunc( const volatile void* );
		static T arg;
	public:
		static const bool value = ( ( sizeof( _IntConvertTestFunc( arg ) ) == sizeof( Bool_True ) ) &&
			( sizeof( _PtrTestFunc( &arg ) ) == sizeof( Bool_True ) ) );
	};

	template< typename T >
	class IsEnumHelper< T, false > { public : static const bool value = false; };

	template< typename T >
	struct IsEnum { static const bool value = IsEnumHelper< T >::value; };

	template< typename T >
	class IsClass {
		template< typename T1 >
		static char _Tester( int T1::* );
		template< typename T1 >
		static int _Tester( ... );
	public:
		static const bool value = sizeof( _Tester< T >( 0 ) ) == sizeof( char ) ? true : false;
	};

	template< class TDerived, class TBase >
	class IsKindOf {
	private:
		typedef typename RemoveCV< TBase >::type _TBase;
		typedef typename RemoveCV< TDerived >::type _TDerived;
		static int  _TestFunc( ... );
		static char _TestFunc( _TBase* );
	public:
		static const bool value = IsClass< _TDerived >::value &&
			IsClass< _TBase >::value && sizeof( _TestFunc( reinterpret_cast< _TDerived* >( 0 ) ) ) == sizeof( char );
	};

	template< class TDerived, class TBase >
	class IsChildOf {
	private:
		typedef typename RemoveCV< TBase >::type _TBase;
		typedef typename RemoveCV< TDerived >::type _TDerived;
		static int  _TestFunc( ... );
		static char _TestFunc( _TBase* );
	public:
		static const bool value = IsClass< _TDerived >::value &&
			IsClass< _TBase >::value && TypeEqual< _TDerived, _TBase >::value == false && sizeof( _TestFunc( reinterpret_cast< _TDerived* >( 0 ) ) ) == sizeof( char );
	};

	template< typename T, bool isClass = false >
	struct IsPolymorphicHelper { static const bool value = false; };

	template< typename T >
	struct IsPolymorphicHelper< T, true > {
		typedef typename RemoveQualifies< T >::type base_type;
		struct class1 : base_type{ ~class1() {} class1& operator = ( const class1& ); };
		struct class2 : base_type{ virtual ~class2() {} class2& operator = ( const class2& ); };
		static const bool value = ( sizeof( class1 ) == sizeof( class2 ) );
	};

	template< typename T >
	struct IsPolymorphic { static const bool value = IsPolymorphicHelper< T, IsClass< T >::value >::value; };

	template< typename TDerived, typename TBase >
	struct BaseOffsetGetter {
		static inline size_t invoke() {
			RFLX_TASSERT( ( IsKindOf< TDerived, TBase >::value ) );
			return (size_t)(TBase*)(TDerived*)0x01 - 0x01;
		}
	};

	template< class T >
	struct RflxObjectName {
		static const char* name( const char* _name = NULL ) {
			static char _n[64] = {0};
			if ( _n[0] == 0 && _name ) {
				const int count = sizeof(_n) / sizeof(_n[0]);
				std::char_traits< char >::copy( _n, _name, count );
				RFLX_DASSERT( isUniqueName( _n ) && "It's not a unique string!" );
			}
			return _n;
		}
	};

	// nopolymorphic and no rtti basic object
	class RFLX_API Rflxable {
	public:
		static const char* _name() { return RflxObjectName< Rflxable >::name( "Rflxable" ); }
		static const PropDef* _getPropDefs( unsigned int* count ){ if ( count ) *count = 0; return NULL; }
		static const Klass& _class() { static Klass c = NULL; return c; }
		static void _initializeHook() {}
		static void _unInitializeHook() {}
		static ErrorCode _createInstance( RflxObject** object ) { if ( object ) *object = NULL; return err_not_implemented; }
		static ErrorCode _destroyInstance( RflxObject* object ) { return err_not_implemented; }
		static ErrorCode _objectProc( Message* msg, void* thisObject ) { return err_ok; }
		static const MsgFunc* _getMsgMap() { return NULL; }
	};

	// nopolymorphic rtti basic object
	class RFLX_API RflxObject : public Rflxable {
		friend RFLX_API void _internal::_initObjectRtti( RflxObject* object, Klass klass );
	public:
		RflxObject();
		RflxObject( const RflxObject& );
		~RflxObject();
		static const char* _name() { return RflxObjectName< RflxObject >::name( "RflxObject" ); }
		static const PropDef* _getPropDefs( unsigned int* count ) { if ( count ) { *count = 0; } return NULL; }
		static const Klass& _class() { static Klass c = NULL; return c; }
		static ErrorCode _createInstance( RflxObject** object ) { if ( object ) { *object = NULL; } return err_not_implemented; }
		static ErrorCode _destroyInstance( RflxObject* object ) { return err_not_implemented; }
		Klass _dynamicClass() const { return _outer; }
		const void* _mbase() const { return const_cast< RflxObject* >( this )->_mbase(); }
		void* _mbase() { return (char*)this - _internal::_getOuter2BaseOffset( _outer ); }
	private:
		Klass	_outer;
	};

	// this is a initializer which will not insert dummy object "__RttiInitializer__" into target
	// __RttiInitializer__ is empty object we just use it as RAII but it will enlarge size of target object.
	// you should use this instead of Normal inherit:
	//		class Derived : public Base {};
	// to
	//		class Derived : public RttiBind< Derived, Base > {};
	template< class TClass, class TBase >
	class RttiBind : public TBase {
		void _bind() {
			TClass* object = ( TClass* )this;
			RFLX_DASSERT( TClass::_class() );
			RFLX_TASSERT( ( IsKindOf< TBase, RflxObject >::value ) );
			rflx::_internal::_initObjectRtti( object, TClass::_class() );
			rflx::_internal::_addClassInstanceCount( TClass::_class() );
		}
	public:
		RttiBind() { _bind(); }
		RttiBind( const RttiBind& ) { _bind(); }
		~RttiBind() { rflx::_internal::_releaseClassInstanceCount( TClass::_class() ); }
	};


	template< typename T, int N >
	class FArray {
	public:
		inline const T& operator[]( size_t i ) const { RFLX_DASSERT( i >= 0 && i < N && "FArray: out of range!" ); return array[i]; }
		inline T& operator[]( size_t i ) { RFLX_DASSERT( i >= 0 && i < N && "FArray: out of range!" ); return array[i]; }
	private:
		T array[N];
	};

	template< typename T >
	struct DataTrait {	
		typedef RFLX_TYPE_LIST_1( T ) internal_types;
		static const ValueDataType type_value = IsEnum< T >::value ? vdt_enum :
			( ( IsRef< T >::value || IsPtr< T >::value ) ? vdt_pointer : vdt_custom );
		static const ValueType value_class = vt_scalar;
	};
	
	template< typename T >
	struct DataTypeTrait : public DataTrait< typename RemoveCV< T >::type > {};

	template< typename T >
	struct IsBaseType {
		typedef typename RemoveCV< T >::type type;
		static const bool value = DataTypeTrait< type >::value_class == vt_scalar && 
			( IsEnum< type >::value || IsPtr< type >::value || IsRef< type >::value || 
			( DataTypeTrait< type >::type_value > vdt_base_type_begin && DataTypeTrait< type >::type_value < vdt_base_type_end ) ||
			DataTypeTrait< type >::type_value == vdt_string ||
			DataTypeTrait< type >::type_value == vdt_wstring );
	};

	template< ValueDataType VType > struct ValueTypeConv;
	template<> struct ValueTypeConv< vdt_bool > { typedef bool type; };
	template<> struct ValueTypeConv< vdt_schar > { typedef signed char type; };
	template<> struct ValueTypeConv< vdt_uchar > { typedef unsigned char type; };
	template<> struct ValueTypeConv< vdt_wchar > { typedef wchar_t type; };
	template<> struct ValueTypeConv< vdt_short > { typedef short type; };
	template<> struct ValueTypeConv< vdt_ushort > { typedef unsigned short type; };
	template<> struct ValueTypeConv< vdt_int > { typedef int type; };
	template<> struct ValueTypeConv< vdt_uint > { typedef unsigned int type; };
	template<> struct ValueTypeConv< vdt_long > { typedef long type; };
	template<> struct ValueTypeConv< vdt_ulong > { typedef unsigned long type; };	
	template<> struct ValueTypeConv< vdt_llong > { typedef long long type; };
	template<> struct ValueTypeConv< vdt_ullong > { typedef unsigned long long type; };
	template<> struct ValueTypeConv< vdt_float > { typedef float type; };
	template<> struct ValueTypeConv< vdt_double > { typedef double type; };
	template<> struct ValueTypeConv< vdt_pointer > { typedef void* type; };

#define RFLX_IMP_DATATYPE_TRAIT( TYPE, NAME ) \
	template<> struct DataTrait< TYPE > {\
		typedef RFLX_TYPE_LIST_1( TYPE ) internal_types;\
		static const ValueDataType type_value = vdt_##NAME;\
		static const ValueType value_class = vt_scalar;\
	};
	RFLX_IMP_DATATYPE_TRAIT( void, nil )
	RFLX_IMP_DATATYPE_TRAIT( bool, bool )
	RFLX_IMP_DATATYPE_TRAIT( signed char, schar )
	RFLX_IMP_DATATYPE_TRAIT( char, char )
	RFLX_IMP_DATATYPE_TRAIT( short, short )
	RFLX_IMP_DATATYPE_TRAIT( int, int )
	RFLX_IMP_DATATYPE_TRAIT( long, long )
	RFLX_IMP_DATATYPE_TRAIT( long long, llong )
	RFLX_IMP_DATATYPE_TRAIT( wchar_t, wchar )
	RFLX_IMP_DATATYPE_TRAIT( unsigned char, uchar )
	RFLX_IMP_DATATYPE_TRAIT( unsigned short, ushort )
	RFLX_IMP_DATATYPE_TRAIT( unsigned int, uint )
	RFLX_IMP_DATATYPE_TRAIT( unsigned long, ulong )
	RFLX_IMP_DATATYPE_TRAIT( unsigned long long, ullong )
	RFLX_IMP_DATATYPE_TRAIT( float, float )
	RFLX_IMP_DATATYPE_TRAIT( double, double )
#undef RFLX_IMP_DATATYPE_TRAIT

#define RFLX_IMP_STRING_TRAIT( TYPE, NAME ) \
	template<> struct DataTrait< TYPE > {\
		typedef RFLX_TYPE_LIST_1( TYPE ) internal_types;\
		static const ValueDataType type_value = vdt_##NAME;\
		static const ValueType value_class = vt_scalar;\
	};

#define RFLX_IMP_STD_STRING_TRAIT( ELEM, NAME ) \
	template< typename Traits, typename Allocator >\
	struct DataTrait< std::basic_string< ELEM, Traits, Allocator > > {\
		typedef RFLX_TYPE_LIST_3( ELEM, Traits, Allocator ) internal_types;\
		static const ValueDataType type_value = vdt_##NAME;\
		static const ValueType value_class = vt_scalar;\
	};

	RFLX_IMP_STD_STRING_TRAIT( char, string )
	RFLX_IMP_STD_STRING_TRAIT( wchar_t, wstring )

	template< typename T, int N >
	struct DataTrait< T[ N ] > {
		typedef RFLX_TYPE_LIST_1( T ) internal_types;
		static const ValueDataType type_value = IsBaseType< T >::value ? DataTypeTrait< T >::type_value : vdt_custom;
		static const ValueType value_class = vt_c_array;
	};

#define	RFLX_IMP_C_ARRAY_TRAIT( TYPE ) \
	template< typename T, int N >\
	struct DataTrait< TYPE< T, N > > {\
		typedef RFLX_TYPE_LIST_1( T ) internal_types;\
		static const ValueDataType type_value = IsBaseType< T >::value ? DataTypeTrait< T >::type_value : vdt_custom;\
		static const ValueType value_class = vt_c_array;\
	};

	RFLX_IMP_C_ARRAY_TRAIT( FArray )

#ifdef RFLX_STL_VECTOR_SUPPORT
	template< typename T, typename Allocator >
	struct DataTrait< std::vector< T, Allocator > > {
		typedef RFLX_TYPE_LIST_2( T, Allocator ) internal_types;
		static const ValueDataType type_value = IsBaseType< T >::value ? DataTypeTrait< T >::type_value : vdt_custom;
		static const ValueType value_class = vt_array;
	};
#endif // RFLX_STL_VECTOR_SUPPORT

#ifdef RFLX_STL_SET_SUPPORT
	template< typename T, typename Pred, typename Allocator >
	struct DataTrait< std::set< T, Pred, Allocator > > {
		typedef RFLX_TYPE_LIST_3( T, Pred, Allocator ) internal_types;
		static const ValueDataType type_value = IsBaseType< T >::value ? DataTypeTrait< T >::type_value : vdt_custom;
		static const ValueType value_class = vt_set;
	};
#endif // RFLX_STL_SET_SUPPORT

#ifdef RFLX_STL_MAP_SUPPORT
	template< typename KEY, typename T, typename Pred, typename Allocator >
	struct DataTrait< std::map< KEY, T, Pred, Allocator > > {
		typedef RFLX_TYPE_LIST_4( KEY, T, Pred, Allocator ) internal_types;
		const static ValueDataType type_value = IsBaseType< T >::value ? DataTypeTrait< T >::type_value : vdt_custom;
		static const ValueType value_class = vt_map;
	};
#endif // RFLX_STL_MAP_SUPPORT

#define RFLX_IMP_ENUM_BEGIN( NAME ) \
	namespace ENUM_##NAME {\
		static const rflx::EnumInfo* _InitEnumInfo();\
	}\
	template<>\
	inline const rflx::EnumInfo* Rflx_GetEnumInfo< NAME >() {\
		RFLX_TASSERT( ( rflx::IsEnum< NAME >::value ) );\
		return ENUM_##NAME::_InitEnumInfo();\
	}\
	namespace ENUM_##NAME {\
	static const rflx::EnumInfo* _InitEnumInfo() { \
			const char* name = #NAME;\
			const rflx::EnumInfo* info = rflx::_internal::_findEnumInfo( name );\
			if ( info ) {\
				return info;\
			}\
			rflx::EnumValue items[] = {\

#define RFLX_IMP_ENUM_ITEM( ITEM )								{ #ITEM, ITEM, "" },
#define RFLX_IMP_ENUM_ITEM_WITH_NAME( ITEM, NAME )				{ NAME, ITEM, "" },
#define RFLX_IMP_ENUM_ITEM_WITH_NAME_DESC( ITEM, NAME, DESC )	{ NAME, ITEM, DESC },
#define RFLX_IMP_ENUM_ITEM_DESC( ITEM, DESC )					{ #ITEM, ITEM, DESC },

#define RFLX_IMP_ENUM_END \
			};\
			info = rflx::_internal::_addEnumInfo( name, items, sizeof( items ) / sizeof( items[0] ) );\
			return info;\
		}\
		static const rflx::EnumInfo* _EnumInfo = _InitEnumInfo();\
	}

	// these flags are used to indicate which function has been implemented by user
	// for example, users implement CustomDataHanders by theirselves, they should overwrite
	// serializerFlag that return flags that was composed of these SerializerImplFlag
	enum SerializerImplFlag {
		sef_none		= 0,
		sef_from_string = 1 << 0,
		sef_to_string   = 1 << 1,
		sef_from_binary = 1 << 2,
		sef_to_binary   = 1 << 3,
	};

	struct CustomDataHandler {
		const char*		( *name )( );
		Klass			( *klass )( );
		void*			( *create )( );
		void*			( *clone )( const void* obj );
		void			( *destroy )( void* obj );
		void			( *copy )( void* obj1, const void* obj2 );
		bool			( *equal )( const void* left, const void* right );
		bool			( *less )( const void* left, const void* right );
		bool			( *great )( const void* left, const void* right );
		bool			( *noteq )( const void* left, const void* right );
		bool			( *lesseq )( const void* left, const void* right );
		bool			( *greateq )( const void* left, const void* right );
		ErrorCode		( *fromString )( void* obj, const char* buf, size_t* count );
		ErrorCode		( *toString )( const void* obj, char* buf, size_t* count );
		ErrorCode		( *fromBinary )( void* obj, const char* buf, size_t* count );
		ErrorCode		( *toBinary )( const void* obj, char* buf, size_t* count );
		unsigned int	( *serializerFlag )(); // SerializerImplFlag
	};

	template< typename T >
	struct IsRflxable {
		typedef typename RemoveQualifies< T >::type base_type;
		const static bool value = IsKindOf< base_type, Rflxable >::value;
	};

	template< typename T, bool _IsRflxObject = IsRflxable< T >::value >
	struct GetClassHelper { static inline Klass invoke() { return NULL; } };
	template< typename T >
	struct GetClassHelper< T, true > { static inline Klass invoke() { return T::_class(); } };

	template< typename T >
	inline Klass GetClass() {
		typedef typename RemovePR< typename RemoveCV< T >::type >::type type;
		return GetClassHelper< type >::invoke();
	};

	template< typename T, int CompareType = IsBaseType< T >::value ? 1 :
		( DataTypeTrait< T >::type_value == vdt_custom || DataTypeTrait< T >::value_class != vt_scalar ? 2 : 0 ) >
	struct ValueBaseCmp{ typedef typename RemoveCV< T >::type value_type; };

	template< typename T >
	struct ValueBaseCmp< T, 1 > {
		typedef T value_type;
		static inline bool pred( const T& l, const T& r, ErrorCode* err ) { if ( err ) *err = err_ok; return l < r; }
		static inline bool equal( const T& l, const T& r ) { return l == r; }
		static inline bool less( const T& l, const T& r ) { return l < r; }
		static inline bool great( const T& l, const T& r ) { return l > r; }
		static inline bool noteq( const T& l, const T& r ) { return l != r; }
		static inline bool lesseq( const T& l, const T& r ) { return l <= r; }
		static inline bool greateq( const T& l, const T& r ) { return l >= r; }
	};

	template< typename T >
	struct ValueBaseCmp< T, 2 > {
		typedef T value_type;
		static inline bool pred( const T& l, const T& r, ErrorCode* err ) { RFLX_DASSERT( err ); *err = err_not_implemented; return false; }
		static inline bool equal( const T& l, const T& r ) { RFLX_DASSERT( 0 && "equal operator is not implemented!" ); return false; }
		static inline bool less( const T& l, const T& r ) { RFLX_DASSERT( 0 && "less operator is not implemented!" ); return false; }
		static inline bool great( const T& l, const T& r ) { RFLX_DASSERT( 0 && "great operator is not implemented!" ); return false; }
		static inline bool noteq( const T& l, const T& r ) { RFLX_DASSERT( 0 && "not equal operator is not implemented!" ); return false; }
		static inline bool lesseq( const T& l, const T& r ) { RFLX_DASSERT( 0 && "less equal operator is not implemented!" ); return false; }
		static inline bool greateq( const T& l, const T& r ) { RFLX_DASSERT( 0 && "great equal operator is not implemented!" ); return false; }
	};

	template< typename T >
	struct IValueCmp : ValueBaseCmp< typename RemoveCV< T >::type > { typedef typename RemoveCV< T >::type value_type; };
		
	template< typename T, bool IsCustomDataType = DataTypeTrait< T >::type_value == vdt_custom >
	struct BaseCustomDataHandler;

	template< typename T >
	struct BaseCustomDataHandler< T, true > {
		static const char*	name() { RFLX_DASSERT( RflxObjectName< T >::name()[0] && "Invalid object name, did you forgot to assign a name to a object?" );return RflxObjectName< T >::name(); }
		static Klass		klass() { return GetClass< T >(); }
		static void*		create() { void* p = NULL; try { p = new T; } catch( ... ) { RFLX_WARNING( "create custom data failed!\n" ); p = NULL; } return p; }
		static void*		clone( const void* obj ) { void* p = NULL; try { p = new T( *(T*)obj ); } catch( ... ) { RFLX_WARNING( "create custom data failed!\n" ); p = NULL; } return p; }
		static void			destroy( void* obj ) { delete (T*)obj; }
		static void			copy( void* dst, const void* src ) { *(T*)dst = *(const T*)src; }
		static bool			equal( const void* left, const void* right ) { return IValueCmp< T >::equal( *(const T*)left, *(const T*)right ); }
		static bool			less( const void* left, const void* right ) { return IValueCmp< T >::less( *(const T*)left, *(const T*)right ); }
		static bool			great( const void* left, const void* right ) { return IValueCmp< T >::great( *(const T*)left, *(const T*)right ); }
		static bool			noteq( const void* left, const void* right ) { return IValueCmp< T >::noteq( *(const T*)left, *(const T*)right ); }
		static bool			lesseq( const void* left, const void* right ) { return IValueCmp< T >::lesseq( *(const T*)left, *(const T*)right ); }
		static bool			greateq( const void* left, const void* right ) { return IValueCmp< T >::greateq( *(const T*)left, *(const T*)right ); }
		static ErrorCode	fromString( void* obj, const char* buf, size_t* count ) { return err_not_implemented; }
		static ErrorCode	toString( const void* obj, char* buf, size_t* count ) { return err_not_implemented; }
		static ErrorCode	fromBinary( void* obj, const char* buf, size_t* count ) { return err_not_implemented; }
		static ErrorCode	toBinary( const void* obj, char* buf, size_t* count ) { return err_not_implemented; }
		static unsigned int	serializerFlag() { return sef_none; }

		static const CustomDataHandler* defaultHandler() {
			RFLX_TASSERT( !IsPtr<T>::value );
			static CustomDataHandler handler = { name, klass, create, clone, destroy, copy, equal, less, great, noteq, lesseq, greateq, fromString, toString, fromBinary, toBinary, serializerFlag };
			return &handler;
		}

		typedef BaseCustomDataHandler base_type;
		typedef T data_type;
	};

	template< typename T >
	struct BaseCustomDataHandler< T, false >;

	template< typename T, bool IsCustomDataType = DataTypeTrait< T >::type_value == vdt_custom ||
		IsRflxable< T >::value && DataTypeTrait< T >::type_value == vdt_pointer ||
		DataTypeTrait< T >::value_class != vt_scalar >
	struct ICustomDataHandler;

	template< typename T >
	struct ICustomDataHandler< T, true > : BaseCustomDataHandler< typename RemoveQualifies< T >::type, true > {};

	template< typename T >
	struct ICustomDataHandler< T, false > { static const CustomDataHandler* defaultHandler() { return NULL; } };

	template< typename T, int CompareType = IsBaseType< T >::value ? 1 : ( DataTypeTrait< T >::type_value == vdt_custom ? 2 : 0 ) >
	struct ValuePredFunc;
	template< typename T >
	struct ValuePredFunc< T, 1 > { inline bool operator()( const T& l, const T& r ) { return l < r; } };
	template< typename T >
	struct ValuePredFunc< T, 2 > {
		inline bool operator()( const T& l, const T& r ) const { 
			ErrorCode err = err_ok;
			bool ret = IValueCmp< T >::pred( l, r, &err );
			if ( err_not_implemented == err ) {
				// this function should never failed!
				ret = ICustomDataHandler< T >::defaultHandler()->less( &l, &r );
				err = err_ok;
			}
			RFLX_DASSERT( err == err_ok && "Invalid Pred function!" );
			return ret;
		}
	};
	
	struct RFLX_API ValueData {
		union {
			bool _bool;
			wchar_t _wchar;
			char _char;
			signed char _schar;
			unsigned char _uchar;
			short _short;
			unsigned short _ushort;
			int _int;
			unsigned int _uint;
			long _long;
			unsigned long _ulong;
			long long _llong;
			unsigned long long _ullong;
			float _float;
			double _double;
			struct {
				char* _string;
				size_t _len;
			};
			struct {
				wchar_t* _wstring;
				size_t _wlen;
			};
			const void* _pointer;
			struct {
				long _enum;
				const EnumInfo* _enumInfo;
			};
			struct {
				void* _custom;
				CustomDataHandler* _customHandler;
			};
		};
		ValueDataType type;
		ValueData() : type( vdt_nil ) {}
		~ValueData();
		void clear();
		ValueData( const ValueData& other );
		explicit ValueData( bool value ) : type( vdt_bool ), _bool( value ) { }
		explicit ValueData( wchar_t value ) : type( vdt_wchar ), _wchar( value ) { }
		explicit ValueData( char value ) : type( vdt_char ), _char( value ) { }
		explicit ValueData( signed char value ) : type( vdt_schar ), _schar( value ) { }
		explicit ValueData( unsigned char value ) : type( vdt_uchar ), _uchar( value ) { }
		explicit ValueData( short value ) : type( vdt_short ), _short( value ) { }
		explicit ValueData( unsigned short value ) : type( vdt_ushort ), _ushort( value ) { }
		explicit ValueData( int value ) : type( vdt_int ), _int( value ) { }
		explicit ValueData( unsigned int value ) : type( vdt_uint ), _uint( value ) { }
		explicit ValueData( long value ) : type( vdt_long ), _long( value ) { }
		explicit ValueData( unsigned long value ) : type( vdt_ulong ), _ulong( value ) { }
		explicit ValueData( const long long& value ) : type( vdt_llong ), _llong( value ) { }
		explicit ValueData( const unsigned long long& value ) : type( vdt_ullong ), _ullong( value ) { }
		explicit ValueData( const float value ) : type( vdt_float ), _float( value ) { }
		explicit ValueData( const double& value ) : type( vdt_double ), _double( value ) { }
		explicit ValueData( const std::string& value );
		explicit ValueData( const std::wstring& value );
		explicit ValueData( const char* value );
		explicit ValueData( const wchar_t* value );
		explicit ValueData( const void* ptr );
		explicit ValueData( const void* custom, const CustomDataHandler* handler );
		ValueData& operator = ( const ValueData& other ) { return copy( other ); }
		ValueData& operator = ( bool value ) { clear(); type = vdt_bool, _bool = value; return *this; }
		ValueData& operator = ( wchar_t value ) { clear(); type = vdt_wchar, _wchar = value; return *this; }
		ValueData& operator = ( char value ) { clear(); type = vdt_char, _char = value; return *this; }
		ValueData& operator = ( signed char value ) { clear(); type = vdt_schar, _schar = value; return *this; }
		ValueData& operator = ( unsigned char value ) { clear(); type = vdt_uchar, _uchar = value; return *this; }
		ValueData& operator = ( short value ) { clear(); type = vdt_short, _short = value; return *this; }
		ValueData& operator = ( unsigned short value ) { clear(); type = vdt_ushort, _ushort = value; return *this; }
		ValueData& operator = ( int value ) { clear(); type = vdt_int, _int = value; return *this; }
		ValueData& operator = ( unsigned int value ) { clear(); type = vdt_uint, _uint = value; return *this; }
		ValueData& operator = ( long value ) { clear(); type = vdt_long, _long = value; return *this; }
		ValueData& operator = ( unsigned long value ) { clear(); type = vdt_ulong, _ulong = value; return *this; }
		ValueData& operator = ( const long long& value ) { clear(); type = vdt_llong, _llong = value; return *this; }
		ValueData& operator = ( const unsigned long long& value ) { clear(); type = vdt_ullong, _ullong = value; return *this; }
		ValueData& operator = ( float value ) { clear(); type = vdt_float, _float = value; return *this; }
		ValueData& operator = ( const double& value ) { clear(); type = vdt_double, _double = value; return *this; }
		ValueData& operator = ( const char* value );
		ValueData& operator = ( const wchar_t* value );
		ValueData& operator = ( const std::string& value );
		ValueData& operator = ( const std::wstring& value );
		ValueData& operator = ( const void* value );
		ValueData& assign( long enumValue, const EnumInfo* enumInfo );
		ValueData& assign( const void* custom, const CustomDataHandler* handler );
		ValueData& copy( const ValueData& other );
		ValueData& swap( ValueData& other );
		bool isNil() const { return type == vdt_nil; }
		bool isBaseType() const { return type > vdt_base_type_begin && type < vdt_base_type_end; }
		bool isNumber() const { return type > vdt_number_begin && type < vdt_number_end; }
		bool isInteger() const { return type > vdt_integer_number_begin && type < vdt_integer_number_end; }
		bool isBoolean() const { return type == vdt_bool; }
		bool isString() const { return type == vdt_wstring || type == vdt_string; }
		bool isUserData() const { return type == vdt_custom; }
		bool isPointer() const { return type == vdt_pointer; }
		bool isEnum() const { return type == vdt_enum; }
		bool isFloat() const { return type == vdt_float || type == vdt_double; }
		bool isSigned() const { return isNumber() && !( type > vdt_unsigned_integer_number_begin && type < vdt_unsigned_integer_number_end ); }
		template< typename T >
		explicit ValueData( const T& value ) throw() : type( vdt_nil ) { PackageObject< T >::invoke( *this, value ); }
		template< typename T >
		inline ValueData& operator = ( const T& value ) throw() { PackageObject< T >::invoke( *this, value ); return *this; }
		template< typename T >
		bool extract( T& output ) const throw() { return ExtractObject< T >::invoke( *this, output ); }
		ValueData cast( ValueDataType targetType ) const;
		bool toString( std::string& result, const CustomDataHandler* handler = NULL ) const throw();
		bool fromString( ValueDataType targetType, const std::string& source, const CustomDataHandler* handler = NULL, Ktex ktex = NULL ) throw();
		bool operator == ( const ValueData& rhs ) const;
		bool operator < ( const ValueData& rhs ) const;
		bool operator > ( const ValueData& rhs ) const;
		bool operator != ( const ValueData& rhs ) const;
		bool operator <= ( const ValueData& rhs ) const;
		bool operator >= ( const ValueData& rhs ) const;
		template< typename T > bool operator == ( const T& rhs ) const { return *this == ValueData( rhs ); }
		template< typename T > bool operator  < ( const T& rhs ) const { return *this  < ValueData( rhs ); }
		template< typename T > bool operator  > ( const T& rhs ) const { return *this  > ValueData( rhs ); }
		template< typename T > bool operator != ( const T& rhs ) const { return *this != ValueData( rhs ); }
		template< typename T > bool operator <= ( const T& rhs ) const { return *this <= ValueData( rhs ); }
		template< typename T > bool operator >= ( const T& rhs ) const { return *this >= ValueData( rhs ); }
	};

	template< typename T > inline bool operator == ( const T& lhs, const ValueData& rhs ) { return rhs == ValueData( lhs ); }
	template< typename T > inline bool operator  < ( const T& lhs, const ValueData& rhs ) { return rhs  > ValueData( lhs ); }
	template< typename T > inline bool operator  > ( const T& lhs, const ValueData& rhs ) { return rhs  < ValueData( lhs ); }
	template< typename T > inline bool operator != ( const T& lhs, const ValueData& rhs ) { return rhs != ValueData( lhs ); }
	template< typename T > inline bool operator <= ( const T& lhs, const ValueData& rhs ) { return rhs >= ValueData( lhs ); }
	template< typename T > inline bool operator >= ( const T& lhs, const ValueData& rhs ) { return rhs <= ValueData( lhs ); }

	template< typename T, typename TBase = typename RemoveQualifies< T >::type >
	struct BaseDataCastHelper {
		typedef TBase base_type;
		inline static base_type cast( const ValueData& value ) {
			const ValueDataType typeValue = DataTypeTrait< base_type >::type_value;
			RFLX_TASSERT( typeValue > vdt_base_type_begin && typeValue < vdt_base_type_end );
			RFLX_DASSERT( value.type > vdt_base_type_begin && value.type < vdt_base_type_end );
			if ( value.type != typeValue ) {
				// TODO: warning report output
			}
			switch ( value.type ) {
				case vdt_bool: return ( base_type )( value._bool ? 1 : 0 );
				case vdt_char: return ( base_type )value._char;
				case vdt_schar: return ( base_type )value._schar;
				case vdt_short: return ( base_type )value._short;
				case vdt_int: return ( base_type )value._int;
				case vdt_long: return ( base_type )value._long;
				case vdt_llong: return ( base_type )value._llong;
				case vdt_wchar: return ( base_type )value._wchar;
				case vdt_uchar: return ( base_type )value._uchar;
				case vdt_ushort: return ( base_type )value._ushort;
				case vdt_uint: return ( base_type )value._uint;
				case vdt_ulong: return ( base_type )value._ulong;
				case vdt_ullong: return ( base_type )value._ullong;
				case vdt_float: return ( base_type )value._float;
				case vdt_double: return ( base_type )value._double;
			}
			RFLX_DASSERT(0);
			return 0;
		}
	};
		
	template< typename T > 
	struct BaseDataCastHelper< T, bool > {
		typedef bool base_type;
		inline static bool cast( const ValueData& value ) 	{
			RFLX_DASSERT( value.type > vdt_base_type_begin && value.type < vdt_base_type_end );
			if ( value.type != vdt_bool ) {
				// TODO: warning report output
			}
			switch ( value.type ) {
				case vdt_bool: return !!value._bool;
				case vdt_char: return !!value._char;
				case vdt_schar: return !!value._schar;
				case vdt_short: return !!value._short;
				case vdt_int: return !!value._int;
				case vdt_long: return !!value._long;
				case vdt_llong: return !!value._llong;
				case vdt_wchar: return !!value._wchar;
				case vdt_uchar: return !!value._uchar;
				case vdt_ushort: return !!value._ushort;
				case vdt_uint: return !!value._uint;
				case vdt_ulong: return !!value._ulong;
				case vdt_ullong: return !!value._ullong;
				case vdt_float: return !!value._float;
				case vdt_double: return !!value._double;
			}
			RFLX_DASSERT(0);
			return false;
		}
	};


	template< typename T >
	inline bool ExtractValue( const ValueData& value, T& output );
	
	template< typename T >
	inline bool PackageValue( ValueData& value, const T& input ); ///< if you got error here, that maybe you forgot to create user-base-type implementation.
	
	template<> inline bool ExtractValue< std::string >( const ValueData& value, std::string& output ) {
		if ( value.type == vdt_string && value._string ) {
			output = value._string;
			return true;
		} else return false;
	}

	template<> inline bool ExtractValue< std::wstring >( const ValueData& value, std::wstring& output ) {
		if ( value.type == vdt_wstring && value._wstring ) {
			output = value._wstring;
			return true;
		} else return false;
	}

	template<> inline bool ExtractValue< const void* >( const ValueData& value, vcptr& output ) {
		if ( value.type == vdt_pointer ) {
			output = value._pointer;
			return true;
		} else return false;
	}
	
	template<> inline bool PackageValue< std::wstring >( ValueData& value, const std::wstring& input ) { value = input; return true; }
	template<> inline bool PackageValue< std::string >( ValueData& value, const std::string& input ) { value = input; return true; }
	template<> inline bool PackageValue< const char* >( ValueData& value, const pcstr& input ) { if ( !input ) return false; value = input; return true; }
	template<> inline bool PackageValue< const wchar_t* >( ValueData& value, const pcwstr& input ) { if ( !input ) return false; value = input; return true; }
	template<> inline bool PackageValue< const void* >( ValueData& value, const vcptr& input ) { value = input; return true; }
	
#define FLX_DEFINE_EXTRACTVALUE( TYPE, NAME )\
	template<> inline bool ExtractValue< TYPE >( const ValueData& value, TYPE& output ) { output = BaseDataCastHelper< TYPE >::cast( value ); return true; }\
	template<> inline bool PackageValue< TYPE >( ValueData& value, const TYPE& input ){ value.clear(); value.type = vdt_##NAME; value._##NAME = input; return true; }
	
	FLX_DEFINE_EXTRACTVALUE( bool, bool )
	FLX_DEFINE_EXTRACTVALUE( char, char )
	FLX_DEFINE_EXTRACTVALUE( signed char, schar )
	FLX_DEFINE_EXTRACTVALUE( wchar_t, wchar )
	FLX_DEFINE_EXTRACTVALUE( unsigned char, uchar )
	FLX_DEFINE_EXTRACTVALUE( short, short )
	FLX_DEFINE_EXTRACTVALUE( unsigned short, ushort )
	FLX_DEFINE_EXTRACTVALUE( int, int )
	FLX_DEFINE_EXTRACTVALUE( unsigned int, uint )
	FLX_DEFINE_EXTRACTVALUE( long, long )
	FLX_DEFINE_EXTRACTVALUE( unsigned long, ulong )
	FLX_DEFINE_EXTRACTVALUE( long long, llong )
	FLX_DEFINE_EXTRACTVALUE( unsigned long long, ullong )
	FLX_DEFINE_EXTRACTVALUE( float, float )
	FLX_DEFINE_EXTRACTVALUE( double, double )

	template< typename T >
	struct ExtractEnum { 
		static bool invoke( const ValueData& value, T& output ) { 
			RFLX_TASSERT( DataTypeTrait< T >::type_value == vdt_enum );
			if ( value.isEnum() ) {
				if ( !value._enumInfo || value.type != vdt_enum ) return false;
				if ( _internal::_strcmp( getEnumInfo< T >()->getName(), value._enumInfo->getName() ) == 0 ) {
					output = (T)value._enum;
					return true;
				}
			} else if ( value.isInteger() ) {
				long _value = 0;
				if ( value.extract( _value ) ) {
					const EnumValue* ei = getEnumInfo< T >()->findItemByValue( _value );
					if ( ei && ei->value == _value ) {
						output = (T)_value;
						return true;
					}
				}
			}
			return false;
		}
	};

	template< typename T >
	struct PackageEnum {
		static bool invoke( ValueData& value, const T& input ) {
			RFLX_TASSERT( DataTypeTrait< T >::type_value == vdt_enum );
			value.clear();
			value.type = vdt_enum;
			value._enum = input;
			const EnumInfo* _enumInfo = getEnumInfo< T >();
			if ( _enumInfo ) {
				value._enumInfo = _enumInfo;
				if ( (unsigned int)-1 == _enumInfo->getValueIndex( (long)input) ) {
					// not found value
					const EnumValue* eval = _enumInfo->getItemByIndex( 0 );
					if ( eval ) {
						value._enum = eval->value;
					} else {
						value._enum = 0;
					}
				}
				return true;
			} else {
				value._enumInfo = NULL;
				value.type = vdt_uint;
				value._uint = 0;
				return false;
			}
		}
	};

	template< typename T >
	struct ExtractPointer {
		static bool invoke( const ValueData& value, T& output ) {
			RFLX_TASSERT( DataTypeTrait< T >::type_value == vdt_pointer );
			if ( value.type == vdt_pointer ) {
				output = (T)value._pointer;
			} else if ( value.type == vdt_custom ) {
				output = (T)value._custom;
			} else {
				return false;
			}
			return true;
		}
	};

	template< typename T >
	struct PackagePointer {
		static bool invoke( ValueData& value, const T& input ) {
			RFLX_TASSERT( DataTypeTrait< T >::type_value == vdt_pointer );
			value = (const void*)input;
			return true;
		}
	};

	template< typename T >
	struct ExtractCustom {
		static bool invoke( const ValueData& value, T& output ) {
			RFLX_TASSERT( DataTypeTrait< T >::type_value == vdt_custom || DataTypeTrait< T >::value_class != vt_scalar ); 
			if ( value.type == vdt_custom && 
				( value._customHandler == ICustomDataHandler< T >::defaultHandler() || 
				value._customHandler->name() == ICustomDataHandler< T >::defaultHandler()->name() || 
				_internal::_strcmp( value._customHandler->name(), ICustomDataHandler< T >::defaultHandler()->name() ) == 0 ) )
			{
				value._customHandler->copy( &output, value._custom );
				return true;
			} else {
				return false;
			}
		}
	};

	template< typename T >
	struct PackageCustom {
		static bool invoke( ValueData& value, const T& input ) {
			RFLX_TASSERT( DataTypeTrait< T >::type_value == vdt_custom || DataTypeTrait< T >::value_class != vt_scalar );
			value.assign( (const void*)&input, ICustomDataHandler< T >::defaultHandler() );
			return !value.isNil();
		}
	};

	template< typename T >
	struct ExtractBaseType {
		static bool invoke( const ValueData& value, T& output ) {
			RFLX_TASSERT( DataTypeTrait< T >::type_value > vdt_base_type_begin && DataTypeTrait< T >::type_value < vdt_base_type_end ||
				DataTypeTrait< T >::type_value == vdt_string ||
				DataTypeTrait< T >::type_value == vdt_wstring ); 
			return ExtractValue< T >( value, output );
		}
	};

	template< typename T > 
	struct PackageBaseType {
		static inline bool invoke( ValueData& value, const T& input ) {
			RFLX_TASSERT( DataTypeTrait< T >::type_value > vdt_base_type_begin && DataTypeTrait< T >::type_value < vdt_base_type_end ||
				DataTypeTrait< T >::type_value == vdt_string ||
				DataTypeTrait< T >::type_value == vdt_wstring );
			return PackageValue< T >( value, input ); 
		}
	};

	template< typename T > 
	struct ExtractRef {
		static inline bool invoke( const ValueData& value, T& output ) {
			RFLX_TASSERT( DataTypeTrait< T >::type_value == vdt_pointer );
			if ( value.type == vdt_pointer ) {
				output = *( typename RemoveRef< T >::type* )value._pointer;
			} else if ( value.type == vdt_custom ) {
				output = *( typename RemoveRef< T >::type* )value._custom;
			} else {
				return false;
			}
			return true;
		} 
	};

	template< typename T > 
	struct PackageRef {
		static inline bool invoke( ValueData& value, T input ) {
			RFLX_TASSERT( DataTypeTrait< T >::type_value == vdt_pointer );
			typename RemoveRef< T >::type* p = &input;
			value = (const void*)p;
			return true;
		}
	};
	
	template<> struct ExtractCustom< ValueData > { static inline bool invoke( const ValueData& value, ValueData& output ) { output = value; return !output.isNil(); } };
	template<> struct PackageCustom< ValueData > { static inline bool invoke( ValueData& value, const ValueData& input ) { value = input; return !value.isNil(); } };
	
	template< typename T >
	struct ExtractObject { typedef
		typename If< DataTypeTrait< T >::value_class == vt_scalar,
			typename If< IsEnum< T >::value,
				ExtractEnum< T >,
				typename If< IsPtr< T >::value,
					ExtractPointer< T >,
					typename If< IsRef< T >::value,
						ExtractRef< T >,
						typename If< DataTypeTrait< T >::type_value == vdt_custom,
							ExtractCustom< T >,
							ExtractBaseType< T >
						>::type
					>::type
				>::type >::type,
				ExtractCustom< T > // if T is not a scalar, treat it as a custom data
		>::type extractor_type;
		static bool invoke( const ValueData& value, T& output ) { return extractor_type::invoke( value, output ); }
	};

	template< typename T >
	struct PackageObject { typedef 
		typename If< DataTypeTrait< T >::value_class == vt_scalar,
			typename If< IsEnum< T >::value,
				PackageEnum< T >,
				typename If< IsPtr< T >::value,
					PackagePointer< T >,
					typename If< IsRef< T >::value,
						PackageRef< T >,
						typename If< DataTypeTrait< T >::type_value == vdt_custom,
							PackageCustom< T >,
							PackageBaseType< T >
						>::type
					>::type
				>::type >::type,
				PackageCustom< T > // if T is not a scalar, treat it as a custom data
		>::type packer_type;
		static bool invoke( ValueData& value, const T& input ) { return packer_type::invoke( value, input ); }
	};

	template< typename T, bool IsCustomDataType = DataTypeTrait< T >::type_value == vdt_custom && IsKindOf< T, Rflxable >::value >
	struct DefaultPropCustomDataInfoAccesser { 
		static const PropDef* getPropDefs( Klass* klass, unsigned int* count, RflxObject* object = NULL ) {
			Klass _klass = T::_class();
			bool isRflxObject = IsKindOf< T, RflxObject >::value;
			if ( object && isRflxObject ) {
				_klass = object->_dynamicClass();
			} 
			if ( klass ) {
				*klass = _klass;
			}
			return T::_getPropDefs( count );
		} 
	};

	template< typename T >
	struct DefaultPropCustomDataInfoAccesser< T, false > {
		static const PropDef* getPropDefs( Klass* klass, unsigned int* count, RflxObject* object = NULL ) {
			if ( klass ) {
				*klass = NULL;
			} 
			if ( count ) {
				*count = 0;
			}
			return NULL;
		}
	};

	struct NullDataTypeAccesser {
		static void* get_ref( const PropDef*, unsigned int, void* ) { return NULL; }
		static ErrorCode get( const PropDef*, unsigned int, void*, ValueData* ) { return err_not_implemented; }
		static ErrorCode set( const PropDef*, unsigned int, void*, const ValueData* ) { return err_not_implemented; }
		static ErrorCode op( const PropDef*, void*, PropOp, va_list ) { return err_not_implemented; }
	};

	template< typename T >
	struct IsConv2Int {
		const static bool value = DataTypeTrait< T >::type_value == vdt_bool ||
			DataTypeTrait< T >::type_value > vdt_integer_number_begin && DataTypeTrait< T >::type_value < vdt_integer_number_end ||
			DataTypeTrait< T >::type_value == vdt_enum && sizeof( int ) >= sizeof( T );
	};

	template< typename T >
	struct ValueDataTypeAccesser {
		static void* get_ref( const PropDef* def, unsigned int, void* userObject ) {
			return (char*)userObject + def->offset;
		}
		static ErrorCode get( const PropDef* def, unsigned int, void* userObject, ValueData* val ) {
			*val = ValueData( *( const T* )( (char*)userObject + def->offset ) );
			RFLX_DASSERT( val->type == DataTypeTrait< T >::type_value );
			return err_ok;
		}
		static ErrorCode set( const PropDef* def, unsigned int, void* userObject, const ValueData* val ) {
			char* target = (char*)userObject + def->offset;
			ExtractObject< T >::invoke( *val, *(T*)target );
			return err_ok;
		}
		static ErrorCode op( const PropDef*, void*, PropOp, va_list ) { return err_not_implemented; }
	};

	template< typename T, int N >
	struct ValueDataTypeAccesser< FArray< T, N > > {
		static void* get_ref( const PropDef* def, unsigned int index, void* userObject ) {
			FArray< T, N >& _array = *( FArray< T, N >* )( (char*)userObject + def->offset );
			RFLX_DASSERT( index < N );
			return &_array[ index ];
		}
		static ErrorCode get( const PropDef* def, unsigned int index, void* userObject, ValueData* val ) {
			const FArray< T, N >& _array = *( const FArray< T, N >* )( (char*)userObject + def->offset );
			*val = _array[ index ];
			RFLX_DASSERT( index < N );
			RFLX_DASSERT( val->type == DataTypeTrait< T >::type_value );
			return err_ok;
		}
		static ErrorCode set( const PropDef* def, unsigned int index, void* userObject, const ValueData* val ) {
			FArray< T, N >& _array = *( FArray< T, N >* )( (char*)userObject + def->offset );
			RFLX_DASSERT( index < N );
			ExtractObject< T >::invoke( *val, _array[ index ] );
			return err_ok;
		}
		static ErrorCode op( const PropDef* def, void* userObject, PropOp opType, va_list va ) 
		{
			switch ( opType ) {
			case op_size:
				{
					size_t* _size = va_arg( va, size_t* );
					if ( _size ) {
						*_size = ( size_t )N;
					}
					return err_ok;
				}
				break;
			case op_empty:
				{			
					bool* _empty = va_arg( va, bool* );
					if ( _empty ) {
						*_empty = false;
					}
					return err_ok;
				}
				break;
			case op_getref:
				{
					size_t _index = va_arg( va, size_t );
					void** _ref = va_arg( va, void** );
					if ( _ref ) {
						FArray< T, N >& _array = *( FArray< T, N >* )( (char*)userObject + def->offset );
						*_ref = &_array[ _index ];
						RFLX_DASSERT( _index < N );
					}
				}
				break;
			case op_index:
				{			
					// arg1 : size_t index
					// arg2 : ValueData* val
					size_t _index = va_arg( va, size_t );
					ValueData* val = va_arg( va, ValueData* );
					RFLX_DASSERT( val );
					FArray< T, N >& _array = *( FArray< T, N >* )( (char*)userObject + def->offset );
					if ( (size_t)_index >= (size_t)N ) {
						return err_array_out_of_range;
					}
					val->extract( _array[ _index ] );
				}
				break;
			case op_at:
				{
					// arg1 : size_t index
					// arg2 : ValueData* val
					size_t _index = va_arg( va, size_t );
					ValueData* val = va_arg( va, ValueData* );
					RFLX_DASSERT( val );
					FArray< T, N >& _array = *( FArray< T, N >* )( (char*)userObject + def->offset );
					if ( (size_t)_index >= (size_t)N ) {
						return err_array_out_of_range;
					}
					*val = _array[ _index ];
				}
				break;
			default:
				return err_not_implemented;
			}
			return err_ok;
		}
	};

	template< typename T, int N >
	struct ValueDataTypeAccesser< T[ N ] > : ValueDataTypeAccesser< FArray< T, N > > {};

#ifdef RFLX_STL_VECTOR_SUPPORT
	template< typename T, typename Allocator >
	struct ValueDataTypeAccesser< std::vector< T, Allocator > > {
		typedef std::vector< T, Allocator > vector_type;
		static void* get_ref( const PropDef* def, unsigned int index, void* userObject ) {
			vector_type& _array = *( vector_type* )( (char*)userObject + def->offset );
			RFLX_DASSERT( (size_t)index < _array.size() );
			return &_array[ index ];
		}
		static ErrorCode get( const PropDef* def, unsigned int index, void* userObject, ValueData* val ) {
			const vector_type& _array = *( const vector_type* )( (char*)userObject + def->offset );
			if ( (size_t)index >= _array.size() ) {
				return err_array_out_of_range;
			}
			*val = _array[ index ];
			RFLX_DASSERT( (size_t)index < _array.size() );
			RFLX_DASSERT( val->type == DataTypeTrait< T >::type_value );
			return err_ok;
		}
		static ErrorCode set( const PropDef* def, unsigned int index, void* userObject, const ValueData* val ) {
			vector_type& _array = *( vector_type* )( (char*)userObject + def->offset );
			if ( (size_t)index >= _array.size() ) {
				return err_array_out_of_range;
			}
			RFLX_DASSERT( (size_t)index < _array.size() );
			ExtractObject< T >::invoke( *val, _array[ index ] );
			return err_ok;
		}	
		static ErrorCode op( const PropDef* def, void* userObject, PropOp opType, va_list va ) 
		{
			vector_type& _array = *( vector_type* )( (char*)userObject + def->offset );
			switch ( opType ) {
			case op_size:
				{
					// arg1 size_t* for output
					size_t* _size = va_arg( va, size_t* );
					if ( _size ) {
						*_size = _array.size();
					}
				}
				break;
			case op_empty:
				{			
					// arg1 bool*
					bool* _empty = va_arg( va, bool* );
					if ( _empty ) {
						*_empty = false;
					}
				}
				break;
			case op_clear:
				_array.clear();
				break;
			case op_resize: 
				{
					// arg1 size_t
					size_t _size = va_arg( va, size_t );
					_array.resize( _size );
				}
				break;
			case op_reserve:
				{
					// arg1 size_t
					size_t _size = va_arg( va, size_t );
					_array.reserve( _size );
				}
				break;
			case op_pop_back:
				_array.pop_back();
				break;
			case op_push_back:
				{
					// arg1 const ValueData*
					const ValueData* value = va_arg( va, const ValueData* );
					T _element;
					ExtractObject< T >::invoke( *value, _element );
					_array.push_back( _element );
				}
				break;
			case op_front:
				{
					// arg1 T* for output
					T* _element = va_arg( va, T* );
					_element = &_array.front();
				}
				break;
			case op_back:
				{
					// arg1 T* for output
					T* _element = va_arg( va, T* );
					_element = &_array.back();
				}
				break;
			case op_insert:
				{
					// arg1 : const ValueData*
					// arg2 : insert position
					const ValueData* value = va_arg( va, const ValueData* );
					T _element;
					ExtractObject< T >::invoke( *value, _element );
					size_t index = va_arg( va, size_t );
					_array.insert( _array.begin() + index, _element );
				}
				break;
			case op_erase:
				{
					// arg1: size_t index
					size_t index = va_arg( va, size_t );
					_array.erase( _array.begin() + index );
				}
				break;	
			case op_getref:
				{
					size_t _index = va_arg( va, size_t );
					void** _ref = va_arg( va, void** );
					if ( _ref ) {
						vector_type& _array = *( vector_type* )( (char*)userObject + def->offset );
						*_ref = &_array[ _index ];
						RFLX_DASSERT( (size_t)_index < _array.size() );
					}
				}
				break;	
			case op_index:
				{
					// arg1 : size_t index
					// arg2 : ValueData* val
					size_t _index = va_arg( va, size_t );
					ValueData* val = va_arg( va, ValueData* );
					RFLX_DASSERT( val );
					vector_type& _array = *( vector_type* )( (char*)userObject + def->offset );
					if ( ( size_t )_index >= _array.size() ) {
						return err_array_out_of_range;
					}
					val->extract( _array[ _index ] );
				}
				break;
			case op_at:
				{
					// arg1 : size_t index
					// arg2 : ValueData* val
					size_t _index = va_arg( va, size_t );
					ValueData* val = va_arg( va, ValueData* );
					RFLX_DASSERT( val );
					vector_type& _array = *( vector_type* )( (char*)userObject + def->offset );
					if ( ( size_t )_index >= _array.size() ) {
						return err_array_out_of_range;
					}
					*val = _array[ _index ];
				}
				break;
			default:
				return err_not_implemented;
			}
			return err_ok;
		}
	};
#endif //RFLX_STL_VECTOR_SUPPORT

#ifdef RFLX_STL_SET_SUPPORT
	template< typename T, typename Pred, typename Allocator >
	struct ValueDataTypeAccesser< std::set< T, Pred, Allocator > > {
		typedef std::set< T, Pred, Allocator > set_type;
		static void* get_ref( const PropDef* def, unsigned int index, void* userObject ) {
			set_type& _set = *( set_type* )( (char*)userObject + def->offset );
			RFLX_DASSERT( ( size_t )index < _set.size() );
			typename set_type::iterator it = _set.begin();
			for ( unsigned int i = 0; i < index; ++i ) {
				++it;
			}
			return const_cast< T* >(&*it);
		}
		static ErrorCode get( const PropDef* def, unsigned int index, void* userObject, ValueData* val ) {
			const set_type& _set = *( const set_type* )( (char*)userObject + def->offset );
			if ( (size_t)index >= _set.size() ) {
				return err_array_out_of_range;
			}
			typename set_type::const_iterator it = _set.begin();
			for ( unsigned int i = 0; i < index; ++i ) {
				++it;
			}
			*val = *it;
			RFLX_DASSERT( val->type == DataTypeTrait< T >::type_value );
			return err_ok;
		}
		static ErrorCode set( const PropDef* def, unsigned int index, void* userObject, const ValueData* val ) {
			set_type& _set = *( set_type* )( (char*)userObject + def->offset );
			if ( (size_t)index >= _set.size() ) {
				return err_array_out_of_range;	
			}
			typename set_type::iterator it = _set.begin();
			for ( unsigned int i = 0; i < index; ++i ) {
				++it;
			}
			ExtractObject< T >::invoke( *val, const_cast< T& >(*it) );
			return err_ok;
		}
		static ErrorCode op( const PropDef* def, void* userObject, PropOp opType, va_list va )
		{ 			
			set_type& _set = *( set_type* )( (char*)userObject + def->offset );
			switch ( opType ) {
			case op_size:
				{
					// arg1 size_t* for output
					size_t* _size = va_arg( va, size_t* );
					if ( _size ) {
						*_size = _set.size();
					}
				}
				break;
			case op_empty:
				{			
					// arg1 bool*
					bool* _empty = va_arg( va, bool* );
					if ( _empty ) {
						*_empty = _set.empty();
					}
				}
				break;
			case op_clear:
				_set.clear();
				break;
			case op_insert:
				{
					// arg1 : const ValueData*
					// arg2 : return bool if success
					const ValueData* value = va_arg( va, const ValueData* );
					bool* ret = va_arg( va, bool* );
					T _element;
					ExtractObject< T >::invoke( *value, _element );
					bool b = _set.insert( _element ).second;
					if ( ret ) {
						*ret = b;
					}
				}
				break;
			case op_erase:
				{
					// arg1 : const ValueData*
					const ValueData* value = va_arg( va, const ValueData* );
					T _element;
					ExtractObject< T >::invoke( *value, _element );
					typename set_type::iterator it = _set.find( _element );
					if ( it != _set.end() ) {
						_set.erase( it );
					}
				}
				break;	
			case op_at:
				{
					// arg1 : size_t index
					// arg2 : ValueData* val
					size_t index = va_arg( va, size_t );
					ValueData* val = va_arg( va, ValueData* );
					RFLX_DASSERT( val );
					if ( (size_t)index >= _set.size() )
					return err_array_out_of_range;		
					typename set_type::iterator it = _set.begin();
					for ( unsigned int i = 0; i < index; ++i ) {
						++it;
					}
					if ( it != _set.end() ) {
						*val = *it;
					}
				}
				break;
			default:
				return err_not_implemented;
			}
			return err_ok;
		}
	};

#endif // RFLX_STL_SET_SUPPORT

#ifdef RFLX_STL_MAP_SUPPORT
	template< typename TMap, bool IsIntegerKey >
	struct Map_ValueDataTypeAccesser;
	template< typename Key, typename T, typename Pred, typename Allocator >
	struct Map_ValueDataTypeAccesser< std::map< Key, T, Pred, Allocator >, true > {
		typedef std::map< Key, T, Pred, Allocator > map_type;
		static void* get_ref( const PropDef* def, unsigned int index, void* userObject ) {
			map_type& _map = *( map_type* )( (char*)userObject + def->offset );
			return &_map[ (Key)index ];
		}
		static ErrorCode get( const PropDef* def, unsigned int index, void* userObject, ValueData* val ) {
			map_type& _map = *( map_type* )( (char*)userObject + def->offset );
			*val = _map[ (Key)index ];
			RFLX_DASSERT( val->type == DataTypeTrait< T >::type_value );
			return err_ok;
		}
		static ErrorCode set( const PropDef* def, unsigned int index, void* userObject, const ValueData* val ) {
			map_type& _map = *( map_type* )( (char*)userObject + def->offset );
			ExtractObject< T >::invoke( *val, _map[ (Key)index ] );
			RFLX_DASSERT( val->type == DataTypeTrait< T >::type_value );
			return err_ok;
		}
		static ErrorCode op( const PropDef* def, void* userObject, PropOp opType, va_list va )
		{
			map_type& _map = *( map_type* )( (char*)userObject + def->offset );
			switch ( opType ) {
			case op_size:
				{
					// arg1 size_t* for output
					size_t* _size = va_arg( va, size_t* );
					if ( _size ) {
						*_size = _map.size();
					}
				}
				break;
			case op_empty:
				{			
					// arg1 bool*
					bool* _empty = va_arg( va, bool* );
					if ( _empty ) {
						*_empty = _map.empty();
					}
				}
				break;
			case op_clear:
				_map.clear();
				break;
			case op_insert:
				{
					// arg1 : const ValueData* Key
					// arg2 : const ValueData* T
					// arg3 : return bool if success
					const ValueData* keyValue = va_arg( va, const ValueData* );
					Key _key;
					ExtractObject< Key >::invoke( *keyValue, _key );
					const ValueData* value = va_arg( va, const ValueData* );
					T _element;
					ExtractObject< T >::invoke( *value, _element );
					bool* ret = va_arg( va, bool* );
					bool b = _map.insert( std::make_pair( _key, _element ) ).second;
					if ( ret ) {
						*ret = b;
					}
				}
				break;
			case op_erase:
				{
					// arg1 : const ValueData* key
					const ValueData* value = va_arg( va, const ValueData* );
					Key _key;
					ExtractObject< Key >::invoke( *value, _key );
					typename map_type::iterator it = _map.find( _key );
					if ( it != _map.end() ) {
						_map.erase( it );
					}
				}
				break;	
			case op_getref:
				{
					const ValueData* value = va_arg( va, const ValueData* );
					Key _key;
					ExtractObject< Key >::invoke( *value, _key );
					void** _ref = va_arg( va, void** );
					if ( _ref ) {
						map_type& _map = *( map_type* )( (char*)userObject + def->offset );
						typename map_type::iterator it = _map.find( _key );
						if ( it != _map.end() ) {
							*_ref = &( it->second );
						} else {
							*_ref = NULL;
						}
					}
				}
				break;
			case op_at:
				{
					// arg1 : size_t index
					// arg2 : ValueData* key
					// arg3 : ValueData* val
					size_t index = va_arg( va, size_t );
					ValueData* key = va_arg( va, ValueData* );
					ValueData* val = va_arg( va, ValueData* );
					RFLX_DASSERT( key && val );
					typename map_type::iterator it = _map.begin();
					for ( unsigned int i = 0; i < index && it != _map.end(); ++i ) {
						++it;
					}
					if ( it != _map.end() ) {
						*key = it->first;
						*val = it->second;
					}
				}
				break;
			default:
				return err_not_implemented;
			}
			return err_ok;
		}
	};

	template< typename Key, typename T, typename Pred, typename Allocator >
	struct Map_ValueDataTypeAccesser< std::map< Key, T, Pred, Allocator >, false > {
		typedef std::map< Key, T, Pred, Allocator > map_type;
		static void* get_ref( const PropDef* def, unsigned int index, void* userObject ) {
			map_type& _map = *( map_type* )( (char*)userObject + def->offset );
			RFLX_DASSERT( (size_t)index < _map.size() );
			typename map_type::iterator it = _map.begin();
			for ( unsigned int i = 0; i < index; ++i ) {
				++it;
			}
			return &( it->second );
		}
		static ErrorCode get( const PropDef* def, unsigned int index, void* userObject, ValueData* val ) {
			map_type& _map = *( map_type* )( (char*)userObject + def->offset );
			if ( (size_t)index >= _map.size() ) {
				return err_array_out_of_range;	
			}
			typename map_type::const_iterator it = _map.begin();
			for ( unsigned int i = 0; i < index; ++i ) {
				++it;
			}
			*val = it->second;
			RFLX_DASSERT( val->type == DataTypeTrait< T >::type_value );
			return err_ok;
		}
		static ErrorCode set( const PropDef* def, unsigned int index, void* userObject, const ValueData* val ) {
			map_type& _map = *( map_type* )( (char*)userObject + def->offset );
			if ( (size_t)index >= _map.size() ) {
				return err_array_out_of_range;
			}
			typename map_type::iterator it = _map.begin();
			for ( unsigned int i = 0; i < index; ++i ) {
				++it;
			}
			ExtractObject< T >::invoke( *val, it->second );
			RFLX_DASSERT( val->type == DataTypeTrait< T >::type_value );
			return err_ok;
		}
		static ErrorCode op( const PropDef* def, void* userObject, PropOp opType, va_list va )
		{
			map_type& _map = *( map_type* )( (char*)userObject + def->offset );
			switch ( opType ) {
			case op_size:
				{
					// arg1 size_t* for output
					size_t* _size = va_arg( va, size_t* );
					if ( _size ) {
						*_size = _map.size();
					}
				}
				break;
			case op_empty:
				{			
					// arg1 bool*
					bool* _empty = va_arg( va, bool* );
					if ( _empty ) {
						*_empty = _map.empty();
					}
				}
				break;
			case op_clear:
				_map.clear();
				break;
			case op_insert:
				{
					// arg1 : const ValueData* Key
					// arg2 : const ValueData* T
					// arg3 : return bool if success
					const ValueData* keyValue = va_arg( va, const ValueData* );
					Key _key;
					ExtractObject< Key >::invoke( *keyValue, _key );
					const ValueData* value = va_arg( va, const ValueData* );
					T _element;
					ExtractObject< T >::invoke( *value, _element );
					bool* ret = va_arg( va, bool* );
					bool b = _map.insert( std::make_pair( _key, _element ) ).second;
					if ( ret ) {
						*ret = b;
					}
				}
				break;
			case op_erase:
				{
					// arg1 : const ValueData* key
					const ValueData* value = va_arg( va, const ValueData* );
					Key _key;
					ExtractObject< Key >::invoke( *value, _key );
					typename map_type::iterator it = _map.find( _key );
					if ( it != _map.end() ) {
						_map.erase( it );
					}
				}
				break;	
			case op_getref:
				{
					const ValueData* value = va_arg( va, const ValueData* );
					Key _key;
					ExtractObject< Key >::invoke( *value, _key );
					void** _ref = va_arg( va, void** );
					if ( _ref ) {
						map_type& _map = *( map_type* )( (char*)userObject + def->offset );
						typename map_type::iterator it = _map.find( _key );
						if ( it != _map.end() ) {
							*_ref = &( it->second );
						} else {
							*_ref = NULL;
						}
					}
				}
				break;	
			case op_at:
				{
					// arg1 : size_t index
					// arg2 : ValueData* key
					// arg3 : ValueData* val
					size_t index = va_arg( va, size_t );
					ValueData* key = va_arg( va, ValueData* );
					ValueData* val = va_arg( va, ValueData* );
					RFLX_DASSERT( key && val );
					typename map_type::iterator it = _map.begin();
					for ( unsigned int i = 0; i < index && it != _map.end(); ++i ) {
						++it;
					}
					if ( it != _map.end() ) {
						*key = it->first;
						*val = it->second;
					}
				}
				break;
			default:
				return err_not_implemented;
			}
			return err_ok;
		}
	};

#endif // RFLX_STL_MAP_SUPPORT
	template< typename T >
	struct DataGetSetSelector {
		typedef typename DataTrait< T >::internal_types internal_types;
		typedef typename If< DataTypeTrait< T >::value_class == vt_scalar, T,
			typename internal_types::head >::type type;
		typedef typename internal_types::head type2;
		typedef ValueDataTypeAccesser< T > setor_type;
		typedef ValueDataTypeAccesser< T > getor_type;
	};

#ifdef RFLX_STL_MAP_SUPPORT
	template< typename TKey, typename TValue, typename Pred, typename Allocator >
	struct DataGetSetSelector< std::map< TKey, TValue, Pred, Allocator > > {
		typedef TValue type;
		typedef TKey type2;
		typedef typename DataTrait< std::map< TKey, TValue, Pred, Allocator > >::internal_types internal_types;
		typedef std::map< TKey, TValue, Pred, Allocator > compound_type;
		typedef Map_ValueDataTypeAccesser< compound_type, IsConv2Int< type2 >::value > setor_type;
		typedef Map_ValueDataTypeAccesser< compound_type, IsConv2Int< type2 >::value > getor_type;
	};	
#endif // RFLX_STL_MAP_SUPPORT

	template< typename T >
	struct TypeTraitsBits {
		const static unsigned short value =
			( IsPtr< T >::value ? ptf_pointer : 0 ) |
			( IsPtrConst< T >::value ? ptf_pointer_const : 0 ) |
			( IsRef< T >::value ? ptf_ref : 0 ) |
			( IsRefConst< T >::value ? ptf_ref_const : 0 ) |
			( IsConst< T >::value ? ptf_const : 0 ) |
			( IsVolatile< T >::value ? ptf_volatile : 0 ) |
			( IsKindOf< typename RemovePR< T >::type, Rflxable >::value ? ptf_rflxable : 0 ) |
			( IsKindOf< typename RemovePR< T >::type, RflxObject >::value ? ptf_rflxobject : 0 ) |
			( IsKindOf< typename RemovePR< T >::type, RflxDynamicObject >::value ? ptf_rflxobject_d : 0 ) |
			( IsPolymorphic< typename RemovePR< T >::type >::value ? ptf_polymorphic : 0 ) |
			( IsEnum< T >::value ? ptf_enum : 0 );
	};

	template< class T >
	struct ValueDataPred { 
		bool operator()( const ValueData& left, const ValueData& right ) const {
			if ( left.type != right.type ) {
				// TODO: warning
			}
			ValuePredFunc< T > pred;
			T l, r;
			left.extract( l );
			right.extract( r );
			return pred( l, r );
		}
	};

	template< class T >
	const ValueData* makeDefaultValue( const T& value )
	{
#ifdef RFLX_STL_SET_SUPPORT
		static std::set< ValueData, ValueDataPred< T > > _bank;
		typename std::set< ValueData, ValueDataPred< T > >::iterator it;
		ValueData _value( value );
		if ( _value.isNil() ) {
			RFLX_DASSERT( 0 && "rflx::makeDefaultValue failed!" );
			return NULL;
		}
		it = _bank.find( _value );
		if ( it != _bank.end() ) {
			return &*it;
		}
		return &*( _bank.insert( _value ).first );
#else
		RFLX_DASSERT( 0 && "rflx::makeDefaultValue is not supported!" );
		return NULL;
#endif
	}

	inline const ValueData* makeDefaultValue( void ) { return NULL; }
	inline const ValueData* makeDefaultValue( NullType ) { return NULL; }

#define RFLX_MAKE_DEFAULT_VALUE_DECL( TYPE ) \
	RFLX_API const ValueData* makeDefaultValue( const TYPE& value );

	RFLX_MAKE_DEFAULT_VALUE_DECL( bool )
	RFLX_MAKE_DEFAULT_VALUE_DECL( char )
	RFLX_MAKE_DEFAULT_VALUE_DECL( signed char )
	RFLX_MAKE_DEFAULT_VALUE_DECL( short )
	RFLX_MAKE_DEFAULT_VALUE_DECL( int )
	RFLX_MAKE_DEFAULT_VALUE_DECL( long )
	RFLX_MAKE_DEFAULT_VALUE_DECL( long long )
	RFLX_MAKE_DEFAULT_VALUE_DECL( wchar_t )
	RFLX_MAKE_DEFAULT_VALUE_DECL( unsigned char )
	RFLX_MAKE_DEFAULT_VALUE_DECL( unsigned short )
	RFLX_MAKE_DEFAULT_VALUE_DECL( unsigned int )
	RFLX_MAKE_DEFAULT_VALUE_DECL( unsigned long )
	RFLX_MAKE_DEFAULT_VALUE_DECL( unsigned long long )
	RFLX_MAKE_DEFAULT_VALUE_DECL( float )
	RFLX_MAKE_DEFAULT_VALUE_DECL( double )
#undef RFLX_MAKE_DEFAULT_VALUE_DECL

	template< typename TOwner, typename TData >
	PropDef _addPropDef( unsigned int flags, TData TOwner::*, size_t _offset, unsigned int _id, const char* _name,
		const char* _userName, const char* _groupName, const char* _userType, const ValueData* _default,
		pfn_get _get, pfn_get_ref _get_ref, pfn_set _set, pfn_get_propdef _getDef, pfn_operator _op, const char* _editorData )
	{
		typedef typename RemoveConst< TData >::type _TData;
		typedef DataGetSetSelector< _TData > TDataHandelType;
		typedef typename TDataHandelType::type TDataHandelType_type;
		typedef typename TDataHandelType::type2 TDataHandelType_type2;
		typedef typename RemoveCV< TDataHandelType_type >::type TBaseDataType;
		typedef typename RemoveCV< TDataHandelType_type2 >::type TBaseDataType2;

		typedef typename TDataHandelType::getor_type TDataHandelType_getor_type;
		typedef typename TDataHandelType::setor_type TDataHandelType_setor_type;
		if ( !_get ) {
			_get = TDataHandelType_getor_type::get;
		}
		if ( !_get_ref ) {
			_get_ref = TDataHandelType_getor_type::get_ref;
		}
		if ( !_set ) {
			_set = TDataHandelType_setor_type::set;
		}
		const CustomDataHandler* _handler = ICustomDataHandler< TBaseDataType >::defaultHandler();
		const CustomDataHandler* _handler2 = ICustomDataHandler< TBaseDataType2 >::defaultHandler();
		if ( !_getDef ) {
			_getDef = DefaultPropCustomDataInfoAccesser< typename RemovePR< TBaseDataType >::type >::getPropDefs;
		}
		if ( !_op ) {
			_op = TDataHandelType_setor_type::op;
		}
		const unsigned short traits_bits = TypeTraitsBits< TDataHandelType_type >::value;
		const unsigned short traits_bits2 = TypeTraitsBits< TDataHandelType_type2 >::value;
		PropDef def = { ( unsigned int )( flags | IsConst< TData >::value ? pdf_read_only : 0 ),
			_id, _offset, _name, _userName, _groupName, _userType,
			DataTypeTrait< TData >::value_class,
			DataTypeTrait< TDataHandelType_type >::type_value,
			DataTypeTrait< TDataHandelType_type2 >::type_value,
			traits_bits, traits_bits2, _handler, _handler2, 
			getEnumInfo< TBaseDataType >(),
			getEnumInfo< TBaseDataType2 >(),
			_default, _get, _get_ref, _set, _getDef, _op, _editorData };
		return def;
	}

	template< class T, class TBase, bool Enable = false, bool IsRflxObject = IsKindOf< TBase, RflxObject >::value >
	struct RttiInitializerHelper{};

	template< class T, class TBase >
	struct RttiInitializerHelper< T, TBase, true, false > {
		RttiInitializerHelper() { rflx::_internal::_addClassInstanceCount( T::_class() ); }
		RttiInitializerHelper( const RttiInitializerHelper& ) { rflx::_internal::_addClassInstanceCount( T::_class() ); }
		~RttiInitializerHelper() { rflx::_internal::_releaseClassInstanceCount( T::_class() ); }
	};

	template< class T, class TBase >
	struct RttiInitializerHelper< T, TBase, true, true > {
		RttiInitializerHelper() {
			T* object = reinterpret_cast< T* >( (char*)this - RFLX_OFFSETOF( T, __RttiInitializer__ ) );
			rflx::_internal::_initObjectRtti( object, T::_class() );
			rflx::_internal::_addClassInstanceCount( T::_class() );
		}
		RttiInitializerHelper( const RttiInitializerHelper& ) {
			T* object = reinterpret_cast< T* >( (char*)this - RFLX_OFFSETOF( T, __RttiInitializer__ ) );
			rflx::_internal::_initObjectRtti( object, T::_class() );
			rflx::_internal::_addClassInstanceCount( T::_class() );
		}
		~RttiInitializerHelper()
		{ 
			rflx::_internal::_releaseClassInstanceCount( T::_class() );
		}
	};

	template< class TDst, class TSrc >
	struct DynamicCastHelperBase {
		typedef typename If< IsPtrConst< TDst >::value,
			typename RemoveConst< TDst >::type,
			TDst >::type _TDst;
		typedef typename If< IsPtrConst< TSrc >::value,
			typename RemoveConst< TSrc >::type,
			TSrc >::type _TSrc;
		typedef typename RemoveQualifies< _TDst >::type __TDstType;
		typedef typename RemoveQualifies< _TSrc >::type __TSrcType;
		const static bool is_const = IsConst< typename RemovePR< _TSrc >::type >::value;
		const static bool is_ptr = IsPtr< _TSrc >::value;
		typedef typename If< is_const, const __TDstType, __TDstType >::type _TDstType;
		typedef typename If< is_const, const __TSrcType, __TSrcType >::type _TSrcType;
		typedef _TDstType* TDstPtrType;
		typedef _TSrcType* TSrcPtrType;
		typedef _TDstType& TDstRefType;
		typedef _TSrcType& TSrcRefType;
		typedef typename If< is_ptr, TDstPtrType, TDstRefType >::type TDstType;
		typedef typename If< is_ptr, TSrcPtrType, TSrcRefType >::type TSrcType;
		DynamicCastHelperBase() {
			RFLX_TASSERT( ( IsKindOf< _TDst, Rflxable >::value &&
				IsKindOf< _TSrc, RflxObject >::value ) );
			RFLX_TASSERT( ( IsRef< __TDstType >::value == false ) );
			RFLX_TASSERT( ( IsPtr< __TDstType >::value == false ) );
			RFLX_TASSERT( ( IsRef< __TSrcType >::value == false ) );
			RFLX_TASSERT( ( IsPtr< __TSrcType >::value == false ) );
		}
	};

	RFLX_API const void*		dynamicCast( Klass src, const void* object, Klass dst );
	RFLX_API void*				dynamicCast( Klass src, void* object, Klass dst );

	template< class TDst, class TSrc, bool Pointer = IsPtr< TSrc >::value >
	struct DynamicCastHelper;

	template< class TDst, class TSrc >
	struct DynamicCastHelper< TDst, TSrc, true > : DynamicCastHelperBase< TDst, TSrc > {
		typedef DynamicCastHelperBase< TDst, TSrc > super;
		// if there is a const pointer, ignore it
		typedef typename If< IsPtrConst< TDst >::value,
			typename RemoveConst< TDst >::type,
			TDst >::type _TDst;
		typedef typename If< IsPtrConst< TSrc >::value,
			typename RemoveConst< TSrc >::type,
			TSrc >::type _TSrc;
		static inline typename super::TDstType invoke( typename super::TSrcPtrType src ) {
			size_t offset = 0;
			RFLX_TASSERT( ( IsPtr< _TDst >::value && IsPtr< _TDst >::value == IsPtr< _TSrc >::value ) );
			// check const
			RFLX_TASSERT( ( IsConst< typename RemovePR< _TSrc >::type >::value == IsConst< typename RemovePR< _TDst >::type >::value ) );
			return reinterpret_cast< typename super::TDstPtrType >( dynamicCast( src->_dynamicClass(), src->_mbase(), typename super::__TDstType::_class() ) );
		}
	};

	template< class TDst, class TSrc >
	struct DynamicCastHelper< TDst, TSrc, false > : DynamicCastHelperBase< TDst, TSrc > {
		typedef DynamicCastHelperBase< TDst, TSrc > super;
		static inline typename super::TDstType invoke( typename super::TSrcRefType src ) {
			size_t offset = 0;
			// must of them are ref type
			RFLX_TASSERT( ( IsRef< TDst >::value && IsRef< TDst >::value == IsRef< TSrc >::value ) );
			// must all of them are constant or inconstant
			RFLX_TASSERT( ( IsConst< typename RemovePR< TSrc >::type >::value == IsConst< typename RemovePR< TDst >::type >::value ) );
			void* rval = dynamicCast( src._dynamicClass(), src._mbase(), typename super::__TDstType::_class() );
			if ( rval ) {
				return *reinterpret_cast< typename super::TDstPtrType >( rval );
			} else {
				RFLX_ERROR( "Rflx fatal error: dynamic cast failed!\n" );
				return *reinterpret_cast< typename super::TDstPtrType >( NULL );
			}
		}
	};

#define RFLX_IMP_COMMON( CLASS ) \
	typedef CLASS this_type;\
	static const rflx::Klass& _class(){ static rflx::Klass _staticClass = NULL; return _staticClass; }\
	static const char* _name() { return rflx::RflxObjectName< CLASS >::name(); }\
	static rflx::ErrorCode _unregisterClass() { return rflx::unregisterClass( CLASS::_name() ); }\
	
#define RFLX_IMP_FACTORY_FUNC( CLASS )\
	static rflx::ErrorCode _createInstance( rflx::RflxObject** object )\
	{\
		RFLX_DASSERT( object );\
		CLASS* p = NULL;\
		try {\
			p = new CLASS();\
		} catch( ... ){ RFLX_WARNING( "Rflx::_createInstance failed!\n" ); p = NULL; }\
		if ( p ) {\
			*object = p;\
			return rflx::err_ok;\
		} else return rflx::err_failed;\
	}\
	static rflx::ErrorCode _destroyInstance( rflx::RflxObject* object )\
	{\
		if ( !object ) {\
			return rflx::err_ok;\
		}\
		delete (CLASS*)object;\
		return rflx::err_ok;\
	}
	
#define RFLX_IMP_NULL_FACTORY_FUNC \
	static rflx::ErrorCode _createInstance( rflx::RflxObject** ){ return rflx::err_not_implemented; }\
	static rflx::ErrorCode _destroyInstance( rflx::RflxObject* ){ return rflx::err_not_implemented; }

#define RFLX_IMP_AUTO_REGISTER( CLASS ) \
	namespace AUTO_REGISTER_##CLASS { const static rflx::ErrorCode __auto_register_result = rflx::RflxObjectName< CLASS >::name( #CLASS ) ? CLASS::_registerClass() : rflx::err_ok; }

#define RFLX_IMP_AUTO_REGISTER_WITH_NAME( CLASS, NAME ) \
	namespace AUTO_REGISTER_##CLASS { const static rflx::ErrorCode __auto_register_result = rflx::RflxObjectName< CLASS >::name( NAME ) ? CLASS::_registerClass() : rflx::err_ok; }

#define RFLX_IMP_AUTO_REGISTER_WITH_NAMEID( CLASS ) \
	namespace AUTO_REGISTER_##CLASS { const static rflx::ErrorCode __auto_register_result = rflx::RflxObjectName< CLASS >::name( typeid( NAME ).name() ) ? CLASS::_registerClass() : rflx::err_ok; }

// a member object use to do the rtti initializing if current class is not derived from rflx::RttiBind
#define RFLX_IMP_DYAMIC_INITIALIZER RFLX_IMP_INNER_DYAMIC_INITIALIZER
#define RFLX_IMP_INNER_DYAMIC_INITIALIZER( CLASS, BASECLASS ) \
	rflx::RttiInitializerHelper< CLASS, BASECLASS, false == rflx::IsChildOf< CLASS, rflx::RttiBind< CLASS, BASECLASS > >::value > __RttiInitializer__;

#define RFLX_OBJECT_DINIT {\
		bool enable = !rflx::IsChildOf< this_type, rflx::RttiBind< this_type, base_type > >::value;\
		if ( enable ) {\
			rflx::_internal::_initObjectRtti( this, this_type::_class() );\
			rflx::_internal::_addClassInstanceCount( this_type::_class() );\
		}\
	}

#define RFLX_OBJECT_DUNINIT {\
		bool enable = !rflx::IsChildOf< this_type, rflx::RttiBind< this_type, base_type > >::value;\
		if ( enable ) {\
			rflx::_internal::_releaseClassInstanceCount( this_type::_class() );\
		}\
	}

#if defined _MSC_VER && !defined NDEBUG
	#define CHECK_CLASS_DECLARATION \
		private: static void _checkClassDeclaration() {\
			if ( __super::_name != base_type::_name ) {\
				printf( "Invalid class declaration!\n" ); system("pause");\
			}\
		}
#define DO_CHECK_CLASS_DECLARATION { this_type::_checkClassDeclaration(); }
#else
	#define CHECK_CLASS_DECLARATION
	#define DO_CHECK_CLASS_DECLARATION
#endif

#define RFLX_IMP_REGISTER_CLASS( CLASS ) \
	CHECK_CLASS_DECLARATION\
	public:\
	static rflx::ErrorCode _registerClass()\
	{\
		DO_CHECK_CLASS_DECLARATION\
		RFLX_TASSERT( ( ( rflx::IsKindOf< this_type, base_type >::value ||\
						  rflx::IsKindOf< this_type, rflx::RttiBind< this_type, base_type > >::value ) &&\
						  rflx::TypeEqual< this_type, base_type >::value == false ) );\
		rflx::RflxObjectName< CLASS >::name( #CLASS );\
		rflx::Klass& klass = const_cast< rflx::Klass& >( CLASS::_class() );\
		rflx::ClassInfo cls;\
		memset( &cls, 0, sizeof( cls ) );\
		cls.className = _name();\
		cls.baseClassName = base_type::_name();\
		rflx::setClassFlag( cls.flags, rflx::IsPolymorphic< this_type >::value ? rflx::cif_polymorphic : rflx::cif_none );\
		cls.methods.createInstance = _createInstance == base_type::_createInstance ? NULL : _createInstance;\
		cls.methods.destroyInstance = _destroyInstance;\
		cls.methods.objectProc = _objectProc == base_type::_objectProc ? NULL : _objectProc;\
		cls.methods.initializeHook = _initializeHook == base_type::_initializeHook ? NULL : _initializeHook;\
		cls.methods.unInitializeHook = _unInitializeHook == base_type::_unInitializeHook ? NULL : _unInitializeHook;\
		cls.propCount = 0;\
		cls.baseOffset = rflx::BaseOffsetGetter< this_type, base_type >::invoke();\
		cls.propDefs = _getPropDefs == base_type::_getPropDefs ? NULL : _getPropDefs( &cls.propCount );\
		cls.msgMap = _getMsgMap == base_type::_getMsgMap ? NULL : _getMsgMap();\
		cls.customDataHandler = NULL;\
		return rflx::registerClass( &cls, &klass );\
	}

//----------------------------------------------------------------------------------------------------------------
// simple class declaration
//----------------------------------------------------------------------------------------------------------------
#define RFLX_IMP_SIMPLE_BASE_CLASS( CLASS ) \
	protected: RFLX_IMP_NULL_FACTORY_FUNC; public:\
	RFLX_IMP_COMMON( CLASS )\
	typedef rflx::Rflxable base_type;\
	RFLX_IMP_REGISTER_CLASS( CLASS )

#define RFLX_IMP_SIMPLE_CLASS( CLASS, BASECLASS ) \
	protected: RFLX_IMP_NULL_FACTORY_FUNC; public:\
	RFLX_IMP_COMMON( CLASS )\
	typedef BASECLASS base_type;\
	RFLX_IMP_REGISTER_CLASS( CLASS )

#define RFLX_IMP_ABSTRACT_SIMPLE_BASE_CLASS( CLASS ) \
	protected: RFLX_IMP_NULL_FACTORY_FUNC; public:\
	RFLX_IMP_COMMON( CLASS )\
	typedef rflx::Rflxable base_type;\
	RFLX_IMP_REGISTER_CLASS( CLASS )

#define RFLX_IMP_ABSTRACT_SIMPLE_CLASS( CLASS, BASECLASS ) \
	protected: RFLX_IMP_NULL_FACTORY_FUNC; public:\
	RFLX_IMP_COMMON( CLASS )\
	typedef BASECLASS base_type;\
	RFLX_IMP_REGISTER_CLASS( CLASS )

//----------------------------------------------------------------------------------------------------------------
// general class declaration
//----------------------------------------------------------------------------------------------------------------
#define RFLX_IMP_ABSTRACT_BASE_CLASS( CLASS ) \
	RFLX_IMP_COMMON( CLASS )\
	typedef rflx::RflxObject base_type;\
	RFLX_IMP_REGISTER_CLASS( CLASS )\
	RFLX_IMP_INNER_DYAMIC_INITIALIZER( CLASS, base_type )

#define RFLX_IMP_BASE_CLASS( CLASS ) \
	RFLX_IMP_COMMON( CLASS )\
	typedef rflx::RflxObject base_type;\
	RFLX_IMP_REGISTER_CLASS( CLASS )\
	RFLX_IMP_FACTORY_FUNC( CLASS )\
	RFLX_IMP_INNER_DYAMIC_INITIALIZER( CLASS, base_type )

#define RFLX_IMP_CLASS( CLASS, BASECLASS ) \
	RFLX_IMP_COMMON( CLASS )\
	typedef BASECLASS base_type;\
	RFLX_IMP_REGISTER_CLASS( CLASS )\
	RFLX_IMP_FACTORY_FUNC( CLASS )\
	RFLX_IMP_INNER_DYAMIC_INITIALIZER( CLASS, base_type )

#define RFLX_IMP_ABSTRACT_CLASS( CLASS, BASECLASS ) \
	RFLX_IMP_COMMON( CLASS )\
	typedef BASECLASS base_type;\
	RFLX_IMP_REGISTER_CLASS( CLASS )\
	RFLX_IMP_INNER_DYAMIC_INITIALIZER( CLASS, base_type )
//----------------------------------------------------------------------------------------------------------------
// general class declaration
//----------------------------------------------------------------------------------------------------------------
#define RFLX_IMP_MANUAL_ABSTRACT_BASE_CLASS( CLASS ) \
	RFLX_IMP_COMMON( CLASS )\
	typedef rflx::RflxObject base_type;\
	RFLX_IMP_REGISTER_CLASS( CLASS )

#define RFLX_IMP_MANUAL_BASE_CLASS( CLASS ) \
	RFLX_IMP_COMMON( CLASS )\
	typedef rflx::RflxObject base_type;\
	RFLX_IMP_REGISTER_CLASS( CLASS )\
	RFLX_IMP_FACTORY_FUNC( CLASS )

#define RFLX_IMP_MANUAL_CLASS( CLASS, BASECLASS ) \
	RFLX_IMP_COMMON( CLASS )\
	typedef BASECLASS base_type;\
	RFLX_IMP_REGISTER_CLASS( CLASS )\
	RFLX_IMP_FACTORY_FUNC( CLASS )

#define RFLX_IMP_MANUAL_ABSTRACT_CLASS( CLASS, BASECLASS ) \
	RFLX_IMP_COMMON( CLASS )\
	typedef BASECLASS base_type;\
	RFLX_IMP_REGISTER_CLASS( CLASS )
//----------------------------------------------------------------------------------------------------------------
// property reflection macros
//----------------------------------------------------------------------------------------------------------------	
#define RFLX_BEGIN_PROPERTY_MAP\
	static const rflx::PropDef* _getPropDefs( unsigned int* count )\
    { \
        typedef this_type _currClass; \
		unsigned int _start = 0;\
		static rflx::PropDef _defs[] = {
		
#define RFLX_END_PROPERTY_MAP \
		}; *count = sizeof(_defs) / sizeof(_defs[0]); \
        return _defs;\
    }

#define RFLX_IMP_EMPTY_PROPERTY_MAP \
	static const rflx::PropDef *_getPropDefs( unsigned int* count ){\
        *count = 0;\
        return NULL;\
    }

#define RFLX_IMP_PROPERTY_DEF( FLAGS, PROP, PROPNAME, DESC, GROUPNAME, USERTYPE, DEFAULT, GET, GETREF, SET, GETDEF, OPERATE, EDITORDATA ) \
	rflx::_addPropDef( ( unsigned int )FLAGS, &_currClass::PROP, RFLX_OFFSETOF( _currClass, PROP ),  _start++, #PROPNAME,\
						DESC, GROUPNAME, USERTYPE, rflx::makeDefaultValue( DEFAULT ), GET,\
						GETREF, SET, GETDEF, OPERATE, EDITORDATA ),
																																				//								FLAGS, PROP, PROPNAME, DESC,		GROUPNAME, USERTYPE, DEFAULT,		   GET,  GETREF, SET,		GETDEF, OPERATE, EDITORDATA
#define RFLX_IMP_PROPERTY( PROP )																												RFLX_IMP_PROPERTY_DEF( rflx::pdf_none, PROP, PROP,	   NULL,		NULL,	   NULL,	 rflx::NullType(), NULL, NULL,   NULL,		NULL,   NULL,	NULL )
#define RFLX_IMP_PROPERTY_FLAGS( PROP, FLAGS )																									RFLX_IMP_PROPERTY_DEF( FLAGS,		   PROP, PROP,	   NULL,		NULL,	   NULL,	 rflx::NullType(), NULL, NULL,   NULL,		NULL,   NULL,    NULL )
#define RFLX_IMP_PROPERTY_DEFAULT( PROP, DEFAULT )																								RFLX_IMP_PROPERTY_DEF( rflx::pdf_none, PROP, PROP,	   NULL,		NULL,	   NULL,	 DEFAULT,		   NULL, NULL,   NULL,		NULL,   NULL,	NULL )
#define RFLX_IMP_PROPERTY_DEFAULT_FLAGS( PROP, DEFAULT, FLAGS )																					RFLX_IMP_PROPERTY_DEF( FLAGS,		   PROP, PROP,	   NULL,		NULL,	   NULL,	 DEFAULT,		   NULL, NULL,   NULL,		NULL,   NULL,    NULL )
#define RFLX_IMP_PROPERTY_NAME( PROP, PROPNAME )																								RFLX_IMP_PROPERTY_DEF( rflx::pdf_none, PROP, PROPNAME, NULL,		NULL,	   NULL,	 rflx::NullType(), NULL, NULL,   NULL,		NULL,   NULL,    NULL )
#define RFLX_IMP_PROPERTY_NAME_FLAGS( PROP, PROPNAME, FLAGS )																					RFLX_IMP_PROPERTY_DEF( FLAGS,		   PROP, PROPNAME, NULL,		NULL,	   NULL,	 rflx::NullType(), NULL, NULL,   NULL,		NULL,   NULL,    NULL )
#define RFLX_IMP_PROPERTY_NAME_GROUPNAME( PROP, PROPNAME, GROUPNAME )																			RFLX_IMP_PROPERTY_DEF( rflx::pdf_none, PROP, PROPNAME, NULL,		GROUPNAME, NULL,	 rflx::NullType(), NULL, NULL,   NULL,		NULL,   NULL,    NULL )
#define RFLX_IMP_PROPERTY_NAME_GROUPNAME_FLAGS( PROP, PROPNAME, GROUPNAME, FLAGS )																RFLX_IMP_PROPERTY_DEF( FLAGS,		   PROP, PROPNAME, NULL,		GROUPNAME, NULL,	 rflx::NullType(), NULL, NULL,   NULL,		NULL,   NULL,    NULL )
#define RFLX_IMP_PROPERTY_NAME_GROUPNAME_DESC( PROP, PROPNAME, GROUPNAME, DESCRIPTION )															RFLX_IMP_PROPERTY_DEF( rflx::pdf_none, PROP, PROPNAME, DESCRIPTION, GROUPNAME, NULL,	 rflx::NullType(), NULL, NULL,   NULL,		NULL,   NULL,    NULL )
#define RFLX_IMP_PROPERTY_NAME_GROUPNAME_DESC_FLAGS( PROP, PROPNAME, GROUPNAME, DESCRIPTION, FLAGS )											RFLX_IMP_PROPERTY_DEF( FLAGS,		   PROP, PROPNAME, DESCRIPTION, GROUPNAME, NULL,	 rflx::NullType(), NULL, NULL,   NULL,		NULL,   NULL,    NULL )
#define RFLX_IMP_PROPERTY_NAME_GROUPNAME_DESC_USERTYPE( PROP, PROPNAME, GROUPNAME, DESCRIPTION, USERTYPE )										RFLX_IMP_PROPERTY_DEF( rflx::pdf_none, PROP, PROPNAME, DESCRIPTION, GROUPNAME, USERTYPE, rflx::NullType(), NULL, NULL,   NULL,		NULL,   NULL,    NULL )
#define RFLX_IMP_PROPERTY_NAME_GROUPNAME_DESC_USERTYPE_FLAGS( PROP, PROPNAME, GROUPNAME, DESCRIPTION, USERTYPE, FLAGS )							RFLX_IMP_PROPERTY_DEF( FLAGS,		   PROP, PROPNAME, DESCRIPTION, GROUPNAME, USERTYPE, rflx::NullType(), NULL, NULL,   NULL,		NULL,   NULL,    NULL )
#define RFLX_IMP_PROPERTY_NAME_DEFAULT_GROUPNAME( PROP, PROPNAME, DEFAULT, GROUPNAME )															RFLX_IMP_PROPERTY_DEF( rflx::pdf_none, PROP, PROPNAME, NULL,		GROUPNAME, NULL,	 DEFAULT,		   NULL, NULL,   NULL,		NULL,   NULL,    NULL )
#define RFLX_IMP_PROPERTY_NAME_DEFAULT_GROUPNAME_FLAGS( PROP, PROPNAME, DEFAULT, GROUPNAME, FLAGS )												RFLX_IMP_PROPERTY_DEF( FLAGS,		   PROP, PROPNAME, NULL,		GROUPNAME, NULL,	 DEFAULT,		   NULL, NULL,   NULL,		NULL,   NULL,    NULL )
#define RFLX_IMP_PROPERTY_NAME_DEFAULT_GROUPNAME_DESC( PROP, PROPNAME, DEFAULT, GROUPNAME, DESCRIPTION )										RFLX_IMP_PROPERTY_DEF( rflx::pdf_none, PROP, PROPNAME, DESCRIPTION, GROUPNAME, NULL,	 DEFAULT,		   NULL, NULL,   NULL,		NULL,   NULL,    NULL )
#define RFLX_IMP_PROPERTY_NAME_DEFAULT_GROUPNAME_DESC_FLAGS( PROP, PROPNAME, DEFAULT, GROUPNAME, DESCRIPTION, FLAGS )							RFLX_IMP_PROPERTY_DEF( FLAGS,		   PROP, PROPNAME, DESCRIPTION, GROUPNAME, NULL,	 DEFAULT,		   NULL, NULL,   NULL,		NULL,   NULL,    NULL )
#define RFLX_IMP_PROPERTY_NAME_DEFAULT_GROUPNAME_DESC_USERTYPE( PROP, PROPNAME, DEFAULT, GROUPNAME, DESCRIPTION, USERTYPE )						RFLX_IMP_PROPERTY_DEF( rflx::pdf_none, PROP, PROPNAME, DESCRIPTION, GROUPNAME, USERTYPE, DEFAULT,		   NULL, NULL,   NULL,		NULL,   NULL,    NULL )
#define RFLX_IMP_PROPERTY_NAME_DEFAULT_GROUPNAME_DESC_USERTYPE_FLAGS( PROP, PROPNAME, DEFAULT, GROUPNAME, DESCRIPTION, USERTYPE, FLAGS )		RFLX_IMP_PROPERTY_DEF( FLAGS,		   PROP, PROPNAME, DESCRIPTION, GROUPNAME, USERTYPE, DEFAULT,		   NULL, NULL,   NULL,		NULL,   NULL,    NULL )
#define RFLX_IMP_PROPERTY_NAME_DESC( PROP, PROPNAME, DESCRIPTION )																				RFLX_IMP_PROPERTY_DEF( rflx::pdf_none, PROP, PROPNAME, DESCRIPTION, NULL,	   NULL,	 rflx::NullType(), NULL, NULL,   NULL,		NULL,   NULL,    NULL )
#define RFLX_IMP_PROPERTY_NAME_DESC_FLAGS( PROP, PROPNAME, DESCRIPTION, FLAGS )																	RFLX_IMP_PROPERTY_DEF( FLAGS,		   PROP, PROPNAME, DESCRIPTION, NULL,	   NULL,	 rflx::NullType(), NULL, NULL,   NULL,		NULL,   NULL,    NULL )
#define RFLX_IMP_PROPERTY_NAME_DESC_USERTYPE( PROP, PROPNAME, DESCRIPTION, USERTYPE )															RFLX_IMP_PROPERTY_DEF( rflx::pdf_none, PROP, PROPNAME, DESCRIPTION, NULL,	   USERTYPE, rflx::NullType(), NULL, NULL,   NULL,		NULL,   NULL,    NULL )
#define RFLX_IMP_PROPERTY_NAME_DESC_USERTYPE_FLAGS( PROP, PROPNAME, DESCRIPTION, USERTYPE, FLAGS )												RFLX_IMP_PROPERTY_DEF( FLAGS,		   PROP, PROPNAME, DESCRIPTION, NULL,	   USERTYPE, rflx::NullType(), NULL, NULL,   NULL,		NULL,   NULL,    NULL )
#define RFLX_IMP_PROPERTY_NAME_DEFAULT( PROP, PROPNAME, DEFAULT )																				RFLX_IMP_PROPERTY_DEF( rflx::pdf_none, PROP, PROPNAME, NULL,		NULL,	   NULL,	 DEFAULT,		   NULL, NULL,   NULL,		NULL,   NULL,    NULL )
#define RFLX_IMP_PROPERTY_NAME_DEFAULT_FLAGS( PROP, PROPNAME, DEFAULT, FLAGS )																	RFLX_IMP_PROPERTY_DEF( FLAGS,		   PROP, PROPNAME, NULL,		NULL,	   NULL,	 DEFAULT,		   NULL, NULL,   NULL,		NULL,   NULL,    NULL )
#define RFLX_IMP_PROPERTY_NAME_DEFAULT_DESC( PROP, PROPNAME, DEFAULT, DESCRIPTION )																RFLX_IMP_PROPERTY_DEF( rflx::pdf_none, PROP, PROPNAME, DESCRIPTION, NULL,	   NULL,	 DEFAULT,		   NULL, NULL,   NULL,		NULL,   NULL,    NULL )
#define RFLX_IMP_PROPERTY_NAME_DEFAULT_DESC_FLAGS( PROP, PROPNAME, DEFAULT, DESCRIPTION, FLAGS )												RFLX_IMP_PROPERTY_DEF( FLAGS,		   PROP, PROPNAME, DESCRIPTION, NULL,	   NULL,	 DEFAULT,		   NULL, NULL,   NULL,		NULL,   NULL,    NULL )
#define RFLX_IMP_PROPERTY_NAME_DEFAULT_DESC_USERTYPE( PROP, PROPNAME, DEFAULT, DESCRIPTION, USERTYPE )											RFLX_IMP_PROPERTY_DEF( rflx::pdf_none, PROP, PROPNAME, DESCRIPTION, NULL,	   USERTYPE, DEFAULT,		   NULL, NULL,   NULL,		NULL,   NULL,    NULL )
#define RFLX_IMP_PROPERTY_NAME_DEFAULT_DESC_USERTYPE_FLAGS( PROP, PROPNAME, DEFAULT, DESCRIPTION, USERTYPE, FLAGS )								RFLX_IMP_PROPERTY_DEF( FLAGS,		   PROP, PROPNAME, DESCRIPTION, NULL,	   USERTYPE, DEFAULT,		   NULL, NULL,   NULL,		NULL,   NULL,    NULL )
#define RFLX_IMP_PROPERTY_NAME_SETTER( PROP, PROPNAME, SETTER )																					RFLX_IMP_PROPERTY_DEF( rflx::pdf_none, PROP, PROPNAME, NULL,		NULL,	   NULL,	 rflx::NullType(), NULL, NULL,   SETTER,	NULL,   NULL,    NULL )
#define RFLX_IMP_PROPERTY_NAME_DEFAULT_SETTER( PROP, PROPNAME, DEFAULT, SETTER )																RFLX_IMP_PROPERTY_DEF( rflx::pdf_none, PROP, PROPNAME, NULL,		NULL,	   NULL,	 DEFAULT,		   NULL, NULL,   SETTER,	NULL,   NULL,    NULL )
//----------------------------------------------------------------------------------------------------------------	
	namespace _internal
	{
		template< typename To, typename From >
		To _horrbleCast( From func ) {
			RFLX_TASSERT(
				( TypeEqual< To, MsgFunc::MFP >::value && sizeof( From ) <= sizeof( To ) ) ||
				( TypeEqual< From, MsgFunc::MFP >::value && sizeof( From ) >= sizeof( To ) ) );
			To _to;
			memcpy( &_to, &func, sizeof( From ) < sizeof( To ) ? sizeof( From ) : sizeof( To ) );
			return _to;
		}
		
		template< typename T > struct MessageHandleProp;
		template< typename TOwner > struct MessageHandleProp< void ( TOwner::* )( rflx::Message&, bool& ) >;
		template< typename TOwner > struct MessageHandleProp< rflx::ErrorCode ( TOwner::* )( rflx::Message&, bool& ) >;
		template< typename TOwner > struct MessageHandleProp< void ( TOwner::* )( const rflx::Message&, bool& ) >;
		template< typename TOwner > struct MessageHandleProp< rflx::ErrorCode ( TOwner::* )( const rflx::Message&, bool& ) >;
		template< typename TOwner > struct MessageHandleProp< void ( TOwner::* )( rflx::Message&, bool& ) const >;
		template< typename TOwner > struct MessageHandleProp< rflx::ErrorCode ( TOwner::* )( rflx::Message&, bool& ) const >;
		template< typename TOwner > struct MessageHandleProp< void ( TOwner::* )( const rflx::Message&, bool& ) const >;
		template< typename TOwner > struct MessageHandleProp< rflx::ErrorCode ( TOwner::* )( const rflx::Message&, bool& ) const >;

		template< typename TOwner >
		struct MessageHandleProp< void ( TOwner::* )( rflx::Message& ) > {
			const static unsigned int flag = 0;
			const static size_t payloadSize = -1;
			const static bool isConst = false;
			typedef RFLX_TYPE_LIST_2( void, rflx::Message& ) typelist;
		};

		template< typename TOwner >
		struct MessageHandleProp< void ( TOwner::* )( rflx::Message& ) const > :
			MessageHandleProp< void ( TOwner::* )( rflx::Message& ) >
		{
			const static bool isConst = true;
		};

		template< typename TOwner, typename TPayload >
		struct MessageHandleProp< void ( TOwner::* )( TPayload& ) > {
			const static unsigned int flag = 1;
			const static size_t payloadSize = sizeof( TPayload );
			const static bool isConst = false;
			typedef RFLX_TYPE_LIST_2( void, TPayload& ) typelist;
		};

		template< typename TOwner, typename TPayload >
		struct MessageHandleProp< void ( TOwner::* )( const TPayload& ) > : 
			MessageHandleProp< void ( TOwner::* )( TPayload& ) > {};

		template< typename TOwner, typename TPayload >
		struct MessageHandleProp< void ( TOwner::* )( TPayload& ) const > : 
			MessageHandleProp< void ( TOwner::* )( TPayload& ) > 
		{
			const static bool isConst = true;
		};

		template< typename TOwner, typename TPayload >
		struct MessageHandleProp< void ( TOwner::* )( const TPayload& ) const > : 
			MessageHandleProp< void ( TOwner::* )( TPayload& ) const > {};

		template< typename TOwner, typename TPayload >
		struct MessageHandleProp< void ( TOwner::* )( TPayload&, bool& ) > {
			const static unsigned int flag = 2;
			const static size_t payloadSize = sizeof( TPayload );
			const static bool isConst = false;
			typedef RFLX_TYPE_LIST_3( void, TPayload&, bool& ) typelist;
		};

		template< typename TOwner, typename TPayload >
		struct MessageHandleProp< void ( TOwner::* )( const TPayload&, bool& ) > :
			MessageHandleProp< void ( TOwner::* )( TPayload&, bool& ) > {};

		template< typename TOwner, typename TPayload >
		struct MessageHandleProp< void ( TOwner::* )( TPayload&, bool& ) const > :
			MessageHandleProp< void ( TOwner::* )( TPayload&, bool& ) >
		{
			const static bool isConst = true;
		};

		template< typename TOwner, typename TPayload >
		struct MessageHandleProp< void ( TOwner::* )( const TPayload&, bool& ) const > :
			MessageHandleProp< void ( TOwner::* )( TPayload&, bool& ) const > {};

		template< typename TOwner >
		struct MessageHandleProp< rflx::ErrorCode ( TOwner::* )( rflx::Message& ) > {
			const static unsigned int flag = 3;
			const static size_t payloadSize = ( size_t )~0;
			const static bool isConst = false;
			typedef RFLX_TYPE_LIST_2( rflx::ErrorCode, rflx::Message& ) typelist;
		};

		template< typename TOwner >
		struct MessageHandleProp< rflx::ErrorCode ( TOwner::* )( rflx::Message& ) const > :
			 MessageHandleProp< rflx::ErrorCode ( TOwner::* )( rflx::Message& ) > 
		{
			 const static bool isConst = true;
		};

		template< typename TOwner, typename TPayload >
		struct MessageHandleProp< rflx::ErrorCode ( TOwner::* )( TPayload& ) > {
			const static unsigned int flag = 4;
			const static size_t payloadSize = sizeof( TPayload );
			const static bool isConst = false;
			typedef RFLX_TYPE_LIST_2( rflx::ErrorCode, TPayload& ) typelist;
		};

		template< typename TOwner, typename TPayload >
		struct MessageHandleProp< rflx::ErrorCode ( TOwner::* )( const TPayload& ) > :
			MessageHandleProp< rflx::ErrorCode ( TOwner::* )( TPayload& ) > {};

		template< typename TOwner, typename TPayload >
		struct MessageHandleProp< rflx::ErrorCode ( TOwner::* )( TPayload& ) const > :
			MessageHandleProp< rflx::ErrorCode ( TOwner::* )( TPayload& ) >
		{
			const static bool isConst = true;
		};

		template< typename TOwner, typename TPayload >
		struct MessageHandleProp< rflx::ErrorCode ( TOwner::* )( const TPayload& ) const > :
			MessageHandleProp< rflx::ErrorCode ( TOwner::* )( TPayload& ) const > {};

		template< typename TOwner, typename TPayload >
		struct MessageHandleProp< rflx::ErrorCode ( TOwner::* )( TPayload&, bool& ) > {
			const static unsigned int flag = 5;
			const static size_t payloadSize = sizeof( TPayload );
			const static bool isConst = false;
			typedef RFLX_TYPE_LIST_3( rflx::ErrorCode, TPayload&, bool& ) typelist;
		};

		template< typename TOwner, typename TPayload >
		struct MessageHandleProp< rflx::ErrorCode ( TOwner::* )( const TPayload&, bool& ) > :
			MessageHandleProp< rflx::ErrorCode ( TOwner::* )( TPayload&, bool& ) > {};

		template< typename TOwner, typename TPayload >
		struct MessageHandleProp< rflx::ErrorCode ( TOwner::* )( TPayload&, bool& ) const > :
			MessageHandleProp< rflx::ErrorCode ( TOwner::* )( TPayload&, bool& ) > {
			const static bool isConst = true;
		};

		template< typename TOwner, typename TPayload >
		struct MessageHandleProp< rflx::ErrorCode ( TOwner::* )( const TPayload&, bool& ) const > :
			MessageHandleProp< rflx::ErrorCode ( TOwner::* )( const TPayload&, bool& ) const > {};

		template< typename TOwner, int sel >
		struct MessageHandleSelector {
			typedef RFLX_TYPE_LIST_6( 
				void ( TOwner::* )( void* ), // rflx::Message&
				void ( TOwner::* )( void* ), // TPayload&
				void ( TOwner::* )( void*, bool& ), // TPayload&, bool&
				rflx::ErrorCode ( TOwner::* )( void* ),// rflx::Message&
				rflx::ErrorCode ( TOwner::* )( void* ),// rflx::TPayload&
				rflx::ErrorCode ( TOwner::* )( void*, bool& )// rflx::Message&
			) MessageHandleTypes_t;
			typedef typename TypeList_GetAt< MessageHandleTypes_t, sel >::type type;
		};

		template< typename TFunc >
		unsigned int getMessageHandlePropFlag( TFunc f ) {
			return MessageHandleProp< TFunc >::flag;
		}

		template< typename TFunc >
		size_t getMessageHandleDataSize( TFunc f ) {
			return MessageHandleProp< TFunc >::payloadSize;
		}
	}

#define RFLX_BEGIN_MSG_MAP\
	static rflx::ErrorCode _objectProc( rflx::Message* message, void* thisObject ) {\
		this_type* _this = ( this_type* )thisObject;\
		const rflx::MsgFunc* funcInfo = rflx::findMessageHandler( this_type::_class(), ( rflx::MessageId )message->msgId );\
		if ( funcInfo ) {\
			switch ( funcInfo->propFlags ) {\
				case 0: {\
						typedef rflx::_internal::MessageHandleSelector< this_type, 0 >::type MessageHandle_t;\
						MessageHandle_t f = rflx::_internal::_horrbleCast< MessageHandle_t >( funcInfo->func );\
						( _this->*f )( message );\
					}\
					break;\
				case 1: {\
						typedef rflx::_internal::MessageHandleSelector< this_type, 1 >::type MessageHandle_t;\
						MessageHandle_t f = rflx::_internal::_horrbleCast< MessageHandle_t >( funcInfo->func );\
						RFLX_DASSERT( funcInfo->payloadSize == message->cbData );\
						( _this->*f )( message->data );\
					}\
					break;\
				case 2: {\
						typedef rflx::_internal::MessageHandleSelector< this_type, 2 >::type MessageHandle_t;\
						MessageHandle_t f = rflx::_internal::_horrbleCast< MessageHandle_t >( funcInfo->func );\
						RFLX_DASSERT( funcInfo->payloadSize == message->cbData );\
						( _this->*f )( message->data, message->handled );\
					}\
					break;\
				case 3: {\
						typedef rflx::_internal::MessageHandleSelector< this_type, 3 >::type MessageHandle_t;\
						MessageHandle_t f = rflx::_internal::_horrbleCast< MessageHandle_t >( funcInfo->func );\
						return ( _this->*f )( message );\
					}\
					break;\
				case 4: {\
						typedef rflx::_internal::MessageHandleSelector< this_type, 4 >::type MessageHandle_t;\
						MessageHandle_t f = rflx::_internal::_horrbleCast< MessageHandle_t >( funcInfo->func );\
						RFLX_DASSERT( funcInfo->payloadSize == message->cbData );\
						return ( _this->*f )( message->data );\
					}\
					break;\
				case 5: {\
						typedef rflx::_internal::MessageHandleSelector< this_type, 5 >::type MessageHandle_t;\
						MessageHandle_t f = rflx::_internal::_horrbleCast< MessageHandle_t >( funcInfo->func );\
						RFLX_DASSERT( funcInfo->payloadSize == message->cbData );\
						return ( _this->*f )( message->data, message->handled );\
					}\
					break;\
				default:;\
			}\
		}\
		return rflx::err_ok;\
	}\
	static const rflx::MsgFunc* _getMsgMap()\
    { \
        typedef this_type _currClass; \
		unsigned int _start = 0;\
		static rflx::MsgFunc _msgMap[] = {
		
#define RFLX_END_MSG_MAP \
		{ rflx::xm_none } };\
        return _msgMap;\
    }

	// Messaging system supports these types of function prototype:
// Sig: void onMessageXX( rflx::Message& message )
// Sig: void onMessageXX( T& userData )
// Sig: void onMessageXX( T& userData, bool& handled )
// Sig: rflx::ErrorCode onMessageXX( rflx::Message& message )
// Sig: rflx::ErrorCode onMessageXX( T& userData )
// Sig: rflx::ErrorCode onMessageXX( T& userData, bool& handled )

#define RFLX_ON_MESSAGE( Msg, MemberFunc )\
	{ ( rflx::MessageId )Msg,\
		rflx::_internal::_horrbleCast< rflx::MsgFunc::MFP >( MemberFunc ),\
		rflx::_internal::getMessageHandlePropFlag( MemberFunc ),\
		rflx::_internal::getMessageHandleDataSize( MemberFunc ) },

#define RFLX_IMP_EMPTY_MSG_MAP \
	static const MsgFunc* _getMsgMap() { return NULL; }
   
#define RFLX_MSG_GUARD( ENABLE )		rflx::MessageEnableGuard __MessageEnableGuard_##__LINE__( ENABLE )
#define RFLX_MSG_ENABLE_GUARD()			rflx::MessageEnableGuard __MessageEnableGuard_##__LINE__( true )
#define RFLX_MSG_DISABLE_GUARD()		rflx::MessageEnableGuard __MessageEnableGuard_##__LINE__( false )
//----------------------------------------------------------------------------------------------------------------	
	// a dynamic value stack
	// normally it design for function parameters passing
	typedef ValueData Variant;
	namespace DValueStack
	{
		RFLX_API DVStack*			create( size_t maxSize = 0 );
		RFLX_API void				destroy( DVStack* stack );
		RFLX_API DVStack&			getGlobal();
		RFLX_API size_t				maxSize( const DVStack& stack );
		RFLX_API size_t				size( const DVStack& stack );
		RFLX_API ValueData&			getAt( DVStack& stack, int index );
		RFLX_API const ValueData&	getAt( const DVStack& stack, int index );
		RFLX_API void				pop( DVStack& stack );
		RFLX_API void				push( DVStack& stack, const ValueData& value );
		RFLX_API ValueData			popData( DVStack& stack );

		template< typename T >
		void pushValue( DVStack& stack, const T& value ) {
			ValueData dv;
			PackageObject< T >::invoke( dv, value );
			push( stack, dv );
		}

		template< typename T >
		T& popValue( DVStack& stack, T& value ) { 
			ExtractObject< T >::invoke( popData( stack ), value );
			return value;
		}

		template< typename T >
		T& getValue( const DVStack& stack, int index, T& value ) {
			const ValueData& dv = getAt( stack, index );
			ExtractObject< T >::invoke( getAt( stack, index ), value );
			return value;
		}
	}
	inline size_t			getValueDataTypeSize( ValueDataType type ) { return _internal::_getValueDataTypeSizeNameTable()[ type ].size; }
	inline const char*		getValueDataTypeName( ValueDataType type ) { return _internal::_getValueDataTypeSizeNameTable()[ type ].name; }
	inline const char*		getValueDataTypeName2( ValueDataType type ) { const _internal::DataTypeSizeName& item = _internal::_getValueDataTypeSizeNameTable()[ type ]; return item.name2 && item.name2[0] ? item.name2 : item.name; }
	inline unsigned int		makeClassFlag( ClassInheritType inheritType, ClassTypeFlag classType ){ return ( ( (unsigned int)inheritType & 0xff ) << 8 ) | ( (unsigned int)classType & 0xff );}
	inline void				setClassFlag( unsigned int& flag, ClassTypeFlag classType ) { flag |= ( (unsigned int)classType & 0xff ); }
	inline void				setInheritType( unsigned int& flag, ClassInheritType inheritType ) { flag &= 0xffff00ff; flag |= ( (unsigned int)inheritType & 0xff ) << 8; }
	inline ClassInheritType getInheritType( unsigned int flag ) { return (ClassInheritType)( ( flag & 0x0000ff00 ) >> 8 ); }
	inline bool				checkClassFlag( unsigned int flag, ClassTypeFlag classType ) { return !!( flag & classType ); }
	
	struct HookInfo {
		void ( *pfn_on_init )();
		void ( *pfn_on_uninit )();
		void ( *pfn_error )( const char* format, ... );
		void ( *pfn_warning )( const char* format, ... );
	};

	RFLX_API ErrorCode			initialize( const HookInfo* hook = NULL );
	RFLX_API ErrorCode			unInitialize();
	RFLX_API bool				hasInitialized();
	RFLX_API const HookInfo*	getCurrentHook();
	RFLX_API ErrorCode			registerClass( const ClassInfo* classInfo, Klass* klass = NULL );
	RFLX_API ErrorCode			unregisterClass( const char* name );
	// set user's hook if NULL passed in, it will be restored to the default setting.
	// the Hook you have passed in must be global or static
	// return the old hook if succeed
	// return NULL if failed
	RFLX_API const HookInfo*	setUserHook( const HookInfo* hook = NULL );

	class RFLX_API RflxDynamicObject : public RflxObject {
	public:
		RFLX_IMP_MANUAL_CLASS( RflxDynamicObject, RflxObject )
		RflxDynamicObject();
		RflxDynamicObject( const RflxDynamicObject& );
		virtual ~RflxDynamicObject();
	};

	// if you got a compiling error here, that means the template args are invalid:
	// TSrc must derives from Rflxable
	// TDst must derives from RflxObject
#define __RFLX_DC_RET_REF \
	typename If< IsPtr< TDst >::value,\
		typename RemovePtr< TDst >::type,\
	TDst >::type

#define __RFLX_DC_RET_PTR \
	typename If< IsPtr< TDst >::value,\
			TDst, TDst* >::type

	template< class TDst, class TSrc >
	static inline __RFLX_DC_RET_REF dynamicCast( TSrc& src ) {
		return DynamicCastHelper< TDst, TSrc& >::invoke( src );
	}
	template< class TDst, class TSrc >
	static inline __RFLX_DC_RET_PTR dynamicCast( TSrc* src ) {
		if ( !src ) return NULL;
		return DynamicCastHelper< TDst, TSrc* >::invoke( src );
	}

#undef __RFLX_DC_RET_REF
#undef __RFLX_DC_RET_PTR

	
	RFLX_API Ktex				getContext( Klass klass = NULL );
	RFLX_API Ktex				getCurrentContext();
	RFLX_API Klass				findClass( const char* name, Ktex* ktex = NULL );
	RFLX_API ErrorCode			unregisterRootClass( const char* name );// unregister all classes derived from a class you specified.
	RFLX_API ErrorCode			getClassInfo( Klass klass, ClassInfo* classInfo );
	RFLX_API ErrorCode			createObject( const char* name, RflxObject** object );
	RFLX_API ErrorCode			createObject( Klass klass, RflxObject** object );
	RFLX_API ErrorCode			destroyObject( RflxObject* object );
	RFLX_API ErrorCode			getObjectClass( RflxObject* object, ClassInfo* info );
	RFLX_API bool				isInstanceOf( const RflxObject* object, Klass klass );
	RFLX_API bool				isInstanceOfEx( const RflxObject* object, Klass klass, size_t* offset = NULL );
	RFLX_API bool				isKindOf( Klass klass, Klass base );
	RFLX_API bool				isKindOfEx( Klass klass, Klass base, size_t* offset = NULL );
	RFLX_API bool				isDerivesFrom( Klass klass, Klass base );
	RFLX_API bool				isDerivesFromEx( Klass klass, Klass base, size_t* offset = NULL );
	RFLX_API bool				isPolymorphic( Klass klass ); // is it a Polymorphic object?
	RFLX_API void*				getBaseObject( Klass src, void* object );
	RFLX_API const void*		getBaseObject( Klass src, const void* object );
	RFLX_API unsigned int		getChildrenNum( Klass klass ); // how many classes derive from?
	RFLX_API Klass				getBaseClass( Klass klass );
	RFLX_API const char*		getClassName( Klass klass );
	RFLX_API unsigned int		getClassFlags( Klass klass ); // return flags: PropTraitFlag
	RFLX_API bool				setClassDataHandler( Klass klass, const CustomDataHandler* handler, const CustomDataHandler** prev = NULL );
	RFLX_API const PropDef*		getClassPropDef( Klass klass, unsigned int* count ); // get property table directlly
	RFLX_API unsigned int		getClassPropertyId( Klass klass, const char* name ); // get property index
	RFLX_API const PropDef*		getClassProperty( Klass klass, const char* name );
	RFLX_API const PropDef*		getClassPropertyDefById( Klass klass, unsigned int id );
	RFLX_API const PropDef*		getClassPropertyDefByPosition( const PropPos* pos );
	RFLX_API const PropDef*		getClassPropertyEx( Klass klass, const char* fullPath );
	// get property position, use this position to access it value after
	// if a object has tow or more properties have same name, you should use "owner" to specify which one is you want.
	// "recur" means if property not found in current class, it will try to find in its base class.
	RFLX_API const PropPos*		getClassPropertyPosition( Klass klass, const char* name, Klass owner = NULL, bool recur = true );
	// this function present you a easy way to access complex propery such as custom type, container, pointer.
	// fullPath could be "yourStructArray[10].address.x"
	RFLX_API const PropPosEx*	getClassPropertyPositionEx( Klass klass, const void* object, const char* fullPath );
	RFLX_API ErrorCode			forBaseEachProperty( Klass klass, void* mbase, void ( *func )( const PropDef* def, void* object, void* extra ), void* extra = NULL );
	RFLX_API ErrorCode			forBaseEachPropertyEx( Klass klass, void* mbase, bool ( *filter )( Klass klass ), void ( *func )( const PropDef* def, void* outerObject, void* extra ), void* extra = NULL );
	RFLX_API ErrorCode			setBaseProperty( void* mbase, const PropPos* pos, const ValueData* value, unsigned int index = 0 );
	RFLX_API ErrorCode			getBaseProperty( const void* mbase, const PropPos* pos, ValueData* value, unsigned int index = 0 );
	RFLX_API ErrorCode			setBasePropertyByName( Klass klass, void* mbase, const ValueData* value, const char* name, unsigned int index = 0, Klass owner = NULL, bool recur = true );
	RFLX_API ErrorCode			getBasePropertyByName( Klass klass, const void* mbase, ValueData* value, const char* name, unsigned int index = 0, Klass owner = NULL, bool recur = true );
	RFLX_API ErrorCode			getBasePropertyRef( Klass klass, const void* mbase, const PropPos* pos, void** ref );
	RFLX_API ErrorCode			getBasePropertyRefByName( Klass klass, const void* mbase, void** ref, const char* name, Klass owner = NULL, bool recur = true );
	RFLX_API ErrorCode			setBasePropertyByNameEx( Klass klass, void* mbase, const ValueData* value, const char* fullpath, unsigned int index = 0 );
	RFLX_API ErrorCode			getBasePropertyByNameEx( Klass klass, const void* mbase, ValueData* value, const char* fullpath, unsigned int index = 0 );
	RFLX_API ErrorCode			setBasePropertyByNameFromString( Klass klass, const void* mbase, const char* name, const char* value, unsigned int index = 0, Klass owner = NULL, bool recur = true );
	RFLX_API ErrorCode			forObjectEachProperty( RflxObject* object, void ( *func )( const PropDef* def, void* object, void* extra ), void* extra = NULL );
	RFLX_API ErrorCode			forObjectEachPropertyEx( RflxObject* object, bool ( *filter )( Klass klass ), void ( *func )( const PropDef* def, void* outerObject, void* extra ), void* extra = NULL );
	RFLX_API const PropDef*		getObjectPropertyDef( const RflxObject* object, const char* name, PropPos* pos, Klass owner = NULL, bool recur = true );
	RFLX_API const PropPos*		getObjectPropertyPosition( const RflxObject* object, const char* name, Klass owner = NULL, bool recur = true );
	RFLX_API ErrorCode			setObjectProperty( RflxObject* object, const PropPos* pos, const ValueData* value, unsigned int index = 0 );
	RFLX_API ErrorCode			getObjectProperty( const RflxObject* object, const PropPos* pos, ValueData* value, unsigned int index = 0 );
	// parse a string and try to convert it into value
	// NOTE: it supports base type: number, bool, string, and enum fully and supports custom data type partially, no pointer support
	RFLX_API ErrorCode			setObjectPropertyByNameFromString( RflxObject* object, const char* name, const char* value, unsigned int index = 0, Klass owner = NULL, bool recur = true );
	RFLX_API ErrorCode			setObjectPropertyByName( RflxObject* object, const ValueData* value, const char* name, unsigned int index = 0, Klass owner = NULL, bool recur = true );
	RFLX_API ErrorCode			getObjectPropertyByName( const RflxObject* object, ValueData* value, const char* name, unsigned int index = 0, Klass owner = NULL, bool recur = true );
	RFLX_API ErrorCode			setObjectPropertyByNameEx( RflxObject* object, const ValueData* value, const char* fullpath, unsigned int index = 0 );
	RFLX_API ErrorCode			getObjectPropertyByNameEx( const RflxObject* object, ValueData* value, const char* fullpath, unsigned int index = 0 );
	RFLX_API ErrorCode			getObjectPropertyRef( const RflxObject* object, const PropPos* pos, void** ref ); // return a property's address directlly for fast usecase.
	RFLX_API ErrorCode			getObjectPropertyRefByName( RflxObject* object, void** ref, const char* name, Klass owner = NULL, bool recur = true );
	RFLX_API ErrorCode			operateFunctionWrapper( pfn_operator func, const PropDef* def, void* userObject, PropOp op, ... );
	// these are function use to deal container type such as std::vector, std::map
	RFLX_API ErrorCode			operateObjectPropertyByName( RflxObject* object, const char* name, PropOp opType, ... );
	RFLX_API ErrorCode			operateObjectPropertyByNameEx( RflxObject* object, const char* fullpath, PropOp opType, ... );
	RFLX_API ErrorCode			operateObjectProperty( RflxObject* object, const PropPos* pos, PropOp opType, ... );
	RFLX_API ErrorCode			operateObjectPropertyEx( const PropPosEx* pos, PropOp opType, ... );
	// set all properties of object to default value if any.
	// you could specify class as a start point of recursion.
	RFLX_API ErrorCode			setObjectPropertiesDefault( RflxObject* object, Klass target = NULL, bool recur = false );
	RFLX_API ErrorCode			setBasePropertiesDefault( void* mbase, Klass klass, Klass target = NULL, bool recur = false );
	RFLX_API const Klass*		getFirstChildren( Klass klass ); // the all child classes array begin
	RFLX_API unsigned int		getChildNum( Klass klass );
	RFLX_API unsigned int		getClassInstanceCount( Klass klass ); // how many instances have been created?
	RFLX_API const char*		removeNamespace( const char* name );
	RFLX_API const EnumInfo*	findEnumInfo( const char* name, Ktex ktex = NULL );
	RFLX_API const EnumInfo*	findEnumInfoByItemName( const char* name, long* value, Ktex ktex = NULL );
	RFLX_API const MsgFunc*		findMessageHandler( Klass klass, MessageId id );
	RFLX_API const MsgFuncChain*findMessageHandlerChain( Klass klass, MessageId id );
	// for RflxObject based class
	RFLX_API ErrorCode			sendMessage( RflxObject* object, Message* msg );
	// for Rflxable based class
	RFLX_API ErrorCode			sendMessage( Klass klass, void* mbase, Message* msg );
	RFLX_API void				messageEnable( bool enable = true );
	RFLX_API bool				isMessageEnabled();
}

#ifdef _MSC_VER
	#pragma warning( pop )
#endif

#include "Rflext.h"

#endif
