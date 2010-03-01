/*
 * Copyright 2010. The SimGrid Team. All right reserved.
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 *(GNU LGPL) which comes with this package. 
 */
#include "bindings/ruby_bindings.h"

// Free Method
void rb_host_free(m_host_t ht) {
  //Nothing to do !!?
}

// New Method : return a Host
VALUE rb_host_get_by_name(VALUE class, VALUE name) {
  
  const char * h_name = RSTRING(name)->ptr;
  m_host_t host = MSG_get_host_by_name(h_name);
  if(!host)
    rb_raise(rb_eRuntimeError,"MSG_get_host_by_name() failled");
  
  return Data_Wrap_Struct(class,0,host_free,host);

}

//Get Name
VALUE rb_host_name(VALUE class,VALUE host) {
  
  // Wrap Ruby Value to m_host_t struct
  m_host_t ht;
  Data_Get_Struct(host, m_host_t, ht);
  return rb_str_new2(MSG_host_get_name(ht));
   
}

// Get Number
VALUE rb_host_number(VALUE class) {
  
  return INT2NUM(MSG_get_host_number());
  
}

// Host Speed ( Double )
VALUE rb_host_speed(VALUE class,VALUE host) {
  m_host_t ht ;
  Data_Get_Struct(host,m_host_t,ht);
  return MSG_get_host_speed(ht);
  
}

// Host Set Data
void rb_host_set_data(VALUE class,VALUE host,VALUE data) {
  //...
}

// Host Get Data
VALUE rb_host_get_data(VALUE class,VALUE host) {
  //...
  return Qnil;
}

// Host is Avail
VALUE rb_host_is_avail(VALUE class,VALUE host) {
  m_host_t ht;
  Data_Get_Struct(host,m_host_t,ht);
  if (!ht) {
    rb_raise(rb_eRuntimeError,"Host not Bound");
    return Qnil;
  }
  
  if(MSG_host_is_avail(ht))
    return Qtrue;
  
  return Qfalse;
}
