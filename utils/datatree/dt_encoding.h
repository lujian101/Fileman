#ifndef __DT_ENCODING_H__
#define __DT_ENCODING_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int dt_str_to_hex4(const char** buf, unsigned int* hex);

int dt_utf8_encoding(char** buf, unsigned int codepoint);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DT_ENCODING_H__ */
