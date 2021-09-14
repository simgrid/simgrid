! Copyright (c) 2018-2021. The SimGrid Team. All rights reserved.

! This program is free software; you can redistribute it and/or modify it
! under the terms of the license (GNU LGPL) which comes with this package.

! Check that getarg does something sensible.
program getarg_1
  use mpi
  CHARACTER*10 ARGS, ARGS2
  INTEGER*4 I
  INTEGER*2 I2
  INTEGER ierr
  I = 0
  call MPI_Init(ierr)
  CALL GETARG(I,ARGS)
  ! This should return the invoking command.  The actual value depends 
  ! on the OS, but a blank string is wrong no matter what.
  ! ??? What about deep embedded systems?
  if (args.eq.'') STOP 2
  I = 1
  CALL GETARG(I,ARGS)
  if (args.ne.'a') STOP 3
  I = -1
  CALL GETARG(I,ARGS)
  if (args.ne.'') STOP 4
  ! Assume we won't have been called with more that 4 args.
  I = 4
  CALL GETARG(I,ARGS)
  if (args.ne.'') STOP 5
  I = 1000
  CALL GETARG(I,ARGS)
  if (args.ne.'') STOP 6
  call MPI_Finalize(ierr)
end
