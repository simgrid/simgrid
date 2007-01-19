/* 	$Id$	 */

/* Copyright (c) 2007 Arnaud Legrand, Pedro Velho. All rights reserved.     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "maxmin_private.h"
#include <stdlib.h>

#include <declarations.h>

#ifndef MATH
#include <math.h>
#endif

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_sdp, surf,
				"Logging specific to SURF (sdp)");





/*
########################################################################
######################## Simple Proportionnal fairness #################
########################################################################
####### Original problem ##########
#
#  Max x_1*x_2*...*x_n
#    (A.x)_1 <= b_1
#    (A.x)_2 <= b_2
#     ...
#    (A.x)_m <= b_m
#    x>=0
#
#  We assume in the following that n=2^K
#
####### Standard SDP form #########
#
# A SDP can be written in the following standard form:
# 
#   (P) min c1*x1+c2*x2+...+cm*xm
#       st  F1*x1+F2*x2+...+Fm*xm-F0=X
#                                 X >= 0
#
# Where F1, F2, ..., Fm are symetric matrixes of size n by n. The
# constraint X>0 means that X must be positive semidefinite positive.
#
########## Equivalent SDP #########
#
#  Variables:
#
#    y(k,i) for k in [0,K] and i in [1,2^k]
#
#  Structure :
#                             y(0,1)
#                y(1,1)                      y(1,2)
#        y(2,1)        y(2,2)        y(2,3)        y(2,4)
#    y(3,1) y(3,2) y(3,3) y(3,4) y(3,5) y(3,6) y(3,7) y(3,8)
#    -------------------------------------------------------
#     x_1     x_2    x_3    x_4    x_5    x_6    x_7    x_8
#  
#
#  Structure Constraints:
#
#    forall k in [0,K-1], and i in [1,2^k]
#      [ y(k,  2i-1)   y(k-1, i) ]
#      [ y(k-1, i  )   y(k,  2i) ]
#
#    2^K-1
#
#  Capacity Constraints:
#     forall l in [1,m]
#        -(A.y(K,*))_l >= - b_l
#
#  Positivity Constraints:
#    forall k in [0,K], and i in [1,2^k]
#        y(k,i) >= 0
#
#  Minimize -y(0,1)
*/




//typedef struct lmm_system {
//  int modified;
//  s_xbt_swag_t variable_set;	/* a list of lmm_variable_t */
//  s_xbt_swag_t constraint_set;	/* a list of lmm_constraint_t */
//  s_xbt_swag_t active_constraint_set;	/* a list of lmm_constraint_t */
//  s_xbt_swag_t saturated_variable_set;	/* a list of lmm_variable_t */
//  s_xbt_swag_t saturated_constraint_set;	/* a list of lmm_constraint_t_t */
//  xbt_mallocator_t variable_mallocator;
//} s_lmm_system_t;

#define get_y(a,b) (pow(2,a)+b-1)

void sdp_solve(lmm_system_t sys)
{
  lmm_variable_t var = NULL;

  /*
   * SDP mapping variables.
   */
  int K=0;
  int nb_var = 0;
  int nb_cnsts = 0;
  int flows = 0;
  int links = 0;
  int nb_cnsts_capacity=0;
  int nb_cnsts_struct=0;
  int nb_cnsts_positivy=0;
  int block_num=0;
  int block_size;  
  int *isdiag=NULL;
  FILE *sdpout = fopen("SDPA-printf.tmp","w");
  int matno=0;
  int i=0;
  int j=0;
  int k=0;
  
  /* 
   * CSDP library specific variables.
   */
  struct blockmatrix C;
  struct blockmatrix X,Z;
  double *y;
  double pobj, dobj;
  double *a;
  struct constraintmatrix *constraints;  
 
  /*
  lmm_constraint_t cnst = NULL;
  lmm_element_t elem = NULL;
  xbt_swag_t cnst_list = NULL;
  xbt_swag_t var_list = NULL;
  xbt_swag_t elem_list = NULL;
  double min_usage = -1;
  */

  if ( !(sys->modified))
    return;

  /*
   * Those fields are the top level description of the platform furnished in the xml file.
   */
  flows = xbt_swag_size(&(sys->variable_set));
  links = xbt_swag_size(&(sys->active_constraint_set));

  /* 
   * This  number is found based on the tree structure explained on top.
   */
  K = (int)log((double)flows)/log(2.0);
  
  /* 
   * The number of variables in the SDP style.
   */
  nb_var = get_y(K, pow(2,K));
  
  /*
   * Find the size of each group of constraints.
   */
  nb_cnsts_capacity = links + ((int)pow(2,K)) - flows;
  nb_cnsts_struct = (int)pow(2,K) - 1;
  nb_cnsts_positivy = (int)pow(2,K);

  /*
   * The total number of constraints.
   */
  nb_cnsts = nb_cnsts_capacity + nb_cnsts_struct + nb_cnsts_positivy;


  /*
   * Keep track of which blocks have off diagonal entries. 
   */
  isdiag=(int *)calloc((nb_cnsts+1), sizeof(int));
  for (i=1; i<=nb_cnsts; i++)
    isdiag[i]=1;

  C.nblocks = nb_cnsts;
  C.blocks  = (struct blockrec *) calloc((C.nblocks+1),sizeof(struct blockrec));
  constraints = (struct constraintmatrix *)calloc((nb_var+1),sizeof(struct constraintmatrix));

  for(i = 1; i <= nb_var; i++){
    constraints[i].blocks=NULL; 
  }
  
  a = (double *)calloc(nb_var+1, sizeof(double)); 

  /*
   * Block sizes.
   */
  block_num=1;
  block_size=0;

  /*
   * For each constraint block do.
   */
  for(i = 1; i <= nb_cnsts; i++){
    
    /*
     * Structured blocks are size 2 and all others are size 1.
     */
    if(i <= nb_cnsts_struct){
      block_size = 2;
      fprintf(sdpout,"2 ");
    }else{
      block_size = 1;
      fprintf(sdpout,"1 ");
    }
    
    /*
     * All blocks are matrices.
     */
    C.blocks[block_num].blockcategory = MATRIX;
    C.blocks[block_num].blocksize = block_size;
    C.blocks[block_num].data.mat = (double *)calloc(block_size * block_size, sizeof(double));
 
    block_num++;
  }

  fprintf(sdpout,"\n");


  /*
   * Creates de objective function array.
   */
  a = (double *)calloc((nb_var+1), sizeof(double));
  
  for(i = 1; i <= nb_var; i++){
    if(get_y(0,1)==i){
      fprintf(sdpout,"-1 ");
      a[i]=-1;
    }else{
      fprintf(sdpout,"0 ");
      a[i]=0;
    }
  }
  fprintf(sdpout,"\n");


  /*
   * Structure contraint blocks.
   */
  block_num = 1;
  matno = 1;  
  for(k = 1; k <= K; k++){
    for(i = 1; i <= pow(2,k-1); i++){
      matno=get_y(k,2*i-1);
      fprintf(sdpout,"%d %d 1 1 1\n", matno  , block_num);
      addentry(constraints, &C, matno, block_num, 1, 1, 1.0, C.blocks[block_num].blocksize);

      matno=get_y(k,2*i);
      fprintf(sdpout,"%d %d 2 2 1\n", matno  , block_num);
      addentry(constraints, &C, matno, block_num, 2, 2, 1.0, C.blocks[block_num].blocksize);

      matno=get_y(k-1,i);
      fprintf(sdpout,"%d %d 1 2 1\n", matno  , block_num);
      addentry(constraints, &C, matno, block_num, 1, 2, 1.0, C.blocks[block_num].blocksize);      
      
      matno=get_y(k-1,i);
      fprintf(sdpout,"%d %d 2 1 1\n", matno  , block_num);
      addentry(constraints, &C, matno, block_num, 2, 1, 1.0, C.blocks[block_num].blocksize);
      
      isdiag[block_num] = 0;
      block_num++;
    }
  }
  

  

}
