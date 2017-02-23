#ifndef __XPLEXT_FILE_H__
#define __XPLEXT_FILE_H__

#include "xpl.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

enum xplext_status_t {
	XS_RETURN = XS_COUNT + 1,				/**< Return current call */
};

typedef struct xplext_source_seg_info_t {
  const char* name;
  const char* source;
} xplext_source_seg_info_t;

typedef struct xplext_source_t {
	xplext_source_seg_info_t* items;
	int count;
	char* text;
} xplext_source_t;

xplext_source_t* xplext_load_source(const char* file);
int              xplext_unload_source(xplext_source_t* s);
const char*      xplext_find_source(xplext_source_t* s, const char* key);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
