#ifndef __MISC_H__
#define __MISC_H__

#include <vector>

struct Misc
{
	struct TreeWalker {
		virtual bool operator()( bool isdir, const char* name ) {
			Misc::trace( "%s\n", name );
			return true;
		};
	};
	struct TreeMoveReporter {
		virtual bool copyFile( const char* src, const char* dst, bool overwrite );
		virtual bool filter( const char* fileName ) { return false; }
		virtual void operator()( const char* fileName, bool succeeded ) {
			Misc::trace( "copy file: %s %s\n", fileName, succeeded ? "succeeded" : "failed" );
		};
	};
	struct OpResult {
		union {
			struct {
				int failedCount;
				int succeessCount;
			};
			int statusCount[2];
		};
		int opCount;
		OpResult() : succeessCount( 0 ), failedCount( 0 ), opCount( 0 ) {}
		operator bool() const { return opCount > 0 && succeessCount == opCount && failedCount == 0;}
		OpResult operator + ( const OpResult& o ) const
		{
			OpResult r;
			r.opCount = opCount + o.opCount;
			r.failedCount = failedCount + o.failedCount;
			r.succeessCount = succeessCount + o.succeessCount;
			return r;
		}
	};

	struct FileList
	{
		int fileNum;
		long long totalSize;
		std::vector< std::string > filePathList;
		FileList() : fileNum( 0 ), totalSize( 0 ) {} 
	};

	typedef std::vector< char > ByteBuffer;

	typedef std::vector< std::vector< std::string > > StringTable;

	static void			treeWalk( const char* dir, TreeWalker& walker );
	static void			debugOutputString( const char* text );
	static void			trace( const char* format, ... );
	static void			error( const char* format, ... );
	static bool			isDebuggerPresent();
	static void			breakPoint();
	static void			breakToDebugger();
	static void			breakIfNoDebugger();
	static void			traceEx( const char* fileName, unsigned int line, bool debugbreak, bool msgBox, const char* format, ... );
	static bool			copyFile( const char* src, const char* dst, bool overwrite = true );
	static OpResult		treeMove( const char* src, const char* dst, const char* srcRoot = NULL, bool overwrite = true, TreeMoveReporter* rep = NULL );
	static OpResult		treeMoveWithList( const char* listFile, const char* src, const char* dst, const char* dstRoot = NULL, bool overwrite = true, TreeMoveReporter* rep = NULL );
	static OpResult		treeMoveWithList( const FileList& fileList, const char* src, const char* dst, const char* dstRoot = NULL, bool overwrite = true, TreeMoveReporter* rep = NULL );
	static bool			readFileListFile( const char* listFile, FileList* fileList );
	static bool			readFileListFileFromMemory( const Misc::ByteBuffer& buf, FileList* fileList );
	static bool			isTreeMoving();
	static bool			terminateTreeMoving();
	static bool			deleteDirectory( const char* name );
	static bool			clearDirectory( const char* name );
	static bool			createDirectory( const char* name );
	static bool			createDirectories( const char* dirPath );
	static bool			isDirectory( const char* name );
	static bool			isFile( const char* name );
	static bool			deleteFile( const char* file );
	static bool			renameFile( const char* oldName, const char* newName );
	static bool			isFileInZip( const char* zipFileName, const char* file );
	static int			getFileLength( const char* file );
	static bool			readFileAllFromZip( const char* zipFileName, const char* file, ByteBuffer& buf );
	static bool			readFileAll( const char* file, ByteBuffer& buf );
	static bool			writeFileAll( const char* file, const char* data, size_t len );
	static void			sleep( int ms );
	static long long	getGlobalTimeMS();
	static void			md5( const char* file, unsigned char out[16] ){}
	static void			md5( const char* file, char out[33] ){}
	static bool			readCSV( const char* file, StringTable& out );
	static bool			readCSVFromMemory( const Misc::ByteBuffer& buf, StringTable& out );
	static bool			matchWildcard( const char* str, const char* wildcard );
};

#define __TracePrint( format, ... )							do { Misc::traceEx(   NULL,	  (unsigned int)__LINE__, false, false, format, ##__VA_ARGS__ ); } while(0)
#define __Trace( format, ... )								do { Misc::traceEx( __FILE__, (unsigned int)__LINE__, false, false, format, ##__VA_ARGS__ ); } while(0)
#define __TraceMsg( format, ... )							do { Misc::traceEx( __FILE__, (unsigned int)__LINE__, false, true,  format, ##__VA_ARGS__ ); } while(0)
#define __TraceBreak( format, ... )							do { Misc::traceEx( __FILE__, (unsigned int)__LINE__, true,  false, format, ##__VA_ARGS__ ); } while(0)
#define __TraceError( format, ... )							do { Misc::traceEx( __FILE__, (unsigned int)__LINE__, true,  true,  format, ##__VA_ARGS__ ); } while(0)

#endif
//EOF
