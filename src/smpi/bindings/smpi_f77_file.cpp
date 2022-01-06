/* Copyright (c) 2010-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.hpp"
#include "smpi_file.hpp"
#include "smpi_errhandler.hpp"
#include "smpi_info.hpp"

extern "C" { // This should really use the C linkage to be usable from Fortran

void mpi_file_close_ ( int* file, int* ierr){
  MPI_File tmp = simgrid::smpi::File::f2c(*file);
  *ierr =  MPI_File_close(&tmp);
  if(*ierr == MPI_SUCCESS) {
    simgrid::smpi::F2C::free_f(*file);
  }
}

void mpi_file_delete_ ( char* filename, int* info, int* ierr){
  *ierr= MPI_File_delete(filename, simgrid::smpi::Info::f2c(*info));
}

void mpi_file_open_ ( int* comm, char* filename, int* amode, int* info, int* fh, int* ierr){
  MPI_File tmp;
  *ierr= MPI_File_open(simgrid::smpi::Comm::f2c(*comm), filename, *amode, simgrid::smpi::Info::f2c(*info), &tmp);
  if (*ierr == MPI_SUCCESS) {
    *fh = tmp->c2f();
  }
}

void mpi_file_seek_(int* fh, MPI_Offset* offset, int* whence, int* ierr){
	*ierr= MPI_File_seek(simgrid::smpi::File::f2c(*fh), *offset, *whence);
}

void mpi_file_seek_shared_(int* fh, MPI_Offset* offset, int* whence, int* ierr){
	*ierr= MPI_File_seek_shared(simgrid::smpi::File::f2c(*fh), *offset, *whence);
}

void mpi_file_get_position_(int* fh, MPI_Offset* offset, int* ierr){
	*ierr= MPI_File_get_position(simgrid::smpi::File::f2c(*fh), offset);
}
void mpi_file_get_position_shared_(int* fh, MPI_Offset* offset, int* ierr){
	*ierr= MPI_File_get_position_shared(simgrid::smpi::File::f2c(*fh), offset);
}

void mpi_file_read_ ( int* fh, void* buf, int* count, int* datatype, MPI_Status* status, int* ierr){
  *ierr=  MPI_File_read(simgrid::smpi::File::f2c(*fh), buf, *count, simgrid::smpi::Datatype::f2c(*datatype), status);
}

void mpi_file_read_shared_(int* fh, void *buf, int* count, int* datatype, MPI_Status* status, int* ierr){
  *ierr=  MPI_File_read_shared(simgrid::smpi::File::f2c(*fh), buf, *count, simgrid::smpi::Datatype::f2c(*datatype), status);
}

void mpi_file_read_all_(int* fh, void *buf, int* count, int* datatype, MPI_Status* status, int* ierr){
  *ierr=  MPI_File_read_all(simgrid::smpi::File::f2c(*fh), buf, *count, simgrid::smpi::Datatype::f2c(*datatype), status);
}

void mpi_file_read_ordered_(int* fh, void *buf, int* count, int* datatype, MPI_Status* status, int* ierr){
  *ierr=  MPI_File_read_ordered(simgrid::smpi::File::f2c(*fh), buf, *count, simgrid::smpi::Datatype::f2c(*datatype), status);
}

void mpi_file_read_at_(int* fh, MPI_Offset* offset, void *buf, int* count, int* datatype, MPI_Status* status, int* ierr){
  *ierr=  MPI_File_read_at(simgrid::smpi::File::f2c(*fh), *offset, buf, *count, simgrid::smpi::Datatype::f2c(*datatype), status);
}

void mpi_file_read_at_all_(int* fh, MPI_Offset* offset, void *buf, int* count, int* datatype, MPI_Status* status, int* ierr){
  *ierr=  MPI_File_read_at_all(simgrid::smpi::File::f2c(*fh), *offset, buf, *count, simgrid::smpi::Datatype::f2c(*datatype), status);
}

void mpi_file_write_ ( int* fh, void* buf, int* count, int* datatype, MPI_Status* status, int* ierr){
  *ierr=  MPI_File_write(simgrid::smpi::File::f2c(*fh), buf, *count, simgrid::smpi::Datatype::f2c(*datatype), status);
}

void mpi_file_write_shared_(int* fh, void *buf, int* count, int* datatype, MPI_Status* status, int* ierr){
  *ierr=  MPI_File_write_shared(simgrid::smpi::File::f2c(*fh), buf, *count, simgrid::smpi::Datatype::f2c(*datatype), status);
}

void mpi_file_write_ordered_(int* fh, void *buf, int* count, int* datatype, MPI_Status* status, int* ierr){
  *ierr=  MPI_File_write_ordered(simgrid::smpi::File::f2c(*fh), buf, *count, simgrid::smpi::Datatype::f2c(*datatype), status);
}

void mpi_file_write_at_(int* fh, MPI_Offset* offset, void *buf, int* count, int* datatype, MPI_Status* status, int* ierr){
  *ierr=  MPI_File_write_at(simgrid::smpi::File::f2c(*fh), *offset, buf, *count, simgrid::smpi::Datatype::f2c(*datatype), status);
}

void mpi_file_write_at_all_(int* fh, MPI_Offset* offset, void *buf, int* count, int* datatype, MPI_Status* status, int* ierr){
  *ierr=  MPI_File_write_at_all(simgrid::smpi::File::f2c(*fh), *offset, buf, *count, simgrid::smpi::Datatype::f2c(*datatype), status);
}

void mpi_file_write_all_ ( int* fh, void* buf, int* count, int* datatype, MPI_Status* status, int* ierr){
  *ierr=  MPI_File_write_all(simgrid::smpi::File::f2c(*fh), buf, *count, simgrid::smpi::Datatype::f2c(*datatype), status);
}

void mpi_file_set_view_ ( int* fh, MPI_Offset* offset, int* etype, int* filetype, char* datarep, int* info, int* ierr){
  *ierr= MPI_File_set_view(simgrid::smpi::File::f2c(*fh) , *offset, simgrid::smpi::Datatype::f2c(*etype), simgrid::smpi::Datatype::f2c(*filetype), datarep, simgrid::smpi::Info::f2c(*info));
}

void mpi_file_get_view_(int* fh, MPI_Offset* disp, int* etype, int* filetype, char *datarep, int* ierr){
  MPI_Datatype tmp;
  MPI_Datatype tmp2;
  *ierr= MPI_File_get_view(simgrid::smpi::File::f2c(*fh) , disp, &tmp, &tmp2, datarep);
  if(*ierr == MPI_SUCCESS) {
    *etype = tmp->c2f();
    *filetype = tmp2->c2f();
  }
}


}