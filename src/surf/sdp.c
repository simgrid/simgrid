/* 	$Id$	 */

/* Copyright (c) 2007 Arnaud Legrand, Pedro Velho. All rights reserved.     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#include "xbt/log.h"
#include "xbt/sysdep.h"
#include "maxmin_private.h"

#include <stdio.h>
#include <stdlib.h>
#ifndef MATH
#include <math.h>
#endif

/*
 * SDP specific variables.
 */
#define NOSHORTS
#include <declarations.h>


static void create_cross_link(struct constraintmatrix *myconstraints, int k);

static void addentry(struct constraintmatrix *constraints,
                     struct blockmatrix *, int matno, int blkno,
                     int indexi, int indexj, double ent, int blocksize);


XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_sdp, surf,
                                "Logging specific to SURF (sdp)");
XBT_LOG_NEW_SUBCATEGORY(surf_sdp_out, surf, "Logging specific to SURF (sdp)");
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
#        x_1 <= c_1
#        x_2 <= c_2
#         ...
#        x_m <= c_m
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
#  Latency Constraints:
#    forall i in [1,2^k] and v in [0,m-1]
#        if(i <= m-1){
#            y(k-1, i) <= bound
#        }else{
#            y(k-1, i) <= 1
#        }
#  Minimize -y(0,1)
*/

//typedef struct lmm_system {
//  int modified;
//  s_xbt_swag_t variable_set;  /* a list of lmm_variable_t */
//  s_xbt_swag_t constraint_set;        /* a list of lmm_constraint_t */
//  s_xbt_swag_t active_constraint_set; /* a list of lmm_constraint_t */
//  s_xbt_swag_t saturated_variable_set;        /* a list of lmm_variable_t */
//  s_xbt_swag_t saturated_constraint_set;      /* a list of lmm_constraint_t_t */
//  xbt_mallocator_t variable_mallocator;
//} s_lmm_system_t;

#define get_y(a,b) (pow(2,a)+b-1)

void sdp_solve(lmm_system_t sys)
{

  /*
   * SDP mapping variables.
   */
  int K = 0;
  int nb_var = 0;
  int nb_cnsts = 0;
  int flows = 0;
  int links = 0;
  int nb_cnsts_capacity = 0;
  int nb_cnsts_struct = 0;
  int nb_cnsts_positivy = 0;
  int nb_cnsts_latency = 0;
  int block_num = 0;
  int block_size;
  int total_block_size = 0;
  int *isdiag = NULL;
  //  FILE *sdpout = fopen("SDPA-printf.tmp","w");
  int blocksz = 0;
  double *tempdiag = NULL;
  int matno = 0;
  int i = 0;
  int j = 0;
  int k = 0;

  /* 
   * CSDP library specific variables.
   */
  struct blockmatrix C;
  struct blockmatrix X, Z;
  double *y;
  double pobj, dobj;
  double *a;
  struct constraintmatrix *constraints;

  /*
   * Classic maxmin variables.
   */
  lmm_constraint_t cnst = NULL;
  lmm_element_t elem = NULL;
  xbt_swag_t cnst_list = NULL;
  xbt_swag_t var_list = NULL;
  xbt_swag_t elem_list = NULL;
  lmm_variable_t var = NULL;

  double tmp_k;
  struct sparseblock *p;
  char *tmp = NULL;
  FILE *stdout_sav;
  int ret;



  if (!(sys->modified))
    return;

  /* 
   * Initialize the var list variable with only the active variables. 
   * Associate an index in the swag variables.
   */
  var_list = &(sys->variable_set);
  i = 0;
  xbt_swag_foreach(var, var_list) {
    if (var->weight != 0.0)
      i++;
  }



  flows = i;
  DEBUG1("Variable set : %d", xbt_swag_size(var_list));
  DEBUG1("Flows : %d", flows);

  if (flows == 0) {
    return;
  }


  xbt_swag_foreach(var, var_list) {
    var->value = 0.0;
    if (var->weight) {
      var->index = i--;
    }
  }

  cnst_list = &(sys->active_constraint_set);
  DEBUG1("Active constraints : %d", xbt_swag_size(cnst_list));
  DEBUG1("Links : %d", links);

  /*
   * Those fields are the top level description of the platform furnished in the xml file.
   */
  links = xbt_swag_size(&(sys->active_constraint_set));

  /* 
   * This  number is found based on the tree structure explained on top.
   */
  tmp_k = (double) log((double) flows) / log(2.0);
  K = (int) ceil(tmp_k);
  //xbt_assert1(K!=0, "Invalide value of K (=%d) aborting.", K);


  /* 
   * The number of variables in the SDP program. 
   */
  nb_var = get_y(K, pow(2, K));
  DEBUG1("Number of variables in the SDP program : %d", nb_var);


  /*
   * Find the size of each group of constraints.
   */
  nb_cnsts_capacity = links + ((int) pow(2, K)) - flows;
  nb_cnsts_struct = (int) pow(2, K) - 1;
  nb_cnsts_positivy = (int) pow(2, K);
  nb_cnsts_latency = nb_var;


  /*
   * The total number of constraints.
   */
  nb_cnsts =
    nb_cnsts_capacity + nb_cnsts_struct + nb_cnsts_positivy +
    nb_cnsts_latency;
  CDEBUG1(surf_sdp_out, "Number of constraints = %d", nb_cnsts);
  DEBUG1("Number of constraints in the SDP program : %d", nb_cnsts);

  /*
   * Keep track of which blocks have off diagonal entries. 
   */
  isdiag = (int *) calloc((nb_cnsts + 1), sizeof(int));
  for (i = 1; i <= nb_cnsts; i++)
    isdiag[i] = 1;

  C.nblocks = nb_cnsts;
  C.blocks =
    (struct blockrec *) calloc((C.nblocks + 1), sizeof(struct blockrec));
  constraints =
    (struct constraintmatrix *) calloc((nb_var + 1),
                                       sizeof(struct constraintmatrix));

  for (i = 1; i <= nb_var; i++) {
    constraints[i].blocks = NULL;
  }

  a = (double *) calloc(nb_var + 1, sizeof(double));

  /*
   * Block sizes.
   */
  block_num = 1;
  block_size = 0;

  /*
   * For each constraint block do.
   */
  for (i = 1; i <= nb_cnsts; i++) {

    /*
     * Structured blocks are size 2 and all others are size 1.
     */
    if (i <= nb_cnsts_struct) {
      total_block_size += block_size = 2;
      DEBUG0("2 ");
    } else {
      total_block_size += block_size = 1;
      CDEBUG0(surf_sdp_out, "1 ");
    }

    /*
     * All blocks are matrices.
     */
    C.blocks[block_num].blockcategory = MATRIX;
    C.blocks[block_num].blocksize = block_size;
    C.blocks[block_num].data.mat =
      (double *) calloc(block_size * block_size, sizeof(double));

    block_num++;
  }

  CDEBUG0(surf_sdp_out, " ");


  /*
   * Creates de objective function array.
   */
  a = (double *) calloc((nb_var + 1), sizeof(double));

  for (i = 1; i <= nb_var; i++) {
    if (get_y(0, 1) == i) {
      //CDEBUG0(surf_sdp_out,"-1 ");
      a[i] = -1;
    } else {
      //CDEBUG0(surf_sdp_out,"0 ");
      a[i] = 0;
    }
  }


  /*
   * Structure contraint blocks.
   */
  block_num = 1;
  matno = 1;
  for (k = 1; k <= K; k++) {
    for (i = 1; i <= pow(2, k - 1); i++) {
      matno = get_y(k, 2 * i - 1);
      CDEBUG2(surf_sdp_out, "%d %d 1 1 1", matno, block_num);
      addentry(constraints, &C, matno, block_num, 1, 1, 1.0,
               C.blocks[block_num].blocksize);

      matno = get_y(k, 2 * i);
      CDEBUG2(surf_sdp_out, "%d %d 2 2 1", matno, block_num);
      addentry(constraints, &C, matno, block_num, 2, 2, 1.0,
               C.blocks[block_num].blocksize);

      matno = get_y(k - 1, i);
      CDEBUG2(surf_sdp_out, "%d %d 1 2 1", matno, block_num);
      addentry(constraints, &C, matno, block_num, 1, 2, 1.0,
               C.blocks[block_num].blocksize);

      matno = get_y(k - 1, i);
      CDEBUG2(surf_sdp_out, "%d %d 2 1 1", matno, block_num);
      addentry(constraints, &C, matno, block_num, 2, 1, 1.0,
               C.blocks[block_num].blocksize);

      isdiag[block_num] = 0;
      block_num++;
    }
  }


  /*
   * Capacity constraint block.
   */
  xbt_swag_foreach(cnst, cnst_list) {

    CDEBUG2(surf_sdp_out, "0 %d 1 1 %f", block_num, -(cnst->bound));
    addentry(constraints, &C, 0, block_num, 1, 1, -(cnst->bound),
             C.blocks[block_num].blocksize);

    elem_list = &(cnst->element_set);
    xbt_swag_foreach(elem, elem_list) {
      if (elem->variable->weight <= 0)
        break;
      matno = get_y(K, elem->variable->index);
      CDEBUG3(surf_sdp_out, "%d %d 1 1 %f", matno, block_num, -(elem->value));
      addentry(constraints, &C, matno, block_num, 1, 1, -(elem->value),
               C.blocks[block_num].blocksize);

    }
    block_num++;
  }


  /*
   * Positivy constraint blocks.
   */
  for (i = 1; i <= pow(2, K); i++) {
    matno = get_y(K, i);
    CDEBUG2(surf_sdp_out, "%d %d 1 1 1", matno, block_num);
    addentry(constraints, &C, matno, block_num, 1, 1, 1.0,
             C.blocks[block_num].blocksize);
    block_num++;
  }
  /*
   * Latency constraint blocks.
   */
  xbt_swag_foreach(var, var_list) {
    var->value = 0.0;
    if (var->weight && var->bound > 0) {
      matno = get_y(K, var->index);
      CDEBUG3(surf_sdp_out, "%d %d 1 1 %f", matno, block_num, var->bound);
      addentry(constraints, &C, matno, block_num, 1, 1, var->bound,
               C.blocks[block_num].blocksize);
    }
  }

  /*
   * At this point, we'll stop to recognize whether any of the blocks
   * are "hidden LP blocks"  and correct the block type if needed.
   */
  for (i = 1; i <= nb_cnsts; i++) {
    if ((C.blocks[i].blockcategory != DIAG) &&
        (isdiag[i] == 1) && (C.blocks[i].blocksize > 1)) {
      /*
       * We have a hidden diagonal block!
       */

      blocksz = C.blocks[i].blocksize;
      tempdiag = (double *) calloc((blocksz + 1), sizeof(double));
      for (j = 1; j <= blocksz; j++)
        tempdiag[j] = C.blocks[i].data.mat[ijtok(j, j, blocksz)];
      free(C.blocks[i].data.mat);
      C.blocks[i].data.vec = tempdiag;
      C.blocks[i].blockcategory = DIAG;
    };
  };


  /*
   * Next, setup issparse and NULL out all nextbyblock pointers.
   */
  p = NULL;
  for (i = 1; i <= k; i++) {
    p = constraints[i].blocks;
    while (p != NULL) {
      /*
       * First, set issparse.
       */
      if (((p->numentries) > 0.25 * (p->blocksize))
          && ((p->numentries) > 15)) {
        p->issparse = 0;
      } else {
        p->issparse = 1;
      };

      if (C.blocks[p->blocknum].blockcategory == DIAG)
        p->issparse = 1;

      /*
       * Setup the cross links.
       */

      p->nextbyblock = NULL;
      p = p->next;
    };
  };


  /*
   * Create cross link reference.
   */
  create_cross_link(constraints, nb_var);


  /*
   * Debuging print problem in SDPA format.
   */
  if (XBT_LOG_ISENABLED(surf_sdp, xbt_log_priority_debug)) {
    DEBUG0("Printing SDPA...");
    tmp = strdup("SURF-PROPORTIONNAL.sdpa");
    write_prob(tmp, total_block_size, nb_var, C, a, constraints);
  }

  /*
   * Initialize parameters.
   */
  DEBUG0("Initializing solution...");
  initsoln(total_block_size, nb_var, C, a, constraints, &X, &y, &Z);



  /*
   * Call the solver.
   */
  DEBUG0("Calling the solver...");
  stdout_sav = stdout;
  stdout = fopen("/dev/null", "w");
  ret =
    easy_sdp(total_block_size, nb_var, C, a, constraints, 0.0, &X, &y,
             &Z, &pobj, &dobj);
  fclose(stdout);
  stdout = stdout_sav;

  switch (ret) {
  case 0:
  case 1:
    DEBUG0("SUCCESS The problem is primal infeasible");
    break;

  case 2:
    DEBUG0("SUCCESS The problem is dual infeasible");
    break;

  case 3:
    DEBUG0
      ("Partial SUCCESS A solution has been found, but full accuracy was not achieved. One or more of primal infeasibility, dual infeasibility, or relative duality gap are larger than their tolerances, but by a factor of less than 1000.");
    break;

  case 4:
    DEBUG0("Failure. Maximum number of iterations reached.");
    break;

  case 5:
    DEBUG0("Failure. Stuck at edge of primal feasibility.");
    break;

  }

  if (XBT_LOG_ISENABLED(surf_sdp, xbt_log_priority_debug)) {
    tmp = strdup("SURF-PROPORTIONNAL.sol");
    write_sol(tmp, total_block_size, nb_var, X, y, Z);
  }

  /*
   * Write out the solution if necessary.
   */
  xbt_swag_foreach(cnst, cnst_list) {

    elem_list = &(cnst->element_set);
    xbt_swag_foreach(elem, elem_list) {
      if (elem->variable->weight <= 0)
        break;

      i = (int) get_y(K, elem->variable->index);
      elem->variable->value = y[i];

    }
  }


  /*
   * Free up memory.
   */
  free_prob(total_block_size, nb_var, C, a, constraints, X, y, Z);

  free(isdiag);
  free(tempdiag);
  free(tmp);

  sys->modified = 0;

  if (XBT_LOG_ISENABLED(surf_sdp, xbt_log_priority_debug)) {
    lmm_print(sys);
  }

}


/*
 * Create the cross_link reference in order to have a byblock list.
 */
void create_cross_link(struct constraintmatrix *myconstraints, int k)
{

  int i, j;
  int blk;
  struct sparseblock *p;
  struct sparseblock *q;

  struct sparseblock *prev;

  /*
   * Now, cross link.
   */
  prev = NULL;
  for (i = 1; i <= k; i++) {
    p = myconstraints[i].blocks;
    while (p != NULL) {
      if (p->nextbyblock == NULL) {
        blk = p->blocknum;

        /*
         * link in the remaining blocks.
         */
        for (j = i + 1; j <= k; j++) {
          q = myconstraints[j].blocks;

          while (q != NULL) {
            if (q->blocknum == p->blocknum) {
              if (p->nextbyblock == NULL) {
                p->nextbyblock = q;
                q->nextbyblock = NULL;
                prev = q;
              } else {
                prev->nextbyblock = q;
                q->nextbyblock = NULL;
                prev = q;
              }
              break;
            }
            q = q->next;
          }
        }
      }
      p = p->next;
    }
  }
}




void addentry(struct constraintmatrix *constraints,
              struct blockmatrix *C,
              int matno,
              int blkno, int indexi, int indexj, double ent, int blocksize)
{
  struct sparseblock *p;
  struct sparseblock *p_sav;

  p = constraints[matno].blocks;

  if (matno != 0.0) {
    if (p == NULL) {
      /*
       * We haven't yet allocated any blocks.
       */
      p = (struct sparseblock *) calloc(1, sizeof(struct sparseblock));

      //two entries because this library ignores indices starting in zerox
      p->constraintnum = matno;
      p->blocknum = blkno;
      p->numentries = 1;
      p->next = NULL;

      p->entries = calloc(p->numentries + 1, sizeof(double));
      p->iindices = calloc(p->numentries + 1, sizeof(int));
      p->jindices = calloc(p->numentries + 1, sizeof(int));

      p->entries[p->numentries] = ent;
      p->iindices[p->numentries] = indexi;
      p->jindices[p->numentries] = indexj;

      p->blocksize = blocksize;

      constraints[matno].blocks = p;
    } else {
      /*
       * We have some existing blocks.  See whether this block is already
       * in the chain.
       */
      while (p != NULL) {
        if (p->blocknum == blkno) {
          /*
           * Found the right block.
           */
          p->constraintnum = matno;
          p->blocknum = blkno;
          p->numentries = p->numentries + 1;

          p->entries =
            realloc(p->entries, (p->numentries + 1) * sizeof(double));
          p->iindices =
            realloc(p->iindices, (p->numentries + 1) * sizeof(int));
          p->jindices =
            realloc(p->jindices, (p->numentries + 1) * sizeof(int));

          p->entries[p->numentries] = ent;
          p->iindices[p->numentries] = indexi;
          p->jindices[p->numentries] = indexj;

          return;
        }
        p_sav = p;
        p = p->next;
      }

      /*
       * If we get here, we have a non-empty structure but not the right block
       * inside hence create a new structure.
       */

      p = (struct sparseblock *) calloc(1, sizeof(struct sparseblock));

      //two entries because this library ignores indices starting in zerox
      p->constraintnum = matno;
      p->blocknum = blkno;
      p->numentries = 1;
      p->next = NULL;

      p->entries = calloc(p->numentries + 1, sizeof(double));
      p->iindices = calloc(p->numentries + 1, sizeof(int));
      p->jindices = calloc(p->numentries + 1, sizeof(int));

      p->entries[p->numentries] = ent;
      p->iindices[p->numentries] = indexi;
      p->jindices[p->numentries] = indexj;

      p->blocksize = blocksize;

      p_sav->next = p;
    }
  } else {
    if (ent != 0.0) {
      int blksz = C->blocks[blkno].blocksize;
      if (C->blocks[blkno].blockcategory == DIAG) {
        C->blocks[blkno].data.vec[indexi] = ent;
      } else {
        C->blocks[blkno].data.mat[ijtok(indexi, indexj, blksz)] = ent;
        C->blocks[blkno].data.mat[ijtok(indexj, indexi, blksz)] = ent;
      };
    };

  }
}
