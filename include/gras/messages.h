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
 *  @brief Defining messages and callbacks, and exchanging messages (Communication facility) 
 * 
 * <center><table><tr><td><b>Top</b>    <td> [\ref index]::[\ref GRAS_API]
 *                <tr><td><b>Prev</b>   <td> [\ref GRAS_sock]
 *                <tr><td><b>Next</b>   <td> [\ref GRAS_timer]
 *                <tr><td><b>Down</b>   <td> [\ref GRAS_msg_decl]            </table></center>
 *
 *  There is two way to receive messages in GRAS. The first one is to
 *  register a given function as callback to a given type of messages (see
 *  \ref gras_cb_register and associated section). But you can also
 *  explicitely wait for a given message with the \ref gras_msg_wait
 *  function.
 * 
 *  Usually, both ways are not intended to be mixed of a given type of
 *  messages. But if you do so, it shouldn't trigger any issue.  If the
 *  message arrives when gras_msg_wait is blocked, then it will be routed
 *  to it. If it arrives when before or after gras_msg_wait, it will be
 *  passed to the callback.
 * 
 *  For an example of use, please refer to \ref GRAS_ex_ping.
 * 
 *  @{
 */

/** @defgroup GRAS_msg_decl Message declaration and retrival 
 *  
 * <center><table><tr><td><b>Top</b>    <td> [\ref index]::[\ref GRAS_API]::[\ref GRAS_msg]
 *                <tr><td>   Prev       <td> 
 *                <tr><td><b>Next</b>   <td> [\ref GRAS_msg_cb]               </table></center>
 *
 *  GRAS messages can only accept one type of payload. If you absolutely want to declare a message
 *  able to convey several datatypes, you can always say that it conveys a generic reference (see 
 *  \ref gras_datadesc_ref_generic).
 * 
 *  In order to ease the upgrade of GRAS applications, it is possible to \e version the messages, ie 
 *  to add a version number to the message (by default, the version is set to 0). Any messages of the 
 *  wrong version will be ignored by the applications not providing any specific callback for them.
 *  
 *  This mechanism (stolen from the dynamic loader one) should ensure you to change the semantic of a given
 *  message while still understanding the old one.
 */
/** @{ */  
/** \brief Opaque type */
typedef struct s_gras_msgtype *gras_msgtype_t;

  void gras_msgtype_declare  (const char           *name,
			      gras_datadesc_type_t  payload);
  void gras_msgtype_declare_v(const char           *name,
			      short int             version,
			      gras_datadesc_type_t  payload);

  gras_msgtype_t gras_msgtype_by_name (const char *name);
  gras_msgtype_t gras_msgtype_by_namev(const char *name, short int version);
  gras_msgtype_t gras_msgtype_by_id(int id);

/** @} */  
/** @defgroup GRAS_msg_cb Callback declaration and use
 * 
 * <center><table><tr><td><b>Top</b>    <td> [\ref index]::[\ref GRAS_API]::[\ref GRAS_msg]
 *                <tr><td><b>Prev</b>   <td> [\ref GRAS_msg_decl]
 *                <tr><td><b>Next</b>   <td> [\ref GRAS_msg_exchange]       </table></center>
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
  typedef int (*gras_msg_cb_t)(gras_socket_t  expeditor,
			       void          *payload);

  void gras_cb_register  (gras_msgtype_t msgtype, gras_msg_cb_t cb);
  void gras_cb_unregister(gras_msgtype_t msgtype, gras_msg_cb_t cb);

/** @} */  
/** @defgroup GRAS_msg_exchange Message exchange 
 *
 * <center><table><tr><td><b>Top</b>    <td> [\ref index]::[\ref GRAS_API]::[\ref GRAS_msg]
 *                <tr><td><b>Prev</b>   <td> [\ref GRAS_msg_cb]
 *                <tr><td>   Next       <td>                        </table></center>
 */
/** @{ */

  void gras_msg_send(gras_socket_t   sock,
		     gras_msgtype_t  msgtype,
		     void           *payload);
  void gras_msg_wait(double          timeout,    
		     gras_msgtype_t  msgt_want,
		     gras_socket_t  *expeditor,
		     void           *payload);
  void gras_msg_handle(double timeOut);

/* @} */
/* @} */

SG_END_DECL()

#endif /* GRAS_MSG_H */

