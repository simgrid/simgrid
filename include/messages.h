/* $Id$ */

/* gras/messages.h - Public interface to GRAS messages                      */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003 the OURAGAN project.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */


#ifndef GRAS_MSG_H
#define GRAS_MSG_H

#include <stddef.h>    /* offsetof() */
#include <sys/types.h>  /* size_t */
#include <stdarg.h>


/*! C++ users need love */
#ifndef BEGIN_DECL
# ifdef __cplusplus
#  define BEGIN_DECL extern "C" {
# else
#  define BEGIN_DECL 
# endif
#endif

/*! C++ users need love */
#ifndef END_DECL
# ifdef __cplusplus
#  define END_DECL }
# else
#  define END_DECL 
# endif
#endif
/* End of cruft for C++ */

BEGIN_DECL

typedef unsigned int gras_msgid_t;
typedef struct gras_msgheader_s gras_msgheader_t;
typedef struct gras_msgentry_s  gras_msgentry_t;

/**
 * A message sent or received to/from the network
 *
 * Do not mess with the content of this structure. Only access it through the following functions:
 * @gras_msg_new() or @gras_msg_copy() to create such struct.
 * @gras_msg_free() to get ride of it.
 * @gras_msg_ctn() to read its content.
 *
 */
typedef struct {
  /* public */
  gras_sock_t *sock; /** the socket on which the message was received (to answer) */

  /* private */
  gras_msgheader_t *header;
  gras_msgentry_t *entry;
  unsigned int *dataCount;
  void **data; 
  e_gras_free_directive_t freeDirective;
} gras_msg_t;

/**
 * gras_msgtype_register:
 * @msgId: identificator of such messages
 * @name: name as it should be used for logging messages
 * @sequence_count: number of groups in variadics
 * @Varargs: List of (const DataDescriptor *, int DDcount), describing the 
 *  elements in each sequence (DDlength is the length of the corresponding 
 *  DataDescriptor).
 * @Returns: the error code (or no_error).
 *
 * Registers a message to the GRAS mecanism.
 */
gras_error_t
gras_msgtype_register(gras_msgid_t msgId,
		      const char *name,
		      int sequence_count,
		      ...);

/**
 * gras_msg_new_and_send:
 * @sd: Socket on which the message should be sent
 * @msgId: identificator this messages
 * @Varargs: List of (void **data, int seqLen) forming the payload of the message.
 *  The number of sequences is given by the registration of ht
 * @Returns: the error code (or no_error).
 *
 * Create a new message, and send it on the network through the given socket.
 */
gras_error_t
gras_msg_new_and_send(gras_sock_t *sd,
		      gras_msgid_t msgId,
		      int seqCount,
		      ...);


/**
 * gras_msg_new:
 * @msgId: identificator this messages
 * @free_data_on_free: boolean indicating wheater the data must be freed when the msg get freed.
 * @seqCount: number of sequences in this message (must be the same than the value 
 *  registered using gras_msgtype_register() for that msgId)
 * @Varargs: List of (void **data, int seqLen) forming the payload of the message.
 *  The number of sequences is given by the registration of ht
 * @Returns: the message built or NULL on error
 *
 * Build a message to be sent
 */

gras_msg_t *gras_msg_new(gras_msgid_t msgId,
			 e_gras_free_directive_t free_data_on_free,
			 int seqCount,
			 ...);

/**
 * gras_msg_new_va:
 *
 * Build a new message in the exact same way than gras_msg_new(), but taking its arguments as 
 * variadic ones.
 */
gras_msg_t *gras_msg_new_va(gras_msgid_t msgId,
			e_gras_free_directive_t free_data,
			int seqCount,
			va_list ap);

/**
 * gras_msg_copy:
 * @msg: original to copy. 
 *
 * Copy a message.
 */

gras_msg_t *gras_msg_copy(gras_msg_t *msg);

/**
 * gras_msg_free:
 * @msg: poor guy going to diediedie.
 *
 * Free a msg built with gras_msg_new().
 */
void gras_msg_free(gras_msg_t *msg);

/**
 * gras_msg_ctn:
 * @msg: the carrier of the data
 * @sequence: Sequence in which you want to see the data.
 * @num: Number in this sequence of the element to access.
 * @type: type of the element to access.
 *
 * Access to the content of a message.
 */
#define gras_msg_ctn(msg,sequence,num,type) \
  ((type *)msg->data[sequence])[num]

/**
 * gras_cb_t:
 * @msg: The message itself
 * @Returns: true if the message was accepted by the callback and false if it should be passed to the next one.
 *
 * Type of message callback functions. Once a such a function is registered to 
 * handle messages of a given type with RegisterCallback(), it will be called 
 * each time such a message incomes.
 *
 * If the callback accepts the message, it should free it after use.
 */

typedef int (*gras_cb_t)(gras_msg_t *msg);

/**
 * gras_cb_register:
 * @message: id of the concerned messages
 * @TTL: How many time should this callback be used
 * @cb: The callback.
 * @Returns: the error code (or no_error).
 *
 * Indicates a desire that the function #cb# be called whenever a
 * #message# message comes in.  
 * #TTL# is how many time this callback should be used. After that, this 
 * callback will be unregistred. If <0, the callback will never be unregistered.
 * (ie, it will be permanent)
 */
gras_error_t
gras_cb_register(gras_msgid_t message,
		 int TTL,
		 gras_cb_t cb);

/**
 * gras_msg_handle:
 * @timeOut: How long to wait for incoming messages
 * @Returns: the error code (or no_error).
 *
 * Waits up to #timeOut# seconds to see if a message comes in; if so, calls the
 * registered listener for that message (see RegisterCallback()).
 */
gras_error_t gras_msg_handle(double timeOut);


/**
 * gras_msg_send:
 * @sd: Socket to write on
 * @msg: to send (build it with @gras_msg_new())
 * @freeDirective: if the msg passed as argument should be gras_msg_free'ed after sending.
 * @Returns: the error code (or no_error).
 *
 * Sends the message on the socket sd using an automatic and adaptative timeout.
 */

gras_error_t
gras_msg_send(gras_sock_t *sd,
	      gras_msg_t *msg,
	      e_gras_free_directive_t freeDirective);

/**
 * gras_msg_wait:
 * @timeout: How long should we wait for this message.
 * @id: id of awaited msg
 * @message: where to store the message when it comes.
 * @Returns: the error code (or no_error).
 *
 * Waits for a message to come in over a given socket.
 *
 * Every message of another type received before the one waited will be queued
 * and used by subsequent call to this function or MsgHandle().
 */
gras_error_t
gras_msg_wait(double timeout,
	      gras_msgid_t id,
	      gras_msg_t **message);


END_DECL

#endif /* GRAS_MSG_H */

