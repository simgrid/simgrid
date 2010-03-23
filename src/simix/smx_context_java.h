#ifndef _XBT_CONTEXT_JAVA_H
#define _XBT_CONTEXT_JAVA_H

#include "portable.h"
#include "xbt/misc.h"

#include "private.h"
#include "java/jmsg.h"
#include "java/jmsg_process.h"

SG_BEGIN_DECL()

typedef struct s_smx_ctx_java {
  s_smx_ctx_base_t super;  /* Fields of super implementation */
  jobject jprocess;        /* the java process instance binded with the msg process structure                                                      */
  JNIEnv *jenv;            /* jni interface pointer associated to this thread                                                                                      */
} s_smx_ctx_java_t, *smx_ctx_java_t;

SG_END_DECL()
#endif /* !_XBT_CONTEXT_JAVA_H */
