/*
 * include/command.h - type representing a command.
 *
 * Copyright 2008,2009 Martin Quinson, Malek Cherier All right reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it 
 * under the terms of the license (GNU LGPL) which comes with this package.
 *
 * Purpose:
 * 		This file contains all the declarations of the functions related with
 * 		the tesh command type.
 *
 */  
    
#ifndef __COMMAND_H
#define __COMMAND_H
    
#include <com.h>
    
#ifdef __cplusplus
extern "C" {
  
#endif  /*  */
  
/*! \brief command_new - create a new fstream.
 *
 * \param unit			The unit contained the command.
 * \param context		The context of the excecution of the command.
 * \param mutex			A mutex to synchronize the access of the properties of its unit.
 *
 * \return				If successful the function returns the newly created
 *						command. Otherwise the function returns NULL and sets the
 *						global variable errno with the appropiate error code.
 * remarks:
 *						If the parameter directory is NULL, the parameter name
 *						must be `stdin`.
 *
 * errors :
 *						[EINVAL] if one of the parameters is invalid.
 *						[ENOMEM] if the system has not enough space to allocate
 *								 the command.
 */ 
  command_t 
      command_new(unit_t unit, context_t context, xbt_os_mutex_t mutex);
   
/*! \brief command_free - destroy a command object.
 *
 * \param ptr			A pointer to the command object to destroy.
 *
 * \return				If successful the function returns the 0. Otherwise 
 *						the function returns -1 and sets the global variable 
 *						errno with the appropiate error code.
 *
 * errors :
 *						[EINVAL] if the command object pointed to by the ptr parameter is invalid.
 *					
 */ 
  int  command_free(command_t * ptr);
   
/*! \brief command_run - run a command object.
 *
 * \param command		The command to run.
 *
 * \return				If successful the function returns the 0. Otherwise 
 *						the function returns -1 and sets the global variable 
 *						errno with the appropiate error code.
 *
 * errors :
 *						[EINVAL] if the parameter is invalid.
 *
 * remarks :
 *						The type of running (asynchonus or no) depend of the
 *						context of the command.
 *					
 */ 
  int  command_run(command_t command);
   
/*! \brief command_exec - execute a command object.
 *
 * \param command		The command object to run.
 * \param command_line	The command line of the process to create.				
 */ 
  void  command_exec(command_t command, const char *command_line);
  
/*! \brief command_wait - wait for the end of a command.
 *
 * \param command		The command object to wait for.		
 */ 
  void  command_wait(command_t command);
   
/*! \brief command_interrupt - wait for the end of a command.
 *
 * \param command		The command object to interrupt.		
 */ 
  void  command_interrupt(command_t command);
   
/*! \brief command_summarize - print the summary of the execution of a command.
 *
 * \param command		The command object to display the summary.	
 *
 * remark:
 *						The summary of the command is displayed only if the user
 *						specifies both summary and detail-summary options on the
 *						tesh command line.
 */ 
  void  command_summarize(command_t command);
   
/*! \brief command_handle_failure - handle a failure caused by a command.
 *
 * \param command		The command to handle the failure.	
 *
 * remark:
 *						The behavior of the command failure handling depends
 *						of the specification of the options keep-going and
 *						keep-going-unit. If the user has specified the option
 *						keep-going on the command line of tesh, only the unit 
 *						containing the failed command is interrupted and all 
 *						other units continue.
 */ 
  void  command_handle_failure(command_t command, cs_reason_t reason);
    void command_kill(command_t command);
    void  command_check(command_t command);
   
#ifdef __cplusplus
} 
#endif  /*  */

#endif  /* !_COMMAND_H */
