/* $Id$ */

/* base - several addons to do specific stuff not in GRAS itself            */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/error.h"
#include "gras/datadesc.h"
#include "amok/base.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(amok,XBT_LOG_ROOT_CAT,"All AMOK categories");

amok_remoterr_t amok_remoterr_new(xbt_error_t param_errcode, 
				  const char* format,...) {
   
  amok_remoterr_t res;
   
  va_list ap;
  va_start(ap,format);
  res = amok_remoterr_new_va(param_errcode,format,ap);
  va_end(ap);
  return res;
}

amok_remoterr_t amok_remoterr_new_va(xbt_error_t param_errcode, 
				     const char* format,va_list ap) {
  amok_remoterr_t res=xbt_new(s_amok_remoterr_t,1);
  res->code=param_errcode;
  if (format) {
     res->msg=(char*)xbt_malloc(1024);
     vsnprintf(res->msg,1024,format,ap);
  } else {
     res->msg = NULL;
  }
   
  return res;   
}

void amok_remoterr_free(amok_remoterr_t *err) {
   if (err && *err) {
      if ((*err)->msg) free((*err)->msg);
      free(*err);
      err=NULL;
   }
}


void
amok_repport_error (gras_socket_t sock, gras_msgtype_t msgtype,
		    xbt_error_t param_errcode, const char* format,...) {
  amok_remoterr_t error;
  xbt_error_t errcode;
  va_list ap;

  error=xbt_new(s_amok_remoterr_t,1);
  error->code=param_errcode;
  error->msg=(char*)xbt_malloc(1024); /* FIXME */
  va_start(ap,format);
  vsnprintf(error->msg,1024,format,ap);
  va_end(ap);

  errcode = gras_msg_send(sock,msgtype,error);
  if (errcode != no_error) {
     CRITICAL4("Error '%s' while reporting error '%s' to %s:%d",
	       xbt_error_name(errcode),error->msg,
	       gras_socket_peer_name(sock),gras_socket_peer_port(sock) );
  }
}

void amok_base_init(void) {
  gras_datadesc_type_t host_desc, remoterr_desc;
     
  /* Build the datatype descriptions */
  host_desc = gras_datadesc_struct("xbt_host_t");
  gras_datadesc_struct_append(host_desc,"name",gras_datadesc_by_name("string"));
  gras_datadesc_struct_append(host_desc,"exp_size",gras_datadesc_by_name("int"));
  gras_datadesc_struct_close(host_desc);
  host_desc = gras_datadesc_ref("xbt_host_t*",host_desc);
   
  remoterr_desc = gras_datadesc_struct("s_amok_remoterr_t");
  gras_datadesc_struct_append(remoterr_desc,"msg",gras_datadesc_by_name("string"));
  gras_datadesc_struct_append(remoterr_desc,"code",gras_datadesc_by_name("unsigned int"));
  gras_datadesc_struct_close(remoterr_desc);
  remoterr_desc = gras_datadesc_ref("amok_remoterr_t",remoterr_desc);
}

void amok_base_exit(void) {
   /* No real module mecanism in GRAS so far, nothing to do. */
}

