
enum FileStatus {
	FS_Normal,
	FS_Deleted,
	FS_New,
};

static auto source_file_filter = []( LPCTSTR name, LPCTSTR fullName )->BOOL {
	if ( !g_ignoreTable.empty() ) {
		String _fullName( fullName );
		StringUtil::toLowerCase( _fullName );
		for ( size_t i = 0; i < g_ignoreTable.size(); ++i ) {
			if ( Misc::matchWildcard( _fullName.c_str(), g_ignoreTable[i].c_str() ) ) {
				return false;
			}
		}
	}
	return true;
};

class Settings
{
public:
	enum Type
	{
		Unknown = 0,
		Directory,
		File,
	};
	Settings( const Settings& );
	Settings& operator = ( const Settings& );
	Settings( const String& _name, Type _type ) : data( NULL ),
		original( NULL ), handle( NULL ), node( NULL ),
		resolved( false ), deleted( false ), name( _name ), type( _type )
	{
		assert( !name.empty() );
	}
	~Settings() {
		if ( data && data != original ) {
			rflx::destroyObject( data );
			data = NULL;
		}
	}

	Type getType() const { return type; }

	WinUtil::DirTreeNode* getDirNode() {
		return type == Directory ? node : NULL;
	}

	WinUtil::DirTreeNode* getFileDirNode() {
		return type == File ? node : NULL;
	}

	void copyFrom( const Settings& o ) {
		if ( this == &o ) {
			return;
		}
		if ( data && data != original ) {
			rflx::destroyObject( data );
			data = NULL;
		}
		original = o.original;
		resolved = o.resolved;
		deleted = o.deleted;
		if ( o.data ) {
			data = o.data->createInstance( true );
		}
		handle = handle;
		node = NULL;
		name = o.name;
		assert( type == o.type );
		type = o.type;
	}

	const char* _getTypeName() const {
		return original ? original->getName() : "";
	}

	String getTypeName() const {
		return original ? original->getName() : "";
	}

	bool setTypeName( const String& name ) {
		original = rflext::findObjectTemplate( AppOTLib, name.c_str() );
		return !!original;
	}
	
	rflext::TRflxObject* getInstanceData() {
		return data;
	}

	const rflext::TRflxObject* getInstanceData() const {
		return data;
	}

	const rflext::TRflxObject* getPropObj() const {
		if ( !data ) {
			return original;
		}
		return data;
	}

	const rflext::TRflxObject* getOriPropObj() const {
		return original;
	}

	rflext::TRflxObject* getPropObj() {
		if ( !data && original ) {
			data = original->createInstance();
		}
		return data;
	}

	const rflext::TRflxObject* getPropObjForRead() const {
		return data ? data : original;
	}

	void deleteInstanceData() {
		if ( data ) {
			delete data;
			data = NULL;
		}
	}

	const rflx::ValueData* getPropVal( const String& name ) const
	{
		const TRflxObject* cur = getPropObjForRead();
		if ( cur ) {
			const PropDefExt* p = cur->findProp( name );
			if ( p ) {
				return &p->valueHolder;
			}
		}
		return NULL;
	}

	bool _hasProp( const char* name ) const {
		if ( !name ) {
			return false;
		}
		const rflx::ValueData* v = getPropVal( name );
		return v != NULL;
	}

	bool _getPropVal_bool( const char* name ) const {
		if ( !name ) {
			return false;
		}
		const rflx::ValueData* v = getPropVal( name );
		bool val = false;
		if ( v ) {
			if ( !v->isBoolean() ) {
				rflx::ValueData vv = v->cast( rflx::vdt_bool );
				if ( !vv.isNil() ) {
					vv.extract( val );
				}
			} else {
				v->extract( val );
			}
		}
		return val;
	}

	int _getPropVal_int( const char* name ) const {
		if ( !name ) {
			return 0;
		}
		const rflx::ValueData* v = getPropVal( name );
		int val = 0;
		if ( v ) {
			if ( !v->isInteger() ) {
				rflx::ValueData vv = v->cast( rflx::vdt_int );
				if ( !vv.isNil() ) {
					vv.extract( val );
				}
			} else {
				v->extract( val );
			}
		}
		return val;
	}

	double _getPropVal_real( const char* name ) const {
		if ( !name ) {
			return 0;
		}
		const rflx::ValueData* v = getPropVal( name );
		double val = 0;
		if ( v ) {
			if ( !v->isNumber() ) {
				rflx::ValueData vv = v->cast( rflx::vdt_double );
				if ( !vv.isNil() ) {
					vv.extract( val );
				}
			} else {
				v->extract( val );
			}
		}
		return val;
	}

	const char* _getPropVal_string( const char* name ) const {	
		if ( !name ) {
			return false;
		}
		const rflx::ValueData* v = getPropVal( name );
		static String val;
		val.clear();
		if ( v ) {
			if ( !v->isString() ) {
				rflx::ValueData vv = v->cast( rflx::vdt_string );
				if ( !vv.isNil() ) {
					vv.extract( val );
				}
			} else {
				v->extract( val );
			}
		}
		return val.c_str();
	}

	const char* _getName() const { return name.c_str(); }

	bool hasInstanceData() const {
		return data != original && data != NULL && original != NULL;
	}

	const String& getName() const {
		return name;
	}

	void setDirNode( WinUtil::DirTreeNode* _node ) {
		assert( type == Unknown || type == Directory );
		assert( _node );
		node = _node;
		type = Directory;
	}

	void setFileNode( WinUtil::DirTreeNode* _node ) {
		assert( type == Unknown || type == File );
		assert( _node );
		node = _node;
		type = File;
	}

	String getPath() const {
		if ( type == Unknown ) {
			return "<unknown setting>";
		}
		String path;
		path.reserve( 128 );
		WinUtil::DirTreeNode* cur = node;
		while ( cur ) {
			if ( !path.empty() ) {
				path.insert( 0, "/" );
			}
			path.insert( 0, cur->curDir );
			cur = cur->parent;
		}
		return path;
	}

	const char* _getPath() const {
		static String s;
		s.clear();
		s = getPath();
		return s.c_str();
	}

	const char* _getPathName() const {
		static String s;
		s.clear();
		s = getPath();
		if ( type == File ) {
			if ( !s.empty() ) {
				s.push_back( '/' );
			}
			s.append( name );
		}
		return s.c_str();
	}

	bool isDir() const {
		return type == Directory;
	}

	bool isFile() const {
		return type == File;
	}

	bool isDeleted() const {
		return deleted;
	}
	bool isResolved() const {
		return resolved;
	}
	bool isValid() const {
		return resolved && !deleted;
	}
public:
	HTREEITEM handle;
private:
	rflext::TRflxObject* data;
	const rflext::TRflxObject* original;
	WinUtil::DirTreeNode* node;
	Type type;
	std::string name;
public:
	bool resolved;
	bool deleted;
};

template < typename FUNC >
void forEachProperty( const TRflxObject& obj, FUNC func ) {
	const TRflxObject* cur = &obj;
	while ( cur ) {
		std::for_each( cur->getProps().begin(), cur->getProps().end(), func );
		cur = cur->getBaseObj();
	}
}

static ValueData _applyValueTo( const String& propName, const ValueData& from, Settings& to, ValueData& modFrom ) {
	const String& name = propName;
	const ValueData& newVal = from;
	ValueData oldVal;
	modFrom = from;
	if ( !to.hasInstanceData() ) {
		// check do we need a instance here
		const rflext::TRflxObject* ori = to.getOriPropObj();
		const PropDefExt* vd = ori->findProp( name );
		if ( vd && vd->defaultVal != NULL && vd->nameHolder == name ) {
			if ( !newVal.isNil() && !vd->defaultVal->isNil() &&
				( newVal.type == vd->defaultVal->type ||
				newVal.isString() == vd->defaultVal->isString() ) &&
				newVal != *vd->defaultVal ||
				vd->defaultVal->isNil() ) {
					AfxTrace( "value %s will be modify letter.\n", name );
			} else {
				return oldVal;
			}
		}
	}			
	rflext::TRflxObject* obj = to.getPropObj();
	if ( obj ) {
		PropDefExt* vd = obj->findProp( name );
		if ( vd != NULL ) {
			oldVal = vd->valueHolder;
			extern bool g_AddModify;
			auto _newVal = newVal;
			if ( g_AddModify && vd->type == vdt_string ) {
				String _s;
				String _x;
				if ( oldVal.toString( _s ) && newVal.toString( _x ) ) {
					StringUtil::trim( _s );
					std::vector<String> _sl;
					std::set<String> _ss;
					StringUtil::split( _sl, _s, " ,;" );
					for ( size_t x = 0; x < _sl.size(); ++x ) {
						StringUtil::trim( _sl[x] );
						_ss.insert( _sl[x] );
					}
					std::vector<String> _dl;
					std::set<String> _ds;
					StringUtil::trim( _x );
					StringUtil::split( _dl, _x, " ,;" );
					for ( size_t x = 0; x < _dl.size(); ++x ) {
						auto s = _dl[x];
						StringUtil::trim( s );
						if ( !StringUtil::startsWith( s, "-" ) ) {
							if ( _ss.find( s ) == _ss.end() ) {
								_ss.insert( s );
							}
						} else {
							std::replace( s.begin(), s.end(), '-', ' ' );
							StringUtil::trim( s );
							if ( _ss.find( s ) != _ss.end() ) {
								_ss.erase( s );
							}
						}
					}
					String buf;
					for ( const String& s : _ss ) {
						if ( !buf.empty() ) {
							buf.append( "," );
							buf.append( s );
						} else {
							buf = s;
						}
					}
					_newVal = buf;
					modFrom = _newVal;
				}
			}
			if ( vd->type == newVal.type ) {
				vd->valueHolder = _newVal;
			} else {
				vd->valueHolder = _newVal;
				vd->valueHolder = vd->valueHolder.cast( vd->type );
			}
		}
	}
	
	return oldVal;
}

static int _applyValueTo( const TRflxObject& from, Settings& to )
{
	int count = 0;
	forEachProperty( from,
		[&]( const TRflxObject::PropTable::value_type& item ) {
			const String& name = item.first;
			const ValueData& newVal = item.second->valueHolder;
			if ( !to.hasInstanceData() ) {
				// check do we need a instance here
				const rflext::TRflxObject* ori = to.getOriPropObj();
				const PropDefExt* vd = ori->findProp( name );
				if ( vd && vd->defaultVal != NULL && vd->nameHolder == name ) {
					if ( !newVal.isNil() && !vd->defaultVal->isNil() &&
						( newVal.type == vd->defaultVal->type || newVal.isString() == vd->defaultVal->isString() ) &&
						newVal != *vd->defaultVal ||
						vd->defaultVal->isNil() ) {
							AfxTrace( "value %s will be modify letter.\n", name );
					} else {
						return;
					}
				}
			}			
			rflext::TRflxObject* obj = to.getPropObj();
			if ( obj ) {
				PropDefExt* vd = obj->findProp( name );
				if ( vd && vd->ownerName == item.second->ownerName ) {
					if ( vd->type == newVal.type ) {
						vd->valueHolder = newVal;
					} else {
						vd->valueHolder = newVal;
						vd->valueHolder = vd->valueHolder.cast( vd->type );
					}
					++count;
				}
			}
		}
	);
	return count;
}

template< typename T >
static void _walkSettings( std::vector< Settings* >& sels, T func, int maxDepth = -1 ) {
	std::vector< Settings* > stack;
	std::vector< int > depth;
	std::for_each( sels.begin(), sels.end(),
		[&]( Settings* s ) {
			if ( s->handle ) {
				stack.clear();
				depth.clear();
				stack.push_back( s );
				depth.push_back( 0 );
				while ( !stack.empty() ) {
					Settings* p = stack.back();
					int curDepth = depth.back();
					stack.pop_back();
					depth.pop_back();
					func( p );
					if ( p->getDirNode() ) {
						std::for_each( p->getDirNode()->fileNames.begin(), p->getDirNode()->fileNames.end(),
							[&]( const WinUtil::FileItem& fi ) {
								Settings* s = reinterpret_cast< Settings* >( fi.userData );
								func( s );
							}
						);
						if ( maxDepth < 0 || curDepth < maxDepth ) {
							std::for_each( p->getDirNode()->subDirs.begin(), p->getDirNode()->subDirs.end(),
								[&]( WinUtil::DirTreeNode* child ) {
									depth.push_back( curDepth + 1 );
									stack.push_back( reinterpret_cast< Settings* >( child->userData ) );
								}
							);
						}
					}
				}
			}
		}
	);
}

static std::vector< String > _getSettingsItemNames( const Settings& settings )
{
	std::vector< String > ret;
	const TRflxObject* obj = settings.getPropObj();
	while ( obj ) {
		std::for_each( obj->getProps().begin(), obj->getProps().end(),
			[&] ( const TRflxObject::PropTable::value_type& item ) {
				ret.push_back( item.first );
			} 
		);
		obj = obj->getBaseObj();
	}
	return ret;
}

static inline HTREEITEM _HandleFromNode( WinUtil::DirTreeNode* node )
{
	if ( node->userData ) {
		if ( node->userData ) {
			return ( ( Settings* )node->userData )->handle;
		}
	}
	return NULL;
}

static inline HTREEITEM _ParentHandleFromNode( WinUtil::DirTreeNode* node )
{
	if ( node->userData ) {
		if ( node->userData ) {
			return node->parent ? _HandleFromNode( node->parent ) : NULL;
		}
	}
	return NULL;
}

static std::map< std::string, rflx::ValueData > _getSettingsItems( const Settings& settings )
{
	std::map< std::string, rflx::ValueData > ret;
	const TRflxObject* obj = settings.getPropObj();
	while ( obj ) {
		std::for_each( obj->getProps().begin(), obj->getProps().end(),
			[&] ( const TRflxObject::PropTable::value_type& item ) {
				ret[ item.first ] = item.second->valueHolder;
			}
		);
		obj = obj->getBaseObj();
	}
	return ret;
}

static std::vector< String > _getSettingTypeList( const Settings& settings ) {
	std::vector< String > ret;
	const TRflxObject* obj = settings.getOriPropObj();
	while ( obj ) {
		ret.insert( ret.begin(), obj->getName() );
		obj = obj->getBaseObj();
	}
	return ret;
}

static std::vector< String > _getCommonSettingTypeList( const std::vector< Settings* >& vSettings )
{
	if ( vSettings.size() == 1 ) {
		return _getSettingTypeList( *vSettings[0] );
	} else if ( !vSettings.empty() ) {
		auto ret = std::vector< String >();
		auto a = _getSettingTypeList( *vSettings[0] );
		auto num = a.size();
		for ( size_t i = 1; i < vSettings.size(); ++i ) {
			auto b = _getSettingTypeList( *vSettings[i] );
			num = __min( num, b.size() );
			ret.clear();
			for ( size_t j = 0; j < num; ++j ) {
				if ( a[j] == b[j] ) {
					ret.push_back( a[j] );
				} else {
					break;
				}
			}
			num = ret.size();
		}
		return ret;
	}
	return std::vector< String >();
}

static std::vector< String > _getEqualPropNames( const std::vector< Settings* >& vSettings )
{	
	if ( vSettings.empty() ) {
		return std::vector< String >();
	} else if ( vSettings.size() == 1 ) {
		return _getSettingsItemNames( *vSettings.back() );
	} else {
		std::vector< String > ret;
		std::map< std::string, rflx::ValueData > vals = _getSettingsItems( **vSettings.begin() );
		for ( size_t i = 1; i < vSettings.size(); ++i ) {
			std::map< std::string, rflx::ValueData > ovals = _getSettingsItems( *vSettings[i] );
			auto it = ovals.begin();
			while ( it != ovals.end() ) {
				auto iit = vals.find( it->first );
				if ( iit != vals.end() ) {
					// found, compare it now
					if ( iit->second == it->second || iit->second.isNil() && it->second.isNil() ) {
						ret.push_back( it->first );
					}
				} else {
					// update compare property list
					vals[ it->first ] = it->second;
				}
				++it;
			}
		}
		return ret;
	}
}

static std::vector< String > _getNotEqualPropNames( const std::vector< Settings* >& vSettings )
{
	if ( vSettings.empty() ) {
		return std::vector< String >();
	} else if ( vSettings.size() == 1 ) {
		return _getSettingsItemNames( *vSettings.back() );
	} else {
		std::vector< String > ret;
		std::map< std::string, rflx::ValueData > vals = _getSettingsItems( **vSettings.begin() );
		for ( size_t i = 1; i < vSettings.size(); ++i ) {
			std::map< std::string, rflx::ValueData > ovals = _getSettingsItems( *vSettings[i] );
			auto it = ovals.begin();
			while ( it != ovals.end() ) {
				auto iit = vals.find( it->first );
				if ( iit != vals.end() ) {
					// found, compare it now
					if ( g_OverwriteMultiEdit ) {
						if ( iit->second.type != it->second.type ) {
							ret.push_back( it->first );
						}
					} else {
						if ( iit->second != it->second ) {
							ret.push_back( it->first );
						}
					}
				} else {
					// update compare property list
					vals[ it->first ] = it->second;
					ret.push_back( it->first );
				}
				++it;
			}
		}
		return ret;
	}
}

static inline Settings* _SettingFromNode( WinUtil::DirTreeNode* node )
{
	return ( Settings* )node->userData;
}

struct _ForEacher {
	static std::vector< rflext::PropDefExt* > list;
	static void f( const rflext::TRflxObject* obj ) {
		for ( auto p : obj->getProps() ) {
			list.push_back( p.second );
		}
	}
};

std::vector< rflext::PropDefExt* > _ForEacher::list;

std::vector< rflext::PropDefExt* > _getAllPropDefsFromLib()
{
	_ForEacher::list.clear();
	if ( AppOTLib != NULL ) {
		foreachOTL( AppOTLib, _ForEacher::f );
	}
	return _ForEacher::list;
}

std::vector< Settings* > CFilemanDlg::GetCurrentSelSettingItems()
{
	std::vector< Settings* > ret;
	HTREEITEM hItem = m_FileTreeCtrl.GetFirstSelectedItem();
	while ( hItem ) {
		Settings* setting = ( Settings* )m_FileTreeCtrl.GetItemData( hItem );
		if ( setting ) {
			ret.push_back( setting );
		}
		hItem = m_FileTreeCtrl.GetNextSelectedItem( hItem );
	}
	return ret;
}
