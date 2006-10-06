/* $Id$ */

/* messaging - high level communication (send/receive messages)             */
/* module's public interface exported to end user.                          */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_MESSAGES_H
#define GRAS_MESSAGES_H

#include "xbt/misc.h"
#include "gras/transport.h"
#include "gras/datadesc.h"

SG_BEGIN_DECL()

/** @addtogroup GRAS_msg
 *  @brief Defining messages and callbacks, and exchanging messages
 * 
 *  There is two way to receive messages in GRAS. The first one is to
 *  register a given function as callback to a given type of messages (see
 *  \ref gras_cb_register and associated section). But you can also
 *  explicitely wait for a given message with the \ref gras_msg_wait
 *  function.
 * 
 *  Usually, both ways are not intended to be mixed of a given type of
 *  messages. But if you do so, it shouldn't trigger any issue.  If the
 *  message arrives when gras_msg_wait is blocked, then it will be routed to
 *  it. If it arrives when before or after \ref gras_msg_wait, it will be
 *  passed to the callback.
 * 
 *  For an example of use, please refer to \ref GRAS_ex_ping. The archive
 *  contains much more examples, but their are not properly integrated into
 *  this documentation yet. 
 */

/** @defgroup GRAS_msg_decl Message declaration and retrival 
 *  @ingroup  GRAS_msg
 *  
 *  GRAS messages can only accept one type of payload. See \ref GRAS_dd for
 *  more information on how to describe data in GRAS.
 *
 *  If you absolutely want use a message able to convey several datatypes,
 *  you can always say that it conveys a generic reference (see
 *  \ref gras_datadesc_ref_generic).
 * 
 *  In order to ease the upgrade of GRAS applications, it is possible to \e
 *  version the messages, ie to add a version number to the message (by
 *  default, the version is set to 0). Any messages of the wrong version will
 *  be ignored by the applications not providing any specific callback for
 *  them.
 *  
 *  This mechanism (stolen from the dynamic loader one) should ensure you to
 *  change the semantic of a given message while still understanding the old
 *  one.
 */
/** @{ */  
/** \brief Opaque type */
typedef struct s_gras_msgtype *gras_msgtype_t;

  XBT_PUBLIC void gras_msgtype_declare  (const char           *name,
			      gras_datadesc_type_t  payload);
  XBT_PUBLIC void gras_msgtype_declare_v(const char           *name,
			      short int             version,
			      gras_datadesc_type_t  payload);

  XBT_PUBLIC gras_msgtype_t gras_msgtype_by_name (const char *name);
  XBT_PUBLIC gras_msgtype_t gras_msgtype_by_namev(const char *name, short int version);
  XBT_PUBLIC gras_msgtype_t gras_msgtype_by_id(int id);

/** @} */  
/** @defgroup GRAS_msg_cb Callback declaration and use
 *  @ingroup  GRAS_msg
 * 
 *
 * This is how to register a given function so that it gets called when a
 * given type of message arrives.
 * 
 * You can register several callbacks to the same kind of messages, and
 * they will get stacked. The lastly added callback gets the message first.
 * If it consumes the message, it should return a true value when done. If
 * not, it should return 0, and the message will be passed to the second
 * callback of the stack, if any.
 * 
 * @{
 */
 
  /** \brief Context of callbacks (opaque structure) */
  typedef struct s_gras_msg_cb_ctx *gras_msg_cb_ctx_t;

XBT_PUBLIC gras_socket_t gras_msg_cb_ctx_from(gras_msg_cb_ctx_t ctx);
XBT_PUBLIC gras_msg_cb_ctx_t gras_msg_cb_ctx_new(gras_socket_t expe, 
				      gras_msgtype_t msgtype,
				      unsigned long int ID,
				      double timeout);
XBT_PUBLIC void gras_msg_cb_ctx_free(gras_msg_cb_ctx_t ctx) ;
   
   
  /** \brief Type of message callback functions. 
   *
   * \param expeditor: a socket to contact who sent this message
   * \param payload: the message itself
   *
   * \return true if the message was consumed by the callback, 
   *  false if the message was refused by the callback (and should be 
   *  passed to the next callback of the stack for this message)
   *
   * Once a such a function is registered to handle messages of a given
   * type with \ref gras_cb_register(), it will be called each time such
   * a message arrives (unless a gras_msg_wait() intercepts it on arrival).
   *
   * If the callback accepts the message, it should free it after use.
   */
  typedef int (*gras_msg_cb_t)(gras_msg_cb_ctx_t  ctx,
			       void             *payload);

  XBT_PUBLIC void gras_cb_register  (gras_msgtype_t msgtype, gras_msg_cb_t cb);
  XBT_PUBLIC void gras_cb_unregister(gras_msgtype_t msgtype, gras_msg_cb_t cb);

/** @} */  

/** @defgroup GRAS_msg_exchange Message exchange 
 *  @ingroup  GRAS_msg
 *
 */
/** @{ */


  XBT_PUBLIC void gras_msg_send(gras_socket_t   sock,
		     gras_msgtype_t  msgtype,
		     void           *payload);
  XBT_PUBLIC void gras_msg_wait(double          timeout,    
		     gras_msgtype_t  msgt_want,
		     gras_socket_t  *expeditor,
		     void           *payload);
  XBT_PUBLIC void gras_msg_handleall(double period);   
  XBT_PUBLIC void gras_msg_handle(double timeOut);

/** @} */

/** @defgroup GRAS_msg_rpc RPC specific functions
 *  @ingroup  GRAS_msg
 *
 * Remote Procedure Call (RPC) are a classical mecanism to request a service
 * from a remote host. Using this set of functions, you let GRAS doing most of
 * the work of sending the request, wait for an answer, make sure it is the
 * right answer from the right host and so on.  Any exception raised on the
 * server is also passed over the network to the client.
 * 
 * Callbacks are attached to RPC incomming messages the regular way using
 * \ref gras_cb_register.
 * 
 * For an example of use, check the examples/gras/rpc directory of the distribution.
 */
/** @{ */

/* declaration */
XBT_PUBLIC void gras_msgtype_declare_rpc(const char           *name,
			      gras_datadesc_type_t  payload_request,
			      gras_datadesc_type_t  payload_answer);

XBT_PUBLIC void gras_msgtype_declare_rpc_v(const char           *name,
				short int             version,
				gras_datadesc_type_t  payload_request,
				gras_datadesc_type_t  payload_answer);

/* client side */
XBT_PUBLIC void gras_msg_rpccall(gras_socket_t server,
		      double timeOut,
		      gras_msgtype_t msgtype,
		      void *request, void *answer);
XBT_PUBLIC gras_msg_cb_ctx_t
gras_msg_rpc_async_call(gras_socket_t server,
			double timeOut,
			gras_msgtype_t msgtype,
			void *request);
XBT_PUBLIC void gras_msg_rpc_async_wait(gras_msg_cb_ctx_t ctx,
			     void *answer);

/* server side */
XBT_PUBLIC void gras_msg_rpcreturn(double timeOut, gras_msg_cb_ctx_t ctx,void *answer);


/** @} */

/** @defgroup GRAS_msg_exchangeadv Message exchange (advanced interface)
 *  @ingroup  GRAS_msg
 *
 */
/** @{ */

/** @brief Message kind (internal enum) */
typedef enum {
  e_gras_msg_kind_unknown = 0,

  e_gras_msg_kind_oneway=1,    /**< good old regular messages */

  e_gras_msg_kind_rpccall=2,   /**< RPC request */
  /* HACK: e_gras_msg_kind_rpccall also designate RPC message *type* in 
     msgtype_t, not only in msg_t*/
  e_gras_msg_kind_rpcanswer=3, /**< RPC successful answer */
  e_gras_msg_kind_rpcerror=4,  /**< RPC failure on server (payload=exception); should not leak to user-space */

   /* future:
       call cancel, and others
     even after:
       forwarding request and other application level routing stuff
       group communication
  */

  e_gras_msg_kind_count=5 /* sentinel, dont mess with */
} e_gras_msg_kind_t;


/** @brief Message instance (internal struct) */
typedef struct {
    gras_socket_t   expe;
  e_gras_msg_kind_t kind;
    gras_msgtype_t  type;
    unsigned long int ID;
    void           *payl;
    int             payl_size;
} s_gras_msg_t, *gras_msg_t;

typedef int (*gras_msg_filter_t)(gras_msg_t msg,void *ctx);

XBT_PUBLIC void gras_msg_wait_ext(double           timeout,    
		       gras_msgtype_t   msgt_want,
		       gras_socket_t    expe_want,
		       gras_msg_filter_t filter,
		       void             *filter_ctx, 
		       gras_msg_t       msg_got);

XBT_PUBLIC void gras_msg_wait_or(double         timeout,    
		      xbt_dynar_t    msgt_want,
		      gras_msg_cb_ctx_t *ctx,
		      int           *msgt_got,
		      void          *payload);


/* @} */

SG_END_DECL()

#endif /* GRAS_MSG_H */

