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

BEGIN_DECL()

/** @addtogroup GRAS_msg
 *  @brief Defining messages and callbacks, and exchanging messages (Communication facility) 
 *  @{
 */

/** @name Message declaration and retrival 
 *  
 *  GRAS messages can only accept one type of payload. If you absolutely want to declare a message
 *  able to convey several datatypes, you can always say that it conveys a generic reference (see 
 *  \ref gras_datadesc_ref_generic).
 * 
 *  In order to ease the upgrade of GRAS applications, it is possible to \e version the messages, ie 
 *  to add a version number to the message (by default, the version is set to 0). Any messages of the 
 *  wrong version will be ignored by the applications not providing any specific callback for them.
 *  
 *  This mecanism (stolen from the dynamic loader one) should ensure you to change the semantic of a given
 *  message while still understanding the old one.
 */
/** @{ */  
/** \brief Opaque type */
typedef struct s_gras_msgtype *gras_msgtype_t;

/** \brief declare a new message type of the given name. It only accepts the given datadesc as payload */
void gras_msgtype_declare  (const char           *name,
			    gras_datadesc_type_t  payload);
/** \brief declare a new versionned message type of the given name and payload. */
void gras_msgtype_declare_v(const char           *name,
			    short int             version,
			    gras_datadesc_type_t  payload);

/** \brief retrive an existing message type from its name. */
gras_msgtype_t gras_msgtype_by_name (const char     *name);
/** \brief retrive an existing message type from its name and version number. */
gras_msgtype_t gras_msgtype_by_namev(const char     *name,
				     short int       version);
/** @} */  

/** @name Callback declaration and use */
/** @{ */
/** \brief Type of message callback functions. 
 * \param msg: The message itself
 * \return true if the message was consumed by the callback, false if the message was
 * refused by the callback (and should be passed to the next callback of the stack for
 * this message)
 *
 * Once a such a function is registered to handle messages of a given type with 
 * \ref gras_cb_register(), it will be called each time such a message arrives.
 *
 * If the callback accepts the message, it should free it after use.
 */
typedef int (*gras_cb_t)(gras_socket_t  expeditor,
			 void          *payload);
/** \brief Bind the given callback to the given message type 
 *
 * Several callbacks can be attached to a given message type. The lastly added one will get the message first, and 
 * if it returns false, the message will be passed to the second one. 
 * And so on until one of the callbacks accepts the message.
 */
void gras_cb_register  (gras_msgtype_t msgtype,
			gras_cb_t      cb);
/** \brief Unbind the given callback from the given message type */
void gras_cb_unregister(gras_msgtype_t msgtype,
			gras_cb_t      cb);

/** @} */  
/** @name Message exchange */
/** @{ */
/** \brief Send the data pointed by \a payload as a message of type \a msgtype to the peer \a sock */
xbt_error_t gras_msg_send(gras_socket_t   sock,
			   gras_msgtype_t  msgtype,
			   void           *payload);
/** \brief Waits for a message to come in over a given socket. */
xbt_error_t gras_msg_wait(double          timeout,    
			   gras_msgtype_t  msgt_want,
			   gras_socket_t  *expeditor,
			   void           *payload);
xbt_error_t gras_msg_handle(double timeOut);

/*@}*/

END_DECL()

#endif /* GRAS_MSG_H */

