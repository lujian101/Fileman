#include "xpl/xpl.h"
#include "dt_rbtree.h"
#include "dt_query.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef _MSC_VER
#	pragma warning(disable : 4702)
#	pragma warning(disable : 4996)
#endif /* _MSC_VER */

/*
** {========================================================
** Macros
*/

#ifndef DTQVER
#	define DTQVER_MAJOR	0
#	define DTQVER_MINOR	9
#	define DTQVER_PATCH	5
#	define DTQVER ((DTQVER_MAJOR * 0x01000000) + (DTQVER_MINOR * 0x00010000) + (DTQVER_PATCH))
#endif /* DTQVER */

#ifndef _DT_IS_PATH_PREFIX
#	define _DT_IS_PATH_PREFIX(xpl) (*(xpl)->cursor != '\0' && (*(xpl)->cursor == '.' || *(xpl)->cursor == '<'))
#endif /* _DT_IS_PATH_PREFIX */
#ifndef _DT_IS_FUNC
#	define _DT_IS_FUNC(xpl) (*(xpl)->cursor != '\0' && xpl_peek_func((xpl), NULL) == XS_OK)
#endif /* _DT_IS_FUNC */
#ifndef _DT_IS_NOT_FUNC
#	define _DT_IS_NOT_FUNC(xpl) (*(xpl)->cursor != '\0' && xpl_peek_func((xpl), NULL) != XS_OK)
#endif /* _DT_IS_NOT_FUNC */

#ifndef _DT_GOT_VAL
#	define _DT_GOT_VAL(m) ((m) == DTQ_GOT_REF || (m) == DTQ_GOT_NOREF)
#endif /* _DT_GOT_VAL */

#ifndef _DT_FROM_CONTEXT_GUARD
#	define _DT_FROM_CONTEXT_GUARD
#	define _DT_PUSH_FROM_CONTEXT(cmd, stat, val) \
	struct _dt_value_t* __old = (cmd)->target; \
	size_t* __old_stat = (cmd)->statement_cursor; \
	size_t __old_stat_cursor = stat; \
	(cmd)->target = (struct _dt_value_t*)(val); \
	(cmd)->statement_cursor = &(stat);
#	define _DT_POP_FROM_CONTEXT(cmd, stat) \
	(stat) = __old_stat_cursor; \
	(cmd)->statement_cursor = __old_stat; \
	(cmd)->target = __old;
#endif /* _DT_FROM_CONTEXT_GUARD */

#ifndef _DT_ARRAY_ELEM_PATH
#	define _DT_ARRAY_ELEM_PATH(cmd, arr, path, idx, val) \
	do { \
		struct _dt_value_t* __tmp = (cmd)->target; \
		dt_array_elem_at(DT_DUMMY_DATATREE, (arr), (idx), &(val)); \
		(cmd)->target = (struct _dt_value_t*)val; \
		_dt_access_by_path(DT_DUMMY_DATATREE, (struct _dt_value_t*)(val), (path), (struct _dt_value_t**)&(val)); \
		(cmd)->target = __tmp; \
	} while(0)
#endif /* _DT_ARRAY_ELEM_PATH */

#ifndef _DT_FREE_EXPR_BOOL
#	define _DT_FREE_EXPR_BOOL(b) \
	do { \
		if((b)->type == DT_EXPR_NODE_BOOL) \
			dt_free((void**)&(b)); \
	} while(0)
#endif /* _DT_FREE_EXPR_BOOL */
#ifndef _DT_POP_FREE_EXPR_BOOL
#	define _DT_POP_FREE_EXPR_BOOL(s) \
	do { \
		_dt_expr_node_t* __n = _dt_expr_pop(&(s)); \
		while(__n) { \
			_DT_FREE_EXPR_BOOL(__n); \
			__n = _dt_expr_pop(&(s)); \
		} \
	} while(0)
#endif /* _DT_POP_FREE_EXPR_BOOL */

/* ========================================================} */

/*
** {========================================================
** Data type declarations and consts
*/

struct _dt_value_t;
struct _dt_command_t;

typedef enum _dt_type_t {
	DT_INTEGER = DT_TYPE_COUNT + 1,
	DT_FLOAT,
	DT_NUMBER
} _dt_type_t;

typedef enum _dt_expr_node_type_t {
	DT_EXPR_NODE_EVAL,
	DT_EXPR_NODE_LEFT_PARENTHESES,
	DT_EXPR_NODE_RIGHT_PARENTHESES,
	DT_EXPR_NODE_LESS,
	DT_EXPR_NODE_GREATER,
	DT_EXPR_NODE_LESS_EQUAL,
	DT_EXPR_NODE_GREATER_EQUAL,
	DT_EXPR_NODE_EQUAL,
	DT_EXPR_NODE_NOT_EQUAL,
	DT_EXPR_NODE_AND,
	DT_EXPR_NODE_OR,
	DT_EXPR_NODE_IS,
	DT_EXPR_NODE_HAS,
	DT_EXPR_NODE_PATH,
	DT_EXPR_NODE_VALUE,
	DT_EXPR_NODE_BOOL
} _dt_expr_node_type_t;

typedef enum _dt_path_node_type_t {
	DT_PATH_NODE_OBJECT_KEY,
	DT_PATH_NODE_INDEX,
	DT_PATH_NODE_BUILDIN_THIS,
	DT_PATH_NODE_BUILDIN_LEN
} _dt_path_node_type_t;

typedef enum _dt_statement_type_t {
	DT_STATEMENT_COMD,
	DT_STATEMENT_EXPR,
	DT_STATEMENT_PATH,
	DT_STATEMENT_CTOR,
	DT_STATEMENT_CURR
} _dt_statement_type_t;

typedef struct _dt_expr_node_t {
	_dt_expr_node_type_t type;
	union {
		struct _dt_path_node_t* path;
		struct _dt_value_t* value;
		int bool_value;
	} data;
	struct _dt_expr_node_t* next;
	struct _dt_expr_node_t* helper;
} _dt_expr_node_t;

typedef struct _dt_path_node_t {
	_dt_path_node_type_t type;
	union {
		dt_value_t key;
		size_t index;
	} value;
	struct _dt_path_node_t* next;
} _dt_path_node_t;

typedef struct _dt_ctor_t {
	dt_value_t* key;
	_dt_path_node_t** value_path;
	size_t count;
} _dt_ctor_t;

typedef struct _dt_statement_t {
	xpl_func_t opcode;
	struct {
		_dt_statement_type_t type;
		union {
			struct _dt_command_t* cmd;
			struct _dt_expr_node_t* expr;
			struct _dt_path_node_t* path;
			struct _dt_ctor_t* ctor;
		} data;
	} oprand;
} _dt_statement_t;

typedef struct _dt_command_t {
	xpl_context_t xpl;
	struct _dt_statement_t* statements;
	size_t statement_count;
	size_t* statement_cursor;
	dt_parse_error_handler_t error_handler;
	int parsed;
	struct _dt_value_t* target;
	struct _dt_value_t* got;
	dt_query_status_t result_mode;
	int expr_eval_result;
	dt_rbtree_t variable_table;
} _dt_command_t;

static const char* const _DTQ_STATUS_MSG[] = {
	"No error",
	"Reference value got",
	"No-reference value got",
	"Nothing got",
	"Right parentheses expected",
	"Right angle bracket expected",
	"Equal sign expected",
	"Datatree array expected",
	"Datatree array or object expected",
	"Path expected",
	"Constructor expected",
	"Expression expected"
};

static const char _DT_EXPR_OPCODE_PRECEDE[][13] = {
	/* =    (    )    <    >    <=   >=   ==   <>   AND  OR   IS   HAS */
	{ '=', '<', '>', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<' }, /* = */
	{ ' ', '<', '=', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<' }, /* ( */
	{ '>', ' ', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>' }, /* ) */
	{ '>', '<', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>' }, /* < */
	{ '>', '<', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>' }, /* > */
	{ '>', '<', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>' }, /* <= */
	{ '>', '<', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>' }, /* >= */
	{ '>', '<', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>' }, /* == */
	{ '>', '<', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>' }, /* <> */
	{ '>', '<', '>', '<', '<', '<', '<', '<', '<', '>', '>', '>', '>' }, /* AND */
	{ '>', '<', '>', '<', '<', '<', '<', '<', '<', '>', '>', '>', '>' }, /* OR */
	{ '>', '<', '>', '<', '<', '<', '<', '<', '<', '>', '>', '>', '>' }, /* IS */
	{ '>', '<', '>', '<', '<', '<', '<', '<', '<', '>', '>', '>', '>' }  /* HAS */
};

/* ========================================================} */

/*
** {========================================================
** Protected function externs
*/

extern int _dt_is_separator_char(unsigned char _c);

extern xpl_status_t _dt_pop_xpl_string(xpl_context_t* _s, char** _o);

extern void _dt_value_parse(const char* s, dt_sad_handler_t* h);

extern void _dt_value_assign(struct _dt_value_t* v, const char* s);

extern void _dt_on_error(const xpl_context_t* xpl, const char* const* msg, dt_enum_compatible_t status, dt_parse_error_handler_t error_handler);

/* ========================================================} */

/*
** {========================================================
** Private function declarations
*/

static unsigned int _dt_string_to_type(const char* str);
static int _dt_value_is(struct _dt_value_t* val, unsigned int type);

static int _dt_is_command_separator_char(unsigned char _c);

static int _dt_var_name_cmp_func(const void* l, const void* r);
static void _dt_var_name_dest_func(void* p);
static void _dt_var_name_info_dest_func(void* p);
static void _dt_var_name_print_func(const void* p);
static void _dt_value_print_info_func(void* p);

static xpl_status_t _dt_query_create(xpl_context_t* _s);
static xpl_status_t _dt_query_from(xpl_context_t* _s);
static xpl_status_t _dt_query_where(xpl_context_t* _s);
static xpl_status_t _dt_query_select(xpl_context_t* _s);
static xpl_status_t _dt_query_orderby(xpl_context_t* _s);
static xpl_status_t _dt_query_insert(xpl_context_t* _s);
static xpl_status_t _dt_query_delete(xpl_context_t* _s);
static xpl_status_t _dt_query_update(xpl_context_t* _s);
static xpl_status_t _dt_query_value(xpl_context_t* _s);
static xpl_status_t _dt_query_open(xpl_context_t* _s);
static xpl_status_t _dt_query_close(xpl_context_t* _s);
static xpl_status_t _dt_query_sync(xpl_context_t* _s);
static xpl_status_t _dt_query_push(xpl_context_t* _s);
static xpl_status_t _dt_query_pop(xpl_context_t* _s);
static xpl_status_t _dt_query_peek(xpl_context_t* _s);

static void _dt_sort_array(_dt_command_t* cmd, _dt_path_node_t* path, dt_array_t arr, int m, int n);

static _dt_statement_t* _dt_resize_statements(_dt_statement_t** p, size_t s);
static void _dt_clear_statements(_dt_statement_t* p, size_t s);
static _dt_statement_t* _dt_append_statement(_dt_command_t* cmd);

static int _dt_expr_is_op(_dt_expr_node_t* e);
static void _dt_expr_push(_dt_expr_node_t** s, _dt_expr_node_t* e);
static _dt_expr_node_t* _dt_expr_pop(_dt_expr_node_t** s);
static _dt_expr_node_t* _dt_expr_top(_dt_expr_node_t** s);
static _dt_expr_node_t* _dt_expr_operate(dt_datatree_t d, struct _dt_value_t* target, _dt_expr_node_t* a, _dt_expr_node_t* theta, _dt_expr_node_t* b);
static dt_query_status_t _dt_create_expr(_dt_expr_node_t** e, dt_parse_error_handler_t eh, xpl_context_t* _s);
static void _dt_destroy_expr(_dt_expr_node_t* e);
static dt_query_status_t _dt_eval_expr(dt_datatree_t d, struct _dt_value_t* target, _dt_expr_node_t* e, int* ret);

static dt_query_status_t _dt_create_path_node(_dt_path_node_t**p, dt_parse_error_handler_t eh, xpl_context_t* _s);
static void _dt_destroy_path_node(_dt_path_node_t* p);
static dt_query_status_t _dt_create_path(_dt_path_node_t** p, dt_parse_error_handler_t eh, xpl_context_t* _s);
static void _dt_destroy_path(_dt_path_node_t* p);
static dt_query_status_t _dt_access_by_path(dt_datatree_t d, struct _dt_value_t* target, _dt_path_node_t* path, struct _dt_value_t** ret);

static dt_query_status_t _dt_create_ctor(_dt_ctor_t** c, dt_parse_error_handler_t eh, xpl_context_t* _s);
static void _dt_destroy_ctor(_dt_ctor_t* c);
static dt_query_status_t _dt_make_new_object(dt_datatree_t d, dt_parse_error_handler_t eh, struct _dt_value_t* target, _dt_ctor_t* ctor, struct _dt_value_t** ret);

XPL_FUNC_BEGIN_EMPTY(_dt_query_statements)
	XPL_FUNC_ADD("create", _dt_query_create)
	XPL_FUNC_ADD("from", _dt_query_from)
	XPL_FUNC_ADD("where", _dt_query_where)
	XPL_FUNC_ADD("select", _dt_query_select)
	XPL_FUNC_ADD("orderby", _dt_query_orderby)
	XPL_FUNC_ADD("insert", _dt_query_insert)
	XPL_FUNC_ADD("delete", _dt_query_delete)
	XPL_FUNC_ADD("update", _dt_query_update)
	XPL_FUNC_ADD("value", _dt_query_value)
	XPL_FUNC_ADD("open", _dt_query_open)
	XPL_FUNC_ADD("close", _dt_query_close)
	XPL_FUNC_ADD("sync", _dt_query_sync)
	XPL_FUNC_ADD("push", _dt_query_push)
	XPL_FUNC_ADD("pop", _dt_query_pop)
	XPL_FUNC_ADD("peek", _dt_query_peek)
	XPL_FUNC_ADD("CREATE", _dt_query_create)
	XPL_FUNC_ADD("FROM", _dt_query_from)
	XPL_FUNC_ADD("WHERE", _dt_query_where)
	XPL_FUNC_ADD("SELECT", _dt_query_select)
	XPL_FUNC_ADD("ORDERBY", _dt_query_orderby)
	XPL_FUNC_ADD("INSERT", _dt_query_insert)
	XPL_FUNC_ADD("DELETE", _dt_query_delete)
	XPL_FUNC_ADD("UPDATE", _dt_query_update)
	XPL_FUNC_ADD("VALUE", _dt_query_value)
	XPL_FUNC_ADD("OPEN", _dt_query_open)
	XPL_FUNC_ADD("CLOSE", _dt_query_close)
	XPL_FUNC_ADD("SYNC", _dt_query_sync)
	XPL_FUNC_ADD("PUSH", _dt_query_push)
	XPL_FUNC_ADD("POP", _dt_query_pop)
	XPL_FUNC_ADD("PEEK", _dt_query_peek)
XPL_FUNC_END

/* ========================================================} */

/*
** {========================================================
** Private function definitions
*/

unsigned int _dt_string_to_type(const char* str) {
	unsigned int result = (unsigned int)(~0);
	DT_ASSERT(str);

	if(_xpl_strcmp(str, "null_t") == 0 || _xpl_strcmp(str, "NULL_T") == 0) result = DT_NULL;
	else if(_xpl_strcmp(str, "bool_t") == 0 || _xpl_strcmp(str, "BOOL_T") == 0) result = DT_BOOL;
	else if(_xpl_strcmp(str, "byte_t") == 0 || _xpl_strcmp(str, "BYTE_T") == 0) result = DT_BYTE;
	else if(_xpl_strcmp(str, "ubyte_t") == 0 || _xpl_strcmp(str, "UBYTE_T") == 0) result = DT_UBYTE;
	else if(_xpl_strcmp(str, "short_t") == 0 || _xpl_strcmp(str, "SHORT_T") == 0) result = DT_SHORT;
	else if(_xpl_strcmp(str, "ushort_t") == 0 || _xpl_strcmp(str, "USHORT_T") == 0) result = DT_USHORT;
	else if(_xpl_strcmp(str, "int_t") == 0 || _xpl_strcmp(str, "INT_T") == 0) result = DT_INT;
	else if(_xpl_strcmp(str, "uint_t") == 0 || _xpl_strcmp(str, "UINT_T") == 0) result = DT_UINT;
	else if(_xpl_strcmp(str, "long_t") == 0 || _xpl_strcmp(str, "LONG_T") == 0) result = DT_LONG;
	else if(_xpl_strcmp(str, "ulong_t") == 0 || _xpl_strcmp(str, "ULONG_T") == 0) result = DT_ULONG;
	else if(_xpl_strcmp(str, "single_t") == 0 || _xpl_strcmp(str, "SINGLE_T") == 0) result = DT_SINGLE;
	else if(_xpl_strcmp(str, "double_t") == 0 || _xpl_strcmp(str, "DOUBLE_T") == 0) result = DT_DOUBLE;
	else if(_xpl_strcmp(str, "string_t") == 0 || _xpl_strcmp(str, "STRING_T") == 0) result = DT_STRING;
	else if(_xpl_strcmp(str, "object_t") == 0 || _xpl_strcmp(str, "OBJECT_T") == 0) result = DT_OBJECT;
	else if(_xpl_strcmp(str, "array_t") == 0 || _xpl_strcmp(str, "ARRAY_T") == 0) result = DT_ARRAY;
	else if(_xpl_strcmp(str, "integer_t") == 0 || _xpl_strcmp(str, "INTEGER_T") == 0) result = DT_INTEGER;
	else if(_xpl_strcmp(str, "float_t") == 0 || _xpl_strcmp(str, "FLOAT_T") == 0) result = DT_FLOAT;
	else if(_xpl_strcmp(str, "number_t") == 0 || _xpl_strcmp(str, "NUMBER_T") == 0) result = DT_NUMBER;
	else { DT_ASSERT(0 && "Invalid type string"); }

	return result;
}

int _dt_value_is(struct _dt_value_t* val, unsigned int type) {
	int result = 0;
	unsigned int t = (unsigned int)(~0);
	DT_ASSERT(val);

	t = dt_value_type((dt_value_t)val);
	switch(type) {
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
	case DT_DOUBLE: /* fall through */
	case DT_STRING: /* fall through */
	case DT_OBJECT: /* fall through */
	case DT_ARRAY:
		result = (t == type);
		break;
	case DT_INTEGER:
		result = (t >= DT_BYTE && t <= DT_ULONG);
		break;
	case DT_FLOAT:
		result = (t == DT_SINGLE || t == DT_DOUBLE);
		break;
	case DT_NUMBER:
		result = (t >= DT_BYTE && t <= DT_DOUBLE);
		break;
	default:
		DT_ASSERT(0 && "Invalid type");
	}

	return result;
}

int _dt_is_command_separator_char(unsigned char _c) {
	return _dt_is_separator_char(_c) ||
		_c == '(' || _c == ')' ||
		_c == '<' || _c == '>' ||
		_c == '=' ||
		_c == '.' || _c == ',';
}

int _dt_var_name_cmp_func(const void* l, const void* r) {
	int result = 0;
	char* lv = (char*)l;
	char* rv = (char*)r;
	DT_ASSERT(l && r);
	result = strcmp(lv, rv);

	return DT_SGN(result);
}

void _dt_var_name_dest_func(void* p) {
	DT_ASSERT(p);
}

void _dt_var_name_info_dest_func(void* p) {
	DT_ASSERT(p);
}

void _dt_var_name_print_func(const void* p) {
	DT_ASSERT(p);
}

void _dt_value_print_info_func(void* p) {
	DT_ASSERT(p);
}

xpl_status_t _dt_query_create(xpl_context_t* _s) {
	_dt_command_t* cmd = NULL;
	xpl_assert(_s && _s->userdata);
	cmd = (_dt_command_t*)_s->userdata;
	if(cmd->parsed) {
	} else {
	}

	return XS_OK;
}

xpl_status_t _dt_query_from(xpl_context_t* _s) {
	_dt_command_t* cmd = NULL;
	_dt_statement_t* stat = NULL;
	xpl_assert(_s && _s->userdata);
	cmd = (_dt_command_t*)_s->userdata;
	if(cmd->parsed) {
		size_t where_stat = (size_t)(~0);
		size_t select_stat = (size_t)(~0);
		size_t orderby_stat = (size_t)(~0);
		dt_array_t array = NULL;
		dt_status_t s = DT_OK;
		size_t i = 0;
		size_t cnt = 0;
		dt_value_t val = NULL;
		dt_query_status_t status = DTQ_OK;
		int cond = 1;
		dt_value_t result_object = NULL;
		dt_array_t result_array = NULL;
		dt_value_t middle = NULL;

		(*cmd->statement_cursor)++;

		for(i = *cmd->statement_cursor; i < cmd->statement_count; i++) {
			stat = &cmd->statements[i];
			if(stat->opcode == _dt_query_where) where_stat = i;
			else if(stat->opcode == _dt_query_select) select_stat = i;
			else if(stat->opcode == _dt_query_orderby) orderby_stat = i;
			else { /* TODO */ }
		}

		stat = &cmd->statements[*cmd->statement_cursor - 1];
		if(stat->oprand.type == DT_STATEMENT_PATH) {
			status = _dt_access_by_path(DT_DUMMY_DATATREE, cmd->target, stat->oprand.data.path, &cmd->target);
		} else if(stat->oprand.type == DT_STATEMENT_COMD) {
			dt_query((dt_value_t)cmd->target, (dt_command_t)stat->oprand.data.cmd, &middle);
			cmd->target = (struct _dt_value_t*)middle;
		} else {
			DT_ASSERT(0 && "Invalid oprand type");
		}

		if(!cmd->target) { 
			*cmd->statement_cursor = cmd->statement_count;
			cmd->result_mode = DTQ_ARRAY_EXPECTED;
			return XS_OK;
		}
		s = dt_value_data_as(&array, (dt_value_t)cmd->target, DT_ARRAY);
		if(s != DT_OK) {
			*cmd->statement_cursor = cmd->statement_count;
			cmd->result_mode = DTQ_ARRAY_EXPECTED;
			return XS_OK;
		}

		dt_create_value(DT_DUMMY_DATATREE, &result_object, cmd->error_handler, "[]");
		dt_value_data_as(&result_array, result_object, DT_ARRAY);

		dt_array_elem_count(DT_DUMMY_DATATREE, array, &cnt);
		for(i = 0; i < cnt; i++) {
			dt_array_elem_at(DT_DUMMY_DATATREE, array, i, &val);

			cond = 1;
			if(where_stat != (~0)) {
				_DT_PUSH_FROM_CONTEXT(cmd, where_stat, val); {
					cmd->expr_eval_result = 1;
					cmd->statements[where_stat].opcode(_s);
					cond = !!cmd->expr_eval_result;
				}
				_DT_POP_FROM_CONTEXT(cmd, where_stat);
			}
			if(!cond) continue;

			do {
				_DT_PUSH_FROM_CONTEXT(cmd, select_stat, val); {
					cmd->statements[select_stat].opcode(_s);
					if(_DT_GOT_VAL(cmd->result_mode)) {
						if(cmd->result_mode == DTQ_GOT_NOREF) {
							dt_add_array_elem(DT_DUMMY_DATATREE, result_array, (dt_value_t)cmd->got, NULL);
							dt_destroy_value(DT_DUMMY_DATATREE, (dt_value_t)cmd->got);
						} else if(cmd->result_mode == DTQ_GOT_REF) {
							dt_value_t _got_ref = NULL;
							dt_create_value(DT_DUMMY_DATATREE, &_got_ref, cmd->error_handler, NULL, DT_NULL);
							dt_clone_value(DT_DUMMY_DATATREE, (dt_value_t)cmd->got, _got_ref);
							dt_add_array_elem(DT_DUMMY_DATATREE, result_array, _got_ref, NULL);
							dt_destroy_value(DT_DUMMY_DATATREE, _got_ref);
						}
					}
				}
				_DT_POP_FROM_CONTEXT(cmd, select_stat);
			} while(0);
		}
		cmd->got = (struct _dt_value_t*)result_object;
		cmd->result_mode = DTQ_GOT_NOREF;

		if(orderby_stat != (~0)) {
			_DT_PUSH_FROM_CONTEXT(cmd, orderby_stat, cmd->got); {
				cmd->statements[orderby_stat].opcode(_s);
			}
			_DT_POP_FROM_CONTEXT(cmd, orderby_stat);
		}

		if(middle)
			dt_destroy_value(DT_DUMMY_DATATREE, middle);

		*cmd->statement_cursor = cmd->statement_count;
	} else {
		xpl_func_info_t* func = NULL;
		xpl_assert(_s->text);
		stat = _dt_append_statement(cmd);
		stat->opcode = _dt_query_from;
		XPL_SKIP_MEANINGLESS(_s);
		if(xpl_peek_func(_s, &func) != XS_OK) {
			stat->oprand.type = DT_STATEMENT_PATH;
			_dt_create_path(&stat->oprand.data.path, cmd->error_handler, _s);
		} else if(func->func == _dt_query_from) {
			xpl_status_t ret = XS_OK;
			int met_select_stat = 0;
			_dt_command_t* old_cmd = NULL;
			stat->oprand.type = DT_STATEMENT_COMD;
			xpl_assert(_s && _s->text && "Empty program");
			stat->oprand.data.cmd = (_dt_command_t*)dt_malloc(sizeof(_dt_command_t));
			dt_memclr(stat->oprand.data.cmd, sizeof(_dt_command_t));
			stat->oprand.data.cmd->error_handler = cmd->error_handler;
			old_cmd = (_dt_command_t*)_s->userdata;
			_s->userdata = stat->oprand.data.cmd;
			while(*_s->cursor && ret == XS_OK) {
				XPL_SKIP_MEANINGLESS(_s);
				xpl_peek_func(_s, &func);
				if(!met_select_stat && func->func == _dt_query_select)
					met_select_stat = 1;

				ret = xpl_step(_s);

				XPL_SKIP_MEANINGLESS(_s);
				xpl_peek_func(_s, &func);
				if(met_select_stat && (func->func == _dt_query_select || func->func == _dt_query_orderby))
					break;
			}
			stat->oprand.data.cmd->xpl.userdata = stat->oprand.data.cmd;
			stat->oprand.data.cmd->parsed = 1;
			_s->userdata = old_cmd;
		} else {
			stat->oprand.type = DT_STATEMENT_CURR;
		}
	}

	return XS_OK;
}

xpl_status_t _dt_query_where(xpl_context_t* _s) {
	_dt_command_t* cmd = NULL;
	_dt_statement_t* stat = NULL;
	xpl_assert(_s && _s->userdata);
	cmd = (_dt_command_t*)_s->userdata;
	if(cmd->parsed) {
		stat = &cmd->statements[*cmd->statement_cursor];
		_dt_eval_expr(DT_DUMMY_DATATREE, cmd->target, stat->oprand.data.expr, &cmd->expr_eval_result);
	} else {
		xpl_assert(_s->text);
		stat = _dt_append_statement(cmd);
		stat->opcode = _dt_query_where;
		if(_DT_IS_NOT_FUNC(_s)) {
			stat->oprand.type = DT_STATEMENT_EXPR;
			cmd->result_mode = _dt_create_expr(&stat->oprand.data.expr, cmd->error_handler, _s);
		} else {
			cmd->result_mode = DTQ_EXPRESSION_EXPECTED;
		}
	}

	return XS_OK;
}

xpl_status_t _dt_query_select(xpl_context_t* _s) {
	_dt_command_t* cmd = NULL;
	_dt_statement_t* stat = NULL;
	xpl_assert(_s && _s->userdata);
	cmd = (_dt_command_t*)_s->userdata;
	if(cmd->parsed) {
		_dt_path_node_t* node = NULL;
		struct _dt_value_t* target = cmd->target;
		stat = &cmd->statements[*cmd->statement_cursor];
		if(stat->oprand.type == DT_STATEMENT_PATH) {
			if(!stat->oprand.data.path) {
				cmd->result_mode = DTQ_PATH_EXPECTED;
			} else {
				node = stat->oprand.data.path;
				cmd->result_mode = _dt_access_by_path(DT_DUMMY_DATATREE, target, node, &cmd->got);
			}
		} else if(stat->oprand.type == DT_STATEMENT_CTOR) {
			if(!stat->oprand.data.ctor) {
				cmd->result_mode = DTQ_CTOR_EXPECTED;
			} else {
				cmd->result_mode = _dt_make_new_object(DT_DUMMY_DATATREE, cmd->error_handler,
					target, stat->oprand.data.ctor, &cmd->got);
			}
		} else {
			DT_ASSERT(0 && "Invalid statement data type");
		}
		(*cmd->statement_cursor)++;
	} else {
		xpl_assert(_s->text);
		stat = _dt_append_statement(cmd);
		stat->opcode = _dt_query_select;
		if(_DT_IS_PATH_PREFIX(_s) && _DT_IS_NOT_FUNC(_s)) {
			stat->oprand.type = DT_STATEMENT_PATH;
			_dt_create_path(&stat->oprand.data.path, cmd->error_handler, _s);
		} else {
			stat->oprand.type = DT_STATEMENT_CTOR;
			_dt_create_ctor(&stat->oprand.data.ctor, cmd->error_handler, _s);
		}
	}

	return XS_OK;
}

xpl_status_t _dt_query_orderby(xpl_context_t* _s) {
	_dt_command_t* cmd = NULL;
	_dt_statement_t* stat = NULL;
	xpl_assert(_s && _s->userdata);
	cmd = (_dt_command_t*)_s->userdata;
	if(cmd->parsed) {
		dt_array_t arr = NULL;
		size_t n = 0;
		stat = &cmd->statements[*cmd->statement_cursor];
		dt_value_data_as(&arr, (dt_value_t)cmd->got, DT_ARRAY);
		dt_array_elem_count(DT_DUMMY_DATATREE, arr, &n);
		_dt_sort_array(cmd, stat->oprand.data.path, arr, 0, (int)(n - 1));
	} else {
		xpl_assert(_s->text);
		stat = _dt_append_statement(cmd);
		stat->opcode = _dt_query_orderby;
		stat->oprand.type = DT_STATEMENT_PATH;
		_dt_create_path(&stat->oprand.data.path, cmd->error_handler, _s);
	}

	return XS_OK;
}

xpl_status_t _dt_query_insert(xpl_context_t* _s) {
	_dt_command_t* cmd = NULL;
	xpl_assert(_s && _s->userdata);
	cmd = (_dt_command_t*)_s->userdata;
	if(cmd->parsed) {
	} else {
	}

	return XS_OK;
}

xpl_status_t _dt_query_delete(xpl_context_t* _s) {
	_dt_command_t* cmd = NULL;
	xpl_assert(_s && _s->userdata);
	cmd = (_dt_command_t*)_s->userdata;
	if(cmd->parsed) {
	} else {
	}

	return XS_OK;
}

xpl_status_t _dt_query_update(xpl_context_t* _s) {
	_dt_command_t* cmd = NULL;
	xpl_assert(_s && _s->userdata);
	cmd = (_dt_command_t*)_s->userdata;
	if(cmd->parsed) {
	} else {
	}

	return XS_OK;
}

xpl_status_t _dt_query_value(xpl_context_t* _s) {
	_dt_command_t* cmd = NULL;
	xpl_assert(_s && _s->userdata);
	cmd = (_dt_command_t*)_s->userdata;
	if(cmd->parsed) {
	} else {
	}

	return XS_OK;
}

xpl_status_t _dt_query_open(xpl_context_t* _s) {
	_dt_command_t* cmd = NULL;
	xpl_assert(_s && _s->userdata);
	cmd = (_dt_command_t*)_s->userdata;
	if(cmd->parsed) {
	} else {
	}

	return XS_OK;
}

xpl_status_t _dt_query_close(xpl_context_t* _s) {
	_dt_command_t* cmd = NULL;
	xpl_assert(_s && _s->userdata);
	cmd = (_dt_command_t*)_s->userdata;
	if(cmd->parsed) {
	} else {
	}

	return XS_OK;
}

xpl_status_t _dt_query_sync(xpl_context_t* _s) {
	_dt_command_t* cmd = NULL;
	xpl_assert(_s && _s->userdata);
	cmd = (_dt_command_t*)_s->userdata;
	if(cmd->parsed) {
	} else {
	}

	return XS_OK;
}

xpl_status_t _dt_query_push(xpl_context_t* _s) {
	_dt_command_t* cmd = NULL;
	xpl_assert(_s && _s->userdata);
	cmd = (_dt_command_t*)_s->userdata;
	if(cmd->parsed) {
	} else {
	}

	return XS_OK;
}

xpl_status_t _dt_query_pop(xpl_context_t* _s) {
	_dt_command_t* cmd = NULL;
	xpl_assert(_s && _s->userdata);
	cmd = (_dt_command_t*)_s->userdata;
	if(cmd->parsed) {
	} else {
	}

	return XS_OK;
}

xpl_status_t _dt_query_peek(xpl_context_t* _s) {
	_dt_command_t* cmd = NULL;
	xpl_assert(_s && _s->userdata);
	cmd = (_dt_command_t*)_s->userdata;
	if(cmd->parsed) {
	} else {
	}

	return XS_OK;
}

void _dt_sort_array(_dt_command_t* cmd, _dt_path_node_t* path, dt_array_t arr, int m, int n) {
	int i, j, k;
	dt_value_t l = NULL;
	dt_value_t r = NULL;
	dt_value_t key = NULL;
	if(m < n) {
		k = (m + n) / 2;
		_DT_ARRAY_ELEM_PATH(cmd, arr, path, m, l);
		_DT_ARRAY_ELEM_PATH(cmd, arr, path, k, r);
		dt_array_elem_at(DT_DUMMY_DATATREE, arr, m, &l);
		dt_array_elem_at(DT_DUMMY_DATATREE, arr, k, &r);
		dt_value_mem_swap(l, r);
		_DT_ARRAY_ELEM_PATH(cmd, arr, path, m, key);
		i = m + 1;
		j = n;
		while(i <= j) {
			dt_value_t t = NULL;
			_DT_ARRAY_ELEM_PATH(cmd, arr, path, i, t);
			while((i <= n) && (dt_value_compare(t, key, DT_TRUE) <= 0)) {
				i++;
				if(i > n) break;
				_DT_ARRAY_ELEM_PATH(cmd, arr, path, i, t);
			}
			_DT_ARRAY_ELEM_PATH(cmd, arr, path, j, t);
			while((j >= m) && (dt_value_compare(t, key, DT_TRUE) > 0)) {
				j--;
				if(j < m) break;
				_DT_ARRAY_ELEM_PATH(cmd, arr, path, j, t);
			}
			if(i < j) {
				_DT_ARRAY_ELEM_PATH(cmd, arr, path, i, l);
				_DT_ARRAY_ELEM_PATH(cmd, arr, path, j, r);
				dt_array_elem_at(DT_DUMMY_DATATREE, arr, i, &l);
				dt_array_elem_at(DT_DUMMY_DATATREE, arr, j, &r);
				dt_value_mem_swap(l, r);
			}
		}

		_DT_ARRAY_ELEM_PATH(cmd, arr, path, m, l);
		_DT_ARRAY_ELEM_PATH(cmd, arr, path, j, r);
		dt_array_elem_at(DT_DUMMY_DATATREE, arr, m, &l);
		dt_array_elem_at(DT_DUMMY_DATATREE, arr, j, &r);
		dt_value_mem_swap(l, r);

		_dt_sort_array(cmd, path, arr, m, j - 1);
		_dt_sort_array(cmd, path, arr, j + 1, n);
	}
}

_dt_statement_t* _dt_resize_statements(_dt_statement_t** p, size_t s) {
	DT_ASSERT(p && s);

	return (_dt_statement_t*)dt_realloc((void**)p, s * sizeof(_dt_statement_t));
}

void _dt_clear_statements(_dt_statement_t* p, size_t s) {
	size_t i = 0;
	_dt_statement_t* stat = NULL;
	DT_ASSERT(p);
	for(i = 0; i < s; i++) {
		stat = &p[i];
		switch(stat->oprand.type) {
		case DT_STATEMENT_PATH:
			if(stat->oprand.data.path)
				_dt_destroy_path(stat->oprand.data.path);
			break;
		case DT_STATEMENT_CTOR:
			if(stat->oprand.data.ctor)
				_dt_destroy_ctor(stat->oprand.data.ctor);
			break;
		case DT_STATEMENT_COMD:
			if(stat->oprand.data.cmd)
				dt_destroy_command((dt_command_t)stat->oprand.data.cmd);
			break;
		case DT_STATEMENT_EXPR:
			if(stat->oprand.data.expr)
				_dt_destroy_expr(stat->oprand.data.expr);
			break;
		case DT_STATEMENT_CURR: /* do nothing */
			break;
		default:
			DT_ASSERT(0 && "Invalid statement data type");
		}
	}
}

_dt_statement_t* _dt_append_statement(_dt_command_t* cmd) {
	_dt_statement_t* result = NULL;
	DT_ASSERT(cmd);
	cmd->statements = _dt_resize_statements(&cmd->statements, ++cmd->statement_count);
	result = &cmd->statements[cmd->statement_count - 1];
	dt_memclr(result, sizeof(_dt_statement_t));

	return result;
}

int _dt_expr_is_op(_dt_expr_node_t* e) {
	return e && e->type < DT_EXPR_NODE_PATH;
}

void _dt_expr_push(_dt_expr_node_t** s, _dt_expr_node_t* e) {
	DT_ASSERT(s && e);
	if(*s) {
		e->helper = *s;
		*s = e;
	} else {
		(*s) = e;
	}
}

_dt_expr_node_t* _dt_expr_pop(_dt_expr_node_t** s) {
	_dt_expr_node_t* result = NULL;
	DT_ASSERT(s);
	if(!(*s)) {
		result = NULL;
	} else if((*s)->helper) {
		result = (*s);
		*s = (*s)->helper;
	} else {
		result = (*s);
		*s = NULL;
	}

	return result;
}

_dt_expr_node_t* _dt_expr_top(_dt_expr_node_t** s) {
	DT_ASSERT(s);
	if(!(*s)) {
		return NULL;
	} else {
		return *s;
	}
}

_dt_expr_node_t* _dt_expr_operate(dt_datatree_t d, struct _dt_value_t* target, _dt_expr_node_t* a, _dt_expr_node_t* theta, _dt_expr_node_t* b) {
	_dt_expr_node_t* result = NULL;
	union { struct _dt_value_t* val; int b; } val_a, val_b;
	DT_ASSERT(d && target && a && theta && b);
	val_a.val = val_b.val = NULL;
	result = (_dt_expr_node_t*)dt_malloc(sizeof(_dt_expr_node_t));
	dt_memclr((void*)(result), sizeof(_dt_expr_node_t));
	result->type = DT_EXPR_NODE_BOOL;
	result->data.bool_value = 0;

	if(a->type == DT_EXPR_NODE_VALUE) {
		val_a.val = a->data.value;
	} else if(a->type == DT_EXPR_NODE_PATH) {
		_dt_access_by_path(d, target, a->data.path, &val_a.val);
		if(!val_a.val)
			return result;
	} else if(a->type == DT_EXPR_NODE_BOOL) {
		val_a.b = a->data.bool_value;
	}

	if(b->type == DT_EXPR_NODE_VALUE) {
		val_b.val = b->data.value;
	} else if(b->type == DT_EXPR_NODE_PATH) {
		_dt_access_by_path(d, target, b->data.path, &val_b.val);
		if(!val_b.val)
			return result;
	} else if(b->type == DT_EXPR_NODE_BOOL) {
		val_b.b = b->data.bool_value;
	}

	switch(theta->type) {
	case DT_EXPR_NODE_EVAL:
		break;
	case DT_EXPR_NODE_LEFT_PARENTHESES:
		break;
	case DT_EXPR_NODE_RIGHT_PARENTHESES:
		break;
	case DT_EXPR_NODE_LESS:
		result->data.bool_value = dt_value_compare((dt_value_t)val_a.val, (dt_value_t)val_b.val, DT_FALSE) < 0;
		break;
	case DT_EXPR_NODE_GREATER:
		result->data.bool_value = dt_value_compare((dt_value_t)val_a.val, (dt_value_t)val_b.val, DT_FALSE) > 0;
		break;
	case DT_EXPR_NODE_LESS_EQUAL:
		result->data.bool_value = dt_value_compare((dt_value_t)val_a.val, (dt_value_t)val_b.val, DT_FALSE) <= 0;
		break;
	case DT_EXPR_NODE_GREATER_EQUAL:
		result->data.bool_value = dt_value_compare((dt_value_t)val_a.val, (dt_value_t)val_b.val, DT_FALSE) >= 0;
		break;
	case DT_EXPR_NODE_EQUAL:
		result->data.bool_value = dt_value_compare((dt_value_t)val_a.val, (dt_value_t)val_b.val, DT_FALSE) == 0;
		break;
	case DT_EXPR_NODE_NOT_EQUAL:
		result->data.bool_value = dt_value_compare((dt_value_t)val_a.val, (dt_value_t)val_b.val, DT_FALSE) != 0;
		break;
	case DT_EXPR_NODE_AND:
		result->data.bool_value = val_a.b && val_b.b;
		break;
	case DT_EXPR_NODE_OR:
		result->data.bool_value = val_a.b || val_b.b;
		break;
	case DT_EXPR_NODE_IS: {
			char* str = NULL;
			unsigned int t = (unsigned int)(~0);
			dt_value_data_as(&str, (dt_value_t)val_b.val, DT_STRING);
			t = _dt_string_to_type(str);
			result->data.bool_value = _dt_value_is(val_a.val, t);
		}
		break;
	case DT_EXPR_NODE_HAS: {
			dt_type_t t = dt_value_type((dt_value_t)val_a.val);
			dt_value_t o = NULL;
			dt_object_t obj = NULL;
			if(t != DT_OBJECT) break;
			dt_value_data_as(&obj, (dt_value_t)val_a.val, DT_OBJECT);
			dt_find_object_member_by_key(d, obj, (dt_value_t)val_b.val, &o, NULL);
			result->data.bool_value = !!o;
		}
		break;
	default:
		DT_ASSERT(0 && "Invalid type");
	}

	return result;
}

dt_query_status_t _dt_create_expr(_dt_expr_node_t** e, dt_parse_error_handler_t eh, xpl_context_t* _s) {
	_dt_expr_node_t* expr = NULL;
	DT_ASSERT(e && _s);
	expr = NULL;
	while(_DT_IS_NOT_FUNC(_s)) {
		if(expr) {
			expr->next = (_dt_expr_node_t*)dt_malloc(sizeof(_dt_expr_node_t));
			dt_memclr((void*)(expr->next), sizeof(_dt_expr_node_t));
			expr = expr->next;
			expr->next = NULL;
		} else {
			expr = (_dt_expr_node_t*)dt_malloc(sizeof(_dt_expr_node_t));
			dt_memclr((void*)(expr), sizeof(_dt_expr_node_t));
			*e = expr;
		}

		XPL_SKIP_MEANINGLESS(_s);
		if(*_s->cursor == '\0') { /* TODO */ }
		if((_s->cursor[0] == 'l' && _s->cursor[1] == 'e') || (_s->cursor[0] == 'L' && _s->cursor[1] == 'E')) {
			expr->type = DT_EXPR_NODE_LESS_EQUAL;
			_s->cursor += 2;
		} else if((_s->cursor[0] == 'g' && _s->cursor[1] == 'e') || (_s->cursor[0] == 'G' && _s->cursor[1] == 'E')) {
			expr->type = DT_EXPR_NODE_GREATER_EQUAL;
			_s->cursor += 2;
		} else if((_s->cursor[0] == 'e' && _s->cursor[1] == 'q') || (_s->cursor[0] == 'E' && _s->cursor[1] == 'Q')) {
			expr->type = DT_EXPR_NODE_EQUAL;
			_s->cursor += 2;
		} else if((_s->cursor[0] == 'n' && _s->cursor[1] == 'e') || (_s->cursor[0] == 'N' && _s->cursor[1] == 'E')) {
			expr->type = DT_EXPR_NODE_NOT_EQUAL;
			_s->cursor += 2;
		} else if((_s->cursor[0] == 'l' && _s->cursor[1] == 't') || (_s->cursor[0] == 'L' && _s->cursor[1] == 'T')) {
			expr->type = DT_EXPR_NODE_LESS;
			_s->cursor += 2;
		} else if((_s->cursor[0] == 'g' && _s->cursor[1] == 't') || (_s->cursor[0] == 'G' && _s->cursor[1] == 'T')) {
			expr->type = DT_EXPR_NODE_GREATER;
			_s->cursor += 2;
		} else if(*_s->cursor == '(') {
			expr->type = DT_EXPR_NODE_LEFT_PARENTHESES;
			_s->cursor += 1;
		} else if(*_s->cursor == ')') {
			expr->type = DT_EXPR_NODE_RIGHT_PARENTHESES;
			_s->cursor += 1;
		} else if(_xpl_strcmp(_s->cursor, "and") == 0 || _xpl_strcmp(_s->cursor, "AND") == 0) {
			expr->type = DT_EXPR_NODE_AND;
			_s->cursor += 3;
		} else if(_xpl_strcmp(_s->cursor, "or") == 0 || _xpl_strcmp(_s->cursor, "OR") == 0) {
			expr->type = DT_EXPR_NODE_OR;
			_s->cursor += 2;
		} else if(_xpl_strcmp(_s->cursor, "is") == 0 || _xpl_strcmp(_s->cursor, "IS") == 0) {
			expr->type = DT_EXPR_NODE_IS;
			_s->cursor += 2;
		} else if(_xpl_strcmp(_s->cursor, "has") == 0 || _xpl_strcmp(_s->cursor, "HAS") == 0) {
			expr->type = DT_EXPR_NODE_HAS;
			_s->cursor += 3;
		} else if(_DT_IS_PATH_PREFIX(_s)) {
			expr->type = DT_EXPR_NODE_PATH;
			_dt_create_path(&expr->data.path, eh, _s);
		} else {
			int step = 0;
			expr->type = DT_EXPR_NODE_VALUE;
			step = dt_create_value_ex(DT_DUMMY_DATATREE, (dt_value_t*)&expr->data.value,
				_dt_is_command_separator_char, eh, _s->cursor);
			_s->cursor += step;
		}
		XPL_SKIP_MEANINGLESS(_s);
	}

	return DTQ_OK;
}

void _dt_destroy_expr(_dt_expr_node_t* e) {
	_dt_expr_node_t* expr = NULL;
	DT_ASSERT(e);
	while(e) {
		expr = e;
		e = e->next;
		switch(expr->type) {
		case DT_EXPR_NODE_LEFT_PARENTHESES: /* fall through */
		case DT_EXPR_NODE_RIGHT_PARENTHESES: /* fall through */
		case DT_EXPR_NODE_LESS: /* fall through */
		case DT_EXPR_NODE_GREATER: /* fall through */
		case DT_EXPR_NODE_LESS_EQUAL: /* fall through */
		case DT_EXPR_NODE_GREATER_EQUAL: /* fall through */
		case DT_EXPR_NODE_EQUAL: /* fall through */
		case DT_EXPR_NODE_NOT_EQUAL: /* fall through */
		case DT_EXPR_NODE_AND: /* fall through */
		case DT_EXPR_NODE_OR: /* fall through */
		case DT_EXPR_NODE_IS: /* fall through */
		case DT_EXPR_NODE_HAS: /* fall through */
		case DT_EXPR_NODE_BOOL: /* do nothing */
			break;
		case DT_EXPR_NODE_PATH:
			_dt_destroy_path(expr->data.path);
			break;
		case DT_EXPR_NODE_VALUE:
			dt_destroy_value(DT_DUMMY_DATATREE, (dt_value_t)expr->data.value);
			break;
		default:
			DT_ASSERT(0 && "Invalid type");
		}
		dt_free((void**)&expr);
	}
}

dt_query_status_t _dt_eval_expr(dt_datatree_t d, struct _dt_value_t* target, _dt_expr_node_t* e, int* ret) {
	_dt_expr_node_t* optr = NULL;
	_dt_expr_node_t* opnd = NULL;
	_dt_expr_node_t* node = NULL;
	_dt_expr_node_t* c = NULL;
	_dt_expr_node_t* x = NULL;
	_dt_expr_node_t* a = NULL;
	_dt_expr_node_t* b = NULL;
	_dt_expr_node_t* theta = NULL;
	DT_ASSERT(d && target && e && ret);
	*ret = 1;

	node = (_dt_expr_node_t*)dt_malloc(sizeof(_dt_expr_node_t));
	dt_memclr((void*)(node), sizeof(_dt_expr_node_t));
	node->next = NULL;
	node->type = DT_EXPR_NODE_EVAL;
	_dt_expr_push(&optr, node);

	c = e;
	while((c && c->type != DT_EXPR_NODE_EVAL) || _dt_expr_top(&optr)->type != DT_EXPR_NODE_EVAL) {
		if(!c) c = node;
		if(!_dt_expr_is_op(c)) {
			_dt_expr_push(&opnd, c);
			c = c->next;
		} else {
			switch(_DT_EXPR_OPCODE_PRECEDE[_dt_expr_top(&optr)->type][c->type]) {
			case '<':
				_dt_expr_push(&optr, c);
				c = c->next;
				break;
			case '=':
				x = _dt_expr_pop(&optr);
				_DT_FREE_EXPR_BOOL(x);
				c = c->next;
				break;
			case '>':
				theta = _dt_expr_pop(&optr);
				b = _dt_expr_pop(&opnd);
				a = _dt_expr_pop(&opnd);
				_dt_expr_push(&opnd, _dt_expr_operate(d, target, a, theta, b));
				_DT_FREE_EXPR_BOOL(a);
				_DT_FREE_EXPR_BOOL(b);
				break;
			default:
				DT_ASSERT(0 && "Invalid precede");
			}
        }
    }

	*ret = _dt_expr_top(&opnd)->data.bool_value;

	dt_free((void**)&node);
	_DT_POP_FREE_EXPR_BOOL(opnd);

	return DTQ_OK;
}

dt_query_status_t _dt_create_path_node(_dt_path_node_t**p, dt_parse_error_handler_t eh, xpl_context_t* _s) {
	_dt_path_node_t* node = NULL;
	DT_ASSERT(p && _s);
	*p = (_dt_path_node_t*)dt_malloc(sizeof(_dt_path_node_t));
	dt_memclr(*p, sizeof(_dt_path_node_t));
	node = *p;
	XPL_SKIP_MEANINGLESS(_s);
	switch(*_s->cursor) {
	case '.': {
			int step = 0;
			_s->cursor++;
			node->type = DT_PATH_NODE_OBJECT_KEY;
			step = dt_create_value_ex(DT_DUMMY_DATATREE, &node->value.key, _dt_is_command_separator_char, eh, _s->cursor);
			_s->cursor += step;
			if(dt_value_type(node->value.key) == DT_STRING) {
				char* str_data = NULL;
				dt_value_data_as(&str_data, node->value.key, DT_STRING);
				if(!strcmp(str_data, "_this_"))
					node->type = DT_PATH_NODE_BUILDIN_THIS;
				else if(!strcmp(str_data, "_len_"))
					node->type = DT_PATH_NODE_BUILDIN_LEN;
			}
		}
		break;
	case '<': {
			long index = 0;
			_s->cursor++;
			XPL_SKIP_MEANINGLESS(_s);
			node->type = DT_PATH_NODE_INDEX;
			xpl_pop_long(_s, &index);
			node->value.index = (size_t)index;

			XPL_SKIP_MEANINGLESS(_s);
			if(*_s->cursor != '>') {
				free((void**)(*p));
				*p = NULL;
				_dt_on_error(_s, _DTQ_STATUS_MSG, DTQ_RIGHT_ANGLE_BRACKET_EXPECTED, eh);

				return DTQ_RIGHT_ANGLE_BRACKET_EXPECTED;
			}
			_s->cursor++;
		}
		break;
	default:
		DT_ASSERT(0 && "Invalid charactor");
	}

	return DTQ_OK;
}

void _dt_destroy_path_node(_dt_path_node_t* p) {
	DT_ASSERT(p);
	switch(p->type) {
	case DT_PATH_NODE_BUILDIN_THIS: /* fall through */
	case DT_PATH_NODE_BUILDIN_LEN: /* fall through */
	case DT_PATH_NODE_OBJECT_KEY:
		dt_destroy_value(DT_DUMMY_DATATREE, p->value.key);
		break;
	case DT_PATH_NODE_INDEX: /* do nothing */
		break;
	default:
		DT_ASSERT(0 && "Invalid path node type");
	}
	dt_free((void**)&p);
}

dt_query_status_t _dt_create_path(_dt_path_node_t** p, dt_parse_error_handler_t eh, xpl_context_t* _s) {
	dt_query_status_t result = DTQ_OK;
	_dt_path_node_t* last = NULL;
	DT_ASSERT(p && _s);
	*p = NULL;
	XPL_SKIP_MEANINGLESS(_s);
	while(_DT_IS_PATH_PREFIX(_s) && _DT_IS_NOT_FUNC(_s)) {
		_dt_path_node_t* node = NULL;
		result = _dt_create_path_node(&node, eh, _s);
		if(result != DTQ_OK)
			return result;
		XPL_SKIP_MEANINGLESS(_s);
		if(last)
			last->next = node;
		else
			*p = node;
		last = node;
	}

	return result;
}

void _dt_destroy_path(_dt_path_node_t* p) {
	DT_ASSERT(p);
	while(p) {
		_dt_path_node_t* n = p->next;
		_dt_destroy_path_node(p);
		p = n;
	}
}

dt_query_status_t _dt_access_by_path(dt_datatree_t d, struct _dt_value_t* target, _dt_path_node_t* path, struct _dt_value_t** ret) {
	_dt_path_node_t* node = NULL;
	dt_type_t type = DT_NULL;
	union { dt_value_t value; dt_object_t object; dt_array_t array; } tmp, got;
	got.value = (dt_value_t)target;
	node = path;
	while(node) {
		switch(node->type) {
		case DT_PATH_NODE_BUILDIN_THIS: /* fall through */
		case DT_PATH_NODE_BUILDIN_LEN: /* fall through */
		case DT_PATH_NODE_OBJECT_KEY: {
				dt_value_t data = got.value;
				if(!data)
					return DTQ_GOT_NOTHING;
				type = dt_value_type(got.value);
				if(type == DT_OBJECT) {
					dt_value_data_as(&tmp.object, got.value, DT_OBJECT);
					dt_find_object_member_by_key(d, tmp.object, node->value.key, &got.value, NULL);
					if(node->type == DT_PATH_NODE_OBJECT_KEY || got.value)
						break;
				}
				if(node->type == DT_PATH_NODE_BUILDIN_THIS) {
					got.value = data;
					break;
				} else if(node->type == DT_PATH_NODE_BUILDIN_LEN) {
					int len = dt_value_data_length(data);
					dt_create_value(DT_DUMMY_DATATREE, &got.value, NULL, NULL, DT_INT, len);
					*ret = (struct _dt_value_t*)got.value;

					return DTQ_GOT_NOREF;
				} else {
					got.value = NULL;

					return DTQ_GOT_NOTHING;
				}
			}
			DT_ASSERT(0 && "Never here");
		case DT_PATH_NODE_INDEX:
			type = dt_value_type(got.value);
			if(type == DT_OBJECT) {
				dt_value_data_as(&tmp.object, got.value, DT_OBJECT);
				dt_object_member_at(d, tmp.object, node->value.index, NULL, &got.value);
			} else if(type == DT_ARRAY) {
				dt_value_data_as(&tmp.array, got.value, DT_ARRAY);
				dt_array_elem_at(d, tmp.array, node->value.index, &got.value);
			} else {
				return DTQ_ARRAY_OR_OBJECT_EXPECTED;
			}
			break;
		default:
			DT_ASSERT(0 && "Invalid path node type");
		}
		node = node->next;
	}
	*ret = (struct _dt_value_t*)got.value;

	if(!*ret)
		return DTQ_GOT_NOTHING;

	return DTQ_GOT_REF;
}

dt_query_status_t _dt_create_ctor(_dt_ctor_t** c, dt_parse_error_handler_t eh, xpl_context_t* _s) {
	dt_query_status_t result = DTQ_OK;
	int step = 0;
	dt_value_t val = NULL;
	_dt_path_node_t* path = NULL;
	_dt_ctor_t* ctor = NULL;
	DT_ASSERT(c && _s);

	*c = (_dt_ctor_t*)dt_malloc(sizeof(_dt_ctor_t));
	ctor = *c;
	dt_memclr(ctor, sizeof(_dt_ctor_t));

	for( ; ; ) {
		step = dt_create_value_ex(DT_DUMMY_DATATREE, &val, _dt_is_command_separator_char, eh, _s->cursor);
		_s->cursor += step;
		XPL_SKIP_MEANINGLESS(_s);

		if(*_s->cursor != '=') {
			result = DTQ_EQUAL_SIGN_EXPECTED;
			dt_destroy_value(DT_DUMMY_DATATREE, val);
			goto _exit;
		}
		_s->cursor++;
		XPL_SKIP_MEANINGLESS(_s);

		result = _dt_create_path(&path, eh, _s);
		if(result != DTQ_OK) { dt_destroy_value(DT_DUMMY_DATATREE, val); goto _exit; }
		XPL_SKIP_MEANINGLESS(_s);

		ctor->count++;
		ctor->key = (dt_value_t*)dt_realloc((void**)&(ctor->key), ctor->count * sizeof(dt_value_t));
		ctor->value_path = (_dt_path_node_t**)dt_realloc((void**)&(ctor->value_path),
			ctor->count * sizeof(_dt_path_node_t));
		ctor->key[ctor->count - 1] = val;
		ctor->value_path[ctor->count - 1] = path;

		if(*_s->cursor != ',') break;
		_s->cursor++;
		XPL_SKIP_MEANINGLESS(_s);
	}

_exit:
	if(result != DTQ_OK) {
		_dt_destroy_ctor(*c);
		*c = NULL;
	}

	return result;
}

void _dt_destroy_ctor(_dt_ctor_t* c) {
	size_t i = 0;
	dt_value_t val = NULL;
	_dt_path_node_t* path = NULL;
	DT_ASSERT(c);

	for(i = 0; i < c->count; i++) {
		val = c->key[i];
		path = c->value_path[i];
		dt_destroy_value(DT_DUMMY_DATATREE, val);
		_dt_destroy_path(path);
	}
	if(c->key)
		dt_free((void**)&(c->key));
	if(c->value_path)
		dt_free((void**)&(c->value_path));
	c->count = 0;
	dt_free((void**)&c);
}

dt_query_status_t _dt_make_new_object(dt_datatree_t d, dt_parse_error_handler_t eh, struct _dt_value_t* target, _dt_ctor_t* ctor, struct _dt_value_t** ret) {
	size_t i = 0;
	dt_value_t tmp_key = NULL;
	dt_value_t tmp_val = NULL;
	dt_value_t o = NULL;
	dt_object_t obj = NULL;
	_dt_path_node_t* path = NULL;
	dt_value_t key = NULL;
	struct _dt_value_t* val = NULL;
	dt_query_status_t mode = DTQ_OK;
	DT_ASSERT(d && target && ctor && ret);

	dt_create_value(d, &o, eh, "{}");
	dt_value_data_as(&obj, o, DT_OBJECT);
	for(i = 0; i < ctor->count; i++) {
		dt_create_value(d, &tmp_key, eh, NULL, DT_NULL);
		dt_create_value(d, &tmp_val, eh, NULL, DT_NULL);
		key = ctor->key[i];
		path = ctor->value_path[i];
		mode = _dt_access_by_path(d, target, path, &val);
		if(!_DT_GOT_VAL(mode)) {
			dt_destroy_value(d, tmp_key);
			dt_destroy_value(d, tmp_val);
			dt_destroy_value(d, o);

			return mode;
		}
		dt_clone_value(d, key, tmp_key);
		dt_clone_value(d, (dt_value_t)val, tmp_val);
		dt_add_object_member(d, obj, tmp_key, tmp_val, NULL);
		dt_destroy_value(d, tmp_key);
		dt_destroy_value(d, tmp_val);
	}

	*ret = (struct _dt_value_t*)o;

	return DTQ_GOT_NOREF;
}

/* ========================================================} */

/*
** {========================================================
** Protected function definitions
*/

unsigned int dt_qver(void) {
	return DTQVER;
}

const char* const dt_query_status_message(dt_query_status_t s) {
	DT_ASSERT(s >= 0 && s < DT_COUNT_OF(_DTQ_STATUS_MSG));
	if(s < 0 || s >= DT_COUNT_OF(_DTQ_STATUS_MSG))
		return NULL;

	return _DTQ_STATUS_MSG[s];
}

void dt_create_command(dt_command_t* c) {
	_dt_command_t* result = NULL;

	result = (_dt_command_t*)dt_malloc(sizeof(_dt_command_t));
	dt_memclr(result, sizeof(_dt_command_t));

	result->variable_table = dt_rbtree_create(
		_dt_var_name_cmp_func,
		_dt_var_name_dest_func,
		_dt_var_name_info_dest_func,
		_dt_var_name_print_func,
		_dt_value_print_info_func
	);

	*c = (dt_command_t)result;
}

void dt_destroy_command(dt_command_t c) {
	_dt_command_t* cmd = (_dt_command_t*)c;
	DT_ASSERT(cmd);
	xpl_close(&cmd->xpl);
	dt_clear_command(c);
	if(cmd->variable_table) {
		/* TODO */
		dt_rbtree_destroy(cmd->variable_table);
	}
	dt_free((void**)&cmd);
}

int dt_parse_command(dt_command_t c, dt_parse_error_handler_t eh, const char* fmt, ...) {
	int count = 1;
	_dt_command_t* cmd = (_dt_command_t*)c;
	char buf[DT_CMD_STR_LEN];
	va_list argptr;
	va_start(argptr, fmt);

	cmd->error_handler = eh;

	cmd->parsed = 0;

	vsprintf(buf, fmt, argptr);
	xpl_open(&cmd->xpl, _dt_query_statements, _dt_is_separator_char);
	cmd->xpl.use_hack_pfunc = 0;
	cmd->xpl.userdata = cmd;
	xpl_load(&cmd->xpl, buf);
		xpl_run(&cmd->xpl);
		count = (int)(cmd->xpl.cursor - cmd->xpl.text);
	xpl_unload(&cmd->xpl);
	va_end(argptr);

	cmd->parsed = 1;

	return count;
}

void dt_clear_command(dt_command_t c) {
	_dt_command_t* cmd = (_dt_command_t*)c;
	DT_ASSERT(cmd);
	if(cmd->statements) {
		_dt_clear_statements(cmd->statements, cmd->statement_count);
		dt_free((void**)&cmd->statements);
		cmd->statement_count = 0;
	}
}

dt_query_status_t dt_query(dt_value_t t, dt_command_t c, dt_value_t* ret) {
	_dt_command_t* cmd = (_dt_command_t*)c;
	_dt_statement_t* stat = NULL;
	size_t i = 0;
	DT_ASSERT(t && c);
	cmd->target = (struct _dt_value_t*)t;
	cmd->statement_cursor = &i;
	while(i < cmd->statement_count) {
		stat = &cmd->statements[i];
		DT_ASSERT(stat && stat->opcode);
		(*stat->opcode)(&cmd->xpl);
	}
	cmd->target = cmd->got;
	if(ret)
		*ret = (dt_value_t)cmd->got;

	return cmd->result_mode;
}

dt_value_t dt_last_queried(dt_command_t c) {
	_dt_command_t* cmd = (_dt_command_t*)c;
	DT_ASSERT(c);
	if(!c) return NULL;
	return (dt_value_t)cmd->got;
}

/* ========================================================} */

#ifdef __cplusplus
}
#endif /* __cplusplus */
