#ifndef __DT_STACK_H__
#define __DT_STACK_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef void* dt_data_t;
typedef void* dt_stack_t;

dt_stack_t dt_stack_join(dt_stack_t s1, dt_stack_t s2);
dt_stack_t dt_stack_create(void);
void dt_stack_push(dt_stack_t s, dt_data_t info);
dt_stack_t dt_stack_pop(dt_stack_t s);
int dt_stack_not_empty(dt_stack_t s);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DT_STACK_H__ */
