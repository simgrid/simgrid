#ifndef RB_MSG_PROCESS
#define RB_MSG_PROCESS

#include <ruby.h>
#include <stdio.h>
#include "msg/msg.h"
#include "msg/datatypes.h"

#include "msg/private.h"
#include "msg/mailbox.h"
#include "surf/surfxml_parse.h"
#include "simix/simix.h"
#include "simix/private.h"
#include "xbt/sysdep.h"        
#include "xbt/log.h"
#include "xbt/asserts.h"
#include "rb_msg_host.h"


/**************************************************************************
There are 2 section in This File:

1 - Functions to Manage The Ruby Process  >> Up Call
2 - Functions to Manage The Native Process Simulation Bound >> Down Call

***************************************************************************/
// Init Ruby : To Call Ruby Methods From C

static void initRuby();

/***********************************************

Functions for Ruby Process Management ( Up Call )

Independant Methods

************************************************/

// Get Name
static VALUE process_getName( VALUE ruby_process );

// Get  Process ID
static VALUE process_getID(VALUE ruby_process);

// Get Bind : return the ID of Bind member
static VALUE process_getBind(VALUE ruby_class);

// Set Bind 
static void process_setBind(VALUE ruby_class,long bind);

// isAlive
static VALUE process_isAlive(VALUE ruby_process);

// Kill Process
static void process_kill(VALUE ruby_process);

// join Process
static void process_join( VALUE ruby_process );

// unschedule Process
static void process_unschedule( VALUE ruby_process );

// schedule Process
static void process_schedule( VALUE ruby_process );




/***************************************************

Function for Native Process ( Bound ) Management

Methods Belong to The MSG Module
****************************************************/

// ProcessBind Method ; Process Ruby >> Process C

//friend Method // Not belong to the Class but Called within !!
static m_process_t process_to_native(VALUE ruby_process);

// Binding Process >> Friend Method
static void processBind(VALUE ruby_class,m_process_t process);

// CreateProcess Method
static void processCreate(VALUE Class,VALUE rb_process,VALUE host);

// ProcessSuspend
static void processSuspend(VALUE Class,VALUE ruby_process);

// ProcessResume
static void processResume(VALUE Class,VALUE ruby_process);

//ProcessIsSuspend return Boolean ( Qtrue / Qfalse )
static VALUE  processIsSuspend(VALUE Class,VALUE ruby_process);

//Processkill
static void processKill(VALUE Class,VALUE ruby_process);

//ProcessGetHost
static VALUE processGetHost(VALUE Class,VALUE ruby_process);

//ProcessExit
static void processExit(VALUE Class,VALUE ruby_process); 

#endif