/*
 * include/witer.h - type representing the object used to write to the stdin 
 * (redirected) of a command.
 *
 * Copyright 2008,2009 Martin Quinson, Malek Cherier All right reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it 
 * under the terms of the license (GNU LGPL) which comes with this package.
 *
 * Purpose:
 * 		This file contains all the declarations of the functions related with
 * 		the tesh writer type.
 *
 */  
    
#ifndef __WRITER_H
#define __WRITER_H
    
#include <com.h>
    
#ifdef __cplusplus
extern "C" {
  
#endif  /*  */
  
/*! \brief writer_new - create a new writer.
 *
 * \param command	The command owning the stdin written by the writer.
 *
 * \return			If successful the function returns the newly created
 *					writer. Otherwise the function returns NULL and sets the
 *					global variable errno with the appropiate error code.
 *
 * errors :
 *					[EINVAL] if the parameter is invalid.
 *					[ENOMEM] if the system has not enough space to allocate
 *					         the writer.
 */ 
  writer_t  writer_new(command_t command);
    
/*! \brief writer_free - destroy a writer object.
 *
 * \param ptr		A pointer to the writer object to destroy.
 *
 * \return			If successful the function returns the 0. Otherwise 
 *					the function returns -1 and sets the global variable 
 *					errno with the appropiate error code.
 *
 * errors :
 *					[EINVAL] if the writer object pointed to by the parameter ptr is invalid.
 *					
 *					
 */ 
  int  writer_free(writer_t * ptr);
    void  writer_write(writer_t writer);
    void  writer_wait(writer_t writer);
   
#ifdef __cplusplus
} 
#endif  /*  */

#endif  /* !__WRITER_H */
