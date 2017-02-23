#ifndef __DT_CORE_H__
#define __DT_CORE_H__

#ifdef _MSC_VER
//#	define _CRTDBG_MAP_ALLOC
//#	include <crtdbg.h>
#endif /* _MSC_VER */
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <malloc.h>
#include <assert.h>
#include <math.h>

#ifdef _MSC_VER
#	ifdef INTE_COMPILE
#		define DT_API/* __declspec(dllexport)*/
#	else
#		define DT_API/* __declspec(dllimport)*/
#	endif
#else
#	define DT_API __attribute__ ((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Whether enable alloc statistics
 * @note Now only enabled under DEBUG mode, this information is very useful
 *       for memory occupation analyze and optimization during development
 *       period, but could be economized in release; you can modify here to
 *       change the strategy
 */
#if (defined _DEBUG && !defined NDEBUG)
#	ifndef DT_ENABLE_ALLOC_STAT
#		define DT_ENABLE_ALLOC_STAT
#	endif /* DT_ENABLE_ALLOC_STAT */
#endif /* (defined _DEBUG && !defined NDEBUG) */

/**
 * @brief Size of a pointer
 */
#ifndef DT_POINTER_SIZE
#	define DT_POINTER_SIZE (sizeof(void*))
#endif /* DT_POINTER_SIZE */

/**
 * @brief Run time assertion
 */
#ifndef DT_ASSERT
#	define DT_ASSERT(e) assert(e)
#endif /* DT_ASSERT */
/**
 * @brief Compile time assertion
 */
#ifndef DT_STATIC_ASSERT
#	define _STATIC_ASSERT_IMPL(cond, msg) typedef char static_assertion_##msg[(!!(cond)) * 2 - 1]
#	define _COMPILE_TIME_ASSERT3(x, l) _STATIC_ASSERT_IMPL(x, static_assertion_at_line_##l)
#	define _COMPILE_TIME_ASSERT2(x, l) _COMPILE_TIME_ASSERT3(x, l)
#	define DT_STATIC_ASSERT(x) _COMPILE_TIME_ASSERT2(x, __LINE__)
#endif /* DT_STATIC_ASSERT */

/**
 * @brief Get the count of elements in an array
 */
#ifndef DT_COUNT_OF
#	define DT_COUNT_OF(a) (sizeof(a) / sizeof((a)[0]))
#endif /* DT_COUNT_OF */

/**
 * @brief Gets the alignment of a structure
 */
#ifndef DT_ALIGN_OF
#	define DT_ALIGN_OF(s) (sizeof(struct { char _1; s _n; }) - sizeof(s))
#endif /* DT_ALIGN_OF */

/**
 * @brief Max string length for allocating, for common usage
 */
#ifndef DT_STR_LEN
#	define DT_STR_LEN 512
#endif /* DT_STR_LEN */

/**
 * @brief Target platform endian mode
 */
#ifndef DT_LITTLE_ENDIAN
#	define DT_LITTLE_ENDIAN
#endif /* DT_LITTLE_ENDIAN */

/**
 * @brief Allocates on stack
 * @note Considering some compilers do not support this functionality well,
 *       please redefine this with some other things like the original heap
 *       malloc, and at the same time, don't forget to redefine DT_DEALLOCA
 *       as well
 */
#ifndef DT_ALLOCA
#	define DT_ALLOCA(s) alloca(s)
#endif /* DT_ALLOCA */
/**
 * @brief Frees allocated memory on stack
 * @note Considering some compilers do not support allocation on stack well,
 *       please redefine DT_ALLOCA and DT_DEALLOCA
 */
#ifndef DT_DEALLOCA
#	define DT_DEALLOCA(p) (void)(p)
#endif /* DT_DEALLOCA */

/**
 * @brief Get the sign of a number
 */
#ifndef DT_SGN
#	define DT_SGN(a) ((a) == 0 ? 0 : ((a) > 0 ? 1 : -1))
#endif /* DT_SGN */

/**
 * @brief Invalid index
 */
#ifndef DT_INVALID_INDEX
#	define DT_INVALID_INDEX (~0)
#endif /* DT_INVALID_INDEX */

/**
 * @brief Dummy datatree, for passing to some APIs which requires a non-zero
 *        datatree pointer parameter
 */
#ifndef DT_DUMMY_DATATREE
#	define DT_DUMMY_DATATREE ((dt_datatree_t)~0)
#endif /* DT_DUMMY_DATATREE */

/**
 * @brief Boolean type
 */
typedef enum dt_bool_t {
	DT_FALSE, /**< false */
	DT_TRUE   /**< true */
} dt_bool_t;

/**
 * @brief Valid data types of a datatree value
 */
typedef enum dt_type_t {
	DT_NULL,      /**< null */
	DT_BOOL,      /**< true or false */
	DT_BYTE,      /**< Signed 8bits */
	DT_UBYTE,     /**< Unsigned 8bits */
	DT_SHORT,     /**< Signed short integer */
	DT_USHORT,    /**< Unsigned short integer */
	DT_INT,       /**< Signed integer */
	DT_UINT,      /**< Unsigned integer */
	DT_LONG,      /**< Signed long integer */
	DT_ULONG,     /**< Unsigned long integer */
	DT_SINGLE,    /**< Single float point */
	DT_DOUBLE,    /**< Double float point */
	DT_STRING,    /**< String */
	DT_OBJECT,    /**< Object */
	DT_ARRAY,     /**< Array */
	DT_TYPE_COUNT /**< Types count */
} dt_type_t;

/**
 * @brief Execution status of datatree operations
 */
typedef enum dt_status_t {
	DT_OK,                            /**< Totally okay, use this one to decide whether an operation executed succesfully */
	DT_LOAD_FAILED,                   /**< Datatree load failed from a string, file or binary */
	DT_REDUNDANCE_CHAR_LEFT,          /**< Datatree load failed from a string, redundance charactor(s) left */
	DT_LEFT_BRACE_EXPECTED,           /**< '{' symbol expected */
	DT_RIGHT_BRACE_EXPECTED,          /**< '}' symbol expected */
	DT_LEFT_SQUARE_BRACKET_EXPECTED,  /**< '[' symbol expected */
	DT_RIGHT_SQUARE_BRACKET_EXPECTED, /**< ']' symbol expected */
	DT_COLON_EXPECTED,                /**< ':' symbol expected */
	DT_TOO_MANY_COMMAS,               /**< Too many ',' symbols found */
	DT_VALUE_EXPECTED,                /**< Valid datatree value not found */
	DT_KEY_EXISTS,                    /**< Given key already exists in object members */
	DT_KEY_DOES_NOT_EXIST,            /**< Given key not exists in object members */
	DT_INDEX_OUT_OF_RANGE,            /**< An index of an object or array is out of range */
	DT_TYPE_NOT_MATCHED,              /**< Type not matched in a conversion */
	DT_VALUE_IS_READONLY              /**< Value is readonly, means you can not change the key of an object member */
} dt_status_t;

/**
 * @brief Enum compatible type, for generic arguments passing
 */
typedef unsigned int dt_enum_compatible_t;

/**
 * @brief Pointer to a datatree
 */
typedef struct dt_datatree_d { char* dummy; }* dt_datatree_t;
/**
 * @brief Pointer to a value
 */
typedef struct dt_value_d { char* dummy; }* dt_value_t;
/**
 * @brief Pointer to an object
 */
typedef struct dt_object_d { char* dummy; }* dt_object_t;
/**
 * @brief Pointer to an array
 */
typedef struct dt_array_d { char* dummy; }* dt_array_t;

/**
 * @brief SAD(Simple API for Datatree) event handler structure
 */
typedef struct dt_sad_handler_t {
	/**
	 * @brief Handles a simple typed value parsed event
	 *
	 * @param[in] type - Parsed data type
	 * @param[in] data - Pointer to parsed data
	 * @param[in] sad  - SAD context
	 */
	void (* simple_parsed)(dt_type_t type, void* data, struct dt_sad_handler_t* sad);
	/**
	 * @brief Handles an object begin event
	 *
	 * @param[in] sad - SAD context
	 */
	void (* object_begin)(struct dt_sad_handler_t* sad);
	/**
	 * @brief Handles an object end event
	 *
	 * @param[in] sad - SAD context
	 */
	void (* object_end)(struct dt_sad_handler_t* sad);
	/**
	 * @brief Handles an array begin event
	 *
	 * @param[in] sad - SAD context
	 */
	void (* array_begin)(struct dt_sad_handler_t* sad);
	/**
	 * @brief Handles an array end event
	 *
	 * @param[in] sad - SAD context
	 */
	void (* array_end)(struct dt_sad_handler_t* sad);
	/**
	 * @brief User data, assign whatever you want, normally for some parsing context
	 */
	void* userdata;
} dt_sad_handler_t;

/**
 * @brief Separator charactor detect functor
 *
 * @param[in] _c - Charactor to be detected
 * @return - Non zero if separator detected
 */
typedef int (* dt_separator_char_detect_func_t)(unsigned char _c);
/**
 * @brief Datatree string parsing error handler functor
 *
 * @param[in] status - Parsing status
 * @param[in] msg    - Error message text
 * @param[in] pos    - Error position in text format, for locating
 * @param[in] row    - Error row in source text, for locating
 * @param[in] col    - Error column in source text, for locating
 */
typedef void (* dt_parse_error_handler_t)(dt_enum_compatible_t status, const char* msg, const char* pos, size_t row, size_t col);
/**
 * @brief Datatree object members enumeration handler functor
 *
 * @param[in] d   - Datatree
 * @param[in] o   - Object
 * @param[in] key - Key of current enumerating member
 * @param[in] val - Value of current enumerating member
 * @param[in] idx - Index of current enumerating member
 */
typedef void (* dt_object_member_walker_t)(dt_datatree_t d, const dt_object_t o, const dt_value_t key, const dt_value_t val, size_t idx);
/**
 * @brief Datatree array elements enumeration handler functor
 *
 * @param[in] d   - Datatree
 * @param[in] a   - Array
 * @param[in] v   - Current enumerating element
 * @param[in] idx - Index of current enumerating element
 */
typedef void (* dt_array_walker_t)(dt_datatree_t d, const dt_array_t a, const dt_value_t v, size_t idx);

/**
 * @brief Gets the version of datatree
 *
 * @return - The version of datatree
 */
DT_API unsigned int dt_ver(void);
/**
 * @brief Gets the version of datatree binary format
 *
 * @return - The version of datatree binary format
 */
DT_API unsigned int dt_bver(void);

/**
 * @brief Gets a message text of a status
 *
 * @param[in] s - Status
 * @return - A message text
 */
DT_API const char* const dt_status_message(dt_status_t s);

/**
 * @brief Gets the allocated bytes for datatree sturctures,
 *        this function is only valid if DT_ENABLE_ALLOC_STAT is defined
 *
 * @return - Allocated bytes
 */
DT_API size_t dt_allocated(void);

/**
 * @brief Allocates a piece of memory in given size
 *
 * @param[in] s - Bytes to be allocated
 * @return - A pointer to allocated memory
 */
DT_API void* dt_malloc(size_t s);
/**
 * @brief Reallocates a piece of memory in a new size
 *
 * @param[out] p - Pointer to a pointer which points to a piece of memory to be reallocated
 * @param[in] s  - New size
 * @return - A pointer to reallocated memory
 */
DT_API void* dt_realloc(void** p, size_t s);
/**
 * @brief Frees a piece of allocated memory
 *
 * @param[in/out] p - Pointer to a pointer which points to a piece of memory to be freed
 */
DT_API void dt_free(void** p);
/**
 * @brief Fills a piece of memory with zero
 *
 * @param[in] p - Pointer to a piece of memory to be cleared
 * @param[in] s - Size to be cleared, in bytes
 */
DT_API void dt_memclr(void* p, size_t s);
/**
 * @brief Compares two pieces of memory with a given size
 *
 * @param[in] l - Left memory
 * @param[in] r - Right memory
 * @param[in] s - Size to be compared
 * @return - 0 if l == r, 1 if l > r, -1 if l < r
 */
DT_API int dt_memcmp(const void* l, const void* r, size_t s);
/**
 * @brief Copies a piece of memory to another with a given size
 *
 * @param[in] dst - Destination memory, would be filled
 * @param[in] src - Source memory
 * @param[in] s   - Size to be copied
 * @return - A pointer to copied destination memory
 */
DT_API void* dt_memcpy(void* dst, const void* src, size_t s);
/**
 * @brief Swaps two pieces of memory with a given size
 *
 * @param[in] l - Left memory
 * @param[in] r - Right memory
 * @param[in] s - Size to be swapped
 */
DT_API void dt_memswap(void* l, void* r, size_t s);

/**
 * @brief Creates a datatree
 *
 * @param[out] d - Pointer to created datatree
 * @param[in] eh - Parsing error handler
 */
DT_API void dt_create_datatree(dt_datatree_t* d, dt_parse_error_handler_t eh);
/**
 * @brief Unloads a datatree, a datatree needs to be unloaded before destroied
 *
 * @param[in] d - Datatree to be unloaded
 */
DT_API void dt_unload_datatree(dt_datatree_t d);
/**
 * @brief Destroies a datatree, a datatree needs to be unloaded before destroied
 *
 * @param[in] d - Datatree to be destroied
 */
DT_API void dt_destroy_datatree(dt_datatree_t d);

/**
 * @brief Gets the userdata of a datatree
 *
 * @param[in] d - Target datatree
 * @return - Set userdata
 */
DT_API void* dt_get_datatree_userdata(dt_datatree_t d);
/**
 * @brief Sets the userdata to a datatree
 *
 * @param[in] d  - Target datatree
 * @param[in] ud - Userdata
 * @return - Old userdata
 */
DT_API void* dt_set_datatree_userdata(dt_datatree_t d, void* ud);

/**
 * @brief Loads a text file to a datatree in SAD mode
 *
 * @param[in] d - Target datatree
 * @param[in] h - SAD event handler
 * @param[in] f - Source file path
 * @return - Parsing status
 */
DT_API dt_status_t dt_load_datatree_file_sad(dt_datatree_t d, dt_sad_handler_t* h, const char* f);
/**
 * @brief Loads a text file to a datatree
 *
 * @param[in] d - Target datatree
 * @param[in] f - Source file path
 * @return - Parsing status
 */
DT_API dt_status_t dt_load_datatree_file(dt_datatree_t d, const char* f);
/**
 * @brief Saves a datatree to a text file
 *
 * @param[in] d       - Source datatree
 * @param[in] f       - Target file path
 * @param[in] compact - Save as compact without spaces and newlines
 */
DT_API void dt_save_datatree_file(dt_datatree_t d, const char* f, dt_bool_t compact);
/**
 * @brief Loads a string to a datatree in SAD mode
 *
 * @param[in] d - Target datatree
 * @param[in] h - SAD event handler
 * @param[in] s - Source string
 * @return - Parsing status
 */
DT_API dt_status_t dt_load_datatree_string_sad(dt_datatree_t d, dt_sad_handler_t* h, const char* s);
/**
 * @brief Loads a string to a datatree
 *
 * @param[in] d - Target datatree
 * @param[in] s - Source string
 * @return - Parsing status
 */
DT_API dt_status_t dt_load_datatree_string(dt_datatree_t d, const char* s);
/**
 * @brief Saves a datatree to a string
 *
 * @param[in] d       - Source datatree
 * @param[out] s      - Target string
 * @param[in] compact - Save as compact without space and newlines
 */
DT_API void dt_save_datatree_string(dt_datatree_t d, char** s, dt_bool_t compact);
/**
 * @brief Loades a binary buffer to a datatree in SAD mode
 *
 * @param[in] d - Target datatree
 * @param[in] h - SAD event handler
 * @param[in] b - Source binary buffer
 * @return - Loading status
 */
DT_API dt_status_t dt_load_datatree_bin_sad(dt_datatree_t d, dt_sad_handler_t* h, const void* b);
/**
 * @brief Loades a binary buffer to a datatree
 *
 * @param[in] d - Target datatree
 * @param[in] b - Source binary buffer
 * @return - Loading status
 */
DT_API dt_status_t dt_load_datatree_bin(dt_datatree_t d, const void* b);
/**
 * @brief Saves a datatree to a binary buffer
 *
 * @param[in] d  - Source datatree
 * @param[out] b - Target binary buffer
 * @param[out] s - Bytes saved
 */
DT_API void dt_save_datatree_bin(dt_datatree_t d, void** b, size_t* s);

/**
 * @brief Creates a value
 * @usage dt_create_value(?, ?, on_error, "string allows %? style escape", more arguments for escape) or
 *        dt_create_value(?, ?, on_error, NULL, dt_type_t, data)
 *
 * @param[in] d   - Host datatree
 * @param[out] v  - Value holder pointer
 * @param[in] eh  - Optional, customized error handler
 * @param[in] fmt - Begining of a variable arguments list
 * @return - Non zero if succeed
 */
DT_API int dt_create_value(dt_datatree_t d, dt_value_t* v, dt_parse_error_handler_t eh, const char* fmt, ...);
/**
 * @brief Creates a value
 *
 * @param[in] d   - Host datatree
 * @param[out] v  - Value holder pointer
 * @param[in] sd  - Separator detector
 * @param[in] eh  - Optional, customized error handler
 * @param[in] fmt - String to be parsed
 * @return - Non zero if succeed
 */
DT_API int dt_create_value_ex(dt_datatree_t d, dt_value_t* v, dt_separator_char_detect_func_t sd, dt_parse_error_handler_t eh, const char* fmt);
/**
 * @brief Destroies a value
 *
 * @param[in] d - Host datatree
 * @param[in] v - Value to be destroied
 */
DT_API void dt_destroy_value(dt_datatree_t d, dt_value_t v);
/**
 * @brief Formats a value to string
 *
 * @param[in] d       - Host datatree
 * @param[in] v       - Value to be formatted
 * @param[out] s      - Target string
 * @param[in] compact - Compact mode
 */
DT_API void dt_format_value(dt_datatree_t d, const dt_value_t v, char** s, dt_bool_t compact);
/**
 * @brief Clones a value to another
 *
 * @param[in] d  - Host datatree
 * @param[in] v  - Input value
 * @param[out] o - Output value
 */
DT_API void dt_clone_value(dt_datatree_t d, const dt_value_t v, dt_value_t o);

/**
 * @brief Finds an object member by key
 *
 * @param[in] d    - Host datatree
 * @param[in] o    - Object to be searched
 * @param[in] key  - Searching key
 * @param[out] val - Found value
 * @param[out] idx - Index of found member
 */
DT_API void dt_find_object_member_by_key(dt_datatree_t d, const dt_object_t o, const dt_value_t key, dt_value_t* val, size_t* idx);
/**
 * @brief Gets the count of members in an object
 *
 * @param[in] d  - Host datatree
 * @param[in] o  - Object to be counted
 * @param[out] c - Count of members in an object
 */
DT_API void dt_object_member_count(dt_datatree_t d, const dt_object_t o, size_t* c);
/**
 * @brief Gets an object member
 *
 * @param[in] d    - Host datatree
 * @param[in] o    - Object to be operated
 * @param[out] key - Got key
 * @param[out] val - Got value
 */
DT_API void dt_object_member_at(dt_datatree_t d, const dt_object_t o, size_t idx, dt_value_t* key, dt_value_t* val);
/**
 * @brief Gets the count of elements in an array
 *
 * @param[in] d  - Host datatree
 * @param[in] a  - Array to be counted
 * @param[out] c - Count of elements in an array
 */
DT_API void dt_array_elem_count(dt_datatree_t d, const dt_array_t a, size_t* c);
/**
 * @brief Gets an array element
 *
 * @param[in] d   - Host datatree
 * @param[in] a   - Array to be operated
 * @param[in] idx - Target index
 * @param[out] v  - Got value
 */
DT_API void dt_array_elem_at(dt_datatree_t d, const dt_array_t a, size_t idx, dt_value_t* v);

/**
 * @brief Addes a new object member
 *
 * @param[in] d    - Host datatree
 * @param[in] o    - Target object
 * @param[in] key  - Given key, will be set to the null value after adding successfully
 * @param[in] val  - Given value, will be set to the null value after adding successfully
 * @param[out] idx - Appended position
 * @return - Operating status
 */
DT_API dt_status_t dt_add_object_member(dt_datatree_t d, dt_object_t o, dt_value_t key, dt_value_t val, size_t* idx);
/**
 * @brief Inserts a new object member
 *
 * @param[in] d     - Host datatree
 * @param[in] o     - Target object
 * @param[in] where - Insertion position
 * @param[in] key   - Given key, will be set to the null value after inserting successfully
 * @param[in] val   - Given value, will be set to the null value after inserting successfully
 * @return - Operating status
 */
DT_API dt_status_t dt_insert_object_member(dt_datatree_t d, dt_object_t o, size_t where, dt_value_t key, dt_value_t val);
/**
 * @brief Addes a new array element
 *
 * @param[in] d    - Host datatree
 * @param[in] a    - Target array
 * @param[in] v    - Given value, will be set to the null value after adding successfully
 * @param[out] idx - Appended position
 * @return - Operating status
 */
DT_API dt_status_t dt_add_array_elem(dt_datatree_t d, dt_array_t a, dt_value_t v, size_t* idx);
/**
 * @brief Inserts a new array element
 *
 * @param[in] d     - Host datatree
 * @param[in] a     - Target array
 * @param[in] where - Insertion position
 * @param[in] v     - Given value, will be set to the null value after inserting successfully
 * @return - Operating status
 */
DT_API dt_status_t dt_insert_array_elem(dt_datatree_t d, dt_array_t a, size_t where, dt_value_t v);

/**
 * @brief Removes an object member by key
 *
 * @param[in] d       - Host datatree
 * @param[in] o       - Object to be operated
 * @param[in/out] key - Given key, this will be filled with the removed key after function returned
 * @param[out] val    - Removed value
 * @return - Operating status
 */
DT_API dt_status_t dt_remove_object_member_by_key(dt_datatree_t d, dt_object_t o, dt_value_t key, dt_value_t val);
/**
 * @brief Deletes an object member by key
 *
 * @param[in] d   - Host datatree
 * @param[in] o   - Object to be operated
 * @param[in] key - Given key
 * @return - Operating status
 */
DT_API dt_status_t dt_delete_object_member_by_key(dt_datatree_t d, dt_object_t o, const dt_value_t key);
/**
 * @brief Removes an object member by index
 *
 * @param[in] d     - Host datatree
 * @param[in] o     - Object to be operated
 * @param[in] where - Given index
 * @param[out] key  - Removed key
 * @param[out] val  - Removed value
 * @return - Operating status
 */
DT_API dt_status_t dt_remove_object_member_at(dt_datatree_t d, dt_object_t o, size_t where, dt_value_t key, dt_value_t val);
/**
 * @brief Deletes an object member by index
 *
 * @param[in] d     - Host datatree
 * @param[in] o     - Object to be operated
 * @param[in] where - Given index
 * @return - Operating status
 */
DT_API dt_status_t dt_delete_object_member_at(dt_datatree_t d, dt_object_t o, size_t where);
/**
 * @brief Clears and deletes all members of an array
 *
 * @param[in] d - Host datatree
 * @param[in] o - Object to be cleared
 * @return - Operating status
 */
DT_API dt_status_t dt_clear_object_member(dt_datatree_t d, dt_object_t o);
/**
 * @brief Removes an array element by index
 *
 * @param[in] d     - Host datatree
 * @param[in] a     - Array to be operated
 * @param[in] where - Given index
 * @param[out] val  - Removed value
 * @return - Operating status
 */
DT_API dt_status_t dt_remove_array_elem_at(dt_datatree_t d, dt_array_t a, size_t where, dt_value_t val);
/**
 * @brief Deletes an array element by index
 *
 * @param[in] d     - Host datatree
 * @param[in] a     - Array to be operated
 * @param[in] where - Given index
 * @return - Operating status
 */
DT_API dt_status_t dt_delete_array_elem_at(dt_datatree_t d, dt_array_t a, size_t where);
/**
 * @brief Cleares and deletes all elements of an array
 *
 * @param[in] d - Host datatree
 * @param[in] a - Array to be cleared
 * @return - Operating status
 */
DT_API dt_status_t dt_clear_array_elem(dt_datatree_t d, dt_array_t a);

/**
 * @brief Updates an object member value, located by key
 *
 * @param[in] d   - Host datatree
 * @param[in] o   - Object to be updated
 * @param[in] key - Given key, where to be updated
 * @param[in] val - Given value
 * @return - Operating status
 */
DT_API dt_status_t dt_update_object_member_by_key(dt_datatree_t d, dt_object_t o, const dt_value_t key, dt_value_t val);
/**
 * @brief Updates an object member value, located by index
 *
 * @param[in] d     - Host datatree
 * @param[in] o     - Object to be updated
 * @param[in] where - Given index, where to be updated
 * @param[in] val   - Given value
 * @return - Operating status
 */
DT_API dt_status_t dt_update_object_member_at(dt_datatree_t d, dt_object_t o, size_t where, dt_value_t val);
/**
 * @brief Updates an array element
 *
 * @param[in] d     - Host datatree
 * @param[in] a     - Array to be updated
 * @param[in] where - Given index, where to be updated
 * @param[val] val  - Given value
 * @return - Operating status
 */
DT_API dt_status_t dt_udpate_array_elem_at(dt_datatree_t d, dt_array_t a, size_t where, dt_value_t val);

/**
 * @brief Enumerates all members in an object
 *
 * @param[in] d - Host datatree
 * @param[in] o - Object to be enumerated
 * @param[in] w - Enumeration handler
 */
DT_API void dt_foreach_object_member(dt_datatree_t d, dt_object_t o, dt_object_member_walker_t w);
/**
 * @brief Enumerates all elements in an array
 *
 * @param[in] d - Host datatree
 * @param[in] a - Array to be enumerated
 * @param[in] w - Enumeration handler
 */
DT_API void dt_foreach_array_elem(dt_datatree_t d, dt_array_t a, dt_array_walker_t w);

/**
 * @brief Gets the root value of a datatree, a root value can be an object or an array in datatree
 *
 * @param[in] d - Host datatree
 * @return - The root value
 */
DT_API dt_value_t dt_root_value(dt_datatree_t d);
/**
 * @brief Tries to get the root value of a datatree as object
 *
 * @param[in] d - Host datatree
 * @return - Root value as object, NULL if it's not an object (but an array)
 */
DT_API dt_object_t dt_root_as_object(dt_datatree_t d);
/**
 * @brief Gets the type of a value
 *
 * @param[in] v - Given value
 * @return - The type the given value
 */
DT_API dt_type_t dt_value_type(const dt_value_t v);
/**
 * @brief Gets a pointer to the data of a value
 *
 * @param[in] v - Given value
 * @return - A pointer to the data of the given value
 */
DT_API void* dt_value_data(const dt_value_t v);
/**
 * @brief Tries to get a data ov a value
 * @note Numeric data can be assigned to a variable of a higher precision,
 *       other types of data can only be assigned to an exact variable in the same type
 *
 * @param[out] des - Pointer to target variable
 * @param[in] v    - Given value
 * @param[in] t    - Expected type
 * @return - Conversion status, DT_OK if successeful, otherwise DT_TYPE_NOT_MATCHED
 */
DT_API dt_status_t dt_value_data_as(void* des, const dt_value_t v, dt_type_t t);
/**
 * @brief Compares two values
 *
 * @param[in] l           - Left value
 * @param[in] r           - Right value
 * @param[in] num_raw_cmp - Whether compares numbers using raw memory
 * @return - 0 if l == r, 1 if l > r, -1 if l < r
 */
DT_API int dt_value_compare(const dt_value_t l, const dt_value_t r, dt_bool_t num_raw_cmp);
/**
 * @brief Gets the length of a value
 *
 * @param[in] v - Value
 * @return - Members count for object, elements count for array,
 *           charactors (without ending '\0') count for string,
 *           raw size in bytes for other types, -1 for error
 */
DT_API int dt_value_data_length(const dt_value_t v);
/**
 * @brief Swaps the memory of two values
 * @note Do not call this function unless you know what you are doing
 *
 * @param[in] l - Left value
 * @param[in] r - Right value
 */
DT_API void dt_value_mem_swap(dt_value_t l, dt_value_t r);
/**
 * @brief Moves the memory of a value to another
 * @note Do not call this function unless you know what you are doing
 *
 * @param[in] l - Left value
 * @param[in] r - Right value
 */
DT_API void dt_value_mem_move(dt_value_t l, dt_value_t r);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DT_CORE_H__ */
