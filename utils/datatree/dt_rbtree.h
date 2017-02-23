#ifndef __DT_RBTREE_H__
#define __DT_RBTREE_H__

#include "dt_stack.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef void* dt_rbtree_t;
typedef void* dt_rbnode_t;

dt_rbtree_t dt_rbtree_create(
	int (*comp_func)(const void*, const void*),
	void (*dest_func)(void*),
	void (*info_dest_func)(void*),
	void (*print_func)(const void*),
	void (*print_info)(void*)
);
dt_rbnode_t dt_rbtree_insert(dt_rbtree_t, void* key, void* info);
void dt_rbtree_print(dt_rbtree_t);
void dt_rbtree_delete(dt_rbtree_t, dt_rbnode_t);
void dt_rbtree_destroy(dt_rbtree_t);
dt_rbnode_t dt_tree_predecessor(dt_rbtree_t, dt_rbnode_t);
dt_rbnode_t dt_tree_successor(dt_rbtree_t, dt_rbnode_t);
dt_rbnode_t dt_rbtree_exact_query(dt_rbtree_t, void*);
dt_stack_t dt_rbtree_enumerate(dt_rbtree_t tree, void* low, void* high);
void dt_null_function(void*);
void* dt_rbtree_node_key(dt_rbnode_t);
void* dt_rbtree_node_info(dt_rbnode_t);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DT_RBTREE_H__ */
