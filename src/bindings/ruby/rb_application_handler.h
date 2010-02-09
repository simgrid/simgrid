#ifndef _RB_APPLICATION_HANDLER_H_
#define _RB_APPLICATION_HANDLER_H_
  
#include <ruby.h>
#include "msg/msg.h"



VALUE current; // The Current Instance of ApplicationHandler Class ,is it the same as Current Thread ??!!

static void  r_init(void);

static void  application_handler_on_start_document(void);

static void  application_handler_on_end_document(void);

static void  application_handler_on_begin_process(void);

static void  application_handler_on_process_arg(void);

static void  application_handler_on_property(void);

static void  application_handler_on_end_process(void);


#endif
