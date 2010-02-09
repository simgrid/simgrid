#ifndef RB_MSG_HOST
#define RB_MSG_HOST

#include <ruby.h>
#include "msg/msg.h"

// Free Method
static void host_free(m_host_t ht);

// New Method
static VALUE host_get_by_name(VALUE Class, VALUE name);

//Get Name
static VALUE host_name(VALUE Class,VALUE host);

//Get Number
static VALUE host_number(VALUE Class);

// get Speed
static VALUE host_speed(VALUE Class,VALUE host);

// Set Data
static void host_set_data(VALUE Class,VALUE host,VALUE data);

// Get Data
static VALUE host_get_data( VALUE Class,VALUE host);


//is Available
static VALUE host_is_avail(VALUE Class,VALUE host);

#endif