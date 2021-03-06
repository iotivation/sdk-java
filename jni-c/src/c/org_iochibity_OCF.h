/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class org_iochibity_OCF */

#ifndef _Included_org_iochibity_OCF
#define _Included_org_iochibity_OCF
#ifdef __cplusplus
extern "C" {
#endif
#undef org_iochibity_OCF_CLIENT
#define org_iochibity_OCF_CLIENT 0L
#undef org_iochibity_OCF_SERVER
#define org_iochibity_OCF_SERVER 1L
#undef org_iochibity_OCF_CLIENT_SERVER
#define org_iochibity_OCF_CLIENT_SERVER 2L
#undef org_iochibity_OCF_GATEWAY
#define org_iochibity_OCF_GATEWAY 3L
#undef org_iochibity_OCF_QOS_LOW
#define org_iochibity_OCF_QOS_LOW 0L
#undef org_iochibity_OCF_QOS_MEDIUM
#define org_iochibity_OCF_QOS_MEDIUM 1L
#undef org_iochibity_OCF_QOS_HIGH
#define org_iochibity_OCF_QOS_HIGH 2L
#undef org_iochibity_OCF_QOS_NA
#define org_iochibity_OCF_QOS_NA 3L
/*
 * Class:     org_iochibity_OCF
 * Method:    Init
 * Signature: (ILjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_iochibity_OCF_Init__ILjava_lang_String_2
  (JNIEnv *, jclass, jint, jstring);

/*
 * Class:     org_iochibity_OCF
 * Method:    Init
 * Signature: (III)V
 */
JNIEXPORT void JNICALL Java_org_iochibity_OCF_Init__III
  (JNIEnv *, jclass, jint, jint, jint);

/*
 * Class:     org_iochibity_OCF
 * Method:    OCRegisterPersistentStorageHandler
 * Signature: (Ljava/lang/Object;)I
 */
JNIEXPORT jint JNICALL Java_org_iochibity_OCF_OCRegisterPersistentStorageHandler
  (JNIEnv *, jobject, jobject);

/*
 * Class:     org_iochibity_OCF
 * Method:    OCStartMulticastServer
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_iochibity_OCF_OCStartMulticastServer
  (JNIEnv *, jobject);

/*
 * Class:     org_iochibity_OCF
 * Method:    OCStopMulticastServer
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_iochibity_OCF_OCStopMulticastServer
  (JNIEnv *, jobject);

/*
 * Class:     org_iochibity_OCF
 * Method:    run
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_iochibity_OCF_run
  (JNIEnv *, jclass);

/*
 * Class:     org_iochibity_OCF
 * Method:    stop
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_iochibity_OCF_stop
  (JNIEnv *, jclass);

/*
 * Class:     org_iochibity_OCF
 * Method:    OCCancel
 * Signature: (Ljava/lang/Object;ILjava/lang/Object;B)V
 */
JNIEXPORT void JNICALL Java_org_iochibity_OCF_OCCancel
  (JNIEnv *, jobject, jobject, jint, jobject, jbyte);

#ifdef __cplusplus
}
#endif
#endif
