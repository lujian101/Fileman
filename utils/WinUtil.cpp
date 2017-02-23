#include "WinUtil.h"
#include "StringUtil.h"
#include <shlwapi.h>
#include <process.h>
#include <shlobj.h>
#include <algorithm>
#pragma comment( lib, "shlwapi.lib" )
#pragma comment( lib, "shell32.lib" )

namespace WinUtil
{
	BOOL getPathDlg( TCHAR path[MAX_PATH], HWND hWnd, LPCTSTR title )
	{	
		BOOL ret = FALSE;
		if( path == NULL || title == NULL ) return FALSE;
		path[0] = 0;
		LPITEMIDLIST p = NULL;
		LPITEMIDLIST pidlBeginAt = NULL;
		BROWSEINFO BrInfo;
		ZeroMemory( &BrInfo,sizeof(BrInfo) );
		BrInfo.hwndOwner = hWnd;
		BrInfo.lpszTitle = title;
		BrInfo.pidlRoot  = pidlBeginAt;
		BrInfo.ulFlags = BIF_USENEWUI;
		SHGetSpecialFolderLocation( hWnd, 0, &pidlBeginAt );
		p = SHBrowseForFolder( &BrInfo );
		if ( p ) 
			ret = SHGetPathFromIDList( p, path );
		IMalloc * imalloc = 0;
		if ( SUCCEEDED( SHGetMalloc ( &imalloc ) ) )
		{
			imalloc->Free ( p );
			imalloc->Release ();
		}
		return ret;
	}

	namespace
	{
		static std::vector< TCHAR* > __pathStack;
	}

	BOOL pushPath( LPCTSTR path )
	{
		if ( !path )
			return FALSE;
		if ( !__pathStack.size() )
		{
			TCHAR* str = new TCHAR[ MAX_PATH ];
			GetCurrentDirectory( MAX_PATH, str );
			__pathStack.push_back( str );
		} 
		else
		{
			TCHAR* oldPath = __pathStack.back();
			TCHAR* str = new TCHAR[ MAX_PATH ];
			StrCpy( str, oldPath );
			__pathStack.push_back( str );
		}
		return SetCurrentDirectory( path );
	}

	BOOL popPath()
	{
		if ( !__pathStack.size() ) return FALSE;
		SetCurrentDirectory( __pathStack.back() );
		delete [] __pathStack.back();
		__pathStack.pop_back();
		return TRUE;
	}

	BOOL  getFileLists( std::vector< String >& fnlist, LPCTSTR path,
		BOOL filterfunc( LPCTSTR name, LPCTSTR fullName ),
		void addFileCallback ( LPCTSTR name, LPCTSTR fullName ) )
	{
		TCHAR OldPath[ MAX_PATH ];
		if ( path[0] != 0 ) {
			GetCurrentDirectory( MAX_PATH, OldPath );
			BOOL ret = SetCurrentDirectory( path );
			if ( !ret )
				return FALSE;
		}
		WIN32_FIND_DATA fileinfo;
		HANDLE handle = FindFirstFile( TEXT("*.*"), &fileinfo );
		if ( handle != 0 ) { 
			do {
				if ( !( fileinfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) ) {
					String full_path( StringUtil::standardisePath( path ) );
					if ( filterfunc( fileinfo.cFileName, full_path.c_str() ) ) {
						if ( addFileCallback != NULL ) {
							addFileCallback( fileinfo.cFileName, full_path.c_str() );
						}
						full_path.append( fileinfo.cFileName );
						fnlist.push_back( full_path );
					}
				} else if ( fileinfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
					TCHAR* p = fileinfo.cFileName;
					if ( p && *p != TCHAR('.') ) {
						String child_path( StringUtil::standardisePath( path ) );
						String full_path = child_path + fileinfo.cFileName;
						if ( filterfunc( fileinfo.cFileName, full_path.c_str() ) ) {
							getFileLists( fnlist, child_path.c_str(), filterfunc, addFileCallback );
						}
					}
				}
			}
			while ( FindNextFile( handle, &fileinfo ) );
		}
		if ( path[0] != 0 ) {
			SetCurrentDirectory( OldPath );
		}
		return fnlist.size() ? TRUE : FALSE;
	}

	BOOL  getFileLists( DirTreeNode& curDir, LPCTSTR path,
		BOOL filterfunc( LPCTSTR name, LPCTSTR fullPath ),
		void addFileCallback( LPCTSTR name, LPCTSTR fullPath ),
		std::vector< FileDirInfo >* fullPathFileNameList )
	{
		TCHAR OldPath[ MAX_PATH ];
		if ( path[0] != 0 ) {
			GetCurrentDirectory( MAX_PATH, OldPath );
			BOOL ret = SetCurrentDirectory( path );
			if ( !ret )
				return FALSE;
		}
		WIN32_FIND_DATA fileinfo;
		HANDLE handle = FindFirstFile( TEXT("*.*"), &fileinfo );
		if ( handle != 0 ) { 
			do {
				if ( !( fileinfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) ) {
					String full_path( StringUtil::standardisePath( path ) );
					full_path.append( fileinfo.cFileName );
					if ( filterfunc( fileinfo.cFileName, full_path.c_str() ) ) {
						if ( addFileCallback != NULL ) {
							addFileCallback( fileinfo.cFileName, full_path.c_str() );
						}
						curDir.fileNames.push_back( String( fileinfo.cFileName ) );
						if ( fullPathFileNameList ) {
							fullPathFileNameList->push_back( FileDirInfo() );
							fullPathFileNameList->back().info = &curDir;
							fullPathFileNameList->back().name = full_path;
						}
					}
				} else if ( fileinfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
					TCHAR* p = fileinfo.cFileName;
					if ( p && *p != TCHAR('.') ) {
						String child_path( StringUtil::standardisePath( path ) );
						String full_path = child_path + fileinfo.cFileName;
						if ( filterfunc( fileinfo.cFileName, full_path.c_str() ) ) {
							DirTreeNode* subDirNode = new DirTreeNode;
							subDirNode->curDir = fileinfo.cFileName;
							subDirNode->parent = &curDir;
							curDir.subDirs.push_back( subDirNode );
							getFileLists( *subDirNode, full_path.c_str(), filterfunc, addFileCallback, fullPathFileNameList );
						}
					}
				}
			}
			while ( FindNextFile( handle, &fileinfo ) );
		}
		if ( path[0] != 0 )
			SetCurrentDirectory( OldPath );
		return TRUE;
	}
	
	void DirTreeNode::sort()
	{
		DirTreeNode* cur = this;
		std::vector< DirTreeNode* > stack;
		stack.push_back( cur );
		while ( !stack.empty() ) {
			DirTreeNode* cur = stack.back();
			std::sort( cur->fileNames.begin(), cur->fileNames.end() );
			std::sort( cur->subDirs.begin(), cur->subDirs.end(),
				[]( const DirTreeNode* l, const DirTreeNode* r ) {
					return l->curDir >= r->curDir;
				}
			);
			stack.pop_back();
			for ( size_t i = 0; i < cur->subDirs.size(); ++i ) {
				stack.push_back( cur->subDirs[i] );
			}
		}
	}

	void DirTreeNode::swap( DirTreeNode& other )
	{
		if ( this != &other ) {
			std::swap( curDir, other.curDir );
			std::swap( subDirs, other.subDirs );
			std::swap( fileNames, other.fileNames );
			std::swap( parent, other.parent );
			std::swap( userData, other.userData );
			std::for_each( subDirs.begin(), subDirs.end(), [this]( DirTreeNode* n ){ n->parent = this; } );
			std::for_each( other.subDirs.begin(), other.subDirs.end(), [&other]( DirTreeNode* n ){ n->parent = &other; } );
		}
	}

	void DirTreeNode::clear( FuncFreeUserData freeFunc )
	{
		clearRecur( this, freeFunc );
	}

	size_t DirTreeNode::fileNum() const
	{
		size_t num = 0;
		getFileNumRecur( num, this );
		return num;
	}

	size_t DirTreeNode::dirNum() const
	{
		size_t num = 0;
		getDirNumRecur( num, this );
		return num;
	}

	void DirTreeNode::clearRecur( DirTreeNode* cur, FuncFreeUserData freeFunc )
	{
		if ( freeFunc ) {
			for ( size_t i = 0; i < cur->fileNames.size(); ++i ) {
				const FileItem& fi = cur->fileNames[i];
				freeFunc( fi.userData, false );
			}
			freeFunc( cur->userData, true );
			cur->userData = NULL;
		}
		cur->fileNames.clear();
		std::vector< DirTreeNode* >::iterator it, end;
		it = cur->subDirs.begin();
		end = cur->subDirs.end();
		while ( it != end ) {
			clearRecur( *it, freeFunc );
			delete *it++;
		}
		cur->subDirs.clear();
	}

	void DirTreeNode::getFileNumRecur( size_t& num, const DirTreeNode* cur ) const
	{
		num += cur->fileNames.size();
		std::vector< DirTreeNode* >::const_iterator it, end;
		it = cur->subDirs.begin();
		end = cur->subDirs.end();
		while ( it != end ) {
			getFileNumRecur( num, *it++ );
		}
	}

	void DirTreeNode::getDirNumRecur( size_t& num, const DirTreeNode* cur ) const
	{
		num += cur->subDirs.size();
		std::vector< DirTreeNode* >::const_iterator it, end;
		it = cur->subDirs.begin();
		end = cur->subDirs.end();
		while ( it != end ) {
			getDirNumRecur( num, *it++ );
		}
	}

	const FileItem* DirTreeNode::findFileWithPath( const String& path, const String& fileName ) const
	{
		const DirTreeNode* dir = findDir( path );
		if ( dir ) {
			for ( size_t i = 0; i < dir->fileNames.size(); ++i ) {
				if ( fileName == dir->fileNames[i].name ) {
					return &dir->fileNames[i];
				}
			}
		}
		return NULL;
	}

	const FileItem* DirTreeNode::findFile( const String& fileName, const std::vector< String >* excludeDirs ) const
	{
		return findFileRecur( fileName, this, excludeDirs );
	}

	const DirTreeNode* DirTreeNode::findDir( const String& dirName, const std::vector< String >* excludeDirs ) const
	{
		std::vector< String > seg;
		StringUtil::split( seg, dirName, "/\\" );
		const DirTreeNode* cur = this;
		if ( !seg.empty() ) {
			size_t i = 0;
			if ( cur->curDir != seg[0] ) {
				return NULL;
			}
			for ( i = 1; i < seg.size() && cur; ++i ) {
				bool found = false;
				for ( size_t k = 0; k < cur->subDirs.size(); ++k ) {
					if ( cur->subDirs[k]->curDir == seg[i] ) {
						cur = cur->subDirs[k];
						found = true;
						break;
					}
				}
				if ( !found ) {
					cur = NULL;
					break;
				}
			}
			return cur;
		} else {
			return NULL;
		}
	}

	const FileItem*	DirTreeNode::findFileRecur( const String& name, const DirTreeNode* cur, const std::vector< String >* excludeDirs ) const
	{
		if ( excludeDirs ) {
			if ( excludeDirs->end() != std::find( excludeDirs->begin(), excludeDirs->end(), name ) )
				return NULL;
		}
		std::vector< FileItem >::const_iterator it = std::find( cur->fileNames.begin(), cur->fileNames.end(), name );
		if ( it != cur->fileNames.end() ) {
			return &*it;
		}
		std::vector< DirTreeNode* >::const_iterator subIt, subEnd;
		subIt = cur->subDirs.begin();
		subEnd = cur->subDirs.end();
		while ( subIt != subEnd ) {
			const FileItem* node = findFileRecur( name, *subIt++, excludeDirs );
			if ( node )
				return node;
		}
		return NULL;
	}
}
//EOF
