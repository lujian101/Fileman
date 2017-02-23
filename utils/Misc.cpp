#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <string>
#include <errno.h>
#include <sys/stat.h>
#include <assert.h>
#include <vector>
#include <algorithm>
#include <fstream>
#include "StringUtil.h"

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <direct.h>
	#include <time.h>
	#include "_dirent.h"
#else // _WIN32
	#include <sys/stat.h>
	#include <dirent.h>
	#include <sys/time.h>
	#include <unistd.h>
#endif // _WIN32
#include "Misc.h"

#pragma warning( disable : 4996 )

#ifdef _WIN32
	#ifndef mkdir
		#define mkdir( n, m ) _mkdir( n ) // discart the attributes flags for VS
	#endif
	#ifndef rmdir
		#define rmdir _rmdir
	#endif
	#ifndef chdir
		#define chdir _chdir
	#endif
#endif

enum TreeMovingState {
	None,
	TreeIsMoving,
	TreeMovingBreak,
};

static TreeMovingState _TMState = None;

bool Misc::isTreeMoving()
{
	return _TMState == TreeIsMoving;
}

bool Misc::terminateTreeMoving()
{
	if ( _TMState == TreeIsMoving ) {
		_TMState = TreeMovingBreak;
		return true;
	}
	return false;
}


bool Misc::TreeMoveReporter::copyFile( const char* src, const char* dst, bool overwrite )
{
	return Misc::copyFile( src, dst, overwrite );
}

Misc::OpResult Misc::treeMove( const char* src, const char* dst, const char* srcRoot, bool overwrite, TreeMoveReporter* rep )
{
	OpResult ret;
	if ( !isDirectory( src ) ) {
		return ret;
	}
	if ( !isDirectory( dst ) ) {
		ret.opCount = 1;
		ret.statusCount[ createDirectories( dst ) ]++;
		if ( ret.failedCount ) {
			return ret;
		}
	}
	struct _treeWalker : TreeWalker {
		std::string dst;
		std::string srcRoot;
		OpResult* ret;
		TreeMoveReporter* reporter;
		bool overwrite;
		_treeWalker( const char* _dst, const char* _srcRoot, OpResult* r, bool o, TreeMoveReporter* rep ) :
		dst( _dst ), ret( r ), overwrite( o ), reporter( rep ) {
			if ( _srcRoot ) {
				srcRoot = _srcRoot;
			}
			if ( dst[ dst.size() - 1 ] != '/' &&
				dst[ dst.size() - 1 ] != '\\' ) {
				dst.push_back( '/' );
			}
		}
		virtual bool operator()( bool isdir, const char* name ) {
			if ( !isdir ) {
				const char* sep = NULL;
				if ( srcRoot.empty() ) {
					// remove the first dir name
					// or dirver name if it's a abs path
					// example:
					// in: F:/A/B/C, A/B/C
					// out: A/B/C, B/C
					sep = strchr( name, '/' );
					if ( !sep ) {
						sep = strchr( name, '\\' );
					}
					if ( sep ) {
						++sep; // remove the last '/'
					}
				} else {
					// cut from the leading dir name
					// example:
					// in A/B/C, srcRoot = B
					// out: C
					sep = strstr( name, srcRoot.c_str() );
					if ( sep ) {
						sep += srcRoot.length();
						++sep; // remove the last '/'
					}
				}
				// make the output file path
				std::string dstFilePath = dst + ( sep ? sep : name );
				// make the output dir name
				std::string dirPath( dstFilePath );
				const char* sep2 = strrchr( dirPath.c_str(), '/' );
				if ( !sep2 ) {
					sep2 = strrchr( dirPath.c_str(), '\\' );
				}
				if ( sep2 ) {
					// must be here to test the dst dir if exists
					dirPath.resize( sep2 + 1 - dirPath.c_str() );
					if ( !isDirectory( dirPath.c_str() ) ) {
						ret->opCount++;
						ret->statusCount[ createDirectories( dirPath.c_str() ) ]++;
					}
				}
				if ( !reporter || !reporter->filter( name ) ) {
					ret->opCount++;
					bool r = false;
					if ( reporter ) {
						r = reporter->copyFile( name, dstFilePath.c_str(), overwrite );
					} else {
						r = copyFile( name, dstFilePath.c_str(), overwrite );
					}
					ret->statusCount[ r ]++;
					if ( reporter ) {
						( *reporter )( dstFilePath.c_str(), r );
					}
				}
			}
			return _TMState != TreeMovingBreak;
		}
	};
	{
		{
			_TMState = TreeIsMoving;
		}
        _treeWalker _tw( dst, srcRoot, &ret, overwrite, rep );
		treeWalk( src, _tw );
		{
			_TMState = None;
		}
		return ret;
	}
}

bool Misc::readFileListFile( const char* listFile, FileList* fileList )
{
	if ( !listFile || listFile[0] == '\0' || !fileList || !Misc::isFile( listFile ) ) {
		return false;
	}
	Misc::ByteBuffer buf;
	if ( Misc::readFileAll( listFile, buf ) ) {
		return readFileListFileFromMemory( buf, fileList );
	} else {
		return false;
	}
}

bool Misc::readFileListFileFromMemory( const Misc::ByteBuffer& buf, FileList* fileList )
{
	if ( !fileList || buf.empty() ) {
		return false;
	}
	fileList->fileNum = 0;
	fileList->totalSize = 0;
	fileList->filePathList.clear();
	const char* s = &*buf.begin();
	std::string line;
	std::vector< std::string >& lines = fileList->filePathList;
	const char* p = s;
	while ( *p != '\0' ) {
		// skip writespace
		while ( *p == ' ' || *p == '\t' ) { 
			++p;
			continue;
		}
		// read one line
		while ( *p != '\r' && *p != '\n' && *p != '\0' ) {
			line.push_back( *p++ );
		}
		if ( !line.empty() ) {
			if ( fileList->fileNum == 0 ) {
				std::vector< std::string > params;
				StringUtil::split( params, line, ", " );
				if ( params.size() >= 2 ) {
					sscanf( params[0].c_str(), "%d", &fileList->fileNum );
					typedef char test_long_long_size[ sizeof( long long ) == 8 ];
					sscanf( params[1].c_str(), "%lld", &fileList->totalSize );		
					lines.reserve( fileList->fileNum );
				}
			} else {
				lines.push_back( line );
			}
			line.clear();
		}
		++p;
	}
	if ( !line.empty() ) {
		lines.push_back( line );
	}
	return true;
}

Misc::OpResult Misc::treeMoveWithList( const FileList& fileList, const char* src, const char* dst, const char* dstRoot, bool overwrite, TreeMoveReporter* rep )
{
	OpResult ret;
	if ( !isDirectory( dst ) ) {
		ret.opCount = 1;
		ret.statusCount[ createDirectories( dst ) ]++;
		if ( ret.failedCount ) {
			return ret;
		}
	}

	std::string filePath;
	if ( src && src[0] != '\0' ) {
		filePath.append( src );
		if ( !filePath.empty() ) {
			filePath = StringUtil::standardisePath( filePath );
		}
	}
	std::string dstFilePath;
	if ( dst && dst[0] != '\0' ) {
		dstFilePath.append( dst );
	}	
	dstFilePath = StringUtil::standardisePath( dstFilePath );
	if ( dstRoot && dstRoot[0] != '\0' ) {
		dstFilePath.append( dstRoot );
	}
	dstFilePath = StringUtil::standardisePath( dstFilePath );
	{
		_TMState = TreeIsMoving;
	}
	for ( size_t i = 0; i < fileList.filePathList.size(); ++i ) {
		{
			if ( _TMState == TreeMovingBreak ) {
				break;
			}
		}
		std::string _filePath( filePath );
		_filePath.append( fileList.filePathList[i] );
		std::string _dstFilePath( dstFilePath );
		_dstFilePath.append( fileList.filePathList[i] );
		
		// cut the filename
		std::string dirPath( _dstFilePath );
		const char* sep = strrchr( dirPath.c_str(), '/' );
		if ( !sep ) {
			strrchr( dirPath.c_str(), '\\' );
		}
		if ( sep ) {
			// must be here to test the dst dir if exists
			dirPath.resize( sep + 1 - dirPath.c_str() );
			if ( !isDirectory( dirPath.c_str() ) ) {
				ret.opCount++;
				ret.statusCount[ createDirectories( dirPath.c_str() ) ]++;
			}
		}
		ret.opCount++;
		bool r = false;
		if ( rep ) {
			r = rep->copyFile( _filePath.c_str(), _dstFilePath.c_str(), overwrite );
		} else {
			r = copyFile( _filePath.c_str(), _dstFilePath.c_str(), overwrite );
		}
		ret.statusCount[ r ]++;
		if ( rep ) {
			( *rep )( _filePath.c_str(), r );
		}
	}
	{
		_TMState = None;
	}
	return ret;
}

Misc::OpResult Misc::treeMoveWithList( const char* listFile, const char* src, const char* dst, const char* dstRoot, bool overwrite, TreeMoveReporter* rep )
{
	OpResult ret;
	FileList fileList;
	ret.opCount++;
	ret.statusCount[ readFileListFile( listFile, &fileList ) ]++;
	return ret + treeMoveWithList( fileList, src, dst, dstRoot, overwrite, rep );
}

bool Misc::copyFile( const char* src, const char* dst, bool overwrite )
{
	if ( !src || !dst || src == dst || strcmp( src, dst ) == 0 ) {
		return false;
	}
	if ( !isFile( src ) ) {
		return false;
	}
	if ( !overwrite && isFile( dst ) ) {
		// can't overwrite the old file
		return false;
	}
	bool readError = false;
	bool writeError = false;
	int inRet = 0;
	int outRet = 0;
	size_t readTotal = 0;
	size_t wroteTotal = 0;
	FILE* fpIn = fopen( src, "rb" );
	FILE* fpOut = fopen( dst, "wb" );
	if ( fpIn && fpOut ) {
		while ( !feof( fpIn ) ) {
			const int BUFFER_SIZE = 4096;
			char buffer[ BUFFER_SIZE ];
			size_t count = fread( buffer, 1, BUFFER_SIZE, fpIn );
			if ( ferror( fpIn ) ) {
				readError = true;
				break;
			}
			readTotal += count;

			count = fwrite( buffer, 1, count, fpOut );
			if ( ferror( fpOut ) ) {
				writeError = true;
				break;
			}
			wroteTotal += count;
		}
	}
	if ( fpIn ) {
		inRet = fclose( fpIn );
	}
	if ( fpOut ) {
		outRet = fclose( fpOut );
	}
	if ( !readError && !writeError && 
		inRet == 0 && outRet == 0 &&
		wroteTotal == readTotal ) {
		return true;
	} else {
		Misc::deleteFile( dst );
		return false;
	}
}

struct rmdir_functor {
	void operator()( const std::string& s ) {
		int r = rmdir( s.c_str() );
		if ( !r ) {
			// ERROR
		}
	}
};

bool Misc::clearDirectory( const char* name )
{
	if ( !isDirectory( name ) ) {
		return false;
	}
	struct _treeWalker : TreeWalker {
		std::vector< std::string > dirs;
		virtual bool operator()( bool isdir, const char* name ) {
			if ( isdir ) {
				dirs.push_back( name );
			} else {
				deleteFile( name );
			}
			return true;
		}
		~_treeWalker() {
			std::for_each( dirs.rbegin(), dirs.rend(), rmdir_functor() );
		}
	} _tw;
	treeWalk( name, _tw );
	return true;
}

bool Misc::deleteDirectory( const char* name )
{
	return clearDirectory( name ) && ( rmdir( name ) == 0 );
}

void Misc::treeWalk( const char* path, TreeWalker& walker )
{
	std::vector< std::string > dirStack;
	dirStack.push_back( path );
	struct _DIRGuard {
		DIR* _dir;
		_DIRGuard( DIR* dir ) : _dir( dir ) {}
		~_DIRGuard() {
			if ( _dir ) {
				closedir( _dir );
			}
		}
	};
	while ( !dirStack.empty() ) {
		std::string lastPath = dirStack.back();
		dirStack.pop_back();
		DIR* dir = opendir( lastPath.c_str() );
		_DIRGuard dirGuard( dir );
		if ( dir != NULL ) {
			struct dirent* ent = NULL;
			while ( ( ent = readdir( dir ) ) != NULL ) {
				switch ( ent->d_type ) {
				case DT_DIR:
					if ( strcmp( ent->d_name, "." ) && strcmp( ent->d_name, ".." ) ) {
						std::string p = lastPath;
						if ( p[p.size() - 1] == '/' ) {
							p += ent->d_name;
						} else {
							p = p + "/" + ent->d_name;
						}
						dirStack.push_back( p );
						if ( !walker( true, p.c_str() ) ) {
							goto EXIT;
						}
					}
					break;
				case DT_REG: {
						std::string f = lastPath;
						if ( f[ f.size() - 1 ] == '/' ) {
							f += ent->d_name;
						} else {
							f = f + "/" + ent->d_name;
						}
						if ( !walker( false, f.c_str() ) ) {
							goto EXIT;
						}
					}
					break;
				default:
					break;
				}
			}
		}
	}
EXIT:;
}

bool Misc::renameFile( const char* oldName, const char* newName )
{
	int r = rename( oldName, newName );
	return r == 0;
}

void Misc::debugOutputString( const char* text )
{
#ifdef _WIN32
	printf( text );
	OutputDebugStringA( text );
#else
	printf( text );
#endif
}

void Misc::error( const char* format, ... )
{
	char text[ 1024 ];
	va_list valist;
	va_start( valist, format );
	text[ 1023 ] = '\0';
	vsnprintf( text, 1020, format, valist );
	va_end( valist );
	size_t len = strlen( text );
	if ( text[ len - 1 ] != '\n' ) {
		text[ len ] = '\n';
		text[ len + 1 ] = '\0';
	}
	debugOutputString( text );
}

void Misc::trace( const char* format, ... )
{
	char text[ 1024 ];
	va_list valist;
	va_start( valist, format );
	text[ 1023 ] = '\0';
	vsnprintf( text, 1020, format, valist );
	va_end( valist );
	size_t len = strlen( text );
	if ( text[ len - 1 ] != '\n' ) {
		text[ len ] = '\n';
		text[ len + 1 ] = '\0';
	}
	debugOutputString( text );
}

bool Misc::isDebuggerPresent()
{
#if defined( WIN32 ) || defined( WIN64 )
	#if (_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400)
		return IsDebuggerPresent() ? true : false;
	#else
		DWORD v = 0;
		__asm
		{
			mov	  eax, dword ptr fs: [0x18]
			mov	  eax, dword ptr [eax+0x30]
			movzx eax, byte  ptr [eax+0x02]
			mov   v,   eax
		}
		return v ? true : false;
	#endif
#else
	return false;
#endif
}	

void Misc::breakPoint()
{
#if defined( WIN32 ) || defined( WIN64 )
 	DebugBreak();
#else
	printf( "debug breakPoint hited!\n" );
	// force crash!
	int* p = NULL;
	*p = 0;
#endif
}

void Misc::breakToDebugger()
{
	if ( isDebuggerPresent() ) {
		breakPoint();
	}
}

void Misc::breakIfNoDebugger()
{
	if ( !isDebuggerPresent() ) {
		breakPoint();
	}
}

void Misc::traceEx( const char* fileName, unsigned int line, bool debugbreak, bool msgBox, const char* format, ... )
{
	char buf1[1024];
	char buf2[1024];
	char buf3[1024];
	buf1[0] = 0;
	buf2[0] = 0;
	buf3[0] = 0;
	if ( fileName != NULL ) {
		sprintf( buf1, "%s(%d):", fileName, line );
		va_list valist;
		va_start( valist, format );
		vsnprintf( buf2, 1024, format, valist );
		va_end( valist );
		sprintf( buf3, "%s %s\n", buf1, buf2 );
	} else {
		va_list valist;
		va_start( valist, format );
		vsnprintf( buf3, 1024, format, valist );
		va_end( valist );
	}
#if defined( WIN32 ) || defined( WIN64 )
	printf( buf3 );
	OutputDebugStringA( buf3 );
	if ( msgBox && ( !isDebuggerPresent() || debugbreak ) ) {
		int ret = IDYES;
		static int count = 0;
		ret = MessageBoxA( GetForegroundWindow(), buf3, "Debug trace", MB_ICONERROR | MB_ABORTRETRYIGNORE | MB_DEFBUTTON2 );
		switch ( ret )
		{
		case IDABORT:
			if ( isDebuggerPresent() ) {
				breakPoint();
			} else {
				exit( 1 );
			}
			break;
		case IDRETRY:
			{
				++count;
				if ( isDebuggerPresent() ) {
					breakPoint();
				}
			}
			break;
		case IDIGNORE:
			{
				++count;
			}
			break;
		}
		if ( !isDebuggerPresent() && count > 10 ) {
			MessageBoxA( GetForegroundWindow(),
				"You have got so many errors, program must be shutdown. sorry:(", "Error", MB_OK );
			exit(1);
		}
	}
	
#else
	printf( buf3 );
#endif
	if ( debugbreak ) {
		breakPoint();
	}
}

bool Misc::createDirectory( const char* name )
{
#ifdef _WIN32
	int r = _mkdir( name );
#else // _WIN32
	int r = mkdir( name, 0770 );
#endif // _WIN32
	if ( r == -1 && errno != EEXIST ) {
		return false;
	} else {
		return r == 0;
	}
}

bool Misc::createDirectories( const char* dirPath )
{	
	char buf[ 512 ] = { 0 };
	strcpy( buf, dirPath );
	size_t len = strlen( buf );
	if ( len == 0 )
		return false;
	if ( buf[ len - 1 ] == '/' || buf[ len - 1 ] == '\\' )
		buf[ len - 1 ] = '\0';
	if ( isDirectory( buf ) ) {
		return true;
	}
	std::string curDir;
	const char* delim = "/\\";
	const char* n = strtok( buf, delim );
	while ( n ) {
		if ( !curDir.empty() ) {
			curDir.push_back( '/' );
		}
#ifdef _WIN32
		bool isAbs = strchr( n, ':' ) != NULL;
#else
		bool isAbs = n[0] == '/';
#endif
		curDir.append( n );
		if ( !isAbs ) {
			if ( !isDirectory( curDir.c_str() ) ) {
				if ( !createDirectory( curDir.c_str() ) ) {
					return false;
				}
			}
		}
		n = strtok( NULL, delim );
	}
	return true;
}

bool Misc::isDirectory( const char* name )
{
	struct stat fs;
	char tmp[ 512 ] = { 0 };
	if ( !name || name[0] == '\0' ) {
		return false;
	}
	// eat the last seperator if only
	// for the stupid API
	strcpy( tmp, name );
	size_t len = strlen( tmp );
	if ( tmp[ len - 1 ] == '/' || tmp[ len - 1 ] == '\\' ) {
		tmp[ len - 1 ] = '\0';
	}
	if ( stat( tmp, &fs ) == 0 ) {
		if ( fs.st_mode & S_IFDIR )
			return true;
	}
	return false;
}

bool Misc::isFile( const char* name )
{	
	struct stat fs;
	if ( stat( name, &fs ) == 0 ) {
		if ( fs.st_mode & S_IFREG )
			return true;
	}
	return false;
}

bool Misc::deleteFile( const char* file )
{
	int r = remove( file );
	return r == 0;
}

int Misc::getFileLength( const char* file )
{
	struct stat fs;
	if ( stat( file, &fs ) == 0 ) {
		return ( int )fs.st_size;
	}
	return 0;
}

bool Misc::readFileAll( const char* file, ByteBuffer& buf )
{
	char* result = NULL;
	FILE* fp = NULL;
	fp = fopen( file, "rb" );
	if ( fp != NULL ) {
		long curpos = ftell( fp );
		fseek( fp, 0L, SEEK_END );
		long len = ftell( fp );
		fseek( fp, curpos, SEEK_SET );
		buf.resize( len );
		if ( len > 0 ) {
			fread( &*buf.begin(), 1, len, fp );
		}
		fclose( fp );
		return true;
	} else {
		buf.clear();
		return false;
	}
}

bool Misc::writeFileAll( const char* file, const char* data, size_t len )
{
	std::string dir = file;
	for ( int i = (int)dir.size() - 1; i >= 0; i-- ) {
		if ( dir[i] == '/' || dir[i] == '\\') {
			dir = dir.substr( 0, i );
			break;
		}
	}
#ifdef _WIN32
	_mkdir( dir.c_str() );
#else // _WIN32
	mkdir( dir.c_str(), 0770 );
#endif // _WIN32

	FILE* fp = fopen( file, "wb" );
	if ( fp != NULL ) {
		size_t _len = fwrite( data, sizeof( char ), len, fp );
		return _len == len && 0 == fclose( fp );
	} else {
		return false;
	}
}

void Misc::sleep( int ms )
{
#ifdef _WIN32
	Sleep( ms );
#else
	usleep( ms * 1000 );
#endif
}

long long Misc::getGlobalTimeMS()
{
#ifdef _WIN32
	SYSTEMTIME wtm;
	GetLocalTime( &wtm );
	struct tm tTm;
	tTm.tm_year = wtm.wYear - 1900;
	tTm.tm_mon = wtm.wMonth - 1;
	tTm.tm_mday = wtm.wDay;
	tTm.tm_hour = wtm.wHour;
	tTm.tm_min = wtm.wMinute;
	tTm.tm_sec = wtm.wSecond;
	tTm.tm_isdst = -1;
	return mktime( &tTm ) * 1000 + wtm.wMilliseconds;
#else
	struct timeval tv;
	struct timezone tz;
	gettimeofday( &tv , &tz );
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif
}

bool Misc::readCSVFromMemory( const Misc::ByteBuffer& buf, StringTable& out )
{
	struct ParseCSVLine {
		std::string convertEscape( const std::string& _s ) {
			std::string s;
			const char* p = _s.c_str();
			while ( *p ) {
				if ( p[0] == '\\' ) {
					switch ( p[1] ) {
					case 'a':
						s.push_back( '\a' );
						break;
					case 'b':
						s.push_back( '\b' );
						break;
					case 'f':
						s.push_back( '\f' );
						break;
					case 'n':
						s.push_back( '\n' );
						break;
					case 'r':
						s.push_back( '\r' );
						break;
					case 't':
						s.push_back( '\t' );
						break;
					case 'v':
						s.push_back( '\v' );
						break;
					case '\\':
						s.push_back( '\\' );
						break;
					}
					p += 2;
				} else {
					s.push_back( *p );
					++p;
				}
			}
			return s;
		}
		bool operator()( std::vector< std::string >& record, const std::string& _line, char delimiter ) {
			int linepos = 0;
			int inquotes = false;
			char c;
			int linemax = _line.length();
			const char* line = _line.c_str(); 
			std::string curstring;
			record.clear();
			while ( line[ linepos ]!=0 && linepos < linemax ) {
				c = line[ linepos ];
				if ( !inquotes && curstring.length() == 0 && c == '"' ) {
					//beginquotechar
					inquotes=true;
				} else if ( inquotes && c == '"' ) {
					//quotechar
					if ( ( linepos + 1 < linemax ) && ( line[ linepos + 1 ] == '"') ) {
						//encountered 2 double quotes in a row (resolves to 1 double quote)
						curstring.push_back( c );
						linepos++;
					} else {
						//endquotechar
						inquotes = false; 
					}
				} else if ( !inquotes && c == delimiter ) {
					//end of field
					record.push_back( convertEscape( curstring ) );
					curstring.clear();
				}
				else if ( !inquotes && ( c == '\r' || c == '\n' ) ) {
					record.push_back( convertEscape( curstring ) );
					return true;
				} else {
					curstring.push_back( c );
				}
				++linepos;
			}
			record.push_back( convertEscape( curstring ) );
			return true;
		}
	} _parser;
	out.clear();
	const unsigned char* p = ( const unsigned char* )&*buf.begin();
	std::string line;
	if ( buf.size() > 3 ) {
		if ( p[0] == 0xEF && p[1] == 0xBB && p[2] == 0xBF ) {
			// utf-8
			p += 3;
		}
	} else if ( buf.size() > 2 ) {
		if ( p[0] == 0xFF && p[1] == 0xFE ) {
			// unicode
			p += 2;
		} else if ( p[0] == 0xFE && p[1] == 0xFF ) {
			// unicode big endian
			p += 2;
		}
	}
	while ( *p ) {
		if ( *p == '\n' || p[1] == '\0' ) {
			out.resize( out.size() + 1 );
			_parser( out.back(), line, ',' );
			line.clear();
		} else {
			line.push_back( *p );
		}
		++p;
	}
	return true;
}

bool Misc::readCSV( const char* file, StringTable& out )
{	
	ByteBuffer buf;
	if ( !readFileAll( file, buf ) ) {
		return false;
	}
	buf.push_back( '\0' );
	return readCSVFromMemory( buf, out );
}

bool Misc::matchWildcard( const char* str, const char* wildcard )
{
	const char* s = str;
	const char* w = wildcard;
	// skip the obviously the same starting substring
	while ( *w && *w != '*' && *w != '?' ) {
		if ( *s != *w ) {
			 // must be exact match unless there's a wildcard character in the wildcard string
			return false;
		} else {
			++s;
			++w;
		}
	}

	if ( !*s ) {
		// this will only match if there are no non-wild characters in the wildcard
		for ( ; *w; ++w ) {
			if ( *w != '*' && *w != '?' ) {
				return false;
			}
		}
		return true;
	}

	switch ( *w ) {
	case '\0':
		return false; // the only way to match them after the leading non-wildcard characters is !*pString, which was already checked
		// we have a wildcard with wild character at the start.
	case '*': {
			// merge consecutive ? and *, since they are equivalent to a single *
			while ( *w == '*' || *w == '?' ) {
				++w;
			}
			if ( !*w ) {
				// the rest of the string doesn't matter: the wildcard ends with *
				return true;
			}
			for ( ; *s; ++s ) {
				if ( matchWildcard( s, w ) ) {
					return true;
				}
			}
			return false;
		}
	case '?':
		return matchWildcard( s + 1, w + 1 ) || matchWildcard( s, w + 1 );
	default:
		assert( 0 );
		return false;
	}
}

//EOF
