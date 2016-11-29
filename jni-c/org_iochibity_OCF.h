/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class org_iochibity_OCF */

#ifndef _Included_org_iochibity_OCF
#define _Included_org_iochibity_OCF
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     org_iochibity_OCF
 * Method:    Init
 * Signature: (Ljava/lang/String;IILjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_org_iochibity_OCF_Init
  (JNIEnv *, jclass, jstring, jint, jint, jstring);

/*
 * Class:     org_iochibity_OCF
 * Method:    OCInit1
 * Signature: (III)I
 */
JNIEXPORT jint JNICALL Java_org_iochibity_OCF_OCInit1
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
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_iochibity_OCF_OCStartMulticastServer
  (JNIEnv *, jobject);

/*
 * Class:     org_iochibity_OCF
 * Method:    OCStopMulticastServer
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_iochibity_OCF_OCStopMulticastServer
  (JNIEnv *, jobject);

/*
 * Class:     org_iochibity_OCF
 * Method:    run
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_iochibity_OCF_run
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
 * Method:    setPlatformInfo
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_org_iochibity_OCF_setPlatformInfo
  (JNIEnv *, jclass, jstring, jstring, jstring, jstring, jstring, jstring, jstring, jstring, jstring, jstring, jstring);

/*
 * Class:     org_iochibity_OCF
 * Method:    setDeviceInfo
 * Signature: (Ljava/lang/String;[Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_org_iochibity_OCF_setDeviceInfo
  (JNIEnv *, jclass, jstring, jobjectArray, jstring, jobjectArray);

/*
 * Class:     org_iochibity_OCF
 * Method:    OCDoResource
 * Signature: (Ljava/lang/Object;ILjava/lang/String;Ljava/lang/Object;Ljava/lang/Object;IILjava/lang/Object;Ljava/lang/Object;B)I
 */
JNIEXPORT jint JNICALL Java_org_iochibity_OCF_OCDoResource
  (JNIEnv *, jobject, jobject, jint, jstring, jobject, jobject, jint, jint, jobject, jobject, jbyte);

/*
 * Class:     org_iochibity_OCF
 * Method:    OCCancel
 * Signature: (Ljava/lang/Object;ILjava/lang/Object;B)I
 */
JNIEXPORT jint JNICALL Java_org_iochibity_OCF_OCCancel
  (JNIEnv *, jobject, jobject, jint, jobject, jbyte);

/*
 * Class:     org_iochibity_OCF
 * Method:    sendResponse
 * Signature: (Lorg/iochibity/Response;)I
 */
JNIEXPORT jint JNICALL Java_org_iochibity_OCF_sendResponse
  (JNIEnv *, jclass, jobject);

#ifdef __cplusplus
}
#endif
#endif