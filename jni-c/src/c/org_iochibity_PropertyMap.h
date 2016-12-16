/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class org_iochibity_PropertyMap */

#ifndef _Included_org_iochibity_PropertyMap
#define _Included_org_iochibity_PropertyMap
#ifdef __cplusplus
extern "C" {
#endif
#undef org_iochibity_PropertyMap_serialVersionUID
#define org_iochibity_PropertyMap_serialVersionUID 362498820763181265LL
#undef org_iochibity_PropertyMap_DEFAULT_INITIAL_CAPACITY
#define org_iochibity_PropertyMap_DEFAULT_INITIAL_CAPACITY 16L
#undef org_iochibity_PropertyMap_MAXIMUM_CAPACITY
#define org_iochibity_PropertyMap_MAXIMUM_CAPACITY 1073741824L
#undef org_iochibity_PropertyMap_DEFAULT_LOAD_FACTOR
#define org_iochibity_PropertyMap_DEFAULT_LOAD_FACTOR 0.75f
#undef org_iochibity_PropertyMap_TREEIFY_THRESHOLD
#define org_iochibity_PropertyMap_TREEIFY_THRESHOLD 8L
#undef org_iochibity_PropertyMap_UNTREEIFY_THRESHOLD
#define org_iochibity_PropertyMap_UNTREEIFY_THRESHOLD 6L
#undef org_iochibity_PropertyMap_MIN_TREEIFY_CAPACITY
#define org_iochibity_PropertyMap_MIN_TREEIFY_CAPACITY 64L
/*
 * Class:     org_iochibity_PropertyMap
 * Method:    getProp
 * Signature: (Ljava/lang/String;)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_org_iochibity_PropertyMap_getProp
  (JNIEnv *, jobject, jstring);

/*
 * Class:     org_iochibity_PropertyMap
 * Method:    setProp
 * Signature: (Ljava/lang/String;Ljava/lang/Object;J)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_org_iochibity_PropertyMap_setProp
  (JNIEnv *, jobject, jstring, jobject, jlong);

#ifdef __cplusplus
}
#endif
#endif