#ifndef APP_CLASS_H
#define APP_CLASS_H
/*
 * appclass.h - OOP Implementaion built up from AppClass structure.
 * 
 * include LICENSE
 */

#include <appmem.h>

typedef struct _AppClass AppClass;
typedef void (*AppClassFP_Destroy) (void *klass);

/* a generic function callback */
typedef int (*App_Run_FP)( AppClass *app, void *user_data );

struct _AppClass {
   int type;                    /* application may use this field to store a type */
   int ref_count;               /* Object reference count */
   AppClassFP_Destroy destroy;	/* pointer to destroy function, can be overridden */
};

extern AppClass *app_class_new(void);
extern void app_class_construct(AppClass * klass);
extern void app_class_destroy(void *klass);
extern void app_class_overload_destroy(AppClass * klass,
				       AppClassFP_Destroy destroy);
extern void app_class_ref(AppClass * klass);
extern void app_class_unref(AppClass * klass);

#ifdef USE_CLASS_TYPE
void app_class_set_type(AppClass * klass, int type);
int app_class_get_type(AppClass * klass);
#endif /* USE_CLASS_TYPE */

#endif				/* APP_CLASS_H */
