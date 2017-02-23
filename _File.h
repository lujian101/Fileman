
static void _savePropertiesToXML( TiXmlElement* root, const Settings* settings )
{	
	if ( settings->resolved ) {
		root->SetAttribute( "Resolved", "true" );
	}
	if ( settings && settings->hasInstanceData() ) {
		const rflext::TRflxObject* obj = settings->getPropObj();
		int count = 0;
		TiXmlElement* pnode = new TiXmlElement( "PropertyList" );
		while ( obj ) {
			std::for_each( obj->getProps().begin(), obj->getProps().end(),
				[&]( const TRflxObject::PropTable::value_type& item ) {
					if ( item.second->defaultVal &&
						item.second->valueHolder == item.second->defaultValHolder ) {
						return;
					}
					String str;
					if ( item.second->valueHolder.toString( str ) ) {
						TiXmlElement* propNode = new TiXmlElement( "Property" );
						pnode->LinkEndChild( propNode );
						propNode->SetAttribute( "Name", item.first.c_str() );
						propNode->SetAttribute( "Value", str.c_str() );
						++count;
					} else {
						AfxTrace( "[%s]'s value is nil", item.first.c_str() );
					}
				}
			);
			obj = obj->getBaseObj();
		}
		if ( count > 0 ) {
			root->LinkEndChild( pnode );
		} else {
			delete pnode;
		}
	}
}

static bool _loadPropertiesToXML( TiXmlElement* root, Settings* settings )
{
	const char* attr = root->Attribute( "Resolved" );
	if ( attr ) {
		rflx::ValueData vd;
		vd.fromString( rflx::vdt_bool, attr );
		vd.extract( settings->resolved );
	} else {
		settings->resolved = false;
	}
	TiXmlElement* propNodes = root->FirstChildElement( "PropertyList" );
	if ( propNodes ) {
		TiXmlElement* propNode = propNodes->FirstChildElement( "Property" );
		while ( propNode ) {
			const char* name = propNode->Attribute( "Name" );
			const char* value = propNode->Attribute( "Value" );
			if ( name && value ) {
				auto obj = settings->getPropObj();
				PropDefExt* pv = obj->findProp( name );
				if ( pv ) {
					pv->valueHolder.fromString( pv->type, std::string( value ) );
				} else {
					__Trace( "property %s no found in class: %s", name, obj->getName() );
				}
			}
			propNode = propNode->NextSiblingElement( "Property" );
		}
	}
	return true;
}

static void _savePropertiesToXML( TiXmlElement* root, const WinUtil::DirTreeNode& node )
{
	const Settings* settings = reinterpret_cast< Settings* >( node.userData );
	_savePropertiesToXML( root, settings );
}

static void _savePropertiesToXML( TiXmlElement* root, const WinUtil::FileItem& fi )
{
	const Settings* settings = reinterpret_cast< Settings* >( fi.userData );
	_savePropertiesToXML( root, settings );
}

static bool _loadPropertiesToXML( TiXmlElement* root, WinUtil::DirTreeNode& node )
{
	auto settings = std::auto_ptr< Settings >( new Settings( node.curDir, Settings::Directory ) );
	settings->setTypeName( "DirectorySettings" );
	assert( node.userData == NULL );
	node.userData = settings.get();
	bool ret = _loadPropertiesToXML( root, settings.get() );
	if ( ret ) {
		node.userData = settings.release();
	} else {
		node.userData = NULL;
	}
	return ret;
}

static bool _loadPropertiesToXML( TiXmlElement* root, WinUtil::FileItem& fi )
{
	auto settings = std::auto_ptr< Settings >( new Settings( fi.name, Settings::File ) );
	settings->setTypeName( "FileSettings" );
	assert( fi.userData == NULL );
	fi.userData = settings.get();
	bool ret = _loadPropertiesToXML( root, settings.get() );
	if ( ret ) {
		fi.userData = settings.release();
	} else {
		fi.userData = NULL;
	}
	return ret;
}

static bool _loadDirRecur( TiXmlElement* dirNode, WinUtil::DirTreeNode& node )
{
	using WinUtil::DirTreeNode;
	const char* dirName = dirNode->Attribute( "DirName" );
	if ( !dirName ) {
		return false;
	}
	node.curDir = dirName;
	_loadPropertiesToXML( dirNode, node );
	TiXmlElement* fileNode = dirNode->FirstChildElement( "File" );
	while ( fileNode ) {
		WinUtil::FileItem fi;
		const char* name = fileNode->Attribute( "FileName" );
		if ( name ) {
			fi.name = name;
			_loadPropertiesToXML( fileNode, fi );
			node.fileNames.push_back( fi );
		}
		fileNode = fileNode->NextSiblingElement( "File" );
	}
	TiXmlElement* subDirNode = dirNode->FirstChildElement( "Directory" );
	while ( subDirNode ) {
		DirTreeNode* cn = new DirTreeNode();
		if ( _loadDirRecur( subDirNode, *cn ) ) {
			cn->parent = &node;
			node.subDirs.push_back( cn );
		} else {
			cn->clear( _DirTreeNode_UserData_Free );
			delete cn;
		}
		subDirNode = subDirNode->NextSiblingElement( "Directory" );
	}
	return true;
}

static void _getFileList( std::set< String >& fileList, std::set< String >& dirList, const WinUtil::DirTreeNode& node, const String& parent )
{
	String curPath( parent );
	if ( !curPath.empty() ) {
		curPath.push_back( '/' );
	}
	curPath.append( node.curDir );
	dirList.insert( curPath );
	std::for_each( node.fileNames.begin(), node.fileNames.end(),
		[&]( const WinUtil::FileItem& fi ) {
			fileList.insert( curPath + "/" + fi.name );
		}
	);
	for ( size_t i = 0; i < node.subDirs.size(); ++i ) {
		_getFileList( fileList, dirList, *node.subDirs[i], curPath );
	}
}

BOOL CFilemanDlg::SaveDirTreeNodeToXML( const char* fileName, const WinUtil::DirTreeNode& root )
{
	TiXmlDocument doc;
	using WinUtil::DirTreeNode;
	using WinUtil::FileItem;
	TiXmlElement* rootElement = new TiXmlElement( "DirTree" );
	std::vector< std::pair< const WinUtil::DirTreeNode*, TiXmlElement* > > stack;
	stack.push_back( std::make_pair( &root, rootElement ) );
	while ( !stack.empty() ) {
		const DirTreeNode* cur = stack.back().first;
		TiXmlElement* parent = stack.back().second;
		const Settings* settings = reinterpret_cast< Settings* >( cur->userData );
		if ( settings && !settings->deleted ) {
			TiXmlElement* dirNode = new TiXmlElement( "Directory" );
			dirNode->SetAttribute( "DirName", cur->curDir.c_str() );
			_savePropertiesToXML( dirNode, *cur );
			parent->LinkEndChild( dirNode );
			for ( size_t i = 0; i < cur->fileNames.size(); ++i ) {
				const FileItem& fi = cur->fileNames[i];
				const Settings* settings = reinterpret_cast< Settings* >( fi.userData );
				if ( settings && !settings->deleted ) {
					TiXmlElement* fileNode = new TiXmlElement( "File" );
					_savePropertiesToXML( fileNode, fi );
					fileNode->SetAttribute( "FileName", fi.name.c_str() );
					dirNode->LinkEndChild( fileNode );
				}
			}
			stack.pop_back();
			std::for_each( cur->subDirs.rbegin(), cur->subDirs.rend(),
				[&]( DirTreeNode* n ) {
					stack.push_back( std::make_pair( n, dirNode ) );
				}
			);
		} else {
			stack.pop_back();
		}
	}
	doc.LinkEndChild( rootElement );
	return doc.SaveFile( fileName );
}

BOOL CFilemanDlg::LoadDirTreeFromXML( const char* fileName, WinUtil::DirTreeNode& root )
{
	root.clear( _DirTreeNode_UserData_Free );
	TiXmlDocument doc;
	if ( !doc.LoadFile( fileName ) ) {
		return FALSE;
	}
	TiXmlElement* rootElement = doc.FirstChildElement( "DirTree" );
	if ( !rootElement ) {
		return FALSE;
	}
	TiXmlElement* element = rootElement->FirstChildElement( "Directory" );
	if ( element ) {
		_loadDirRecur( element, root );
	}
	root.sort();
	return TRUE;
}

BOOL CFilemanDlg::SearchFileFromRoot( const String& _rootPath, WinUtil::DirTreeNode& out, const String* ignoreList )
{
	if ( _rootPath.length() == 0 ) {
		return FALSE;
	}
	BOOL ret = TRUE;
	std::vector< String > dirs;
	StringUtil::split( dirs, _rootPath, "\\/" );
	String rootPath = StringUtil::standardisePath( _rootPath );
	WinUtil::DirTreeNode& dirTree = out;
	dirTree.clear( _DirTreeNode_UserData_Free );
	dirTree.curDir = dirs.empty() ? "./" : dirs.back();
	g_ignoreTable.clear();
	std::for_each( g_globalIgnoreTable.begin(), g_globalIgnoreTable.end(),
		[]( const String& src ) {
			g_ignoreTable.push_back( src );
		}
	);
	if ( ignoreList != NULL ) {
		std::vector<String> ignoreTable;
		StringUtil::split( ignoreTable, *ignoreList, ",;\n\r" );
		std::for_each( ignoreTable.begin(), ignoreTable.end(),
			[]( String& src ) {
				std::transform( src.begin(), src.end(), src.begin(), ::tolower );
			}
		);
	}
	WinUtil::getFileLists( dirTree, rootPath.c_str(), source_file_filter, NULL );
	return ret;
}

BOOL CFilemanDlg::MergeDir( WinUtil::DirTreeNode& out, const WinUtil::DirTreeNode& src, const WinUtil::DirTreeNode& dst )
{
	if ( src.curDir != dst.curDir ) {
		__Trace( "Dir tree root must be the same!" );
		return FALSE;
	}
	// usually, dst is the real file system
	// src is the file tree comes from config file
	// if file in src not found in dst, we consider this file is deleted, directory also.
	std::set< String > srcFileList;
	std::set< String > dstFileList;	
	std::set< String > srcDirList;
	std::set< String > dstDirList;
	out.clear();
	_getFileList( srcFileList, srcDirList, src, String() );
	_getFileList( dstFileList, dstDirList, dst, String() );
	// find new files
	typedef std::pair< String, FileStatus > value_t;
	std::vector< value_t > mergedFiles;
	std::vector< value_t > mergedDirs;
	for ( auto i : dstFileList ) {
		if ( srcFileList.find( i ) == srcFileList.end() ) {
			mergedFiles.push_back( std::make_pair( i, FS_New ) );
		} else {
			mergedFiles.push_back( std::make_pair( i, FS_Normal ) );
		}
	}	
	for ( auto i : dstDirList ) {
		if ( srcDirList.find( i ) == srcDirList.end() ) {
			mergedDirs.push_back( std::make_pair( i, FS_New ) );
		} else {
			mergedDirs.push_back( std::make_pair( i, FS_Normal ) );
		}
	}
	// find deleted files
	for ( auto i : srcFileList ) {
		if ( dstFileList.find( i ) == dstFileList.end() ) {
			mergedFiles.push_back( std::make_pair( i, FS_Deleted ) );
		}
	}	
	for ( auto i : srcDirList ) {
		if ( dstDirList.find( i ) == dstDirList.end() ) {
			mergedDirs.push_back( std::make_pair( i, FS_Deleted ) );
		}
	}
	auto cmp = []( const value_t& l, const value_t& r ){ return l.first < r.first; };
	std::sort( mergedFiles.begin(), mergedFiles.end(), cmp );
	std::sort( mergedDirs.begin(), mergedDirs.end(), cmp );
	using WinUtil::DirTreeNode;
	DELETE_TREE_NODE_DATA( out );
	{
		// root
		assert( out.userData == NULL );
		Settings* s = new Settings( src.curDir, Settings::Directory );
		out.userData = s;
		out.curDir = src.curDir;
		s->deleted = false;
		if ( src.userData ) {
			const Settings* ss = reinterpret_cast< const Settings* >( src.userData );
			s->copyFrom( *ss );
		} else {
			s->setTypeName( "DirectorySettings" );
		}
	}
	for ( auto i : mergedDirs ) {
		FileStatus fileState = i.second;
		const String& dirPath = i.first;
		std::vector< String > seg;
		StringUtil::split( seg, i.first, "/\\" );
		DirTreeNode* cur = &out;
		assert( cur->curDir.empty() || cur->curDir == seg[0] );
		if ( cur->curDir.empty() ) {
			cur->curDir = seg[0];
		}
		for ( size_t i = 1; i < seg.size(); ++i ) {
			auto it = std::find_if( cur->subDirs.begin(), cur->subDirs.end(),
				[&]( const DirTreeNode* n ) {
					return n->curDir == seg[i];
				}
			);
			if ( it == cur->subDirs.end() ) {
				// not found
				DirTreeNode* newNode = new DirTreeNode;
				newNode->curDir = seg[i];
				newNode->parent = cur;
				{
					Settings* s = new Settings( newNode->curDir, Settings::Directory );
					newNode->userData = s;
					s->setDirNode( newNode );
					const DirTreeNode* n = NULL;
					if ( fileState == FS_New ) {
						n = dst.findDir( dirPath );
					} else {
						n = src.findDir( dirPath );
					}
					assert( n );
					if ( n->userData ) {
						const Settings* ss = reinterpret_cast< const Settings* >( n->userData );
						s->copyFrom( *ss );
					} else {
						s->setTypeName( "FileSettings" );
					}
					s->deleted = fileState == FS_Deleted;
				}
				cur->subDirs.push_back( newNode );
				cur = newNode;
			} else {
				cur = *it;
			}
		}
	}
	for ( auto i : mergedFiles ) {
		FileStatus fileState = i.second;
		const String& filePath = i.first;
		String pathName;
		String fileName;
		size_t p = i.first.find_last_of( '/' );
		assert( p != String::npos );
		pathName = i.first.substr( 0, p );
		fileName = i.first.substr( p + 1 );
		DirTreeNode* dir = const_cast< DirTreeNode* >( out.findDir( pathName ) );
		assert( dir );
#ifdef _DEBUG
		auto it = std::find( dir->fileNames.begin(), dir->fileNames.end(), fileName );
		assert( it == dir->fileNames.end() );
#endif
		WinUtil::FileItem fi;
		fi.name = fileName;
		dir->fileNames.push_back( fi );
		WinUtil::FileItem* newNode = &dir->fileNames.back();
		Settings* s = new Settings( newNode->name, Settings::File );
		s->setFileNode( dir );
		newNode->userData = s;
		const WinUtil::FileItem* n = NULL;
		if ( fileState == FS_New ) {
			n = dst.findFileWithPath( pathName, fileName );
		} else {
			n = src.findFileWithPath( pathName, fileName );
		}
		assert( n );
		if ( n->userData ) {
			const Settings* ss = reinterpret_cast< const Settings* >( n->userData );
			s->copyFrom( *ss );
		} else {
			s->setTypeName( "FileSettings" );
		}
		s->deleted = fileState == FS_Deleted;
	}
	return TRUE;
}
