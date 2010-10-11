/*
 * include/fstreams.h - type representing a vector of file streams.
 *
 * Copyright 2008,2009 Martin Quinson, Malek Cherier All right reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it 
 * under the terms of the license (GNU LGPL) which comes with this package.
 *
 * Purpose:
 * 		This file contains all the declarations of the functions related with
 * 		the tesh fstreams type.
 *
 */  
#ifndef __FSTREAMS_H
#define __FSTREAMS_H
    
#include <com.h>
    
#ifdef __cplusplus
extern "C" {
  
#endif  /*  */
  
/*! \brief fstreams_new - create a new fstreams.
 *
 * \param directory	The directory of the tesh file.
 * \param name		The name of the tesh file.
 *
 * \return			If successful the function returns the newly created
 *					unit. Otherwise the function returns NULL and sets the
 *					global variable errno with the appropiate error code.
 * remarks:
 *					If the parameter directory is NULL, the parameter name
 *					must be `stdin`.
 *
 * errors :
 *					[EINVAL] if one of the parameters is invalid.
 *					[ENOMEM] if the system has not enough space to allocate
 *					         the file stream.
 */ 
  fstreams_t  fstreams_new(void_f_pvoid_t fn_finalize);
    int  fstreams_exclude(fstreams_t fstreams, excludes_t excludes);
    int  fstreams_contains(fstreams_t fstreams, fstream_t fstream);
    int  fstreams_add(fstreams_t fstreams, fstream_t fstream);
    int  fstreams_free(void **fstreamsptr);
  int  fstreams_get_size(fstreams_t fstreams);
    int  fstreams_is_empty(fstreams_t fstreams);
    int  fstreams_contains(fstreams_t fstreams, fstream_t fstream);
    int  fstreams_load(fstreams_t fstreams);
     
#ifdef __cplusplus
} 
#endif  /*  */
 
#endif  /* !__FSTREAMS_H */
