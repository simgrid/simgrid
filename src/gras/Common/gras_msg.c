/* $Id$ */

/* grasMsg - Function related to messaging (code shared between RL and SG)  */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003 the OURAGAN project.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras_private.h"
#include <string.h>

/*@null@*/static gras_msgentry_t *grasMsgList = NULL;
static unsigned int grasMsgCount = 0;

/**
 * Register a new message type to the system
 */

gras_error_t
gras_msgtype_register(gras_msgid_t message,
	    const char *name,
	    int sequence_count,
	    ...) {
  gras_msgentry_t *entry=grasMsgEntryGet(message);
  gras_cblist_t *cbl=gras_cb_get(message);
  int i;
  DataDescriptor *dd;
  size_t ddCount;
  va_list ap;

  //  fprintf(stderr,"Register message '%s' under ID %d. Sequence count=%d\n",
  //name,(int)message,sequence_count);

  if (entry) { /* Check that it's the same entry re-registered */
    if (strcmp(name,entry->name)) {
      fprintf(stderr,"Second registration of message %d with another name. (old=%s,new=%s)\n",
	      (int)message,entry->name,name);
      return malloc_error;
    }
    if (sequence_count != entry->seqCount) {
      fprintf(stderr,
	      "Second registration of message %s with another sequence count. (old=%d,new=%d)\n",
	      entry->name,entry->seqCount,sequence_count);
      return mismatch_error;
    }

    va_start(ap, sequence_count);
    for (i=0;i<sequence_count;i++) {
      dd=va_arg(ap, DataDescriptor*);
      ddCount=va_arg(ap, size_t);
      if (ddCount != entry->ddCount[i]) {
	fprintf(stderr,
		"Different re-registration of message %s: DataDescriptor count is different in sequence %d (is %d, was %d)\n",
		entry->name, i, ddCount, entry->ddCount[i]);
	return sanity_error;
      }
      if (gras_datadesc_cmp(dd,ddCount, entry->dd[i],ddCount)) {
	fprintf(stderr,
		"Different re-registration of message %s: DataDescriptor of sequence %d is different\n",
		entry->name, i);
	return sanity_error;
      }
    }
    va_end(ap);

  } else { /* build a new entry */
    if (grasMsgCount++) {
      grasMsgList = (gras_msgentry_t *)realloc(grasMsgList,sizeof(gras_msgentry_t)*grasMsgCount);
    } else {
      grasMsgList = (gras_msgentry_t *)malloc(sizeof(gras_msgentry_t)*grasMsgCount);
    }
    if (!grasMsgList) {
      fprintf(stderr, "PANIC: memory allocation of %d bytes in gras_msgtype_register() failed (Message table LOST).\n",
	      sizeof(gras_msgentry_t)*grasMsgCount);
      grasMsgCount=0;
      return malloc_error;
    }
    entry = &(grasMsgList[grasMsgCount-1]);

    entry->id = message;
    if (!(entry->name = strdup(name))) {
      fprintf(stderr, "gras_msgtype_register: memory allocation failed.\n");
      grasMsgCount--;
      return malloc_error;
    }
    entry->seqCount = sequence_count;
    if (sequence_count) {
      if (!(entry->dd = (DataDescriptor**)malloc(sizeof(DataDescriptor*)*sequence_count))) {
	fprintf(stderr, "gras_msgtype_register: memory allocation of %d bytes failed.\n",
		sizeof(DataDescriptor*)*sequence_count);
	free(entry->name);
	grasMsgCount--;
	return malloc_error;
      }
      if (!(entry->ddCount = (size_t*)malloc(sizeof(size_t)*sequence_count))) {
	fprintf(stderr, "gras_msgtype_register: memory allocation of %d bytes failed.\n",
		sizeof(size_t)*sequence_count);
	free(entry->dd);
	free(entry->name);
	grasMsgCount--;
	return malloc_error;
      }
    } else {
      entry->dd=NULL;
      entry->ddCount=NULL;
    }
    va_start(ap, sequence_count);
    for (i=0;i<sequence_count;i++) {
      dd=va_arg(ap, DataDescriptor*);
      ddCount=va_arg(ap, size_t);

      entry->ddCount[i]=ddCount;
      if (ddCount) {
	if (!(entry->dd[i] = (DataDescriptor*)malloc(sizeof(DataDescriptor)*ddCount))) {
	  fprintf(stderr, "gras_msgtype_register: memory allocation of %d bytes failed.\n",
		  sizeof(DataDescriptor)*ddCount);
	  for (i--;i>=0;i--) free(entry->dd[i]);
	  free(entry->ddCount);
	  free(entry->dd);
	  free(entry->name);
	  grasMsgCount--;
	  return malloc_error;
	}
      } else {
	entry->dd[i]=NULL;
      }
      memcpy(entry->dd[i],dd,sizeof(DataDescriptor)*ddCount);
    }
    va_end(ap);
  }
  if (cbl) {
    fprintf(stderr,"Warning, message type %s registered twice on this host\n",
	    entry->name);
  } else {
    return gras_cb_create(entry->id);
  }

  return no_error;
}

/*
 * Retrieve the entry associated with a message id
 */
gras_msgentry_t *
grasMsgEntryGet(gras_msgid_t id) {
  int i;

  for (i=0 ; i<grasMsgCount && grasMsgList[i].id != id ; i++);
  return i==grasMsgCount ? NULL : &grasMsgList[i];
}

/*
 * Create the appropriate header
 */
gras_msgheader_t *grasMsgHeaderNew(gras_msgid_t msgId, 
				  unsigned int dataSize,
				  unsigned int seqCount) {
  gras_msgheader_t *res;
  if (!(res=(gras_msgheader_t*)malloc(sizeof(gras_msgheader_t)))) {
    fprintf(stderr,"gras_msg_new(): Malloc error (wanted %d bytes)\n",sizeof(gras_msgheader_t));
    return NULL;
  }    
  memset(res->version,0,sizeof(res->version));
  strcpy(res->version,GRASVERSION);
  res->message = msgId;
  res->dataSize = dataSize;
  res->seqCount = seqCount;

  return res;
}

gras_msg_t *gras_msg_new_va(gras_msgid_t msgId,
			e_gras_free_directive_t free_data,
			int seqCount,
			va_list ap) {
  gras_msg_t *res;
  int i;
  unsigned int networkSize=0;
   
  /* malloc the needed room, and sanity check */
  if (!(res=(gras_msg_t*)malloc(sizeof(gras_msg_t)))) {
    fprintf(stderr,"gras_msg_new(): Malloc error (wanted %d bytes)\n",sizeof(gras_msg_t));
    return NULL;
  }
  res->freeDirective=free_data;

  if (!(res->entry=grasMsgEntryGet(msgId))) {
    fprintf(stderr,"gras_msg_new(): unknown msg id %d\n",msgId);
    free(res);
    return NULL;
  }
  if (res->entry->seqCount != seqCount) {
    fprintf(stderr,"Damnit: you passed %d sequences to build a %s msg, where %d were expected\n",
	    seqCount,res->entry->name,res->entry->seqCount);
    free(res);
    return NULL;
  }
  if (seqCount) {
    if (!(res->dataCount=(unsigned int*)malloc(sizeof(unsigned int)*seqCount))) {
      fprintf(stderr,"gras_msg_new(): Malloc error (wanted %d bytes)\n",
	      (sizeof(unsigned int)*seqCount));
      free(res);
      return NULL;
    }
    if (!(res->data=(void**)malloc(sizeof(void*)*seqCount))) {
      fprintf(stderr,"gras_msg_new(): Malloc error (wanted %d bytes)\n",
	      (sizeof(void*)*seqCount));
      free(res->dataCount);
      free(res);
      return NULL;
    }
  } else {
    res->dataCount = NULL;
    res->data = NULL;
  }
  
  /* populate the message */
  networkSize += DataSize(headerDescriptor,headerDescriptorCount,NETWORK_FORMAT);
  networkSize += DataSize(countDescriptor,countDescriptorCount,NETWORK_FORMAT) * seqCount;

  for (i=0; i<seqCount; i++) {
    res->data[i]=va_arg(ap, void*);
    res->dataCount[i]=va_arg(ap, int);
    if (res->dataCount[i] > 1000) {
      fprintf(stderr,"GRAS WARNING: datacount>1000 in a message. You may want to check the arguments passed to gras_msg_new().\n");
    }
    if (res->dataCount[i] < 0) {
      fprintf(stderr,"GRAS ERROR: datacount<0 in a message. Check the arguments passed to gras_msg_new().\n");
      free(res->dataCount);
      free(res->data);
      free(res);
      return NULL;
    }

    networkSize += res->dataCount[i] * 
      DataSize(res->entry->dd[i],res->entry->ddCount[i],NETWORK_FORMAT);
  }

  /* finish filling the fields */
  if (!(res->header=grasMsgHeaderNew(msgId,networkSize,seqCount))) {
    free(res->data);
    free(res->dataCount);
    free(res);
    return NULL;
  }
  res->sock=NULL;
  return res;
}

gras_msg_t *gras_msg_new(gras_msgid_t msgId,
		      e_gras_free_directive_t free_data,
		      int seqCount,
		      ...) {
  gras_msg_t *res;
  va_list ap;

  va_start(ap, seqCount);
  res=gras_msg_new_va(msgId,free_data,seqCount,ap);
  va_end(ap);

  return res;
}

gras_error_t
gras_msg_new_and_send(gras_sock_t *sd,
	       gras_msgid_t msgId,
	       int seqCount,
	       ...) {

  gras_msg_t *msg;
  va_list ap;

  va_start(ap, seqCount);
  msg=gras_msg_new_va(msgId,free_after_use,seqCount,ap);
  va_end(ap);
  if (!msg) return unknown_error;
  
  return gras_msg_send(sd,msg,free_after_use);
}


gras_msg_t *gras_msg_copy(gras_msg_t *msg) {
  gras_msg_t *res;
  int i;

  fprintf(stderr,"gras_msg_copy: \n");

  /* malloc the needed room, and sanity check */
  if (!(res=(gras_msg_t*)malloc(sizeof(gras_msg_t)))) {
    fprintf(stderr,"gras_msg_new(): Malloc error (wanted %d bytes)\n",sizeof(gras_msg_t));
    return NULL;
  }
  res->freeDirective=free_after_use;
  res->entry=msg->entry;

  if (!(res->dataCount=(unsigned int*)malloc(sizeof(unsigned int)*res->entry->seqCount))) {
    fprintf(stderr,"gras_msg_new(): Malloc error (wanted %d bytes)\n",
	    (sizeof(unsigned int)*res->entry->seqCount));
    free(res);
    return NULL;
  }
  if (!(res->data=(void**)malloc(sizeof(void*)*res->entry->seqCount))) {
    fprintf(stderr,"gras_msg_new(): Malloc error (wanted %d bytes)\n",
	    (sizeof(void*)*res->entry->seqCount));
    free(res->dataCount);
    free(res);
    return NULL;
  }
  
  /* populate the message */
  for (i=0; i<res->entry->seqCount; i++) {
    res->data[i]= gras_datadesc_copy_data(msg->entry->dd[i],msg->entry->ddCount[i],res->data[i]);
    res->dataCount[i]=msg->dataCount[i];
  }
  
  /* finish filling the fields */
  if (!(res->header=grasMsgHeaderNew(msg->header->message,
				     msg->header->dataSize,
				     msg->header->seqCount))) {
    free(res->data);
    free(res->dataCount);
    free(res);
    return NULL;
  }
  res->sock=msg->sock;

  return res;
}


void gras_msg_free(gras_msg_t *msg) {
  int i;

  if (!msg) return;
  if (msg->freeDirective == free_after_use)
    for (i=0; i<msg->entry->seqCount; i++) 
      free(msg->data[i]);
  gras_sock_close(msg->sock);
  free(msg->header);
  // data isn't copied by MsgNew
  free (msg->data);
  free (msg->dataCount);
  free (msg);
}

gras_error_t gras_msg_handle(double timeOut) {
  grasProcessData_t *pd=grasProcessDataGet();
  int i;
  gras_error_t errcode;
  gras_msg_t *msg;
  gras_cblist_t *cbl;

  if (pd->grasMsgQueueLen) {
    /* handle queued message */

    msg = pd->grasMsgQueue[0];
    memmove(pd->grasMsgQueue[0],pd->grasMsgQueue[1],(pd->grasMsgQueueLen-1)*sizeof(gras_msg_t));
    pd->grasMsgQueueLen--;
    if (pd->grasMsgQueueLen == 0) {
      /* size reached 0. Free the queue so that the next enlargement (with malloc) don't leak*/
      free(pd->grasMsgQueue);
      /* if size!=0 don't loose the time to realloc to only gain 4 bytes */
    }
    fprintf(stderr,"%s:%d: gras_msg_handle: The message was queued\n",__FILE__,__LINE__);
    return no_error;
  } else {
    /* receive a message from the net */
    if ((errcode=grasMsgRecv(&msg,timeOut))) {
      if (errcode == timeout_error) {
	return no_error;
      } else {
	fprintf(stderr,"gras_msg_handle: error '%s' while receiving\n",gras_error_name(errcode));
	return errcode;
      }
    }
  }
  
  /*
  fprintf(stderr,"GRAS: Handle an incomming message '%s' (datasize=%d, sd=%p)\n",
	  msg->entry->name,msg->header->dataSize,msg->sock);
  */  

  if (!(cbl=gras_cb_get(msg->entry->id))) {
    fprintf(stderr,"Message %s is not registered on this host.\n",
	    msg->entry->name);
    gras_msg_free(msg);
    return mismatch_error;
  }
    
  for (i = cbl->cbCount - 1; i>=0 ; i--) {
    if ((*(cbl->cb[i]))(msg)) {
      //      if (cbl->cbTTL[i] > 0 && (!--(cbl->cbTTL[i]))) {
      //fprintf(stderr,"GRAS FIXME: Remove the callback from the queue after use if needed.\n");
      //}
      break;
    }
  }

  if (i<0) {
    fprintf(stderr,
	    "No callback of msg type %s accepts this message. Discarding it\n",
	    msg->entry->name);
    gras_msg_free(msg);
    return mismatch_error;
  } 
  return no_error;
}

gras_error_t 
gras_msg_wait(double timeOut,
	    gras_msgid_t id,
	    gras_msg_t **message) {
  int i;
  gras_error_t errcode;
  double start,now;
  gras_msgentry_t *entry=grasMsgEntryGet(id);
  grasProcessData_t *pd=grasProcessDataGet();

  if (!entry) {
    fprintf(stderr,"gras_msg_wait: message id %d is not registered\n",id);
    return mismatch_error;
  }
    
  *message = NULL;
  start=now=gras_time();
  
  for (i=0;i<pd->grasMsgQueueLen;i++) {
    if (pd->grasMsgQueue[i]->header->message == id) {
      *message = pd->grasMsgQueue[i];
      memmove(pd->grasMsgQueue[i],pd->grasMsgQueue[i+1],(pd->grasMsgQueueLen-i-1)*sizeof(gras_msg_t));
      pd->grasMsgQueueLen--;
      if (pd->grasMsgQueueLen == 0) {
	/* size reached 0. Free the queue so that the next enlargement (with malloc) don't leak*/
	free(pd->grasMsgQueue);
	/* if size!=0 don't loose the time to realloc to only gain 4 bytes */
      }
      fprintf(stderr,"%s:%d: gras_msg_wait: The message was queued\n",__FILE__,__LINE__);
      return no_error;
    }
  }

  while (1) {    
    if ((errcode=grasMsgRecv(message,timeOut))) {
      if (errcode != timeout_error)
	fprintf(stderr,"gras_msg_wait: error '%s' while receiving\n",gras_error_name(errcode));
      return errcode;
    }
    
    if ((*message)->header->message != id) {
      fprintf(stderr,"gras_msg_wait: Got message %s while waiting for message %s. Queue it.\n",
	      (*message)->entry->name,entry->name);
      if (pd->grasMsgQueueLen++) {
	pd->grasMsgQueue = (gras_msg_t **)realloc(pd->grasMsgQueue,
						 sizeof(gras_msg_t)*pd->grasMsgQueueLen);
      } else {
	pd->grasMsgQueue = (gras_msg_t **)malloc(sizeof(gras_msg_t)*pd->grasMsgQueueLen);
      }
      if (!pd->grasMsgQueue) {
	fprintf(stderr, "PANIC: memory allocation of %d bytes in gras_msg_wait() failed (Queued messages are LOST).\n",
		sizeof(gras_msg_t)*pd->grasMsgQueueLen);
	pd->grasMsgQueueLen=0;
	return malloc_error;
      }
      pd->grasMsgQueue[pd->grasMsgQueueLen - 1] = *message;
      *message=NULL;
    } else {
      //      fprintf(stderr,"Waited for %s successfully\n",(*message)->entry->name);
      return no_error;
    }
    now=gras_time();
    if (now - start + 0.001 < timeOut)
      return timeout_error;
  }
}


gras_error_t
gras_cb_register(gras_msgid_t message,
		     int TTL,
		     gras_cb_t cb) {

  gras_cblist_t *cbl=gras_cb_get(message);

  if (!cbl) {
    fprintf(stderr,"Try to register a callback for an unregistered message id %d\n",message);
    return sanity_error;
  }
  if (cbl->cbCount++) {
    cbl->cb = (gras_cb_t *)realloc(cbl->cb,
						sizeof(gras_cb_t)*cbl->cbCount);
    cbl->cbTTL = (int *)realloc(cbl->cbTTL, sizeof(int)*cbl->cbCount);
  } else {
    cbl->cb = (gras_cb_t *)malloc(sizeof(gras_cb_t)*cbl->cbCount);
    cbl->cbTTL = (int *)malloc( sizeof(int)*cbl->cbCount);
  }
  if (!cbl->cb || !cbl->cbTTL) {
    fprintf(stderr,"gras_cb_register(): Malloc error (All callbacks for msg %d lost)\n",
	    message);
    cbl->cb=NULL;
    cbl->cbTTL=NULL; /* Yes, leaking here, but we're dead anyway */
    cbl->cbCount=0;
    return malloc_error;
  }
  cbl->cb   [ cbl->cbCount-1 ]=cb;
  cbl->cbTTL[ cbl->cbCount-1 ]=TTL;

  return no_error;
}
