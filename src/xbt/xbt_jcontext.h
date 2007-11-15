#ifndef _XBT_JCONTEXT_H
#define _XBT_JCONTEXT_H

#include <jni.h>					/* use java native interface to bind the msg structures to the java instances				*/	
#include "portable.h"
#include "xbt/misc.h"

#ifndef _XBT_CONTEXT_PRIVATE_H
#include "xbt_context_private.h"
#endif /* _XBT_CONTEXT_PRIVATE_H */

SG_BEGIN_DECL()

#ifndef _XBT_CONTEXT_FACTORY_T_DEFINED
typedef struct s_xbt_context_factory* xbt_context_factory_t;
#define _XBT_CONTEXT_FACTORY_T_DEFINED
#endif /* !_XBT_CONTEXT_FACTORY_T_DEFINED */

typedef struct s_xbt_jcontext
{
	XBT_CTX_BASE_T;
   	jobject jprocess;  				/* the java process instance binded with the msg process structure							*/
	JNIEnv* jenv;	  				/* jni interface pointer associated to this thread											*/
}s_xbt_jcontext_t,* xbt_jcontext_t;

int
xbt_jcontext_factory_init(xbt_context_factory_t* factory);


SG_END_DECL()




#endif /* !_XBT_JCONTEXT_H */
