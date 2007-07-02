/* $Id$ */

/* layout_simple - a dumb log layout                                        */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "portable.h" /* execinfo when available */
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "gras/virtu.h"
#include <stdio.h>
#include "xbt/ex_interface.h" /* backtraces */

extern const char *xbt_log_priority_names[7];


static char *xbt_log_layout_format_doit(xbt_log_layout_t l,
					xbt_log_event_t ev, 
					const char *msg_fmt) {
  static char res[2048];
  static double begin_of_time = -1;
  char *p,*q;
  int precision=-1;

  if (begin_of_time<0) 
    begin_of_time=gras_os_time();

  p = res;
  q = l->data;

  while (*q != '\0') {
    if (*q == '%') {
      q++;
       handle_modifier:
      switch (*q) {
      case '\0':
	THROW1(arg_error,0,"Layout format (%s) ending with %%",(char*)l->data);
      case '%':
	*p++ = '%';
	break;
      case 'n': /* platform-dependant line separator (LOG4J compliant) */
	p += sprintf(p,"\n");
	break;
      case 'e': /* plain space (SimGrid extension) */
	p += sprintf(p," ");
	break;
	 
      case '.': /* precision specifyier */
	q++;
	q += sscanf(q,"%d",&precision);
	goto handle_modifier;

      case 'c': /* category name; LOG4J compliant
		   should accept a precision postfix to show the hierarchy */
	if (precision == -1)
	   p += sprintf(p,"%s",ev->cat->name);
        else {	      
	   p += sprintf(p,"%.*s",precision,ev->cat->name);
	   precision = -1;
	}	 
	break;
      case 'p': /* priority name; LOG4J compliant */	
	if (precision == -1)
	   p += sprintf(p, "%s", xbt_log_priority_names[ev->priority] );
        else {	      
	   p += sprintf(p, "%.*s", precision, xbt_log_priority_names[ev->priority] );
	   precision = -1;
	}	 
	break;

      case 'h': /* host name; SimGrid extension */
	if (precision == -1)
	   p += sprintf(p, "%s", gras_os_myname());
        else {	      
	   p += sprintf(p, "%.*s", precision, gras_os_myname());
	   precision = -1;
	}	 
	break;
      case 't': /* process name; LOG4J compliant (thread name) */
	if (precision == -1)
	   p += sprintf(p, "%s", xbt_procname());
        else {	      
	   p += sprintf(p, "%.*s", precision,xbt_procname());
	   precision = -1;
	}	 
	break;
      case 'i': /* process PID name; SimGrid extension */
	if (precision == -1)
	   p += sprintf(p, "%d", (*xbt_getpid)());
        else {	      
	   p += sprintf(p, "%.*d", precision, (*xbt_getpid)());
	   precision = -1;
	}	 
	break;

      case 'F': /* file name; LOG4J compliant */
	if (precision == -1)
	   p += sprintf(p,"%s",ev->fileName);
        else {	      
	   p += sprintf(p,"%.*s",precision, ev->fileName);
	   precision = -1;
	}	 
	break;
      case 'l': /* location; LOG4J compliant */
	if (precision == -1)
	   p += sprintf(p, "%s:%d", ev->fileName, ev->lineNum);
        else {	      
	   p += snprintf(p, precision, "%s:%d", ev->fileName, ev->lineNum);
	   precision = -1;
	}	 
	break;
      case 'L': /* line number; LOG4J compliant */
	if (precision == -1)
	   p += sprintf(p, "%d", ev->lineNum);
        else {	      
	   p += sprintf(p, "%.*d", precision, ev->lineNum);
	   precision = -1;
	}	 
	break;
      case 'M': /* method (ie, function) name; LOG4J compliant */
	if (precision == -1)
	   p += sprintf(p, "%s", ev->functionName);
        else {	      
	   p += sprintf(p, "%.*s", precision, ev->functionName);
	   precision = -1;
	}	 
	break;
      case 'b': /* backtrace; called %throwable in LOG4J */
      case 'B': /* short backtrace; called %throwable{short} in LOG4J */
#if defined(HAVE_EXECINFO_H) && defined(HAVE_POPEN) && defined(ADDR2LINE)
	{
	  xbt_ex_t e;
	  int i;
	  
	  e.used     = backtrace((void**)e.bt,XBT_BACKTRACE_SIZE);
	  e.bt_strings = NULL;
	  e.msg=NULL;
	  e.remote=0;
	  xbt_ex_setup_backtrace(&e);
	  if (*q=='B') {
	     if (precision == -1)
	       p += sprintf(p,"%s",e.bt_strings[2]+8);
	     else {	      
		p += sprintf(p,"%.*s",precision, e.bt_strings[2]+8);
		precision = -1;
	     }	 
	  } else {
	    for (i=2; i<e.used; i++)
	       if (precision == -1)
		 p += sprintf(p,"%s\n",e.bt_strings[i]+8);
	     else {	      
		 p += sprintf(p,"%.*s\n",precision,e.bt_strings[i]+8);
		precision = -1;
	     }	 
	  }
	   
	  xbt_ex_free(e);
	}
#else
	p+=sprintf(p,"(no backtrace on this arch)");
#endif
	break;

      case 'd': /* date; LOG4J compliant */
	if (precision == -1)
	   p += sprintf(p,"%f", gras_os_time());
        else {	      
	   p += sprintf(p,"%.*f", precision, gras_os_time());
	   precision = -1;
	}	 
	break;
      case 'r': /* application age; LOG4J compliant */
	if (precision == -1)
	   p += sprintf(p,"%f", gras_os_time()-begin_of_time);
        else {	      
	   p += sprintf(p,"%.*f", precision, gras_os_time()-begin_of_time);
	   precision = -1;
	}	 
	break;
	
      case 'm': /* user-provided message; LOG4J compliant */
	if (precision == -1)
	   p += vsprintf(p, msg_fmt, ev->ap);
        else {	      
	   p += vsnprintf(p, precision, msg_fmt, ev->ap);
	   precision = -1;
	}	 
	break;

      default:
	THROW2(arg_error,0,"Unknown %%%c sequence in layout format (%s)",
	       *q,(char*)l->data);
      }
      q++;
    } else {
      *(p++) = *(q++);
    }
  }
  *p = '\0';
  return res;
}

static void xbt_log_layout_format_free(xbt_log_layout_t lay) {
  free(lay->data);
}
xbt_log_layout_t xbt_log_layout_format_new(char *arg) {
  xbt_log_layout_t res = xbt_new0(s_xbt_log_layout_t,1);
  res->do_layout = xbt_log_layout_format_doit;
  res->free_     = xbt_log_layout_format_free;
  res->data = xbt_strdup((char*)arg);
  return res;
}
