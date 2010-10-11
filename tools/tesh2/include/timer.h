/*
 * include/timer.h - type representing the timer object used to manage the 
 * time allocated to a command.
 *
 * Copyright 2008,2009 Martin Quinson, Malek Cherier All right reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it 
 * under the terms of the license (GNU LGPL) which comes with this package.
 *
 * Purpose:
 * 		This file contains all the declarations of the functions related with
 * 		the tesh timer type.
 *
 */  
    
#ifndef __TIMER_H
#define __TIMER_H
    
#include <com.h>
    
#ifdef __cplusplus
extern "C" {
  
#endif  /*  */
   
/*! \brief timer_new - create a new timer.
 *
 * \param command	The command to keep a wath.
 *
 * \return			If successful the function returns the newly created
 *					timer. Otherwise the function returns NULL and sets the
 *					global variable errno with the appropiate error code.
 *
 * errors :
 *					[EINVAL] if the parameter is invalid.
 *					[ENOMEM] if the system has not enough space to allocate
 *					         the timer.
 */ 
  ttimer_t  timer_new(command_t command);
   
/*! \brief timer_free - destroy a timer object.
 *
 * \param ptr		A pointer to the timer object to destroy.
 *
 * \return			If successful the function returns the 0. Otherwise 
 *					the function returns -1 and sets the global variable 
 *					errno with the appropiate error code.
 *
 * errors :
 *					[EINVAL] if the timer object pointed to by the parameter ptr is invalid.
 *					
 *					
 */ 
  int  timer_free(ttimer_t * ptr);
    void  timer_time(ttimer_t timer);
    void  timer_wait(ttimer_t timer);
   
#ifdef __cplusplus
} 
#endif  /*  */
 
#endif  /* !__TIMER_H */
