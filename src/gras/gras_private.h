/* $Id$ */

/* gras_private.h - GRAS private definitions                                */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003 the OURAGAN project.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_PRIVATE_H
#define GRAS_PRIVATE_H

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Oli's macro */
#ifndef GS_FAILURE_CONTEXT
#  define GS_FAILURE_CONTEXT
#endif /* FAILURE_CONTEXT */

#define GS_FAILURE(str) \
     (fprintf(stderr, "FAILURE: %s(%s:%d)" GS_FAILURE_CONTEXT "%s\n", __func__, __FILE__, __LINE__, (str)), \
      abort())

#define aligned(v, a) (((v) + (a - 1)) & ~(a - 1))
#define max(a, b) (((a) > (b))?(a):(b))
#define min(a, b) (((a) < (b))?(a):(b))

/* end of Oli's cruft */

#include "gras_config.h"

#include "gras/error.h"
#include "gras/log.h"
#include "gras/dynar.h"
#include "gras/dict.h"
#include "gras/config.h"

#include "gras/data_description.h"
#include "gras/dd_type_bag.h"

#include "gras/core.h"
#include "gras/datadesc.h"
#include "gras/socket.h"
#include "gras/messages.h"

#define TRUE  1
#define FALSE 0

#define ASSERT(cond,msg) do {if (!(cond)) { fprintf(stderr,msg); abort(); }} while(0)

/* **************************************************************************
 * Locking system
 ****************************************************************************/
/**
 * gras_lock:
 * @Returns: 1 if succesfull 0 otherwise.
 *
 * Get the GRAS lock.
 */
int
gras_lock(void);

/**
 * gras_unlock:
 * @Returns: 1 if succesfull 0 otherwise.
 *
 * release the GRAS general lock.
 */
int
gras_unlock(void);

/* **************************************************************************
 * Messaging stuff
 * **************************************************************************/

/**
 * gras_cblist_t:
 *
 * The list of callbacks for a given message type on a given host
 */
typedef struct {
  gras_msgid_t id;     /** identificator of this message */
  
  int cbCount;         /** number of registered callbacks */
  gras_cb_t *cb;       /** callbacks */
  int *cbTTL;          /** TTL of each callback (in number of use)*/
} gras_cblist_t;

/**
 * gras_msgentry_t:
 *
 * An entry in the registered message list.
 */

struct gras_msgentry_s {
  gras_msgid_t id;     /** identificator of this message */
  char *name;               /** printable name of this message */

  int seqCount;             /** number of sequence for this message */
  DataDescriptor **dd;      /** list of datadescriptor for each sequence */
  size_t *ddCount;          /** list of datadescriptor for each sequence */
  unsigned int *networkSize;/** network size of one element in each sequence */
  unsigned int *hostSize;   /** host size of one element in each sequence */
};
/* **************************************************************************
 * GRAS globals
 * **************************************************************************/

/** 
 * gras_hostglobal_t:
 * 
 * Globals for a given host. 
 */
typedef struct {
  gras_cblist_t *grasCbList; /** callbacks for registered messages */
  unsigned int grasCbListCount;      /** length of previous array */
} gras_hostglobal_t;

/**
 * gras_msgheader_t:
 *
 * A header sent with messages.  #version# is the NWS version and is presently
 * ignored, but it could be used for compatibility.  #message# is the actual
 * message. #seqCount# is the number of sequence accompagning this message.
 */
struct gras_msgheader_s {
  char          version[10];
  gras_msgid_t  message;
  unsigned int  dataSize;
  unsigned int  seqCount;
};

/**
 * grasProcessData_t:
 *
 * Data for each process 
 */
typedef struct {
  /* queue of messages which where received but not wanted in msgWait, and therefore
     temporarly queued until the next msgHandle */
  int grasMsgQueueLen;
  gras_msg_t **grasMsgQueue;

  /* registered callbacks for each message */
  int grasCblListLen;
  gras_cblist_t *grasCblList;

  /* The channel we are listening to in SG for formated messages */
  int chan;
  /* The channel we are listening to in SG for raw send/recv */
  int rawChan; 

  /* globals of the process */
  void *userdata;               
} grasProcessData_t;


/*@unused@*/static const DataDescriptor headerDescriptor[] =
  {SIMPLE_MEMBER(CHAR_TYPE, 10, offsetof(gras_msgheader_t, version)),
   SIMPLE_MEMBER(UNSIGNED_INT_TYPE, 1, offsetof(gras_msgheader_t, message)),
   SIMPLE_MEMBER(UNSIGNED_INT_TYPE, 1, offsetof(gras_msgheader_t, dataSize)),
   SIMPLE_MEMBER(UNSIGNED_INT_TYPE, 1, offsetof(gras_msgheader_t, seqCount))};
#define headerDescriptorCount 4

/*@unused@*/static const DataDescriptor countDescriptor[] =
  {SIMPLE_DATA(UNSIGNED_INT_TYPE,1)};
#define countDescriptorCount 1

/**
 * GRASVERSION
 *
 * This string is sent in each message to identify the version of the 
 * communication protocol used. This may be paranoid, but I feel so right now.
 */
#define GRASVERSION "0.0.020504"

/**
 * grasMsgEntryGet: 
 * @id: msg id to look for
 * @Returns: the entry if found, NULL otherwise
 * 
 * Get the entry corresponding to this message id;
 */ /*@observer@*/
gras_msgentry_t * grasMsgEntryGet(gras_msgid_t id);

/**
 * grasProcessDataGet: 
 * 
 * Get the GRAS globals for this host
 */ /*@observer@*/
grasProcessData_t *grasProcessDataGet(void);

/**
 * gras_cb_get: 
 * @id: msg id to look for
 * 
 * Get the callback list corresponding to this message id;
 */ /*@observer@*/
gras_cblist_t * gras_cb_get(gras_msgid_t id);

/**
 * gras_cb_create:
 * @id: the id of the new msg
 *
 * Create a new callback list for a new message id.
 */
gras_error_t gras_cb_create(gras_msgid_t message);

/**
 * grasMsgHeaderNew:
 * @msgId: 
 * @dataSize: total size in network format, including headers and seqcount
 * @seqCount: Number of sequences in this message (to check that everything goes well)
 * @Returns: the created header
 *
 * Create a new header containing the passed values.
 */
gras_msgheader_t *grasMsgHeaderNew(gras_msgid_t msgId, 
				  unsigned int dataSize,
				  unsigned int seqCount);

/**
 * grasMsgRecv:
 *
 * Receive the next message arriving within the given timeout
 */
gras_error_t grasMsgRecv(gras_msg_t **msg, double timeout);

/**
 * gras_sock_new:
 *
 * Create an empty socket
 */
gras_sock_t *gras_sock_new(void);

/**
 * grasSockFree:
 *
 * Frees an old socket
 */
void grasSockFree(gras_sock_t *s);



/* **************************************************************************
 * Handling DataDescriptors
 * **************************************************************************/
typedef enum {HOST_FORMAT, NETWORK_FORMAT} FormatTypes;
size_t DataSize(const DataDescriptor *description,
		size_t length,
		FormatTypes format);
void *gras_datadesc_copy_data(const DataDescriptor *dd, unsigned int c, void *data);
int gras_datadesc_cmp(/*@null@*/const DataDescriptor *dd1, unsigned int c1,
		      /*@null@*/const DataDescriptor *dd2, unsigned int c2);
void gras_datadesc_dump(/*@null@*/const DataDescriptor *dd, unsigned int c);

#endif /* GRAS_PRIVATE_H */
