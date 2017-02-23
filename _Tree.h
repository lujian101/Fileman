
void CFilemanDlg::OnTvnSelchangingFileTree( NMHDR* pNMHDR, LRESULT* pResult )
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	*pResult = 0;
}

void CFilemanDlg::UpdateTree()
{
	UpdateTreeItemStateAll();
}

void CFilemanDlg::UpdateTreeItemState( const std::vector< Settings* >& vSettings )
{
	typedef std::pair< int, Settings* > value_t;
	std::vector< value_t > _settings;
	std::for_each( vSettings.begin(), vSettings.end(),
		[&]( Settings* s ) {
			_settings.push_back(
				std::make_pair(
					[&]() {
						// get tree depth
						auto cur = s->handle;
						auto depth = 0;
						while ( cur ) {
							++depth;
							cur = m_FileTreeCtrl.GetParentItem( cur );
						}
						return depth;
					}(),
					s
				)
			);
		}
	);
	// sort by tree depth
	// more deep node put near at the list' top
	std::sort( _settings.begin(), _settings.end(),
		[]( const value_t& left, const value_t& right ) {
			return left.first > right.first;
		}
	);
	std::for_each( _settings.begin(), _settings.end(),
		[&]( const value_t& s ) {
			UpdateTreeItemState( s.second );
		}
	);
}

static auto _getPathList = []( const CMultiTree& tree, HTREEITEM item ) {
	std::deque< std::string > ret;
	const Settings* _s = reinterpret_cast< Settings* >( tree.GetItemData( item ) );
	ret.push_front( _s->getName() );
	HTREEITEM parent = tree.GetParentItem( item );
	while ( parent ) {
		_s = reinterpret_cast< Settings* >( tree.GetItemData( parent ) );
		ret.push_front( _s->getName() );
		parent = tree.GetParentItem( parent );
	}
	return ret;
};

static auto _getPathListString = []( const CMultiTree& tree, HTREEITEM item ) {
	auto pl = _getPathList( tree, item );
	std::string ret;
	for ( auto s : pl ) {
		if ( ret.length() > 0 ) {
			ret.append( "/" );
		}
		ret.append( s );
	}
	return ret;
};

static auto _toPathList = []( const std::string& path ) {
	std::deque< std::string > ret;
	StringUtil::split( ret, path, "/" );
	return ret;
};

template<class T>
static int _doTreeItems( CMultiTree& tree,
						 const std::deque<std::string>& pl,
						 const Settings* sel,
						 T fun )
{
	const Settings* _sels = sel;
	// root name check, must be a dir node
	auto cur = tree.GetRootItem();
	if ( cur == NULL ) {
		return 0;
	}
	const Settings* _s = reinterpret_cast< Settings* >( tree.GetItemData( cur ) );
	if ( pl[0] != _s->getName() ) {
		return 1;
	}
	auto target = cur;
	for ( size_t i = 1; i < pl.size() && target; ++i ) {
		target = NULL;
		auto child = tree.GetChildItem( cur );
		while ( child ) {
			Settings* s = reinterpret_cast< Settings* >( tree.GetItemData( child ) );
			if ( s->getName() == pl[i] ) {
				// inner must be a dir
				// outter could be dir or file both
				if ( s->isDir() && i != pl.size() - 1 || _sels == NULL ||
					s == _sels && i == pl.size() - 1 ) {
						target = child;
						cur = child;
						break;
				}
			}
			child = tree.GetNextSiblingItem( child );
		}
	}
	if ( target ) {
#ifdef _DEBUG
		auto left = _getPathList( tree, target );
		if ( left != pl ) {
			String path;
			for ( auto i : pl ) {
				if ( path.empty() ) {
					path.push_back( '/' );
				}
				path.append( i );
			}
			CString txt;
			txt.Format( "Item %s not found!", path.c_str() );
			_AppendLog( txt );
		} else
#endif
			fun( tree, target );
	}
	return 0;
}

static void _TreeSelectAllRecur( CMultiTree& tree, UINT state, HTREEITEM cur ) {
	tree.SetItemState( cur, state, TVIS_SELECTED );
	auto child = tree.GetChildItem( cur );
	while ( child != NULL ) {
		_TreeSelectAllRecur( tree, state, child );
		child = tree.GetNextSiblingItem( child );
	}
}

static void _TreeSelectAll( CMultiTree& tree, BOOL bSelect = TRUE ) {
	auto item = tree.GetRootItem();
	auto state = bSelect ? TVIS_SELECTED : 0;
	if ( item != NULL ) {
		_TreeSelectAllRecur( tree, state, item );
	}
}

static void _TreeSelectAllIgnore( CMultiTree& tree, BOOL bSelect, HTREEITEM cur, const std::vector<HTREEITEM>& ignoreList ) {
	UINT nState = bSelect ? TVIS_SELECTED : 0;
	if ( ignoreList.end() == std::find( ignoreList.begin(), ignoreList.end(), cur ) ) {
		if ( tree.IsSelected( cur ) != bSelect ) {
			tree.SetItemState( cur, nState, TVIS_SELECTED );
		}
	}
	auto child = tree.GetChildItem( cur );
	while ( child != NULL ) {
		_TreeSelectAllIgnore( tree, bSelect, child, ignoreList );
		child = tree.GetNextSiblingItem( child );
	}
}

static void _TreeSelectAllIgnore( CMultiTree& tree, BOOL bSelect = TRUE, const std::vector<HTREEITEM>* ignoreList = NULL ) {
	if ( ignoreList != NULL ) {
		auto item = tree.GetRootItem();
		_TreeSelectAllIgnore( tree, bSelect, item, *ignoreList );
	} else {
		_TreeSelectAll( tree, bSelect );
	}
}

static void _TreeSortAllRecur( CMultiTree& tree, HTREEITEM item ) {
   if ( item != NULL ) {
      if ( item == TVI_ROOT || tree.ItemHasChildren( item ) ) {
         HTREEITEM child = tree.GetChildItem( item );
         while( child != NULL ) {
            _TreeSortAllRecur( tree, child );
            child = tree.GetNextItem( child, TVGN_NEXT );
         }
         tree.SortChildren( item );
      }
   }
}

void CFilemanDlg::Select( lua_tinker::table& pathList )
{
	int count = 0;
	std::vector<std::string> plist;
	_TreeSelectAll( m_FileTreeCtrl, FALSE );
	for ( int i = 1;  i < 100000; ++i ) {
		const char* p = pathList.array_get<const char*>( i );
		if ( p == NULL || *p == '\0' ) {
			break;
		}
		auto pl = _toPathList( p );
		auto ret = _doTreeItems( m_FileTreeCtrl, pl, NULL,
			[]( CMultiTree& tree, HTREEITEM item ) {
				tree.SetItemState( item, TVIS_SELECTED | TVIS_FOCUSED, TVIS_SELECTED | TVIS_FOCUSED );
			}
		);		
		if ( ret == 0 ) {
			AppendLogText( p );
			++count;
		}
	}			
	CString str;
	str.Format( "%d items selected.", count );
	AppendLogText( str );
}

void CFilemanDlg::ReverseSelect( lua_tinker::table& pathList )
{	
	std::vector<std::string> plist;
	std::vector<HTREEITEM> ignore;
	_TreeSelectAll( m_FileTreeCtrl, TRUE );
	int count = 0;
	for ( int i = 1;  i < 100000; ++i ) {
		const char* p = pathList.array_get<const char*>( i );
		if ( p == NULL || *p == '\0' ) {
			break;
		}
		auto pl = _toPathList( p );
		auto ret = _doTreeItems( m_FileTreeCtrl, pl, NULL,
			[&]( CMultiTree& tree, HTREEITEM item ) {
				tree.SetItemState( item, 0, TVIS_SELECTED | TVIS_FOCUSED );
			}
		);
		if ( ret == 0 ) {
			++count;
			AppendLogText( p );
		}
	}
	CString str;
	str.Format( "%d items selected.", m_FileTreeCtrl.GetCount() - count );
	AppendLogText( str );
}

void CFilemanDlg::OnTvnSelchangedFilterTree(NMHDR *pNMHDR, LRESULT *pResult)
{
	if ( !m_FileTreeCtrl.GetRootItem() ) {
		return;
	}
	std::vector< std::pair< HTREEITEM, const Settings* > > sels;
	HTREEITEM hItem = m_FilterTreeCtrl.GetFirstSelectedItem();
	while ( hItem ) {
		const Settings* s = reinterpret_cast<const Settings*>( m_FilterTreeCtrl.GetItemData( hItem ) );
		sels.push_back( std::make_pair( hItem, s ) );
		hItem = m_FilterTreeCtrl.GetNextSelectedItem( hItem );
	}
	m_FileTreeCtrl.SelectAll( FALSE );
	for ( auto item : sels ) {
		auto pl = _getPathList( m_FilterTreeCtrl, item.first );
		const Settings* _sels = item.second;
		auto ret = _doTreeItems( m_FileTreeCtrl, pl, _sels,
			[]( CMultiTree& tree, HTREEITEM item ) {
				tree.SetItemState( item, TVIS_SELECTED | TVIS_FOCUSED, TVIS_SELECTED | TVIS_FOCUSED );
			}
		);
	}
	*pResult = 0;
}

void CFilemanDlg::FilterTree( const WinUtil::DirTreeNode& data )
{
	m_FilterTreeCtrl.DeleteAllItems();
	if ( data.curDir.empty() ) {
		return;
	}
	assert( data.userData != NULL );
	if ( m_FilterScript ) {
		using WinUtil::DirTreeNode;
		m_PostLog = TRUE;
		_filterTreeRecur filter( m_FilterScript, m_FilterTreeCtrl );
		try {
			filter( data, NULL );
		} catch( const std::runtime_error& e ) {
			Misc::trace( "Exception has been catched: %s", e.what() );
		}
		m_PostLog = FALSE;
		m_OutputWindow.SetWindowText( m_LogText );
		if ( m_FilterTreeCtrl.GetRootItem() ) {
			m_FilterTreeCtrl.Expand( m_FilterTreeCtrl.GetRootItem(), TVM_EXPAND | TVE_EXPANDPARTIAL );
		}
	}
}

void CFilemanDlg::UpdatePropertiesFromTreeSelection()
{	
	std::vector< Settings* > sels = GetCurrentSelSettingItems();
	if ( !sels.empty() ) {
		std::vector< String > excludeList = _getNotEqualPropNames( sels );
		std::vector< String > ownerNames = _getCommonSettingTypeList( sels );
		UpdateProperty( *sels.back(), m_PropGridCtrl,
			sels.size() > 1 ? &excludeList : NULL,
			sels.size() > 1 ? &ownerNames : NULL );
		UpdateTreeItemState( sels );
	}
}

void CFilemanDlg::OnTvnSelchangedFileTree(NMHDR *pNMHDR, LRESULT *pResult)
{
	if ( m_FileTreeCtrl.IsMultiSelect() ) {
		static DWORD _lastTime = GetCurrentTime();
		if ( m_TreeSelHackTimerId == -1 ) {
			m_TreeSelHackTimerId = TREE_SEL_TIMER_ID;
			SetTimer( TREE_SEL_TIMER_ID, TREE_SEL_INTERVAL, NULL );
		}
		if ( GetCurrentTime() - _lastTime < TREE_SEL_INTERVAL ) {
			m_TreeSelUpdate = TRUE;
			return;
		}
		_lastTime = GetCurrentTime();
	}
	*pResult = 0;
	UpdatePropertiesFromTreeSelection();
}

void CFilemanDlg::OnNMRClickFileTree(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;
	CPoint oPoint;
	GetCursorPos( &oPoint );
	CPoint pt( oPoint );
	m_FileTreeCtrl.ScreenToClient( &pt );
	std::vector< HTREEITEM > sels;
	HTREEITEM sel = m_FileTreeCtrl.HitTest( pt );
	HTREEITEM item = m_FileTreeCtrl.GetFirstSelectedItem();
	int selCount = m_FileTreeCtrl.GetSelectedCount();
	while ( item ) {
		if ( sel == item ) {
			const Settings* setting = reinterpret_cast< Settings* >( m_FileTreeCtrl.GetItemData( sel ) );
			if ( setting->deleted ) {
				CMenu menu, *subMenu;
				menu.LoadMenu( IDR_TREE_MENU );
				subMenu = menu.GetSubMenu( 1 );	
				subMenu->TrackPopupMenu( TPM_LEFTALIGN, oPoint.x, oPoint.y, this );
			} else {
				if ( setting && setting->handle == item ) {
					CMenu menu, *subMenu;
					menu.LoadMenu( IDR_TREE_MENU );
					subMenu = menu.GetSubMenu( 0 );	
					if ( selCount == 1 && !m_FileTreeCtrl.ItemHasChildren( item ) ) {
						subMenu->DeleteMenu( ID_TREERIGHTCLICK_RESETCHILDREN, MF_BYCOMMAND );
						subMenu->DeleteMenu( ID_TREERIGHTCLICK_APPLYTOCHILDREN, MF_BYCOMMAND );
						subMenu->DeleteMenu( ID_TREERIGHTCLICK_RESETALL, MF_BYCOMMAND );
						subMenu->DeleteMenu( ID_TREERIGHTCLICK_RESETCHILDREN, MF_BYCOMMAND );
						subMenu->DeleteMenu( ID_TREERIGHTCLICK_APPLYALL, MF_BYCOMMAND );
					}
					subMenu->TrackPopupMenu( TPM_LEFTALIGN, oPoint.x, oPoint.y, this );
				}
			}
			break;
		}
		item = m_FileTreeCtrl.GetNextSelectedItem( item );
	}
}

void CFilemanDlg::OnTreeItemReset()
{
	std::vector< Settings* > sels = GetCurrentSelSettingItems();
	if ( sels.empty() ) {
		return;
	}
	MarkDirty( TRUE );
	std::for_each( sels.begin(), sels.end(),
		[&]( Settings* s ) {
			s->deleteInstanceData();
		}
	);
	UpdateTreeItemState( sels );
	UpdatePropertiesFromTreeSelection();
}

void CFilemanDlg::OnTreerightclickResetchildren()
{
	std::vector< Settings* > sels = GetCurrentSelSettingItems();
	if ( sels.empty() ) {
		return;
	}
	MarkDirty( TRUE );
	std::vector< Settings* > updateList;
	std::for_each( sels.begin(), sels.end(),
		[&]( Settings* s ) {
			if ( s->handle ) {
				HTREEITEM cur = s->handle;
				Settings* p = reinterpret_cast< Settings* >( m_FileTreeCtrl.GetItemData( cur ) );
				p->deleteInstanceData();
				updateList.push_back( p );
				HTREEITEM child = m_FileTreeCtrl.GetChildItem( cur );
				while ( child ) {
					Settings* s = reinterpret_cast< Settings* >( m_FileTreeCtrl.GetItemData( child ) );
					s->deleteInstanceData();
					updateList.push_back( s );
					child = m_FileTreeCtrl.GetNextSiblingItem( child );
				}
			}
		}
	);
	UpdateTreeItemState( updateList );
	UpdatePropertiesFromTreeSelection();
}

void CFilemanDlg::OnTreerightclickResetall()
{	
	std::vector< Settings* > sels = GetCurrentSelSettingItems();
	if ( sels.empty() ) {
		return;
	}
	MarkDirty( TRUE );
	std::vector< Settings* > stack;
	std::vector< Settings* > updateList;
	std::for_each( sels.begin(), sels.end(),
		[&]( Settings* s ) {
			if ( s->handle ) {
				stack.clear();
				stack.push_back( s );
				while ( !stack.empty() ) {
					HTREEITEM cur = stack.back()->handle;
					Settings* p = reinterpret_cast< Settings* >( m_FileTreeCtrl.GetItemData( cur ) );
					stack.pop_back();
					p->deleteInstanceData();
					updateList.push_back( p );
					HTREEITEM child = m_FileTreeCtrl.GetChildItem( cur );
					while ( child ) {
						Settings* s = reinterpret_cast< Settings* >( m_FileTreeCtrl.GetItemData( child ) );
						stack.push_back( s );
						updateList.push_back( s );
						child = m_FileTreeCtrl.GetNextSiblingItem( child );
					}
				}
			}
		}
	);
	UpdateTreeItemState( updateList );
	UpdatePropertiesFromTreeSelection();
}

void CFilemanDlg::OnTreerightclickApplytochildren()
{
	const std::vector< Settings* > sels = GetCurrentSelSettingItems();
	if ( sels.empty() ) {
		return;
	}
	MarkDirty( TRUE );
	const TRflxObject* propObj = sels.front()->getPropObjForRead();
	if ( propObj ) {
		std::for_each( sels.begin(), sels.end(),
			[&]( Settings* s ) {
				if ( s->handle ) {
					HTREEITEM cur = s->handle;
					Settings* p = reinterpret_cast< Settings* >( m_FileTreeCtrl.GetItemData( cur ) );
					if ( s != p ) {
						_applyValueTo( *propObj, *p );
					}
					HTREEITEM child = m_FileTreeCtrl.GetChildItem( cur );
					while ( child ) {
						Settings* s = reinterpret_cast< Settings* >( m_FileTreeCtrl.GetItemData( child ) );
						_applyValueTo( *propObj, *s );
						child = m_FileTreeCtrl.GetNextSiblingItem( child );
					}
				}
			}
		);
	}
	UpdateTreeItemState( sels );
	UpdatePropertiesFromTreeSelection();
}

void CFilemanDlg::OnTreerightclickApplyall()
{	
	std::vector< Settings* > sels = GetCurrentSelSettingItems();
	if ( sels.empty() ) {
		return;
	}
	MarkDirty( TRUE );
	const TRflxObject* propObj = sels.front()->getPropObjForRead();
	if ( propObj ) {
		std::vector< Settings* > stack;
		std::vector< Settings* > updateList;
		std::for_each( sels.begin(), sels.end(),
			[&]( Settings* s ) {
				if ( s->handle ) {
					stack.clear();
					stack.push_back( s );
					while ( !stack.empty() ) {
						HTREEITEM cur = stack.back()->handle;
						Settings* p = reinterpret_cast< Settings* >( m_FileTreeCtrl.GetItemData( cur ) );
						stack.pop_back();
						if ( s != p ) {
							_applyValueTo( *propObj, *p );
							updateList.push_back( p );
						}
						HTREEITEM child = m_FileTreeCtrl.GetChildItem( cur );
						while ( child ) {
							Settings* s = reinterpret_cast< Settings* >( m_FileTreeCtrl.GetItemData( child ) );
							_applyValueTo( *propObj, *s );
							updateList.push_back( s );
							stack.push_back( s );
							child = m_FileTreeCtrl.GetNextSiblingItem( child );
						}
					}
				}
			}
		);
		UpdateTreeItemState( updateList );
		UpdatePropertiesFromTreeSelection();
	}
}

void CFilemanDlg::UpdateTreeItemState( Settings* item )
{
	if ( item->handle ) {
		// if this is a dir node, check inner files here
		if ( !item->deleted ) {
			auto dirNode = item->getDirNode();
			if ( dirNode ) {
				item->resolved = true;
				bool thisResolved = item->resolved;
				if ( thisResolved ) {
					for ( size_t i = 0; i < dirNode->fileNames.size(); ++i ) {
						auto s = reinterpret_cast< const Settings* >( dirNode->fileNames[i].userData );
						if ( s ) {
							if ( !s->resolved || s->deleted ) {
								thisResolved = false;
								break;
							}
						}
					}
					for ( size_t i = 0; i < dirNode->subDirs.size(); ++i ) {
						auto s = reinterpret_cast< Settings* >( dirNode->subDirs[i]->userData );
						if ( s ) {
							if ( !s->resolved || s->deleted ) {
								thisResolved = false;
								break;
							}
						}
					}
				}
				item->resolved = thisResolved || ( dirNode->subDirs.empty() && dirNode->fileNames.empty() );
			}
		} else {
			item->resolved = false;
		}
		if ( item->deleted ) {
			m_FileTreeCtrl.SetItemImage( item->handle, TREE_ICON_DELETED, TREE_ICON_DELETED );
		} else {
			if ( item->resolved && item->getTypeName() == "DirectorySettings" ) {
				m_FileTreeCtrl.SetItemImage( item->handle, TREE_ICON_FOLDER, TREE_ICON_FOLDER );
			} else if ( item->resolved && item->getTypeName() == "FileSettings" ) {
				m_FileTreeCtrl.SetItemImage( item->handle, TREE_ICON_FILE, TREE_ICON_FILE );
			} else {
				m_FileTreeCtrl.SetItemImage( item->handle, TREE_ICON_UNKNOWN, TREE_ICON_UNKNOWN );
			}
		}
		String prefix = _getVisualString( *item );
		if ( !prefix.empty() ) {
			m_FileTreeCtrl.SetItemText( item->handle, ( prefix + " : " + item->getName() ).c_str() ); 
		}
		auto parentHandle = m_FileTreeCtrl.GetParentItem( item->handle );
		if ( parentHandle ) {
			auto s = reinterpret_cast< Settings* >( m_FileTreeCtrl.GetItemData( parentHandle ) );
			if ( s ) {
				UpdateTreeItemState( s );
			}
		}
	}
}

void CFilemanDlg::OnTreerightclickMarkasresolved()
{
	std::vector< Settings* > sels = GetCurrentSelSettingItems();
	if ( !sels.empty() ) {
		MarkDirty( TRUE );
	}
	std::for_each( sels.begin(), sels.end(),
		[&]( Settings* s ) {
			if ( !s->deleted ) {
				s->resolved = true;
			}
			UpdateTreeItemState( s );
		}
	);
}

void CFilemanDlg::OnTreerightclickResolveall()
{
	std::vector< Settings* > sels = GetCurrentSelSettingItems();
	if ( !sels.empty() ) {
		MarkDirty( TRUE );
	}
	_walkSettings( sels,
		[&]( Settings* s ) {
			if ( !s->deleted ) {
				s->resolved = true;
			}
			UpdateTreeItemState( s );
		}
	);
}

void CFilemanDlg::OnTreerightclickMarkasunresolved()
{
	std::vector< Settings* > sels = GetCurrentSelSettingItems();
	if ( !sels.empty() ) {
		MarkDirty( TRUE );
	}
	std::for_each( sels.begin(), sels.end(),
		[&]( Settings* s ) {
			if ( !s->deleted ) {
				s->resolved = false;
			}
			UpdateTreeItemState( s );
		}
	);
}

void CFilemanDlg::OnTreerightclickUnresolveall()
{
	std::vector< Settings* > sels = GetCurrentSelSettingItems();
	if ( !sels.empty() ) {
		MarkDirty( TRUE );
	}
	_walkSettings( sels,
		[&]( Settings* s ) {
			s->resolved = false;
			UpdateTreeItemState( s );
		}
	);
}

void CFilemanDlg::OnTreerightclickResolvechildren()
{
	std::vector< Settings* > sels = GetCurrentSelSettingItems();
	if ( !sels.empty() ) {
		MarkDirty( TRUE );
	}
	_walkSettings( sels,
		[&]( Settings* s ) {
			if ( !s->deleted ) {
				s->resolved = true;
			}
			UpdateTreeItemState( s );
		},
		0
	);
}

void CFilemanDlg::OnTreerightclickUnresolvechildren()
{
	std::vector< Settings* > sels = GetCurrentSelSettingItems();
	if ( !sels.empty() ) {
		MarkDirty( TRUE );
	}
	_walkSettings( sels,
		[&]( Settings* s ) {
			if ( !s->deleted ) {
				s->resolved = false;	
			}
			UpdateTreeItemState( s );
		},
		0
	);
}


void CFilemanDlg::UpdateTreeItemStateAll()
{	
	if ( m_FileTreeCtrl.GetRootItem() ) {
		_walkTreeItems( m_FileTreeCtrl, m_FileTreeCtrl.GetRootItem(),
			[ this ]( HTREEITEM item ) {
				UpdateTreeItemState( reinterpret_cast< Settings* >( m_FileTreeCtrl.GetItemData( item ) ) );
			}
		);
	}
	if ( m_FileTreeCtrl.GetRootItem() ) {
		m_FileTreeCtrl.Expand( m_FileTreeCtrl.GetRootItem(), TVM_EXPAND | TVE_EXPANDPARTIAL );
	}
}


BOOL CFilemanDlg::UpdateTree( WinUtil::DirTreeNode& data, CTreeCtrl& tree )
{
	using WinUtil::DirTreeNode;
	tree.DeleteAllItems();
	std::vector< DirTreeNode* > stack;
	std::vector< HTREEITEM > handleStack;
	stack.push_back( &data );
	handleStack.push_back( NULL );
	while ( !stack.empty() ) {
		DirTreeNode* cur = stack.back();
		stack.pop_back();
		HTREEITEM parent = handleStack.back();
		handleStack.pop_back();
		parent = tree.InsertItem( cur->curDir.c_str(), TREE_ICON_UNKNOWN, TREE_ICON_UNKNOWN, parent );
		tree.SetItemData( parent, ( DWORD_PTR )cur->userData );
		Settings* settings = reinterpret_cast< Settings* >( cur->userData );
		assert( settings );
		settings->setDirNode( cur );
		settings->handle = parent;
		for ( size_t i = 0; i < cur->subDirs.size(); ++i ) {
			handleStack.push_back( parent );
			stack.push_back( cur->subDirs[i] );
		}
		for ( size_t i = 0; i < cur->fileNames.size(); ++i ) {
			WinUtil::FileItem& fi = cur->fileNames[i];
			Settings* settings = reinterpret_cast< Settings* >( fi.userData );
			assert( settings );
			settings->setFileNode( cur );
			HTREEITEM hChild = tree.InsertItem( fi.name.c_str(), TREE_ICON_UNKNOWN, TREE_ICON_UNKNOWN, parent );
			settings->handle = hChild;
			tree.SetItemData( hChild, ( DWORD_PTR )fi.userData );
		}
	}
	return TRUE;
}

BOOL CFilemanDlg::CreateTree( WinUtil::DirTreeNode& data, CTreeCtrl& tree )
{
	tree.DeleteAllItems();
	using WinUtil::DirTreeNode;
	std::vector< DirTreeNode* > stack;
	stack.push_back( &data );
	HTREEITEM parent = NULL;
	while ( !stack.empty() ) {
		DirTreeNode* cur = stack.back();
		Settings* settings = new Settings( cur->curDir, Settings::Directory );
		settings->setDirNode( cur );
		assert( cur->userData == NULL );
		cur->userData = settings;
		stack.pop_back();
		settings->setTypeName( "DirectorySettings" );
		HTREEITEM parent = _ParentHandleFromNode( cur );
		cur->userData = settings;
		parent = tree.InsertItem( cur->curDir.c_str(), TREE_ICON_UNKNOWN, TREE_ICON_UNKNOWN, parent );
		settings->handle = parent;
		tree.SetItemData( parent, ( DWORD_PTR )settings );
		for ( size_t i = 0; i < cur->subDirs.size(); ++i ) {
			stack.push_back( cur->subDirs[i] );
		}
		for ( size_t i = 0; i < cur->fileNames.size(); ++i ) {
			WinUtil::FileItem& fi = cur->fileNames[i];
			Settings* settings = new Settings( fi.name, Settings::File );
			settings->setFileNode( cur );
			assert( fi.userData == NULL );
			fi.userData = settings;
			settings->setTypeName( "FileSettings" );
			HTREEITEM hChild = tree.InsertItem( fi.name.c_str(), TREE_ICON_UNKNOWN, TREE_ICON_UNKNOWN, parent );
			settings->handle = hChild;
			tree.SetItemData( hChild, ( DWORD_PTR )settings );
		}
	}
	return TRUE;
}

void CFilemanDlg::OnTreeeditDelete()
{
	OnTreerightclickDelete();
}


BOOL CFilemanDlg::ResetTree( WinUtil::DirTreeNode& data, CTreeCtrl& tree, CMFCPropertyGridCtrl& ui )
{
	if ( tree.GetRootItem() ) {
		tree.Select( tree.GetRootItem(), 0 );
		HTREEITEM hItem = tree.GetRootItem();
		Settings* setting = ( Settings* )tree.GetItemData( hItem );
		if ( hItem ) {
			tree.Expand( hItem, TVE_COLLAPSERESET );
			UpdateProperty( *setting, ui );
		}
	}
	return TRUE;
}



void CFilemanDlg::OnTreerightclickOpen()
{
	std::vector< Settings* > vSettings = GetCurrentSelSettingItems();
	if ( !vSettings.empty() ) {
		String path = StringUtil::standardisePath( m_CurrentOpenPathName.GetBuffer(0) );
		for ( size_t i = 0; i < vSettings.size(); ++i ) {
			const Settings* p = vSettings[i];
			if ( p->isDir() ) {
				
				String _path = path + p->getPath();
				std::replace( _path.begin(), _path.end(), '/', '\\' );
				//system( ( String( "explorer " ) + _path ).c_str() );
				ShellExecute( NULL, "open", "explorer.exe",  _path.c_str(), NULL, SW_SHOW );
			} else if ( p->isFile() ) {
				String _path = path + p->_getPathName();
				std::replace( _path.begin(), _path.end(), '/', '\\' );
				ShellExecute( 0, 0, _path.c_str(), 0, 0 , SW_SHOW );
				//system( ( String( "explorer /select, " ) + _path ).c_str() );
				//ShellExecute( 0, "open", L"c:\\windows\\notepad.exe" ,L"c:\\outfile.txt" , 0 , SW_SHOW );
			}
		}
	}
}

void CFilemanDlg::OnTreerightclickDelete()
{
	typedef std::pair< int, Settings* > value_t;
	std::vector< value_t > _settings;
	std::vector< Settings* > vSettings = GetCurrentSelSettingItems();
	if ( !vSettings.empty() ) {
		MarkDirty();
	}
	std::for_each( vSettings.begin(), vSettings.end(),
		[&]( Settings* s ) {
			_settings.push_back(
				std::make_pair(
					[&]() {
						auto cur = s->handle;
						auto ret = 0;
						while ( cur ) {
							++ret;
							cur = m_FileTreeCtrl.GetParentItem( cur );
						}
						return ret;
					}(),
					s
				)
			);
		}
	);
	// more deep node put near at the list' top
	std::sort( _settings.begin(), _settings.end(),
		[]( const value_t& left, const value_t& right ) {
			return left.first > right.first;
		}
	);
	auto a = _settings;
	std::for_each( _settings.begin(), _settings.end(),
		[&]( value_t& s ) {
			Settings* settings = s.second;
			auto handle = settings->handle;
			if ( settings->getType() == Settings::File ) {
				// leaf node: file node
				// tree item must be a leaf also
				assert( m_FileTreeCtrl.ItemHasChildren( handle ) == false );
				auto parent = m_FileTreeCtrl.GetParentItem( handle );
				if ( parent ) {
					Settings* ps = reinterpret_cast< Settings* >( m_FileTreeCtrl.GetItemData( parent ) );
					auto dirNode = ps->getDirNode();
					assert( dirNode );
					auto it = std::find_if( dirNode->fileNames.begin(), dirNode->fileNames.end(),
						[ settings ]( const WinUtil::FileItem& fi ) {
							return fi.userData == ( void* )settings;
						}
					);
					if ( it != dirNode->fileNames.end() ) {
						// remove user data
						_DirTreeNode_UserData_Free( it->userData, false );
						dirNode->fileNames.erase( it );
					}
					m_FileTreeCtrl.DeleteItem( handle );
				}
			} else {
				// parent node
				auto dirNode = settings->getDirNode();
				if ( dirNode->parent ) {
					auto parentNode = dirNode->parent;
					auto it = std::find( parentNode->subDirs.begin(), parentNode->subDirs.end(), dirNode );
					if ( it != parentNode->subDirs.end() ) {
						parentNode->subDirs.erase( it );
						WinUtil::DirTreeNode* node = dirNode;
						WinUtil::DirTreeNode::clearRecur( dirNode, _DirTreeNode_UserData_Free );
						delete node;
						m_FileTreeCtrl.DeleteItem( handle );
					}
				} else {
					// root node
					assert( &m_FileSettingTree == settings->getDirNode() );
					m_FileSettingTree.clear( _DirTreeNode_UserData_Free );
					m_FileTreeCtrl.DeleteItem( handle );
				}
			}
		}
	);
	if ( m_FileTreeCtrl.GetRootItem() ) {
		UpdateTreeItemState( reinterpret_cast< Settings* >( m_FileTreeCtrl.GetItemData( m_FileTreeCtrl.GetRootItem() ) ) );
	}
}
