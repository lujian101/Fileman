#ifndef __RFLX_EXT_H__
#define __RFLX_EXT_H__

#include <map>
#include "Rflx.h"

namespace rflext
{
	class RflxOTLib;
	typedef RflxOTLib* OTLib;

	struct PropDefExt : rflx::PropDef {
		PropDefExt();
		PropDefExt( const PropDefExt& o );
		PropDefExt& operator = ( const PropDefExt& o );
		void bind( bool resetValue = true );
		rflx::ValueData defaultValHolder;
		rflx::ValueData valueHolder;
		std::string nameHolder;
		std::string groupNameHolder;
		std::string descriptionHolder;
		std::string editorDataHolder;
		std::string ownerName;
	};

	class TRflxObject : public rflx::RflxObject {
	public:
		friend class RflxOTLib;
		typedef std::map< std::string, PropDefExt* > PropTable;
		typedef PropTable::iterator PropTable_It;
		typedef PropTable::const_iterator PropTable_ConstIt;
		RFLX_IMP_CLASS( TRflxObject, rflx::RflxObject );
	public:
		TRflxObject( bool readonly = false );
		TRflxObject( const TRflxObject& );
		TRflxObject& operator = ( const TRflxObject& );
		~TRflxObject();
		void					clear();
		void					setOwner( OTLib _lib ) { lib =  _lib; }
		OTLib					getOwner() const { return lib; }
		const char*				getName() const { return className.c_str(); }
		const char*				getBaseName() const { return baseClassName.c_str(); }
		const PropDefExt*		findProp( const std::string& name, bool recur = true ) const;
		PropDefExt*				findProp( const std::string& name, bool recur = true );
		PropTable&				getProps() { return props; }
		const PropTable&		getProps() const { return props; }
		TRflxObject*			createInstance( bool clone = false ) const;
		OTLib					getLib() const { return lib; }
		TRflxObject*			getOuter() const;
		bool					isTopmost() const { return !derive; }
		bool					valid() const { return lib != NULL && className.length() > 0; }
		TRflxObject*			getBaseObj() const { return base; }
		void					link();
	private:
		OTLib			lib;
		std::string		className;
		std::string		baseClassName;
		std::string		description;
		PropTable		props;
		TRflxObject*	derive;
		TRflxObject*	base;
		bool			readonly;
	public:
		RFLX_BEGIN_PROPERTY_MAP
			RFLX_IMP_PROPERTY_NAME( className, Name )
			RFLX_IMP_PROPERTY_NAME( baseClassName, BaseClassName )
			RFLX_IMP_PROPERTY_NAME( description, Description )
		RFLX_END_PROPERTY_MAP
	};

	OTLib				createOTLib( const char* libName );
	void				destroyOTLib( OTLib lib );
	bool				loadOTLibFromXML( OTLib lib, const char* fileName );
	const TRflxObject*	findObjectTemplate( OTLib lib, const char* objName );
	bool				isObjectTemplateValid( OTLib lib, const TRflxObject* obj );
	int					foreachOTL( OTLib lib, void ( *fun )( const TRflxObject* ) );

	struct RFLX_API ISerializer {
		enum Format {
			ser_fmt_binary = 0,
			ser_fmt_dtree,
			ser_fmt_xml,
		};
		enum Usage {
			ser_usage_read = 0,
			ser_usage_write
		};
		enum Behavior {
			ser_bhv_none = 0,
			ser_bhv_skip_default_value,
		};
		virtual ~ISerializer(){}
		virtual Format format() const = 0;
		virtual Usage usage() const = 0;
		virtual ISerializer& operator << ( const rflx::RflxObject& ) = 0;
		virtual ISerializer& operator >> ( rflx::RflxObject*& ) = 0;
		virtual ISerializer& operator >> ( rflx::RflxObject& ) = 0;
		virtual rflx::ErrorCode getLastError() const = 0;
		virtual bool setUsage( Usage usage ) = 0;
		virtual bool setCursor( unsigned int pos ) = 0;
		virtual unsigned int getCursor() const = 0;
		virtual bool toString( std::string& out ) = 0;
		virtual bool toFile( const std::string& fileName, bool bin = false ) = 0;
		virtual bool fromString( const std::string& in ) = 0;
		virtual bool fromFile( const std::string& fileName ) = 0;
	};

	RFLX_API ISerializer*	createSerializer( ISerializer::Format fmt,
											  ISerializer::Usage usage = ISerializer::ser_usage_read );

	RFLX_API void destroySerializer( ISerializer* ser );
}


#endif
