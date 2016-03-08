/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "colls_private.h"
#if HAVE_MC
#include "src/mc/mc_private.h"
#endif
#include <float.h>

//attempt to do a quick autotuning version of the collective,

#define TRACE_AUTO_COLL(cat) if (TRACE_is_enabled()){\
        type_t type = PJ_type_get_or_null (#cat, PJ_type_get_root());\
         if (!type){\
             type=PJ_type_event_new(#cat, PJ_type_get_root());\
         }\
         char cont_name[25];\
         sprintf(cont_name, "rank-%d", smpi_process_index());\
         val_t value = PJ_value_get_or_new(mpi_coll_##cat##_description[i].name,"1.0 1.0 1.0", type);\
         new_pajeNewEvent (SIMIX_get_clock(), PJ_container_get(cont_name), type, value);\
      }


#define AUTOMATIC_COLL_BENCH(cat, ret, args, args2)\
    ret smpi_coll_tuned_ ## cat ## _ ## automatic(COLL_UNPAREN args)\
{\
  double time1, time2, time_min=DBL_MAX;\
  volatile int min_coll=-1, global_coll=-1;\
  volatile int i;\
  xbt_ex_t ex;\
  double buf_in, buf_out, max_min=DBL_MAX;\
  for (i = 0; mpi_coll_##cat##_description[i].name; i++){\
      if(!strcmp(mpi_coll_##cat##_description[i].name, "automatic"))continue;\
      if(!strcmp(mpi_coll_##cat##_description[i].name, "default"))continue;\
      smpi_mpi_barrier(comm);\
      TRACE_AUTO_COLL(cat)\
      time1 = SIMIX_get_clock();\
      TRY{\
      ((int (*) args)\
          mpi_coll_##cat##_description[i].coll) args2 ;\
      }\
      CATCH(ex) {\
        xbt_ex_free(ex);\
        continue;\
      }\
      time2 = SIMIX_get_clock();\
      buf_out=time2-time1;\
      smpi_mpi_reduce((void*)&buf_out,(void*)&buf_in, 1, MPI_DOUBLE, MPI_MAX, 0,comm );\
      if(time2-time1<time_min){\
          min_coll=i;\
          time_min=time2-time1;\
      }\
      if(smpi_comm_rank(comm)==0){\
          if(buf_in<max_min){\
              max_min=buf_in;\
              global_coll=i;\
          }\
      }\
  }\
  if(smpi_comm_rank(comm)==0){\
      XBT_WARN("For rank 0, the quickest was %s : %f , but global was %s : %f at max",mpi_coll_##cat##_description[min_coll].name, time_min,mpi_coll_##cat##_description[global_coll].name, max_min);\
  }else\
  XBT_WARN("The quickest %s was %s on rank %d and took %f",#cat,mpi_coll_##cat##_description[min_coll].name, smpi_comm_rank(comm), time_min);\
  return (min_coll!=-1)?MPI_SUCCESS:MPI_ERR_INTERN;\
}\


COLL_APPLY(AUTOMATIC_COLL_BENCH, COLL_ALLGATHERV_SIG, (send_buff, send_count, send_type, recv_buff, recv_count, recv_disps, recv_type, comm));
COLL_APPLY(AUTOMATIC_COLL_BENCH, COLL_ALLREDUCE_SIG, (sbuf, rbuf, rcount, dtype, op, comm));
COLL_APPLY(AUTOMATIC_COLL_BENCH, COLL_GATHER_SIG, (send_buff, send_count, send_type, recv_buff, recv_count, recv_type, root, comm));
COLL_APPLY(AUTOMATIC_COLL_BENCH, COLL_ALLGATHER_SIG, (send_buff,send_count,send_type,recv_buff,recv_count,recv_type,comm));
COLL_APPLY(AUTOMATIC_COLL_BENCH, COLL_ALLTOALL_SIG,(send_buff, send_count, send_type, recv_buff, recv_count, recv_type,comm));
COLL_APPLY(AUTOMATIC_COLL_BENCH, COLL_ALLTOALLV_SIG, (send_buff, send_counts, send_disps, send_type, recv_buff, recv_counts, recv_disps, recv_type, comm));
COLL_APPLY(AUTOMATIC_COLL_BENCH, COLL_BCAST_SIG , (buf, count, datatype, root, comm));
COLL_APPLY(AUTOMATIC_COLL_BENCH, COLL_REDUCE_SIG,(buf,rbuf, count, datatype, op, root, comm));
COLL_APPLY(AUTOMATIC_COLL_BENCH, COLL_REDUCE_SCATTER_SIG ,(sbuf,rbuf, rcounts,dtype,op,comm));
COLL_APPLY(AUTOMATIC_COLL_BENCH, COLL_SCATTER_SIG ,(sendbuf, sendcount, sendtype,recvbuf, recvcount, recvtype,root, comm));
COLL_APPLY(AUTOMATIC_COLL_BENCH, COLL_BARRIER_SIG,(comm));
