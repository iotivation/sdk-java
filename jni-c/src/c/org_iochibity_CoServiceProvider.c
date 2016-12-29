/**
 * @file org_iochibity_CoServiceProvider.c
 * @author Gregg Reynolds
 * @date December 2016
 *
 * @brief JNI implementation of CoServiceProvider (client) Java API:
 * `Java_org_iochibity_CoServiceProvider_exhibitStimulus` and `c_CoServiceProvider_react`
 */

#include <ctype.h>
#include <string.h>
#include <stdlib.h>

/* http://www.informit.com/articles/article.aspx?p=2036582&seqNum=5
OpenBSD:  strlcpy
C11:      strncpy_s
*/

/* c11
#include <thread.h>
*/
#include <pthread.h>

#include "org_iochibity_CoServiceProvider.h"
#include "jni_utils.h"
#include "org_iochibity_Init.h"
#include "org_iochibity_Exceptions.h"

#include "octypes.h"
#include "oic_malloc.h"
#include "ocpayload.h"
#include "ocresource.h"
#include "ocresourcehandler.h"
#include "ocstack.h"

/* externs */
// FIXME: put these in init.c?
_Thread_local response_in_t* gtls_response_in	= NULL;

_Thread_local jobject gtls_CoSP			= NULL;

/* _Thread_local OCDevAddr* gtls_defaultCoAddress		= NULL; */

/* PRIVATE */

/* tls struct capturing params to OCDoResource */
typedef struct RequestOut
{
  OCDoHandle           handle;	/* key; will be returned in response_in */
  /* val: */
  OCMethod             method;
  const char         * requestUri;
  OCDevAddr          * destination;
  OCPayload          * payload;
  OCConnectivityType   connectivityType;
  OCQualityOfService   qos;
  OCCallbackData     * cbData;
  OCHeaderOption     * options;
  uint8_t              numOptions;

  struct RequestOut  * next;
} request_out_t;

request_out_t* g_request_out = NULL;

_Thread_local request_out_t* gtls_request_out = NULL;

void request_out_ctor()
{
  gtls_request_out		= OICCalloc(sizeof (request_out_t), 1);

  gtls_request_out->handle		= NULL;
  gtls_request_out->method		= OC_REST_NOMETHOD;
  /* never pass NULL for destination */
  gtls_request_out->destination		= (OCDevAddr *)OICCalloc(sizeof (OCDevAddr), 1);
  gtls_request_out->connectivityType	= CT_DEFAULT; /* 0 */
  gtls_request_out->qos			= OC_LOW_QOS;
  gtls_request_out->cbData		= NULL;
  gtls_request_out->options		= NULL;
  gtls_request_out->numOptions		= 0;


  gtls_request_out->next		= NULL;
}

/**
 * @brief Convert OCClientResponse to an `ObservationIn` object
 *
 * Allocate an `ObservationIn` java object, then use data from incoming
 * `OCClientResponse` to initialize it.  Return the initialize object.
 * Called internally by `c_CoServiceProvider_react`.
 *
 * @param [in] env JNIEnv pointer
 * @param [in] c_OCClientResponse response data from server
 * @return newly allocated and initialized `ObservationIn` object
 */
jobject OCClientResponse_to_ObservationIn(JNIEnv* env, OCClientResponse* c_OCClientResponse)
{
    printf("%s: OCClientResponse_to_ObservationIn ENTRY\n", __FILE__);
    jobject j_ObservationIn = (*env)->NewObject(env,
						K_OBSERVATION_IN,
						MID_OBIN_CTOR); // request_in_ctor);
    if (j_ObservationIn == NULL) {
	printf("NewObject failed for ObservationIn\n");
	return NULL;
    }

    /* set the c struct handle */
    (*env)->SetIntField(env, j_ObservationIn, FID_MSG_LOCAL_HANDLE, (jlong)c_OCClientResponse);

    /* OCDevAddr */
    jobject j_DeviceAddress = (*env)->NewObject(env, K_DEVICE_ADDRESS, MID_DA_CTOR);
    if (j_DeviceAddress == NULL) {
	printf("NewObject failed for DeviceAddress\n");
	return NULL;
    }
    (*env)->SetIntField(env, j_DeviceAddress,
			FID_DA_NETWORK_PROTOCOL, c_OCClientResponse->devAddr.adapter);

    /* OCTransportFlags */
    printf("TRANSPORT FLAGS: 0x%08X\n", c_OCClientResponse->devAddr.flags);
    (*env)->SetIntField(env, j_DeviceAddress,
			FID_DA_NETWORK_POLICIES, c_OCClientResponse->devAddr.flags >> 4);
    (*env)->SetIntField(env, j_DeviceAddress,
			FID_DA_NETWORK_SCOPE, c_OCClientResponse->devAddr.flags & 0x000F);
    (*env)->SetBooleanField(env, j_DeviceAddress,
			    FID_DA_TRANSPORT_SECURITY, c_OCClientResponse->devAddr.flags & 0x0010);

    (*env)->SetIntField(env, j_DeviceAddress,
			FID_DA_PORT, c_OCClientResponse->devAddr.port);

    jstring j_addr = (*env)->NewStringUTF(env, c_OCClientResponse->devAddr.addr);
    (*env)->SetObjectField(env, j_DeviceAddress, FID_DA_ADDRESS, j_addr);

    (*env)->SetIntField(env, j_DeviceAddress, FID_DA_IFINDEX, c_OCClientResponse->devAddr.ifindex);

    /* jstring j_route = (*env)->NewStringUTF(env, c_OCClientResponse->devAddr.routeData); */
    /* (*env)->SetObjectField(env, j_DeviceAddress, FID_DA_ROUTE_DATA, j_route); */

    (*env)->SetObjectField(env, j_ObservationIn, FID_OBIN_REMOTE_DEVADDR, j_DeviceAddress);

    /* connectivity type */
    /* note: this duplicates info in the devaddr */
    printf("RESPONSE CONNECTION TYPE: 0x%08X\n",  c_OCClientResponse->connType);
    (*env)->SetIntField(env, j_ObservationIn, FID_OBIN_CONN_TYPE, c_OCClientResponse->connType);

    /* FIXME: use id_length */
    jstring j_sid = (*env)->NewStringUTF(env, (char*)c_OCClientResponse->identity.id);
    (*env)->SetObjectField(env, j_ObservationIn, FID_OBIN_REMOTE_SID, j_sid);

    (*env)->SetIntField(env, j_ObservationIn, FID_OBIN_RESULT, c_OCClientResponse->result);
    (*env)->SetIntField(env, j_ObservationIn, FID_OBIN_SERIAL, c_OCClientResponse->sequenceNumber);

    jstring j_uri = (*env)->NewStringUTF(env, c_OCClientResponse->resourceUri);
    (*env)->SetObjectField(env, j_ObservationIn, FID_OBIN_URI, j_uri);

    (*env)->SetLongField(env, j_ObservationIn,
			 FID_OBIN_OBSERVATION_HANDLE,
			 (long)(intptr_t)c_OCClientResponse->payload);

    /* set optionCount and ptr_Options in Message */
    printf("%s: OCClientResponse_to_ObservationIn EXIT\n", __FILE__);
    return j_ObservationIn;
}

/**
 * @brief Implementation of `OCClientResponseHandler` function type;
 * called by stack on receipt of incoming `OCClientResponse` from
 * server.
 *
 * Converts the incoming response data to an `Observation` object,
 * and then passes that to the `observeBehavior` method of the
 * transaction's `CoServiceProvider` object for handling.
 *
 * The `CoServiceProvider` object is conveyed by the c_CoSP parameter
 * (in the C API, void* context), which was passed (as the "context"
 * pointer of the `OCCallbackData` param of `OCDoResource`) by the
 * `exhibitStimulus` routine of the `CoServiceProvider`.
 *
 * IMPORTANT: this is an internal implementation routine, with no
 * header prototoype.  It serves as an intermediary between the stack
 * the the Java application's callback method.
 *
 *  @param [in] c_CoSP handle to `CoServiceProvider` (client) object
 *  containing callback method; `context` in C API
 *
 * @param [in] c_TransactionHandle token identifying originating
 * `OCDoResource` invocation (not the same as the CoAP token)
 *
 * @param [in] c_OCClientResponse response data from server
 *
 * @return result code of type `OCStackApplicationResult`
 *
 * @see Java_org_iochibity_CoServiceProvider_exhibitStimulus
 * @see observe_stimulus
 * @see Java_org_iochibity_ServiceProvider_exhibitBehavior
 */
OCStackApplicationResult c_org_iochibity_CoServiceProvider_react(void* c_CoSP,
					  OCDoHandle c_TransactionHandle,
					  OCClientResponse* c_OCClientResponse)
{
  printf("%s : %s ENTRY, %ld\n", __FILE__, __func__, (intptr_t)pthread_self());

    /* To support multi-threading, we cache the incoming
       OCClientResponse record in a TLS var: */
    gtls_response_in = OICCalloc(sizeof (response_in_t), 1);
    gtls_response_in->handle   = c_TransactionHandle;
    gtls_response_in->response = c_OCClientResponse;

    /* now look up the corresponding request_out record, using
       c_TransactionHandle as key. */



    /* printf("TLS: %ld\n", (intptr_t)gtls_response_in); */

    /* printf("STACK RESULT: %d\n", c_OCClientResponse->result); */

    /* printf("SP Address: %s\n", */
    /* 	   ((OCClientResponse*)gtls_response_in->response)->devAddr.addr); */

    /* 1. set up jvm, env */
    /* http://stackoverflow.com/questions/12900695/how-to-obtain-jni-interface-pointer-jnienv-for-asynchronous-calls */
    /* http://adamish.com/blog/archives/327 */
    JNIEnv * env;
    // double check it's all ok
    int getEnvStat = (*g_JVM)->GetEnv(g_JVM, (void **)&env, JNI_VERSION_1_6);
    if (getEnvStat == JNI_EDETACHED) {
	/* printf("GetEnv: not attached; attaching now\n"); */
	if ((*g_JVM)->AttachCurrentThread(g_JVM, (void **) &env, NULL) != 0) {
	    printf("ERROR %s %d (%s): AttachCurrentThread failure\n", __FILE__, __LINE__,__func__);
	    return OC_STACK_DELETE_TRANSACTION;
	}
    } else if (getEnvStat == JNI_OK) {
	/* printf("GetEnv: attached\n"); */
    } else if (getEnvStat == JNI_EVERSION) {
	printf("ERROR %s %d (%s): JNI version not supported\n", __FILE__, __LINE__,__func__);
	return OC_STACK_DELETE_TRANSACTION;
    }
    if ((*env)->ExceptionCheck(env)) {
	(*env)->ExceptionDescribe(env);
    }

    /* print_class_name(env, c_CoSP); */

    /* if ctx param is null something went wrong */
    if (c_CoSP == NULL) {

	printf("ERROR %s %d (%s): ctx param is NULL for c_CoServiceProvider_react\n",
	       __FILE__, __LINE__,__func__);
	return OC_STACK_DELETE_TRANSACTION;
    }

    // FIXME: we do not need ObservationIn if we're caching an
    // OCClientResponse* and using native methods to access details ...

    /* 2. construct ObservationIn from incoming OCClientResponse ... */
    /* jobject j_ObservationIn = OCClientResponse_to_ObservationIn(env, c_OCClientResponse); */

    /* /\* ... and insert pointer to it.*\/ */
    /* (*env)->SetLongField(env, j_ObservationIn, */
    /* 			 FID_MSG_LOCAL_HANDLE, (intptr_t)c_OCClientResponse); */


    // To call the Java-side handler for incoming response data, we
    // need to get a reference to it.

    /* 2. extract ref to observeBehavior routine from CoSP callback param */
    /* /\* we need to get the method from the object, not the class? */
    jclass k_cosp = (*env)->GetObjectClass(env, (jobject)c_CoSP);
    if (k_cosp == NULL) {
    	THROW_JNI_EXCEPTION("GetObjectClass failed for CoServiceProvider object\n");
    	return OC_STACK_DELETE_TRANSACTION;
    }

    jmethodID mid_cosp_react = (*env)->GetMethodID(env, k_cosp, "react", "()V");
    if (mid_cosp_react == NULL) {
    	THROW_JNI_EXCEPTION("GetMethodID failed for mid_cosp_react of CoServiceProvider\n");
    	return OC_STACK_DELETE_TRANSACTION;
    }

    /* jmethodID mid_cosp_c_CoServiceProvider_react = (*env)->GetMethodID(env, k_cosp, */
    /* 								 "observeBehavior", */
    /* 								 "(Lorg/iochibity/ObservationIn;)I"); */
    /* if (mid_cosp_c_CoServiceProvider_react == NULL) { */
    /* 	THROW_JNI_EXCEPTION("GetMethodID failed for mid_cosp_c_CoServiceProvider_react of CoServiceProvider\n"); */
    /* 	return OC_STACK_DELETE_TRANSACTION; */
    /* } */

    /* 5. call the observeBehavior method of the CoServiceProvider */
    int op_result = OC_EH_OK;
    op_result = (*env)->CallIntMethod(env,
				      (jobject)c_CoSP,
				      mid_cosp_react);
				      /* mid_cosp_c_CoServiceProvider_react, */
				      /* MID_ICOSP_C_SERVICEPROVIDER_REACT, */
				      /* j_ObservationIn); */

    if (op_result != OC_STACK_OK) {
        printf("ERROR:  CallIntMethod failed for CoServiceProvider.ObserveStimulus\n");
	return OC_STACK_DELETE_TRANSACTION;
    }

    /* free the global cosp that we pinned in exhibitStimulus */
    /* (*env)->DeleteGlobalRef(env, (jobject)c_CoSP); */

    (*g_JVM)->DetachCurrentThread(g_JVM);

    printf("c_CoServiceProvider_react EXIT\n");
    /* the stack handles this automatically so we can return anything */
    return OC_STACK_KEEP_TRANSACTION;
}

/* EXTERNAL */

/*
 * Class:     org_iochibity_CoServiceProvider
 * Method:    ctorCoSP
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_iochibity_CoServiceProvider_ctorCoSP
(JNIEnv * env, jobject this)
{
    OC_UNUSED(env);
    OC_UNUSED(this);
    printf("%s : %s ENTRY, %ld\n", __FILE__, __func__, (intptr_t)pthread_self());
    /* gtls_CoSP = this; */
    request_out_ctor();
}

/*
 * Class:     org_iochibity_CoServiceProvider
 * Method:    uriPath
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_iochibity_CoServiceProvider_uriPath__
(JNIEnv * env, jobject this)
{
  OC_UNUSED(env);
  OC_UNUSED(this);
  jstring j_uri = (*env)->NewStringUTF(env,
				       RESPONSE_IN->resourceUri);
  return  j_uri;
}

/*
 * Class:     org_iochibity_CoServiceProvider
 * Method:    uriPath
 * Signature: (Ljava/lang/String;)Lorg/iochibity/CoServiceProvider;
 */
JNIEXPORT jobject JNICALL Java_org_iochibity_CoServiceProvider_uriPath__Ljava_lang_String_2
(JNIEnv * env, jobject this, jstring j_uriPath)
{
  printf("Java_org_iochibity_CoServiceProvider_setUriPath ENTRY\n");
  gtls_request_out->requestUri = (*env)->GetStringUTFChars(env, j_uriPath, 0);
  return this;
}

/*
 * Class:     org_iochibity_CoServiceProvider
 * Method:    method
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_iochibity_CoServiceProvider_method__
(JNIEnv * env, jobject this)
{
    OC_UNUSED(env);
    OC_UNUSED(this);
    if (gtls_request_out) {
	return gtls_request_out->method;
    } else {
	/* FIXME: pull method for request_out associated with response_in */
	return -1;
    }
}

/*
 * Class:     org_iochibity_CoServiceProvider
 * Method:    method
 * Signature: (I)Lorg/iochibity/CoServiceProvider;
 */
JNIEXPORT jobject JNICALL Java_org_iochibity_CoServiceProvider_method__I
(JNIEnv * env, jobject this, jint m)
{
    OC_UNUSED(env);
    OC_UNUSED(this);
    OC_UNUSED(m);
    if (gtls_request_out) {
	gtls_request_out->method = m;
    } else {
	/* response_in is read-only? */
    }
  return this;
}

/*
 * Class:     org_iochibity_CoServiceProvider
 * Method:    networkAdapter
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_iochibity_CoServiceProvider_networkAdapter
(JNIEnv * env, jobject this)
{
    OC_UNUSED(env);
    OC_UNUSED(this);
  return -1;
}

JNIEXPORT jint JNICALL Java_org_iochibity_CoServiceProvider_networkFlags
(JNIEnv * env, jobject this)
{
    OC_UNUSED(env);
    OC_UNUSED(this);
    return gtls_request_out->connectivityType;
}

/* **************************************************************** */
jboolean test_network_flag(int flag)
{
    return (bool)(gtls_request_out->connectivityType & flag);
}

void set_network_flag(int flag, jboolean torf)
{
    if (torf) {
	gtls_request_out->connectivityType |= flag;
    } else {
	gtls_request_out->connectivityType &= ~flag;
    }
}
/* **************************************************************** */
/*
 * Class:     org_iochibity_CoServiceProvider
 * Method:    transportIsSecure
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_org_iochibity_CoServiceProvider_transportIsSecure__
(JNIEnv * env, jobject this)
{
    OC_UNUSED(env);
    OC_UNUSED(this);
    return test_network_flag(CT_FLAG_SECURE);
}

/*
 * Class:     org_iochibity_CoServiceProvider
 * Method:    transportIsSecure
 * Signature: (Z)Lorg/iochibity/CoServiceProvider;
 */
JNIEXPORT jobject JNICALL Java_org_iochibity_CoServiceProvider_transportIsSecure__Z
(JNIEnv * env, jobject this, jboolean torf)
{
    OC_UNUSED(env);
    OC_UNUSED(this);
    set_network_flag(CT_FLAG_SECURE, torf);
    return this;
}

/* **************************************************************** */
/*
 * Class:     org_iochibity_CoServiceProvider
 * Method:    transportIsUDP
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_org_iochibity_CoServiceProvider_transportIsUDP__
(JNIEnv * env, jobject this)
{
    OC_UNUSED(env);
    OC_UNUSED(this);
    return test_network_flag(CT_ADAPTER_IP);
}

/*
 * Class:     org_iochibity_CoServiceProvider
 * Method:    transportIsUDP
 * Signature: (Z)Lorg/iochibity/CoServiceProvider;
 */
JNIEXPORT jobject JNICALL Java_org_iochibity_CoServiceProvider_transportIsUDP__Z
(JNIEnv * env, jobject this, jboolean torf)
{
    OC_UNUSED(env);
    OC_UNUSED(this);
    /* printf("%s: ENTRY, torf = %d\n", __func__, torf); */
    set_network_flag(CT_ADAPTER_IP, torf);
    return this;
}

/* **************************************************************** */
/*
 * Class:     org_iochibity_CoServiceProvider
 * Method:    transportIsTCP
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_org_iochibity_CoServiceProvider_transportIsTCP__
(JNIEnv * env, jobject this)
{
    OC_UNUSED(env);
    OC_UNUSED(this);
    return (bool)(gtls_request_out->connectivityType & CT_ADAPTER_TCP);
}


/*
 * Class:     org_iochibity_CoServiceProvider
 * Method:    transportIsTCP
 * Signature: (Z)Lorg/iochibity/CoServiceProvider;
 */
JNIEXPORT jobject JNICALL Java_org_iochibity_CoServiceProvider_transportIsTCP__Z
(JNIEnv * env, jobject this, jboolean torf)
{
    OC_UNUSED(env);
    OC_UNUSED(this);
    if (torf) {
	gtls_request_out->connectivityType |= CT_ADAPTER_TCP;
    } else {
	gtls_request_out->connectivityType &= ~CT_ADAPTER_TCP;
    }
    return this;
}

/* **************************************************************** */
/*
 * Class:     org_iochibity_CoServiceProvider
 * Method:    transportIsGATT
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_org_iochibity_CoServiceProvider_transportIsGATT__
(JNIEnv * env, jobject this)
{
    OC_UNUSED(env);
    OC_UNUSED(this);
    return (bool)(gtls_request_out->connectivityType & CT_ADAPTER_GATT_BTLE);
}

/*
 * Class:     org_iochibity_CoServiceProvider
 * Method:    transportIsGATT
 * Signature: (Z)Lorg/iochibity/CoServiceProvider;
 */
JNIEXPORT jobject JNICALL Java_org_iochibity_CoServiceProvider_transportIsGATT__Z
(JNIEnv * env, jobject this, jboolean torf)
{
    OC_UNUSED(env);
    OC_UNUSED(this);
    if (torf) {
	gtls_request_out->connectivityType |= CT_ADAPTER_GATT_BTLE;
    } else {
	gtls_request_out->connectivityType &= ~CT_ADAPTER_GATT_BTLE;
    }
    return this;
}

/* **************************************************************** */
/*
 * Class:     org_iochibity_CoServiceProvider
 * Method:    transportIsRFCOMM
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_org_iochibity_CoServiceProvider_transportIsRFCOMM__
(JNIEnv * env, jobject this)
{
    OC_UNUSED(env);
    OC_UNUSED(this);
    return (bool)(gtls_request_out->connectivityType & CT_ADAPTER_RFCOMM_BTEDR);
}

/*
 * Class:     org_iochibity_CoServiceProvider
 * Method:    transportIsRFCOMM
 * Signature: (Z)Lorg/iochibity/CoServiceProvider;
 */
JNIEXPORT jobject JNICALL Java_org_iochibity_CoServiceProvider_transportIsRFCOMM__Z
(JNIEnv * env, jobject this, jboolean torf)
{
    OC_UNUSED(env);
    OC_UNUSED(this);
    if (torf) {
	gtls_request_out->connectivityType |= CT_ADAPTER_RFCOMM_BTEDR;
    } else {
	gtls_request_out->connectivityType &= ~CT_ADAPTER_RFCOMM_BTEDR;
    }
    return this;
}

/* **************************************************************** */
/*
 * Class:     org_iochibity_CoServiceProvider
 * Method:    transportIsNFC
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_org_iochibity_CoServiceProvider_transportIsNFC__
(JNIEnv * env, jobject this)
{
    OC_UNUSED(env);
    OC_UNUSED(this);
    return (bool)(gtls_request_out->connectivityType & CT_ADAPTER_NFC);
}

/*
 * Class:     org_iochibity_CoServiceProvider
 * Method:    transportIsNFC
 * Signature: (Z)Lorg/iochibity/CoServiceProvider;
 */
JNIEXPORT jobject JNICALL Java_org_iochibity_CoServiceProvider_transportIsNFC__Z
(JNIEnv * env, jobject this, jboolean torf)
{
    OC_UNUSED(env);
    OC_UNUSED(this);
    if (torf) {
	gtls_request_out->connectivityType |= CT_ADAPTER_NFC;
    } else {
	gtls_request_out->connectivityType &= ~CT_ADAPTER_NFC;
    }
    return this;
}

/* **************************************************************** */
/*
 * Class:     org_iochibity_CoServiceProvider
 * Method:    networkIsIP
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_org_iochibity_CoServiceProvider_networkIsIP__
(JNIEnv * env, jobject this)
{
    OC_UNUSED(env);
    OC_UNUSED(this);
    return (bool)((gtls_request_out->connectivityType & CT_ADAPTER_IP)
		  ||
		  (gtls_request_out->connectivityType & CT_ADAPTER_TCP));
}

/*
 * Class:     org_iochibity_CoServiceProvider
 * Method:    networkIsIP
 * Signature: (Z)Lorg/iochibity/ICoServiceProvider;
 */
JNIEXPORT jobject JNICALL Java_org_iochibity_CoServiceProvider_networkIsIP__Z
(JNIEnv * env, jobject this, jboolean torf)
{
    OC_UNUSED(env);
    OC_UNUSED(this);
    OC_UNUSED(torf);
    // FIXME: this is just a dummy, IP is implied by both
    // xx_ADAPTER_IP (i.e. UDP) and xx_ADAPTER_TCP. The implementation
    // does not currently allow selection of IP independently, only of
    // IP version.
    return this;
}


/* **************************************************************** */
/*
 * Class:     org_iochibity_CoServiceProvider
 * Method:    networkIsIPv4
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_org_iochibity_CoServiceProvider_networkIsIPv4__
(JNIEnv * env, jobject this)
{
    OC_UNUSED(env);
    OC_UNUSED(this);
    return (bool)(gtls_request_out->connectivityType & CT_IP_USE_V4);
}


/*
 * Class:     org_iochibity_CoServiceProvider
 * Method:    networkIsIPv4
 * Signature: (Z)Lorg/iochibity/CoServiceProvider;
 */
JNIEXPORT jobject JNICALL Java_org_iochibity_CoServiceProvider_networkIsIPv4__Z
(JNIEnv * env, jobject this, jboolean torf)
{
    OC_UNUSED(env);
    OC_UNUSED(this);
    if (torf) {
	gtls_request_out->connectivityType |= CT_IP_USE_V4;
    } else {
	gtls_request_out->connectivityType &= ~CT_IP_USE_V4;
    }
    return this;
}

/* **************************************************************** */
/*
 * Class:     org_iochibity_CoServiceProvider
 * Method:    networkIsIPv6
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_org_iochibity_CoServiceProvider_networkIsIPv6__
(JNIEnv * env, jobject this)
{
    OC_UNUSED(env);
    OC_UNUSED(this);
    return (bool)(gtls_request_out->connectivityType & CT_IP_USE_V6);
}

/*
 * Class:     org_iochibity_CoServiceProvider
 * Method:    networkIsIPv6
 * Signature: (Z)Lorg/iochibity/CoServiceProvider;
 */
JNIEXPORT jobject JNICALL Java_org_iochibity_CoServiceProvider_networkIsIPv6__Z
(JNIEnv * env, jobject this, jboolean torf)
{
    OC_UNUSED(env);
    OC_UNUSED(this);
    if (torf) {
	gtls_request_out->connectivityType |= CT_IP_USE_V6;
    } else {
	gtls_request_out->connectivityType &= ~CT_IP_USE_V6;
    }
    return this;
}

/* **************************************************************** */
/*
 * Class:     org_iochibity_CoServiceProvider
 * Method:    scopeIsInterface
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_org_iochibity_CoServiceProvider_scopeIsInterface__
(JNIEnv * env, jobject this)
{
    OC_UNUSED(env);
    OC_UNUSED(this);
    return (bool)(gtls_request_out->connectivityType & CT_SCOPE_INTERFACE);
}

/*
 * Class:     org_iochibity_CoServiceProvider
 * Method:    scopeIsInterface
 * Signature: (Z)Lorg/iochibity/CoServiceProvider;
 */
JNIEXPORT jobject JNICALL Java_org_iochibity_CoServiceProvider_scopeIsInterface__Z
(JNIEnv * env, jobject this, jboolean torf)
{
    OC_UNUSED(env);
    OC_UNUSED(this);
    if (torf) {
	gtls_request_out->connectivityType |= CT_SCOPE_INTERFACE;
    } else {
	gtls_request_out->connectivityType &= ~CT_SCOPE_INTERFACE;
    }
    return this;
}

/* **************************************************************** */
/*
 * Class:     org_iochibity_CoServiceProvider
 * Method:    scopeIsLink
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_org_iochibity_CoServiceProvider_scopeIsLink__
(JNIEnv * env, jobject this)
{
    OC_UNUSED(env);
    OC_UNUSED(this);
    return (bool)(gtls_request_out->connectivityType & CT_SCOPE_LINK);
}

/*
 * Class:     org_iochibity_CoServiceProvider
 * Method:    scopeIsLink
 * Signature: (Z)Lorg/iochibity/CoServiceProvider;
 */
JNIEXPORT jobject JNICALL Java_org_iochibity_CoServiceProvider_scopeIsLink__Z
(JNIEnv * env, jobject this, jboolean torf)
{
    OC_UNUSED(env);
    OC_UNUSED(this);
    if (torf) {
	gtls_request_out->connectivityType |= CT_SCOPE_LINK;
    } else {
	gtls_request_out->connectivityType &= ~CT_SCOPE_LINK;
    }
    return this;
}

/* **************************************************************** */
/*
 * Class:     org_iochibity_CoServiceProvider
 * Method:    scopeIsRealm
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_org_iochibity_CoServiceProvider_scopeIsRealm__
(JNIEnv * env, jobject this)
{
    OC_UNUSED(env);
    OC_UNUSED(this);
    return (bool)(gtls_request_out->connectivityType & CT_SCOPE_REALM);
}

/*
 * Class:     org_iochibity_CoServiceProvider
 * Method:    scopeIsRealm
 * Signature: (Z)Lorg/iochibity/CoServiceProvider;
 */
JNIEXPORT jobject JNICALL Java_org_iochibity_CoServiceProvider_scopeIsRealm__Z
(JNIEnv * env, jobject this, jboolean torf)
{
    OC_UNUSED(env);
    OC_UNUSED(this);
    if (torf) {
	gtls_request_out->connectivityType |= CT_SCOPE_REALM;
    } else {
	gtls_request_out->connectivityType &= ~CT_SCOPE_REALM;
    }
    return this;
}

/* **************************************************************** */
/*
 * Class:     org_iochibity_CoServiceProvider
 * Method:    scopeIsAdmin
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_org_iochibity_CoServiceProvider_scopeIsAdmin__
(JNIEnv * env, jobject this)
{
    OC_UNUSED(env);
    OC_UNUSED(this);
    return (bool)(gtls_request_out->connectivityType & CT_SCOPE_ADMIN);
}

/*
 * Class:     org_iochibity_CoServiceProvider
 * Method:    scopeIsAdmin
 * Signature: (Z)Lorg/iochibity/CoServiceProvider;
 */
JNIEXPORT jobject JNICALL Java_org_iochibity_CoServiceProvider_scopeIsAdmin__Z
(JNIEnv * env, jobject this, jboolean torf)
{
    OC_UNUSED(env);
    OC_UNUSED(this);
    if (torf) {
	gtls_request_out->connectivityType |= CT_SCOPE_ADMIN;
    } else {
	gtls_request_out->connectivityType &= ~CT_SCOPE_ADMIN;
    }
    return this;
}

/* **************************************************************** */
/*
 * Class:     org_iochibity_CoServiceProvider
 * Method:    scopeIsSite
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_org_iochibity_CoServiceProvider_scopeIsSite__
(JNIEnv * env, jobject this)
{
    OC_UNUSED(env);
    OC_UNUSED(this);
    return (bool)(gtls_request_out->connectivityType & CT_SCOPE_SITE);
}

/*
 * Class:     org_iochibity_CoServiceProvider
 * Method:    scopeIsSite
 * Signature: (Z)Lorg/iochibity/CoServiceProvider;
 */
JNIEXPORT jobject JNICALL Java_org_iochibity_CoServiceProvider_scopeIsSite__Z
(JNIEnv * env, jobject this, jboolean torf)
{
    OC_UNUSED(env);
    OC_UNUSED(this);
    if (torf) {
	gtls_request_out->connectivityType |= CT_SCOPE_SITE;
    } else {
	gtls_request_out->connectivityType &= ~CT_SCOPE_SITE;
    }
    return this;
}

/* **************************************************************** */
/*
 * Class:     org_iochibity_CoServiceProvider
 * Method:    scopeIsOrg
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_org_iochibity_CoServiceProvider_scopeIsOrg__
(JNIEnv * env, jobject this)
{
    OC_UNUSED(env);
    OC_UNUSED(this);
    return (bool)(gtls_request_out->connectivityType & CT_SCOPE_ORG);
}

/*
 * Class:     org_iochibity_CoServiceProvider
 * Method:    scopeIsOrg
 * Signature: (Z)Lorg/iochibity/CoServiceProvider;
 */
JNIEXPORT jobject JNICALL Java_org_iochibity_CoServiceProvider_scopeIsOrg__Z
(JNIEnv * env, jobject this, jboolean torf)
{
    OC_UNUSED(env);
    OC_UNUSED(this);
    if (torf) {
	gtls_request_out->connectivityType |= CT_SCOPE_ORG;
    } else {
	gtls_request_out->connectivityType &= ~CT_SCOPE_ORG;
    }
    return this;
}

/* **************************************************************** */
/*
 * Class:     org_iochibity_CoServiceProvider
 * Method:    scopeIsGlobal
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_org_iochibity_CoServiceProvider_scopeIsGlobal__
(JNIEnv * env, jobject this)
{
    OC_UNUSED(env);
    OC_UNUSED(this);
    return (bool)(gtls_request_out->connectivityType & CT_SCOPE_GLOBAL);
}

/*
 * Class:     org_iochibity_CoServiceProvider
 * Method:    scopeIsGlobal
 * Signature: (Z)Lorg/iochibity/CoServiceProvider;
 */
JNIEXPORT jobject JNICALL Java_org_iochibity_CoServiceProvider_scopeIsGlobal__Z
(JNIEnv * env, jobject this, jboolean torf)
{
    OC_UNUSED(env);
    OC_UNUSED(this);
    if (torf) {
	gtls_request_out->connectivityType |= CT_SCOPE_GLOBAL;
    } else {
	gtls_request_out->connectivityType &= ~CT_SCOPE_GLOBAL;
    }
    return this;
}

/* **************************************************************** */
/*
 * Class:     org_iochibity_CoServiceProvider
 * Method:    routingIsMulticast
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_org_iochibity_CoServiceProvider_routingIsMulticast__
(JNIEnv * env, jobject this)
{
    OC_UNUSED(env);
    OC_UNUSED(this);
    if (gtls_request_out->destination) {
	return gtls_request_out->destination->flags & OC_MULTICAST;
    } else {
	return false;
    }
}

/*
 * Class:     org_iochibity_CoServiceProvider
 * Method:    routingIsMulticast
 * Signature: (Z)Lorg/iochibity/CoServiceProvider;
 */
JNIEXPORT jobject JNICALL Java_org_iochibity_CoServiceProvider_routingIsMulticast__Z
(JNIEnv * env, jobject this, jboolean torf)
{
    OC_UNUSED(env);
    OC_UNUSED(this);
    if (gtls_request_out->destination) {
	if (torf) {
	    gtls_request_out->destination->flags
		= gtls_request_out->destination->flags | OC_MULTICAST;
	} else {
	    gtls_request_out->destination->flags &= ~OC_MULTICAST;
	}
    } else {
	/* create new dev addr? */
    }
    return this;
}
/*
 * Class:     org_iochibity_CoServiceProvider
 * Method:    port
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_iochibity_CoServiceProvider_port
  (JNIEnv *, jobject);

/*
 * Class:     org_iochibity_CoServiceProvider
 * Method:    ipAddress
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_iochibity_CoServiceProvider_ipAddress
  (JNIEnv *, jobject);

/*
 * Class:     org_iochibity_CoServiceProvider
 * Method:    qualityOfService
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_iochibity_CoServiceProvider_qualityOfService__
  (JNIEnv *, jobject);

/*
 * Class:     org_iochibity_CoServiceProvider
 * Method:    qualityOfService
 * Signature: (I)Lorg/iochibity/CoServiceProvider;
 */
JNIEXPORT jobject JNICALL Java_org_iochibity_CoServiceProvider_qualityOfService__I
  (JNIEnv *, jobject, jint);

/*
 * Class:     org_iochibity_CoServiceProvider
 * Method:    coAddress
 * Signature: ()Lorg/iochibity/DeviceAddress;
 */
JNIEXPORT jobject JNICALL Java_org_iochibity_CoServiceProvider_coAddress
(JNIEnv * env, jobject this)
{
    OC_UNUSED(env);
    OC_UNUSED(this);
  /* printf("%s ENTRY, thread %ld\n", __func__, (intptr_t)pthread_self()); */
  /* WARNING: we cannot set a devaddr field in our CoSP object, due to
     threading concerns but we can create on and return it on the call
     stack, which is thread safe. */
  /* The methods of the returned DeviceAddress object will pull info
     from the TLS OCClientResponse var */
  if (gtls_response_in) {
    jobject j_CoAddress = NULL;
    j_CoAddress =  (*env)->NewObject(env, K_DEVICE_ADDRESS, MID_DA_CTOR);
    if (j_CoAddress == NULL) {
      THROW_JNI_EXCEPTION("NewObject failed for K_DEVICE_ADDRESS");
      return NULL;
    } else {
      return j_CoAddress;
    }
  } else {
    return NULL;
  }
}

/*
 * Class:     org_iochibity_CoServiceProvider
 * Method:    getCoSecurityId
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_iochibity_CoServiceProvider_getCoSecurityId
(JNIEnv * env, jobject this)
{
    OC_UNUSED(env);
    OC_UNUSED(this);
  printf("%s ENTRY\n", __func__);
  size_t length; /* this will hold the length of string copied */
  char buffer[MAX_IDENTITY_SIZE]; /* fast mem on stack */
  length = strlcpy(buffer, (char*) RESPONSE_IN->identity.id, RESPONSE_IN->identity.id_length);
  if (length < sizeof(buffer)) {
    /* printf("CoSecurityID len %d, str: %s\n", */
    /* 	   RESPONSE_IN->identity.id_length, buffer); */
    jstring sid = (*env)->NewStringUTF(env, buffer);
    // FIXME: free sid? no need since we're returning it to jvm
    return sid;
  } else {
    /* printf("CoSecurityID: ERROR\n"); */
    THROW_JNI_EXCEPTION("URI too long!");
    return NULL;
  }
}

/*
 * Class:     org_iochibity_CoServiceProvider
 * Method:    getCoResult
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_iochibity_CoServiceProvider_getCoResult
(JNIEnv * env, jobject this)
{
    OC_UNUSED(env);
    OC_UNUSED(this);
  /* printf("%s ENTRY\n", __func__); */
  return gtls_response_in->response->result;
}

/*
 * Class:     org_iochibity_CoServiceProvider
 * Method:    getObservationSerial
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_iochibity_CoServiceProvider_getObservationSerial
(JNIEnv * env, jobject this)
{
    OC_UNUSED(env);
    OC_UNUSED(this);
    if (gtls_response_in) {
	return RESPONSE_IN->sequenceNumber;
    } else {
	return -1;
    }
}

/*
 * Class:     org_iochibity_CoServiceProvider
 * Method:    exhibit
 * Signature: ()V
 */
/**
 * @brief Wrapper for `OCDoResource`; called by called by the user to
 * send a request to a server.
 *
 * It wraps `OCDoResource`.  The parameters to `OCDoResource` are
 * reified in a hidden request_out_t struct.
 *
 * @param [in] env JNIEnv pointer
 *
 * @param [in] this_CoSP reference to the calling `CoServiceProvider`
 * instance
 *
 * @return void
 *
 * @throws org.iochibity.exceptions.JNIRuntimeException Indicates a
 * bad result from a JNI API routine (e.g. `GetIntField`)
 *
 * @see observe_stimulus
 * @see Java_org_iochibity_ServiceProvider_exhibitBehavior
 * @see c_CoServiceProvider_react
 */
JNIEXPORT void JNICALL Java_org_iochibity_CoServiceProvider_exhibit
/* JNIEXPORT void JNICALL Java_org_iochibity_CoServiceProvider_exhibit */
(JNIEnv * env, jobject this_CoSP)
{
    OC_UNUSED(env);
    printf("Java_org_iochibity_CoServiceProvider_exhibit ENTRY\n");

    /* We will pass this CoSP object as the callbackParam to
       OCDoResource.  It will be passed back on a different thread, so
       after we call OCDoResource, we will store our thread-local
       request_out struct in the global map, so it can later be
       retrieved for the matching response.  We already have our
       thread-local request_out (created by ctor), so we can use it
       directly to parameterize OCDoReource.
    */

       /* gtls_CoSP = (*env)->NewGlobalRef(env, this_CoSP); */

    if (gtls_response_in == NULL) {

      /* ADDRESSING */
      /* MULTICAST: dev addr is null, connectivity type is used */
      /* UNICAST:   dev addr is used, connectivity type is ignored? */

      /* FIXME: use case: we're using a predefined address */
      printf("MULTICASTING\n");
      if (gtls_request_out->destination->flags & OC_MULTICAST) {
      /* if (gtls_request_out->method == OC_REST_GET) { */
	  gtls_request_out->method = OC_REST_DISCOVER;
	  gtls_request_out->destination = NULL;
      }
      OCStackResult ret;
      OCCallbackData cbData;
      cbData.cb = c_org_iochibity_CoServiceProvider_react;
      cbData.context = (void*)(long)this_CoSP;
      cbData.cd = NULL;
      OCDoHandle c_handle = NULL;
      ret = OCDoResource(&c_handle,	/* OCDoHandle = void* */
			 gtls_request_out->method,      // (OCMethod)c_method,
			 gtls_request_out->requestUri,  // c_uri,
			 gtls_request_out->destination, // c_destDevAddr,
			 gtls_request_out->payload,     // OCPayload* payload
			 gtls_request_out->connectivityType,
			 /* CT_ADAPTER_IP */
			 /* CT_FLAG_SECURE,	 /\* OCConnectivityType conn_type *\/ */
			 OC_LOW_QOS,
			 &cbData,	/* OCCallbackData* cbData */
			 NULL,	/* OCHeaderOptions* options */
			 0);	/* uint8_t numOptions */
      gtls_request_out->handle = c_handle;

      /* now copy tls request_out to the global list, so that we can
       * correlate with response_in in the react routine. we cannot
       * just refer to the TLS data since it will have gone away
       * if/when the thread died. */
      /* why not just use the global list directly, no tls? because we
	 don't have a key (handle) until we call OCDoResource. */
      request_out_t* rqst	= OICCalloc(sizeof (request_out_t), 1);
      rqst->handle		= gtls_request_out->handle;
      rqst->method		= gtls_request_out->method;
      size_t length; /* this will hold the length of string copied */
      char buffer[MAX_URI_LENGTH]; /* fast mem on stack */
      length			= strlcpy(buffer,
					  gtls_request_out->requestUri,
					  sizeof(buffer));
      if (length < sizeof(buffer)) {
	rqst->requestUri	= buffer;	/* it fit, use the stack buffer */
      } else {
	THROW_JNI_EXCEPTION("URI too long!");
      }
      // FIXME: verify the ptr stuff is heap-allocated
      rqst->destination = gtls_request_out->destination;
      rqst->payload = gtls_request_out->payload;
      rqst->connectivityType = gtls_request_out->connectivityType;
      rqst->qos = gtls_request_out->qos;
      rqst->cbData = gtls_request_out->cbData;
      rqst->options = gtls_request_out->options;
      rqst->numOptions = gtls_request_out->numOptions;

    } else {

      /* we have an OCClientResponse - this CoSP is interacting with an SP */

      /* But we're on an exhibit thread; we have a gtls_request_out,
       * but the args we need for OCDoResource are on a react thread,
       * in gtls_response_in
       */

      /* FIXME: pull method from the dual react thread */
      /* TEMPORARY: pull from java object: */
      /* OCMethod c_method = (OCMethod)(*env)->GetIntField(env, */
      /* 							this_CoSP, */
      /* 							FID_COSP_METHOD); */
      /* if (c_method == 0) { */
      /* 	THROW_JNI_EXCEPTION("Method OC_REST_NOMETHOD (0) not allowed"); */
      /* 	return; */
      /* } */
      /* uri */
      /* For thread safety, we must use the Uri from the response */
      /* char* c_uri gtls_response_in->resourceUri, NULL); */

      /* if (c_uri == NULL) { */
      /* 	THROW_JNI_EXCEPTION("No resourceUri in gtls_response_in\n"); */
      /* 	return; */
      /* } */

      /* ADDRESSING */
      /* MULTICAST: dev addr is null, connectivity type is used */
      /* UNICAST:   dev addr is used, connectivity type is ignored? */

      OCDevAddr* c_destDevAddr = NULL;
      /* jobject j_destDevAddr = NULL; */
      /* j_destDevAddr = (*env)->GetObjectField(env, gtls_CoSP, FID_COSP_DESTINATION); */

      printf("UNICASTING\n");
      /* we have an OCDevAddr from a response */
      c_destDevAddr = &(gtls_response_in->response->devAddr);

      OCStackResult ret;
      OCCallbackData cbData;
      cbData.cb = c_org_iochibity_CoServiceProvider_react;
      cbData.context = (void*)(long)gtls_CoSP;
      cbData.cd = NULL;
      OCDoHandle c_handle = NULL;
      /* ret = OCDoResource(&c_handle,	/\* OCDoHandle = void* *\/ */
      /* 			 (OCMethod)c_method, */
      /* 			 c_uri, */
      /* 			 c_destDevAddr, */
      /* 			 NULL,		 /\* OCPayload* payload *\/ */
      /* 			 CT_DEFAULT, */
      /* 			 /\* CT_ADAPTER_IP *\/ */
      /* 			 /\* CT_FLAG_SECURE,	 /\\* OCConnectivityType conn_type *\\/ *\/ */
      /* 			 OC_LOW_QOS, */
      /* 			 &cbData,	/\* OCCallbackData* cbData *\/ */
      /* 			 NULL,	/\* OCHeaderOptions* options *\/ */
      /* 			 0);	/\* uint8_t numOptions *\/ */

      /* now update the handle field */
      /* FIXME: thread safety */
      (*env)->SetLongField(env, gtls_CoSP, FID_COSP_HANDLE, (intptr_t)c_handle);
    }

    printf("Java_org_iochibity_CoServiceProvider_exhibit EXIT\n");
    return;
}

/* /\* */
/*  * Class:     org_iochibity_CoServiceProvider */
/*  * Method:    getStimulusOut */
/*  * Signature: ()Lorg/iochibity/StimulusOut; */
/*  *\/ */
/* JNIEXPORT jobject JNICALL Java_org_iochibity_CoServiceProvider_getStimulusOut */
/* (JNIEnv * env, jobject this_CoSP) */
/* { */
/*     printf("Java_org_iochibity_CoServiceProvider_getStimulusOut ENTRY\n"); */

/*     jobject j_StimulusOut = (*env)->NewObject(env, K_MSG_REQUEST_OUT, */
/* 					      MID_RQO_CTOR, this_CoSP); */

/*     /\* set uri *\/ */
/*     jstring j_uriPath  = (*env)->GetObjectField(env, this_CoSP, FID_COSP_URI_PATH); */
/*     if (j_uriPath == NULL) { */
/* 	THROW_JNI_EXCEPTION("GetObjectField failed for FID_COSP_URI_PATH on j_StimulusOut\n"); */
/* 	return NULL; */
/*     } */
/*     (*env)->SetObjectField(env, j_StimulusOut, FID_RQO_URI_PATH, j_uriPath); */

/*     /\* set method *\/ */
/*     int c_Method = (int)(*env)->GetIntField(env, this_CoSP, FID_COSP_METHOD); */
/*     if (c_Method == 0) { */
/* 	THROW_JNI_EXCEPTION("OC_REST_NOMETHOD DISALLOWED"); */
/* 	return NULL; */
/*     } */
/*     /\* (*env)->SetObjectField(env, j_StimulusOut, FID_RQO_METHOD, c_Method); *\/ */
/*     (*env)->SetIntField(env, j_StimulusOut,  FID_RQO_METHOD, c_Method); */

/*     /\* set dest device address *\/ */
/*     jobject j_Destination = (*env)->GetObjectField(env, this_CoSP, FID_COSP_DESTINATION); */
/*     if (j_Destination == NULL) { */
/*     	printf("NULL DEST: MULTICAST\n"); */
/*     } else { */
/*     	printf("Setting Unicast destination\n"); */
/* 	(*env)->SetObjectField(env, j_StimulusOut, FID_RQO_DEST, j_Destination); */
/*     } */

/*     /\* set payload *\/ */

/*     /\* set callback (CoServiceProvider, i.e. this_CoSP) *\/ */
/*     (*env)->SetObjectField(env, j_StimulusOut, FID_RQO_CO_SERVICE_PROVIDER, this_CoSP); */


/*     /\* set context *\/ */

/*     /\* set context deleter *\/ */

/*     /\* set header options *\/ */

/*     return j_StimulusOut; */
/* } */