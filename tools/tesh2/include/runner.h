/*
 * include/runner.h - type representing the tesh runner .
 *
 * Copyright 2008,2009 Martin Quinson, Malek Cherier All right reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it 
 * under the terms of the license (GNU LGPL) which comes with this package.
 *
 * Purpose:
 * 		This file contains all the declarations of the functions related with
 * 		the tesh runner type.
 *
 */  
    
#ifndef __RUNNER_H	
#define __RUNNER_H
    
#include <com.h>
    
#ifdef __cplusplus
extern "C" {
  
#endif  /*  */
  
/*! \brief runner_init - initialize the tesh runner.
 *
 * \param check_syntax_flag		If 1, the runner check the syntax of all the tesh files before running.
 * \param timeout				The time alloted to the run of all the units (if -1 no time alloted).
 * \param fstreams				A fstreams object containing the file streams representing the tesh files to run.
 *
 * \return						If successful the function returns 0. Otherwise the function returns -1 and sets
 *								the global variable errno with the appropiate error code.
 *
 * errors :
 *								[EALREADY] if the runner is already initialized.
 *								[ENOMEM] if the system has not enough space to initialize the runner.
 *								[ESYNTAX] if the parameter check_syntax_flag is 1 and a syntax error is detected.
 */ 
  int 
      runner_init( /*int check_syntax_flag, */ int timeout,
                  fstreams_t fstreams);
  
/*! \brief runner_run - run the tesh files.
 */ 
  void  runner_run(void);
  
/*! \brief runner_destroy - destroy the runner (release all the resources allocated by the runner)
 */ 
  void  runner_destroy(void);
  
/*! \brief runner_summarize - display the summary of the execution of all the tests of the tesh files.
 */ 
  void  runner_summarize(void);
  
/*! \brief runner_interrupt - interrupt all the units of the run.
 */ 
  void  runner_interrupt(void);
  int  runner_is_timedout(void);
  
#ifdef __cplusplus
} 
#endif  /*  */

#endif  /* !__RUNNER_H */
