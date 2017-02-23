#include "xplext_file.h"
#include <stdio.h>

#ifdef _MSC_VER
#  pragma warning(disable : 4996)
#endif /* _MSC_VER */

#define _CACHE_SIZE 100

XPLINTERNAL int _xplext_flen(FILE* fp) {
	int result = 0;
	int curpos = 0;

	xpl_assert(fp);
	curpos = ftell(fp);
	fseek(fp, 0L, SEEK_END);
	result = ftell(fp);
	fseek(fp, curpos, SEEK_SET);

	return result;
}

XPLINTERNAL char* _xplext_freadall(const char* file) {
	char* result = NULL;
	FILE* fp = NULL;
	xpl_assert(file);
	fp = fopen(file, "rb");
	if(fp != NULL) {
		int l = _xplext_flen(fp);
		result = (char*)malloc(sizeof(char) * l + 1);
		xpl_assert(result);
		fread(result, 1, l, fp);
		fclose(fp);
		result[l] = 0;
	}

	return result;
}

XPLINTERNAL const char* _xplext_next_line(const char* s) {
	while(*s != '\n') ++s;
	return s;
}

XPLINTERNAL const char* _xplext_parse_cpp_comment(const char* p) {
	const char* result = p;
	if(p[1] == '/') {
		result =_xplext_next_line(p);
	} else if(p[1] == '*') {
		p += 2;
		while(*p && (*p != '*' || p[1] != '/'))
			++p;
		result = (*p != 0) ? (p + 2) : p;
	}
	return result;
}

XPLINTERNAL const char* _xplext_parse_xpl_comment(const char* p) {
	while(*p && *p != '\'') ++p;
	return *p ? p + 1 : p;
}

XPLINTERNAL int _xplext_func_name_srt_cmp(const void* _l, const void* _r) {
  const xplext_source_seg_info_t* l = (const xplext_source_seg_info_t*)_l;
  const xplext_source_seg_info_t* r = (const xplext_source_seg_info_t*)_r;
  xpl_assert(l && r);

  return _xpl_strcmp(l->name, r->name);
}

XPLINTERNAL int _xplext_func_info_sch_cmp(const void* _k, const void* _i) {
  int ret;
  const char* k = (const char*)_k;
  const xplext_source_seg_info_t* i = (xplext_source_seg_info_t*)_i;
  xpl_assert(k && i);
  ret = _xpl_strcmp(i->name, k);
  if(ret < 0) ret = 1;
  else if(ret > 0) ret = -1;
  return ret;
}

XPLINTERNAL void _xplext_parse(xplext_source_t* s) {
	const char* p;
	const char* key = NULL;
	const char* source = NULL;
	const char* keyCache[_CACHE_SIZE];
	const char* sourceCache[_CACHE_SIZE];
	int i;
	xpl_assert(s && s->text);
	p = s->text;
	s->count = 0;
	while(*p) {
		_xpl_trim(&p);
		switch(*p) {
		case '/':
			if(!*(p = _xplext_parse_cpp_comment(p))) goto END;
			key = NULL;
			break;	
		case '\'':
			if(!*(p = _xplext_parse_xpl_comment(p+1))) goto END;
			key = NULL;
			break;
		case ':':
			if(key) {
				_xpl_trim(&key);
				keyCache[s->count] = key;
			}
			break;
		case '{':
			++p;
			source = p;
			_xpl_trim(&source);
			while(*p && *p != '}') ++p;
			if(*p == 0) goto END;
			*(char*)p = '\0';
			sourceCache[s->count] = source;
			key = NULL;
			break;
		case ',':
			++p;
			if(key) {
				key = NULL;
				source = NULL;
				xpl_assert(s->count < _CACHE_SIZE);
				++s->count;
			}
			continue;
		}
		if(!key)
			key = p;
		++p;
	}
END:
	if(key && source) {
		xpl_assert(s->count < _CACHE_SIZE);
		++s->count;
	}
	if(s->count) {
		s->items = (xplext_source_seg_info_t*)malloc(s->count * sizeof(xplext_source_seg_info_t));
		for( i=0; i<s->count; ++i ) {
			s->items[i].name = keyCache[i];
			s->items[i].source = sourceCache[i];
		}
		qsort(s->items, s->count, sizeof(xplext_source_seg_info_t), _xplext_func_name_srt_cmp);
	}
}

xplext_source_t* xplext_load_source(const char* file) {
	xplext_source_t* result = NULL;
	char* s = _xplext_freadall(file);
	if(s) {
		result = (xplext_source_t*)malloc(sizeof(xplext_source_t));
		memset(result, 0, sizeof(*result));
		result->text = s;
		_xplext_parse(result);
	}
	return result;
}

int xplext_unload_source(xplext_source_t* s) {
	if(s) {
		free(s->items);
		free(s->text);
		free(s);
	}
	return 1;
}

const char* xplext_find_source(xplext_source_t* s, const char* key) {
	xplext_source_seg_info_t* item = (xplext_source_seg_info_t*)bsearch(key, s->items, s->count, sizeof(xplext_source_seg_info_t), _xplext_func_info_sch_cmp);
	return item ? item->source : NULL;
}

