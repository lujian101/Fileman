#include "dt_stack.h"
#include "rb_tree/stack.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

dt_stack_t dt_stack_join(dt_stack_t s1, dt_stack_t s2) {
	return StackJoin((stk_stack*)s1, (stk_stack*)s2);
}

dt_stack_t dt_stack_create(void) {
	return StackCreate();
}

void dt_stack_push(dt_stack_t s, dt_data_t info) {
	StackPush((stk_stack*)s, info);
}

dt_stack_t dt_stack_pop(dt_stack_t s) {
	return StackPop((stk_stack*)s);
}

int dt_stack_not_empty(dt_stack_t s) {
	return StackNotEmpty((stk_stack*)s);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
