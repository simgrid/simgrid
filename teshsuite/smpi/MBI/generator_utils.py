# Copyright 2021-2022. The MBI project. All rights reserved.
# This program is free software; you can redistribute it and/or modify it under the terms of the license (GNU GPL).

# This is a simple templating system, dedicated to the systematic generation of MPI source code

import os
import re

# Collectives
coll = ['MPI_Barrier','MPI_Bcast', 'MPI_Reduce', 'MPI_Gather', 'MPI_Scatter', 'MPI_Scan', 'MPI_Exscan', 'MPI_Allgather', 'MPI_Allreduce', 'MPI_Allgatherv', 'MPI_Alltoall', 'MPI_Alltoallv']
icoll = ['MPI_Ibcast', 'MPI_Ireduce', 'MPI_Igather', 'MPI_Iscatter', 'MPI_Iscan', 'MPI_Iexscan', 'MPI_Iallgather', 'MPI_Iallreduce', 'MPI_Iallgatherv', 'MPI_Ialltoall', 'MPI_Ialltoallv']
ibarrier = ['MPI_Ibarrier']
coll4op = ['MPI_Reduce', 'MPI_Allreduce']
icoll4op = ['MPI_Ireduce', 'MPI_Iallreduce']
coll4root =  ['MPI_Reduce', 'MPI_Bcast', 'MPI_Gather', 'MPI_Scatter']
icoll4root = ['MPI_Ireduce', 'MPI_Ibcast', 'MPI_Igather', 'MPI_Iscatter']
pcoll = []
tcoll = ['MPI_Comm_split', 'MPI_Op_create', 'MPI_Comm_dup', 'MPI_Type_contiguous', 'MPI_Comm_create', 'MPI_Group_excl'] # MPI_Comm_dup removed
tcoll4color = ['MPI_Comm_split']
tcoll4topo = ['MPI_Cart_get']

# P2P
allsend = ['MPI_Send', 'MPI_Isend', 'MPI_Ssend', 'MPI_Bsend','MPI_Send_init']
allrecv = ['MPI_Recv', 'MPI_Irecv', 'MPI_Recv_init']
send = ['MPI_Send']
ssend = ['MPI_Ssend']
bsend = ['MPI_Bsend']
isend = ['MPI_Isend']
psend = ['MPI_Send_init']
recv = ['MPI_Recv']
irecv = ['MPI_Irecv']
precv = ['MPI_Recv_init']
probe = ['MPI_Probe']

# RMA
epoch = ['MPI_Win_fence', 'MPI_Win_lock', 'MPI_Win_lock_all']
rma = ['MPI_Get', 'MPI_Put']
get = ['MPI_Get']
put = ['MPI_Put']
store = ['store']
load = ['load']
rstore = ['rstore']
rload = ['rload']
loadstore = ['loadstore']


# setup
init = {}
start = {}
operation = {}
fini = {}
free = {}
write = {}
error = {}
epoch = {}
finEpoch = {}


### COLL:basic

init['MPI_Bcast'] = lambda n: f'int buf{n}[buff_size];'
start['MPI_Bcast'] = lambda n: ""
operation['MPI_Bcast'] = lambda n: f'MPI_Bcast(buf{n}, buff_size, type, root, newcom);'
fini['MPI_Bcast'] = lambda n: ""
free['MPI_Bcast'] = lambda n: ""
write['MPI_Bcast'] = lambda n: ""

init['MPI_Barrier'] = lambda n: ""
start['MPI_Barrier'] = lambda n: ""
operation['MPI_Barrier'] = lambda n: 'MPI_Barrier(newcom);'
fini['MPI_Barrier'] = lambda n: ""
free['MPI_Barrier'] = lambda n: ""
write['MPI_Barrier'] = lambda n: ""

init['MPI_Reduce'] = lambda n: f"int sum{n}, val{n} = 1;"
start['MPI_Reduce'] = lambda n: ""
operation['MPI_Reduce'] = lambda n: f"MPI_Reduce(&val{n}, &sum{n}, 1, type, op, root, newcom);"
fini['MPI_Reduce'] = lambda n: ""
free['MPI_Reduce'] = lambda n: ""
write['MPI_Reduce'] = lambda n: ""

init['MPI_Gather'] = lambda n: f"int val{n}=1, buf{n}[buff_size];"
start['MPI_Gather'] = lambda n: ""
operation['MPI_Gather'] = lambda n: f"MPI_Gather(&val{n}, 1, type, buf{n},1, type, root, newcom);"
fini['MPI_Gather'] = lambda n: ""
free['MPI_Gather'] = lambda n: ""
write['MPI_Gather'] = lambda n: ""

init['MPI_Scatter'] = lambda n: f"int val{n}, buf{n}[buff_size];"
start['MPI_Scatter'] = lambda n: ""
operation['MPI_Scatter'] = lambda n: f"MPI_Scatter(&buf{n}, 1, type, &val{n}, 1, type, root, newcom);"
fini['MPI_Scatter'] = lambda n: ""
free['MPI_Scatter'] = lambda n: ""
write['MPI_Scatter'] = lambda n: ""

init['MPI_Allreduce'] = lambda n: f"int sum{n}, val{n} = 1;"
start['MPI_Allreduce'] = lambda n: ""
operation['MPI_Allreduce'] = lambda n: f"MPI_Allreduce(&val{n}, &sum{n}, 1, type, op, newcom);"
fini['MPI_Allreduce'] = lambda n: ""
free['MPI_Allreduce'] = lambda n: ""
write['MPI_Allreduce'] = lambda n: ""

init['MPI_Scan'] = lambda n: f"int outbuf{n}[buff_size], inbuf{n}[buff_size];"
start['MPI_Scan'] = lambda n: ""
operation['MPI_Scan'] = lambda n: f"MPI_Scan(&outbuf{n}, inbuf{n}, buff_size, type, op, newcom);"
fini['MPI_Scan'] = lambda n: ""
free['MPI_Scan'] = lambda n: ""
write['MPI_Scan'] = lambda n: ""

init['MPI_Exscan'] = lambda n: f"int outbuf{n}[buff_size], inbuf{n}[buff_size];"
start['MPI_Exscan'] = lambda n: ""
operation['MPI_Exscan'] = lambda n: f"MPI_Exscan(&outbuf{n}, inbuf{n}, buff_size, type, op, newcom);"
fini['MPI_Exscan'] = lambda n: ""
free['MPI_Exscan'] = lambda n: ""
write['MPI_Exscan'] = lambda n: ""

init['MPI_Allgather'] = lambda n: f"int val{n}=1, *rbuf{n} = (int*)malloc(dbs);"
start['MPI_Allgather'] = lambda n: ""
operation['MPI_Allgather'] = lambda n: f"MPI_Allgather(&val{n}, 1, type, rbuf{n}, 1, type, newcom);"
fini['MPI_Allgather'] = lambda n: ""
free['MPI_Allgather'] = lambda n: f"free(rbuf{n});"
write['MPI_Allgather'] = lambda n: ""

init['MPI_Alltoallv'] = lambda n: (f"int *sbuf{n}=(int*)malloc(dbs*2), *rbuf{n}=(int*)malloc(dbs*2), *scounts{n}=(int*)malloc(dbs), *rcounts{n}=(int*)malloc(dbs), *sdispls{n}=(int*)malloc(dbs), *rdispls{n}=(int*)malloc(dbs);\n"
  +  "  for (int i = 0; i < nprocs; i++) {\n"
  + f"    scounts{n}[i] = 2;\n"
  + f"    rcounts{n}[i] = 2;\n"
  + f"    sdispls{n}[i] = (nprocs - (i + 1)) * 2;\n"
  + f"    rdispls{n}[i] = i * 2;\n"
  +  "  }")
start['MPI_Alltoallv'] = lambda n: ""
operation['MPI_Alltoallv'] = lambda n: f"MPI_Alltoallv(sbuf{n}, scounts{n}, sdispls{n}, type, rbuf{n}, rcounts{n}, rdispls{n}, type, newcom);"
fini['MPI_Alltoallv'] = lambda n: ""
free['MPI_Alltoallv'] = lambda n: f"free(sbuf{n});free(rbuf{n});free(scounts{n});free(rcounts{n});free(sdispls{n});free(rdispls{n});"
write['MPI_Alltoallv'] = lambda n: ""

init['MPI_Alltoall'] = lambda n: f"int *sbuf{n} = (int*)malloc(dbs), *rbuf{n} = (int*)malloc(dbs);"
start['MPI_Alltoall'] = lambda n: ""
operation['MPI_Alltoall'] = lambda n: f"MPI_Alltoall(sbuf{n}, 1, type, rbuf{n}, 1, type, newcom);"
fini['MPI_Alltoall'] = lambda n: ""
free['MPI_Alltoall'] = lambda n: f"free(sbuf{n});free(rbuf{n});"
write['MPI_Alltoall'] = lambda n: ""

init['MPI_Allgatherv'] = lambda n: (f"int *rbuf{n} = (int*)malloc(dbs*2), *rcounts{n}=(int*)malloc(dbs),  *displs{n}=(int*)malloc(dbs);\n"
  +  "  for (int i = 0; i < nprocs; i++) {\n"
  + f"    rcounts{n}[i] = 1;\n"
  + f"    displs{n}[i] = 2 * (nprocs - (i + 1));\n"
  +  "  }")
start['MPI_Allgatherv'] = lambda n: ""
operation['MPI_Allgatherv'] = lambda n: f"MPI_Allgatherv(&rank, 1, type, rbuf{n}, rcounts{n}, displs{n}, type, newcom);"
fini['MPI_Allgatherv'] = lambda n: ""
free['MPI_Allgatherv'] = lambda n: f"free(rbuf{n});free(rcounts{n});free(displs{n});"
write['MPI_Allgatherv'] = lambda n: ""


### COLL:nonblocking

init['MPI_Ibarrier'] = lambda n: f"MPI_Request req{n}=MPI_REQUEST_NULL; MPI_Status stat{n};"
start['MPI_Ibarrier'] = lambda n: ""
operation['MPI_Ibarrier'] = lambda n: f'MPI_Ibarrier(newcom, &req{n});'
fini['MPI_Ibarrier'] = lambda n: f"MPI_Wait(&req{n}, &stat{n});"
free['MPI_Ibarrier'] = lambda n: f'if(req{n} != MPI_REQUEST_NULL) MPI_Request_free(&req{n});'
write['MPI_Ibarrier'] = lambda n: ""

init['MPI_Ireduce'] = lambda n: f"MPI_Request req{n}=MPI_REQUEST_NULL; MPI_Status stat{n}; int sum{n}, val{n} = 1;"
start['MPI_Ireduce'] = lambda n: ""
operation['MPI_Ireduce'] = lambda n: f"MPI_Ireduce(&val{n}, &sum{n}, 1, type, op, root, newcom, &req{n});"
fini['MPI_Ireduce'] = lambda n: f"MPI_Wait(&req{n}, &stat{n});"
free['MPI_Ireduce'] = lambda n: f'if(req{n} != MPI_REQUEST_NULL) MPI_Request_free(&req{n});'
write['MPI_Ireduce'] = lambda n: f"sum{n}++;"

init['MPI_Iallreduce'] = lambda n: f'MPI_Request req{n}=MPI_REQUEST_NULL;MPI_Status stat{n}; int sum{n}, val{n} = 1;'
start['MPI_Iallreduce'] = lambda n: ""
operation['MPI_Iallreduce'] = lambda n: f'MPI_Iallreduce(&val{n}, &sum{n}, 1, type, op, newcom, &req{n});'
fini['MPI_Iallreduce'] = lambda n: f'MPI_Wait(&req{n}, &stat{n});'
free['MPI_Iallreduce'] = lambda n: f"if(req{n} != MPI_REQUEST_NULL) MPI_Request_free(&req{n});"
write['MPI_Iallreduce'] = lambda n: f"sum{n}++;"

init['MPI_Ibcast'] = lambda n: f'MPI_Request req{n}=MPI_REQUEST_NULL; MPI_Status sta{n};int buf{n}[buff_size];'
start['MPI_Ibcast'] = lambda n: ""
operation['MPI_Ibcast'] = lambda n: f'MPI_Ibcast(buf{n}, buff_size, type, root, newcom, &req{n});'
fini['MPI_Ibcast'] = lambda n: f"MPI_Wait(&req{n},&sta{n});"
free['MPI_Ibcast'] = lambda n: f'if(req{n} != MPI_REQUEST_NULL) MPI_Request_free(&req{n});'
write['MPI_Ibcast'] = lambda n: f'buf{n}[0]++;'

init['MPI_Igather'] = lambda n: f"int val{n}=1, buf{n}[buff_size];MPI_Request req{n}=MPI_REQUEST_NULL;MPI_Status sta{n};"
start['MPI_Igather'] = lambda n: ""
operation['MPI_Igather'] = lambda n: f'MPI_Igather(&val{n}, 1, type, &buf{n},1, type, root, newcom, &req{n});'
write['MPI_Igather'] = lambda n: f'val{n}=3;'
fini['MPI_Igather'] = lambda n: f'MPI_Wait(&req{n},&sta{n});'
free['MPI_Igather'] = lambda n: f'if(req{n} != MPI_REQUEST_NULL) MPI_Request_free(&req{n});'

init['MPI_Iscatter'] = lambda n: f"MPI_Request req{n}=MPI_REQUEST_NULL;MPI_Status sta{n};int val{n}, buf{n}[buff_size];"
start['MPI_Iscatter'] = lambda n: ""
operation['MPI_Iscatter'] = lambda n: f"MPI_Iscatter(&buf{n}, 1, type, &val{n}, 1, type, root, newcom,&req{n});"
fini['MPI_Iscatter'] = lambda n: f"MPI_Wait(&req{n},&sta{n});"
free['MPI_Iscatter'] = lambda n: f'if(req{n} != MPI_REQUEST_NULL) MPI_Request_free(&req{n});'
write['MPI_Iscatter'] = lambda n: f'buf{n}[0]++;'

init['MPI_Iscan'] = lambda n: f"MPI_Request req{n}=MPI_REQUEST_NULL;MPI_Status sta{n}; int outbuf{n}[buff_size], inbuf{n}[buff_size];"
start['MPI_Iscan'] = lambda n: ""
operation['MPI_Iscan'] = lambda n: f"MPI_Iscan(&outbuf{n}, inbuf{n}, buff_size, type, op, newcom,&req{n});"
fini['MPI_Iscan'] = lambda n: f"MPI_Wait(&req{n},&sta{n});"
free['MPI_Iscan'] = lambda n: f'if(req{n} != MPI_REQUEST_NULL) MPI_Request_free(&req{n});'
write['MPI_Iscan'] = lambda n: f'outbuf{n}[0]++;'

init['MPI_Iexscan'] = lambda n: f"MPI_Request req{n}=MPI_REQUEST_NULL;MPI_Status sta{n};int outbuf{n}[buff_size], inbuf{n}[buff_size];"
start['MPI_Iexscan'] = lambda n: ""
operation['MPI_Iexscan'] = lambda n: f"MPI_Iexscan(&outbuf{n}, inbuf{n}, buff_size, type, op, newcom,&req{n});"
fini['MPI_Iexscan'] = lambda n: f"MPI_Wait(&req{n},&sta{n});"
free['MPI_Iexscan'] = lambda n: f'if(req{n} != MPI_REQUEST_NULL) MPI_Request_free(&req{n});'
write['MPI_Iexscan'] = lambda n: f'outbuf{n}[0]++;'

init['MPI_Iallgather'] = lambda n: f"MPI_Request req{n}=MPI_REQUEST_NULL;MPI_Status sta{n};int val{n}=1, *rbuf{n} = (int*)malloc(dbs);"
start['MPI_Iallgather'] = lambda n: ""
operation['MPI_Iallgather'] = lambda n: f"MPI_Iallgather(&val{n}, 1, type, rbuf{n}, 1, type, newcom,&req{n});"
fini['MPI_Iallgather'] = lambda n: f"MPI_Wait(&req{n},&sta{n});"
free['MPI_Iallgather'] = lambda n: f"free(rbuf{n});"
write['MPI_Iallgather'] = lambda n: f'val{n}++;'

init['MPI_Iallgatherv'] = lambda n: (f"MPI_Request req{n}=MPI_REQUEST_NULL;MPI_Status sta{n};int *rbuf{n} = (int*)malloc(dbs*2), *rcounts{n}=(int*)malloc(dbs),  *displs{n}=(int*)malloc(dbs);\n"
  +  "  for (int i = 0; i < nprocs; i++) {\n"
  + f"    rcounts{n}[i] = 1;\n"
  + f"    displs{n}[i] = 2 * (nprocs - (i + 1));\n"
  +  "  }")
start['MPI_Iallgatherv'] = lambda n: ""
operation['MPI_Iallgatherv'] = lambda n: f"MPI_Iallgatherv(&rank, 1, type, rbuf{n}, rcounts{n}, displs{n}, type, newcom,&req{n});"
fini['MPI_Iallgatherv'] = lambda n: f"MPI_Wait(&req{n},&sta{n});"
free['MPI_Iallgatherv'] = lambda n: f"free(rbuf{n});free(rcounts{n});free(displs{n});"
write['MPI_Iallgatherv'] = lambda n: f"rbuf{n}[0]++;"

init['MPI_Ialltoall'] = lambda n: f"MPI_Request req{n}=MPI_REQUEST_NULL;MPI_Status sta{n};int *sbuf{n} = (int*)malloc(dbs), *rbuf{n} = (int*)malloc(dbs);"
start['MPI_Ialltoall'] = lambda n: ""
operation['MPI_Ialltoall'] = lambda n: f"MPI_Ialltoall(sbuf{n}, 1, type, rbuf{n}, 1, type, newcom, &req{n});"
fini['MPI_Ialltoall'] = lambda n: f"MPI_Wait(&req{n},&sta{n});"
free['MPI_Ialltoall'] = lambda n: f"free(sbuf{n});free(rbuf{n});"
write['MPI_Ialltoall'] = lambda n: f"rbuf{n}[0]++;"

init['MPI_Ialltoallv'] = lambda n: (f"MPI_Request req{n}=MPI_REQUEST_NULL;MPI_Status sta{n};int *sbuf{n}=(int*)malloc(dbs*2), *rbuf{n}=(int*)malloc(dbs*2), *scounts{n}=(int*)malloc(dbs), *rcounts{n}=(int*)malloc(dbs), *sdispls{n}=(int*)malloc(dbs), *rdispls{n}=(int*)malloc(dbs);\n"
  +  "  for (int i = 0; i < nprocs; i++) {\n"
  + f"    scounts{n}[i] = 2;\n"
  + f"    rcounts{n}[i] = 2;\n"
  + f"    sdispls{n}[i] = (nprocs - (i + 1)) * 2;\n"
  + f"    rdispls{n}[i] = i * 2;\n"
  +  "  }")
start['MPI_Ialltoallv'] = lambda n: ""
operation['MPI_Ialltoallv'] = lambda n: f"MPI_Ialltoallv(sbuf{n}, scounts{n}, sdispls{n}, type, rbuf{n}, rcounts{n}, rdispls{n}, type, newcom,&req{n});"
fini['MPI_Ialltoallv'] = lambda n: f"MPI_Wait(&req{n},&sta{n});"
free['MPI_Ialltoallv'] = lambda n: f"free(sbuf{n});free(rbuf{n});free(scounts{n});free(rcounts{n});free(sdispls{n});free(rdispls{n});"
write['MPI_Ialltoallv'] = lambda n: f"rbuf{n}[0]++;"

### COLL:persistent



### COLL:tools

init['MPI_Comm_split'] = lambda n: f'MPI_Comm com[size]; color = rank % 2; int key = 1;'
start['MPI_Comm_split'] = lambda n: ""
operation['MPI_Comm_split'] = lambda n: 'MPI_Comm_split(MPI_COMM_WORLD,color,key, &com[j]);'
error['MPI_Comm_split'] = 'CommunicatorLeak'
fini['MPI_Comm_split'] = lambda n: "if(com[j] != MPI_COMM_NULL) MPI_Comm_free(&com[j]);"
free['MPI_Comm_split'] = lambda n: ""


init['MPI_Cart_get'] = lambda n: ""
start['MPI_Cart_get'] = lambda n: ""
operation['MPI_Cart_get'] = lambda n: f'MPI_Cart_get(newcom, 2, dims, periods, coords);'
write['MPI_Cart_get'] = lambda n: ""
fini['MPI_Cart_get'] = lambda n: ""
free['MPI_Cart_get'] = lambda n: ""


init['MPI_Op_create'] = lambda n: 'MPI_Op op[size];'
operation['MPI_Op_create'] = lambda n: 'MPI_Op_create((MPI_User_function *)myOp, 0, &op[j]);'
error['MPI_Op_create'] = 'OperatorLeak'
fini['MPI_Op_create'] = lambda n: "MPI_Op_free(&op[j]);"
free['MPI_Op_create'] = lambda n: ""

init['MPI_Comm_group'] = lambda n: 'MPI_Group grp[size];'
operation['MPI_Comm_group'] = lambda n: 'MPI_Comm_group(MPI_COMM_WORLD, &grp[j]);'
error['MPI_Comm_group'] = 'GroupLeak'
fini['MPI_Comm_group'] = lambda n: "MPI_Group_free(&grp[j]);"
free['MPI_Comm_group'] = lambda n: ""

init['MPI_Group_excl'] = lambda n: 'MPI_Group worldgroup, grp[size];\n MPI_Comm_group(MPI_COMM_WORLD, &worldgroup);'
operation['MPI_Group_excl'] = lambda n: 'MPI_Group_excl(worldgroup, 1, &rank, &grp[j]);'
error['MPI_Group_excl'] = 'GroupLeak'
fini['MPI_Group_excl'] = lambda n: "MPI_Group_free(&grp[j]);"
free['MPI_Group_excl'] = lambda n: "MPI_Group_free(&worldgroup);"

init['MPI_Comm_create'] = lambda n: 'MPI_Comm com[size]; MPI_Group grp[size];'
operation['MPI_Comm_create'] = lambda n: 'MPI_Comm_group(MPI_COMM_WORLD, &grp[j]);\n MPI_Comm_create(MPI_COMM_WORLD, grp[j], &com[j]);\n MPI_Group_free(&grp[j]);'
error['MPI_Comm_create'] = 'CommunicatorLeak'
fini['MPI_Comm_create'] = lambda n: "MPI_Comm_free(&com[j]);"
free['MPI_Comm_create'] = lambda n: ""

init['MPI_Comm_dup'] = lambda n: f'MPI_Comm com[size];'
operation['MPI_Comm_dup'] = lambda n: 'MPI_Comm_dup(MPI_COMM_WORLD, &com[j]);'
error['MPI_Comm_dup'] = 'CommunicatorLeak'
fini['MPI_Comm_dup'] = lambda n: "MPI_Comm_free(&com[j]);"
free['MPI_Comm_dup'] = lambda n: ""

init['MPI_Type_contiguous'] = lambda n: 'MPI_Datatype type[size];'
operation['MPI_Type_contiguous'] = lambda n: 'MPI_Type_contiguous(2, MPI_DOUBLE, &type[j]);'
error['MPI_Type_contiguous'] = 'TypeLeak'
fini['MPI_Type_contiguous'] = lambda n: "MPI_Type_free(&type[j]);"
free['MPI_Type_contiguous'] = lambda n: ""




### P2P:basic

init['MPI_Send'] = lambda n: f'int buf{n}=rank;'
start['MPI_Send'] = lambda n: ""
operation['MPI_Send'] = lambda n: f'MPI_Send(&buf{n}, buff_size, type, dest, stag, newcom);'
fini['MPI_Send'] = lambda n: ""
free['MPI_Send'] = lambda n: ""
write['MPI_Send'] = lambda n: ""

init['MPI_Ssend'] = lambda n: f'int buf{n}=rank;'
start['MPI_Ssend'] = lambda n: ""
operation['MPI_Ssend'] = lambda n: f'MPI_Ssend(&buf{n}, buff_size, type, dest, stag, newcom);'
fini['MPI_Ssend'] = lambda n: ""
free['MPI_Ssend'] = lambda n: ""
write['MPI_Ssend'] = lambda n: ""

init['MPI_Bsend'] = lambda n: (f'int buf{n}=rank;\n'
            + f'int buffer_attached_size{n} = MPI_BSEND_OVERHEAD + sizeof(int);\n'
            + f'char* buffer_attached{n} = (char*)malloc(buffer_attached_size{n});\n'
            + f'MPI_Buffer_attach(buffer_attached{n}, buffer_attached_size{n});')
start['MPI_Bsend'] = lambda n: ""
operation['MPI_Bsend'] = lambda n: f'MPI_Bsend(&buf{n}, buff_size, type, dest, stag, newcom);'
fini['MPI_Bsend'] = lambda n: ""
free['MPI_Bsend'] = (lambda n: f'MPI_Buffer_detach(&buffer_attached{n}, &buffer_attached_size{n});\n'
            + f'free(buffer_attached{n});')
write['MPI_Bsend'] = lambda n: ""

init['MPI_Recv'] = lambda n: f'int buf{n}=-1; MPI_Status sta{n};'
start['MPI_Recv'] = lambda n: ""
operation['MPI_Recv'] = lambda n: f'MPI_Recv(&buf{n}, buff_size, type, src, rtag, newcom, &sta{n});'
fini['MPI_Recv'] = lambda n: ""
free['MPI_Recv'] = lambda n: ""
write['MPI_Recv'] = lambda n: ""

init['MPI_Probe'] = lambda n: ""
start['MPI_Probe'] = lambda n: ""
operation['MPI_Probe'] = lambda n: f'MPI_Probe(src, 0, newcom, &sta);'
fini['MPI_Probe'] = lambda n: ""
free['MPI_Probe'] = lambda n: ""
write['MPI_Probe'] = lambda n: ""



### P2P:nonblocking

init['MPI_Isend'] = lambda n: f'int buf{n}=rank; MPI_Request req{n}=MPI_REQUEST_NULL;'
start['MPI_Isend'] = lambda n: ""
operation['MPI_Isend'] = lambda n: f'MPI_Isend(&buf{n}, buff_size, type, dest, stag, newcom, &req{n});'
fini['MPI_Isend'] = lambda n: f'MPI_Wait(&req{n}, MPI_STATUS_IGNORE);'
free['MPI_Isend'] = lambda n: f'if(req{n} != MPI_REQUEST_NULL) MPI_Request_free(&req{n});'
write['MPI_Isend'] = lambda n: f'buf{n}=4;'

init['MPI_Irecv'] = lambda n: f'int buf{n}=-1; MPI_Request req{n}=MPI_REQUEST_NULL;'
start['MPI_Irecv'] = lambda n: ""
operation['MPI_Irecv'] = lambda n: f'MPI_Irecv(&buf{n}, buff_size, type, src, rtag, newcom, &req{n});'
fini['MPI_Irecv'] = lambda n: f' MPI_Wait(&req{n}, MPI_STATUS_IGNORE);'
free['MPI_Irecv'] = lambda n: f'if(req{n} != MPI_REQUEST_NULL) MPI_Request_free(&req{n});'
write['MPI_Irecv'] = lambda n: f'buf{n}++;'

### P2P:persistent

init['MPI_Send_init'] = lambda n: f'int buf{n}=rank; MPI_Request req{n}=MPI_REQUEST_NULL;'
operation['MPI_Send_init'] = lambda n: f'MPI_Send_init(&buf{n}, buff_size, type, dest, stag, newcom, &req{n});'
start['MPI_Send_init'] = lambda n: f'MPI_Start(&req{n});'
fini['MPI_Send_init'] = lambda n: f'MPI_Wait(&req{n}, MPI_STATUS_IGNORE);'
free['MPI_Send_init'] = lambda n: f'if(req{n} != MPI_REQUEST_NULL) MPI_Request_free(&req{n});'
write['MPI_Send_init'] = lambda n: f'buf{n}=4;'

init['MPI_Recv_init'] = lambda n: f'int buf{n}=-1; MPI_Request req{n}=MPI_REQUEST_NULL;'
start['MPI_Recv_init'] = lambda n: f'MPI_Start(&req{n});'
operation['MPI_Recv_init'] = lambda n: f'MPI_Recv_init(&buf{n}, buff_size, type, src, rtag, newcom, &req{n});'
fini['MPI_Recv_init'] = lambda n: f'MPI_Wait(&req{n}, MPI_STATUS_IGNORE);'
free['MPI_Recv_init'] = lambda n: f'if(req{n} != MPI_REQUEST_NULL) MPI_Request_free(&req{n});'
write['MPI_Recv_init'] = lambda n: f'buf{n}++;'

### RMA

epoch['MPI_Win_fence'] =lambda n: 'MPI_Win_fence(0, win);'
finEpoch['MPI_Win_fence'] =lambda n: 'MPI_Win_fence(0, win);'
epoch['MPI_Win_lock'] =lambda n: 'MPI_Win_lock(MPI_LOCK_SHARED, target, 0, win);'
finEpoch['MPI_Win_lock'] =lambda n: 'MPI_Win_unlock(target, win);'
epoch['MPI_Win_lock_all'] =lambda n: 'MPI_Win_lock_all(0,win);'
finEpoch['MPI_Win_lock_all'] =lambda n: 'MPI_Win_unlock_all(win);'

init['MPI_Put'] = lambda n: f'int localbuf{n}[N] = {{12345}};'
operation['MPI_Put'] = lambda n: f'MPI_Put(&localbuf{n}, N, MPI_INT, target, 0, N, type, win);'

init['MPI_Get'] = lambda n: f'int localbuf{n}[N] = {{54321}};'
operation['MPI_Get'] = lambda n: f'MPI_Get(&localbuf{n}, N, MPI_INT, target, 0, N, type, win);'

init['store'] = lambda n: f'int localbuf{n}[N] = {{0}};'
operation['store'] = lambda n: f'localbuf{n}[0] = 8;'

init['rstore'] = lambda n: ""
operation['rstore'] = lambda n: f'winbuf[20] = 12346;'

init['load'] = lambda n: f'int localbuf{n}[N] = {{0}};'
operation['load'] = lambda n: f'int load = localbuf{n}[0];'

init['rload'] = lambda n: ""
operation['rload'] = lambda n: "int load = winbuf[20];"

init['loadstore'] = lambda n: f'int localbuf{n}[N] = {{0}};'
operation['loadstore'] = lambda n: f'if (localbuf{n}[0] % 2 == 0)  localbuf{n}[0]++; '




def find_line(content, target, filename):
    res = 1
    for line in content.split('\n'):
        if re.search(f'[^:]{target}', line):
            #print(f'Found {target} at {line}')
            return res
        res += 1
    raise ValueError(f"Line target {target} not found in {filename}.")


def make_file(template, filename, replace):
    output = template
    filename = filename.replace("_MPI_", "_")
    replace['filename'] = filename
    # Replace all variables that don't have a ':' in their name
    while re.search("@\{[^@:]*\}@", output):
        m = re.search("@\{([^@:]*)\}@", output)
        target = m.group(1)
        #print(f"Replace @{{{target}}}@")
        if target in replace.keys():
            output = re.sub(f'@\{{{target}\}}@', replace[target], output)
            #print(f"Replace {target} -> {replace[target]}")
        else:
            raise ValueError(f"Variable {target} used in template, but not defined.")
    # Now replace all variables with a ':' in their name: line targets are like that, and we don't want to resolve them before the others change the lines
    while re.search("@\{([^:@]*):([^@]*)\}@", output):
        m = re.search("@\{([^:@]*):([^@]*)\}@", output)
        (kind, target) = (m.group(1), m.group(2))
        if kind == 'line':
            replace = f'{find_line(output, target, filename)}'
            #print(f"Replace @{{line:{target}}}@ with '{replace}'")
            output = re.sub(f'@\{{line:{target}\}}@', replace, output)
        else:
            raise ValueError(f"Unknown variable kind: {kind}:{target}")

    if os.path.exists(filename):
        with open(filename, 'r') as file:
            prev = file.read().split('\n')[0]
            prev = re.sub('^.*?scripts/','scripts/', prev)
            prev = re.sub('. DO NOT EDIT.', '', prev)
        now = output.split('\n')[0]
        now = re.sub('^.*?scripts/','scripts/', now)
        now = re.sub('. DO NOT EDIT.', '', now)

        print(f'WARNING: overwriting {filename}. Previously generated by: {prev}; regenerated by {now}')

    # Ready to output it
    with open(filename, 'w') as outfile:
        outfile.write(output)
