/*
 * include/unit.h - type representing the tesh unit concept.
 *
 * Copyright 2008,2009 Martin Quinson, Malek Cherier All right reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it 
 * under the terms of the license (GNU LGPL) which comes with this package.
 *
 * Purpose:
 * 		This file contains all the declarations of the functions related with
 * 		the tesh unit concept.
 *
 */  
    
#ifndef __UNIT_H	
#define __UNIT_H
    
#include <com.h>
    
#ifdef __cplusplus
extern "C" {
  
#endif  /*  */
  
/*! \brief unit_new - create a new unit.
 *
 * \param runner	The runner which runs the unit.
 * \param owner		The unit which owns the unit if it is an included unit.
 * \param fstream	The file stream object used to parse the unit.
 *
 * \return			If successful the function returns the newly created
 *					unit. Otherwise the function returns NULL and sets the
 *					global variable errno with the appropiate error code.
 *
 * errors :
 *					[EINVAL] if one of the parameters is invalid.
 *					[ENOMEM] if the system has not enough space to allocate
 *					         the unit.
 */ 
  unit_t 
      unit_new(runner_t runner, unit_t root, unit_t owner,
               fstream_t fstream);
   
/*! \brief unit_free - destroy a tesh unit.
 *
 * \param ptr		A pointer to the unit to destroy.
 *
 * \return			If successful the function returns 0. Otherwise the 
 *					function returns -1 and sets the global variable errno 
 *					with the appropiate error code.
 *
 * errors :
 *					[EINVAL] if the pointer to the unit to destroy is invalid.
 */ 
  int  unit_free(unit_t * ptr);
   
/*! \brief unit_run - run a tesh unit.
 *
 * \param unit		The unit to run.
 * \param mutex		A mutex used to synchronize the access of the runner properties.
 *
 * \return			If successful the function returns 0. Otherwise the 
 *					function returns -1 and sets the global variable errno 
 *					with the appropiate error code.
 *
 * errors :
 *					[EINVAL] if the unit specified as parameter is invalid.
 *					         if the mutex specified as parameter is invalid.
 *
 * remarks :		If the runner is interrupted during a call of this function,
 *					the unit is not launched but its flag interrupted is signaled.
 */ 
  int  unit_run(unit_t unit, xbt_os_mutex_t mutex);
    
/*! \brief unit_interrupt - interrupt a tesh unit.
 *
 * \param unit		The unit to interrupt.
 *
 * \return			If successful the function returns 0. Otherwise the 
 *					function returns -1 and sets the global variable errno 
 *					with the appropiate error code.
 *
 * errors :
 *					[EINVAL] if the unit specified as parameter is invalid.
 *					[EALREADY] if the unit is already interrupted.
 *
 * remarks :		If the runner is interrupted during a call of this function,
 *					the unit is not launched but its flag interrupted is signaled.
 */ 
  int  unit_interrupt(unit_t unit);
   
/*! \brief unit_summuarize - summarize the run of tesh unit.
 *
 * \param unit		The unit to summarize the run.
 *
 * \return			If successful the function returns 0. Otherwise the 
 *					function returns -1 and sets the global variable errno 
 *					with the appropiate error code.
 *
 * errors :
 *					[EINVAL] if the unit specified as parameter is invalid.
 */ 
  int  unit_summuarize(unit_t unit);
    int  unit_reset(unit_t unit);
    void 
      unit_set_error(unit_t unit, int errcode, int kind, const char *line);
  
#ifdef __cplusplus
} 
#endif  /*  */

#endif  /* !__UNIT_H */
