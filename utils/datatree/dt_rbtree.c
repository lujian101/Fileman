#include "dt_rbtree.h"
#include "rb_tree/red_black_tree.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

dt_rbtree_t dt_rbtree_create(
		int (*comp_func)(const void*, const void*),
		void (*dest_func)(void*),
		void (*info_dest_func)(void*),
		void (*print_func)(const void*),
		void (*print_info)(void*)) {
	return RBTreeCreate(comp_func, dest_func, info_dest_func, print_func, print_info);
}

dt_rbnode_t dt_rbtree_insert(dt_rbtree_t t, void* key, void* info) {
	return RBTreeInsert((rb_red_blk_tree*)t, key, info);
}

void dt_rbtree_print(dt_rbtree_t t) {
	RBTreePrint((rb_red_blk_tree*)t);
}

void dt_rbtree_delete(dt_rbtree_t t, dt_rbnode_t n) {
	RBDelete((rb_red_blk_tree*)t, (rb_red_blk_node*)n);
}

void dt_rbtree_destroy(dt_rbtree_t t) {
	RBTreeDestroy((rb_red_blk_tree*)t);
}

dt_rbnode_t dt_tree_predecessor(dt_rbtree_t t, dt_rbnode_t n) {
	return TreePredecessor((rb_red_blk_tree*)t, (rb_red_blk_node*)n);
}

dt_rbnode_t dt_tree_successor(dt_rbtree_t t, dt_rbnode_t n) {
	return TreeSuccessor((rb_red_blk_tree*)t, (rb_red_blk_node*)n);
}

dt_rbnode_t dt_rbtree_exact_query(dt_rbtree_t t, void* d) {
	return RBExactQuery((rb_red_blk_tree*)t, d);
}

dt_stack_t dt_rbtree_enumerate(dt_rbtree_t t, void* low, void* high) {
	return RBEnumerate((rb_red_blk_tree*)t, low, high);
}

void dt_null_function(void* d) {
	NullFunction(d);
}

void* dt_rbtree_node_key(dt_rbnode_t n) {
	rb_red_blk_node* node = (rb_red_blk_node*)n;

	return node->key;
}

void* dt_rbtree_node_info(dt_rbnode_t n) {
	rb_red_blk_node* node = (rb_red_blk_node*)n;

	return node->info;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
