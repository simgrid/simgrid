/*
 * include/contex.h - type representing the context execution of a command.
 *
 * Copyright 2008,2009 Martin Quinson, Malek Cherier All right reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it 
 * under the terms of the license (GNU LGPL) which comes with this package.
 *
 * Purpose:
 * 		This file contains all the declarations of the functions related with
 * 		the tesh context type.
 *
 */  
    
#ifndef _CONTEXT_H
#define _CONTEXT_H
    
#include <com.h>
    
#ifdef __cplusplus
extern "C" {
  
#endif  /*  */
  
/*! \brief create_new - create a new context.
 *
 * \return			If successful the function returns the newly created
 *					command. Otherwise the function returns NULL and sets the
 *					global variable errno with the appropiate error code.
 *					[ENOMEM] if the system has not enough space to allocate
 *					         the context.
 */ 
  context_t  context_new(void);
  
/*! \brief context_free - destroy a context object.
 *
 * \param ptr		A pointer to the context object to destroy.
 *
 * \return			If successful the function returns the 0. Otherwise 
 *					the function returns -1 and sets the global variable 
 *					errno with the appropiate error code.
 *
 * errors :
 *					[EINVAL] if the context object pointed to by the ptr parameter is invalid.
 *					
 */ 
  int  context_free(context_t * ptr);
   
/*! \brief context_dup - duplicate a context object.
 *
 * \param context	The context to duplicate.
 *
 * \return			If successful the function returns the duplicate. Otherwise 
 *					the function returns NULL and sets the global variable 
 *					errno with the appropiate error code.
 *
 * errors :
 *					[EINVAL] if the parameter is invalid.
 *					[ENOMEM] if there is not enough memory to allocate the duplicate.
 *					
 */ 
   context_t  context_dup(context_t context);
    
/*! \brief context_reset - reset a context object.
 *
 * \param context	The context to reset.
 *
 * \return			If successful the function returns 0. Otherwise 
 *					the function returns -1 and sets the global variable 
 *					errno with the appropiate error code.
 *
 * errors :
 *					[EINVAL] if the parameter is invalid.
 *					
 */ 
  int  context_reset(context_t context);
    void  context_clear(context_t context);
    void  context_input_write(context_t context, const char *buffer);
  void  context_ouput_read(context_t context, const char *buffer);
  
#ifdef __cplusplus
extern} 
#endif  /*  */

#endif  /* !_CONTEXT_H */
