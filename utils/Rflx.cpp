#include "Rflx.h"
#include <sstream>
#include <memory>
#include <algorithm>
#include <vector>
#include <map>
#include <set>
#ifdef _MSC_VER
	#pragma warning( disable : 4996 4706 )
#endif

namespace rflx
{
	static inline bool _isSpace( char c ) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }

	static char* _strtrim( char* str ) {
		const char* cur = str;
		int dst = 0;
		while ( *cur ) {
			if ( !_isSpace( *cur ) ) {
				str[ dst++ ] = *cur;
			}
			++cur;
		}
		str[ dst ] = 0;
		return str;
	}

	template< typename T >
	static T* _strdup( const T* str ) {		
		size_t count = std::char_traits< T >::length( str ) + 1;
		T* p = ( T* )RFLX_MALLOC( sizeof( T ) * count );
		std::char_traits< T >::copy( p, str, count );
		return p;
	};

	template< typename T >
	void _freestr( T*& str ) {
		RFLX_FREE( str );
		str = NULL;
	}

	char* w2m( const wchar_t* wcs ) {
		int len;
		char* buf;
		len = wcstombs( NULL, wcs, 0 );
		if ( len == 0 ) {
			return NULL;
		}
		buf = (char*)malloc( sizeof( char ) * ( len + 1 ) );
		memset( buf, 0, sizeof( char ) * ( len + 1 ) );
		len = wcstombs( buf, wcs, len + 1 );
		return buf;
	}

	std::string w2m( const std::wstring& str ) {
		char* p = w2m( str.c_str() );
		std::string ret( p );
		free( p );
		return ret;
	}

	wchar_t* m2w( const char* mbs ) {
		int len;
		wchar_t* buf;
		len = mbstowcs( NULL, mbs, 0 );
		if ( len == 0 ) {
			return NULL;
		}
		buf = ( wchar_t* )malloc( sizeof( wchar_t ) * ( len + 1 ) );
		memset( buf, 0, sizeof( wchar_t ) * ( len + 1 ) );
		len = mbstowcs( buf, mbs, len + 1 );
		return buf;
	}

	std::wstring m2w( const std::string& str ) {
		wchar_t* p = m2w( str.c_str() );
		std::wstring ret( p );
		free( p );
		return ret;
	}

	namespace _internal
	{
		static bool _messageEnabled = true;
		static inline bool _isMessageEnabled()
		{
			return _messageEnabled;
		}
		template< typename T >
		int _tstrcmp( const T* src, const T* dst )
		{
			RFLX_DASSERT( src && dst );
			if ( src == dst ) {
				return 0;
			}
			int ret = 0;
			while ( ! ( ret = *( unsigned int* )src - *( unsigned int* )dst ) && *dst ) {
				++src, ++dst;
			}
			if ( ret < 0 ) {
				ret = -1 ;
			} else if ( ret > 0 ) {
				ret = 1 ;
			}
			return ret;
		}
		int _strcmp( const char* src, const char* dst )
		{
			return _tstrcmp( src, dst );
		}
	}

	namespace _internal
	{
		const DataTypeSizeName* _getValueDataTypeSizeNameTable()
		{
			static const DataTypeSizeName _ValueDataTypeSizeNameTable[ vdt_max_num ] = 
			{
				{ 0, vdt_nil, "<nil>" },
				{ 0, vdt_reflexable_begin, "<reflexable_begin>" },
				{ 0, vdt_base_type_begin, "<base_type_begin>" },
						{ sizeof(bool), vdt_bool, "bool", "Boolean" },
						{ 0, vdt_number_begin, "<number_begin>" },
							{ 0, vdt_integer_number_begin, "<integer_number_begin>" },
								{ 0, vdt_signed_integer_number_begin, "<signed_integer_number_begin>" },
									{ sizeof(signed char), vdt_schar, "signed char", "SChar" },
									{ sizeof(char), vdt_char, vdt_char == vdt_schar_ ? "char" : "", vdt_char == vdt_schar_ ? "Char" : "" },
									{ sizeof(short), vdt_short, "short", "Int16" },
									{ sizeof(int), vdt_int, "int", "Int32" },
									{ sizeof(long), vdt_long, "long", "Long" },
									{ sizeof(long long), vdt_llong, "long long", "LLong" },
								{ 0, vdt_signed_integer_number_end, "<signed_integer_number_end>" },
								{ 0, vdt_unsigned_integer_number_begin, "<unsigned_integer_number_begin>" },
									{ sizeof(wchar_t), vdt_wchar, "wchar_t" },
									{ sizeof(unsigned char), vdt_uchar, "unsigned char", "UChar" },
									{ sizeof(char), vdt_char, vdt_char == vdt_uchar_ ? "char" : "", vdt_char == vdt_uchar_ ? "Char" : "" },
									{ sizeof(unsigned short), vdt_ushort, "unsigned short", "UInt16" },
									{ sizeof(unsigned int), vdt_uint, "unsigned int", "UInt32" },
									{ sizeof(unsigned long), vdt_ulong, "unsigned long", "ULong" },
									{ sizeof(unsigned long long), vdt_ullong, "unsigned long long", "ULLong" },
								{ 0, vdt_unsigned_integer_number_end, "<unsigned_integer_number_end>" },
							{ 0, vdt_integer_number_end, "<integer_number_end>" },
							{ 0, vdt_float_number_begin, "<float_number_begin>" },
								{ sizeof(float), vdt_float, "float", "Single" },
								{ sizeof(double), vdt_double, "double", "Double" },
							{ 0, vdt_float_number_end, "<float_number_end>" },
						{ 0, vdt_number_end, "<number_end>" },
					{ 0, vdt_base_type_end, "<base_type_end>" },
					{ 0, vdt_string, "std::string", "string" },
					{ 0, vdt_wstring, "std::wstring", "wstring" },
					{ 0, vdt_enum, "enum" },
					{ 0, vdt_custom, "custom" },
					{ sizeof(const void*), vdt_pointer, "pointer" },
				{ 0, vdt_reflexable_end, "<reflexable_end>" },
			};
			return _ValueDataTypeSizeNameTable;
		}
	}

	//-----------------------------------------------------------------------------------------------------------------------
	// PropHandle
	//-----------------------------------------------------------------------------------------------------------------------
	PropHandle& PropHandle::operator = ( const PropHandle& o ) {
		if ( &o != this ) {
			clear();
			if ( o.pos ) {
				pos = new PropPosEx( *o.pos );
			}
		}
		return *this;
	}
	
	PropHandle::PropHandle( const PropHandle& o ) : pos( NULL ) { if ( o.pos ) pos = new PropPosEx( *o.pos ); }
	PropHandle::PropHandle( const PropPosEx* p ) : pos( NULL ) { if ( p ) pos = new PropPosEx( *p ); }
	PropHandle::PropHandle( const PropPos* p ) : pos( NULL ) { if ( p ) pos = new PropPosEx( *p ); }

	PropHandle::~PropHandle() { delete pos; }

	void PropHandle::clear() {
		if ( pos ) {
			delete pos;
			pos = NULL;
		}
	}
	//-----------------------------------------------------------------------------------------------------------------------
	// ValueData
	//-----------------------------------------------------------------------------------------------------------------------
	ValueData::ValueData( const std::string& value )
	{
		type = vdt_string;
		_len = value.length();
		size_t count = _len + 1;
		char* p = ( char* )RFLX_MALLOC( sizeof( char ) * count );
		std::char_traits< char >::copy( p, value.c_str(), count );
		_string = p;
	}

	ValueData::ValueData( const std::wstring& value )
	{
		type = vdt_wstring;
		_len = value.length();
		size_t count = _len + 1;
		wchar_t* p = ( wchar_t* )RFLX_MALLOC( sizeof( wchar_t ) * count );
		if ( p ) {
			std::char_traits< wchar_t >::copy( p, value.c_str(), count );
		}
		_wstring = p;
	}

	ValueData::ValueData( const char* value ) : type( vdt_nil )
	{
		if ( value ) {
			_len = std::char_traits< char >::length( value );
			size_t count = _len + 1;
			char* p = ( char* )RFLX_MALLOC( sizeof( char ) * count );
			if ( p ) {
				std::char_traits< char >::copy( p, value, count );
			}
			_string = p;
			type = vdt_string;
		}
	}

	ValueData::ValueData( const wchar_t* value ) : type( vdt_nil )
	{
		if ( value ) {
			_len = std::char_traits< wchar_t >::length( value );
			size_t count = _len + 1;
			wchar_t* p = ( wchar_t* )RFLX_MALLOC( sizeof( wchar_t ) * count );
			if ( p ) {
				std::char_traits< wchar_t >::copy( p, value, count );
			}
			_wstring = p;
			type = vdt_wstring;
		}
	}

	ValueData::ValueData( const void* ptr ) : type( vdt_pointer ), _pointer( ptr )
	{
	}

	ValueData::ValueData( const void* custom, const CustomDataHandler* handler ) : type( vdt_nil )
	{
		if ( custom && handler ) {
			_customHandler = ( CustomDataHandler* )::RFLX_MALLOC( sizeof( CustomDataHandler ) );
			if ( _customHandler ) {
				memcpy( _customHandler, handler, sizeof( CustomDataHandler ) );
			}
			_custom = handler->clone( custom );
			type = vdt_custom;
		}
	}

	ValueData::~ValueData()
	{
		clear();
	}

	ValueData::ValueData( const ValueData& other ) : type( vdt_nil )
	{
		copy( other );
	}

	void ValueData::clear()
	{
		switch( type ) {
			case vdt_string: {
					::RFLX_FREE( _string );
					_string = NULL;
					_len = 0;
				}
				break;
			case vdt_wstring: {
					::RFLX_FREE( _wstring );
					_wstring = NULL;
					_len = 0;
				}
				break;
			case vdt_enum:
				_enumInfo  = NULL;
				break;
			case vdt_custom: {
					if ( _customHandler ) {
						_customHandler->destroy( _custom );
					}
					_custom = NULL;
					::RFLX_FREE( _customHandler );
					_customHandler = NULL;
				}
				break;
		};
		type = vdt_nil;
	}

	ValueData& ValueData::swap( ValueData& other ) {
		if ( this != &other ) {
			char m[ sizeof( *this ) ];
			memcpy( m, &other, sizeof( *this ) );
			memcpy( &other, this, sizeof( *this ) );
			memcpy( this, m, sizeof( *this ) );
		}
		return *this;
	}

	ValueData& ValueData::copy( const ValueData& other )
	{
		if ( this == &other )
			return *this;
		clear();
		type = other.type;
		switch ( type ) {
		case vdt_string:
			{
				_len = other._len;
				size_t count = _len + 1;
				char* p = ( char* )RFLX_MALLOC( sizeof( char ) * count );
				if ( p ) {
					std::char_traits< char >::copy( p, other._string, count );
					_string = p;
				} else {
					_string = NULL;
				}
			}
			break;
		case vdt_wstring:
			{
				_len = other._len;
				size_t count = _len + 1;
				wchar_t* p = ( wchar_t* )RFLX_MALLOC( sizeof( wchar_t ) * count );
				if ( p ) {
					std::char_traits< wchar_t >::copy( p, other._wstring, count );
					_wstring = p;
				} else {
					_wstring = NULL;
				}
			}
			break;
		case vdt_enum:
			{
				_enum = other._enum;
				_enumInfo = other._enumInfo;
			}
			break;
		case vdt_custom:
			{
				_custom = NULL;
				RFLX_DASSERT( other._customHandler );
				if ( other._custom ) {
					_custom = other._customHandler->clone( other._custom );
				}
				_customHandler = ( CustomDataHandler* )::RFLX_MALLOC( sizeof( CustomDataHandler ) );
				RFLX_DASSERT( _customHandler );
				memcpy( _customHandler, other._customHandler, sizeof( CustomDataHandler ) );
			}
			break;
		default:;
			size_t size = getValueDataTypeSize( other.type );
			memcpy( &_bool, &other._bool, size );
		}
		return *this;
	}

	ValueData& ValueData::operator = ( const char* value )
	{
		clear();
		if ( value ) {
			_len = std::char_traits< char >::length( value );
			size_t count = _len + 1;
			char* p = ( char* )RFLX_MALLOC( sizeof( char ) * count );
			if ( p ) {
				std::char_traits< char >::copy( p, value, count );
			}
			_string = p;
			type = vdt_string;
		}
		return *this;
	}

	ValueData& ValueData::operator = ( const wchar_t* value )
	{	
		clear();
		if ( value ) {
			_len = std::char_traits< wchar_t >::length( value );
			size_t count = _len + 1;
			wchar_t* p = ( wchar_t* )RFLX_MALLOC( sizeof( wchar_t ) * count );
			if ( p ) {
				std::char_traits< wchar_t >::copy( p, value, count );
			}
			_wstring = p;
			type = vdt_wstring;
		}
		return *this;
	}

	ValueData& ValueData::operator = ( const std::string& value )
	{		
		clear();
		_len = value.length();
		size_t count = _len + 1;
		char* p = ( char* )RFLX_MALLOC( sizeof( char ) * count );
		if ( p ) {
			std::char_traits< char >::copy( p, value.c_str(), count );
		}
		_string = p;
		type = vdt_string;
		return *this;
	}

	ValueData& ValueData::operator = ( const std::wstring& value )
	{
		clear();
		_len = value.length();
		size_t count = _len + 1;
		wchar_t* p = ( wchar_t* )RFLX_MALLOC( sizeof( wchar_t ) * count );
		if ( p ) {
			std::char_traits< wchar_t >::copy( p, value.c_str(), count );
		}
		_wstring = p;
		type = vdt_wstring;
		return *this;
	}

	ValueData& ValueData::operator = ( const void* value )
	{
		clear();
		type = vdt_pointer;
		_pointer = value;
		return *this;
	}

	ValueData& ValueData::assign( const void* custom, const CustomDataHandler* handler )
	{
		clear();
		RFLX_DASSERT( handler );
		_customHandler = ( CustomDataHandler* )::RFLX_MALLOC( sizeof( CustomDataHandler ) );
		RFLX_DASSERT( _customHandler );
		memcpy( _customHandler, handler, sizeof( CustomDataHandler ) );
		if ( custom ) {
			_custom = handler->clone( custom );
			type = vdt_custom;
		}
		return *this;
	}

	ValueData& ValueData::assign( long enumValue, const EnumInfo* enumInfo )
	{
		if ( enumInfo ) {
			const EnumValue* item = enumInfo->findItemByValue( enumValue );
			clear();
			type = vdt_enum;
			_enumInfo = enumInfo;
			if ( item ) {
				RFLX_DASSERT( item->value == enumValue );
				_enum = enumValue;
			} else {
				_enum = enumInfo->getCount() > 0 ? enumInfo->getItemByIndex(0)->value : 0L;
				// TODO: warning report
			}
		}
		return *this;
	}

	bool ValueData::toString( std::string& result, const CustomDataHandler* handler ) const throw()
	{
		result.clear();
		std::stringstream interpreter;
		interpreter.imbue( std::locale::classic() );
		switch ( type ) {
		case vdt_bool:
			result = _bool ? "true" : "false";
			return true;
			break;
		case vdt_schar:	
		case vdt_schar_:
			interpreter << (int)_char;
			break;
		case vdt_short:			
			interpreter << (int)_short;
			break;
		case vdt_int:			
			interpreter << _int;
			break;
		case vdt_long:			
			interpreter << _long;
			break;
		case vdt_llong:			
			interpreter << _llong;
			break;
		case vdt_wchar:			
			interpreter << (unsigned int)_wchar;
			break;
		case vdt_uchar:	
		case vdt_uchar_:
			interpreter << (unsigned int)_uchar;
			break;
		case vdt_ushort:			
			interpreter << (unsigned int)_ushort;
			break;
		case vdt_uint:			
			interpreter << _uint;
			break;
		case vdt_ulong:			
			interpreter << _ulong;
			break;
		case vdt_ullong:			
			interpreter << _ullong;
			break;
		case vdt_float:			
			interpreter << _float;
			break;
		case vdt_double:			
			interpreter << _double;
			break;
		case vdt_string:
			{
				if ( _string ) {
					result = _string;
				}
				return true;
			}
			break;
		case vdt_wstring:
			{
				char* mstr = w2m( _wstring );
				if ( mstr != NULL ) {
					result.assign( mstr );
				} else {
					result.clear();
				}
				free( mstr );
				return true;
			}
			break;
		case vdt_enum:
			{
				RFLX_DASSERT( _enumInfo );
				const EnumValue* item = _enumInfo->findItemByValue( _enum );
				std::string buf( _enumInfo->getName() );
				buf.append( "::" );
				buf.append( item->name );
				interpreter << buf;
			}
			break;
		case vdt_custom:
			{
				const CustomDataHandler* ch = handler ? handler : _customHandler;
				RFLX_DASSERT( ch && "None custom data handler!" );
				char buf[1024];
				buf[0] = 0;
				size_t len = 1024;
				ErrorCode e = ch->toString( _custom, buf, &len );
				if ( e == err_ok ) {
					result = buf;
					return true;
				}
			}
			break;
		case vdt_pointer:
			{
				// can't not convert a non-null pointer to string
				if ( _pointer == NULL ) {
					interpreter << "nil";
				}
			}
			break;
		default:
			return false;
		}
		if ( !interpreter || !( interpreter >> result ) || !( interpreter >> std::ws ).eof() ) {
			return false;
		}
		return true;
	}

	bool ValueData::fromString( ValueDataType targetType, const std::string& source, const CustomDataHandler* handler, Ktex ktex ) throw()
	{
		switch ( targetType )
		{
		case vdt_custom:
			{
				ValueData temp;
				const CustomDataHandler* ch = handler ? handler : _customHandler;
				RFLX_DASSERT( ch && "None custom data handler!" );
				void* obj = ( ch->create && ch->destroy ) ? ch->create() : NULL;
				size_t size = source.size();
				if ( obj && err_ok == ch->fromString( obj, source.c_str(), &size ) ) {
					temp.assign( obj, ch );
				}
				if ( !temp.isNil() ) {
					ch->destroy( obj );
					swap( temp );
					return true;
				}
			}
			break;
		case vdt_enum:
			{
				long enumValue = 0;
				const EnumInfo* enumInfo = findEnumInfoByItemName( source.c_str(), &enumValue, ktex );
				if ( !enumInfo ) {
					return false;
				}
				assign( enumValue, enumInfo );
			}
			break;
		case vdt_pointer:
			{
				// simple convert it to NULL pointer
				*this = ( const void* )NULL;
				return true;
			}
			break;
		default:
			{
				ValueData temp;
				std::stringstream interpreter;
				interpreter.imbue( std::locale::classic() );
				if ( targetType != vdt_string && targetType != vdt_wstring ) {
					interpreter << source;
					if ( !interpreter ) {
						return false;
					}
				}
				temp.type = targetType;
				switch ( targetType )
				{
				case vdt_bool:
					if ( !isdigit( source[0] ) )
					{
						std::string _s = source;
						std::transform( _s.begin(), _s.end(), _s.begin(), (int(*)(int))::tolower );
						std::string::iterator it = std::remove_if( _s.begin(), _s.end(), _isSpace );
						_s.erase( it, _s.end() );
						if ( _s == "true" ) {
							temp._bool = true;
						} else {
							temp._bool = false;
						}
						interpreter.seekg( 0, std::ios_base::end );
					}
					else
						interpreter >> temp._bool;
					break;
				case vdt_schar:
				case vdt_schar_:
					{
						int v = 0;
						interpreter >> v;
						temp._schar = ( signed char )v;
					}
					break;
				case vdt_short:
					interpreter >> temp._short;
					break;
				case vdt_int:
					interpreter >> temp._int;
					break;
				case vdt_long:
					interpreter >> temp._long;
					break;
				case vdt_llong:
					interpreter >> temp._llong;
					break;
				case vdt_wchar:
					unsigned int wchar;
					interpreter >> wchar;
					temp._wchar = (wchar_t)wchar;
					break;
				case vdt_uchar:
				case vdt_uchar_:
					{
						unsigned int v = 0;
						interpreter >> v;
						temp._uchar = ( unsigned char )v;
					}
					break;
				case vdt_ushort:
					interpreter >> temp._ushort;
					break;
				case vdt_uint:
					interpreter >> temp._uint;
					break;
				case vdt_ulong:
					interpreter >> temp._ulong;
					break;
				case vdt_ullong:
					interpreter >> temp._ullong;
					break;
				case vdt_float:
					interpreter >> temp._float;
					break;
				case vdt_double:
					interpreter >> temp._double;
					break;
				case vdt_string:
					{
						temp.type = vdt_nil;
						temp = source;
						swap( temp );
						return true;
					}
					break;
				case vdt_wstring:
					{
						wchar_t* wstr = m2w( source.c_str() );
						std::wstring _ws( wstr );
						free( wstr );
						temp.type = vdt_nil;
						temp = _ws;
						swap( temp );
						return true;
					}
					break;
				default:
					return false;
				}
				if ( !( interpreter >> std::ws ).eof() ) {
					return false;
				}
				swap( temp );
			}
		}
		return true;
	}

	static inline ValueDataType _conv2unsigned( ValueDataType type ) {
		switch ( type ) {
			case vdt_schar: return vdt_uchar;
			case vdt_schar_: return vdt_uchar;
			case vdt_short: return vdt_ushort;
			case vdt_int: return vdt_uint;
			case vdt_long: return vdt_ulong;
			case vdt_llong: return vdt_ullong;
			default: return type;
		}
	}
	
	struct CustomValuePacker {
		void* _custom;
		CustomDataHandler* _customHandler;
	};

#define RFLX_CMPOP_IMP( NAME, OP ) \
	struct CmpOp_##NAME {\
		template< typename T > \
		static inline bool invoke( const T& a, const T& b ) { RFLX_TASSERT( ( rflx::IsPtr<T>::value == false ) ); return a OP b; }\
		static inline bool invoke( char* a, char* b ) { return _internal::_tstrcmp( a, b ) OP 0; };\
		static inline bool invoke( wchar_t* a, wchar_t* b ) { return _internal::_tstrcmp( a, b ) OP 0; }\
		static inline bool invoke( const char* a, const char* b ) { return _internal::_tstrcmp( a, b ) OP 0; };\
		static inline bool invoke( const wchar_t* a, const wchar_t* b ) { return _internal::_tstrcmp( a, b ) OP 0; }\
		static inline bool invoke( const CustomValuePacker& a, const CustomValuePacker& b ) { return a._customHandler->##NAME( a._custom, b._custom ); }\
		/*disable this*/\
		static inline bool invoke( const ValueData& a, const ValueData& b );\
	};

	RFLX_CMPOP_IMP( equal, == )
	RFLX_CMPOP_IMP( less, < )
	RFLX_CMPOP_IMP( great, > )
	RFLX_CMPOP_IMP( noteq, != )
	RFLX_CMPOP_IMP( lesseq, <= )
	RFLX_CMPOP_IMP( greateq, >= )

	template< typename OP >
	struct VCmpOp {
		static bool invoke( const ValueData& a, const ValueData& b ) {
			RFLX_DASSERT( !a.isNil() && a.type == b.type );
			switch ( a.type ) {
				case vdt_char: return OP::invoke( a._char, b._char );
				case vdt_schar: return OP::invoke( a._schar, b._schar );
				case vdt_short: return OP::invoke( a._short, b._short );
				case vdt_int: return OP::invoke( a._int, b._int );
				case vdt_long: return OP::invoke( a._long, b._long );
				case vdt_llong: return OP::invoke( a._llong, b._llong );
				case vdt_wchar: return OP::invoke( a._wchar, b._wchar );
				case vdt_uchar: return OP::invoke( a._uchar, b._uchar );
				case vdt_ushort: return OP::invoke( a._ushort, b._ushort );
				case vdt_uint: return OP::invoke( a._uint, b._uint );
				case vdt_ulong: return OP::invoke( a._ulong, b._ulong );
				case vdt_ullong: return OP::invoke( a._ullong, b._ullong );
				case vdt_float: return OP::invoke( a._float, b._float );
				case vdt_double: return OP::invoke( a._double, b._double );
				default: RFLX_DASSERT( 0 );
			}
			return false;
		}
	};

	template< typename OP >
	struct VCmper {
		static bool invoke( const ValueData& a, const ValueData& b ) {
			/* one or two is nil */
			if ( a.isNil() || b.isNil() ) {
				return false;
			}
			if ( a.isBaseType() && b.isBaseType() ) {
				if ( a.isBoolean() || b.isBoolean() ) {
					bool _a; a.extract( _a );
					bool _b; b.extract( _b );
					return OP::invoke( _a, _b );
				}
				else if ( a.isFloat() || b.isFloat() ) {
					/* if there is a float point type, use float type to perform compare\
					 float type value is bigger than all integer type */
					ValueDataType ct = std::max( a.type, b.type );
					ValueData _a = a.cast( ct );
					ValueData _b = b.cast( ct );
					return VCmpOp< OP >::invoke( _a, _b );
				} else {
					/* handle all integer type here*/
					ValueDataType ct = std::max( a.type, b.type );
					/* is a signed or unsigned?*/
					bool sign_a = a.type > vdt_signed_integer_number_begin && a.type < vdt_signed_integer_number_end;
					/* is b signed or unsigned?*/
					bool sign_b = b.type > vdt_signed_integer_number_begin && b.type < vdt_signed_integer_number_end;
					if ( sign_a != sign_b ) {
						/* TODO: report\
						 signed / unsigned mismatch*/
						ct = _conv2unsigned( ct );
					}
					ValueData _a = a.cast( ct );
					ValueData _b = b.cast( ct );
					return VCmpOp< OP >::invoke( _a, _b );
				}
			}
			else if ( a.isString() && b.isString() ) {
				ValueDataType ct = std::max( a.type, b.type );
				ValueData _a = a.cast( ct );
				ValueData _b = b.cast( ct );
				if ( ct == vdt_string ) {
					return OP::invoke( ( const char*)_a._string, ( const char* )_b._string );
				} else {
					return OP::invoke( _a._wstring, _b._wstring );
				}
			} 
			else if ( a.isEnum() && b.isEnum() ) {
				if ( a._enumInfo == NULL || b._enumInfo == NULL ||
				_internal::_strcmp( a._enumInfo->getName(), b._enumInfo->getName() ) != 0 )\
				{
					RFLX_WARNING( "compare enum failed!\n" );
					return false;
				}
				return OP::invoke( a._enum, b._enum );
			}
			else if ( a.isUserData() && b.isUserData() ) {
				if ( a._customHandler == NULL || b._customHandler == NULL ||
				_internal::_strcmp( a._customHandler->name(), b._customHandler->name() ) != 0 )
				{
					RFLX_WARNING( "compare user data failed!\n" );
					return false;
				}
				CustomValuePacker _a = { a._custom, a._customHandler };
				CustomValuePacker _b = { b._custom, b._customHandler };
				return OP::invoke( _a, _b );
			}
			RFLX_WARNING( "compare failed!\n" );
			return false;
		}
	};

	bool ValueData::operator == ( const ValueData& other ) const
	{
		return VCmper< CmpOp_equal >::invoke( *this, other );
	}

	bool ValueData::operator < ( const ValueData& other ) const
	{
		return VCmper< CmpOp_less >::invoke( *this, other );
	}

	bool ValueData::operator > ( const ValueData& other ) const
	{
		return VCmper< CmpOp_great >::invoke( *this, other );
	}

	bool ValueData::operator != ( const ValueData& other ) const
	{
		return VCmper< CmpOp_noteq >::invoke( *this, other );
	}

	bool ValueData::operator <= ( const ValueData& other ) const
	{
		return VCmper< CmpOp_lesseq >::invoke( *this, other );
	}

	bool ValueData::operator >= ( const ValueData& other ) const
	{
		return VCmper< CmpOp_greateq >::invoke( *this, other );
	}

	ValueData ValueData::cast( ValueDataType targetType ) const
	{
		if ( type == targetType )
			return *this;
		ValueData result;
		std::string stream;
		if ( toString( stream ) )
			result.fromString( targetType, stream );
		return result;
	}
	//-----------------------------------------------------------------------------------------------------------------------
	//DVStack
	//-----------------------------------------------------------------------------------------------------------------------
	struct DVStack
	{
		size_t maxSize;
		std::vector< ValueData > stack;
	};

	namespace DValueStack
	{
		static DVStack GValueStack;

		DVStack* create( size_t maxSize )
		{
			DVStack* stack = new DVStack;
			stack->maxSize = maxSize;
			return stack;
		}

		void destroy( DVStack* stack )
		{
			if ( stack == &GValueStack )
				return;
			delete stack;
		}

		DVStack& getGlobal()
		{
			return GValueStack;
		}

		size_t maxSize( const DVStack& stack )
		{
			return stack.maxSize;
		}

		size_t size( const DVStack& stack )
		{
			return stack.stack.size();
		}

		ValueData& getAt( DVStack& stack, int index )
		{
			if ( index > 0 ) {
				RFLX_DASSERT( index <= (int)stack.stack.size() );
				return stack.stack[ index - 1 ];
			} else if ( index < 0 ) {
				RFLX_DASSERT( -index <= (int)stack.stack.size() );
				return stack.stack[ stack.stack.size() + index ];
			}
			return stack.stack[0];
		}

		const ValueData& getAt( const DVStack& stack, int index )
		{
			return getAt( const_cast< DVStack& >( stack ), index );
		}

		void pop( DVStack& stack )
		{
			if ( !stack.stack.empty() ) {
				stack.stack.pop_back();
			}
		}

		void push( DVStack& stack, const ValueData& value )
		{
			if ( stack.stack.size() < stack.maxSize ) {
				stack.stack.push_back( value );
			} else {
				RFLX_DASSERT( 0 && "DVStack is overflow!" );
			}
		}

		ValueData popData( DVStack& stack )
		{
			if ( stack.stack.empty() )
				return ValueData();
			else {
				ValueData result = stack.stack.back();
				stack.stack.pop_back();
				return result;
			}
		}
	}
	//-----------------------------------------------------------------------------------------------------------------------
	namespace _internal
	{
		struct _RflxClass;
		template< typename T > struct deleter {
			inline void operator()( T*& p ){ delete p; p = NULL; }
			inline void operator()( T& p ){ delete p; p = NULL; }
		};

		template< typename Key, typename T > struct map_deleter {
			typedef std::pair< Key, T > value_type;
			inline void operator()( const value_type& p ) { delete p.second; }
		};

		struct PropDefSorter {
			inline bool operator()( const PropDef* _left, const PropDef* _right ) const {
				return strcmp( _left->name, _right->name ) < 0;
			}
		};

		struct MsgMapSorter {
			inline bool operator()( const MsgFunc& _left, const MsgFunc& _right ) const {
				return _left.id < _right.id;
			}
		};

		static inline _RflxClass* _handle2Class( Klass klass )
		{
			return reinterpret_cast< _RflxClass* >( klass );
		}

		static inline Klass _class2Handle( const _RflxClass* _class )
		{
			return reinterpret_cast< Klass >( const_cast< _RflxClass* >( _class ) );
		}


		const char* _safeString( const char* str ) {
			static char _s[] = "";
			return str ? str : _s;
		}
		
		struct _PropGroup {
			std::string groupName;
			std::vector< const PropDef* > props;
			_PropGroup( const std::string& name ) : groupName( name ){}
		};

		struct _ClassInfo {
			unsigned int flags;
			size_t baseOffset;
			const char* className;
			const char* baseClassName;
			ObjectMethods methods;
			const PropDef* propDefs;
			unsigned int propCount;
			Klass* outputAddress;
			const CustomDataHandler* customDataHandler;
			unsigned int msgMapSize;
			const MsgFunc* msgMap;
			_ClassInfo( const ClassInfo& src, Klass* output ) : outputAddress( output ) {
				className = _safeString( src.className );
				baseClassName = _safeString( src.baseClassName );
				flags = src.flags;
				methods = src.methods;
				propDefs = src.propDefs;
				propCount = src.propCount;
				baseOffset = src.baseOffset;
				customDataHandler = src.customDataHandler;
				msgMap = src.msgMap;
			}
			void convertTo( ClassInfo& dst ) const {
				dst.className = className;
				dst.baseClassName = baseClassName;
				dst.flags = flags;
				dst.methods = methods;
				dst.propDefs = propDefs;
				dst.propCount = propCount;
				dst.baseOffset = baseOffset;
				dst.customDataHandler = customDataHandler;
				dst.msgMap = msgMap;
			}
		};

		struct _RflxClass;
		struct _ParentNode
		{
			const _RflxClass* klass;
			size_t offset;
			_ParentNode( const _RflxClass* _klass, size_t _offset = 0 ) : klass( _klass ), offset( _offset ) {}
			bool operator < ( const _ParentNode& right ) const { return klass < right.klass; }
			bool operator == ( const _ParentNode& right ) const { return klass == right.klass; }
			operator const _RflxClass* () const { return klass; }
		};

		typedef std::map< std::string, _RflxClass* > RflxClassBank;
		typedef std::map< std::string, EnumInfo* > EnumBank;

		static void _default_on_init() { 
			printf( "Rflx initialized!\n" );
		}

		static void _default_on_uninit() { 
			printf( "Rflx uninitialized!\n" );
		}

		static void _default_error_func( const char* format, ... ) {
			char text[ 1024 ];
			va_list ap;
			va_start( ap, format );
			vsnprintf( text, RFLX_ARRAY_COUNT( text ), format, ap );
			va_end( ap );
			RFLX_DASSERT( 0 && text );
		}

		static void _default_warning_func( const char* format, ... ) {
			char text[ 1024 ];
			va_list ap;
			va_start( ap, format );
			vsnprintf( text, RFLX_ARRAY_COUNT( text ), format, ap );
			va_end( ap );
		}

		static HookInfo _defaultHook = {
			_default_on_init,
			_default_on_uninit,
			_default_error_func,
			_default_warning_func
		};

		struct Context {
			RflxClassBank classBank;
			EnumBank enumBank;
			const HookInfo* hookInfo;
			bool initialized;
			int autoRegisteringFlag;
			Klass* autoRegisterClassHandleAddress;
			std::vector< _ClassInfo* > autoRegisterClassCache;
			static Context* instance;
			Context() : initialized( false ), autoRegisteringFlag( 0 ) {
				RFLX_DASSERT( instance == NULL );
				instance = this;
				hookInfo = &_defaultHook;
			}
			~Context() { 
				std::for_each( autoRegisterClassCache.begin(), autoRegisterClassCache.end(), deleter< _ClassInfo >() );
				std::for_each( enumBank.begin(), enumBank.end(), map_deleter< std::string, EnumInfo* >() );
			}
			void doAutoRegister() throw();
			void addAutoRegisterInfo( const ClassInfo& classInfo, Klass* outputAddress )
			{ autoRegisterClassCache.push_back( new _ClassInfo( classInfo, outputAddress ) ); }
		};

		Context* Context::instance = NULL;

		static Context& _getContext() {
			static Context _context;
			return _context;
		}

		class _MsgHandleTable {
			const static int HASH_SIZE = 61;
			std::vector< MsgFuncChain* >	entries[ HASH_SIZE ];
			struct MsgFuncChainSorter {
				inline bool operator()( const MsgFuncChain* _left, const MsgFuncChain* _right ) const {
					return _left->mf->id < _right->mf->id;
			}
		};
		public:
			static inline int hashMsgId( MessageId id ) {
				return id % HASH_SIZE;
			}
			MsgFuncChain* find( MessageId id ) {
				int idx =  hashMsgId( id );
				const std::vector< MsgFuncChain* >& entry = entries[ idx ];
				MsgFuncChainSorter func;
				MsgFunc _target = { id };
				MsgFuncChain target( &_target, NULL );
				// binary searching
				std::vector< MsgFuncChain* >::const_iterator end = entry.end();
				std::vector< MsgFuncChain* >::const_iterator it = std::lower_bound( entry.begin(), end, &target, func );
				if ( it != end && !func( &target, *it ) ) {
					return (*it);
				}
				return NULL;
			}
			MsgFuncChain* addEntry( const MsgFunc* mf ) {
				int idx =  hashMsgId( mf->id );
				std::vector< MsgFuncChain* >& entry = entries[ idx ];
				for ( size_t i = 0; i < entry.size(); ++i ) {
					if ( entry[i]->mf->id == mf->id ) {
						// already exist
						return entry[i];
					}
				}
				entry.push_back( new MsgFuncChain( mf, NULL ) );
				return entry.back();
			}
			void add( const MsgFunc* mf, Klass klass ) {
				MsgFuncChain* entry = addEntry( mf );
				RFLX_DASSERT( entry );
				if ( entry->klass == NULL ) {
					entry->klass = klass;
					return;
				}
				RFLX_DASSERT( entry->klass != klass && "redundant message handler!" );
				while ( entry->next != NULL ) {
					entry = entry->next;
				}
				entry->next = new MsgFuncChain( mf, klass );
			}
			void init() {
				for ( int i = 0; i < HASH_SIZE; ++i ) {
					std::vector< MsgFuncChain* >& entry = entries[ i ];
					std::sort( entry.begin(), entry.end(), MsgFuncChainSorter() );
					std::vector< MsgFuncChain* >( entry ).swap( entry );
				}
			}
			~_MsgHandleTable()
			{
				for ( int i = 0; i < HASH_SIZE; ++i ) {
					for ( size_t j = 0; j < entries[i].size(); ++j ) {
						MsgFuncChain* cur = entries[i][j];
						while ( cur ) {
							MsgFuncChain* next = cur->next;
							delete cur;
							cur = next;
						}
					}
				}
			}
		};

		struct _RflxClass
		{
			// do not change flags declaration position in this struct.
			// ref: _getOuter2BaseOffset
			unsigned int flags;
			size_t base2OuterOffset; // offset from polymorphic object to static object
			size_t baseOffset; // to parent class
			int	id;
			int endId;
			std::string className;
			std::string baseClassName;
			ObjectMethods methods;
			_RflxClass* parent;
			const CustomDataHandler* customDataHandler;
			MsgFunc* msgMap;
			std::vector< _RflxClass* > children; // id = (id, endId)
			std::set< _ParentNode > parentsLookup;
			std::map< std::string, _PropGroup* > propGroup;
			PropDef* propDefs;
			std::vector< const PropDef* > sortedPropDefs;
			_MsgHandleTable* msgTable;
			unsigned int propCount;
			unsigned int msgMapSize;
			unsigned int instanceCount;
			bool deprecated;
			bool staticClass;
			const Context* ownerContext;
			inline bool isStaticClass() const { return staticClass; }
			inline bool isRoot() const { return parent == NULL; }
			inline bool isLeaf() const { return children.empty(); }
			_RflxClass( const Context* context, const CustomDataHandler* handle ) :
				parent( NULL ), baseOffset( 0 ), id( -1 ), endId( -1 ), base2OuterOffset( 0 ), propDefs( NULL ), msgTable( NULL ), propCount( 0 ), instanceCount( 0 ),
				deprecated( false ), staticClass( false ), customDataHandler( handle ), msgMap( NULL ), msgMapSize( 0 ), ownerContext( context )
			{}
			~_RflxClass()
			{
				delete [] propDefs;
				delete [] msgMap;
				delete msgTable;
				std::for_each( propGroup.begin(), propGroup.end(), map_deleter< std::string, _PropGroup* >() );
				RFLX_DASSERT( _getContext().initialized == false || _getContext().initialized == true && instanceCount == 0 );
			}
			void markAsDeprecated()
			{
				deprecated = true;
				parentsLookup.clear();
				parent = NULL;
				children.clear();
				sortedPropDefs.clear();
				delete [] propDefs;
				delete [] msgMap;
				delete msgTable;
				msgTable = NULL;
				propDefs = NULL;
				msgMap = NULL;
				msgMapSize = 0;
				propCount = 0;
				std::for_each( propGroup.begin(), propGroup.end(), map_deleter< std::string, _PropGroup* >() );
				propGroup.clear();
			}
			unsigned int decInstCount() {
				RFLX_DASSERT( instanceCount > 0 );
				--instanceCount;
				unsigned int ret = instanceCount;
				if ( ret == 0 && deprecated ) {
					delete this;
				}
				return ret;
			}
			inline unsigned int incInstCount()
			{ 
				++instanceCount;
				return instanceCount;
			}
			static void initLookup( _RflxClass* _class, std::set< _ParentNode >* lookup = NULL, size_t baseOffset = 0 )
			{
				if ( !lookup ) {
					lookup = &_class->parentsLookup;
					if ( !lookup->empty() ) {
						return;
					}
				}
				if ( _class->parent ) {
					lookup->insert( _ParentNode( _class->parent, _class->baseOffset + baseOffset ) );
					initLookup( _class->parent, lookup, _class->baseOffset + baseOffset );
				}
			}
			static void initProps( _RflxClass* _class )
			{
				if ( !_class->propCount || !_class->propGroup.empty() ) {
					return;
				}
				for ( unsigned int i = 0; i < _class->propCount; ++i ) {
					const PropDef& def = _class->propDefs[ i ];
					std::string groupName = _safeString( def.groupName );
					std::map< std::string, _PropGroup* >::iterator it = _class->propGroup.find( groupName );
					if ( it == _class->propGroup.end() ) {
						it = _class->propGroup.insert( std::make_pair( groupName, new _PropGroup( groupName ) ) ).first;
					}
					it->second->props.push_back( &def );
				}
			}
			static void initMsgHandleTable( _RflxClass* _class )
			{
				if ( _class->msgMapSize > 0 && _class->msgTable == NULL ) {
					_class->msgTable = new _MsgHandleTable;
					for ( unsigned int i = 0; i < _class->msgMapSize; ++i ) {
						MsgFunc* mf = _class->msgMap + i;
						_class->msgTable->add( mf, _class2Handle( _class ) );
					}
					// add base class's function to here
					_RflxClass* parent = _class->parent;
					while ( parent ) {
						for ( unsigned int i = 0; i < parent->msgMapSize; ++i ) {
							MsgFunc* mf = parent->msgMap + i;
							_class->msgTable->add( mf, _class2Handle( parent ) );
						}
						parent = parent->parent;
					}
					_class->msgTable->init();
				}
			}
		};

		static inline unsigned int _addClassInstanceCount_inline( Klass klass )
		{
			return _handle2Class( klass )->incInstCount();
		}
		unsigned int _addClassInstanceCount( Klass klass )
		{
			return _handle2Class( klass )->incInstCount();
		}

		unsigned int _releaseClassInstanceCount( Klass klass )
		{
			return _handle2Class( klass )->decInstCount();
		}

		static void _reflashClassId_r( _RflxClass* _class, int& curId )
		{
			_class->id = curId++;
			for ( size_t i = 0; i < _class->children.size(); ++i ) {
				_reflashClassId_r( _class->children[ i ], curId );
			}
			_class->endId = curId++;
		}
		static void _reflashClassId()
		{
			static int startId;
			if ( Rflxable::_class() == NULL ) {
				return;
			}
			startId = 1;
			_RflxClass* base = _handle2Class( Rflxable::_class() );
			_reflashClassId_r( base, startId );
		}
		static ErrorCode _unregisterClass( _RflxClass* _class )
		{
			RFLX_DASSERT( _class && _class->ownerContext == &_getContext() );
			RflxClassBank::iterator it = _getContext().classBank.find( _class->className );
			if ( it == _getContext().classBank.end() ) {
				return err_class_not_found;
			}
			// class can't be release util its child have been released.
			if ( _getContext().initialized == true ) {
				// not unInitialize call
				if ( _class->isStaticClass() ) {
					return err_static_class_can_not_be_unregistered;
				}
				if ( !_class->children.empty() ) {
					return err_class_is_being_used;
				}
			}
			if ( _class->parent ) {
				_RflxClass* parent = _class->parent;
				parent->children.erase( std::find( parent->children.begin(), parent->children.end(), _class ) );
			}
			_getContext().classBank.erase( it );
			if ( _class->methods.unInitializeHook )
				_class->methods.unInitializeHook();
			if ( _class->instanceCount == 0 ) {
				delete _class;
			}
			else {// mark this class info deprecated, it will be released latter.
				 // depredcated class part of information will lose.
				_class->markAsDeprecated();
			}
			_reflashClassId();
			return err_ok;
		}

		static ErrorCode _unregisterRootClass( _RflxClass* _class )
		{
			while ( !_class->children.empty() ) {
				ErrorCode err = _unregisterRootClass( _class->children.back() );
				if ( err != err_ok ) {
					RFLX_DASSERT( 0 && "_unregisterRootClass failed!" );
					return err;
				}
			}
			RFLX_DASSERT( _class->isLeaf() );
			return _unregisterClass( _class );
		}

		static ErrorCode _registerBaseObject() {
			RflxObjectName< Rflxable >::name( "Rflxable" );
			RflxObjectName< RflxObject >::name( "RflxObject" );
			if ( Rflxable::_class() || RflxObject::_class() ) {
				return err_ok;
			}
			ErrorCode err = err_ok;
			{
				ClassInfo ci;
				Klass& klass = const_cast< Klass& >( Rflxable::_class() );
				memset( &ci, 0, sizeof( ci ) );
				ci.baseClassName = NULL;
				ci.className = Rflxable::_name();
				err = registerClass( &ci, &klass );
				if ( err == err_class_is_already_registered ) {
					err = err_ok;
				}
				_RflxClass* _class = _handle2Class( Rflxable::_class() );
				_class->baseOffset = 0;
			}	
			{
				ClassInfo ci;
				Klass& klass = const_cast< Klass& >( RflxObject::_class() );
				memset( &ci, 0, sizeof( ci ) );
				ci.baseClassName = Rflxable::_name();
				ci.className = RflxObject::_name();
				err = registerClass( &ci, &klass );
				if ( err == err_class_is_already_registered ) {
					err = err_ok;
				}
				_RflxClass* _class = _handle2Class( RflxObject::_class() );
				_class->baseOffset = 0;
			}
			return err;
		}

		const EnumInfo* _addEnumInfo( const char* name, const EnumValue* values, unsigned int count )
		{
			RFLX_DASSERT( name && values );
			std::string _name( name );
			EnumBank::iterator it = _getContext().enumBank.find( _name );
			if ( _getContext().enumBank.end() != it ) {
				RFLX_DASSERT( count == it->second->getCount() && "Enum definition miss match!" );
				return it->second;
			}
			return _getContext().enumBank.insert( std::make_pair( _name, new EnumInfo( name, values, count ) ) ).first->second;
		}

		const EnumInfo* _findEnumInfo( const char* name )
		{
			RFLX_DASSERT( name );
			std::string _name( name );
			EnumBank::iterator it = _getContext().enumBank.find( _name );
			if ( _getContext().enumBank.end() != it ) {
				return it->second;
			}
			return NULL;
		}

		void Context::doAutoRegister() throw()
		{
			for ( ;!autoRegisterClassCache.empty(); ) {
				std::vector< _ClassInfo* >::iterator it = autoRegisterClassCache.begin();
				while ( it != autoRegisterClassCache.end() ) {
					const _ClassInfo* classInfo = *it;
					ClassInfo ci;
					classInfo->convertTo( ci );
					Klass& klass = *classInfo->outputAddress;
					if ( !classInfo->baseClassName || classInfo->baseClassName[0] == '\0' ) {
						if ( err_ok != registerClass( &ci, &klass ) ) {
							RFLX_DASSERT( 0 && "doAutoRegister registerClass failed!" );
						}
						delete classInfo;
						it = autoRegisterClassCache.erase( it );
					} else {
						ErrorCode err = registerClass( &ci, &klass );
						if ( err == err_ok || err == err_class_is_already_registered ) {
							delete classInfo;
							it = autoRegisterClassCache.erase( it );
						} else if ( err == err_base_class_not_found ) {
							++it;
						} else {
							RFLX_DASSERT( 0 && "doaAutoRegister registerClass failed!" );
						}
					}
				}
			}
			std::for_each( autoRegisterClassCache.begin(), autoRegisterClassCache.end(), deleter< _ClassInfo >() );
		}
	
		static inline void _initObjectRtti_inline( RflxObject* object, Klass klass ) 
		{
			// hack the memory
			*reinterpret_cast< Klass* >( object ) = klass;
		}
		void _initObjectRtti( RflxObject* object, Klass klass ) 
		{
			object->_outer = klass;
		}
	}

	using namespace _internal;

	template< class T >
	static inline const ValueData* _makeDefaultValue( const T& value )
	{
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
	}

#define RFLX_MAKE_DEFAULT_VALUE_IMP( TYPE ) \
	const ValueData* makeDefaultValue( const TYPE& value ) { return _makeDefaultValue( value ); }

	RFLX_MAKE_DEFAULT_VALUE_IMP( char )
	RFLX_MAKE_DEFAULT_VALUE_IMP( signed char )
	RFLX_MAKE_DEFAULT_VALUE_IMP( short )
	RFLX_MAKE_DEFAULT_VALUE_IMP( int )
	RFLX_MAKE_DEFAULT_VALUE_IMP( long )
	RFLX_MAKE_DEFAULT_VALUE_IMP( long long )
	RFLX_MAKE_DEFAULT_VALUE_IMP( wchar_t )
	RFLX_MAKE_DEFAULT_VALUE_IMP( unsigned char )
	RFLX_MAKE_DEFAULT_VALUE_IMP( unsigned short )
	RFLX_MAKE_DEFAULT_VALUE_IMP( unsigned int )
	RFLX_MAKE_DEFAULT_VALUE_IMP( unsigned long )
	RFLX_MAKE_DEFAULT_VALUE_IMP( unsigned long long )
	RFLX_MAKE_DEFAULT_VALUE_IMP( float )
	RFLX_MAKE_DEFAULT_VALUE_IMP( double )
	RFLX_MAKE_DEFAULT_VALUE_IMP( std::string )
	RFLX_MAKE_DEFAULT_VALUE_IMP( std::wstring )

	const ValueData* makeDefaultValue( const bool& value ) {
		static ValueData _true( true );
		static ValueData _false( false );
		return value ? &_true : &_false;
	}

	RflxObject::RflxObject() : _outer( NULL )
	{
		Klass hclass = _class();
		RFLX_DASSERT( hclass );
		_internal::_initObjectRtti_inline( this, hclass );
		_internal::_addClassInstanceCount_inline( hclass );
	}

	RflxObject::RflxObject( const RflxObject& )
	{
		Klass hclass = _class();
		RFLX_DASSERT( hclass );
		_internal::_initObjectRtti_inline( this, hclass );
		_internal::_addClassInstanceCount_inline( hclass );
	}

	RflxObject::~RflxObject()
	{
		_internal::_releaseClassInstanceCount( _class() );
	}

	RFLX_IMP_AUTO_REGISTER_WITH_NAME( RflxDynamicObject, "RflxDynamicObject" )

	RflxDynamicObject::RflxDynamicObject()
	{
		Klass hclass = _class();
		RFLX_DASSERT( hclass );
		_internal::_initObjectRtti_inline( this, hclass );
		_internal::_addClassInstanceCount_inline( hclass );
	}

	RflxDynamicObject::RflxDynamicObject( const RflxDynamicObject& )
	{	
		Klass hclass = _class();
		RFLX_DASSERT( hclass );
		_internal::_initObjectRtti_inline( this, hclass );
		_internal::_addClassInstanceCount_inline( hclass );
	}

	RflxDynamicObject::~RflxDynamicObject()
	{
		_internal::_releaseClassInstanceCount( _class() );
	}

	const HookInfo* setUserHook( const HookInfo* hook )
	{
		if ( !_getContext().initialized ) {
			return NULL;
		}
		if ( hook ) {
			RFLX_DASSERT( hook->pfn_error != NULL );
			RFLX_DASSERT( hook->pfn_warning != NULL );
			const HookInfo* old = _getContext().hookInfo;
			_getContext().hookInfo = hook;
			return old;
		} else {
			_getContext().hookInfo = &_defaultHook;
			return &_defaultHook;
		}
	}

	const HookInfo* getCurrentHook()
	{
		return _getContext().hookInfo;
	}

	ErrorCode initialize( const HookInfo* hook )
	{
		if ( hook ) {
			_getContext().hookInfo = hook;
		}
		for ( int i = 0; i < vdt_max_num; ++i ) {
			if ( _getValueDataTypeSizeNameTable()[ i ].name[0] == '\0' ) {
				continue;
			}
			RFLX_DASSERT( _getValueDataTypeSizeNameTable()[ i ].value == ( ValueDataType )i );
		}
		if ( _getContext().initialized ) {
			return err_ok;
		}
		_getContext().autoRegisteringFlag = 1;
		_registerBaseObject();
		_getContext().doAutoRegister();
		_getContext().autoRegisteringFlag = 2;
		_getContext().initialized = true;
		if ( _getContext().hookInfo->pfn_on_init )
			_getContext().hookInfo->pfn_on_init();
		return err_ok;
	}

	bool hasInitialized()
	{
		return Rflxable::_class() != NULL;
	}

	ErrorCode unInitialize()
	{
		// clean the root class ref
		// _reflashClassId will not work any more
		const_cast< Klass& >( Rflxable::_class() ) = NULL;
		const_cast< Klass& >( RflxObject::_class() ) = NULL;
		const_cast< Klass& >( RflxDynamicObject::_class() ) = NULL;

		if ( _getContext().hookInfo->pfn_on_uninit )
			_getContext().hookInfo->pfn_on_uninit();
		RflxClassBank& classBank = _getContext().classBank;
		_getContext().initialized = false;
		std::vector< _RflxClass* > rootClasses;
		RflxClassBank::iterator it = classBank.begin();
		while ( it != classBank.end() ) {
			if ( it->second->isRoot() ) {
				rootClasses.push_back( it->second );
			}
			++it;
		}
		std::for_each( rootClasses.begin(), rootClasses.end(), _unregisterRootClass );
		classBank.clear();
		return err_ok;
	}

	Ktex getContext( Klass klass )
	{
		if ( !klass ) { 
			return ( Ktex )&_getContext();
		}
		_RflxClass* _class = _handle2Class( klass );
		return ( Ktex )_class->ownerContext;
	}

	Ktex getCurrentContext()
	{
		return ( Ktex )&_getContext();
	}

	Klass findClass( const char* name, Ktex* ktex )
	{
		RFLX_DASSERT( name );
		const Context& context = ktex ? *(const Context*)ktex : _getContext();
		const RflxClassBank& classBank = context.classBank;
		RflxClassBank::const_iterator it = classBank.find( name );
		if ( it == classBank.end() ) {
			return NULL;
		}
		return _class2Handle( it->second );
	}

	ErrorCode registerClass( const ClassInfo* classInfo, Klass* klass )
	{
		RFLX_DASSERT( klass && classInfo && classInfo->className && classInfo->className[0] != 0 );
		if ( !_getContext().initialized && _getContext().autoRegisteringFlag == 0 ) {
			if ( *klass == NULL ) {
				// mark this is already in the list which will be registered automaticlly.
				*klass = (Klass)0x01;
				_getContext().addAutoRegisterInfo( *classInfo, klass );
			}
			return err_ok;
		}
		if ( findClass( classInfo->className ) ) {
			return err_class_is_already_registered;
		}
		_RflxClass* baseClass = NULL;
		if ( classInfo->baseClassName && classInfo->baseClassName[0] != 0 ) {
			Klass base = findClass( classInfo->baseClassName );
			if ( !base ) {
				return err_base_class_not_found;
			}
			baseClass = _handle2Class( base );
			if ( checkClassFlag( baseClass->flags, cif_final ) ) {
				return err_wrong_inheriting;
			}
		}
		RflxClassBank& classBank = _getContext().classBank;
		std::auto_ptr< _RflxClass > newClass( new _RflxClass( &_getContext(), classInfo->customDataHandler ) );
		if ( classBank.insert( std::make_pair( std::string( classInfo->className ), newClass.get() ) ).second ) {
			newClass->className = classInfo->className;
			newClass->baseClassName = _safeString( classInfo->baseClassName );
			newClass->methods = classInfo->methods;
			newClass->flags = classInfo->flags;
			newClass->baseOffset = classInfo->baseOffset;
			if ( checkClassFlag( newClass->flags, cif_abstract ) ) { 
				newClass->methods.createInstance = NULL;	
				newClass->methods.destroyInstance = NULL;
			}
			// properties copy and sorting by their names
			newClass->propCount = classInfo->propCount;
			if ( newClass->propCount > 0 ) {
				newClass->propDefs = new PropDef[ newClass->propCount ];
				newClass->sortedPropDefs.resize( newClass->propCount );
				for ( unsigned int i = 0; i < classInfo->propCount; ++i ) {
					newClass->propDefs[ i ] = classInfo->propDefs[ i ];
					newClass->sortedPropDefs[ i ] = newClass->propDefs + i;
				}
				std::sort( newClass->sortedPropDefs.begin(), newClass->sortedPropDefs.end(), PropDefSorter() );
			} else {
				newClass->propDefs = NULL;
			}
			if ( classInfo->msgMap ) {
				int count = 0;
				for ( ; ; ++count ) {
					if ( classInfo->msgMap[ count ].id == xm_none ) {
						break;
					}
				}
				if ( count ) {
					newClass->msgMapSize = count; 
					newClass->msgMap = new MsgFunc[ count ];
					memcpy( newClass->msgMap, classInfo->msgMap, sizeof( MsgFunc ) * count );
					std::sort( newClass->msgMap, newClass->msgMap + count, MsgMapSorter() );
				}
			}

			if ( baseClass ) {
				baseClass->children.push_back( newClass.get() );
				newClass->base2OuterOffset = newClass->baseOffset + baseClass->base2OuterOffset;
			} else {
				// no base class
				RFLX_DASSERT( newClass->baseOffset == 0 );
			}
			newClass->parent = baseClass;
			if ( _getContext().autoRegisteringFlag < 2 ) {
				newClass->staticClass = true;
			}
			_RflxClass::initLookup( newClass.get() );
			_RflxClass::initProps( newClass.get() );
			_RflxClass::initMsgHandleTable( newClass.get() );
			*klass = _class2Handle( newClass.get() );
			if ( newClass->methods.initializeHook ) {
				newClass->methods.initializeHook();
			}
			newClass.release();
			_reflashClassId();
			return err_ok;
		}
		return err_failed;
	}

	ErrorCode unregisterClass( const char* name )
	{
		RFLX_DASSERT( name && name[0] );
		_RflxClass* _class = _handle2Class( findClass( name ) );
		if ( !_class ) {
			return err_class_not_found;
		}
		return _unregisterClass( _class );
	}

	ErrorCode unregisterRootClass( const char* name )
	{
		RFLX_DASSERT( name && name[0] );
		_RflxClass* _class = _handle2Class( findClass( name ) );
		if ( !_class ) {
			return err_class_not_found;
		}
		return _unregisterRootClass( _class );
	}

	ErrorCode getClassInfo( Klass klass, ClassInfo* classInfo )
	{
		RFLX_DASSERT( klass && classInfo );
		const _RflxClass* _class = _handle2Class( klass );
		classInfo->className = _class->className.c_str();
		classInfo->baseClassName = _class->baseClassName.c_str();
		classInfo->flags = _class->flags;
		classInfo->baseOffset = _class->baseOffset;
		classInfo->propCount = _class->propCount;
		classInfo->propDefs = _class->propDefs;
		classInfo->methods = _class->methods;
		classInfo->customDataHandler = _class->customDataHandler;
		return err_ok;
	}

	unsigned int getChildrenNum( Klass klass )
	{
		RFLX_DASSERT( klass );
		const _RflxClass* _class = _handle2Class( klass );
		return ( unsigned int )_class->children.size();
	}

	Klass getBaseClass( Klass klass )
	{
		RFLX_DASSERT( klass );
		const _RflxClass* _class = _handle2Class( klass );
		return _class2Handle( _class->parent );
	}

	const char* getClassName( Klass klass )
	{
		RFLX_DASSERT( klass );
		const _RflxClass* _class = _handle2Class( klass );
		return _class->className.c_str();
	}

	unsigned int getClassFlags( Klass klass )
	{	
		RFLX_DASSERT( klass );
		const _RflxClass* _class = _handle2Class( klass );
		return _class->flags;
	}

	bool setClassDataHandler( Klass klass, const CustomDataHandler* handler, const CustomDataHandler** prev )
	{
		RFLX_DASSERT( klass );
		_RflxClass* _class = _handle2Class( klass );
		if ( prev ) {
			*prev = _class->customDataHandler;
		}
		if ( handler ) {
			// function and return values check
			if ( !handler->name || !handler->klass || !handler->name() || !handler->klass() ) {
				// valid check
				return false;
			}
			if ( handler->name() != _class->className ) {
				// class name must be same.
				return false;
			}
		}
		_class->customDataHandler = handler;
		return true;
	}

	ErrorCode createObject( const char* name, RflxObject** object )
	{
		RFLX_DASSERT( name && object );
		Klass klass = findClass( name );
		return klass ? createObject( klass, object ) : err_class_not_found;
	}

	ErrorCode createObject( Klass klass, RflxObject** object )
	{
		RFLX_DASSERT( klass && object );
		_RflxClass* _class = _handle2Class( klass );
		if ( !_class ) {
			return err_class_not_found;
		}
		if ( checkClassFlag( _class->flags, cif_abstract ) ) {
			return err_class_is_abstract;
		}
		if ( !_class->methods.createInstance || !_class->methods.destroyInstance ) {
			return err_failed;
		}
		return _class->methods.createInstance( object );
	}

	ErrorCode destroyObject( RflxObject* object )
	{
		if ( !object ) return err_ok;
		_RflxClass* _class = _handle2Class( object->_dynamicClass() );
		if ( !_class || !_class->methods.destroyInstance ) {
			return err_failed;
		}
		return _class->methods.destroyInstance( object );
	}

	ErrorCode getObjectClass( RflxObject* object, ClassInfo* info )
	{
		return getClassInfo( object->_dynamicClass(), info );
	}

	bool isInstanceOf( const RflxObject* object, Klass klass )
	{
		RFLX_DASSERT( klass && object );
		_RflxClass* thisClass = _handle2Class( object->_dynamicClass() );
		_RflxClass* destClass = _handle2Class( klass );
		if ( thisClass->id >= destClass->id && thisClass->id < destClass->endId ) {
			return true;
		}
		if ( thisClass == destClass ) {
			return true;
		}
		return thisClass->parentsLookup.end() != thisClass->parentsLookup.find( destClass );
	}

	bool isInstanceOfEx( const RflxObject* object, Klass klass, size_t* offset )
	{		
		RFLX_DASSERT( klass && object );
		_RflxClass* thisClass = _handle2Class( object->_dynamicClass() );
		_RflxClass* destClass = _handle2Class( klass );
		if ( thisClass == destClass ) {
			if ( offset ) {
				*offset = 0;
			}
			return true;
		}	
		if ( thisClass->id < destClass->id || thisClass->id >= destClass->endId ) {
			return false;
		}
		std::set< _ParentNode >::const_iterator it = thisClass->parentsLookup.find( destClass );
		if ( it != thisClass->parentsLookup.end() ) {
			if ( offset ) {
				*offset = it->offset;
			}
			return true;
		}
		return false;
	}

	bool isDerivesFrom( Klass klass, Klass base )
	{
		RFLX_DASSERT( klass && base );
		if ( klass == base ) {
			return false;
		}
		_RflxClass* thisClass = _handle2Class( klass );
		_RflxClass* destClass = _handle2Class( base );
		if ( thisClass->id >= destClass->id && thisClass->id < destClass->endId ) {
			return true;
		}
		return thisClass->parentsLookup.end() != thisClass->parentsLookup.find( destClass );
	}

	bool isDerivesFromEx( Klass klass, Klass base, size_t* offset )
	{
		RFLX_DASSERT( klass && base );
		_RflxClass* thisClass = _handle2Class( klass );
		_RflxClass* destClass = _handle2Class( base );
		if ( thisClass == destClass ) {
			if ( offset ) {
				*offset = 0;
			}
			return false;
		}
		if ( thisClass->id < destClass->id || thisClass->id >= destClass->endId ) {
			return false;
		}
		std::set< _ParentNode >::const_iterator it = thisClass->parentsLookup.find( destClass );
		if ( it != thisClass->parentsLookup.end() ) {
			if ( offset ) {
				*offset = it->offset;
			}
			return true;
		}
		return false;
	}

	bool isKindOf( Klass klass, Klass base )
	{
		RFLX_DASSERT( klass && base );
		_RflxClass* thisClass = _handle2Class( klass );
		_RflxClass* destClass = _handle2Class( base );
		if ( thisClass == destClass ) {
			return true;
		}
		return thisClass->parentsLookup.end() != thisClass->parentsLookup.find( destClass );
	}

	bool isKindOfEx( Klass klass, Klass base, size_t* offset )
	{
		RFLX_DASSERT( klass && base );
		_RflxClass* thisClass = _handle2Class( klass );
		_RflxClass* destClass = _handle2Class( base );
		if ( klass == base ) {
			if ( offset ) {
				*offset = 0;
			}
			return true;
		}
		if ( thisClass->id < destClass->id || thisClass->id >= destClass->endId ) {
			return false;
		}
		std::set< _ParentNode >::const_iterator it = thisClass->parentsLookup.find( destClass );
		if ( it != thisClass->parentsLookup.end() ) {
			if ( offset ) {
				*offset = it->offset;
			}
			return true;
		}
		return false;
	}

	bool isPolymorphic( Klass klass )
	{
		RFLX_DASSERT( klass );
		_RflxClass* _class = _handle2Class( klass );
		return ( _class->flags & cif_polymorphic ) != 0;
	}

	const void* dynamicCast( Klass src, const void* object, Klass dst )
	{
		return dynamicCast( src, (void*)object, dst );
	}

	void* dynamicCast( Klass src, void* object, Klass dst )
	{		
		RFLX_DASSERT( src && dst );
		if ( !object ) {
			return NULL;
		}
		_RflxClass* thisClass = _handle2Class( src );
		_RflxClass* destClass = _handle2Class( dst );

		if ( thisClass->base2OuterOffset == destClass->base2OuterOffset ) {
			if ( thisClass->id >= destClass->id && thisClass->id < destClass->endId )
			{
				// offset to outer class is the same, so we can cast it directly
				return object;
			}
		}

		if ( src == dst ) {
			return object;
		}
		size_t offset = 0;
		if ( isKindOfEx( src, dst, &offset ) ) {
			return ( char* )object + offset;
		}
		return NULL;
	}

	static inline void* cast2Base( Klass src, void* object )
	{
		RFLX_DASSERT( src );
		_RflxClass* _class = _handle2Class( src );
		return (char*)object + _class->baseOffset;
	}

	static inline const void* cast2Base( Klass src, const void* object )
	{
		return cast2Base( src, (void*)object );
	}

	void* getBaseObject( Klass src, void* object )
	{
		return cast2Base( src, object );
	}

	const void* getBaseObject( Klass src, const void* object )
	{
		return cast2Base( src, object );
	}

	const PropDef* getClassPropDef( Klass klass, unsigned int* count )
	{
		RFLX_DASSERT( klass );
		_RflxClass* _class = _handle2Class( klass );
		if ( count ) {
			*count = _class->propCount;
		}
		return _class->propDefs;
	}

	unsigned int getClassPropertyId( Klass klass, const char* name )
	{
		const PropDef* it = getClassProperty( klass, name );
		if ( it ) {
			return it->id;
		} else {
			return RFLX_INVLAID_PROPID;
		}
	}

	const PropDef* getClassProperty( Klass klass, const char* name )
	{
		RFLX_DASSERT( klass && name && name[0] );
		const _RflxClass* _class = _handle2Class( klass );
		PropDef target;
		target.name = name;
		PropDefSorter func;
		std::vector< const PropDef* >::const_iterator it = std::lower_bound( _class->sortedPropDefs.begin(), _class->sortedPropDefs.end(), &target, func );
		if ( it != _class->sortedPropDefs.end() && !func( &target, *it ) )
			return *it;
		else
			return NULL;
	}

	const PropDef* getClassPropertyDefById( Klass klass, unsigned int id )
	{
		RFLX_DASSERT( klass );
		const _RflxClass* _class = _handle2Class( klass );
		return id <= _class->propCount ? _class->propDefs + id : NULL;
	}

	const PropDef* getClassPropertyDefByPosition( const PropPos* pos )
	{	
		if ( pos ) {
			const _RflxClass* _class = _handle2Class( pos->owner );
			return _class->propDefs + pos->id;
		}
		return NULL;
	}

	static const PropPos* _getClassPropertyPosition( Klass klass, const char* name, Klass owner,
											  size_t* offset, bool recur )
	{
		static PropPos out;
		out.clear();
		if ( !owner || owner == klass ) {
			const PropDef* def = getClassProperty( klass, name );
			if ( def ) {
				out.id = def->id;
				out.owner = klass;
				return &out;
			}
		}
		if ( recur ) {
			const _RflxClass* _class = _handle2Class( klass );
			RFLX_DASSERT( _class );
			if ( _class->parent ) {
				*offset += _class->baseOffset;
				const PropPos* pos = _getClassPropertyPosition( _class2Handle( _class->parent ),
					name, owner, offset, recur );
				if ( pos ) {
					out.offset = *offset;
					return pos;
				}
				*offset -= _class->baseOffset;
			}
		}
		return NULL;
	}

	const PropPos* getClassPropertyPosition( Klass klass, const char* name, Klass owner, bool recur )
	{
		size_t offset = 0;
		return _getClassPropertyPosition( klass, name, owner, &offset, recur );
	}

	const PropDef* getClassPropertyEx( Klass klass, const char* fullPath )
	{
		RFLX_DASSERT( klass && fullPath && fullPath[0] );
		char path[256];
		strncpy( path, fullPath, 256 );
		char delims[] = ".";
		char* result = NULL;
		result = strtok( path, delims );
		while ( result != NULL ) {
			const PropPos* pos = getClassPropertyPosition( klass, result );
			result = pos ? strtok( NULL, delims ) : NULL;
			if ( pos ) {
				const PropDef* def = getClassPropertyDefByPosition( pos );
				if ( !result ) {
					return def;
				}
				unsigned int propCount = 0;
				const PropDef* defs = NULL;
				if ( def->getDefs ) {
					defs = def->getDefs( &klass, &propCount, NULL );
				}
				if ( !klass || !propCount ) {
					result = NULL;
				}
			}
		}
		return NULL;
	}

	static ErrorCode _forObjectEachProperty( void* object,
		const _RflxClass* klass,
		void (*func)( const PropDef* def, void* object, void* extra ),
		void* extra )
	{
		ErrorCode ret = err_ok;
		if ( !object || !klass || !func ) {
			return err_failed;
		}
		for ( unsigned int i = 0; i < klass->propCount; ++i ) {
			func( klass->propDefs + i, object, extra );
		}
		if ( klass->parent ) {
			ret = _forObjectEachProperty( cast2Base( _class2Handle( klass ), object ), klass->parent, func, extra );
			if ( ret == err_ok ) {
				return ret;
			}
		}
		return ret;
	}

	ErrorCode forBaseEachProperty( Klass klass, void* mbase,
									 void (*func)( const PropDef* def, void* object, void* extra ),
									 void* extra )
	{
		const _RflxClass* _class = _handle2Class( klass );
		if ( !_class || !func || !mbase ) {
			return err_failed;
		}
		for ( unsigned int i = 0; i < _class->propCount; ++i ) {
			func( _class->propDefs + i, mbase, extra );
		}
		ErrorCode ret = err_ok;
		if ( _class->parent ) {
			ret = _forObjectEachProperty( cast2Base( klass, mbase ), _class->parent, func, extra );
		}
		return ret;
	}

	ErrorCode forObjectEachProperty( RflxObject* object,
									 void (*func)( const PropDef* def, void* object, void* extra ),
									 void* extra )
	{
		RFLX_DASSERT( object );
		return forBaseEachProperty( object->_dynamicClass(), object->_mbase(), func, extra );
	}

	ErrorCode forBaseEachPropertyEx( Klass klass, void* mbase,
		bool ( *filter )( Klass klass ),
		void ( *func )( const PropDef* def, void* outerObject, void* extra ),
		void* extra )
	{
		ErrorCode ret = err_ok;
		const _RflxClass* _class = _handle2Class( klass );
		if ( !_class || !func ) return err_failed;
		if ( filter && !filter( _class2Handle( _class ) ) ) {
			for ( unsigned int i = 0; i < _class->propCount && ret == err_ok; ++i ) {
				func( _class->propDefs + i, mbase, extra );
			}
		}
		if ( _class->parent ) {
			ret = _forObjectEachProperty( cast2Base( klass, mbase ), _class->parent, func, extra );
		}
		return ret;
	}

	ErrorCode forObjectEachPropertyEx( RflxObject* object,
		bool ( *filter )( Klass klass ),
		void ( *func )( const PropDef* def, void* outerObject, void* extra ),
		void* extra )
	{
		RFLX_DASSERT( object );
		return forBaseEachPropertyEx( object->_dynamicClass(), object->_mbase(), filter, func, extra );
	}

	const PropDef* getObjectPropertyDef( const RflxObject* object,
										 const char* name, PropPos* pos, Klass owner, bool recur )
	{
		const PropPos* p = getClassPropertyPosition( object->_dynamicClass(), name, owner, recur );
		if ( p && pos ) {
			*pos = *p;
		}
		if ( p ) {
			const _RflxClass* _class = _handle2Class( p->owner );
			return _class->propDefs + p->id;
		}
		return NULL;
	}

	const PropPos* getObjectPropertyPosition( const RflxObject* object, const char* name, Klass owner, bool recur )
	{
		const PropPos* pos = getClassPropertyPosition( object->_dynamicClass(), name, owner, recur );
		if ( pos ) {
			( const_cast< PropPos* >( pos ) )->outer = object->_dynamicClass();
		}
		return pos;
	}

	ErrorCode setBaseProperty( void* mbase, const PropPos* pos, const ValueData* value, unsigned int index )
	{
		RFLX_DASSERT( mbase && pos && value );
		const _RflxClass* _class = _handle2Class( pos->owner );
		const PropDef* def = _class->propDefs + pos->id;
		void* mowner = (char*)mbase + pos->offset;
		if ( def->set ) {
			return def->set( def, index, mowner, value );
		}
		return err_failed;
	}

	ErrorCode setObjectProperty( RflxObject* object, const PropPos* pos, const ValueData* value, unsigned int index )
	{
		RFLX_DASSERT( object && pos && value );
		return setBaseProperty( object->_mbase(), pos, value, index );
	}

	ErrorCode getBaseProperty( const void* mbase, const PropPos* pos, ValueData* value, unsigned int index )
	{
		RFLX_DASSERT( mbase && pos && value );
		const _RflxClass* _class = _handle2Class( pos->owner );
		const PropDef* def = _class->propDefs + pos->id;
		if ( def->get ) {
			void* mowner = (char*)mbase + pos->offset;
			return def->get( def, index, mowner, value );
		}
		else return err_failed;
	}

	ErrorCode getObjectProperty( const RflxObject* object, const PropPos* pos, ValueData* value, unsigned int index )
	{
		RFLX_DASSERT( object && pos && value );
		return getBaseProperty( object->_mbase(), pos, value, index );
	}

	ErrorCode setBasePropertyByName( Klass klass, void* mbase, const ValueData* value, const char* name, unsigned int index, Klass owner, bool recur )
	{
		const PropPos* pos = getClassPropertyPosition( klass, name, owner, recur );
		if ( !pos ) {
			return err_property_not_found;
		}
		return setBaseProperty( mbase, pos, value, index );
	}

	ErrorCode setObjectPropertyByName( RflxObject* object, const ValueData* value, const char* name, unsigned int index, Klass owner, bool recur )
	{
		const PropPos* pos = getObjectPropertyPosition( object, name, owner, recur );
		if ( !pos ) {
			return err_property_not_found;
		}
		return setObjectProperty( object, pos, value, index );
	}
	
	ErrorCode getBasePropertyByName( Klass klass, const void* mbase, ValueData* value, const char* name, unsigned int index, Klass owner, bool recur )
	{
		const PropPos* pos = getClassPropertyPosition( klass, name, owner, recur );
		if ( !pos ) {
			return err_property_not_found;
		}
		return getBaseProperty( mbase, pos, value, index );
	}

	ErrorCode getObjectPropertyByName( const RflxObject* object, ValueData* value, const char* name, unsigned int index, Klass owner, bool recur )
	{
		const PropPos* pos = getObjectPropertyPosition( object, name, owner, recur );
		if ( !pos ) {
			return err_property_not_found;
		}
		return getObjectProperty( object, pos, value, index );
	}

		
	ErrorCode getBasePropertyRef( Klass klass, const void* mbase, const PropPos* pos, void** ref )
	{
		RFLX_DASSERT( klass && pos && ref );
		const _RflxClass* _class = _handle2Class( klass );
		const PropDef* def = _class->propDefs + pos->id;
		*ref = (char*)mbase + def->offset + pos->offset;
		return err_ok;
	}

	ErrorCode getObjectPropertyRef( const RflxObject* object, const PropPos* pos, void** ref )
	{
		RFLX_DASSERT( object && pos && ref );
		const _RflxClass* _class = _handle2Class( object->_dynamicClass() );
		const PropDef* def = _class->propDefs + pos->id;
		*ref = (char*)object->_mbase() + def->offset + pos->offset;
		return err_ok;
	}
		
	ErrorCode getBasePropertyRefByName( Klass klass, const void* mbase, void** ref, const char* name, Klass owner, bool recur )
	{
		const PropPos* pos = getClassPropertyPosition( klass, name, owner, recur );
		if ( !pos ) {
			return err_property_not_found;
		}
		return getBasePropertyRef( klass, mbase, pos, ref );
	}

	ErrorCode getObjectPropertyRefByName( RflxObject* object, void** ref, const char* name, Klass owner, bool recur )
	{
		const PropPos* pos = getObjectPropertyPosition( object, name, owner, recur );
		if ( !pos ) { 
			return err_property_not_found;
		}
		return getObjectPropertyRef( object, pos, ref );
	}

	ErrorCode setObjectPropertiesDefault( RflxObject* object, Klass target, bool recur )
	{
		ErrorCode ret = err_ok;
		RFLX_DASSERT( object );
		void* mobj = target ? dynamicCast( object->_dynamicClass(), object->_mbase(), target ) : object->_mbase();
		if ( !mobj ) {
			return err_failed;
		}
		if ( !target ) {
			target = object->_dynamicClass();
		}
		RFLX_DASSERT( target );
		do {
			const _RflxClass* _class = _handle2Class( target );
			const PropDef* props = _class->propDefs;
			for ( unsigned int i = 0; i < _class->propCount && ret == err_ok; ++i ) {
				if ( props[i].defaultVal ) {
					ret = props[i].set( props + i, 0, mobj, props[i].defaultVal );
				}
			}
			if ( recur ) {
				target = getBaseClass( target );
				mobj = target ? cast2Base( target, mobj ) : NULL;
			}
		} while ( target && recur );
		return ret;
	}

	ErrorCode setBasePropertiesDefault( void* mbase, Klass klass, Klass target, bool recur )
	{
		ErrorCode ret = err_ok;
		RFLX_DASSERT( mbase && klass );
		void* mobj = target ? dynamicCast( klass, mbase, target ) : mbase;
		if ( !mobj ) {
			return err_failed;
		}
		if ( !target ) {
			target = klass;
		}
		RFLX_DASSERT( target );
		do {
			const _RflxClass* _class = _handle2Class( target );
			const PropDef* props = _class->propDefs;
			for ( unsigned int i = 0; i < _class->propCount && ret == err_ok; ++i ) {
				const PropDef& prop = props[i];
				if ( prop.defaultVal ) {
					ret = prop.set( props + i, 0, mobj, props[i].defaultVal );
				} else {
					if ( prop.kind == rflx::vt_scalar ) {
						// this member is not a pointer so we know the exactly the object is
						// via consomHandler::klass()
						if ( prop.traits_bits & rflx::ptf_rflxable ) {
							if ( ( prop.traits_bits & rflx::ptf_pointer ) == 0 ) {
								if ( prop.customHandler->klass ) {
									// get the object memory
									void* m = (char*)mobj + prop.offset;
									// object klass
									Klass klass = prop.customHandler->klass();
									setBasePropertiesDefault( m, klass, NULL, recur );
								}
							}
						}
					}
				}
			}
			if ( recur ) {
				target = getBaseClass( target );
				mobj = target ? cast2Base( target, mobj ) : NULL;
			}
		} while ( target && recur );
		return ret;
	}

	const Klass* getFirstChildren( Klass klass )
	{
		const _RflxClass* _class = _handle2Class( klass );
		if ( _class->children.empty() ) {
			return NULL;
		}
		return ( const Klass* )&_class->children[0];
	}

	unsigned int getChildNum( Klass klass )
	{
		RFLX_DASSERT( klass );
		const _RflxClass* _class = _handle2Class( klass );
		return ( unsigned int )_class->children.size();
	}

	unsigned int getClassInstanceCount( Klass klass )
	{
		RFLX_DASSERT( klass );
		const _RflxClass* _class = _handle2Class( klass );
		return ( unsigned int )_class->instanceCount;
	}

	const char* removeNamespace( const char* name )
	{
		const char* p = name;
		int len = (int)strlen( name );
		for ( int i = len - 1; i > 0; --i ) {
			if ( p[i] == ':' && p[i-1] == ':' ) {
				return p + i + 1;
			}
		}
		return name;
	}

	EnumInfo::EnumInfo( const char* _name, const EnumValue* _data, unsigned int _count ) : name( _name ), count( _count )
	{
		RFLX_DASSERT( count != 0 && "Enum item count must not be 0!" );
		data = new EnumValue[ count ];
		// copy to heap
		for ( unsigned int i = 0; i < count; ++i ) {
			data[ i ] = _data[ i ];
			data[ i ].value = _data[ i ].value;
			data[ i ].name = NULL;
			data[ i ].desc = NULL;
			if ( _data[ i ].name ) {
				data[ i ].name = ( const char* )RFLX_MALLOC( strlen( _data[ i ].name ) + 1 );
				strcpy( ( char* )data[ i ].name, _data[ i ].name );
			}
			if ( _data[ i ].desc ) {
				data[ i ].desc = ( const char* )RFLX_MALLOC( strlen( _data[ i ].desc ) + 1 );
				strcpy( ( char* )data[ i ].desc, _data[ i ].desc );
			}
		}
		context = (Ktex)&_getContext();
		RFLX_DASSERT( context );
	}

	EnumInfo::~EnumInfo()
	{
		for ( unsigned int i = 0; i < count; ++i ) {
			RFLX_FREE( ( void* )data[ i ].name );
			RFLX_FREE( ( void* )data[ i ].desc );
		}
		delete [] data;
	}

	bool EnumInfo::getValue( const char* name, long* value, unsigned int* index ) const
	{
		for ( unsigned int i = 0; i < count; ++i ) {
			if ( strcmp( name, data[ i ].name ) == 0 ) {
				if ( value ) {
					*value = data[ i ].value;
				}
				if ( index ) { 
					*index = i;
				}
				return true;
			}
		}
		return false;
	}

	unsigned int EnumInfo::getValueIndex( long value ) const
	{
		for ( unsigned int i = 0; i < count; ++i ) {
			if ( data[ i ].value == value ) {
				return i;
			}
		}
		return (unsigned int)-1;
	}

	const EnumValue* EnumInfo::findItem( const char* name ) const
	{
		const char* _name = strstr( name, "::" );
		if ( _name ) {
			name = _name + 2;
		}
		for ( unsigned int i = 0; i < count; ++i ) {
			if ( strcmp( name, data[ i ].name ) == 0 ) {
				return data + i;
			}
		}
		return NULL;
	}

	const EnumValue* EnumInfo::findItemByValue( long value ) const
	{
		for ( unsigned int i = 0; i < count; ++i ) {
			if ( value == data[ i ].value )
				return data + i;
		}
		return NULL;
	}

	const EnumValue* EnumInfo::getItemByIndex( unsigned int index ) const
	{
		if ( index >= count ) {
			return NULL;
		}
		return data + index;
	}

	const EnumInfo* findEnumInfo( const char* name, Ktex ktex )
	{
		if ( !name || name[ 0 ] == 0 ) return NULL;
		std::string _name( name );
		const Context& context = ktex ? *(const Context*)ktex : _getContext();
		EnumBank::const_iterator it = context.enumBank.find( _name );
		if ( context.enumBank.end() != it ) {
			return it->second;
		} else {
			return NULL;
		}
	}

	const EnumInfo*	findEnumInfoByItemName( const char* name, long* value, Ktex ktex )
	{
		if ( !name || name[ 0 ] == 0 ) return NULL;
		const char* enumName = strstr( name, "::" );
		const char* itemName = enumName ? enumName + 2 : name;
		const EnumInfo* enumInfo = NULL;
		const Context& context = ktex ? *(const Context*)ktex : _getContext();
		if ( enumName && itemName - name > 2 ) {
			std::string _enumName( name, enumName - name );
			EnumBank::const_iterator it = context.enumBank.find( _enumName );
			if ( context.enumBank.end() != it ) {
				enumInfo = it->second;
				const EnumValue* enumValue = enumInfo->findItem( itemName );
				if ( enumValue && value ) {
					*value = enumValue->value;
				}
				return enumInfo;
			}
		} else {
			EnumBank::const_iterator it = context.enumBank.begin();
			while ( context.enumBank.end() != it ) {
				enumInfo = it->second;
				const EnumValue* enumValue = enumInfo->findItem( itemName );
				if ( enumValue ) {
					if ( value ) {
						*value = enumValue->value;
					}
					return enumInfo;
				}
				++it;
			}
		}
		return NULL;
	}

	const PropPosEx* getClassPropertyPositionEx( Klass klass, const void* object, const char* fullPath )
	{
		static PropPosEx ret;
		ret.clear();
		memset( &ret, 0, sizeof( ret ) );
		bool succeeded = false;
		RFLX_DASSERT( object && fullPath && fullPath[0] );
		char path[256];
		strncpy( path, fullPath, 256 );
		_strtrim( path );
		char delims[] = ".";
		char* result = NULL;
		const char* mbase = (const char*)object;
		Klass curKlass = klass;
		result = strtok( path, delims );
		while ( result != NULL ) {
			// parse array index
			char* cur = result;
			char* lb = NULL;
			int idx = 0;
			while ( *cur ) {
				if ( !lb && *cur == '[' )
					lb = cur;
				if ( lb && *cur == ']' ) {
					*lb = 0;
					*cur = 0;
					idx = (int)atoi( lb + 1 );
					break;
				}
				++cur;
			}
			const PropPos* pos = getClassPropertyPosition( curKlass, result );
			result = pos ? strtok( NULL, delims ) : NULL;
			if ( pos ) {
				ret = *pos;
				PropPosEx& cur = ret;
				cur.index = 0;
				cur.mbase = const_cast< char* >( mbase );
				// get current property def
				const PropDef* def = getClassPropertyDefByPosition( pos );
				if ( def->kind == vt_c_array ) {
					size_t arraySize = 0;
					operateFunctionWrapper( def->op, def, NULL, op_size, &arraySize );
					cur.index = idx;
					RFLX_DASSERT( idx >= 0 && idx < (int)arraySize && "error: c array out of range!" );
				}
				else if ( def->kind > vt_container_begin && def->kind < vt_container_end ) {
					RFLX_DASSERT( idx >= 0 && "error: container index can not be negative!" );
					cur.index = idx;
				}
				if ( !result ) {
					succeeded = true;
					break;
				}
				unsigned int propCount = 0;
				const PropDef* defs = NULL;
				// if it's a dynamic object, get the outer class for next property finding.
				// else if klass will be a static class for next.
				Klass klass = NULL;
				if ( def->getDefs ) {
					defs = def->getDefs( &klass, &propCount, NULL );
				}
				if ( !klass || !propCount ) {
					result = NULL;
				}
				if ( def->traits_bits & ( ptf_pointer | ptf_ref ) ) {
					// pointer address
					cur.mbase = *(char**)def->get_ref( def, cur.index, cur.mbase + cur.offset );
					if ( def->traits_bits & ptf_rflxobject ) {
						// get the pointer object static class type
						if ( checkClassFlag( _internal::_getClassFlags( klass ), cif_polymorphic ) )
							cur.mbase += _internal::_getOuter2BaseOffset( klass );
						RflxObject* element = reinterpret_cast< RflxObject* >( cur.mbase );
						cur.mbase = (char*)element->_mbase();
						mbase = cur.mbase;
						curKlass = element->_dynamicClass();
					} else if ( def->traits_bits & ptf_rflxable ) {
						curKlass = klass;
						mbase = cur.mbase;
					} else {
						return NULL;
					}
				} else {
					curKlass = klass;
					mbase = (char*)def->get_ref( def, cur.index, cur.mbase + cur.offset );
				}
			}
		}
		return succeeded ? &ret : NULL;
	}

	ErrorCode setBasePropertyByNameEx( Klass klass, void* mbase, const ValueData* value, const char* fullpath, unsigned int index )
	{
		const PropPosEx* pos = getClassPropertyPositionEx( klass, mbase, fullpath );
		if ( !pos || !pos->mbase ) {
			return err_property_not_found;
		}
		const PropDef* def = NULL;
		def = getClassPropertyDefById( pos->owner, pos->id );
		if ( !def ) {
			return err_property_not_found;
		}
		return def->set( def, index + pos->index, pos->mbase, value );
	}

	ErrorCode setObjectPropertyByNameEx( RflxObject* object, const ValueData* value, const char* fullpath, unsigned int index )
	{
		return setBasePropertyByNameEx( object->_dynamicClass(), object->_mbase(), value, fullpath, index );
	}

	ErrorCode getBasePropertyByNameEx( Klass klass, const void* mbase, ValueData* value, const char* fullpath, unsigned int index )
	{	
		const PropPosEx* pos = getClassPropertyPositionEx( klass, mbase, fullpath );
		if ( !pos || !pos->mbase ) {
			return err_property_not_found;
		}
		const PropDef* def = NULL;
		def = getClassPropertyDefById( pos->owner, pos->id );
		if ( !def ) {
			return err_property_not_found;
		}
		ErrorCode ret = def->get( def, index + pos->index, pos->mbase, value );
		if ( ret == err_ok ) {
		}
		return ret;
	}

	ErrorCode getObjectPropertyByNameEx( const RflxObject* object, ValueData* value, const char* fullpath, unsigned int index )
	{	
		return getBasePropertyByNameEx( object->_dynamicClass(), object->_mbase(), value, fullpath, index );
	}

	ErrorCode operateFunctionWrapper( pfn_operator func, const PropDef* def, void* userObject, PropOp op, ... )
	{			
		va_list va;
		va_start( va, op );
		return func( def, userObject, op, va );
	}

	ErrorCode operateObjectProperty( RflxObject* object, const PropPos* pos, PropOp opType, ... )
	{
		RFLX_DASSERT( object && pos );
		const _RflxClass* _class = _handle2Class( pos->owner );
		const PropDef* def = _class->propDefs + pos->id;
		if ( def && def->op ) {
			va_list va;
			va_start( va, opType );
			return def->op( def, (char*)object->_mbase() + pos->offset, opType, va );
		}
		else return err_failed;
	}

	ErrorCode operateObjectPropertyEx( const PropPosEx* pos, PropOp opType, ... )
	{
		if ( !pos || pos->mbase ) {
			return err_invalid_params;
		}
		const PropDef* def = NULL;
		def = getClassPropertyDefById( pos->owner, pos->id );
		if ( def && def->op ) {
			va_list va;
			va_start( va, opType );
			return def->op( def, pos->mbase, opType, va );
		}
		else return err_failed;
	}

	ErrorCode operateObjectPropertyByName( RflxObject* object, const char* name, PropOp opType, ... )
	{
		const PropPos* pos = getObjectPropertyPosition( object, name );
		const _RflxClass* _class = _handle2Class( pos->owner );
		const PropDef* def = _class->propDefs + pos->id;
		if ( def && def->op ) {
			va_list va;
			va_start( va, opType );
			return def->op( def, (char*)object->_mbase() + pos->offset, opType, va );
		}
		else return err_failed;
	}

	ErrorCode operateObjectPropertyByNameEx( RflxObject* object, const char* fullpath, PropOp opType, ... )
	{
		const PropPosEx* pos = getClassPropertyPositionEx( object->_dynamicClass(), object->_mbase(), fullpath );
		if ( !pos || !pos->mbase ) return err_property_not_found;
		const PropDef* def = NULL;
		def = getClassPropertyDefById( pos->owner, pos->id );
		if ( def && def->op ) {
			va_list va;
			va_start( va, opType );
			return def->op( def, pos->mbase, opType, va );
		}
		else return err_failed;
	}

	ErrorCode setBasePropertyByNameFromString( Klass klass, const void* mbase, const char* name, const char* value, unsigned int index, Klass owner, bool recur )
	{
		const PropDef* def = NULL;
		RFLX_DASSERT( klass && mbase && value && name );
		const PropPos* pos = getClassPropertyPosition( klass, name, owner, recur );
		if ( !pos ) { 
			return err_property_not_found;
		}
		def = getClassPropertyDefById( pos->owner, pos->id );
		if ( !def ) {
			return err_property_not_found;
		}
		if ( def->set ) {
			ValueData data;
			data.fromString( def->type, value, def->customHandler, getContext( klass ) );
			if ( data.isNil() ) {
				return err_failed;
			}
			return def->set( def, index, (char*)mbase + pos->offset, &data );	
		}
		return err_ok;
	}

	ErrorCode setObjectPropertyByNameFromString( RflxObject* object, const char* name, const char* value, unsigned int index, Klass owner, bool recur )
	{
		return setBasePropertyByNameFromString( object->_dynamicClass(), object->_mbase(), name, value, index, owner, recur );
	}

	void messageEnable( bool enable )
	{
		_messageEnabled = enable;
	}

	bool isMessageEnabled( )
	{
		return _messageEnabled;
	}

	bool isUniqueName( const char* name )
	{
		RFLX_DASSERT( name && name[0] );
		static std::set< std::string > _uniqueNames;
		if ( _uniqueNames.find( name ) != _uniqueNames.end() ) {
			return false;
		}
		bool ret = _uniqueNames.insert( name ).second;
		RFLX_DASSERT( ret );
		return ret;
	}

	MessageEnableGuard::MessageEnableGuard( bool enable ) : old( _messageEnabled ) { _messageEnabled = enable; }

	MessageEnableGuard::~MessageEnableGuard( ) { _messageEnabled = old; }

	ErrorCode sendMessage( Klass klass, void* mbase, Message* msg )
	{
		if ( !_messageEnabled ) {
			return err_ok;
		}
		RFLX_DASSERT( klass && mbase && msg );
		ErrorCode hr = err_ok;
		_RflxClass* _class = _handle2Class( klass );
		void* obj = mbase;
		while ( _class ) {
			if ( _class->methods.objectProc ) {
				hr = _class->methods.objectProc( msg, obj );
				if ( msg->handled || hr == err_message_consumed ) {
					return hr;
				}
			}
			if ( _class->parent ) {
				obj = cast2Base( _class2Handle( _class ), obj );
			}
			_class = _class->parent;
		}
		return hr;
	}

	ErrorCode sendMessage( RflxObject* object, Message* msg )
	{
		if ( !_messageEnabled ) {
			return err_ok;
		}
		RFLX_DASSERT( object && msg );
		ErrorCode hr = err_ok;
		_RflxClass* _class = _handle2Class( object->_dynamicClass() );
		void* obj = object->_mbase();
		while ( _class ) {
			if ( _class->methods.objectProc ) {
				hr = _class->methods.objectProc( msg, obj );
				if ( msg->handled || hr == err_message_consumed ) {
					break;
				}
			}
			if ( _class->parent ) {
				obj = cast2Base( _class2Handle( _class ), obj );
			}
			_class = _class->parent;
		}
		return hr;
	}

	const MsgFunc* findMessageHandler( Klass klass, MessageId id )
	{
		RFLX_DASSERT( klass );
		const _RflxClass* _class = _handle2Class( klass );
		if ( _class->msgMap ) {
			MsgMapSorter func;
			MsgFunc target = { id };
			MsgFunc* end = _class->msgMap + _class->msgMapSize;
			const MsgFunc* it = std::lower_bound( _class->msgMap, end, target, func );
			if ( it != end && !func( target, *it ) )
				return it;
		}
		return NULL;
	}

	const MsgFuncChain*	findMessageHandlerChain( Klass klass, MessageId id )
	{
		RFLX_DASSERT( klass );
		const _RflxClass* _class = _handle2Class( klass );
		if ( _class->msgTable ) {
			return _class->msgTable->find( id );
		}
		return NULL;
	}
}

