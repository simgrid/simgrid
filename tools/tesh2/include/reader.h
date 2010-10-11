/*
 * include/reader.h - type representing the object used to read from the stdin 
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
    
#ifndef __READER_H
#define __READER_H
    
#include <com.h>
    
#ifdef __cplusplus
extern "C" {
  
#endif  /*  */
   
/*! \brief reader_new - create a new reader.
 *
 * \param command	The command owning the stdout readed by the reader.
 *
 * \return			If successful the function returns the newly created
 *					reader. Otherwise the function returns NULL and sets the
 *					global variable errno with the appropiate error code.
 *
 * errors :
 *					[EINVAL] if the parameter is invalid.
 *					[ENOMEM] if the system has not enough space to allocate
 *					         the reader.
 */ 
  reader_t  reader_new(command_t command);
   
/*! \brief reader_free - destroy a reader object.
 *
 * \param ptr		A pointer to the reader object to destroy.
 *
 * \return			If successful the function returns the 0. Otherwise 
 *					the function returns -1 and sets the global variable 
 *					errno with the appropiate error code.
 *
 * errors :
 *					[EINVAL] if the reader object pointed to by the parameter ptr is invalid.
 *					
 *					
 */ 
  int  reader_free(reader_t * ptr);
    void  reader_read(reader_t reader);
    void  reader_wait(reader_t reader);
   
#ifdef __cplusplus
} 
#endif  /*  */

#endif  /* !__READER_H */
