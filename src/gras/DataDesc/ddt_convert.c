/* $Id$ */

/* ddt_remote - Stuff needed to get datadescs about remote hosts            */

/* Authors: Olivier Aumage, Martin Quinson                                  */
/* Copyright (C) 2003, 2004 the GRAS posse.                                 */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

/************************************************************************/
/* C combines the power of assembler with the portability of assembler. */
/************************************************************************/

#include "DataDesc/datadesc_private.h"

GRAS_LOG_NEW_DEFAULT_SUBCATEGORY(convert,DataDesc);

/***
 *** Table of all known architectures. 
 ***/

const gras_arch_desc_t gras_arches[gras_arch_count] = {
  {"little32", 0,   {1,2,4,4,8,   4,4,   4,8}},
  {"little64", 0,   {1,2,4,8,8,   8,8,   4.8}},
  {"big32",    1,   {1,2,4,4,8,   4,4,   4,8}},
  {"big64",    1,   {1,2,4,8,8,   8,8,   4,8}}
};

const char *gras_datadesc_arch_name(int code) {
   if (code < 0 || code >= gras_arch_count)
     return "[unknown arch]";
   return gras_arches[code].name;
}

/**
 * Local function doing the grunt work
 */
static void
gras_dd_resize_int(const void *source,
		   size_t sourceSize,
		   void *destination,
		   size_t destinationSize,
		   int signedType,
		   int lowOrderFirst);
static void
gras_dd_reverse_bytes(void *to,
		      const void *from,
		      size_t length);


/**
 * gras_dd_convert_elm:
 *
 * Convert the element described by @type comming from architecture @r_arch.
 * The data to be converted is stored in @src, and is to be stored in @dst.
 * Both pointers may be the same location if no resizing is needed.
 */
gras_error_t
gras_dd_convert_elm(gras_datadesc_type_t *type, int count,
		    int r_arch, 
		    void *src, void *dst) {
  gras_dd_cat_scalar_t scal = type->category.scalar_data;
  int cpt;
  const void *r_data;
  void *l_data;
  size_t r_size, l_size;

  gras_assert(type->category_code == e_gras_datadesc_type_cat_scalar);


  r_size = type->size[r_arch];
  l_size = type->size[GRAS_THISARCH];
  DEBUG4("r_size=%d l_size=%d,    src=%p dst=%p",
	 r_size,l_size,src,dst);

  if (r_arch == GRAS_THISARCH) { 
//      || scal.encoding == e_gras_dd_scalar_encoding_float) {
    DEBUG0("No conversion needed");
    return no_error;
  }

  r_size = type->size[r_arch];
  l_size = type->size[GRAS_THISARCH];

  if(r_size != l_size) {
    for(cpt = 0, r_data = src, l_data = dst; 
	cpt < count; 
	cpt++, 
	  r_data = (char *)r_data + r_size,
	  l_data = (char *)l_data + l_size) {

      DEBUG5("Resize elm %d from %d @%p to %d @%p",cpt, r_size,r_data, l_size,l_data);
      gras_dd_resize_int(r_data, r_size, l_data, l_size,
			 scal.encoding == e_gras_dd_scalar_encoding_sint,
			 gras_arches[r_arch].endian && 
			 gras_arches[r_arch].endian 
			 != gras_arches[GRAS_THISARCH].endian);*/

    }
    src=dst; /* Make sure to reverse bytes on the right data */
  }

  if(gras_arches[r_arch].endian != gras_arches[GRAS_THISARCH].endian && 
     (l_size * count)  > 1) {

    for(cpt = 0, r_data=src, l_data=dst;
	cpt < count;
	cpt++, 
	  r_data = (char *)r_data + l_size, /* resizing already done */
	  l_data = (char *)l_data + l_size) {                

      DEBUG1("Flip elm %d",cpt);
      gras_dd_reverse_bytes(l_data, r_data, l_size);
    }
  }

  return no_error;
}

static void
gras_dd_reverse_bytes(void *to,
		      const void *from,
		      size_t length) {

  char charBegin;
  const char *fromBegin;
  const char *fromEnd;
  char *toBegin;
  char *toEnd;

  for(fromBegin = (const char *)from, 
	fromEnd = fromBegin + length - 1,
	toBegin = (char *)to,
	toEnd = toBegin + length - 1;

      fromBegin <= fromEnd; 

      fromBegin++, fromEnd--, 
	toBegin++, toEnd--) {

    charBegin = *fromBegin;
    *toBegin = *fromEnd;
    *toEnd = charBegin;
  }
}

/*
 * Copies the integer value of size #sourceSize# stored in #source# to the
 * #destinationSize#-long area #destination#.  #signedType# indicates whether
 * or not the source integer is signed; #lowOrderFirst# whether or not the
 * bytes run least-significant to most-significant.
 *
 * It should be thread safe (operates on local variables and calls mem*)
 */
static void
gras_dd_resize_int(const void *r_data,
		   size_t r_size,
		   void *destination,
		   size_t l_size,
		   int signedType,
		   int lowOrderFirst) {

  unsigned char *destinationSign;
  int padding;
  int sizeChange = l_size - r_size;
  unsigned char *r_dataSign;
  
  gras_assert0(sizeChange, "Nothing to resize");

  if(sizeChange < 0) {
    DEBUG1("Truncate %d bytes", -sizeChange);
    /* Truncate high-order bytes. */
    memcpy(destination, 
	   lowOrderFirst?r_data:((char*)r_data-sizeChange),
	   l_size);
    if(signedType) {
      DEBUG0("This is signed");
      /* Make sure the high order bit of r_data and
       * destination are the same */
      destinationSign = lowOrderFirst ? ((unsigned char*)destination + l_size - 1) : (unsigned char*)destination;
      r_dataSign = lowOrderFirst ? ((unsigned char*)r_data + r_size - 1) : (unsigned char*)r_data;
      if((*r_dataSign > 127) != (*destinationSign > 127)) {
	if(*r_dataSign > 127)
	  *destinationSign += 128;
	else
	  *destinationSign -= 128;
      }
    }
  } else {
    DEBUG1("Extend %d bytes", sizeChange);
    /* Pad with zeros or extend sign, as appropriate. */
    if(!signedType)
      padding = 0;
    else {
      r_dataSign = lowOrderFirst ? ((unsigned char*)r_data + r_size - 1)
	                         : (unsigned char*)r_data;
      padding = (*r_dataSign > 127) ? 0xff : 0;
    }
    memset(destination, padding, l_size);
    memcpy(lowOrderFirst ? destination 
                         : ((char *)destination + sizeChange),
	   r_data, r_size);
  }
}


/**
 * gras_arch_selfid:
 *
 * returns the ID of the architecture the process is running on
 */
int
gras_arch_selfid(void) {
  return GRAS_THISARCH;
}
