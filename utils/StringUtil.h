#ifndef __STRING_UTIL_H__
#define __STRING_UTIL_H__

#include <string>
#include <vector>

// From OGRE
class StringUtil {

public:
	template<typename T>
	static T& split( T& out, const std::string& str, const std::string& delims = "\t\n ", unsigned int maxSplits = 0 ) {
		T& ret = out;
		ret.clear();
		unsigned int numSplits = 0;
		// Use STL methods
		size_t start, pos;
		start = 0;
		do {
			pos = str.find_first_of( delims, start );
			if ( pos == start ) {
				// Do nothing
				start = pos + 1;
			} else if ( pos == std::string::npos || ( maxSplits && numSplits == maxSplits ) ) {
				// Copy the rest of the string
				ret.push_back( str.substr( start ) );
				break;
			} else {
				// Copy up to delimiter
				ret.push_back( str.substr( start, pos - start ) );
				start = pos + 1;
			}
			// Parse up to next real data
			start = str.find_first_not_of( delims, start );
			++numSplits;
		} while ( pos != std::string::npos );

		return ret;
	}
	template<typename T>
	static T tokenise( const std::string& str, const std::string& singleDelims = "\t\n ", const std::string& doubleDelims = "\"", unsigned int maxSplits = 0 ) {
		T ret;
		unsigned int numSplits = 0;
		std::string delims = singleDelims + doubleDelims;
		// Use STL methods
		size_t start, pos;
		char curDoubleDelim = 0;
		start = 0;
		do {
			if ( curDoubleDelim != 0 ) {
				pos = str.find( curDoubleDelim, start );
			} else {
				pos = str.find_first_of( delims, start );
			}
			if ( pos == start ) {
				char curDelim = str.at( pos );
				if ( doubleDelims.find_first_of( curDelim ) != std::string::npos ) {
					curDoubleDelim = curDelim;
				}
				// Do nothing
				start = pos + 1;
			} else if ( pos == std::string::npos || ( maxSplits && numSplits == maxSplits ) ) {
				if ( curDoubleDelim != 0 ) {
					// Missing closer. Warn or throw exception?
				}
				// Copy the rest of the string
				ret.push_back( str.substr( start ) );
				break;
			} else {
				if ( curDoubleDelim != 0 ) {
					curDoubleDelim = 0;
				}
				// Copy up to delimiter
				ret.push_back( str.substr( start, pos - start ) );
				start = pos + 1;
			}
			if ( curDoubleDelim == 0 ) {
				// Parse up to next real data
				start = str.find_first_not_of( singleDelims, start );
			}

			++numSplits;
		} while ( pos != std::string::npos );
		return ret;
	}
    static void trim( std::string &str, bool left = true, bool right = true );
	static int countOf( const std::string& str, char what );
    static void toLowerCase( std::string& str );
    static void toUpperCase( std::string& str );
    static bool startsWith( const std::string& str, const std::string& pattern, bool lowerCase = true );
    static bool endsWith( const std::string& str, const std::string& pattern, bool lowerCase = true );
    static std::string standardisePath( const std::string& init );
    static void splitFilename( const std::string& qualifiedName, std::string& outBasename, std::string& outPath );
	static void splitFullFilename( const std::string& qualifiedName, std::string& outBasename, std::string& outExtention, std::string& outPath );
	static void splitBaseFilename( const std::string& fullName, std::string& outBasename, std::string& outExtention);
    static bool match(const std::string& str, const std::string& pattern, bool caseSensitive = true );
	static const std::string replaceAll( const std::string& source, const std::string& replaceWhat, const std::string& replaceWithWhat );
	static std::wstring ansiToUnicode( const std::string& source );
	static std::string unicodeToAnsi( const std::wstring& source );
	static int stricmp( const char* src1, const char* src2 );
    static const std::string BLANK;

};


#endif // __STRING_UTIL_H__
