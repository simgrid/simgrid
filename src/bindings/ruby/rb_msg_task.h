#ifndef RB_MSG_TASK_H
#define RB_MSG_TASK_H

#include <ruby.h>
#include <stdio.h>
#include "msg/msg.h"
#include "msg/datatypes.h"
#include "xbt/sysdep.h"        
#include "xbt/log.h"
#include "xbt/asserts.h"

typedef enum {
  PORT_22 = 0,
  MAX_CHANNEL
} channel_t;

// Free Method
static void task_free(m_task_t tk);

// New Method  >>> Data NULL
static VALUE task_new(VALUE Class, VALUE name,VALUE comp_size,VALUE comm_size);

//Get Computation Size
static VALUE task_comp(VALUE Class,VALUE task);

//Get Name
static VALUE task_name(VALUE Class,VALUE task);

// Execute Task
static VALUE task_execute(VALUE Class,VALUE task);

// Sending Task 
static void task_send(VALUE Class,VALUE task,VALUE mailbox);

// Recieve : return a task
static VALUE task_receive(VALUE Class,VALUE mailbox);

// Recieve Task 2 <<  Not Appreciated 
static void task_receive2(VALUE Class,VALUE task,VALUE mailbox);

// Get Sender
static VALUE task_sender(VALUE Class,VALUE task);

// Get Source
static VALUE task_source(VALUE Class,VALUE task);

//Listen From Alias
static VALUE task_listen(VALUE Class,VALUE task,VALUE alias);

//Listen from Host
static VALUE task_listen_host(VALUE Class,VALUE task,VALUE alias,VALUE host);

// put
static void task_put(VALUE Class,VALUE task,VALUE host);

//get
static VALUE task_get(VALUE Class);
#endif