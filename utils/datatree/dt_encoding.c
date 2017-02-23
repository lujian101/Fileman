#include "dt_core.h"
#include "dt_encoding.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int dt_str_to_hex4(const char** buf, unsigned int* hex) {
	unsigned int code = 0;
	int i = 0;
	for(i = 0; i < 4; i++) {
		unsigned char c = (*buf)[i];
		if(c >= '0' && c <= '9') c -= '0';
		else if(c >= 'A' && c <= 'F') { c += 10; c -= 'A'; }
		else if(c >= 'a' && c <= 'f') { c += 10; c -= 'a'; }
		else return -1;
		code <<= 4;
		code += c;
	}
	*hex = code;
	(*buf) += 4;

	return 4;
}

int dt_utf8_encoding(char** buf, unsigned int codepoint) {
	DT_ASSERT(buf && *buf);
	if(codepoint <= 0x7f) {
		*(*buf)++ = codepoint & 0xff;
		return 1;
	} else if(codepoint <= 0x7ff) {
		*(*buf)++ = 0xc0 | ((codepoint >> 6) & 0xff);
		*(*buf)++ = 0x80 | ((codepoint & 0x3f));
		return 2;
	} else if(codepoint <= 0xffff) {
		*(*buf)++ = 0xe0 | ((codepoint >> 12) & 0xff);
		*(*buf)++ = 0x80 | ((codepoint >> 6) & 0x3f);
		*(*buf)++ = 0x80 | (codepoint & 0x3f);
		return 3;
	} else {
		DT_ASSERT(codepoint <= 0x10ffff);
		*(*buf)++ = 0xf0 | ((codepoint >> 18) & 0xff);
		*(*buf)++ = 0x80 | ((codepoint >> 12) & 0x3f);
		*(*buf)++ = 0x80 | ((codepoint >> 6) & 0x3f);
		*(*buf)++ = 0x80 | (codepoint & 0x3f);
		return 4;
	}
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
