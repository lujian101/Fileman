#ifndef __DT_QUERY_H__
#define __DT_QUERY_H__

#include "dt_core.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Max string length for allocating, for command usage
 */
#ifndef DT_CMD_STR_LEN
#	define DT_CMD_STR_LEN 1024
#endif /* DT_STR_LEN */

/**
 * @brief Execution status of datatree query operations
 */
typedef enum dt_query_status_t {
	DTQ_OK,                            /**< Totally okay */
	DTQ_GOT_REF,                       /**< Reference value got */
	DTQ_GOT_NOREF,                     /**< No-reference value got */
	DTQ_GOT_NOTHING,                   /**< Nothing got */
	DTQ_RIGHT_PARENTHESES_EXPECTED,    /**< ')' symbol expected */
	DTQ_RIGHT_ANGLE_BRACKET_EXPECTED,  /**< '>' symbol expected */
	DTQ_EQUAL_SIGN_EXPECTED,           /**< '=' symbol expected */
	DTQ_ARRAY_EXPECTED,                /**< Datatree array expected */
	DTQ_ARRAY_OR_OBJECT_EXPECTED,      /**< Datatree array or array expected */
	DTQ_PATH_EXPECTED,                 /**< Path expected */
	DTQ_CTOR_EXPECTED,                 /**< Constructor expected */
	DTQ_EXPRESSION_EXPECTED            /**< Expression expected */
} dt_query_status_t;

/**
 * @brief Pointer to a command
 */
typedef struct dt_command_d { char* dummy; }* dt_command_t;

/**
 * @brief Gets the version of datatree query
 *
 * @return - The version of datatree query
 */
DT_API unsigned int dt_qver(void);

/**
 * @brief Gets a message text of a query status
 *
 * @param[in] s - Query status
 * @return - A message text
 */
DT_API const char* const dt_query_status_message(dt_query_status_t s);

/**
 * @brief Creates a query command
 *
 * @param[out] c - Created command
 */
DT_API void dt_create_command(dt_command_t* c);
/**
 * @brief Destroies a query command
 *
 * @param[in] c - Command to be destroied
 */
DT_API void dt_destroy_command(dt_command_t c);

/**
 * @brief Parses a query command
 *
 * @param[in] c   - Command
 * @param[in] eh  - Parsing error handler
 * @param[in] fmt - Begining of a variable arguments list
 * @return - Non zero if succeed
 */
DT_API int dt_parse_command(dt_command_t c, dt_parse_error_handler_t eh, const char* fmt, ...);

/**
 * @brief Clear all statements in a command
 *
 * @param[in] c - Command to be cleared
 */
DT_API void dt_clear_command(dt_command_t c);

/**
 * @brief Executes a query
 *
 * @param[in] t    - Target value to be queried
 * @param[in] c    - Command to be executed
 * @param[out] ret - Query result
 * @return - Execution status
 */
DT_API dt_query_status_t dt_query(dt_value_t t, dt_command_t c, dt_value_t* ret);

/**
 * @brief Gets the last queried result value of a command
 *
 * @param[in] c - Target command
 * @return - Last queried result value
 */
DT_API dt_value_t dt_last_queried(dt_command_t c);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DT_QUERY_H__ */
