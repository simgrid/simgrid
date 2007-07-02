/* 	$Id$	 */

/* A few tests for the maxmin library                                       */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#include "xbt/sysdep.h"
#include "surf/maxmin.h"
#include "surf/maxmin_private.h"

#include "xbt/log.h"
#include "xbt/module.h"


XBT_LOG_NEW_DEFAULT_CATEGORY(surf_test,"Messages specific for surf example");

#define PRINT_VAR(var) DEBUG1(#var " = %g",lmm_variable_getvalue(var));

/*                               */
/*        ______                 */
/*  ==l1==  L2  ==L3==           */
/*        ------                 */
/*                               */

typedef enum {
  MAXMIN,
  SDP,
  LAGRANGE_RENO,
  LAGRANGE_VEGAS,
} method_t;

void test1(method_t method);
void test1(method_t method)
{
  lmm_system_t Sys = NULL ;
  lmm_constraint_t L1 = NULL;
  lmm_constraint_t L2 = NULL;
  lmm_constraint_t L3 = NULL;

  lmm_variable_t R_1_2_3 = NULL;
  lmm_variable_t R_1 = NULL;
  lmm_variable_t R_2 = NULL;
  lmm_variable_t R_3 = NULL;

  if(method==LAGRANGE_VEGAS){
    //set default functions for TCP Vegas
    lmm_set_default_protocol_functions(func_vegas_f, func_vegas_fp, func_vegas_fpi, func_vegas_fpip);
  }else if(method==LAGRANGE_RENO){
    //set default functions for TCP Reno
    lmm_set_default_protocol_functions(func_reno_f, func_reno_fp, func_reno_fpi, func_reno_fpip);
  }


  Sys = lmm_system_new();
  L1 = lmm_constraint_new(Sys, (void *) "L1", 1.0);
  L2 = lmm_constraint_new(Sys, (void *) "L2", 10.0);
  L3 = lmm_constraint_new(Sys, (void *) "L3", 1.0);

  R_1_2_3 = lmm_variable_new(Sys, (void *) "R 1->2->3", 1.0 , -1.0 , 3);
  R_1 = lmm_variable_new(Sys, (void *) "R 1", 1.0 , -1.0 , 1);
  R_2 = lmm_variable_new(Sys, (void *) "R 2", 1.0 , -1.0 , 1);
  R_3 = lmm_variable_new(Sys, (void *) "R 3", 1.0 , -1.0 , 1);

  lmm_expand(Sys, L1, R_1_2_3, 1.0);
  lmm_expand(Sys, L2, R_1_2_3, 1.0);
  lmm_expand(Sys, L3, R_1_2_3, 1.0);

  lmm_expand(Sys, L1, R_1, 1.0);

  lmm_expand(Sys, L2, R_2, 1.0);

  lmm_expand(Sys, L3, R_3, 1.0);

  PRINT_VAR(R_1_2_3);
  PRINT_VAR(R_1);
  PRINT_VAR(R_2);
  PRINT_VAR(R_3);

  DEBUG0("\n");


Added the generic method to model fairness depending on the
transport protocol specific constraints and behavior. Protocols
TCP Reno and Vegas already implemented. Seems to work
with the testbed in src/testsuite/surf/simeng_usage. Still
need to be tested using a msg application.




  if(method==MAXMIN){
    lmm_solve(Sys);
#ifdef HAVE_SDP
  }else if(method==SDP){
    sdp_solve(Sys);    
#endif
  }else if(method==LAGRANGE_VEGAS){
    //set default functions for TCP Vegas
    lagrange_solve(Sys);  
  }else if(method==LAGRANGE_RENO){
    //set default functions for TCP Reno
    lagrange_solve(Sys);  
  }else{ 
    xbt_assert0(0,"Invalid method");
  }

  PRINT_VAR(R_1_2_3);
  PRINT_VAR(R_1);
  PRINT_VAR(R_2);
  PRINT_VAR(R_3);

  lmm_system_free(Sys);
} 

void test2(method_t method);
void test2(method_t method)
{
  lmm_system_t Sys = NULL ;
  lmm_constraint_t CPU1 = NULL;
  lmm_constraint_t CPU2 = NULL;

  lmm_variable_t T1 = NULL;
  lmm_variable_t T2 = NULL;


  if(method==LAGRANGE_VEGAS){
    //set default functions for TCP Vegas
    lmm_set_default_protocol_functions(func_vegas_f, func_vegas_fp, func_vegas_fpi, func_vegas_fpip);
  }else if(method==LAGRANGE_RENO){
    //set default functions for TCP Reno
    lmm_set_default_protocol_functions(func_reno_f, func_reno_fp, func_reno_fpi, func_reno_fpip);
  }

  Sys = lmm_system_new();
  CPU1 = lmm_constraint_new(Sys, (void *) "CPU1", 200.0);
  CPU2 = lmm_constraint_new(Sys, (void *) "CPU2", 100.0);

  T1 = lmm_variable_new(Sys, (void *) "T1", 1.0 , -1.0 , 1);
  T2 = lmm_variable_new(Sys, (void *) "T2", 1.0 , -1.0 , 1);

  lmm_expand(Sys, CPU1, T1, 1.0);
  lmm_expand(Sys, CPU2, T2, 1.0);

  PRINT_VAR(T1);
  PRINT_VAR(T2);

  DEBUG0("\n");


  if(method==MAXMIN){
    lmm_solve(Sys);
#ifdef HAVE_SDP
  }else if(method==SDP){
    sdp_solve(Sys);    
#endif
  }else if(method==LAGRANGE_VEGAS){
    //set default functions for TCP Vegas
    lagrange_solve(Sys);  
  }else if(method==LAGRANGE_RENO){
    //set default functions for TCP Reno
    lagrange_solve(Sys);  
  }else{ 
    xbt_assert0(0,"Invalid method");
  }

  PRINT_VAR(T1);
  PRINT_VAR(T2);

  DEBUG0("\n");

  lmm_system_free(Sys);
}

void test3(method_t method);
void test3(method_t method)
{
  int flows=11;  //flows and conexions are synonnims ?
  int links=10;  //topology info
  
  //just to be carefull
  int i = 0;
  int j = 0;

  double **A;

  lmm_system_t Sys = NULL ;
  lmm_constraint_t *tmp_cnst = NULL;
  lmm_variable_t   *tmp_var  = NULL;
  char **tmp_name;


  /*array to add the the constraints of fictiv variables */
  double B[15] = {10, 10, 10, 10, 10, 10, 10, 10, 10, 10,

		1, 1, 1, 1, 1};

  A = (double **)calloc(links+5, sizeof(double));
 
  for(i=0 ; i< links+5; i++){
    A[i] = (double *)calloc(flows+5, sizeof(double));

    for(j=0 ; j< flows+5; j++){
      A[i][j] = 0.0;

      if(i >= links || j >= flows){
	  A[i][j] = 0.0;
      }
    }
  }

  /*matrix that store the constraints/topollogy*/
  /*double A[15][16]=
    {{0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0,    0, 0, 0, 0, 0},
     {0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0,    0, 0, 0, 0, 0},
     {0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0,    0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,    0, 0, 0, 0, 0},
     {1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0,    0, 0, 0, 0, 0},
     {1, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0,    0, 0, 0, 0, 0},
     {1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1,    0, 0, 0, 0, 0},
     {0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 1,    0, 0, 0, 0, 0},
     {0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1,    0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0,    0, 0, 0, 0, 0},
     
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    1, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    0, 1, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    0, 0, 1, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    0, 0, 0, 1, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    0, 0, 0, 0, 1}
     }; */

  A[0][1]  = 1.0;
  A[0][7]  = 1.0;
  
  A[1][1]  = 1.0;
  A[1][7]  = 1.0;
  A[1][8]  = 1.0;

  A[2][1]  = 1.0;
  A[2][8]  = 1.0;

  A[2][1]  = 1.0;
  A[2][8]  = 1.0;

  A[3][8]  = 1.0;

  A[4][0]  = 1.0;
  A[4][3]  = 1.0;
  A[4][9]  = 1.0;

  A[5][0]  = 1.0;
  A[5][3]  = 1.0;
  A[5][4]  = 1.0;
  A[5][9]  = 1.0;

  A[6][0]  = 1.0;
  A[6][4]  = 1.0;
  A[6][9]  = 1.0;
  A[6][10] = 1.0;

  A[7][2]  = 1.0;
  A[7][4]  = 1.0;
  A[7][6]  = 1.0;
  A[7][9]  = 1.0;
  A[7][10] = 1.0;

  A[8][2]  = 1.0;
  A[8][10] = 1.0;

  A[9][5]  = 1.0;
  A[9][6]  = 1.0;
  A[9][9]  = 1.0;

  
  A[10][11] = 1.0;
  A[11][12] = 1.0;
  A[12][13] = 1.0;
  A[13][14] = 1.0;
  A[14][15] = 1.0;


  if(method==LAGRANGE_VEGAS){
    //set default functions for TCP Vegas
    lmm_set_default_protocol_functions(func_vegas_f, func_vegas_fp, func_vegas_fpi, func_vegas_fpip);
  }else if(method==LAGRANGE_RENO){
    //set default functions for TCP Reno
    lmm_set_default_protocol_functions(func_reno_f, func_reno_fp, func_reno_fpi, func_reno_fpip);
  }

  Sys = lmm_system_new();
  


  tmp_name = (char **) calloc(31, sizeof(char *));
  for(i=0; i<31; i++){
    tmp_name[i] = (char *) calloc(10, sizeof(char));
  }
  
  /*
   * Creates the constraints
   */
  tmp_cnst = calloc(15, sizeof(lmm_constraint_t));
  for(i=0; i<15; i++){
    sprintf(tmp_name[i], "C_%03d", i); 
    tmp_cnst[i] = lmm_constraint_new(Sys, (void *) tmp_name[i], B[i]);
  }


  /*
   * Creates the variables
   */
  tmp_var = calloc(16, sizeof(lmm_variable_t));
  for(j=0; j<16; j++){
    sprintf(tmp_name[i+j], "X_%03d", j); 
    tmp_var[j] = lmm_variable_new(Sys, (void *) tmp_name[i+j], 1.0, -1.0 , 15);
  }

  /*
   * Link constraints and variables
   */
  for(i=0;i<15;i++) { 
    for(j=0; j<16; j++){
      if(A[i][j]){
	lmm_expand(Sys, tmp_cnst[i], tmp_var[j], 1.0); 
      }
    }
  }


  lmm_print(Sys);


  for(j=0; j<16; j++){
    PRINT_VAR(tmp_var[j]);
  }

  if(method==MAXMIN){
    lmm_solve(Sys);
#ifdef HAVE_SDP
  }else if(method==SDP){
    sdp_solve(Sys);    
#endif
  }else if(method==LAGRANGE_VEGAS){
    //set default functions for TCP Vegas
    lagrange_solve(Sys);  
  }else if(method==LAGRANGE_RENO){
    //set default functions for TCP Reno
    lagrange_solve(Sys);  
  }else{ 
    xbt_assert0(0,"Invalid method");
  }

  for(j=0; j<16; j++){
    PRINT_VAR(tmp_var[j]);
  }

  free(tmp_var);
  free(tmp_cnst);
  free(tmp_name);
  lmm_system_free(Sys);
}

#ifdef __BORLANDC__
#pragma argsused
#endif

int main(int argc, char **argv)
{
  xbt_init(&argc,argv);

  DEBUG0("***** Test 1 (Max-Min) ***** \n");
  test1(MAXMIN);
#ifdef HAVE_SDP
  DEBUG0("***** Test 1 (SDP) ***** \n");
  test1(SDP);
#endif
  DEBUG0("***** Test 1 (Lagrange - Vegas) ***** \n");
  test1(LAGRANGE_VEGAS);
  DEBUG0("***** Test 1 (Lagrange - Reno) ***** \n");
  test1(LAGRANGE_RENO);



  DEBUG0("***** Test 2 (Max-Min) ***** \n");
  test2(MAXMIN);
#ifdef HAVE_SDP
  DEBUG0("***** Test 2 (SDP) ***** \n");
  test2(SDP);
#endif
  DEBUG0("***** Test 2 (Lagrange - Vegas) ***** \n");
  test2(LAGRANGE_VEGAS);
  DEBUG0("***** Test 2 (Lagrange - Reno) ***** \n");
  test2(LAGRANGE_RENO);


  DEBUG0("***** Test 3 (Max-Min) ***** \n");
  test3(MAXMIN);
#ifdef HAVE_SDP
  DEBUG0("***** Test 3 (SDP) ***** \n");
  test3(SDP);
#endif 
  DEBUG0("***** Test 3 (Lagrange - Vegas) ***** \n");
  test3(LAGRANGE_VEGAS);
  DEBUG0("***** Test 3 (Lagrange - Reno) ***** \n"); 
  test3(LAGRANGE_RENO); 

  return 0;
}
