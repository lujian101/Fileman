#include "Rflext.h"
#include "datatree/dt_core.h"
#include "datatree/dt_cpp_helper.hpp"
#include "../tinyXml/tinyXml.h"
#include <fstream>
#include <algorithm>

using namespace rflx;
using namespace rflext;

#ifndef _stricmp
	#ifdef _MSC_VER
		#define _stricmp stricmp
	#else
		#define _stricmp strcasecmp
	#endif
#endif

#ifdef _MSC_VER
	#pragma warning( disable : 4189 )
	#pragma warning( disable : 4996 )
	#pragma warning( disable : 4100 )
#endif

RFLX_IMP_AUTO_REGISTER( TRflxObject )

namespace rflext
{
	PropDefExt::PropDefExt()
	{
		rflx::PropDef* base = this;
		memset( base, 0, sizeof( *base ) );
	}
	
	void PropDefExt::bind( bool resetValue )
	{
		name = nameHolder.c_str();
		groupName = groupNameHolder.c_str();
		description = descriptionHolder.c_str();
		defaultVal = &defaultValHolder;
		if ( resetValue || valueHolder.isNil() ) {
			valueHolder = defaultValHolder;
		}
		if ( type == vdt_nil && !valueHolder.isNil() ) {
			type = valueHolder.type;
		}
	}

	PropDefExt::PropDefExt( const PropDefExt& o )
	{
		rflx::PropDef* base = this;
		memcpy( base, (rflx::PropDef*)&o, sizeof( *base ) );
		defaultValHolder = o.defaultValHolder;
		valueHolder = o.valueHolder;
		nameHolder = o.nameHolder;
		groupNameHolder = o.groupNameHolder;
		descriptionHolder = o.descriptionHolder;
		editorDataHolder = o.editorDataHolder;
		ownerName = o.ownerName;
		bind( false );
	}

	PropDefExt& PropDefExt::operator = ( const PropDefExt& o )
	{
		if ( &o != this ) {
			rflx::PropDef* base = this;
			memcpy( base, (rflx::PropDef*)&o, sizeof( *base ) );
			defaultValHolder = o.defaultValHolder;
			valueHolder = o.valueHolder;
			nameHolder = o.nameHolder;
			groupNameHolder = o.groupNameHolder;
			descriptionHolder = o.descriptionHolder;
			editorDataHolder = o.editorDataHolder;
			ownerName = o.ownerName;
			bind( false );
		}
		return *this;
	}

	TRflxObject::TRflxObject( bool _readonly ) : lib( NULL ), derive( NULL ), base( NULL ), readonly( _readonly )
	{
	}

	TRflxObject* TRflxObject::getOuter() const
	{
		TRflxObject* cur = derive;
		while ( cur ) {
			if ( !cur->derive )
				return cur;
			cur = cur->derive;
		}
		return cur;
	}
	
	TRflxObject::TRflxObject( const TRflxObject& o ) : lib( o.lib ), className( o.className ), baseClassName( o.baseClassName ),
		description( o.description ), derive( NULL ), base( NULL )
	{
		RFLX_DASSERT( o.derive == NULL && "Must be the outer object!" );
		PropTable_ConstIt it = o.props.begin();
		while ( it != o.props.end() ) {
			const PropDefExt* pd = it->second;
			PropDefExt* pdef = new PropDefExt( *pd );
			props.insert( std::make_pair( it->first, pdef ) );
			++it;
		}
		if ( o.base ) {
			base = o.base->createInstance( true );
			base->derive = this;
		}
	}

	TRflxObject& TRflxObject::operator = ( const TRflxObject& o )
	{
		if ( this == &o ) return *this;
		RFLX_DASSERT( o.derive == NULL && "Must be the outer object!" );
		clear();
		lib = o.lib;
		className = o.className;
		baseClassName = o.baseClassName;
		description = o.description;
		PropTable_ConstIt it = o.props.begin();
		while ( it != o.props.end() ) {
			const PropDefExt* pd = it->second;
			PropDefExt* pdef = new PropDefExt( *pd );
			pdef->bind( false );
			props.insert( std::make_pair( it->first, pdef ) );
			++it;
		}
		if ( o.base ) {
			base = o.base->createInstance( true );
			base->derive = this;
		}
		return *this;
	}
	
	TRflxObject* TRflxObject::createInstance( bool _clone ) const
	{
		TRflxObject* p = new TRflxObject( false );
		p->lib = lib;
		p->className = className;
		p->baseClassName = baseClassName;
		p->description = description;
		PropTable_ConstIt it = props.begin();
		while ( it != props.end() ) {
			const PropDefExt* pd = it->second;
			PropDefExt* pdef = new PropDefExt( *pd );
			pdef->valueHolder = pd->valueHolder;
			pdef->nameHolder = pd->nameHolder;
			pdef->groupNameHolder = pd->groupNameHolder;
			pdef->descriptionHolder = pd->descriptionHolder;
			pdef->bind( !_clone );
			p->props.insert( std::make_pair( it->first, pdef ) );
			++it;
		}
		if ( base ) {
			p->base = base->createInstance( _clone );
			p->base->derive = p;
		}
		return p;
	}

	void TRflxObject::clear()
	{
		if ( !readonly ) {
			if ( derive != NULL ) {
				if ( derive->base != NULL ) {
					derive->base = NULL;
				}
			}
			// only clear base
			if ( base ) {
				// make it as a outer class.
				base->derive = NULL;
				delete base;
				base = NULL;
			}
		}
		// do this in derive object
		className.clear();
		baseClassName.clear();
		description.clear();
		base = NULL;
		derive = NULL;
		lib = NULL;
		PropTable_It it = props.begin();
		while ( it != props.end() ) {
			delete it->second;
			++it;
		}
		props.clear();
	}

	TRflxObject::~TRflxObject()
	{
		clear();
	}

	const PropDefExt* TRflxObject::findProp( const std::string& name, bool recur ) const
	{
		return const_cast< TRflxObject* >( this )->findProp( name, recur );
	}

	PropDefExt* TRflxObject::findProp( const std::string& name, bool recur )
	{
		if ( readonly || name.empty() )
			return NULL;
		PropTable_ConstIt it = props.find( name );
		if ( it != props.end() )
			return it->second;
		if ( recur && base ) {
			return base->findProp( name );
		}
		return NULL;
	}
	
	void TRflxObject::link()
	{
		if ( readonly ) {
			if ( base == NULL && lib != NULL && !baseClassName.empty() ) {
				const TRflxObject* tbase = findObjectTemplate( lib, baseClassName.c_str() );
				if ( tbase ) {
					base = const_cast< TRflxObject* >( tbase );
					base->link();
				}
			}
		}
	}

	class RflxOTLib
	{
		typedef std::map< std::string, TRflxObject* > TObjTable;
		typedef TObjTable::iterator TObjTable_It;
		typedef TObjTable::const_iterator TObjTable_ConstIt;
	public:
		RflxOTLib( const std::string& _name ) : name( _name ){}
		~RflxOTLib()
		{
			clear();
		}
		void clear()
		{
			TObjTable::iterator it = objTable.begin();
			while ( it != objTable.end() ) {
				rflx::destroyObject( it->second );
				++it;
			}
			objTable.clear();
		}
		const TRflxObject* find( const std::string& objName )
		{
			if ( objName.empty() )
				return NULL;
			TObjTable_ConstIt it = objTable.find( objName );
			if ( it != objTable.end() )
				return it->second;
			return NULL;
		}
		
		void link()
		{
			TObjTable_It it = objTable.begin();
			while ( it != objTable.end() ) {
				TRflxObject* obj = it->second;
				obj->link();
				++it;
			}
		}

		template< class TFUN >
		int foreach( TFUN func ) {
			int count = 0;
			TObjTable_It it = objTable.begin();
			while ( it != objTable.end() ) {
				const TRflxObject* obj = it->second;
				func( obj );
				++it;
				++count;
			}
			return count;
		}

		std::string name;
		std::string version;
		TObjTable objTable;
	};

	OTLib createOTLib( const char* libName )
	{
		RFLX_DASSERT( libName && libName[0] );
		return new RflxOTLib( libName );
	}

	void destroyOTLib( OTLib lib )
	{
		delete lib;
	}

	const TRflxObject*	findObjectTemplate( OTLib lib, const char* objName )
	{
		RFLX_DASSERT( objName );
		return lib->find( objName );
	}

	bool isObjectTemplateValid( OTLib lib, const TRflxObject* obj )
	{
		rflx::Klass klass = rflx::findClass( obj->getName() );
		if ( !klass ) return false;
		rflx::Klass baseKlass = NULL;
		if ( obj->getBaseName()[0] != 0 ) {
			baseKlass = rflx::findClass( obj->getBaseName() );
			return rflx::isDerivesFrom( klass, baseKlass );
		}
		return true;
	}

	int foreachOTL( OTLib lib, void ( *fun )( const TRflxObject* ) )
	{
		return lib->foreach( fun );
	}

	static bool _createDef( const char* _name, rflx::PropDef& def )
	{
		if ( !_name || *_name == 0 ) return false;
		std::string name( _name );
		def.type = vdt_custom;
		/*if ( name == "vector2" )
			def.customHandler = rflx::ICustomDataHandler< fVector2 >::defaultHandler();
		else if ( name == "vector" || name == "vector3" )
			def.customHandler = rflx::ICustomDataHandler< fVector3 >::defaultHandler();
		else if ( name == "vector4" )
			def.customHandler = rflx::ICustomDataHandler< fVector4 >::defaultHandler();
		else if ( name == "aabb2" )
			def.customHandler = rflx::ICustomDataHandler< fAABB2 >::defaultHandler();
		else*/ {
			const rflx::EnumInfo* ei = rflx::findEnumInfo( name.c_str() );
			if ( ei ) {
				def.enumInfo = ei;
				def.type = vdt_enum;
				return true;
			}
		}
		if ( def.customHandler )
			return true;
		def.type = vdt_nil;
		std::transform( name.begin(), name.end(), name.begin(), ::tolower );
		for ( unsigned int i = 0; i < rflx::vdt_max_num; ++i ) {
			const char* name2 = rflx::_internal::_getValueDataTypeSizeNameTable()[ i ].name2;
			if ( name == rflx::_internal::_getValueDataTypeSizeNameTable()[ i ].name ||
				name2 && name == name2 ) {
				def.type = rflx::ValueDataType( i );
				return true;
			}
		}
		RFLX_WARNING( "Rflext: unknown property type : s%!\n", name.c_str() );
		return false;
	}
		
	bool loadOTLibFromXML( OTLib lib, const char* fileName )
	{
		if ( !lib || !fileName || fileName[0] == '\0' )
			return false;
		bool ret = false;
		lib->clear();
		TiXmlDocument doc( fileName );
		while ( doc.LoadFile() ) {
			TiXmlElement* root = doc.FirstChildElement( "RflxOTLib" );
			if ( !root )
				break;
			const char* name = root->Attribute( "Name" );
			const char* version = root->Attribute( "Version" );
			if ( name && lib->name.empty() ) lib->name = name;
			if ( version ) lib->version = version;
			TiXmlElement* classDef = root->FirstChildElement( "Class" );
			while ( classDef ) {
				TRflxObject* tobj = new TRflxObject( true );
				tobj->setOwner( lib );
				if ( tobj ) {
					TiXmlAttribute* attr = classDef->FirstAttribute();
					while ( attr ) {
						if ( attr->Value() && attr->Name() )
							rflx::setObjectPropertyByNameFromString( tobj, attr->Name(), attr->Value() );
						attr = attr->Next();
					}
					if ( lib->objTable.end() == lib->objTable.find( tobj->getName() ) ) {
						bool b = lib->objTable.insert( std::make_pair( std::string( tobj->getName() ), tobj ) ).second;
						if ( !b ) {
							delete tobj;
							tobj = NULL;
						}
					}
				}
				TiXmlElement* PropDef = classDef->FirstChildElement( "PropDef" );
				while ( tobj && PropDef ) {
					PropDefExt* prop = new PropDefExt;
					const char* typeName = PropDef->Attribute( "Type" );
					const char* propName = PropDef->Attribute( "Name" );
					if ( propName && _createDef( typeName, *prop ) ) {
						prop->ownerName = tobj->getName();
						prop->nameHolder = propName;
						prop->groupNameHolder = tobj->getName();
						std::string source;
						TiXmlElement* defaultVal = PropDef->FirstChildElement( "DefaultVal" );
						if ( defaultVal ) {
							if ( defaultVal->GetText() )
								source = defaultVal->GetText();
							else
								source = "";
						}
						const char* desc = PropDef->Attribute( "Description" );
						if ( desc ) {
							prop->descriptionHolder = desc;
						}	
						const char* ed = PropDef->Attribute( "Editor" );
						if ( ed ) {
							prop->editorDataHolder = ed;
						}
						if ( prop->type != rflx::vdt_nil ) {
							prop->defaultVal = &(prop->defaultValHolder);
							if ( !source.empty() && !prop->defaultValHolder.fromString( prop->type, source, prop->customHandler ) ) {
								printf( "Rflext: [%s]Invalid default value!\n", propName );
								prop->defaultVal = NULL;
							}
							prop->defaultVal = &(prop->defaultValHolder);
							prop->bind();
							bool b = tobj->getProps().insert( std::make_pair( std::string( propName ), prop ) ).second;
							if ( b )
								prop = NULL;
						}
					}
					if ( prop )
						delete prop;
					PropDef = PropDef->NextSiblingElement( "PropDef" );
				}
				classDef = classDef->NextSiblingElement( "Class" );
			}
			ret = true;
			break;
		}
		lib->link();
		return ret;
	}
}


#define RFLXEXT_QUALIFIY_TYPE_PREFIX	'@'
#define RFLXEXT_QUALIFIY_NUMBER_PREFIX	'#'
#define RFLXEXT_POINTER_NIL				"nil"
#define RFLXEXT_DTREE_FILE_TXT			"jexon"
#define RFLXEXT_DTREE_FILE_BIN			"jebin"

namespace rflext
{
	struct KlassInst {
		Klass klass;
		void* mbase;
	};
		
	template< size_t Len > 
	struct SBuf {
		enum { max_len = Len };
		char buf[ max_len ];
		size_t len;
		SBuf() : len(0) {}
	};

	typedef SBuf< 2048 > TempStrBuf;

	static std::string _getClassName( Klass klass ) {
		return std::string( 1, RFLXEXT_QUALIFIY_TYPE_PREFIX ) + rflx::getClassName( klass );
	}

	static void _on_parse_error(dt_enum_compatible_t status, const char* msg, const char* pos, size_t row, size_t col) {
		printf("Parsing error.\nError code: %d, error message: %s\nRow: %d, col: %d\nText: %s...\n", status, msg, row, col, pos);
	}

	static bool _dtTypeConv( ValueData& dst, const void* src, dt_type_t type ) {
		dst.clear();
		switch ( type ) {
		case DT_BOOL: dst = ( *( dt_bool_t* )src ) == DT_TRUE; break;
		case DT_BYTE: dst = *( signed char* )src; break;
		case DT_UBYTE: dst = *( unsigned char* )src; break;	
		case DT_SHORT: dst = *( short* )src; break;
		case DT_USHORT: dst = *( unsigned short* )src; break;
		case DT_INT: dst = *( int* )src; break;
		case DT_UINT: dst = *( unsigned int* )src; break;
		case DT_LONG: dst = *( long* )src; break;
		case DT_ULONG: dst = *( unsigned long* )src; break;
		case DT_SINGLE: dst = *( float* )src; break;
		case DT_DOUBLE: dst = *( double* )src; break;
		case DT_STRING: dst = ( const char* )src; break;
		default:;
		}
		return !dst.isNil();
	}

	enum PropSerKind {
		psk_unknown,
		psk_base,
		psk_custom,
		psk_normal_pointer,
		psk_object_pointer,
		psk_array,
		psk_map,
	};

	static PropSerKind _classifyProperty( const PropDef* def ) 
	{
		PropSerKind ret = psk_unknown;
		if ( def->kind == vt_scalar ) {
			if ( def->type == vdt_pointer ) {
				ret = psk_normal_pointer;
				if ( def->customHandler && def->customHandler->klass ) {
					Klass klass = def->customHandler->klass();
					if ( klass && rflx::isKindOf( klass, RflxObject::_class() ) )
						ret = psk_object_pointer;
				}
			} 
			else if ( def->type == vdt_custom && def->customHandler )
				ret = psk_custom;
			else
				ret = psk_base;
		} else
			ret =  def->kind == vt_map ? psk_map : psk_array;
		if ( ret == psk_map ) {
			ret = psk_unknown;
			// check map key and value type here
			ValueDataType types[] = { def->type2, def->type };
			const CustomDataHandler* handlers[] = { def->customHandler2, def->customHandler };
			const EnumInfo* enumInfo[] = { def->enumInfo2, def->enumInfo };
			int validCount = 0;
			for ( int i = 0; i < 2; ++i ) {
				bool valid = false;
				if ( types[i] == vdt_pointer ) {
					if ( handlers[i] ) {
						Klass klass = handlers[i]->klass();
						valid = klass != NULL && ( isKindOf( klass, RflxObject::_class() ) || !isPolymorphic( klass ) );
					}
				} else if ( types[i] == vdt_custom ) {
					RFLX_DASSERT( handlers[i] );
					Klass klass = handlers[i]->klass();
					// valid if it is a rflxable object or can be serialized from string
					valid = klass != NULL || ( handlers[i]->serializerFlag() & sef_from_string );
				} else if ( types[i] == vdt_enum )
					valid = enumInfo[i] != NULL;
				else {
					// base type is ok
					valid = true;
				}
				validCount = valid ? validCount + 1 : validCount;
			}
			if ( validCount == 2 )
				ret = psk_map;
		}
		return ret;
	}

	class DTValue {
		dt_value_t val;
	public:
		DTValue() : val( NULL ){}
		DTValue( const DTValue& o ) { val = o.val; const_cast< DTValue& >(o).val = NULL; }
		DTValue& operator = ( const DTValue& o ) { clear(); val = o.val; const_cast< DTValue& >(o).val = NULL; return *this; }
		~DTValue() { if ( val ) dt_destroy_value( DT_DUMMY_DATATREE, val ); }
		operator dt_value_t() const { return val; }
		dt_value_t* operator&() { return &val; }
		void release() { val = NULL; }
		void clear() { if ( val ) { dt_destroy_value( DT_DUMMY_DATATREE, val ); val = NULL; } }
	};

	struct _PropDef {
		ValueDataType type;
		const CustomDataHandler* customHandler;
	};

	enum _ContainerValueUsage {
		cvu_key,
		cvu_value,
	};

	class DTReadStack
	{
	public:
		typedef std::pair< dt_value_t, dt_value_t > Node;
		typedef std::vector< Node > stack_type;
		dt_datatree_t root;
		stack_type stack;
		DTReadStack( ) : root( NULL ) {}
		void bind( dt_datatree_t _root ) { root = _root; stack.clear(); }
		void push( const Node& p ) { stack.push_back( p ); }
		void pop() { stack.pop_back(); }
		Node& back() { return stack.back(); }
		const Node& back() const { return stack.back(); }
		bool pushObject( const char* key, size_t index )
		{
			Node& node = stack.back();
			char* str = NULL;
			switch ( dt_value_type( node.second ) )
			{
			case DT_OBJECT:
				{
					if ( !key ) return false;
					dt_object_t o = NULL;
					dt_value_data_as( &o, node.second, DT_OBJECT );
					size_t size = 0;
					dt_object_member_count( root, o, &size );
					if ( !size )
						return false;
					DTValue k;
					Node node;
					node.first = NULL;
					node.second = NULL;
					size_t idx = 0;
					if ( key ) {
						dt_create_value( root, &k, _on_parse_error, NULL, DT_STRING, key );
						dt_find_object_member_by_key( root, o, k, NULL, &idx );
						if ( idx == DT_INVALID_INDEX )
							return false;
					} else {
						idx = index;
					}
					dt_object_member_at( root, o, idx, &node.first, &node.second );
					stack.push_back( node );
					return true;
				}
				break;
			case DT_ARRAY:
				{
					dt_array_t a = NULL;
					dt_value_t val = NULL;
					dt_value_data_as( &a, node.second, DT_ARRAY );
					size_t size = 0;
					dt_array_elem_count( root, a, &size );
					if ( !size )
						return false;
					dt_array_elem_at( root, a, index, &val );
					Node node;
					node.first = NULL;
					node.second = val;
					stack.push_back( node );
				}
				break;
			}
			return true;
		}
		bool parseContainerDTValue( dt_value_t value, const _PropDef& info, ValueData& out, _ContainerValueUsage usage )
		{
			out.clear();
			dt_type_t type = dt_value_type( value );
			switch ( type )
			{
			case DT_STRING:
				{
					const char* s = ( const char* )dt_value_data( value );
					if ( s ) {
						std::string source( s );
						return out.fromString( info.type, source, info.customHandler );
					}
				}
				break;
			case DT_OBJECT:
				{
					dt_object_t o = NULL;
					size_t size = 0;
					dt_value_data_as( &o, value, DT_OBJECT );
					dt_object_member_count( root, o, &size );
					// in container, the object type value must be a single object
					// for example: map's value is a pair, pair must be put into a object
					RFLX_DASSERT( size == 1 );
					if ( o && info.customHandler ) {
						dt_value_t key = NULL, val = NULL;
						dt_object_member_at( root, o, 0, &key, &val );
						dt_type_t keyType = dt_value_type( key );
						if ( keyType != DT_STRING ) break;
						const char* className = (const char*)dt_value_data( key );
						dt_object_t o2 = NULL;
						dt_value_data_as( &o2, val, DT_OBJECT );
						push( std::make_pair( (dt_value_t)NULL, val ) );
						Klass klass = info.customHandler->klass();

						if ( klass && dt_value_data( key ) ) 
						{
							const char* outerKlassName = (const char*)dt_value_data( key );
							if ( outerKlassName && outerKlassName[ 0 ] == RFLXEXT_QUALIFIY_TYPE_PREFIX ) {
								outerKlassName += 1;
								Klass outerKlass = findClass( outerKlassName );
								if ( info.type == vdt_pointer && isKindOf( klass, RflxObject::_class() ) &&
									isKindOf( outerKlass, klass ) ) {
									RflxObject* obj = NULL;
									void* mbase = NULL;
									rflx::createObject( outerKlass, &obj );
									if ( obj ) {
										rflx::setBasePropertiesDefault( obj->_mbase(), obj->_dynamicClass(), NULL, true );
										forBaseEachProperty( obj->_dynamicClass(), obj->_mbase(), readFunc, this );
										out = (const void*)dynamicCast( obj->_dynamicClass(), obj->_mbase(), klass );
									}
								} else {
									if ( isKindOf( klass, outerKlass ) ) {
										void* obj = ( info.customHandler->create && info.customHandler->destroy ) ?
											info.customHandler->create() : NULL;
										if ( obj )
											forBaseEachProperty( klass, obj, readFunc, this );
										if ( info.type != vdt_pointer ) {
											out.assign( obj, info.customHandler );
											info.customHandler->destroy( obj );
										}
										else {
											out = (const void*)obj;
										}
									}
								}
							}
						}
						pop();
					}
				}
				break;
			case DT_ARRAY:
			case DT_NULL:
				break;
			default:
				if ( _dtTypeConv( out, dt_value_data( value ), type ) ) {
					if ( out.type != info.type )
						out = out.cast( info.type );
					return !out.isNil();
				}
			}
			return false;
		}

		bool readBase( const PropDef* def, size_t index, void* object )
		{			
			Node& node = stack.back();
			char* str = NULL;
			ValueData vdata;
			switch ( dt_value_type( node.second ) )
			{
			case DT_OBJECT:
				{
					DTValue k;
					dt_object_t o = NULL;
					dt_value_data_as( &o, node.second, DT_OBJECT );
					dt_value_t val = NULL;
					dt_create_value( root, &k, _on_parse_error, NULL, DT_STRING, def->name );
					dt_find_object_member_by_key( root, o, k, &val, NULL );
					if ( !val )
						return false;
					dt_type_t type = dt_value_type( val );
					if ( !_dtTypeConv( vdata, dt_value_data( val ), type ) )
						return false;
				}
				break;
			case DT_ARRAY:
				{
					dt_array_t a = NULL;
					dt_value_t val = NULL;
					dt_value_data_as( &a, node.second, DT_ARRAY );
					dt_array_elem_at( root, a, index, &val );
					if ( !val )
						return false;
					dt_type_t type = dt_value_type( val );
					if ( !_dtTypeConv( vdata, dt_value_data( val ), type ) )
						return false;
				}
				break;
			}
			if ( vdata.type != def->type )
				vdata = vdata.cast( def->type );
			return rflx::err_ok == def->set( def, 0, object, &vdata );
		}
		bool readCustom( const PropDef* def, size_t index, void* object )
		{
			RFLX_DASSERT( def && def->customHandler );	
			if ( !pushObject( def->name, 0 ) )
				return false;
			Klass klass = def->customHandler->klass();
			std::string name;
			if ( klass )
				name = _getClassName( klass );
			else {
				name.push_back( RFLXEXT_QUALIFIY_TYPE_PREFIX );
				name.append( def->customHandler->name() );
			}
			void* propObj = def->get_ref( def, 0, object );
			Node& node = back();
			if ( pushObject( name.c_str(), 0 ) )
			{
				Node& dataNode = back();
				switch ( dt_value_type( dataNode.second ) )
				{
				case DT_OBJECT:
					forBaseEachProperty( klass, propObj, readFunc, this );
					break;
				case DT_STRING:
					{
						const char* s = NULL;
						size_t size = 0;
						dt_value_data_as( &s, dataNode.second, DT_STRING );
						def->customHandler->fromString( propObj, s, &size );
					}
					break;
				}
				pop();
			}
			pop();
			return true;
		}	
		bool readPointer( const PropDef* def, size_t index, void* object )
		{
			ValueData _nullPtr( (const void*)NULL );
			return err_ok == def->set( def, 0, object, &_nullPtr );
		}	
		bool readRflxObject( const PropDef* def, size_t index, void* object )
		{
			RFLX_DASSERT( def && def->customHandler && def->type == vdt_pointer );	
			if ( !pushObject( def->name, 0 ) )
				return false;
			bool ret = false;
			Node& node = back();
			// must has class overrided name as a key
			if ( dt_value_type( node.second ) == DT_OBJECT ) {
				Node node2;
				node2.first = NULL;
				node2.second = NULL;
				dt_object_member_at( root, (dt_object_t)dt_value_data( node.second ), 0, &node2.first, &node2.second );
				const char* className = NULL;
				dt_value_data_as( &className, node2.first, DT_STRING );
				if ( className[0] == RFLXEXT_QUALIFIY_TYPE_PREFIX ) {
					++className;
					Klass klass = findClass( className );
					Klass staticKlass = def->customHandler->klass();
					RFLX_DASSERT( klass && staticKlass && isKindOf( klass, staticKlass ) && isKindOf( klass, rflx::RflxObject::_class() ) );
					RflxObject* newObj = NULL;
					createObject( klass, &newObj );
					rflx::setBasePropertiesDefault( newObj->_mbase(), newObj->_dynamicClass(), NULL, true );
					if ( !newObj ) return false;
					const void* ptr = dynamicCast( newObj->_dynamicClass(), newObj->_mbase(), staticKlass );
					ValueData vptr( ptr );
					def->set( def, 0, object, &vptr );
					if ( pushObject( _getClassName( klass ).c_str(), 0 ) ) {
						Node& dataNode = back();
						switch ( dt_value_type( dataNode.second ) )
						{
						case DT_OBJECT:
							ret = err_ok == forBaseEachProperty( klass, newObj->_mbase(), readFunc, this );
							break;
						case DT_STRING:
							{
								const char* s = NULL;
								size_t size = 0;
								dt_value_data_as( &s, dataNode.second, DT_STRING );
								ret = err_ok == def->customHandler->fromString( newObj->_mbase(), s, &size );
							}
							break;
						}
						pop();
					}
				}
			}
			pop();
			return ret;
		}	
		bool readArray( const PropDef* def, size_t index, void* object )
		{
			if ( psk_array != _classifyProperty( def ) )
				return false;
			if ( !pushObject( def->name, 0 ) )
				return false;
			const Node& node = back();
			if ( dt_value_type( node.second ) != DT_ARRAY )
				return false;
			size_t size = 0;
			dt_array_t a = NULL;
			dt_value_data_as( &a, node.second, DT_ARRAY );
			dt_array_elem_count( root, a, &size );
			_PropDef keyInfo = { def->type2, def->customHandler2 };
			_PropDef valInfo = { def->type, def->customHandler };
			PropOp opType = op_none;
			if ( def->kind == vt_c_array ) {
				size_t destSize = 0;
				operateFunctionWrapper( def->op, def, object, op_size, &destSize );
				if ( size > destSize )
					size = destSize;
				opType = op_index;
			} else if ( def->kind == vt_array ) {
				operateFunctionWrapper( def->op, def, object, op_resize, size );
				opType = op_index;
			} else if ( def->kind == vt_set ) {	
				operateFunctionWrapper( def->op, def, object, op_clear );
				opType = op_insert;
			}
			for ( size_t i = 0; i < size; ++i ) {
				dt_value_t val = NULL;
				dt_array_elem_at( root, a, i, &val );
				ValueData _val;
				parseContainerDTValue( val, valInfo, _val, cvu_value );
				if ( _val.isNil() )
					continue;
				if ( opType == op_index )
					operateFunctionWrapper( def->op, def, object, opType, i, &_val );
				else if ( opType == op_insert ) {
					bool insert_ret = false;
					operateFunctionWrapper( def->op, def, object, opType, &_val, &insert_ret );
				}
			}
			pop();
			return true;
		}
		bool readMap( const PropDef* def, size_t index, void* object )
		{			
			if ( psk_map != _classifyProperty( def ) )
				return false;
			if ( !pushObject( def->name, 0 ) )
				return false;
			const Node& node = back();
			if ( dt_value_type( node.second ) != DT_OBJECT )
				return false;
			size_t size = 0;
			dt_object_t o = NULL;
			dt_value_data_as( &o, node.second, DT_OBJECT );
			dt_object_member_count( root, o, &size );
			_PropDef keyInfo = { def->type2, def->customHandler2 };
			_PropDef valInfo = { def->type, def->customHandler };
			for ( size_t i = 0; i < size; ++i ) {
				dt_value_t key = NULL, val = NULL;
				dt_object_member_at( root, o, i, &key, &val );
				ValueData _key, _val;
				parseContainerDTValue( key, keyInfo, _key, cvu_key );
				if ( _key.isNil() )
					continue;
				parseContainerDTValue( val, valInfo, _val, cvu_value );
				if ( _val.isNil() )
					continue;
				// arg1 : const ValueData* Key
				// arg2 : const ValueData* T
				// arg3 : return bool if success
				bool insert_ret = false;
				operateFunctionWrapper( def->op, def, object, op_insert, &_key, &_val, &insert_ret );
			}
			pop();
			return true;
		}

		static void readFunc( const PropDef* def, void* object, void* extra )
		{
			DTReadStack* _this = ( DTReadStack* )extra;
			_this->readObject( def, 0, object );
		}

		PropSerKind readObject( const PropDef* def, size_t index, void* object )
		{
			bool ret = false;
			PropSerKind kind = _classifyProperty( def );
			switch ( kind ) {
			case psk_base:
				ret = readBase( def, index, object );
				break;
			case psk_custom:				
				ret = readCustom( def, index, object );
				break;
			case psk_normal_pointer:				
				ret = readPointer( def, index, object );
				break;
			case psk_object_pointer:				
				ret = readRflxObject( def, index, object );
				break;
			case psk_array:				
				ret = readArray( def, index, object );
				break;
			case psk_map:			
				ret = readMap( def, index, object );
				break;
			}
			return kind;
		}
	};

	class DTWriteStack
	{
		typedef std::vector< DTValue > stack_type;
		dt_datatree_t root;
		stack_type stack;
	public:
		DTWriteStack() : root( NULL ) {}
		void bind( dt_datatree_t _root ) { root = _root; stack.size(); }
		void push( DTValue& o ) { stack.push_back( o ); }
		void pushObject() {
			DTValue node;
			dt_create_value( root, &node, _on_parse_error, "{}", DT_OBJECT );
			stack.push_back( node );
		}
		void push_array() {
			DTValue node;
			dt_create_value( root, &node, _on_parse_error, "[]", DT_ARRAY );
			stack.push_back( node );
		}
		bool append( const char* key, const char* source, dt_type_t type ) 
		{
			if ( stack.empty() ) return false;
			DTValue& node = stack.back();
			RFLX_DASSERT( node != NULL && source );
			DTValue val;
			if ( type != DT_STRING )
				dt_create_value( root, &val, _on_parse_error, source );
			else
				dt_create_value( root, &val, _on_parse_error, NULL, DT_STRING, source );
			switch ( dt_value_type( node ) )
			{
			case DT_OBJECT:
				{			
					dt_object_t o = NULL;
					dt_status_t ret = dt_value_data_as( &o, node, DT_OBJECT );
					RFLX_DASSERT( key );
					if ( ret == DT_OK ) {
						DTValue dtkey;
						dt_create_value( root, &dtkey, _on_parse_error, NULL, DT_STRING, key );
						ret = dt_add_object_member( root, o, dtkey, val, NULL );
						return ret == DT_OK;
					}
				}
				break;
			case DT_ARRAY:
				{
					dt_array_t o = NULL;
					dt_status_t ret = dt_value_data_as( &o, node, DT_ARRAY );
					RFLX_DASSERT( key );
					if ( ret == DT_OK ) {
						dt_add_array_elem( root, o, val, NULL );
						return ret == DT_OK;
					}
				}
				break;
			default: 
				RFLX_DASSERT( 0 );
			}
			return false;
		}

		bool popPair() {
			if ( stack.size() < 3 ) return false;
			if ( dt_value_type( stack[ stack.size() - 3 ] ) != DT_OBJECT ) {
				// error type
				return false;
			}
			DTValue val = stack.back();
			stack.pop_back();
			DTValue key = stack.back();
			stack.pop_back();
			DTValue& obj = stack.back();
			dt_object_t o = NULL;
			dt_value_data_as( &o, obj, DT_OBJECT );
			return DT_OK == dt_add_object_member( root, o, key, val, NULL );
		}

		void discard( size_t num ) {
			if ( stack.size() >= num && num != 0 )
				stack.resize( stack.size() - num );
		}

		bool empty() const { return stack.empty(); }
		bool pop( const char* keyVal = NULL, dt_type_t type = DT_STRING )
		{
			if ( keyVal && keyVal[0] ) {
				DTValue key;
				if ( type == DT_STRING )
					dt_create_value( root, &key, _on_parse_error, NULL, DT_STRING, keyVal );
				else
					dt_create_value( root, &key, _on_parse_error, keyVal );
				DTValue val = stack.back();
				stack.pop_back();
				if ( stack.empty() ) {
					dt_value_t rtv = dt_root_value( root );
					if ( dt_value_type( rtv ) == DT_OBJECT )
						return DT_OK == dt_add_object_member( root, dt_root_as_object( root ), key, val, NULL );
					else if ( dt_value_type( rtv ) == DT_ARRAY ) {
						dt_array_t a = NULL;
						dt_value_data_as( &a, rtv, DT_ARRAY );
						return DT_OK == dt_add_array_elem( root, a, val, NULL );
					} else {
						return false;
					}
				}
				else {
					DTValue& obj = stack.back();
					dt_object_t o = NULL;
					dt_value_data_as( &o, obj, DT_OBJECT );

					return DT_OK == dt_add_object_member( root, o, key, val, NULL );
				}
			} else {
				if ( stack.size() < 2 ) return false;
				if ( dt_value_type( stack[ stack.size() - 2 ] ) != DT_ARRAY ) {
					// error type
					return false;
				}
				DTValue val = stack.back();
				stack.pop_back();
				DTValue& obj = stack.back();
				dt_array_t a = NULL;
				dt_value_data_as( &a, obj, DT_ARRAY );
				return DT_OK == dt_add_array_elem( root, a, val, NULL );
			}
		}
	};

	class DTreeSerializer : public ISerializer
	{
	private:
		ErrorCode			mLastError;
		DTWriteStack		mWriteStack;
		DTReadStack			mReadStack;
		unsigned int		mCursor;
		Usage				mUsage;
		const Format		mFormat;
		Behavior			mBehaviors;
		dt_datatree_t		mContext;
	private:
		DTreeSerializer( DTreeSerializer& );
		DTreeSerializer& operator = ( const DTreeSerializer& );
		static ValueDataType convType( dt_type_t src )
		{
			static ValueDataType table[] = {
				vdt_nil,
				vdt_bool,
				vdt_schar,
				vdt_uchar,
				vdt_short,
				vdt_ushort,
				vdt_int,
				vdt_uint,
				vdt_long,
				vdt_ulong,
				vdt_float,
				vdt_double,
				vdt_string,
				vdt_nil,
				vdt_nil,
			};
			return table[ src ];
		}
		static dt_type_t convType( ValueDataType src )
		{
			static dt_type_t table[ vdt_max_num ] = {
				DT_NULL,	//vdt_nil = 0,
				DT_NULL,	//vdt_reflexable_begin,
				DT_NULL,	//vdt_base_type_begin,
				DT_BOOL,	//vdt_bool,
				DT_NULL,	//vdt_number_begin,
				DT_NULL,	//vdt_integer_number_begin,
				DT_NULL,	//vdt_signed_integer_number_begin,
				DT_BYTE,	//vdt_schar,
				DT_BYTE,	//vdt_schar_,
				DT_SHORT,	//vdt_short,
				DT_INT,		//vdt_int,
				DT_LONG,	//vdt_long,
				DT_STRING,	//vdt_llong,
				DT_NULL,	//vdt_signed_integer_number_end,
				DT_NULL,	//vdt_unsigned_integer_number_begin,
				DT_USHORT,	//vdt_wchar,
				DT_UBYTE,	//vdt_uchar,
				DT_UBYTE,	//vdt_uchar_,
				DT_USHORT,	//vdt_ushort,
				DT_UINT,	//vdt_uint,
				DT_ULONG,	//vdt_ulong,
				DT_STRING,	//vdt_ullong,
				DT_NULL,	//vdt_unsigned_integer_number_end,
				DT_NULL,	//vdt_integer_number_end,
				DT_NULL,	//vdt_float_number_begin,
				DT_SINGLE,	//vdt_float,
				DT_DOUBLE,	//vdt_double,
				DT_NULL,	//vdt_float_number_end,
				DT_NULL,	//vdt_number_end,
				DT_NULL,	//vdt_base_type_end,
				DT_STRING,	//vdt_string,
				DT_STRING,	//vdt_wstring,
				DT_STRING,	//vdt_enum,
				DT_OBJECT,	//vdt_custom,
				DT_NULL,	//vdt_pointer,
				DT_NULL,	//vdt_reflexable_end,
			};
			return table[ src ];
		}

		std::string getClassName( Klass klass )
		{
			return std::string( 1, RFLXEXT_QUALIFIY_TYPE_PREFIX ) + rflx::getClassName( klass );
		}
	public:
		ISerializer& operator << ( const KlassInst& obj )
		{
			if ( mUsage != ser_usage_write ) {
				mLastError = err_operation_invalid;
				return *this;
			}
			serializeTo( obj );
			return *this;
		}
		ISerializer& operator << ( const rflx::RflxObject& obj )
		{
			if ( mUsage != ser_usage_write ) {
				mLastError = err_operation_invalid;
				return *this;
			}
			bool atRoot = mWriteStack.empty();
			if ( atRoot )
				mWriteStack.pushObject();
			KlassInst sobj = { obj._dynamicClass(), const_cast< void* >( obj._mbase() ) };
			bool ret = serializeTo( sobj );
			if ( atRoot && ret ) {
				size_t size = 0;
				dt_object_member_count( mContext, dt_root_as_object( mContext ), &size );
				TempStrBuf str;
				sprintf( str.buf, "%c%d", RFLXEXT_QUALIFIY_NUMBER_PREFIX, size );
				mWriteStack.pop( str.buf );
			}
			return *this;
		}
		ISerializer& operator >> ( rflx::RflxObject& obj )
		{
			rflx::RflxObject* o = &obj;
			serializeFrom( o );
			return *this;
		}
		ISerializer& operator >> ( rflx::RflxObject*& obj )
		{
			RFLX_DASSERT( !obj );
			serializeFrom( obj );
			return *this;
		}

		ErrorCode getLastError() const { return mLastError; }

		Format format() const { return mFormat; }
		Usage usage() const { return mUsage; }
		DTreeSerializer( Format fmt, Usage usage ) : mFormat( fmt ), mUsage( usage ), mCursor( 0 ), mBehaviors( ser_bhv_none )
		{
			mLastError = err_ok;
			dt_create_datatree( &mContext, _on_parse_error );
			mWriteStack.bind( mContext );
			mReadStack.bind( mContext );
			dt_load_datatree_string( mContext, "{}" );
		}

		~DTreeSerializer()
		{
			dt_unload_datatree( mContext );
			dt_destroy_datatree( mContext );
		}
		
		bool setUsage( Usage usage )
		{
			mLastError = err_ok;
			if ( usage != mUsage ) {
				mUsage = usage;
				return true;
			}
			return false;
		}

		unsigned int getCursor() const
		{
			return mCursor;
		}

		bool setCursor( unsigned int pos )
		{
			if ( mUsage != ser_usage_read ) {
				mLastError = err_operation_invalid;
				return false;
			}
			mLastError = err_ok;
			size_t size = 0;
			dt_object_t o = dt_root_as_object( mContext );
			if ( !o ) return false;
			dt_object_member_count( mContext, o, &size );
			if ( ( size_t )pos >= size ) {
				mLastError = err_out_of_range;
				return false;
			}
			mCursor = pos;
			return true;
		}

		size_t pushDTValue( ValueDataType type, const CustomDataHandler* handle, const ValueData& v, unsigned short traits_bits = 0 )
		{
			size_t rval = 0;
			RFLX_DASSERT( !( type == vdt_pointer && ( handle == NULL || handle->klass() == NULL ) ) );
			if ( type == vdt_pointer || type == vdt_custom ) {
				void* object = v.type == vdt_pointer ? const_cast< void* >( v._pointer ) : v._custom;
				if ( v.type == vdt_custom && strcmp( handle->name(), v._customHandler->name() ) != 0 ) {
					// type is dismatch!
					return 0;
				}

				// create object
				if ( object ) {
					KlassInst obj = { handle->klass(), object };
					if ( traits_bits & ptf_rflxobject ) {
						obj.mbase = dynamicCast( handle->klass(), obj.mbase, RflxObject::_class() );
						RflxObject* o = reinterpret_cast< RflxObject* >( obj.mbase );
						obj.klass = o->_dynamicClass();
						obj.mbase = o->_mbase();
					}

					mWriteStack.pushObject();
					serializeTo( obj );
					rval = 1;
				} else {
					Klass klass = handle->klass();
					if ( klass )
						mWriteStack.append( getClassName( klass ).c_str(), "nil", DT_STRING );
					else {
						// only happen in arary
						mWriteStack.append( NULL, "nil", DT_STRING );
					}
					rval = 0;
				}
			} else {
				std::string str;
				if ( v.toString( str, handle ) ) {
					DTValue val;
					if ( convType( type ) != DT_STRING )
						dt_create_value( mContext, &val, _on_parse_error, str.c_str() );
					else
						dt_create_value( mContext, &val, _on_parse_error, NULL, DT_STRING, str.c_str() );
					mWriteStack.push( val );
					rval = 1;
				}
			}
			return rval;
		}

		bool serializeTo_Pointer( const PropDef* def, void* object )
		{		
			// pointer address
			char* ptr = *(char**)def->get_ref( def, 0, object );
			Klass klass;
			unsigned int propCount = 0;
			def->getDefs( &klass, &propCount, NULL );
			if ( !ptr || !klass )
				return mWriteStack.append( def->name, "nil", DT_STRING );
			if ( klass ) {
				if ( def->traits_bits & ptf_rflxobject ) {
					// get the pointer object static class type
					ptr = (char*)dynamicCast( klass, ptr, RflxObject::_class() );
					RflxObject* element = reinterpret_cast< RflxObject* >( ptr );
					mWriteStack.pushObject();
						serializeTo( *element );
					mWriteStack.pop( def->name );
				} else if ( def->traits_bits & ptf_rflxable ) {
					// TODO: this should be a failure
					// we may need to discard this value here because we don't what the 
					// pointer really is.
					KlassInst sobj = { klass, ptr };
					mWriteStack.pushObject();
						serializeTo( sobj );
					mWriteStack.pop( def->name );
				} else {
					RFLX_DASSERT( 0 );
				}
			}
			return true;
		}

		bool serializeTo_CustomData( const PropDef* def, void* object )
		{				
			RFLX_DASSERT( def->customHandler && def->kind == vt_scalar );	
			ValueData vd;
			def->get( def, 0, object, &vd );
			TempStrBuf tbuf;	
			Klass klass = def->customHandler->klass();
			if ( klass ) {
				char* child = (char*)def->get_ref( def, 0, object );
				KlassInst sobj = { klass, child };
				mWriteStack.pushObject();
					serializeTo( sobj );
				mWriteStack.pop( def->name );

			} else if ( err_ok == def->customHandler->toString( vd._custom, tbuf.buf, &tbuf.len ) ) {
				Klass klass = def->customHandler->klass();
				mWriteStack.pushObject();
				{
					std::string name;
					if ( klass )
						name = getClassName( klass );
					else {
						name.push_back( RFLXEXT_QUALIFIY_TYPE_PREFIX );
						name.append( def->customHandler->name() );
					}
					mWriteStack.append( name.c_str(), tbuf.buf, DT_STRING );
				}
				mWriteStack.pop( def->name );
			} else {
				// unknown custom data!
				return false;
			}
			return true;
		}

		bool serializeTo_Container_Map( const PropDef* def, void* object )
		{
			RFLX_DASSERT( def->kind == vt_map );
			// key is a unknown pointer type ?
			if ( def->type2 == vdt_pointer && ( def->customHandler2 == NULL || def->customHandler2->klass() == NULL ) )
				return false;
			size_t size = 0;
			operateFunctionWrapper( def->op, def, object, op_size, &size );
			for ( size_t i = 0; i < size; ++i ) {
				ValueData key;
				ValueData val;
				if ( err_ok == operateFunctionWrapper( def->op, def, object, op_at, i, &key, &val ) ) {
					size_t num = pushDTValue( def->type2, def->customHandler2, key, def->traits_bits2 );
					num += pushDTValue( def->type, def->customHandler, val, def->traits_bits );
					if ( num == 2 )
						mWriteStack.popPair();
					else
						mWriteStack.discard( num );
				}
			}
			return true;
		}

		bool serializeTo_Container_Array( const PropDef* def, void* object )
		{
			RFLX_DASSERT( def->kind != vt_map );
			size_t size = 0;
			ErrorCode err = operateFunctionWrapper( def->op, def, object, op_size, &size );
			for ( size_t i = 0; i < size; ++i ) {
				ValueData val;
				if ( err_ok == operateFunctionWrapper( def->op, def, object, op_at, i, &val ) ) {
					if ( pushDTValue( def->type, def->customHandler, val, def->traits_bits ) )
						mWriteStack.pop();
				}
			}
			return true;
		}

		bool serializeTo_Container( const PropDef* def, void* object, size_t* elemCount )
		{
			size_t size = 0;
			ErrorCode err = operateFunctionWrapper( def->op, def, object, op_size, &size );
			if ( elemCount ) *elemCount = size;
			if ( !size )
				return true; 
			switch ( def->kind ) {
				case vt_c_array:
				case vt_array:
				case vt_set:
					{
						mWriteStack.push_array();
						serializeTo_Container_Array( def, object );
						mWriteStack.pop( def->name );
					}
					break;
				case vt_map:
					{
						mWriteStack.pushObject();
						serializeTo_Container_Map( def, object );
						mWriteStack.pop( def->name );
					}
					break;
			}
			return true;
		}

		bool serializeTo_BaseType( const PropDef* def, void* object )
		{
			RFLX_DASSERT( def->kind == vt_scalar &&
				def->type != vdt_pointer &&
				def->type != vdt_custom );

			bool ret = false;
			ValueData v;
			def->get( def, 0, object, &v );
			if ( v.type == vdt_string ) {
				if ( v._string[0] == '\0' )
					ret = true;
				else
					ret = mWriteStack.append( def->name, v._string, DT_STRING );
			}
			else if ( v.type == vdt_wstring ) {
				// convert to multi-char string
				if ( v._wstring[0] == '\0' )
					ret = true;
				else {
					v = v.cast( vdt_wstring );
					ret = mWriteStack.append( def->name, v._string, DT_STRING );
				}
			} else if ( v.type == vdt_enum && def->enumInfo != NULL ) {
				if ( v._enum != def->enumInfo->getItemByIndex( 0 )->value ) {
					std::string sv;
					if ( v.toString( sv ) )
						ret = mWriteStack.append( def->name, sv.c_str(), DT_STRING );
				}
				else
					ret = true;
			} else {
				std::string sv;
				if ( v.toString( sv ) )
					ret = mWriteStack.append( def->name, sv.c_str(), DT_OBJECT );
			}
			return ret;
		}

		bool serializeTo_Scalar( const PropDef* def, void* object )
		{
			RFLX_DASSERT( def->kind == vt_scalar );
			if ( def->type == vdt_custom )
				return serializeTo_CustomData( def, object );
			else if ( def->traits_bits & ( ptf_pointer | ptf_ref ) )					
				return serializeTo_Pointer( def, object );
			else 
				return serializeTo_BaseType( def, object );
		}

		bool serializeTo( const KlassInst& obj )
		{
			mWriteStack.pushObject();
			forBaseEachProperty( obj.klass, obj.mbase, serializeTo, this );
			return mWriteStack.pop( getClassName( obj.klass ).c_str() );
		}	
		bool serializeTo( const rflx::RflxObject& obj )
		{
			KlassInst sobj = { obj._dynamicClass(), const_cast< void* >( obj._mbase() ) };
			return serializeTo( sobj );
		}
		bool serializeTo( const PropDef* def, void* object )
		{
			if ( def->kind == vt_scalar )
				return serializeTo_Scalar( def, object );
			else {
				size_t count = 0;
				return serializeTo_Container( def, object, &count );
			}
		}
		bool serializeFrom( rflx::RflxObject*& obj )
		{
			if ( mUsage != ser_usage_read ) {
				mLastError = err_operation_invalid;
				return false;
			}
			dt_value_t val = NULL;
			dt_object_member_at( mContext, dt_root_as_object( mContext ), mCursor, NULL, &val );
			if ( !val ) { 
				mLastError = err_object_not_found;
				return false;
			}
			dt_object_t o = NULL;
			dt_value_data_as( &o, val, DT_OBJECT );
			if ( o ) {
				dt_value_t key1 = NULL;
				dt_value_t val1 = NULL;
				dt_object_t o = NULL;
				dt_value_data_as( &o, val, DT_OBJECT );
				if ( o ) {
					dt_object_member_at( mContext, o, 0, &key1, &val1 );
					char* name = NULL;
					dt_value_data_as( &name, key1, DT_STRING );
					if ( name && *name == RFLXEXT_QUALIFIY_TYPE_PREFIX ) {
						++name;
						Klass klass = rflx::findClass( name );
						if ( klass ) {
							ErrorCode err = err_ok;
							if ( obj == NULL ) {
								err = createObject( klass, &obj );
								rflx::setBasePropertiesDefault( obj->_mbase(), klass, NULL, true );
							}
							else {
								if ( !rflx::isInstanceOf( obj, klass ) ) {
									err = err_failed;
									mLastError = err_data_type_mismatch;
									return false;
								}
							}
							if ( err == err_ok ) {
								mReadStack.push( std::make_pair( key1, val1 ) );
								forObjectEachProperty( obj, DTReadStack::readFunc, &mReadStack );
								mReadStack.pop();
							}
						} else {
							// class not found!
							mLastError = err_class_not_found;
							return false;
						}

					}
				}
			}
			++mCursor;
			return true;
		}

		static void serializeTo( const PropDef* def, void* object, void* extra )
		{
			DTreeSerializer* ser = ( DTreeSerializer* )extra;
			ser->serializeTo( def, object );
		}

		
		bool toString( std::string& out )
		{
			RFLX_DASSERT( mContext );
			out.clear();
			char* str = NULL;
			dt_save_datatree_string( mContext, &str, DT_FALSE );
			out.assign( str );
			dt_free( (void**)&str );
			return true;
		}
		
		bool toFile( const std::string& _fileName, bool _bin )
		{
			bool bin = _bin;
			std::string fileName( _fileName );
			size_t ext = fileName.find_first_of( '.' );
			if ( ext != std::string::npos ) {
				if ( !bin && _stricmp( fileName.c_str() + ext + 1, RFLXEXT_DTREE_FILE_BIN ) == 0 )
					bin = true;
				fileName.erase( ext );
			}
			fileName.push_back( '.' );
			fileName.append( bin ? RFLXEXT_DTREE_FILE_BIN : RFLXEXT_DTREE_FILE_TXT );
			RFLX_DASSERT( mContext );
			if ( bin ) {	
				std::fstream f( fileName.c_str(), std::ios_base::out | std::ios_base::binary );
				if ( !f )
					return false;
				void* bin = NULL;
				size_t size = 0;
				dt_save_datatree_bin( mContext, &bin, &size );
				if ( size == 0 )
					return false;
				f.write( (const char*)bin, ( std::streamsize )size );
				f.close();
				dt_free( (void**)&bin );
			} else {	
				std::fstream f( fileName.c_str(), std::ios_base::out );
				if ( !f )
					return false;
				char* s = NULL;
				dt_save_datatree_string( mContext, &s, DT_FALSE );
				f << s;
				f.close();
				dt_free( (void**)&s );
			}
			return true;
		}
		
		bool fromString( const std::string& in )
		{
			mLastError = err_ok;
			dt_unload_datatree( mContext );
			dt_load_datatree_string( mContext, in.c_str() );
			mWriteStack.bind( mContext );
			return true;
		}
		
		bool fromFile( const std::string& _fileName )
		{
			std::string fileName( _fileName );
			size_t ext = fileName.find_last_of( '.' );
			if ( ext == std::string::npos ) {
				mLastError = err_file_unknown_type;
				return false;
			}
			std::string extName = fileName.substr( ext + 1 );
			std::fstream f( fileName.c_str(), std::ios::binary | std::ios::in );
			if ( !f ) {
				mLastError = err_file_not_exist;
				return false;
			}
			f.seekg( 0, std::ios::end );
			std::streamsize size = f.tellg();
			std::vector< char > buf( ( unsigned int )size + 1 );
			f.seekg( 0 );
			f.read( &buf[0], size );
			mLastError = err_ok;
			dt_unload_datatree( mContext );
			dt_destroy_datatree( mContext );
			mContext = NULL;
			mCursor = 0;
			dt_create_datatree( &mContext, _on_parse_error );
			dt_status_t dt_ret = DT_OK;
			if ( _stricmp( extName.c_str(), RFLXEXT_DTREE_FILE_BIN ) == 0 ) {
				dt_ret = dt_load_datatree_bin( mContext, &buf[0] );
			} else if ( _stricmp( extName.c_str(), RFLXEXT_DTREE_FILE_TXT ) == 0 ) {
				dt_ret = dt_load_datatree_string( mContext, &buf[0] );
			} else {
				mLastError = err_file_unknown_type;
				return false;
			}
			mWriteStack.bind( mContext );
			mReadStack.bind( mContext );
			return dt_ret == DT_OK;
		}
	};

	ISerializer* createSerializer( ISerializer::Format fmt, ISerializer::Usage usage )
	{
		switch ( fmt ) {
			case ISerializer::ser_fmt_dtree: return new DTreeSerializer( fmt, usage );
			default: return NULL;
		}
	}

	void destroySerializer( ISerializer* s )
	{
		delete s;
	}
}
