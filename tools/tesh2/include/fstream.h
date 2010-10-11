/*
 * include/fstream.h - type representing the file stream used to manage the tesh files.
 *
 * Copyright 2008,2009 Martin Quinson, Malek Cherier All right reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it 
 * under the terms of the license (GNU LGPL) which comes with this package.
 *
 * Purpose:
 * 		This file contains all the declarations of the functions related with
 * 		the tesh fstream type.
 *
 */  
#ifndef __FSTREAM_H
#define __FSTREAM_H
    
#include <com.h>
    
#ifdef __cplusplus
extern "C" {
  
#endif  /*  */
  
/*! \brief fstream_new - create a new fstream.
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
  fstream_t  fstream_new(const char *directory, const char *name);
  
/*! \brief fstream_open - open a file stream object.
 *
 * \param fstream	The file stream to open.
 *
 * \return			If successful the function returns the 0. Otherwise 
 *					the function returns -1 and sets the global variable 
 *					errno with the appropiate error code.
 *
 * errors :
 *					[EINVAL] if the fstream parameter is invalid.
 *					[EALREADY] if the file stream is already opened.
 *					
 *					This function may also fail and set errno for any of 
 *					the errors specified for the function fopen.			
 */ 
  int  fstream_open(fstream_t fstream);
   
/*! \brief fstream_close - close a file stream object.
 *
 * \param fstream	The file stream to close.
 *
 * \return			If successful the function returns the 0. Otherwise 
 *					the function returns -1 and sets the global variable 
 *					errno with the appropiate error code.
 *
 * errors :
 *					[EINVAL] if the fstream parameter is invalid.
 *					[EBADF] if the stream is not opened.
 *					
 *					
 *					This function may also fail and set errno for any of 
 *					the errors specified for the function fclose.			
 */ 
  int  fstream_close(fstream_t fstream);
   
/*! \brief fstream_free - destroy a file stream object.
 *
 * \param ptr		A pointer to the file stream object to destroy.
 *
 * \return			If successful the function returns the 0. Otherwise 
 *					the function returns -1 and sets the global variable 
 *					errno with the appropiate error code.
 *
 * errors :
 *					[EINVAL] if the fstream object pointed to by the parameter ptr is invalid.
 *					
 *					
 *					This function may also fail and set errno for any of 
 *					the errors specified for the function fclose.
 *
 * remarks :
 *					Il the file stream object is opened the function close it
 *					before its destruction. 			
 */ 
  int  fstream_free(fstream_t * ptr);
    int  fstream_parse(fstream_t fstream, xbt_os_mutex_t mutex);
    void 
      fstream_lex_line(fstream_t fstream, context_t context,
                       xbt_os_mutex_t mutex, const char *filepos,
                       char *line);
  void  fstream_process_token(fstream_t fstream, context_t context,
                                 xbt_os_mutex_t mutex, const char *filepos,
                                 char token, char *line);
  void  fstream_handle_include(fstream_t fstream, context_t context,
                                  xbt_os_mutex_t mutex,
                                  const char *file_name,
                                  const char *description);
  void  fstream_handle_suite(fstream_t fstream, const char *description,
                                const char *filepos);
  int  fstream_launch_command(fstream_t fstream, context_t context,
                                 xbt_os_mutex_t mutex);
    long fstream_getline(fstream_t fstream, char **buf, size_t * n);
  
#ifdef __cplusplus
} 
#endif  /*  */

#endif  /*! __FSTREAM_H */
