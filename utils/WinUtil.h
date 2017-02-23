#ifndef __WIN_UTILS_H__
#define __WIN_UTILS_H__

#pragma once

#include <Windows.h>
#include <tchar.h>
#include <string>
#include <vector>

#ifdef UNICODE
	typedef std::wstring	String;
#else
	typedef std::string		String;
#endif

#include "Rflext.h"

namespace WinUtil
{
	struct FileItem
	{
		String name;
		void* userData;
		FileItem() : userData( NULL ) {}
		FileItem( const String& s ) : name( s ), userData( NULL ) {}
		operator String() { return name; }
		bool operator == ( const FileItem& o ) const { return name == o.name; }
		bool operator < ( const FileItem& o ) const { return name < o.name; }
	};
	class DirTreeNode
	{
	public:
		typedef void ( *FuncFreeUserData )( void* userData, bool isDir );

		String						curDir;
		std::vector< DirTreeNode* >	subDirs;
		std::vector< FileItem >		fileNames;
		DirTreeNode*				parent;
		void*						userData;
		DirTreeNode() : parent( NULL ), userData( NULL ) { }
		~DirTreeNode()	{ clear(); }
	public:
		void				sort();
		void				swap( DirTreeNode& other );
		void				clear( FuncFreeUserData freeFunc = NULL );
		size_t				fileNum() const;
		size_t				dirNum() const;
		const FileItem*		findFileWithPath( const String& path, const String& fileName ) const;
		const FileItem*		findFile( const String& fileName, const std::vector< String >* excludeDirs = NULL ) const;
		const DirTreeNode*	findDir( const String& dirName, const std::vector< String >* excludeDirs = NULL ) const;
		static void			clearRecur( DirTreeNode* cur, FuncFreeUserData freeFunc );
	private:
		void				getFileNumRecur( size_t& num, const DirTreeNode* cur ) const ;
		void				getDirNumRecur( size_t& num, const DirTreeNode* cur ) const;
		const FileItem*		findFileRecur( const String& name, const DirTreeNode* cur, const std::vector< String >* excludeDirs = NULL ) const;
	};

	struct FileDirInfo
	{
		String				name;
		const DirTreeNode*	info;
	};


	BOOL getPathDlg( TCHAR path[MAX_PATH], HWND hWnd, LPCTSTR title );
	BOOL pushPath( LPCTSTR path );
	BOOL popPath();
	BOOL getFileLists( std::vector< String >& fnlist, LPCTSTR path, BOOL filterfunc( LPCTSTR name, LPCTSTR fullName ), void addFileCallback( LPCTSTR name, LPCTSTR fullName ) = NULL  );
	BOOL getFileLists( DirTreeNode& curDir, LPCTSTR path, BOOL filterfunc( LPCTSTR name, LPCTSTR fullName ), void addFileCallback( LPCTSTR name, LPCTSTR fullName ) = NULL, std::vector< FileDirInfo >* fullPathFileNameList = NULL );

}
#endif
//EOF

