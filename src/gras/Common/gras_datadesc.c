/* $Id$ */

/* gras_datadesc.c - manipulating datadescriptors                           */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003 the OURAGAN project.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras_private.h"

void *gras_datadesc_copy_data(const DataDescriptor *dd, unsigned int c, 
			      void *data) {
  size_t s=DataSize(dd,c,HOST_FORMAT);
  void *res=malloc(s);
  
  if (!res) {
    fprintf(stderr,"grasDataDescCpyData: malloc of %d bytes failed.\n",s);
    return NULL;
  }

  memcpy(res,data,s);

  return data; 
}

int gras_datadesc_cmp(const DataDescriptor *dd1, unsigned int c1,
		      const DataDescriptor *dd2, unsigned int c2) {
  unsigned int i;
  int subcmp;

  if (c1 != c2) 
    return c1 < c2;

  if (!dd1 && !dd2) return 0;
  if (!dd1 || !dd2) return 1;

  for (i=0;i<c1;i++) {
    if (dd1[i].type != dd2[i].type) 
      return  dd1[i].type < dd2[i].type;
    if (dd1[i].repetitions != dd2[i].repetitions) 
      return dd1[i].repetitions < dd2[i].repetitions;
    if (dd1[i].type == STRUCT_TYPE) {
      subcmp = gras_datadesc_cmp(dd1[i].members, dd1[i].length,
				 dd2[i].members, dd2[i].length);
      if (subcmp)
	return subcmp;
    }
  }

  return 0;
}

void gras_datadesc_dump(const DataDescriptor *dd, unsigned int c) {
  unsigned int i;

  if (!dd) return;

  for (i=0;i<c;i++) {
    fprintf(stderr,"DD[%d] = {%s,repetitions=%d,offset=%d,members=%p,len=%d,pad=%d}\n",
	    (int)i,
	    (dd[i].type == CHAR_TYPE ? "char" :
             (dd[i].type == DOUBLE_TYPE ? "double" :
	      (dd[i].type == FLOAT_TYPE ? "float" :
	       (dd[i].type == INT_TYPE ? "int" :
		(dd[i].type == LONG_TYPE ? "long" :
		 (dd[i].type == SHORT_TYPE ? "short" :
		  (dd[i].type == UNSIGNED_INT_TYPE ? "uint" :
		   (dd[i].type == UNSIGNED_LONG_TYPE ? "ulong" :
		    (dd[i].type == UNSIGNED_SHORT_TYPE ? "ushort":
		     (dd[i].type == STRUCT_TYPE ? "struct" : "UNKNOWN")))))))))),
	    dd[i].repetitions, dd[i].offset,
	    dd[i].members, dd[i].length,
	    dd[i].tailPadding);
    if (dd[i].type == STRUCT_TYPE) {
      gras_datadesc_dump(dd[i].members, dd[i].length);
    }
  }
}


