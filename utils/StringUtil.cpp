#include <algorithm>
#include "StringUtil.h"

#pragma warning(push)
#pragma warning(disable : 4127)
#pragma warning(disable : 4996)

const std::string StringUtil::BLANK;

int StringUtil::stricmp( const char* str1, const char* str2 )
{
#ifdef _MSC_VER
	return str1 != str2 ? ::_stricmp( str1, str2 ) : 0;
#else
	return str1 != str2 ? ::strcasecmp( str1, str2 ) : 0;
#endif
}

void StringUtil::trim(std::string &str, bool left, bool right) {
	static const std::string delims = " \t\r\n";
	if(right) {
		str.erase(str.find_last_not_of(delims) + 1); // Trim right
	}
	if(left) {
		str.erase(0, str.find_first_not_of(delims)); // Trim left
	}
}

int StringUtil::countOf(const std::string &str, char what) {
	int count = 0;
	for(std::string::const_iterator it = str.begin(); it != str.end(); ++it) {
		if(*it == what) {
			++count;
		}
	}

	return count;
}

void StringUtil::toLowerCase(std::string &str) {
	std::transform(
		str.begin(),
		str.end(),
		str.begin(),
		tolower);
}

void StringUtil::toUpperCase(std::string &str) {
	std::transform(
		str.begin(),
		str.end(),
		str.begin(),
		toupper);
}

bool StringUtil::startsWith(const std::string &str, const std::string &pattern, bool lowerCase) {
	size_t thisLen = str.length();
	size_t patternLen = pattern.length();
	if(thisLen < patternLen || patternLen == 0) {
		return false;
	}

	std::string startOfThis = str.substr(0, patternLen);
	if(lowerCase) {
		StringUtil::toLowerCase(startOfThis);
	}

	return (startOfThis == pattern);
}

bool StringUtil::endsWith(const std::string &str, const std::string &pattern, bool lowerCase) {
	size_t thisLen = str.length();
	size_t patternLen = pattern.length();
	if(thisLen < patternLen || patternLen == 0) {
		return false;
	}

	std::string endOfThis = str.substr(thisLen - patternLen, patternLen);
	if(lowerCase) {
		StringUtil::toLowerCase(endOfThis);
	}

	return (endOfThis == pattern);
}

std::string StringUtil::standardisePath(const std::string &init) {
	std::string path = init;

	std::replace(path.begin(), path.end(), '\\', '/');
	if(path.size() > 0 && path[path.length() - 1] != '/') {
		path += '/';
	}

	return path;
}

void StringUtil::splitFilename(const std::string &qualifiedName, std::string &outBasename, std::string &outPath) {
	std::string path = qualifiedName;
	// Replace \ with / first
	std::replace(path.begin(), path.end(), '\\', '/');
	// Split based on final /
	size_t i = path.find_last_of('/');

	if(i == std::string::npos) {
		outPath.clear();
		outBasename = qualifiedName;
	} else {
		outBasename = path.substr(i + 1, path.size() - i - 1);
		outPath = path.substr(0, i + 1);
	}
}

void StringUtil::splitBaseFilename(const std::string &fullName, std::string &outBasename, std::string &outExtention) {
	size_t i = fullName.find_last_of(".");
	if(i == std::string::npos) {
		outExtention.clear();
		outBasename = fullName;
	} else {
		outExtention = fullName.substr(i + 1);
		outBasename = fullName.substr(0, i);
	}
}

void StringUtil::splitFullFilename(const std::string &qualifiedName, std::string &outBasename, std::string &outExtention, std::string &outPath) {
	std::string fullName;
	splitFilename(qualifiedName, fullName, outPath);
	splitBaseFilename(fullName, outBasename, outExtention);
}

bool StringUtil::match(const std::string &str, const std::string &pattern, bool caseSensitive) {
	std::string tmpStr = str;
	std::string tmpPattern = pattern;
	if(!caseSensitive) {
		StringUtil::toLowerCase(tmpStr);
		StringUtil::toLowerCase(tmpPattern);
	}

	std::string::const_iterator strIt = tmpStr.begin();
	std::string::const_iterator patIt = tmpPattern.begin();
	std::string::const_iterator lastWildCardIt = tmpPattern.end();
	while(strIt != tmpStr.end() && patIt != tmpPattern.end()) {
		if(*patIt == '*') {
			lastWildCardIt = patIt;
			// Skip over looking for next character
			++patIt;
			if(patIt == tmpPattern.end()) {
				// Skip right to the end since * matches the entire rest of the string
				strIt = tmpStr.end();
			} else {
				// Scan until we find next pattern character
				while(strIt != tmpStr.end() && *strIt != *patIt) {
					++strIt;
				}
			}
		} else {
			if(*patIt != *strIt) {
				if(lastWildCardIt != tmpPattern.end()) {
					// The last wildcard can match this incorrect sequence
					// rewind pattern to wildcard and keep searching
					patIt = lastWildCardIt;
					lastWildCardIt = tmpPattern.end();
				} else {
					// No wildwards left
					return false;
				}
			} else {
				++patIt;
				++strIt;
			}
		}
	}
	// If we reached the end of both the pattern and the string, we succeeded
	if(patIt == tmpPattern.end() && strIt == tmpStr.end()) {
		return true;
	} else {
		return false;
	}
}

const std::string StringUtil::replaceAll(const std::string &source, const std::string &replaceWhat, const std::string &replaceWithWhat) {
	std::string result = source;
	while(1) {
		std::string::size_type pos = result.find(replaceWhat);
		if(pos == std::string::npos) {
			break;
		}
		result.replace(pos,replaceWhat.size(),replaceWithWhat);
	}

	return result;
}

std::wstring StringUtil::ansiToUnicode(const std::string &s) {
	const char* _Source = s.c_str();
	size_t _Dsize = s.size() + 1;
	wchar_t* _Dest = new wchar_t[_Dsize];
	wmemset(_Dest, 0, _Dsize);
	mbstowcs(_Dest, _Source, _Dsize);
	std::wstring result = _Dest;
	delete[] (_Dest);

	return result;
}

std::string StringUtil::unicodeToAnsi(const std::wstring &ws) {
	const wchar_t* _Source = ws.c_str();
	size_t _Dsize = 2 * ws.size() + 1;
	char* _Dest = new char[_Dsize];
	memset(_Dest, 0, _Dsize);
	wcstombs(_Dest, _Source, _Dsize);
	std::string result = _Dest;
	delete[] (_Dest);

	return result;
}

#pragma warning(pop)
