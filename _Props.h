void CFilemanDlg::UpdateProperty( const Settings& settings, CMFCPropertyGridCtrl& control,
								 std::vector< String >* excludePropNames,
								 std::vector< String >* memberOwnerNames )
{
	control.RemoveAll();
	std::vector< CMFCPropertyGridProperty* > propItems;
	const rflext::TRflxObject* propObj = settings.getPropObj();
	while ( propObj ) {
		bool skip = false;
		if ( memberOwnerNames ) {
			if ( memberOwnerNames->end() == std::find( memberOwnerNames->begin(), memberOwnerNames->end(), propObj->getName() ) ) {
				skip = true;
			}
		}
		if ( !skip ) {
			CMFCPropertyGridProperty* title = new MyPropertyGridProperty( propObj->getName() );
			propItems.push_back( title );
			TRflxObject::PropTable_ConstIt it = propObj->getProps().begin();
			while ( it != propObj->getProps().end() ) {
				const PropDefExt* pdf = it->second;
				++it;
				bool enable = true;
				if ( excludePropNames ) {
					if ( excludePropNames->end() != std::find( excludePropNames->begin(), excludePropNames->end(), pdf->name ) ) {
						enable = false;
					}
				}
				CMFCPropertyGridProperty* uiItem = NULL;
				if ( enable ) {
					uiItem = _createMFCPropFromType( pdf->name, pdf->type, &( pdf->valueHolder ), pdf->descriptionHolder.c_str(), pdf->editorDataHolder.c_str() );
				} else {
					uiItem = new MyPropertyGridProperty( pdf->name, ( _variant_t )"...", pdf->descriptionHolder.c_str() );
					uiItem->Enable( enable );
				}
				title->AddSubItem( uiItem );
			}
		}
		propObj = propObj->getBaseObj();
	}
	if ( !propItems.empty() ) {
		std::for_each( propItems.rbegin(), propItems.rend(),
			[&]( CMFCPropertyGridProperty* p ) {
				control.AddProperty( p );
			}
		);
		control.ExpandAll( TRUE );
	}
}

static void _applyValueToWithLog( const String& propName, const ValueData& from, Settings& to )
{						
	CString info;
	ValueData modFrom;
	auto oldVal = _applyValueTo( propName, from, to, modFrom );
	std::string _oldVal;
	std::string _newVal;
	oldVal.toString( _oldVal );
	modFrom.toString( _newVal );
	const char* pathName = to._getPathName();
	info.Empty();
	info.Format( "%s, %s = %s -> %s", pathName, propName.c_str(), _oldVal.c_str(), _newVal.c_str() );
	_AppendLog( info );
}

LRESULT CFilemanDlg::OnPropertyChanged( WPARAM wParam, LPARAM lParam )
{
	CMFCPropertyGridProperty* propUi = ( CMFCPropertyGridProperty* )lParam;
	if ( propUi ) {
		std::vector< Settings* > vSettings = CFilemanDlg::GetCurrentSelSettingItems();
		std::set< Settings* > _settings;
		if ( !vSettings.empty() ) {
			MarkDirty( TRUE );
			rflx::ValueData newVal = _convert_COleVariant( propUi->GetValue() );
			const char* name = propUi->GetName();
			String newValStr;
			newVal.toString( newValStr );
			std::string propName = name;
			for ( size_t i = 0; i < vSettings.size(); ++i ) {
				Settings* s = vSettings[i];
				if ( g_RecurProps ) {
					_settings.insert( s );
				}
				_applyValueToWithLog( name, newVal, *s );
				int maxDepth = g_RecurPropDepth < 0 ? 0x7fffffff : g_RecurPropDepth;
				if ( g_RecurProps && maxDepth > 0 ) {
					// current node is a dir?
					CString info;
					auto dirNode = s->getDirNode();
					if ( dirNode != NULL ) {
						std::vector< WinUtil::DirTreeNode* > stack;
						std::vector< int > stack_depth;
						stack.push_back( dirNode );
						stack_depth.push_back( 0 );
						while ( !stack.empty() ) {
							int curDepth = stack_depth.back();
							WinUtil::DirTreeNode* curDir = stack.back();
							stack.pop_back();				
							stack_depth.pop_back();
							for ( const WinUtil::FileItem& fi : curDir->fileNames ) {
								Settings* settings = reinterpret_cast< Settings* >( fi.userData );
								if ( settings != NULL ) {
									_settings.insert( settings );
									_applyValueToWithLog( propName, newVal, *settings );
								}
							}
							++curDepth;
							if ( curDepth < maxDepth ) {
								for ( auto di : curDir->subDirs ) {
									Settings* settings = reinterpret_cast< Settings* >( di->userData );
									_applyValueToWithLog( propName, newVal, *settings );
									stack.push_back( di );
									stack_depth.push_back( curDepth );
								}
							} else {
								for ( auto di : curDir->subDirs ) {
									Settings* settings = reinterpret_cast< Settings* >( di->userData );
									_applyValueToWithLog( propName, newVal, *settings );
								}
							}
						}
					}
				}
			}
		}
		if ( !_settings.empty() ) {
			vSettings.clear();
			for ( auto s : _settings ) {
				vSettings.push_back( s );
			}
		}
		UpdateTreeItemState( vSettings );
	}
	return 0L;
}
