/*
 * appclass.c - Methods to provide user interfaces to the AppClass structure.
 * 
 * include LICENSE
 * 
 * Originaly written Theodore A. Roth for the simulavr project,
 * it is now used as a common file that can be used in every project.
 * stolen from simulavr.
 */

#include <appclass.h>

#ifdef TRACE_MEM
#include <tracemem.h>
#endif


/** \brief This function should never be used. 
 *
 *  The only potential use for it as a template for derived classes. 
 *  Do Not Use This Function!
 */

AppClass *app_class_new(void)
{
   AppClass *klass = app_new(AppClass, 1);

   app_class_construct(klass);
   return klass;
}

/** \brief Initializes the AppClass data structure. 
 *
 *  A derived class should call this function from their own 
 *  <klass>_construct() function. All classes should
 *  have their constructor function call their parent's constructor
 *  function.
 */

void app_class_construct(AppClass * klass)
{
   if (klass == NULL) {
      msg_fatal(_("passed null ptr"));
   }
   klass->ref_count = 1;
   app_class_overload_destroy(klass, app_class_destroy);
}

/** \brief Releases resources allocated by class's <klass>_new() function. 
 *
 * This function should never be called except as the last statement 
 * of a directly derived class's destroy method. 
 * All classes should have their destroy method call their parent's 
 * destroy method.
 */

void app_class_destroy(void *klass)
{
   if (klass == NULL) {
      return;
   }
   app_free(klass);
}

/** \brief Overload the default destroy method.
 *
 * Derived classes will call this to replace app_class_destroy() with their own
 * destroy method.
 */

void app_class_overload_destroy(AppClass * klass,
				AppClassFP_Destroy destroy)
{
   if (klass == NULL) {
      msg_fatal(_("passed null ptr"));
   }

   klass->destroy = destroy;
}

/** \brief Increments the reference count for the klass object. 
 *
 * The programmer must call this whenever a reference to an object 
 * is stored in more than one place.
 */

void app_class_ref(AppClass * klass)
{
   if (klass == NULL) {
      msg_fatal(_("passed null ptr"));
   }
   klass->ref_count++;
}

/** \brief Decrements the reference count for the klass object. 
 *
 * When the reference count reaches zero, the class's destroy method 
 * is called on the object.
 */

void app_class_unref(AppClass * klass)
{
   if (klass == NULL) {
      msg_fatal(_("passed null ptr"));
   }

   klass->ref_count--;
   if (klass->ref_count == 0) {
      klass->destroy(klass);
   }
}

#ifdef USE_CLASS_TYPE
/** \brief Use the field type of the klass object. 
 *
 * Set the the value of the class type that may be useful for some
 * applications.
 */

void app_class_set_type(AppClass * klass, int type)
{
   if (klass == NULL) {
      msg_fatal(_("passed null ptr"));
   }
   klass->type = type;
}

/** \brief Use the field type of the klass object. 
 *
 * Get the the value of the class type that may be useful for some
 * applications.
 */
int app_class_get_type(AppClass * klass)
{
   if (klass == NULL) {
      msg_fatal(_("passed null ptr"));
   }
   return klass->type;
}

#endif /* USE_CLASS_TYPE */
