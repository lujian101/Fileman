#include "xpl/xpl.h"
#include "dt_rbtree.h"
#include "dt_encoding.h"
#include "dt_core.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef _MSC_VER
#	pragma warning(disable : 4116)
#	pragma warning(disable : 4996)
#endif /* _MSC_VER */

/*
** {========================================================
** Macros
*/

#ifndef DTVER
#	define DTVER_MAJOR	0
#	define DTVER_MINOR	1
#	define DTVER_PATCH	38
#	define DTVER ((DTVER_MAJOR * 0x01000000) + (DTVER_MINOR * 0x00010000) + (DTVER_PATCH))
#endif /* DTVER */
#ifndef DTBVER
#	define DTBVER_MAJOR	1
#	define DTBVER_MINOR	0
#	define DTBVER_PATCH	1
#	define DTBVER ((DTBVER_MAJOR * 0x01000000) + (DTBVER_MINOR * 0x00010000) + (DTBVER_PATCH))
#endif /* DTBVER */

#ifndef _DT_FLOAT_COMP_MODE_STRICT
#	define _DT_FLOAT_COMP_MODE_STRICT 0
#endif /* _DT_FLOAT_COMP_MODE_STRICT */
#ifndef _DT_FLOAT_COMP_MODE_ROUGH
#	define _DT_FLOAT_COMP_MODE_ROUGH 1
#endif /* _DT_FLOAT_COMP_MODE_ROUGH */
#ifndef _DT_FLOAT_COMP_MODE
#	define _DT_FLOAT_COMP_MODE _DT_FLOAT_COMP_MODE_STRICT
#endif /* _DT_FLOAT_COMP_MODE */
#ifndef _DT_SINGLE_COMP
#	if _DT_FLOAT_COMP_MODE == _DT_FLOAT_COMP_MODE_STRICT
#		define _DT_SINGLE_COMP(l, r) ((l) == (r) ? 0 : ((l) > (r) ? 1 : -1))
#	else
#		if _DT_FLOAT_COMP_MODE == _DT_FLOAT_COMP_MODE_ROUGH
#			define _DT_SINGLE_COMP(l, r) ((l) == (r) ? 0 : ((l) > (r) ? 1 : -1))
#		endif
#	endif
#endif /* _DT_SINGLE_COMP */
#ifndef _DT_DOUBLE_COMP
#	if _DT_FLOAT_COMP_MODE == _DT_FLOAT_COMP_MODE_STRICT
#		define _DT_DOUBLE_COMP(l, r) ((l) == (r) ? 0 : ((l) > (r) ? 1 : -1))
#	else
#		if _DT_FLOAT_COMP_MODE == _DT_FLOAT_COMP_MODE_ROUGH
#			define _DT_DOUBLE_COMP(l, r) ((l) == (r) ? 0 : ((l) > (r) ? 1 : -1))
#		endif
#	endif
#endif /* _DT_DOUBLE_COMP */

#ifndef _DT_ARRAY_GROW_STEP
#	define _DT_ARRAY_GROW_STEP 16
#endif /* _DT_ARRAY_GROW_STEP */

#ifndef _DT_STRING_LOOK_AHEAD
#	define _DT_STRING_LOOK_AHEAD 8
#endif /* _DT_STRING_LOOK_AHEAD */

#ifndef _DT_WRITE_CHUNK_SIZE
#	define _DT_WRITE_CHUNK_SIZE(t, s) (*((size_t*)((char*)(t) - DT_POINTER_SIZE)) = s)
#endif /* _DT_WRITE_CHUNK_SIZE */
#ifndef _DT_READ_CHUNK_SIZE
#	define _DT_READ_CHUNK_SIZE(t) (*((size_t*)((char*)(t) - DT_POINTER_SIZE)))
#endif /* _DT_READ_CHUNK_SIZE */

#ifndef _dt_min
#	define _dt_min(l, r) (((l) < (r)) ? (l) : (r))
#endif /* _dt_min */
#ifndef _dt_max
#	define _dt_max(l, r) (((l) > (r)) ? (l) : (r))
#endif /* _dt_max */

#ifndef _DT_CTOR
#	define _DT_CTOR(t, p, a) _dt_ctor_##t((p), (a))
#endif /* _DT_CTOR */
#ifndef _DT_DTOR
#	define _DT_DTOR(t, p) _dt_dtor_##t(p)
#endif /* _DT_DTOR */

#ifndef _DT_NEW
#	define _DT_NEW(t, p, a) do { p = (t*)dt_malloc(sizeof(t)); _DT_CTOR(t, p, a); } while(0)
#endif /* _DT_NEW */
#ifndef _DT_DEL
#	define _DT_DEL(t, p) do { _DT_DTOR(t, p); dt_free((void**)&(p)); } while(0)
#endif /* _DT_DEL */

#ifndef _DT_READ_BIN
#	define _DT_READ_BIN(type, des, buf) do { _dt_read_bin(des, buf, sizeof(type)); } while(0)
#endif /* _DT_READ_BIN */
#ifndef _DT_WRITE_BIN
#	define _DT_WRITE_BIN(type, src, buf) do { _dt_write_bin(src, buf, sizeof(type)); } while(0)
#endif /* _DT_WRITE_BIN */
#ifndef _DT_RESIZE_BIN
#	define _DT_RESIZE_BIN(dt, off, len, s) \
	do { \
		(dt)->ser_buf = (unsigned char*)dt_realloc((void**)&(dt)->ser_buf, *(s) + (len)); \
		*(off) = (dt)->ser_buf + *(s); \
		*(s) += len; \
	} while(0)
#endif /* _DT_RESIZE_BIN */

#ifndef _DT_MAKE_READONLY
#	define _DT_MAKE_READONLY(t, v, r) _DT_MAKE_READONLY_##t(v, r)
#	define _DT_MAKE_READONLY__dt_object_t(v, r) do { ((_dt_object_t*)(v))->readonly = r; } while(0)
#	define _DT_MAKE_READONLY__dt_array_t(v, r) do { ((_dt_array_t*)(v))->readonly = r; } while(0)
#	define _DT_MAKE_VALUE_READONLY(v, r) \
	do { \
		if((v)->type == DT_OBJECT) { _DT_MAKE_READONLY(_dt_object_t, (v)->data._obj, r); } \
		else if((v)->type == DT_ARRAY) { _DT_MAKE_READONLY(_dt_array_t, &(v)->data._array, r); } \
	} while(0)
#endif /* _DT_MAKE_READONLY */

#ifndef _DT_COMPRESS_LONG
#	define _DT_COMPRESS_LONG(l, v) \
	do { \
		if((l) == (long)(char)(l)) { \
			_DT_CTOR(_dt_value_t, (v), DT_BYTE); \
			(v)->data._byte = (char)(l); \
		} else if((l) == (long)(unsigned char)(l)) { \
			_DT_CTOR(_dt_value_t, (v), DT_UBYTE); \
			(v)->data._ubyte = (unsigned char)(l); \
		} else if((l) == (long)(short)(l)) { \
			_DT_CTOR(_dt_value_t, (v), DT_SHORT); \
			(v)->data._short = (short)(l); \
		} else if((l) == (long)(unsigned short)(l)) { \
			_DT_CTOR(_dt_value_t, (v), DT_USHORT); \
			(v)->data._ushort = (unsigned short)(l); \
		} else if((l) == (long)(int)(l)) { \
			_DT_CTOR(_dt_value_t, (v), DT_INT); \
			(v)->data._int = (int)(l); \
		} else if((l) == (long)(unsigned int)(l)) { \
			_DT_CTOR(_dt_value_t, (v), DT_UINT); \
			(v)->data._uint = (unsigned int)(l); \
		} else { \
			_DT_CTOR(_dt_value_t, (v), DT_LONG); \
		} \
	} while(0)
#endif /* _DT_COMPRESS_LONG */

#ifndef _DT_CONVERT
#	define _DT_CONVERT(des, val, t, sub) \
	do { \
		switch(t) { \
		case DT_BYTE: \
			*(char*)(des) = (char)(val)->data.sub; \
			break; \
		case DT_UBYTE: \
			*(unsigned char*)(des) = (unsigned char)(val)->data.sub; \
			break; \
		case DT_SHORT: \
			*(short*)(des) = (short)(val)->data.sub; \
			break; \
		case DT_USHORT: \
			*(unsigned short*)(des) = (unsigned short)(val)->data.sub; \
			break; \
		case DT_INT: \
			*(int*)(des) = (int)(val)->data.sub; \
			break; \
		case DT_UINT: \
			*(unsigned int*)(des) = (unsigned int)(val)->data.sub; \
			break; \
		case DT_LONG: \
			*(long*)(des) = (long)(val)->data.sub; \
			break; \
		case DT_ULONG: \
			*(unsigned long*)(des) = (unsigned long)(val)->data.sub; \
			break; \
		case DT_SINGLE: \
			*(float*)(des) = (float)(val)->data.sub; \
			break; \
		case DT_DOUBLE: \
			*(double*)(des) = (double)(val)->data.sub; \
			break; \
		default: \
			DT_ASSERT(0 && "Invalid type"); \
			result = DT_TYPE_NOT_MATCHED; \
		} \
	} while(0)
#endif /* _DT_CONVERT */

/* ========================================================} */

/*
** {========================================================
** Data type declarations, consts and statics
*/

struct _dt_object_t;
struct _dt_value_t;

typedef struct _dt_string_t {
	char* c_str;
	size_t size;
	size_t length;
} _dt_string_t;

typedef struct _dt_object_t {
	struct _dt_pair_t* members;
	size_t size;
	size_t count : sizeof(size_t) * 8 - 1;
	int readonly : 1;
	dt_rbtree_t mapped;
} _dt_object_t;

typedef struct _dt_array_t {
	struct _dt_value_t* elems;
	size_t count : sizeof(size_t) * 8 - 1;
	int readonly : 1;
} _dt_array_t;

typedef unsigned char _dt_value_raw_t[8];

typedef struct _dt_value_t {
	dt_type_t type;
	union {
		_dt_value_raw_t _raw;
		dt_bool_t _bool;
		char _byte;
		unsigned char _ubyte;
		short _short;
		unsigned short _ushort;
		int _int;
		unsigned int _uint;
		long _long;
		unsigned long _ulong;
		float _single;
		double _double;
		struct _dt_string_t* _str;
		struct _dt_object_t* _obj;
		struct _dt_array_t _array;
	} data;
} _dt_value_t;

typedef struct _dt_indexed_value_t {
	size_t index;
	_dt_value_t obj;
} _dt_indexed_value_t;

typedef struct _dt_pair_t {
	_dt_value_t* key;
	_dt_indexed_value_t* value;
} _dt_pair_t;

typedef struct _dt_bin_header_t {
	size_t size;
	unsigned int version;
	char endian[2];
} _dt_bin_header_t;

typedef struct _dt_datatree_detail_t {
	_dt_value_t root;
	xpl_context_t xpl;
	void* userdata;
	dt_parse_error_handler_t error_handler;
	size_t format_indentation;
	dt_bool_t compact_mode;
	unsigned char* ser_buf;
	_dt_bin_header_t bin_header;
} _dt_datatree_detail_t;

static const char _DT_NEW_LINE = '\n';

static const char* const _DT_STATUS_MSG[] = {
	"No error",
	"Load failed",
	"Redundance unparsed charactor(s) left",
	"Left brace expected",
	"Right brace expected",
	"Left square bracket expected",
	"Right square bracket expected",
	"Colon expected",
	"Too many commas",
	"Value expected",
	"The given key already exists",
	"The given key does not exist",
	"Index out of range",
	"Type not matched",
	"Can not write readonly value"
};

DT_STATIC_ASSERT(sizeof(dt_enum_compatible_t) == sizeof(dt_bool_t));
DT_STATIC_ASSERT(sizeof(dt_enum_compatible_t) == sizeof(dt_type_t));
DT_STATIC_ASSERT(sizeof(dt_enum_compatible_t) == sizeof(dt_status_t));

DT_STATIC_ASSERT(sizeof(_dt_value_raw_t) >= sizeof(double));
DT_STATIC_ASSERT(sizeof(_dt_value_raw_t) >= sizeof(long));
DT_STATIC_ASSERT(sizeof(_dt_value_raw_t) == sizeof(_dt_array_t));

#ifndef __cplusplus
	DT_STATIC_ASSERT(sizeof(_dt_value_raw_t) == DT_ALIGN_OF(_dt_value_t));
#endif /* __cplusplus */

#ifdef DT_ENABLE_ALLOC_STAT
	static volatile size_t _dt_allocated = 0;
#else
	static volatile size_t _dt_allocated = (size_t)(~0);
#endif /* DT_ENABLE_ALLOC_STAT */

/* ========================================================} */

/*
** {========================================================
** Private function declarations
*/

static int _dt_strcmp(const _dt_string_t* l, const _dt_string_t* r);
static void _dt_strcpy(_dt_string_t* d, const char* s);
static char* _dt_strcat(_dt_string_t* d, const char* src);
static char* _dt_strecat(_dt_string_t* d, const char* src);

static void _dt_fensure(const char* file);
static int _dt_flen(FILE* fp);
static char* _dt_freadall(const char* file);
static void _dt_fwriteall(const char* file, void* buf, size_t size);

static void _dt_read_bin(void* des, const unsigned char** buf, size_t size);
static void _dt_write_bin(const void* src, unsigned char** buf, size_t size);

static int _dt_xpl_is_comma(xpl_context_t* _s);
static int _dt_xpl_is_rsolidus(unsigned char _c);
static int _dt_xpl_parse_escape(char** _d, const char** _s);
static int _dt_xpl_need_escape(unsigned char _c);

static void _dt_resize_string(_dt_string_t* s, size_t rs);
static void _dt_resize_members(_dt_object_t* m, size_t rs);
static void _dt_resize_array(_dt_array_t* a, size_t rs);

static int _dt_value_cmp_func(const void* l, const void* r);
static void _dt_value_dest_func(void* p);
static void _dt_value_info_dest_func(void* p);
static void _dt_value_print_func(const void* p);
static void _dt_value_print_info_func(void* p);

static void _dt_ctor__dt_string_t(_dt_string_t* p, ...);
static void _dt_dtor__dt_string_t(_dt_string_t* p);
static void _dt_ctor__dt_value_t(_dt_value_t* p, dt_type_t t);
static void _dt_dtor__dt_value_t(_dt_value_t* p);
static void _dt_ctor__dt_indexed_value_t(_dt_indexed_value_t* p, dt_type_t t);
static void _dt_dtor__dt_indexed_value_t(_dt_indexed_value_t* p);
static void _dt_ctor__dt_pair_t(_dt_pair_t* p, ...);
static void _dt_dtor__dt_pair_t(_dt_pair_t* p);
static void _dt_ctor__dt_array_t(_dt_array_t* p, ...);
static void _dt_dtor__dt_array_t(_dt_array_t* p);
static void _dt_ctor__dt_object_t(_dt_object_t* p, ...);
static void _dt_dtor__dt_object_t(_dt_object_t* p);

static void _dt_indentation_format(_dt_datatree_detail_t* d, _dt_string_t* buf);
static void _dt_value_format(_dt_datatree_detail_t* d, const _dt_value_t* p, _dt_string_t* buf);
static void _dt_pair_format(_dt_datatree_detail_t* d, const _dt_pair_t* p, _dt_string_t* buf);
static void _dt_array_format(_dt_datatree_detail_t* d, const _dt_array_t* p, _dt_string_t* buf);
static void _dt_object_format(_dt_datatree_detail_t* d, const _dt_object_t* p, _dt_string_t* buf);

static void _dt_value_ser(_dt_datatree_detail_t* d, const _dt_value_t* p, unsigned char** buf, size_t* s);
static void _dt_pair_ser(_dt_datatree_detail_t* d, const _dt_pair_t* p, unsigned char** buf, size_t* s);
static void _dt_array_ser(_dt_datatree_detail_t* d, const _dt_array_t* p, unsigned char** buf, size_t* s);
static void _dt_object_ser(_dt_datatree_detail_t* d, const _dt_object_t* p, unsigned char** buf, size_t* s);

static void _dt_value_deser_sad(_dt_datatree_detail_t* d, dt_sad_handler_t* h, const unsigned char** b);
static void _dt_pair_deser_sad(_dt_datatree_detail_t* d, dt_sad_handler_t* h, const unsigned char** b);
static void _dt_array_deser_sad(_dt_datatree_detail_t* d, dt_sad_handler_t* h, const unsigned char** b);
static void _dt_object_deser_sad(_dt_datatree_detail_t* d, dt_sad_handler_t* h, const unsigned char** b);

static void _dt_value_deser(_dt_datatree_detail_t* d, _dt_value_t* p, const unsigned char** b);
static void _dt_pair_deser(_dt_datatree_detail_t* d, _dt_pair_t* p, const unsigned char** b);
static void _dt_array_deser(_dt_datatree_detail_t* d, _dt_array_t* p, const unsigned char** b);
static void _dt_object_deser(_dt_datatree_detail_t* d, _dt_object_t* p, const unsigned char** b);

static void _dt_value_clone(const _dt_value_t* o, _dt_value_t* n);
static void _dt_pair_clone(const _dt_pair_t* o, _dt_pair_t* n);
static void _dt_array_clone(const _dt_array_t* o, _dt_array_t* n);
static void _dt_object_clone(const _dt_object_t* o, _dt_object_t* n);

static int _dt_value_cmp(const _dt_value_t* l, const _dt_value_t* r, dt_bool_t num_raw_cmp);
static int _dt_pair_cmp(const _dt_pair_t* l, const _dt_pair_t* r);
static int _dt_array_cmp(const _dt_array_t* l, const _dt_array_t* r);
static int _dt_object_cmp(const _dt_object_t* l, const _dt_object_t* r);

static dt_status_t _dt_parse_value_sad(xpl_context_t* _s, dt_sad_handler_t* h);
static dt_status_t _dt_parse_pair_sad(xpl_context_t* _s, dt_sad_handler_t* h);
static dt_status_t _dt_parse_array_sad(xpl_context_t* _s, dt_sad_handler_t* h);
static dt_status_t _dt_parse_object_sad(xpl_context_t* _s, dt_sad_handler_t* h);

static dt_status_t _dt_parse_value(xpl_context_t* _s, _dt_value_t* v);
static dt_status_t _dt_parse_pair(xpl_context_t* _s, _dt_pair_t* p);
static dt_status_t _dt_parse_array(xpl_context_t* _s, _dt_array_t* a);
static dt_status_t _dt_parse_object(xpl_context_t* _s, _dt_object_t* o);

static size_t _dt_load_bin_header(_dt_datatree_detail_t* d, const void** b);
static size_t _dt_save_bin_header(_dt_datatree_detail_t* d, const void** b, size_t* s);

static void _dt_on_parse_error(const _dt_datatree_detail_t* dt, dt_status_t status);

/* ========================================================} */

/*
** {========================================================
** Protected function declarations
*/

int _dt_is_separator_char(unsigned char _c);

xpl_status_t _dt_pop_xpl_string(xpl_context_t* _s, char** _o);

void _dt_value_parse(const char* s, dt_sad_handler_t* h);

void _dt_value_assign(_dt_value_t* v, const char* s);

void _dt_on_error(const xpl_context_t* xpl, const char* const* msg, dt_enum_compatible_t status, dt_parse_error_handler_t error_handler);

/* ========================================================} */

/*
** {========================================================
** Private function definitions
*/

int _dt_strcmp(const _dt_string_t* l, const _dt_string_t* r) {
	DT_ASSERT(l && r);
	if(l == r) return 0;

	return strcmp(l->c_str, r->c_str);
}

void _dt_strcpy(_dt_string_t* d, const char* s) {
	size_t l = strlen(s);
	DT_ASSERT(d && s);
	_dt_resize_string(d, l + 1);
	d->length = l;
	memcpy(d->c_str, s, l + 1);
}

char* _dt_strcat(_dt_string_t* des, const char* src) {
	size_t sl = 0;
	size_t dl = 0;
	DT_ASSERT(des && src);

	sl = strlen(src);
	dl = des->length;
	des->length += sl;
	_dt_resize_string(des, des->length + 1);

	memcpy(&(des->c_str[dl]), src, sl + 1);

	return des->c_str;
}

char* _dt_strecat(_dt_string_t* des, const char* src) {
	size_t d = 0;
	size_t l = 0;
	size_t i = 0;
	DT_ASSERT(des && src);
	d = des->length;
	while(src[i]) {
		if(_dt_xpl_need_escape(src[i]))
			l += 2;
		else
			l++;
		i++;
	}
	des->length += l;
	_dt_resize_string(des, des->length + 1);
	while(*src) {
		switch(*src) {
		case '"':
			des->c_str[d++] = '\\';
			des->c_str[d++] = '\"';
			break;
		case '\\':
			des->c_str[d++] = '\\';
			des->c_str[d++] = '\\';
			break;
		case '/':
			des->c_str[d++] = '\\';
			des->c_str[d++] = '/';
			break;
		case '\b':
			des->c_str[d++] = '\\';
			des->c_str[d++] = 'b';
			break;
		case '\f':
			des->c_str[d++] = '\\';
			des->c_str[d++] = 'f';
			break;
		case '\n':
			des->c_str[d++] = '\\';
			des->c_str[d++] = 'n';
			break;
		case '\r':
			des->c_str[d++] = '\\';
			des->c_str[d++] = 'r';
			break;
		case '\t':
			des->c_str[d++] = '\\';
			des->c_str[d++] = 't';
			break;
		default:
			des->c_str[d++] = *src;
		}
		src++;
	}
	des->c_str[d] = '\0';

	return des->c_str;
}

void _dt_fensure(const char* file) {
	FILE* fp = NULL;
	DT_ASSERT(file);
	fp = fopen(file, "rb");
	if(!fp) {
		fp = fopen(file, "wb");
		DT_ASSERT(fp);
		fclose(fp);
	} else {
		fclose(fp);
	}
}

int _dt_flen(FILE* fp) {
	int result = 0;
	int curpos = 0;

	DT_ASSERT(fp);
	curpos = ftell(fp);
	fseek(fp, 0L, SEEK_END);
	result = ftell(fp);
	fseek(fp, curpos, SEEK_SET);

	return result;
}

char* _dt_freadall(const char* file) {
	char* result = NULL;
	FILE* fp = NULL;
	DT_ASSERT(file);
	fp = fopen(file, "rb");
	if(fp != NULL) {
		int l = _dt_flen(fp);
		result = (char*)dt_malloc(l + 4);
		DT_ASSERT(result);
		fread(result, 1, l, fp);
		fclose(fp);
		result[l] = '\0';
		result[l + 1] = 'e';
		result[l + 2] = 'n';
		result[l + 3] = 'd';
	}

	return result;
}

void _dt_fwriteall(const char* file, void* buf, size_t size) {
	FILE* fp = NULL;
	DT_ASSERT(file);
	fp = fopen(file, "wb");
	if(fp != NULL) {
		fwrite(buf, 1, size, fp);
		fclose(fp);
	}
}

void _dt_read_bin(void* des, const unsigned char** buf, size_t size) {
	unsigned char* _des = (unsigned char*)des;
	DT_ASSERT(des && buf && *buf && size);
#ifdef DT_LITTLE_ENDIAN
	dt_memcpy(_des, *buf, size);
#else /* DT_LITTLE_ENDIAN */
	{
		size_t i = size;
		while(i) {
			_des[size - i] = (*buf)[i];
			i--;
		}
	}
#endif /* DT_LITTLE_ENDIAN */
	*buf += size;
}

void _dt_write_bin(const void* src, unsigned char** buf, size_t size) {
	const unsigned char* _src = (const unsigned char*)src;
	DT_ASSERT(src && buf && *buf && size);
#ifdef DT_LITTLE_ENDIAN
	dt_memcpy(*buf, _src, size);
#else /* DT_LITTLE_ENDIAN */
	{
		size_t i = size;
		while(i) {
			(*buf)[size - i] = _src[i];
			i--;
		}
	}
#endif /* DT_LITTLE_ENDIAN */
	*buf += size;
}

int _dt_xpl_is_comma(xpl_context_t* _s) {
	int ret = 0;
	DT_ASSERT(_s);
	ret = _xpl_is_comma(*((unsigned char*)_s->cursor));
	if(ret) _s->cursor++;

	return ret;
}

int _dt_xpl_is_rsolidus(unsigned char _c) {
	return _c == '\\';
}

int _dt_xpl_parse_escape(char** _d, const char** _s) {
	int ret = 0;
	if(*++*_s) {
		switch(*(*_s)++) {
		case '"':
			*(*_d)++ = '"'; ret++;
			break;
		case '\\':
			*(*_d)++ = '\\'; ret++;
			break;
		case '/':
			*(*_d)++ = '/'; ret++;
			break;
		case 'b':
			*(*_d)++ = '\b'; ret++;
			break;
		case 'f':
			*(*_d)++ = '\f'; ret++;
			break;
		case 'n':
			*(*_d)++ = '\n'; ret++;
			break;
		case 'r':
			*(*_d)++ = '\r'; ret++;
			break;
		case 't':
			*(*_d)++ = '\t'; ret++;
			break;
		case 'u': {
				char buf[] = { 0, 0, 0, 0 };
				char* bp = buf;
				unsigned codepoint = 0;
				int i = 0;
				int s = 0;
				dt_str_to_hex4(_s, &codepoint);
				s = dt_utf8_encoding(&bp, codepoint);
				for(i = 0; i < s; i++)
					*(*_d)++ = buf[i];
				ret += 4;
			}
			break;
		default:
			break;
		}
	}

	return ret;
}

int _dt_xpl_need_escape(unsigned char _c) {
	return _c == '"' || _c == '\\' || _c == '/' || _c == '\b' ||
		_c == '\f' || _c == '\n' || _c == '\r' || _c == '\t';
}

void _dt_resize_string(_dt_string_t* s, size_t rs) {
	DT_ASSERT(s);
	if(rs >= s->size) {
		while(rs >= s->size) s->size += _DT_ARRAY_GROW_STEP;
		s->c_str = (char*)dt_realloc((void**)&s->c_str, s->size);
	}
}

void _dt_resize_members(_dt_object_t* m, size_t rs) {
	DT_ASSERT(m);
	if(rs >= m->size) {
		while(rs >= m->size) m->size += _DT_ARRAY_GROW_STEP;
		m->members = (_dt_pair_t*)dt_realloc((void**)&m->members, m->size * sizeof(_dt_pair_t));
	}
}

void _dt_resize_array(_dt_array_t* a, size_t rs) {
	DT_ASSERT(a);
	if(rs >= a->count) {
		a->count = rs;
		a->elems = (_dt_value_t*)dt_realloc((void**)&a->elems, a->count * sizeof(_dt_value_t));
	}
}

int _dt_value_cmp_func(const void* l, const void* r) {
	int result = 0;
	_dt_value_t* lv = (_dt_value_t*)l;
	_dt_value_t* rv = (_dt_value_t*)r;
	DT_ASSERT(l && r);
	result = _dt_value_cmp(lv, rv, DT_TRUE);

	return DT_SGN(result);
}

void _dt_value_dest_func(void* p) {
	_dt_value_t* k = (_dt_value_t*)p;
	DT_ASSERT(p);
	(void)k;
}

void _dt_value_info_dest_func(void* p) {
	_dt_indexed_value_t* v = (_dt_indexed_value_t*)p;
	DT_ASSERT(p);
	(void)v;
}

void _dt_value_print_func(const void* p) {
	DT_ASSERT(p);
}

void _dt_value_print_info_func(void* p) {
	DT_ASSERT(p);
}

void _dt_ctor__dt_string_t(_dt_string_t* p, ...) {
	DT_ASSERT(p);
	dt_memclr(p, sizeof(_dt_string_t));
}

void _dt_dtor__dt_string_t(_dt_string_t* p) {
	DT_ASSERT(p);
	if(p->c_str) dt_free((void**)&p->c_str);
	p->size = p->length = 0;
}

void _dt_ctor__dt_value_t(_dt_value_t* p, dt_type_t t) {
	DT_ASSERT(p);
	dt_memclr(p, sizeof(_dt_value_t));
	switch(t) {
	case DT_NULL: /* fall through */
	case DT_BOOL: /* fall through */
	case DT_BYTE: /* fall through */
	case DT_UBYTE: /* fall through */
	case DT_SHORT: /* fall through */
	case DT_USHORT: /* fall through */
	case DT_INT: /* fall through */
	case DT_UINT: /* fall through */
	case DT_LONG: /* fall through */
	case DT_ULONG: /* fall through */
	case DT_SINGLE: /* fall through */
	case DT_DOUBLE: /* do nothing */
		break;
	case DT_STRING:
		_DT_NEW(_dt_string_t, p->data._str, 0);
		break;
	case DT_OBJECT:
		_DT_NEW(_dt_object_t, p->data._obj, 0);
		break;
	case DT_ARRAY:
		_DT_CTOR(_dt_array_t, &p->data._array, 0);
		break;
	default:
		DT_ASSERT(0 && "Invalid type");
	}
	p->type = t;
}

void _dt_dtor__dt_value_t(_dt_value_t* p) {
	DT_ASSERT(p);
	switch(p->type) {
	case DT_NULL: /* fall through */
	case DT_BOOL: /* fall through */
	case DT_BYTE: /* fall through */
	case DT_UBYTE: /* fall through */
	case DT_SHORT: /* fall through */
	case DT_USHORT: /* fall through */
	case DT_INT: /* fall through */
	case DT_UINT: /* fall through */
	case DT_LONG: /* fall through */
	case DT_ULONG: /* fall through */
	case DT_SINGLE: /* fall through */
	case DT_DOUBLE: /* do nothing */
		break;
	case DT_STRING:
		_DT_DEL(_dt_string_t, p->data._str);
		break;
	case DT_OBJECT:
		_DT_DEL(_dt_object_t, p->data._obj);
		break;
	case DT_ARRAY:
		_DT_DTOR(_dt_array_t, &p->data._array);
		break;
	default:
		DT_ASSERT(0 && "Invalid type");
	}
	p->type = DT_NULL;
	dt_memclr(p->data._raw, sizeof(_dt_value_raw_t));
}

void _dt_ctor__dt_indexed_value_t(_dt_indexed_value_t* p, dt_type_t t) {
	_DT_CTOR(_dt_value_t, &p->obj, t);
	p->index = 0;
}

void _dt_dtor__dt_indexed_value_t(_dt_indexed_value_t* p) {
	_DT_DTOR(_dt_value_t, &p->obj);
}

void _dt_ctor__dt_pair_t(_dt_pair_t* p, ...) {
	DT_ASSERT(p);
	dt_memclr(p, sizeof(_dt_pair_t));
	_DT_NEW(_dt_value_t, p->key, DT_NULL);
	_DT_NEW(_dt_indexed_value_t, p->value, DT_NULL);
}

void _dt_dtor__dt_pair_t(_dt_pair_t* p) {
	DT_ASSERT(p);
	_DT_DEL(_dt_value_t, p->key);
	_DT_DEL(_dt_indexed_value_t, p->value);
}

void _dt_ctor__dt_array_t(_dt_array_t* p, ...) {
	DT_ASSERT(p);
	p->readonly = 0;
}

void _dt_dtor__dt_array_t(_dt_array_t* p) {
	size_t i = 0;
	DT_ASSERT(p);
	if(p->elems) {
		for(i = 0; i < p->count; i++) {
			_DT_DTOR(_dt_value_t, &p->elems[i]);
		}
		dt_free((void**)&p->elems);
		p->count = 0;
	}
}

void _dt_ctor__dt_object_t(_dt_object_t* p, ...) {
	DT_ASSERT(p);
	p->members = NULL;
	p->size = 0;
	p->count = 0;
	p->readonly = 0;
	p->mapped = dt_rbtree_create(
		_dt_value_cmp_func,
		_dt_value_dest_func,
		_dt_value_info_dest_func,
		_dt_value_print_func,
		_dt_value_print_info_func
	);
}

void _dt_dtor__dt_object_t(_dt_object_t* p) {
	size_t i = 0;
	DT_ASSERT(p);
	dt_rbtree_destroy(p->mapped);
	if(p->members) {
		for(i = 0; i < p->count; i++) {
			_DT_DTOR(_dt_pair_t, &p->members[i]);
		}
		dt_free((void**)&p->members);
		p->size = p->count = 0;
	}
}

void _dt_indentation_format(_dt_datatree_detail_t* d, _dt_string_t* buf) {
	size_t i = 0;
	DT_ASSERT(d && buf);
	if((dt_datatree_t)d == DT_DUMMY_DATATREE) return;
	if(d->compact_mode) return;
	for(i = 0; i < d->format_indentation; i++)
		_dt_strcat(buf, "\t");
}

void _dt_value_format(_dt_datatree_detail_t* d, const _dt_value_t* p, _dt_string_t* buf) {
	char tmp[64];
	DT_ASSERT(d && p && buf);
	switch(p->type) {
	case DT_NULL:
		_dt_strcat(buf, "null");
		break;
	case DT_BOOL:
		_dt_strcat(buf, p->data._bool ? "true" : "false");
		break;
	case DT_BYTE:
		sprintf(tmp, "%d", p->data._byte);
		_dt_strcat(buf, tmp);
		break;
	case DT_UBYTE:
		sprintf(tmp, "%d", p->data._ubyte);
		_dt_strcat(buf, tmp);
		break;
	case DT_SHORT:
		sprintf(tmp, "%d", p->data._short);
		_dt_strcat(buf, tmp);
		break;
	case DT_USHORT:
		sprintf(tmp, "%d", p->data._ushort);
		_dt_strcat(buf, tmp);
		break;
	case DT_INT:
		sprintf(tmp, "%d", p->data._int);
		_dt_strcat(buf, tmp);
		break;
	case DT_UINT:
		sprintf(tmp, "%d", p->data._uint);
		_dt_strcat(buf, tmp);
		break;
	case DT_LONG:
		sprintf(tmp, "%ld", p->data._long);
		_dt_strcat(buf, tmp);
		break;
	case DT_ULONG:
		sprintf(tmp, "%lu", p->data._ulong);
		_dt_strcat(buf, tmp);
		break;
	case DT_SINGLE:
		sprintf(tmp, "%f", p->data._single);
		_dt_strcat(buf, tmp);
		break;
	case DT_DOUBLE:
		sprintf(tmp, "%g", p->data._double);
		_dt_strcat(buf, tmp);
		break;
	case DT_STRING: {
			size_t i = 0;
			int use = 0;
			if(p->data._str->c_str[i] == '\0') use = 1;
			while(p->data._str->c_str[i] != '\0') {
				if(_dt_is_separator_char(p->data._str->c_str[i]) || _xpl_is_separator(p->data._str->c_str[i], NULL) || _dt_xpl_need_escape(p->data._str->c_str[i])) { use = 1; break; }
				else if(i >= 64) { use = 1; break; }
				i++;
			}
			if(use) _dt_strcat(buf, "\"");
			_dt_strecat(buf, p->data._str->c_str);
			if(use) _dt_strcat(buf, "\"");
		}
		break;
	case DT_OBJECT:
		_dt_object_format(d, p->data._obj, buf);
		break;
	case DT_ARRAY:
		_dt_array_format(d, &p->data._array, buf);
		break;
	default:
		DT_ASSERT(0 && "Invalid type");
	}
}

void _dt_pair_format(_dt_datatree_detail_t* d, const _dt_pair_t* p, _dt_string_t* buf) {
	DT_ASSERT(d && p && buf);

	_dt_indentation_format(d, buf);
	_dt_value_format(d, p->key, buf);

	if((dt_datatree_t)d != DT_DUMMY_DATATREE)
		_dt_strcat(buf, d->compact_mode ? ":" : " : ");
	else
		_dt_strcat(buf, ":");

	_dt_value_format(d, &p->value->obj, buf);
}

void _dt_array_format(_dt_datatree_detail_t* d, const _dt_array_t* p, _dt_string_t* buf) {
	size_t i = 0;
	DT_ASSERT(d && p && buf);

	if((dt_datatree_t)d != DT_DUMMY_DATATREE) {
		_dt_strcat(buf, d->compact_mode ? "[" : "[\n");
		d->format_indentation++;
	} else {
		_dt_strcat(buf, "[");
	}

	for(i = 0; i < p->count; i++) {
		_dt_indentation_format(d, buf);
		_dt_value_format(d, &p->elems[i], buf);
		if(i != p->count - 1) {
			if((dt_datatree_t)d != DT_DUMMY_DATATREE)
				_dt_strcat(buf, d->compact_mode ? "," : ",\n");
			else
				_dt_strcat(buf, ",");
		} else {
			if((dt_datatree_t)d != DT_DUMMY_DATATREE && !d->compact_mode)
				_dt_strcat(buf, "\n");
		}
	}

	if((dt_datatree_t)d != DT_DUMMY_DATATREE)
		d->format_indentation--;
	_dt_indentation_format(d, buf);
	_dt_strcat(buf, "]");
}

void _dt_object_format(_dt_datatree_detail_t* d, const _dt_object_t* p, _dt_string_t* buf) {
	size_t i = 0;
	DT_ASSERT(d && p && buf);

	if((dt_datatree_t)d != DT_DUMMY_DATATREE) {
		_dt_strcat(buf, d->compact_mode ? "{" : "{\n");
		d->format_indentation++;
	} else {
		_dt_strcat(buf, "{");
	}

	for(i = 0; i < p->count; i++) {
		_dt_pair_format(d, &p->members[i], buf);
		if(i != p->count - 1) {
			if((dt_datatree_t)d != DT_DUMMY_DATATREE)
				_dt_strcat(buf, d->compact_mode ? "," : ",\n");
			else
				_dt_strcat(buf, ",");
		} else {
			if((dt_datatree_t)d != DT_DUMMY_DATATREE && !d->compact_mode)
				_dt_strcat(buf, "\n");
		}
	}

	if((dt_datatree_t)d != DT_DUMMY_DATATREE)
		d->format_indentation--;
	_dt_indentation_format(d, buf);
	_dt_strcat(buf, "}");
}

void _dt_value_ser(_dt_datatree_detail_t* d, const _dt_value_t* p, unsigned char** buf, size_t* s) {
	DT_ASSERT(d && p && buf && *buf && s);
	_DT_RESIZE_BIN(d, buf, sizeof(unsigned int), s); /* c */
	_DT_WRITE_BIN(unsigned int, &p->type, buf);
	switch(p->type) {
	case DT_NULL: /* do nothing */
		break;
	case DT_BOOL:
		_DT_RESIZE_BIN(d, buf, sizeof(dt_bool_t), s);
		_DT_WRITE_BIN(dt_bool_t, &p->data._bool, buf);
		break;
	case DT_BYTE:
		_DT_RESIZE_BIN(d, buf, sizeof(char), s);
		_DT_WRITE_BIN(char, &p->data._byte, buf);
		break;
	case DT_UBYTE:
		_DT_RESIZE_BIN(d, buf, sizeof(unsigned char), s);
		_DT_WRITE_BIN(unsigned char, &p->data._ubyte, buf);
		break;
	case DT_SHORT:
		_DT_RESIZE_BIN(d, buf, sizeof(short), s);
		_DT_WRITE_BIN(short, &p->data._short, buf);
		break;
	case DT_USHORT:
		_DT_RESIZE_BIN(d, buf, sizeof(unsigned short), s);
		_DT_WRITE_BIN(unsigned short, &p->data._ushort, buf);
		break;
	case DT_INT:
		_DT_RESIZE_BIN(d, buf, sizeof(int), s);
		_DT_WRITE_BIN(int, &p->data._int, buf);
		break;
	case DT_UINT:
		_DT_RESIZE_BIN(d, buf, sizeof(unsigned int), s);
		_DT_WRITE_BIN(unsigned int, &p->data._uint, buf);
		break;
	case DT_LONG:
		_DT_RESIZE_BIN(d, buf, sizeof(long), s);
		_DT_WRITE_BIN(long, &p->data._long, buf);
		break;
	case DT_ULONG:
		_DT_RESIZE_BIN(d, buf, sizeof(unsigned long), s);
		_DT_WRITE_BIN(unsigned long, &p->data._ulong, buf);
		break;
	case DT_SINGLE:
		_DT_RESIZE_BIN(d, buf, sizeof(float), s);
		_DT_WRITE_BIN(float, &p->data._single, buf);
		break;
	case DT_DOUBLE:
		_DT_RESIZE_BIN(d, buf, sizeof(double), s);
		_DT_WRITE_BIN(double, &p->data._double, buf);
		break;
	case DT_STRING:
		_DT_RESIZE_BIN(d, buf, sizeof(size_t), s);
		_DT_WRITE_BIN(size_t, &p->data._str->length, buf);
		_DT_RESIZE_BIN(d, buf, p->data._str->length + 1, s);
		_dt_write_bin(p->data._str->c_str, buf, p->data._str->length + 1);
		break;
	case DT_OBJECT:
		_dt_object_ser(d, p->data._obj, buf, s);
		break;
	case DT_ARRAY:
		_dt_array_ser(d, &p->data._array, buf, s);
		break;
	default:
		DT_ASSERT(0 && "Invalid type");
	}
}

void _dt_pair_ser(_dt_datatree_detail_t* d, const _dt_pair_t* p, unsigned char** buf, size_t* s) {
	DT_ASSERT(d && p && buf && *buf && s);
	_dt_value_ser(d, p->key, buf, s);
	_dt_value_ser(d, &p->value->obj, buf, s);
}

void _dt_array_ser(_dt_datatree_detail_t* d, const _dt_array_t* p, unsigned char** buf, size_t* s) {
	size_t i = 0;
	size_t c = 0;
	DT_ASSERT(d && p && buf && *buf && s);
	_DT_RESIZE_BIN(d, buf, sizeof(size_t), s);
	c = p->count;
	_DT_WRITE_BIN(size_t, &c, buf);
	for(i = 0; i < p->count; i++)
		_dt_value_ser(d, &p->elems[i], buf, s);
}

void _dt_object_ser(_dt_datatree_detail_t* d, const _dt_object_t* p, unsigned char** buf, size_t* s) {
	size_t i = 0;
	size_t c = 0;
	DT_ASSERT(d && p && buf && *buf && s);
	_DT_RESIZE_BIN(d, buf, sizeof(size_t), s);
	c = p->count;
	_DT_WRITE_BIN(size_t, &c, buf);
	for(i = 0; i < p->count; i++)
		_dt_pair_ser(d, &p->members[i], buf, s);
}

void _dt_value_deser_sad(_dt_datatree_detail_t* d, dt_sad_handler_t* h, const unsigned char** b) {
	dt_type_t t = DT_NULL;
	DT_ASSERT(d && h && b && *b);
	_DT_READ_BIN(unsigned int, &t, b);
	switch((dt_type_t)t) {
	case DT_NULL:
		(*h->simple_parsed)(DT_NULL, NULL, h);
		break;
	case DT_BOOL: {
			dt_bool_t val = DT_FALSE;
			_DT_READ_BIN(dt_bool_t, &val, b);
			(*h->simple_parsed)(DT_BOOL, &val, h);
		}
		break;
	case DT_BYTE: {
			char val = 0;
			_DT_READ_BIN(char, &val, b);
			(*h->simple_parsed)(DT_BYTE, &val, h);
		}
		break;
	case DT_UBYTE: {
			unsigned char val = 0;
			_DT_READ_BIN(unsigned char, &val, b);
			(*h->simple_parsed)(DT_UBYTE, &val, h);
		}
		break;
	case DT_SHORT: {
			short val = 0;
			_DT_READ_BIN(short, &val, b);
			(*h->simple_parsed)(DT_SHORT, &val, h);
		}
		break;
	case DT_USHORT: {
			unsigned short val = 0;
			_DT_READ_BIN(unsigned short, &val, b);
			(*h->simple_parsed)(DT_USHORT, &val, h);
		}
		break;
	case DT_INT: {
			int val = 0;
			_DT_READ_BIN(int, &val, b);
			(*h->simple_parsed)(DT_INT, &val, h);
		}
		break;
	case DT_UINT: {
			unsigned int val = 0;
			_DT_READ_BIN(unsigned int, &val, b);
			(*h->simple_parsed)(DT_UINT, &val, h);
		}
		break;
	case DT_LONG: {
			long val = 0;
			_DT_READ_BIN(long, &val, b);
			(*h->simple_parsed)(DT_LONG, &val, h);
		}
		break;
	case DT_ULONG: {
			unsigned long val = 0;
			_DT_READ_BIN(unsigned long, &val, b);
			(*h->simple_parsed)(DT_ULONG, &val, h);
		}
		break;
	case DT_SINGLE: {
			float val = 0;
			_DT_READ_BIN(float, &val, b);
			(*h->simple_parsed)(DT_SINGLE, &val, h);
		}
		break;
	case DT_DOUBLE: {
			double val = 0;
			_DT_READ_BIN(double, &val, b);
			(*h->simple_parsed)(DT_DOUBLE, &val, h);
		}
		break;
	case DT_STRING: {
			size_t len = 0;
			char* buf = NULL;

			_DT_READ_BIN(size_t, &len, b);

			buf = (char*)DT_ALLOCA(len + 1);
			_dt_read_bin(buf, b, len + 1);
			(*h->simple_parsed)(DT_STRING, (void*)buf, h);
			DT_DEALLOCA(buf);
		}
		break;
	case DT_OBJECT:
		(*h->object_begin)(h);
		_dt_object_deser_sad(d, h, b);
		(*h->object_end)(h);
		break;
	case DT_ARRAY:
		(*h->array_begin)(h);
		_dt_array_deser_sad(d, h, b);
		(*h->array_end)(h);
		break;
	default:
		DT_ASSERT(0 && "Invalid type");
	}
}

void _dt_pair_deser_sad(_dt_datatree_detail_t* d, dt_sad_handler_t* h, const unsigned char** b) {
	DT_ASSERT(d && h && b && *b);
	_dt_value_deser_sad(d, h, b);
	_dt_value_deser_sad(d, h, b);
}

void _dt_array_deser_sad(_dt_datatree_detail_t* d, dt_sad_handler_t* h, const unsigned char** b) {
	size_t i = 0;
	size_t c = 0;
	DT_ASSERT(d && h && b && *b);
	_DT_READ_BIN(size_t, &c, b);
	for(i = 0; i < c; i++)
		_dt_value_deser_sad(d, h, b);
}

void _dt_object_deser_sad(_dt_datatree_detail_t* d, dt_sad_handler_t* h, const unsigned char** b) {
	size_t i = 0;
	size_t c = 0;
	DT_ASSERT(d && h && b && *b);
	_DT_READ_BIN(size_t, &c, b);
	for(i = 0; i < c; i++) {
		_dt_pair_deser_sad(d, h, b);
	}
}

void _dt_value_deser(_dt_datatree_detail_t* d, _dt_value_t* p, const unsigned char** b) {
	dt_type_t t = DT_NULL;
	DT_ASSERT(d && p && b && *b);
	_DT_READ_BIN(unsigned int, &t, b);
	switch((dt_type_t)t) {
	case DT_NULL:
		_DT_CTOR(_dt_value_t, p, DT_NULL);
		break;
	case DT_BOOL:
		_DT_CTOR(_dt_value_t, p, DT_BOOL);
		_DT_READ_BIN(dt_bool_t, &p->data._bool, b);
		break;
	case DT_BYTE:
		_DT_CTOR(_dt_value_t, p, DT_BYTE);
		_DT_READ_BIN(char, &p->data._byte, b);
		break;
	case DT_UBYTE:
		_DT_CTOR(_dt_value_t, p, DT_UBYTE);
		_DT_READ_BIN(unsigned char, &p->data._ubyte, b);
		break;
	case DT_SHORT:
		_DT_CTOR(_dt_value_t, p, DT_SHORT);
		_DT_READ_BIN(short, &p->data._short, b);
		break;
	case DT_USHORT:
		_DT_CTOR(_dt_value_t, p, DT_USHORT);
		_DT_READ_BIN(unsigned short, &p->data._ushort, b);
		break;
	case DT_INT:
		_DT_CTOR(_dt_value_t, p, DT_INT);
		_DT_READ_BIN(int, &p->data._int, b);
		break;
	case DT_UINT:
		_DT_CTOR(_dt_value_t, p, DT_UINT);
		_DT_READ_BIN(unsigned int, &p->data._uint, b);
		break;
	case DT_LONG:
		_DT_CTOR(_dt_value_t, p, DT_LONG);
		_DT_READ_BIN(long, &p->data._long, b);
		break;
	case DT_ULONG:
		_DT_CTOR(_dt_value_t, p, DT_ULONG);
		_DT_READ_BIN(unsigned long, &p->data._ulong, b);
		break;
	case DT_SINGLE:
		_DT_CTOR(_dt_value_t, p, DT_SINGLE);
		_DT_READ_BIN(float, &p->data._single, b);
		break;
	case DT_DOUBLE:
		_DT_CTOR(_dt_value_t, p, DT_DOUBLE);
		_DT_READ_BIN(double, &p->data._double, b);
		break;
	case DT_STRING:
		_DT_CTOR(_dt_value_t, p, DT_STRING);

		_DT_READ_BIN(size_t, &p->data._str->length, b);

		_dt_resize_string(p->data._str, p->data._str->length + 1);
		_dt_read_bin(p->data._str->c_str, b, p->data._str->length + 1);
		break;
	case DT_OBJECT:
		_DT_CTOR(_dt_value_t, p, DT_OBJECT);
		_dt_object_deser(d, p->data._obj, b);
		break;
	case DT_ARRAY:
		_DT_CTOR(_dt_value_t, p, DT_ARRAY);
		_dt_array_deser(d, &p->data._array, b);
		break;
	default:
		DT_ASSERT(0 && "Invalid type");
	}
}

void _dt_pair_deser(_dt_datatree_detail_t* d, _dt_pair_t* p, const unsigned char** b) {
	DT_ASSERT(d && p && b && *b);
	_dt_value_deser(d, p->key, b);
	_DT_MAKE_VALUE_READONLY(p->key, 1);
	_dt_value_deser(d, &p->value->obj, b);
}

void _dt_array_deser(_dt_datatree_detail_t* d, _dt_array_t* p, const unsigned char** b) {
	size_t i = 0;
	size_t c = 0;
	DT_ASSERT(d && p && b && *b);
	_DT_READ_BIN(size_t, &c, b);
	_dt_resize_array(p, c);
	for(i = 0; i < c; i++)
		_dt_value_deser(d, &p->elems[i], b);
}

void _dt_object_deser(_dt_datatree_detail_t* d, _dt_object_t* p, const unsigned char** b) {
	size_t i = 0;
	size_t c = 0;
	DT_ASSERT(d && p && b && *b);
	_DT_READ_BIN(size_t, &c, b);
	_dt_resize_members(p, c);
	p->count = c;
	for(i = 0; i < c; i++) {
		_dt_pair_t* pair = &p->members[i];
		_DT_CTOR(_dt_pair_t, pair, 0);
		_dt_pair_deser(d, pair, b);
		pair->value->index = i;

		dt_rbtree_insert(p->mapped, pair->key, pair->value);
	}
}

void _dt_value_clone(const _dt_value_t* o, _dt_value_t* n) {
	DT_ASSERT(o && n && n->type == DT_NULL);
	switch(o->type) {
	case DT_NULL: /* do nothing */
		break;
	case DT_BOOL: /* fall through */
	case DT_BYTE: /* fall through */
	case DT_UBYTE: /* fall through */
	case DT_SHORT: /* fall through */
	case DT_USHORT: /* fall through */
	case DT_INT: /* fall through */
	case DT_UINT: /* fall through */
	case DT_LONG: /* fall through */
	case DT_ULONG: /* fall through */
	case DT_SINGLE: /* fall through */
	case DT_DOUBLE:
		dt_memcpy(n->data._raw, o->data._raw, sizeof(_dt_value_raw_t));
		break;
	case DT_STRING:
		_DT_NEW(_dt_string_t, n->data._str, 0);
		_dt_strcpy(n->data._str, o->data._str->c_str);
		break;
	case DT_OBJECT:
		_DT_NEW(_dt_object_t, n->data._obj, 0);
		_dt_object_clone(o->data._obj, n->data._obj);
		break;
	case DT_ARRAY:
		_DT_CTOR(_dt_array_t, &n->data._array, 0);
		_dt_array_clone(&o->data._array, &n->data._array);
		break;
	default:
		DT_ASSERT(0 && "Invalid type");
	}
	n->type = o->type;
}

void _dt_pair_clone(const _dt_pair_t* o, _dt_pair_t* n) {
	DT_ASSERT(o && n);
	_dt_value_clone(o->key, n->key);
	_dt_value_clone(&o->value->obj, &n->value->obj);
	n->value->index = o->value->index;
}

void _dt_array_clone(const _dt_array_t* o, _dt_array_t* n) {
	size_t i = 0;
	DT_ASSERT(o && n);
	_DT_CTOR(_dt_array_t, n, 0);
	_dt_resize_array(n, o->count);
	n->readonly = o->readonly;
	for(i = 0; i < o->count; i++) {
		_DT_CTOR(_dt_value_t, &n->elems[i], DT_NULL);
		_dt_value_clone(&o->elems[i], &n->elems[i]);
	}
}

void _dt_object_clone(const _dt_object_t* o, _dt_object_t* n) {
	size_t i = 0;
	DT_ASSERT(o && n);
	_dt_resize_members(n, o->count);
	n->count = o->count;
	n->readonly = o->readonly;
	for(i = 0; i < o->count; i++) {
		_dt_pair_t* pair = &n->members[i];
		_DT_CTOR(_dt_pair_t, pair, 0);
		_dt_pair_clone(&o->members[i], pair);
		dt_rbtree_insert(n->mapped, pair->key, pair->value);
	}
}

int _dt_value_cmp(const _dt_value_t* l, const _dt_value_t* r, dt_bool_t num_raw_cmp) {
	int result = 0;
	DT_ASSERT(l && r);
	if(l->type != r->type) return l->type > r->type ? 1 : -1;
	switch(l->type) {
	case DT_NULL:
		result = 0;
		break;
	case DT_BOOL:
		if(l->data._bool && !r->data._bool) result = 1;
		else if(!l->data._bool && r->data._bool) result = -1;
		else result = 0;
		break;
	case DT_BYTE: /* fall through */
	case DT_UBYTE: /* fall through */
	case DT_SHORT: /* fall through */
	case DT_USHORT: /* fall through */
	case DT_INT: /* fall through */
	case DT_UINT: /* fall through */
	case DT_LONG: /* fall through */
	case DT_ULONG:
		if(num_raw_cmp) {
			result = dt_memcmp(&l->data, &r->data, sizeof(l->data));
		} else {
			double left = 0.0;
			double right = 0.0;
			dt_value_data_as(&left, (dt_value_t)l, DT_DOUBLE);
			dt_value_data_as(&right, (dt_value_t)r, DT_DOUBLE);
			if(left > right) result = 1;
			else if(left < right) result = -1;
			else result = 0;
		}
		break;
	case DT_SINGLE:
		result = _DT_SINGLE_COMP(l->data._single, r->data._single);
		break;
	case DT_DOUBLE:
		result = _DT_DOUBLE_COMP(l->data._double, r->data._double);
		break;
	case DT_STRING:
		result = _dt_strcmp(l->data._str, r->data._str);
		break;
	case DT_OBJECT:
		result = _dt_object_cmp(l->data._obj, r->data._obj);
		break;
	case DT_ARRAY:
		result = _dt_array_cmp(&l->data._array, &r->data._array);
		break;
	default:
		DT_ASSERT(0 && "Invalid type");
	}

	return result;
}

int _dt_pair_cmp(const _dt_pair_t* l, const _dt_pair_t* r) {
	int result = 0;
	DT_ASSERT(l && r);
	result = _dt_value_cmp(l->key, r->key, DT_TRUE);
	if(!result)
		result = _dt_value_cmp(&l->value->obj, &r->value->obj, DT_TRUE);

	return result;
}

int _dt_array_cmp(const _dt_array_t* l, const _dt_array_t* r) {
	int result = 0;
	size_t i = 0;
	size_t c = 0;
	DT_ASSERT(r && r);
	c = _dt_min(l->count, r->count);
	for(i = 0; i < c; i++) {
		result = _dt_value_cmp(&l->elems[i], &r->elems[i], DT_TRUE);
		if(result) break;
	}
	if(l->count != r->count)
		result = (l->count > r->count) ? 1 : -1;

	return result;
}

int _dt_object_cmp(const _dt_object_t* l, const _dt_object_t* r) {
	int result = 0;
	size_t i = 0;
	size_t c = 0;
	DT_ASSERT(l && r);
	c = _dt_min(l->count, r->count);
	for(i = 0; i < c; i++) {
		result = _dt_pair_cmp(&l->members[i], &r->members[i]);
		if(result) break;
	}
	if(l->count != r->count)
		result = (l->count > r->count) ? 1 : -1;

	return result;
}

dt_status_t _dt_parse_value_sad(xpl_context_t* _s, dt_sad_handler_t* h) {
	dt_status_t result = DT_OK;
	char* buf = NULL;
	DT_ASSERT(_s && h);

	XPL_SKIP_MEANINGLESS(_s);
	if(*_s->cursor == '{') {
		(*h->object_begin)(h);
		result = _dt_parse_object_sad(_s, h);
		(*h->object_end)(h);
		XPL_SKIP_MEANINGLESS(_s);
		goto _exit;
	}

	if(*_s->cursor == '[') {
		(*h->array_begin)(h);
		result = _dt_parse_array_sad(_s, h);
		(*h->array_end)(h);
		XPL_SKIP_MEANINGLESS(_s);
		goto _exit;
	}

	if(xpl_has_param(_s) == XS_OK) _dt_pop_xpl_string(_s, &buf);
	else { result = DT_VALUE_EXPECTED; goto _exit; }
	_dt_value_parse(buf, h);
	dt_free((void**)&buf);
	XPL_SKIP_MEANINGLESS(_s);

_exit:
	return result;
}

dt_status_t _dt_parse_pair_sad(xpl_context_t* _s, dt_sad_handler_t* h) {
	dt_status_t result = DT_OK;
	DT_ASSERT(_s && h);

	result = _dt_parse_value_sad(_s, h);
	if(result != DT_OK) goto _exit;

	if(_xpl_is_colon(*((unsigned char*)_s->cursor))) _s->cursor++;
	else { result = DT_COLON_EXPECTED; goto _exit; }
	XPL_SKIP_MEANINGLESS(_s);

	result = _dt_parse_value_sad(_s, h);

_exit:
	return result;
}

dt_status_t _dt_parse_array_sad(xpl_context_t* _s, dt_sad_handler_t* h) {
	dt_status_t result = DT_OK;
	DT_ASSERT(_s && h);

	if(*_s->cursor == '[') _s->cursor++;
	else { result = DT_LEFT_SQUARE_BRACKET_EXPECTED; goto _exit; }

	XPL_SKIP_MEANINGLESS(_s);
	if(*_s->cursor == ']') {
		_s->cursor++;
		goto _exit;
	}

	do {
		XPL_SKIP_MEANINGLESS(_s);
		result = _dt_parse_value_sad(_s, h);
		if(result != DT_OK) goto _exit;
		XPL_SKIP_MEANINGLESS(_s);
	} while(_dt_xpl_is_comma(_s));

	XPL_SKIP_MEANINGLESS(_s);
	if(*_s->cursor == ']') _s->cursor++;
	else result = DT_RIGHT_SQUARE_BRACKET_EXPECTED;

_exit:
	return result;
}

dt_status_t _dt_parse_object_sad(xpl_context_t* _s, dt_sad_handler_t* h) {
	dt_status_t result = DT_OK;
	DT_ASSERT(_s && h);

	XPL_SKIP_MEANINGLESS(_s);
	if(*_s->cursor == '{') _s->cursor++;
	else { result = DT_LEFT_BRACE_EXPECTED; goto _exit; }

	XPL_SKIP_MEANINGLESS(_s);
	if(*_s->cursor == '}') {
		_s->cursor++;
		goto _exit;
	}

	do {
		XPL_SKIP_MEANINGLESS(_s);
		result = _dt_parse_pair_sad(_s, h);
		if(result != DT_OK) goto _exit;
		XPL_SKIP_MEANINGLESS(_s);
	} while(_dt_xpl_is_comma(_s));

	XPL_SKIP_MEANINGLESS(_s);
	if(*_s->cursor == '}') _s->cursor++;
	else result = DT_RIGHT_BRACE_EXPECTED;

_exit:
	return result;
}

dt_status_t _dt_parse_value(xpl_context_t* _s, _dt_value_t* v) {
	dt_status_t result = DT_OK;
	char* buf = NULL;
	DT_ASSERT(_s && v);

	XPL_SKIP_MEANINGLESS(_s);

	if(*_s->cursor == '{') {
		_DT_CTOR(_dt_value_t, v, DT_OBJECT);
		result = _dt_parse_object(_s, v->data._obj);
		XPL_SKIP_MEANINGLESS(_s);
		goto _exit;
	}

	if(*_s->cursor == '[') {
		_DT_CTOR(_dt_value_t, v, DT_ARRAY);
		result = _dt_parse_array(_s, &v->data._array);
		XPL_SKIP_MEANINGLESS(_s);
		goto _exit;
	}

	if(_xpl_is_comma(*(unsigned char*)_s->cursor)) {
		result = DT_TOO_MANY_COMMAS;
		goto _exit;
	}
	if(xpl_has_param(_s) == XS_OK) _dt_pop_xpl_string(_s, &buf);
	else { result = DT_VALUE_EXPECTED; goto _exit; }
	_dt_value_assign(v, buf);
	dt_free((void**)&buf);
	XPL_SKIP_MEANINGLESS(_s);

_exit:
	return result;
}

dt_status_t _dt_parse_pair(xpl_context_t* _s, _dt_pair_t* p) {
	dt_status_t result = DT_OK;
	DT_ASSERT(_s && p);

	result = _dt_parse_value(_s, p->key);
	if(result != DT_OK) goto _exit;
	_DT_MAKE_VALUE_READONLY(p->key, 1);

	if(_xpl_is_colon(*((unsigned char*)_s->cursor))) _s->cursor++;
	else { result = DT_COLON_EXPECTED; goto _exit; }
	XPL_SKIP_MEANINGLESS(_s);

	result = _dt_parse_value(_s, &p->value->obj);

_exit:
	return result;
}

dt_status_t _dt_parse_array(xpl_context_t* _s, _dt_array_t* a) {
	dt_status_t result = DT_OK;
	DT_ASSERT(_s && a);

	if(*_s->cursor == '[') _s->cursor++;
	else { result = DT_LEFT_SQUARE_BRACKET_EXPECTED; goto _exit; }

	XPL_SKIP_MEANINGLESS(_s);
	if(*_s->cursor == ']') {
		_s->cursor++;
		goto _exit;
	}

	do {
		size_t rs = a->count + 1;
		_dt_value_t* val = NULL;
		_dt_resize_array(a, rs);
		val = &a->elems[a->count - 1];
		_DT_CTOR(_dt_value_t, val, DT_NULL);

		XPL_SKIP_MEANINGLESS(_s);
		result = _dt_parse_value(_s, val);
		if(result != DT_OK) goto _exit;
		XPL_SKIP_MEANINGLESS(_s);
	} while(_dt_xpl_is_comma(_s));

	XPL_SKIP_MEANINGLESS(_s);
	if(*_s->cursor == ']') _s->cursor++;
	else result = DT_RIGHT_SQUARE_BRACKET_EXPECTED;

_exit:
	return result;
}

dt_status_t _dt_parse_object(xpl_context_t* _s, _dt_object_t* o) {
	dt_status_t result = DT_OK;
	DT_ASSERT(_s && o);

	XPL_SKIP_MEANINGLESS(_s);
	if(*_s->cursor == '{') _s->cursor++;
	else { result = DT_LEFT_BRACE_EXPECTED; goto _exit; }

	XPL_SKIP_MEANINGLESS(_s);
	if(*_s->cursor == '}') {
		_s->cursor++;
		goto _exit;
	}

	do {
		size_t rs = ++o->count;
		_dt_pair_t* pair = NULL;
		_dt_resize_members(o, rs);
		pair = &o->members[o->count - 1];
		_DT_CTOR(_dt_pair_t, pair, 0);
		pair->value->index = o->count - 1;

		XPL_SKIP_MEANINGLESS(_s);
		result = _dt_parse_pair(_s, pair);
		if(result != DT_OK) goto _exit;
		XPL_SKIP_MEANINGLESS(_s);

		dt_rbtree_insert(o->mapped, pair->key, pair->value);
	} while(_dt_xpl_is_comma(_s));

	XPL_SKIP_MEANINGLESS(_s);
	if(*_s->cursor == '}') _s->cursor++;
	else result = DT_RIGHT_BRACE_EXPECTED;

_exit:
	return result;
}

size_t _dt_load_bin_header(_dt_datatree_detail_t* d, const void** b) {
	_dt_bin_header_t* header = NULL;
	const unsigned char** bin = (const unsigned char**)b;
	DT_ASSERT(d && b);
	header = &d->bin_header;
	_DT_READ_BIN(size_t, &header->size, bin);
	_DT_READ_BIN(unsigned int, &header->version, bin);
	_DT_READ_BIN(char[2], &header->endian, bin);
	DT_ASSERT(header->endian[0] == 'L' && header->endian[1] == 'E');

	return header->size;
}

size_t _dt_save_bin_header(_dt_datatree_detail_t* d, const void** b, size_t* s) {
	_dt_bin_header_t* header = NULL;
	unsigned char** bin = (unsigned char**)b;
	DT_ASSERT(d && b && *b);
	header = &d->bin_header;
	header->size = sizeof(size_t) + sizeof(unsigned int) + sizeof(char[2]);
	header->version = dt_bver();
	header->endian[0] = 'L';
	header->endian[1] = 'E';

	_DT_RESIZE_BIN(d, bin, sizeof(size_t), s);
	_DT_WRITE_BIN(size_t, &header->size, bin);
	_DT_RESIZE_BIN(d, bin, sizeof(unsigned int), s);
	_DT_WRITE_BIN(unsigned int, &header->version, bin);
	_DT_RESIZE_BIN(d, bin, sizeof(char[2]), s);
	_DT_WRITE_BIN(char[2], &header->endian, bin);

	return header->size;
}

void _dt_on_parse_error(const _dt_datatree_detail_t* dt, dt_status_t status) {
	dt_parse_error_handler_t error_handler = NULL;
	DT_ASSERT(dt);
	error_handler = dt->error_handler;
	if(!error_handler) return;

	_dt_on_error(&dt->xpl, _DT_STATUS_MSG, status, error_handler);
}

/* ========================================================} */

/*
** {========================================================
** Protected function definitions
*/

int _dt_is_separator_char(unsigned char _c) {
	return _c == '[' || _c == ']' ||
		_c == '{' || _c == '}' ||
		_c == '(' || _c == ')' ||
		_c == '<' || _c == '>';
}

xpl_status_t _dt_pop_xpl_string(xpl_context_t* _s, char** _o) {
	const char* src = NULL;
	char* dst = NULL;
	size_t len = _DT_ARRAY_GROW_STEP;
	xpl_assert(_s && _s->text && _o);
	src = _s->cursor;
	dst = *_o = (char*)dt_malloc(len);
	if(_xpl_is_dquote(*(unsigned char*)src)) {
		src++;
		while(!_xpl_is_dquote(*(unsigned char*)src)) {
			if(dst - *_o + _DT_STRING_LOOK_AHEAD > (int)len) {
				size_t diff = dst - *_o;
				len += _DT_ARRAY_GROW_STEP;
				*_o = (char*)dt_realloc((void**)_o, len);
				dst = *_o + diff;
			}
			if(_s->escape_detect && (*_s->escape_detect)(*(unsigned char*)src)) {
				xpl_assert(_s->escape_parse);
				if(!(*_s->escape_parse)(&dst, &src)) {
					*dst++ = '\0';

					return XS_BAD_ESCAPE_FORMAT;
				}
			} else {
				*dst++ = *src++;
			}
		}
		src++;
	} else {
		while(!_xpl_is_separator(*(unsigned char*)src, _s->separator_detect) && *src != '\0') {
			if(dst - *_o + 1 > (int)len) {
				size_t diff = dst - *_o;
				len += _DT_ARRAY_GROW_STEP;
				*_o = (char*)dt_realloc((void**)_o, len);
				dst = *_o + diff;
			}
			*dst++ = *src++;
		}
	}
	_s->cursor = src;
	if(dst - *_o + 1 > (int)len) {
		size_t diff = dst - *_o;
		len += _DT_ARRAY_GROW_STEP;
		*_o = (char*)dt_realloc((void**)_o, len);
		dst = *_o + diff;
	}
	*dst++ = '\0';

	return XS_OK;
}

void _dt_value_parse(const char* s, dt_sad_handler_t* h) {
	if(strcmp(s, "null") == 0) {
		(*h->simple_parsed)(DT_NULL, NULL, h);

		return;
	} else if(strcmp(s, "true") == 0) {
		dt_bool_t val = DT_TRUE;
		(*h->simple_parsed)(DT_BOOL, &val, h);

		return;
	} else if(strcmp(s, "false") == 0) {
		dt_bool_t val = DT_FALSE;
		(*h->simple_parsed)(DT_BOOL, &val, h);

		return;
	} else if(s[0] != '\0') {
		long l = 0;
		double d = 0.0;

		char* conv_suc = NULL;
		l = strtol(s, &conv_suc, 0);
		if(*conv_suc == '\0') {
			(*h->simple_parsed)(DT_LONG, &l, h);

			return;
		}

		conv_suc = NULL;
		d = strtod(s, &conv_suc);
		if(*conv_suc == '\0') {
			if(strlen(s) <= 7) {
				float val = (float)d;
				(*h->simple_parsed)(DT_SINGLE, &val, h);
			} else {
				(*h->simple_parsed)(DT_DOUBLE, &d, h);
			}

			return;
		}
	}

	(*h->simple_parsed)(DT_STRING, (void*)s, h);
}

void _dt_value_assign(_dt_value_t* v, const char* s) {
	if(strcmp(s, "null") == 0) {
		_DT_CTOR(_dt_value_t, v, DT_NULL);

		return;
	} else if(strcmp(s, "true") == 0) {
		_DT_CTOR(_dt_value_t, v, DT_BOOL);
		v->data._bool = DT_TRUE;

		return;
	} else if(strcmp(s, "false") == 0) {
		_DT_CTOR(_dt_value_t, v, DT_BOOL);
		v->data._bool = DT_FALSE;

		return;
	} else if(s[0] != '\0') {
		long l = 0;
		double d = 0.0;

		char* conv_suc = NULL;
		l = strtol(s, &conv_suc, 0);
		if(*conv_suc == '\0') {
			_DT_COMPRESS_LONG(l, v);

			return;
		}

		conv_suc = NULL;
		d = strtod(s, &conv_suc);
		if(*conv_suc == '\0') {
			if(strlen(s) <= 7) {
				_DT_CTOR(_dt_value_t, v, DT_SINGLE);
				v->data._single = (float)d;
			} else {
				_DT_CTOR(_dt_value_t, v, DT_DOUBLE);
				v->data._double = d;
			}

			return;
		}
	}

	_DT_CTOR(_dt_value_t, v, DT_STRING);
	_dt_strcpy(v->data._str, s);
}

void _dt_on_error(const xpl_context_t* xpl, const char* const* msg, dt_enum_compatible_t status, dt_parse_error_handler_t error_handler) {
	const char* source = xpl->text;
	const char* cursor = xpl->cursor;
	size_t row = 1;
	size_t col = 0;
	const char* s = NULL;
	const char* p = NULL;
	char* pos = NULL;
	DT_ASSERT(xpl && source && cursor);
	if(!error_handler) return;
	DT_ASSERT(cursor >= source);
	for(s = source; s < cursor; s++) {
		if(*s == _DT_NEW_LINE) {
			row++;
			col = 0;
			p = s;
		} else {
			col++;
		}
	}
	if(!p) p = (cursor - source > 10) ? (cursor - 10) : source;
	pos = (char*)(DT_ALLOCA(cursor - p + 1));
	memcpy(pos, p, cursor - p);
	pos[cursor - p] = '\0';
	error_handler(status, _DT_STATUS_MSG[status], pos, row, col);
	DT_DEALLOCA(pos);
}

/* ========================================================} */

/*
** {========================================================
** Public function definitions
*/

unsigned int dt_ver(void) {
	return DTVER;
}

unsigned int dt_bver(void) {
	return DTBVER;
}

const char* const dt_status_message(dt_status_t s) {
	DT_ASSERT(s >= 0 && s < DT_COUNT_OF(_DT_STATUS_MSG));
	if(s < 0 || s >= DT_COUNT_OF(_DT_STATUS_MSG))
		return NULL;

	return _DT_STATUS_MSG[s];
}

size_t dt_allocated(void) {
	return _dt_allocated;
}

void* dt_malloc(size_t s) {
	char* ret = NULL;
	size_t rs = s;
#ifdef DT_ENABLE_ALLOC_STAT
	rs += DT_POINTER_SIZE;
#endif /* DT_ENABLE_ALLOC_STAT */
	ret = (char*)malloc(rs);
	DT_ASSERT(ret);
#ifdef DT_ENABLE_ALLOC_STAT
	_dt_allocated += s;
	ret += DT_POINTER_SIZE;
	_DT_WRITE_CHUNK_SIZE(ret, s);
#endif /* DT_ENABLE_ALLOC_STAT */

	return (void*)ret;
}

void* dt_realloc(void** p, size_t s) {
	char* ret = NULL;
	size_t rs = s;
	size_t os = 0; (void)os;
	DT_ASSERT(p);
#ifdef DT_ENABLE_ALLOC_STAT
	if(*p) {
		os = _DT_READ_CHUNK_SIZE(*p);
		*p = (char*)(*p) - DT_POINTER_SIZE;
	}
	rs += DT_POINTER_SIZE;
#endif /* DT_ENABLE_ALLOC_STAT */
	ret = (char*)realloc(*p, rs);
	DT_ASSERT(ret);
#ifdef DT_ENABLE_ALLOC_STAT
	_dt_allocated -= os;
	_dt_allocated += s;
	ret += DT_POINTER_SIZE;
	_DT_WRITE_CHUNK_SIZE(ret, s);
	*p = (void*)ret;
#endif /* DT_ENABLE_ALLOC_STAT */

	return (void*)ret;
}

void dt_free(void** p) {
	DT_ASSERT(p && *p);

#ifdef DT_ENABLE_ALLOC_STAT
	do {
		size_t os = _DT_READ_CHUNK_SIZE(*p);
		_dt_allocated -= os;
		*p = (char*)(*p) - DT_POINTER_SIZE;
	} while(0);
#endif /* DT_ENABLE_ALLOC_STAT */

	free(*p);
	*p = NULL;
}

void dt_memclr(void* p, size_t s) {
	DT_ASSERT(p);

	memset(p, 0, s);
}

int dt_memcmp(const void* l, const void* r, size_t s) {
	DT_ASSERT(l && r);

	return DT_SGN(memcmp(l, r, s));
}

void* dt_memcpy(void* dst, const void* src, size_t s) {
	DT_ASSERT(dst && src);

	return memcpy(dst, src, s);
}

void dt_memswap(void* l, void* r, size_t s) {
	char buf[64];
	void* p = buf;
	if(s > sizeof(buf)) {
		p = DT_ALLOCA(s);
	}
	dt_memcpy(p, l, s);
	dt_memcpy(l, r, s);
	dt_memcpy(r, p, s);
	if(s > sizeof(buf)) {
		DT_DEALLOCA(p);
	}
}

void dt_create_datatree(dt_datatree_t* d, dt_parse_error_handler_t eh) {
	XPL_FUNC_BEGIN_EMPTY(empty)
	XPL_FUNC_END
	_dt_datatree_detail_t* result = (_dt_datatree_detail_t*)dt_malloc(sizeof(_dt_datatree_detail_t));
	xpl_context_t* xpl = NULL;
	dt_memclr(result, sizeof(_dt_datatree_detail_t));
	xpl = &result->xpl;
	xpl_open(xpl, empty, _dt_is_separator_char);
	xpl->use_hack_pfunc = 0;
	xpl->escape_detect = _dt_xpl_is_rsolidus;
	xpl->escape_parse = _dt_xpl_parse_escape;
	xpl->userdata = result;
	result->error_handler = eh;
	*d = (dt_datatree_t)result;
}

void dt_unload_datatree(dt_datatree_t d) {
	_dt_datatree_detail_t* dt = (_dt_datatree_detail_t*)d;
	_DT_DTOR(_dt_value_t, &dt->root);
}

void dt_destroy_datatree(dt_datatree_t d) {
	_dt_datatree_detail_t* dt = (_dt_datatree_detail_t*)d;
	DT_ASSERT((dt->root.type == DT_NULL) && "Did you foget to unload this datatree?");
	xpl_close(&dt->xpl);
	dt_free((void**)&dt);
	DT_ASSERT(d);
}

void* dt_get_datatree_userdata(dt_datatree_t d) {
	_dt_datatree_detail_t* dt = (_dt_datatree_detail_t*)d;
	DT_ASSERT(d);

	return dt->userdata;
}

void* dt_set_datatree_userdata(dt_datatree_t d, void* ud) {
	_dt_datatree_detail_t* dt = (_dt_datatree_detail_t*)d;
	void* old = NULL;
	DT_ASSERT(d);
	old = dt->userdata;
	dt->userdata = ud;

	return old;
}

dt_status_t dt_load_datatree_file_sad(dt_datatree_t d, dt_sad_handler_t* h, const char* f) {
	dt_status_t result = DT_OK;
	char* c = NULL;
	DT_ASSERT(d && f && h);
	c = _dt_freadall(f);
	result = dt_load_datatree_string_sad(d, h, c);
	dt_free((void**)&c);

	return result;
}

dt_status_t dt_load_datatree_file(dt_datatree_t d, const char* f) {
	dt_status_t result = DT_OK;
	char* c = NULL;
	DT_ASSERT(d && f);
	c = _dt_freadall(f);
	result = dt_load_datatree_string(d, c);
	dt_free((void**)&c);

	return result;
}

void dt_save_datatree_file(dt_datatree_t d, const char* f, dt_bool_t compact) {
	_dt_datatree_detail_t* dt = (_dt_datatree_detail_t*)d;
	char* buf = NULL;
	DT_ASSERT(d && f);
	dt->compact_mode = compact;
	dt_save_datatree_string(d, &buf, compact);
	_dt_fwriteall(f, buf, strlen(buf) + 1);
	dt_free((void**)&buf);
}

dt_status_t dt_load_datatree_string_sad(dt_datatree_t d, dt_sad_handler_t* h, const char* s) {
	dt_status_t result = DT_OK;
	_dt_datatree_detail_t* dt = (_dt_datatree_detail_t*)d;
	DT_ASSERT(d && s && h);
	dt_unload_datatree((dt_datatree_t)dt);
	_DT_CTOR(_dt_value_t, &dt->root, DT_NULL);
	xpl_load(&dt->xpl, s);
	result = _dt_parse_value_sad(&dt->xpl, h);
	if(result != DT_OK) {
		_dt_on_parse_error(dt, result);
		result = DT_LOAD_FAILED;
	}
	xpl_unload(&dt->xpl);

	return result;
}

dt_status_t dt_load_datatree_string(dt_datatree_t d, const char* s) {
	dt_status_t result = DT_OK;
	_dt_datatree_detail_t* dt = (_dt_datatree_detail_t*)d;
	DT_ASSERT(d && s);
	dt_unload_datatree((dt_datatree_t)dt);
	_DT_CTOR(_dt_value_t, &dt->root, DT_NULL);
	xpl_load(&dt->xpl, s);
	result = _dt_parse_value(&dt->xpl, &dt->root);
	XPL_SKIP_MEANINGLESS(&dt->xpl);
	if(result == DT_OK && *dt->xpl.cursor != '\0')
		result = DT_REDUNDANCE_CHAR_LEFT;
	if(result != DT_OK) {
		_dt_on_parse_error(dt, result);
		result = DT_LOAD_FAILED;
	}
	xpl_unload(&dt->xpl);

	return result;
}

void dt_save_datatree_string(dt_datatree_t d, char** s, dt_bool_t compact) {
	_dt_datatree_detail_t* dt = (_dt_datatree_detail_t*)d;
	DT_ASSERT(d);
	dt->compact_mode = compact;
	dt_format_value(d, (const dt_value_t)&dt->root, s, compact);
}

dt_status_t dt_load_datatree_bin_sad(dt_datatree_t d, dt_sad_handler_t* h, const void* b) {
	dt_status_t result = DT_OK;
	_dt_datatree_detail_t* dt = (_dt_datatree_detail_t*)d;
	const unsigned char* bin = (const unsigned char*)b;
	DT_ASSERT(d && b && h);
	dt_unload_datatree((dt_datatree_t)dt);
	bin += _dt_load_bin_header(dt, &b);
	_dt_value_deser_sad(dt, h, &bin);

	return result;
}

dt_status_t dt_load_datatree_bin(dt_datatree_t d, const void* b) {
	dt_status_t result = DT_OK;
	_dt_datatree_detail_t* dt = (_dt_datatree_detail_t*)d;
	const unsigned char* bin = (const unsigned char*)b;
	DT_ASSERT(d && b);
	dt_unload_datatree((dt_datatree_t)dt);
	_DT_CTOR(_dt_value_t, &dt->root, DT_NULL);
	bin += _dt_load_bin_header(dt, &b);
	_dt_value_deser(dt, &dt->root, &bin);

	return result;
}

void dt_save_datatree_bin(dt_datatree_t d, void** b, size_t* s) {
	_dt_datatree_detail_t* dt = (_dt_datatree_detail_t*)d;
	unsigned char** bin = (unsigned char**)b;
	DT_ASSERT(d);
	*bin = (unsigned char*)dt_malloc(1);
	dt->ser_buf = *bin;
	_dt_save_bin_header(dt, (const void**)bin, s);
	_dt_value_ser(dt, &dt->root, bin, s);
	*bin = dt->ser_buf;
	dt->ser_buf = NULL;
}

int dt_create_value(dt_datatree_t d, dt_value_t* v, dt_parse_error_handler_t eh, const char* fmt, ...) {
	int result = 1;
	_dt_datatree_detail_t* dt = (_dt_datatree_detail_t*)d;
	_dt_value_t** val = (_dt_value_t**)v;
	va_list argptr;
	va_start(argptr, fmt);
	(void)dt;
	DT_ASSERT(d && v);
	_DT_NEW(_dt_value_t, *val, DT_NULL);
	if(fmt) {
		dt_status_t ret = DT_OK;
		char _buf[DT_STR_LEN];
		char* buf = _buf;
		int need = 0;
		XPL_FUNC_BEGIN_EMPTY(empty)
		XPL_FUNC_END
		xpl_context_t xpl;
		need = 1 + vsnprintf(buf, sizeof(_buf), fmt, argptr);
		if(need > sizeof(_buf)) {
			buf = (char*)dt_malloc(need);
			vsprintf(buf, fmt, argptr);
		}
		xpl_open(&xpl, empty, _dt_is_separator_char);
		xpl.use_hack_pfunc = 0;
		xpl.escape_detect = _dt_xpl_is_rsolidus;
		xpl.escape_parse = _dt_xpl_parse_escape;
		xpl.userdata = d;
		xpl_load(&xpl, buf);
		ret = _dt_parse_value(&xpl, *val);
		if(ret == DT_OK) {
			result = (int)(xpl.cursor - xpl.text);
		} else {
			if(!eh && d && d != DT_DUMMY_DATATREE)
				eh = dt->error_handler;
			if(eh)
				_dt_on_error(&xpl, _DT_STATUS_MSG, ret, eh);
			result = 0;
		}
		xpl_unload(&xpl);
		xpl_close(&xpl);
		if(buf != _buf)
			dt_free((void**)&buf);
	} else {
		dt_type_t t = (dt_type_t)va_arg(argptr, dt_enum_compatible_t);
		switch(t) {
		case DT_NULL: /* do nothing */
			break;
		case DT_BOOL:
			(*val)->data._bool = (dt_bool_t)va_arg(argptr, dt_enum_compatible_t);
			break;
		case DT_BYTE: {
				long l = (long)(char)va_arg(argptr, int/*char*/);
				_DT_COMPRESS_LONG(l, *val);
			}
			break;
		case DT_UBYTE: {
				long l = (long)(unsigned char)va_arg(argptr, int/*unsigned char*/);
				_DT_COMPRESS_LONG(l, *val);
			}
			break;
		case DT_SHORT: {
				long l = (long)(short)va_arg(argptr, int/*short*/);
				_DT_COMPRESS_LONG(l, *val);
			}
			break;
		case DT_USHORT: {
				long l = (long)(unsigned short)va_arg(argptr, int/*unsigned short*/);
				_DT_COMPRESS_LONG(l, *val);
			}
			break;
		case DT_INT: {
				long l = (long)va_arg(argptr, int);
				_DT_COMPRESS_LONG(l, *val);
			}
			break;
		case DT_UINT: {
				long l = (long)va_arg(argptr, unsigned int);
				_DT_COMPRESS_LONG(l, *val);
			}
			break;
		case DT_LONG: {
				long l = (long)va_arg(argptr, long);
				_DT_COMPRESS_LONG(l, *val);
			}
			break;
		case DT_ULONG:
			(*val)->data._ulong = va_arg(argptr, unsigned long);
			break;
		case DT_SINGLE:
			(*val)->data._single = (float)va_arg(argptr, double/*float*/);
			break;
		case DT_DOUBLE:
			(*val)->data._double = va_arg(argptr, double);
			break;
		case DT_STRING: {
				const char* str = va_arg(argptr, const char*);
				_DT_CTOR(_dt_value_t, *val, DT_STRING);
				_dt_strcpy((*val)->data._str, str);
			}
			break;
		case DT_OBJECT:
			DT_ASSERT(0 && "Not supported");
			result = 0;
			break;
		case DT_ARRAY:
			DT_ASSERT(0 && "Not supported");
			result = 0;
			break;
		default:
			DT_ASSERT(0 && "Invalid type");
			result = 0;
		}
		(*val)->type = t;
	}
	va_end(argptr);

	return result;
}

int dt_create_value_ex(dt_datatree_t d, dt_value_t* v, dt_separator_char_detect_func_t sd, dt_parse_error_handler_t eh, const char* fmt) {
	int result = 1;
	_dt_datatree_detail_t* dt = (_dt_datatree_detail_t*)d;
	_dt_value_t** val = (_dt_value_t**)v;
	(void)dt;
	DT_ASSERT(d && v);
	_DT_NEW(_dt_value_t, *val, DT_NULL);
	if(fmt) {
		dt_status_t ret = DT_OK;
		XPL_FUNC_BEGIN_EMPTY(empty)
		XPL_FUNC_END
		xpl_context_t xpl;
		xpl_open(&xpl, empty, sd);
		xpl.use_hack_pfunc = 0;
		xpl.escape_detect = _dt_xpl_is_rsolidus;
		xpl.escape_parse = _dt_xpl_parse_escape;
		xpl.userdata = d;
		xpl_load(&xpl, fmt);
		ret = _dt_parse_value(&xpl, *val);
		if(ret == DT_OK) {
			result = (int)(xpl.cursor - xpl.text);
		} else {
			if(!eh && d && d != DT_DUMMY_DATATREE)
				eh = dt->error_handler;
			if(eh)
				_dt_on_error(&xpl, _DT_STATUS_MSG, ret, eh);
			result = 0;
		}
		xpl_unload(&xpl);
		xpl_close(&xpl);
	}

	return result;
}

void dt_destroy_value(dt_datatree_t d, dt_value_t v) {
	_dt_value_t* val = (_dt_value_t*)v;
	DT_ASSERT(d && v);
	_DT_DEL(_dt_value_t, val);
}

void dt_format_value(dt_datatree_t d, const dt_value_t v, char** s, dt_bool_t compact) {
	_dt_datatree_detail_t* dt = (_dt_datatree_detail_t*)d;
	_dt_value_t* val = (_dt_value_t*)v;
	_dt_string_t str;
	DT_ASSERT(d && v && s && !*s);
	*s = (char*)dt_malloc(_DT_ARRAY_GROW_STEP);
	*s[0] = '\0';
	str.c_str = *s;
	str.length = 0;
	str.size = _DT_ARRAY_GROW_STEP;
	if(d != DT_DUMMY_DATATREE)
		dt->compact_mode = compact;
	_dt_value_format(dt, val, &str);
	*s = str.c_str;
}

void dt_clone_value(dt_datatree_t d, const dt_value_t v, dt_value_t o) {
	_dt_value_t* val = (_dt_value_t*)v;
	_dt_value_t* out = (_dt_value_t*)o;
	DT_ASSERT(d && v && o && v != o);
	if(v == o) return;
	_DT_DTOR(_dt_value_t, out);
	_dt_value_clone(val, out);
}

void dt_find_object_member_by_key(dt_datatree_t d, const dt_object_t o, const dt_value_t key, dt_value_t* val, size_t* idx) {
	_dt_object_t* obj = (_dt_object_t*)o;
	_dt_value_t** v = (_dt_value_t**)val;
	size_t* i = (size_t*)idx;
	dt_rbnode_t rbn = NULL;
	_dt_indexed_value_t* iv = NULL;
	DT_ASSERT(d && o && key && (val || idx));
	rbn = dt_rbtree_exact_query(obj->mapped, key);
	if(rbn) {
		iv = (_dt_indexed_value_t*)dt_rbtree_node_info(rbn);
		if(v) *v = &iv->obj;
		if(i) *i = iv->index;
	} else {
		if(v) *v = NULL;
		if(i) *i = (unsigned int)DT_INVALID_INDEX;
	}
}

void dt_object_member_count(dt_datatree_t d, const dt_object_t o, size_t* c) {
	_dt_object_t* obj = (_dt_object_t*)o;
	DT_ASSERT(d && o && c);
	*c = obj->count;
}

void dt_object_member_at(dt_datatree_t d, const dt_object_t o, size_t idx, dt_value_t* key, dt_value_t* val) {
	_dt_object_t* obj = (_dt_object_t*)o;
	_dt_value_t** k = (_dt_value_t**)key;
	_dt_value_t** v = (_dt_value_t**)val;
	size_t c = 0;
	DT_ASSERT(d && o && (key || val));
	dt_object_member_count(d, o, &c);
	if(idx < c) {
		if(k) *k = obj->members[idx].key;
		if(v) *v = &obj->members[idx].value->obj;
	} else {
		if(k) *k = NULL;
		if(v) *v = NULL;
	}
}

void dt_array_elem_count(dt_datatree_t d, const dt_array_t a, size_t* c) {
	_dt_array_t* arr = (_dt_array_t*)a;
	DT_ASSERT(d && a && c);
	*c = arr->count;
}

void dt_array_elem_at(dt_datatree_t d, const dt_array_t a, size_t idx, dt_value_t* v) {
	_dt_array_t* arr = (_dt_array_t*)a;
	_dt_value_t** val = (_dt_value_t**)v;
	size_t c = 0;
	DT_ASSERT(d && a && v);
	dt_array_elem_count(d, a, &c);
	if(idx < c)
		*val = &arr->elems[idx];
	else
		*val = NULL;
}

dt_status_t dt_add_object_member(dt_datatree_t d, dt_object_t o, dt_value_t key, dt_value_t val, size_t* idx) {
	_dt_object_t* obj = (_dt_object_t*)o;
	DT_ASSERT(d && o && key && val);
	if(idx) *idx = obj->count;

	return dt_insert_object_member(d, o, obj->count, key, val);
}

dt_status_t dt_insert_object_member(dt_datatree_t d, dt_object_t o, size_t where, dt_value_t key, dt_value_t val) {
	_dt_object_t* obj = (_dt_object_t*)o;
	_dt_value_t* k = (_dt_value_t*)key;
	dt_rbnode_t rbn = NULL;
	size_t rs = 0;
	_dt_pair_t* pair = NULL;
	size_t i = 0;
	DT_ASSERT(d && o && key && val);

	if(obj->readonly)
		return DT_VALUE_IS_READONLY;

	rbn = dt_rbtree_exact_query(obj->mapped, k);
	if(rbn) return DT_KEY_EXISTS;

	rs = ++obj->count;
	_dt_resize_members(obj, rs);
	pair = &obj->members[obj->count - 1];
	_DT_CTOR(_dt_pair_t, pair, 0);
	pair->value->index = obj->count - 1;

	if(where < obj->count - 1) {
		_dt_pair_t tmp = obj->members[obj->count - 1];
		memmove(&obj->members[where + 1], &obj->members[where], sizeof(_dt_pair_t) * (obj->count - where - 1));
		obj->members[where] = tmp;
		for(i = where; i < obj->count; i++)
			obj->members[i].value->index = i;
	}

	dt_memswap(obj->members[where].key, key, sizeof(_dt_value_t));
	dt_memswap(&obj->members[where].value->obj, val, sizeof(_dt_value_t));

	dt_rbtree_insert(obj->mapped, pair->key, pair->value);

	return DT_OK;
}

dt_status_t dt_add_array_elem(dt_datatree_t d, dt_array_t a, dt_value_t v, size_t* idx) {
	_dt_array_t* arr = (_dt_array_t*)a;
	DT_ASSERT(d && a && v);
	if(idx) *idx = arr->count;

	return dt_insert_array_elem(d, a, arr->count, v);
}

dt_status_t dt_insert_array_elem(dt_datatree_t d, dt_array_t a, size_t where, dt_value_t v) {
	_dt_array_t* arr = (_dt_array_t*)a;
	_dt_value_t* val = (_dt_value_t*)v;
	size_t rs = 0;
	_dt_value_t* elem = NULL;
	DT_ASSERT(d && a && v);

	if(arr->readonly)
		return DT_VALUE_IS_READONLY;

	rs = ++arr->count;
	_dt_resize_array(arr, rs);
	elem = &arr->elems[arr->count - 1];
	_DT_CTOR(_dt_value_t, elem, DT_NULL);

	if(where < arr->count - 1) {
		_dt_value_t tmp = arr->elems[arr->count - 1];
		memmove(&arr->elems[where + 1], &arr->elems[where], sizeof(_dt_value_t) * (arr->count - where - 1));
		arr->elems[where] = tmp;
	}

	dt_memswap(&arr->elems[where], val, sizeof(_dt_value_t));

	return DT_OK;
}

dt_status_t dt_remove_object_member_by_key(dt_datatree_t d, dt_object_t o, dt_value_t key, dt_value_t val) {
	dt_value_t v = NULL;
	size_t idx = 0;
	DT_ASSERT(d && o && key && val);
	dt_find_object_member_by_key(d, o, key, &v, &idx);
	if(v) return dt_remove_object_member_at(d, o, idx, key, val);
	else return DT_KEY_DOES_NOT_EXIST;
}

dt_status_t dt_delete_object_member_by_key(dt_datatree_t d, dt_object_t o, const dt_value_t key) {
	dt_value_t v = NULL;
	size_t idx = 0;
	DT_ASSERT(d && o && key);
	dt_find_object_member_by_key(d, o, key, &v, &idx);
	if(v) return dt_delete_object_member_at(d, o, idx);
	else return DT_KEY_DOES_NOT_EXIST;
}

dt_status_t dt_remove_object_member_at(dt_datatree_t d, dt_object_t o, size_t where, dt_value_t key, dt_value_t val) {
	_dt_object_t* obj = (_dt_object_t*)o;
	dt_value_t k = NULL;
	dt_value_t v = NULL;
	dt_rbnode_t rbn = NULL;
	size_t i = 0;
	DT_ASSERT(d && o && key && val);

	if(obj->readonly)
		return DT_VALUE_IS_READONLY;

	dt_object_member_at(d, o, where, &k, &v);
	if(k && v) {
		rbn = dt_rbtree_exact_query(obj->mapped, k);
		DT_ASSERT(rbn);
		dt_rbtree_delete(obj->mapped, rbn);

		if(key != k) {
			_DT_DTOR(_dt_value_t, (_dt_value_t*)key);
		}
		dt_memcpy(key, k, sizeof(_dt_value_t));
		dt_memcpy(val, v, sizeof(_dt_value_t));
		_DT_MAKE_VALUE_READONLY((_dt_value_t*)key, 0);
		_DT_MAKE_VALUE_READONLY((_dt_value_t*)val, 0);
		_DT_CTOR(_dt_value_t, (_dt_value_t*)k, DT_NULL);
		_DT_CTOR(_dt_value_t, (_dt_value_t*)v, DT_NULL);
		_DT_DTOR(_dt_pair_t, &obj->members[where]);

		memmove(&obj->members[where], &obj->members[where + 1], sizeof(_dt_pair_t) * (obj->count - where - 1));
		obj->count--;
		for(i = where; i < obj->count; i++)
			obj->members[i].value->index = i;
	} else {
		return DT_INDEX_OUT_OF_RANGE;
	}

	return DT_OK;
}

dt_status_t dt_delete_object_member_at(dt_datatree_t d, dt_object_t o, size_t where) {
	dt_status_t result = DT_OK;
	_dt_value_t* key = NULL;
	_dt_value_t* val = NULL;
	DT_ASSERT(d && o);
	_DT_NEW(_dt_value_t, key, DT_NULL);
	_DT_NEW(_dt_value_t, val, DT_NULL);
	result = dt_remove_object_member_at(d, o, where, (dt_value_t)key, (dt_value_t)val);
	_DT_DEL(_dt_value_t, key);
	_DT_DEL(_dt_value_t, val);

	return result;
}

dt_status_t dt_clear_object_member(dt_datatree_t d, dt_object_t o) {
	_dt_object_t* obj = (_dt_object_t*)o;
	size_t i = 0;
	_dt_pair_t* pair = NULL;
	DT_ASSERT(d && o);

	if(obj->readonly)
		return DT_VALUE_IS_READONLY;

	for(i = 0; i < obj->count; i++) {
		pair = &obj->members[i];
		_DT_DTOR(_dt_pair_t, pair);
	}
	obj->count = 0;

	return DT_OK;
}

dt_status_t dt_remove_array_elem_at(dt_datatree_t d, dt_array_t a, size_t where, dt_value_t val) {
	_dt_array_t* arr = (_dt_array_t*)a;
	dt_value_t e = NULL;
	DT_ASSERT(d && a && val);

	if(arr->readonly)
		return DT_VALUE_IS_READONLY;

	dt_array_elem_at(d, a, where, &e);
	if(e) {
		dt_memcpy(val, e, sizeof(_dt_value_t));
		_DT_CTOR(_dt_value_t, (_dt_value_t*)e, DT_NULL);
		_DT_DTOR(_dt_value_t, (_dt_value_t*)e);

		memmove(&arr->elems[where], &arr->elems[where + 1], sizeof(_dt_value_t) * (arr->count - where - 1));
		arr->count--;

		_dt_resize_array(arr, arr->count);
	} else {
		return DT_INDEX_OUT_OF_RANGE;
	}

	return DT_OK;
}

dt_status_t dt_delete_array_elem_at(dt_datatree_t d, dt_array_t a, size_t where) {
	dt_status_t result = DT_OK;
	_dt_value_t* elem = NULL;
	DT_ASSERT(d && a);
	_DT_NEW(_dt_value_t, elem, DT_NULL);
	result = dt_remove_array_elem_at(d, a, where, (dt_value_t)elem);
	_DT_DEL(_dt_value_t, elem);

	return result;
}

dt_status_t dt_clear_array_elem(dt_datatree_t d, dt_array_t a) {
	_dt_array_t* arr = (_dt_array_t*)a;
	size_t i = 0;
	DT_ASSERT(d && a);

	if(arr->readonly)
		return DT_VALUE_IS_READONLY;

	for(i = 0; i < arr->count; i++) {
		_DT_DTOR(_dt_value_t, &arr->elems[i]);
	}
	arr->count = 0;
	dt_free((void**)&arr->elems);

	return DT_OK;
}

dt_status_t dt_update_object_member_by_key(dt_datatree_t d, dt_object_t o, const dt_value_t key, dt_value_t val) {
	dt_value_t v = NULL;
	size_t idx = 0;
	DT_ASSERT(d && o && key);
	dt_find_object_member_by_key(d, o, key, &v, &idx);
	if(v) return dt_update_object_member_at(d, o, idx, val);
	else return DT_KEY_DOES_NOT_EXIST;
}

dt_status_t dt_update_object_member_at(dt_datatree_t d, dt_object_t o, size_t where, dt_value_t val) {
	_dt_object_t* obj = (_dt_object_t*)o;
	dt_value_t k = NULL;
	dt_value_t v = NULL;
	DT_ASSERT(d && o && val);

	if(obj->readonly)
		return DT_VALUE_IS_READONLY;

	dt_object_member_at(d, o, where, &k, &v);
	if(k && v) {
		_DT_DTOR(_dt_value_t, &obj->members[where].value->obj);

		dt_memcpy(&obj->members[where].value->obj, val, sizeof(_dt_value_t));
	} else {
		return DT_INDEX_OUT_OF_RANGE;
	}

	return DT_OK;
}

dt_status_t dt_udpate_array_elem_at(dt_datatree_t d, dt_array_t a, size_t where, dt_value_t val) {
	dt_value_t e = NULL;
	DT_ASSERT(d && a && val);

	if(((_dt_array_t*)a)->readonly)
		return DT_VALUE_IS_READONLY;

	dt_array_elem_at(d, a, where, &e);
	if(e) {
		_DT_DTOR(_dt_value_t, (_dt_value_t*)e);

		dt_memcpy(e, val, sizeof(_dt_value_t));
	} else {
		return DT_INDEX_OUT_OF_RANGE;
	}

	return DT_OK;
}

void dt_foreach_object_member(dt_datatree_t d, dt_object_t o, dt_object_member_walker_t w) {
	_dt_object_t* obj = (_dt_object_t*)o;
	size_t i = 0;
	_dt_pair_t* pair = NULL;
	DT_ASSERT(d && o && w);
	for(i = 0; i < obj->count; i++) {
		pair = &obj->members[i];
		(*w)(d, o, (dt_value_t)pair->key, (dt_value_t)&pair->value->obj, pair->value->index);
	}
}

void dt_foreach_array_elem(dt_datatree_t d, dt_array_t a, dt_array_walker_t w) {
	_dt_array_t* arr = (_dt_array_t*)a;
	size_t i = 0;
	DT_ASSERT(d && a && w);
	for(i = 0; i < arr->count; i++)
		(*w)(d, a, (dt_value_t)&arr->elems[i], i);
}

dt_value_t dt_root_value(dt_datatree_t d) {
	_dt_datatree_detail_t* dt = (_dt_datatree_detail_t*)d;
	DT_ASSERT(d);

	return (dt_value_t)(&dt->root);
}

dt_object_t dt_root_as_object(dt_datatree_t d) {
	_dt_datatree_detail_t* dt = (_dt_datatree_detail_t*)d;
	DT_ASSERT(d);
	if(dt->root.type == DT_OBJECT)
		return (dt_object_t)dt->root.data._obj;

	return NULL;
}

dt_type_t dt_value_type(const dt_value_t v) {
	_dt_value_t* val = (_dt_value_t*)v;
	DT_ASSERT(v);

	return val->type;
}

void* dt_value_data(const dt_value_t v) {
	_dt_value_t* val = (_dt_value_t*)v;
	void* result = NULL;
	DT_ASSERT(v);
	switch(val->type) {
	case DT_NULL: /* do nothing */
		break;
	case DT_BOOL:
		result = &val->data._bool;
		break;
	case DT_BYTE:
		result = &val->data._byte;
		break;
	case DT_UBYTE:
		result = &val->data._ubyte;
		break;
	case DT_SHORT:
		result = &val->data._short;
		break;
	case DT_USHORT:
		result = &val->data._ushort;
		break;
	case DT_INT:
		result = &val->data._int;
		break;
	case DT_UINT:
		result = &val->data._uint;
		break;
	case DT_LONG:
		result = &val->data._long;
		break;
	case DT_ULONG:
		result = &val->data._ulong;
		break;
	case DT_SINGLE:
		result = &val->data._single;
		break;
	case DT_DOUBLE:
		result = &val->data._double;
		break;
	case DT_STRING:
		result = val->data._str->c_str;
		break;
	case DT_OBJECT:
		result = val->data._obj;
		break;
	case DT_ARRAY:
		result = &val->data._array;
		break;
	default:
		DT_ASSERT(0 && "Invalid type");
	}

	return result;
}

dt_status_t dt_value_data_as(void* des, const dt_value_t v, dt_type_t t) {
	dt_status_t result = DT_OK;
	_dt_value_t* val = (_dt_value_t*)v;
	DT_ASSERT(v);
	switch(val->type) {
	case DT_NULL:
		if(t != DT_NULL) { DT_ASSERT(0 && "Invalid type"); result = DT_TYPE_NOT_MATCHED; break; }
		break;
	case DT_BOOL:
		if(t != DT_BOOL) { DT_ASSERT(0 && "Invalid type"); result = DT_TYPE_NOT_MATCHED; break; }
		else *(dt_bool_t*)des = val->data._bool;
		break;
	case DT_BYTE:
		if(t < val->type || t > DT_DOUBLE) { DT_ASSERT(0 && "Invalid type"); result = DT_TYPE_NOT_MATCHED; break; }
		else { _DT_CONVERT(des, val, t, _byte); }
		break;
	case DT_UBYTE:
		if(t < val->type || t > DT_DOUBLE) { DT_ASSERT(0 && "Invalid type"); result = DT_TYPE_NOT_MATCHED; break; }
		else { _DT_CONVERT(des, val, t, _ubyte); }
		break;
	case DT_SHORT:
		if(t < val->type || t > DT_DOUBLE) { DT_ASSERT(0 && "Invalid type"); result = DT_TYPE_NOT_MATCHED; break; }
		else { _DT_CONVERT(des, val, t, _short); }
		break;
	case DT_USHORT:
		if(t < val->type || t > DT_DOUBLE) { DT_ASSERT(0 && "Invalid type"); result = DT_TYPE_NOT_MATCHED; break; }
		else { _DT_CONVERT(des, val, t, _ushort); }
		break;
	case DT_INT:
		if(t < val->type || t > DT_DOUBLE) { DT_ASSERT(0 && "Invalid type"); result = DT_TYPE_NOT_MATCHED; break; }
		else { _DT_CONVERT(des, val, t, _int); }
		break;
	case DT_UINT:
		if(t < val->type || t > DT_DOUBLE) { DT_ASSERT(0 && "Invalid type"); result = DT_TYPE_NOT_MATCHED; break; }
		else { _DT_CONVERT(des, val, t, _uint); }
		break;
	case DT_LONG:
		if(t < val->type || t > DT_DOUBLE) { DT_ASSERT(0 && "Invalid type"); result = DT_TYPE_NOT_MATCHED; break; }
		else { _DT_CONVERT(des, val, t, _long); }
		break;
	case DT_ULONG:
		if(t < val->type || t > DT_DOUBLE) { DT_ASSERT(0 && "Invalid type"); result = DT_TYPE_NOT_MATCHED; break; }
		else { _DT_CONVERT(des, val, t, _ulong); }
		break;
	case DT_SINGLE:
		if(t < val->type || t > DT_DOUBLE) { DT_ASSERT(0 && "Invalid type"); result = DT_TYPE_NOT_MATCHED; break; }
		else { _DT_CONVERT(des, val, t, _single); }
		break;
	case DT_DOUBLE:
		if(t < val->type || t > DT_DOUBLE) { DT_ASSERT(0 && "Invalid type"); result = DT_TYPE_NOT_MATCHED; break; }
		else { _DT_CONVERT(des, val, t, _double); }
		break;
	case DT_STRING:
		if(t != DT_STRING) { DT_ASSERT(0 && "Invalid type"); result = DT_TYPE_NOT_MATCHED; break; }
		*(char**)(des) = val->data._str->c_str;
		break;
	case DT_OBJECT:
		if(t != DT_OBJECT) { DT_ASSERT(0 && "Invalid type"); result = DT_TYPE_NOT_MATCHED; break; }
		*(_dt_object_t**)(des) = val->data._obj;
		break;
	case DT_ARRAY:
		if(t != DT_ARRAY) { DT_ASSERT(0 && "Invalid type"); result = DT_TYPE_NOT_MATCHED; break; }
		*(_dt_array_t**)(des) = &(val->data._array);
		break;
	default:
		DT_ASSERT(0 && "Invalid type");
		result = DT_TYPE_NOT_MATCHED; 
	}

	return result;
}

int dt_value_compare(const dt_value_t l, const dt_value_t r, dt_bool_t num_raw_cmp) {
	return _dt_value_cmp((const _dt_value_t*)l, (const _dt_value_t*)r, num_raw_cmp);
}

int dt_value_data_length(const dt_value_t v) {
	_dt_value_t* val = (_dt_value_t*)v;
	DT_ASSERT(v);

	if(!v) return -1;
	switch(val->type) {
	case DT_NULL:
		return 0;
	case DT_BOOL:
		return sizeof(val->data._bool);
	case DT_BYTE:
		return sizeof(val->data._byte);
	case DT_UBYTE:
		return sizeof(val->data._ubyte);
	case DT_SHORT:
		return sizeof(val->data._short);
	case DT_USHORT:
		return sizeof(val->data._ushort);
	case DT_INT:
		return sizeof(val->data._int);
	case DT_UINT:
		return sizeof(val->data._uint);
	case DT_LONG:
		return sizeof(val->data._long);
	case DT_ULONG:
		return sizeof(val->data._ulong);
	case DT_SINGLE:
		return sizeof(val->data._single);
	case DT_DOUBLE:
		return sizeof(val->data._double);
	case DT_STRING:
		return (int)(val->data._str->length);
	case DT_OBJECT:
		return (int)(val->data._obj->count);
	case DT_ARRAY:
		return (int)(val->data._array.count);
	default:
		DT_ASSERT(0 && "Invalid type");
		return -1;
	}
}

void dt_value_mem_swap(dt_value_t l, dt_value_t r) {
	_dt_value_t* lv = (_dt_value_t*)l;
	_dt_value_t* rv = (_dt_value_t*)r;
	_dt_value_t tmp;
	DT_ASSERT(l && r);
	tmp = *lv;
	*lv = *rv;
	*rv = tmp;
}

void dt_value_mem_move(dt_value_t l, dt_value_t r) {
	_dt_value_t* lv = (_dt_value_t*)l;
	_dt_value_t* rv = (_dt_value_t*)r;
	DT_ASSERT(l && r);
	*lv = *rv;
}

/* ========================================================} */

#ifdef __cplusplus
}
#endif /* __cplusplus */
