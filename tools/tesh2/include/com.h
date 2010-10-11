#ifndef __COM_H
#define __COM_H

/*
 * include/com.h - contains all common declarations of types and definitions
 * and global variables of tesh.
 *
 * Copyright 2008,2009 Martin Quinson, Malek Cherier All right reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it 
 * under the terms of the license (GNU LGPL) which comes with this package.
 *
 * Purpose:
 * 		This file contains all the declarations common declarations and definitions
 *		and global variables of tesh.
 *
 */ 
    
#include <types.h>
#include <ctype.h>
    
#ifdef __cplusplus
extern "C" {
  
#endif  /*  */
  
/* 
 * the semaphore used by the runner to wait the end of all the units 
 */ 
  extern xbt_os_sem_t  units_sem;
   
/* 
 * the semaphore used to synchronize the jobs 
 */ 
  extern xbt_os_sem_t  jobs_sem;
    
/* the list of tesh include directories */ 
  extern xbt_dynar_t  include_dirs;
   
/*
 * if 1, an interruption was requested by a command or a
 * unit.
 */ 
  extern int  interrupted;
   
/*
 * if 1, tesh doesn't display the commands launched.
 */ 
  extern int  silent_flag;
   
/*
 * if 1, tesh simulates the run.
 */ 
  extern int  dry_run_flag;
   
/* ? */ 
  extern int  just_print_flag;
   
/* if 1, tesh diplay the current directory.
*/ 
  extern int  print_directory_flag;
   
/*
 * this directory object represents the root directory.
 */ 
  extern directory_t  root_directory;
   
/*
 * if 1, the summary is detailed.
 */ 
  extern int  detail_summary_flag;
   
/* 
 * the tesh exit code.
 */ 
  extern int  exit_code;
    extern int  err_kind;
    extern char * err_line;
   
/*
 * the list of the errors of the run.
 */ 
  extern xbt_dynar_t  errors;
   
/*
 * if 1, it's the tesh root (the parent of all tesh processes).
 */ 
  extern int  is_tesh_root;
   
/* 
 * if 1, keep going when some commands can't be founded	
 */ 
  extern int  keep_going_flag;
   
/* 
 * if 1, ignore failures of units or commands.					
 */ 
  extern int  keep_going_unit_flag;
     
#ifdef __cplusplus
} 
#endif  /*  */

#endif  /* !__COM_H */
