/* Copyright (c) 2010-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.hpp"
#include "smpi_file.hpp"
#include "smpi_errhandler.hpp"
#include "smpi_info.hpp"

extern "C" { // This should really use the C linkage to be usable from Fortran

static constexpr int FORTRAN_NULL_HANDLE = -1;

void mpi_file_close_ ( int* file, int* ierr){
  MPI_File tmp = simgrid::smpi::File::f2c(*file);
  *ierr =  MPI_File_close(&tmp);
  if(*ierr == MPI_SUCCESS) {
    simgrid::smpi::F2C::free_f(*file);
    *file = FORTRAN_NULL_HANDLE;
  }
}

void mpi_file_delete_ ( char* filename, int* info, int* ierr, int filename_len){
  std::string name = smpi_fortran_to_string(filename, filename_len);
  *ierr= MPI_File_delete(name.c_str(), simgrid::smpi::Info::f2c(*info));
}

void mpi_file_open_ ( int* comm, char* filename, int* amode, int* info, int* fh, int* ierr, int filename_len){
  MPI_File tmp;
  std::string name = smpi_fortran_to_string(filename, filename_len);
  *fh = FORTRAN_NULL_HANDLE;
  *ierr= MPI_File_open(simgrid::smpi::Comm::f2c(*comm), name.c_str(), *amode, simgrid::smpi::Info::f2c(*info), &tmp);
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

void mpi_file_get_size_(int* fh, MPI_Offset* size, int* ierr){
	*ierr= MPI_File_get_size(simgrid::smpi::File::f2c(*fh), size);
}

void mpi_file_set_size_(int* fh, MPI_Offset* size, int* ierr){
	*ierr= MPI_File_set_size(simgrid::smpi::File::f2c(*fh), *size);
}

void mpi_file_read_ ( int* fh, void* buf, int* count, int* datatype, MPI_Status* status, int* ierr){
  *ierr=  MPI_File_read(simgrid::smpi::File::f2c(*fh), buf, *count, simgrid::smpi::Datatype::f2c(*datatype), FORT_STATUS_IGNORE(status));
}

void mpi_file_read_shared_(int* fh, void *buf, int* count, int* datatype, MPI_Status* status, int* ierr){
  *ierr=  MPI_File_read_shared(simgrid::smpi::File::f2c(*fh), buf, *count, simgrid::smpi::Datatype::f2c(*datatype), FORT_STATUS_IGNORE(status));
}

void mpi_file_read_all_(int* fh, void *buf, int* count, int* datatype, MPI_Status* status, int* ierr){
  *ierr=  MPI_File_read_all(simgrid::smpi::File::f2c(*fh), buf, *count, simgrid::smpi::Datatype::f2c(*datatype), FORT_STATUS_IGNORE(status));
}

void mpi_file_read_ordered_(int* fh, void *buf, int* count, int* datatype, MPI_Status* status, int* ierr){
  *ierr=  MPI_File_read_ordered(simgrid::smpi::File::f2c(*fh), buf, *count, simgrid::smpi::Datatype::f2c(*datatype), FORT_STATUS_IGNORE(status));
}

void mpi_file_read_at_(int* fh, MPI_Offset* offset, void *buf, int* count, int* datatype, MPI_Status* status, int* ierr){
  *ierr=  MPI_File_read_at(simgrid::smpi::File::f2c(*fh), *offset, buf, *count, simgrid::smpi::Datatype::f2c(*datatype), FORT_STATUS_IGNORE(status));
}

void mpi_file_read_at_all_(int* fh, MPI_Offset* offset, void *buf, int* count, int* datatype, MPI_Status* status, int* ierr){
  *ierr=  MPI_File_read_at_all(simgrid::smpi::File::f2c(*fh), *offset, buf, *count, simgrid::smpi::Datatype::f2c(*datatype), FORT_STATUS_IGNORE(status));
}

void mpi_file_write_ ( int* fh, void* buf, int* count, int* datatype, MPI_Status* status, int* ierr){
  *ierr=  MPI_File_write(simgrid::smpi::File::f2c(*fh), buf, *count, simgrid::smpi::Datatype::f2c(*datatype), FORT_STATUS_IGNORE(status));
}

void mpi_file_write_shared_(int* fh, void *buf, int* count, int* datatype, MPI_Status* status, int* ierr){
  *ierr=  MPI_File_write_shared(simgrid::smpi::File::f2c(*fh), buf, *count, simgrid::smpi::Datatype::f2c(*datatype), FORT_STATUS_IGNORE(status));
}

void mpi_file_write_ordered_(int* fh, void *buf, int* count, int* datatype, MPI_Status* status, int* ierr){
  *ierr=  MPI_File_write_ordered(simgrid::smpi::File::f2c(*fh), buf, *count, simgrid::smpi::Datatype::f2c(*datatype), FORT_STATUS_IGNORE(status));
}

void mpi_file_write_at_(int* fh, MPI_Offset* offset, void *buf, int* count, int* datatype, MPI_Status* status, int* ierr){
  *ierr=  MPI_File_write_at(simgrid::smpi::File::f2c(*fh), *offset, buf, *count, simgrid::smpi::Datatype::f2c(*datatype), FORT_STATUS_IGNORE(status));
}

void mpi_file_write_at_all_(int* fh, MPI_Offset* offset, void *buf, int* count, int* datatype, MPI_Status* status, int* ierr){
  *ierr=  MPI_File_write_at_all(simgrid::smpi::File::f2c(*fh), *offset, buf, *count, simgrid::smpi::Datatype::f2c(*datatype), FORT_STATUS_IGNORE(status));
}

void mpi_file_write_all_ ( int* fh, void* buf, int* count, int* datatype, MPI_Status* status, int* ierr){
  *ierr=  MPI_File_write_all(simgrid::smpi::File::f2c(*fh), buf, *count, simgrid::smpi::Datatype::f2c(*datatype), FORT_STATUS_IGNORE(status));
}

void mpi_file_set_view_ ( int* fh, MPI_Offset* offset, int* etype, int* filetype, char* datarep, int* info, int* ierr,
                          int datarep_len){
  std::string rep = smpi_fortran_to_string(datarep, datarep_len);
  *ierr= MPI_File_set_view(simgrid::smpi::File::f2c(*fh) , *offset, simgrid::smpi::Datatype::f2c(*etype), simgrid::smpi::Datatype::f2c(*filetype), rep.c_str(), simgrid::smpi::Info::f2c(*info));
}

void mpi_file_get_type_extent_ ( int* fh, int* datatype, MPI_Aint* extent, int* ierr){
  *ierr= MPI_File_get_type_extent(simgrid::smpi::File::f2c(*fh), simgrid::smpi::Datatype::f2c(*datatype), extent);
}

void mpi_file_get_amode_ ( int* fh, int* amode, int* ierr){
  *ierr= MPI_File_get_amode(simgrid::smpi::File::f2c(*fh), amode);
}

void mpi_file_get_byte_offset_ ( int* fh, MPI_Offset* offset, MPI_Offset* disp, int* ierr){
  *ierr= MPI_File_get_byte_offset(simgrid::smpi::File::f2c(*fh), *offset, disp);
}

void mpi_file_get_atomicity_ ( int* fh, int* flag, int* ierr){
  *ierr= MPI_File_get_atomicity(simgrid::smpi::File::f2c(*fh), flag);
}

void mpi_file_set_atomicity_ ( int* fh, int* flag, int* ierr){
  *ierr= MPI_File_set_atomicity(simgrid::smpi::File::f2c(*fh), *flag);
}

void mpi_file_sync_ ( int* fh, int* ierr){
  *ierr= MPI_File_sync(simgrid::smpi::File::f2c(*fh));
}

void mpi_file_get_group_ ( int* fh, int* group, int* ierr){
  MPI_Group tmp;
  *group = FORTRAN_NULL_HANDLE;
  *ierr= MPI_File_get_group(simgrid::smpi::File::f2c(*fh), &tmp);
  if (*ierr == MPI_SUCCESS) {
    *group = tmp->c2f();
  }
}

void mpi_file_get_info_ ( int* fh, int* info, int* ierr){
  MPI_Info tmp;
  *info = FORTRAN_NULL_HANDLE;
  *ierr= MPI_File_get_info(simgrid::smpi::File::f2c(*fh), &tmp);
  if (*ierr == MPI_SUCCESS) {
    *info = tmp->c2f();
  }
}

void mpi_file_set_info_ ( int* fh, int* info, int* ierr){
  *ierr= MPI_File_set_info(simgrid::smpi::File::f2c(*fh), simgrid::smpi::Info::f2c(*info));
}

void mpi_file_get_errhandler_ ( int* file, int* errhandler, int* ierr){
  MPI_Errhandler tmp;
  *errhandler = FORTRAN_NULL_HANDLE;
  *ierr= MPI_File_get_errhandler(simgrid::smpi::File::f2c(*file), &tmp);
  if (*ierr == MPI_SUCCESS && tmp != MPI_ERRHANDLER_NULL) {
    *errhandler = tmp->c2f();
  }
}

void mpi_file_set_errhandler_ ( int* file, int* errhandler, int* ierr){
  *ierr= MPI_File_set_errhandler(simgrid::smpi::File::f2c(*file), simgrid::smpi::Errhandler::f2c(*errhandler));
}

void mpi_file_get_view_(int* fh, MPI_Offset* disp, int* etype, int* filetype, char *datarep, int* ierr, int datarep_len){
  MPI_Datatype tmp;
  MPI_Datatype tmp2;
  std::vector<char> rep(MPI_MAX_DATAREP_STRING);
  *ierr= MPI_File_get_view(simgrid::smpi::File::f2c(*fh) , disp, &tmp, &tmp2, rep.data());
  if(*ierr == MPI_SUCCESS) {
    *etype = tmp->c2f();
    *filetype = tmp2->c2f();
    smpi_fortran_from_string(datarep, datarep_len, rep.data(), strlen(rep.data()));
  }
}


}
