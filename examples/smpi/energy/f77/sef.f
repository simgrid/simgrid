! Copyright (c) 2013-2019. The SimGrid Team.
! All rights reserved.

! This program is free software; you can redistribute it and/or modify it
! under the terms of the license (GNU LGPL) which comes with this package.

      program main
      include 'mpif.h'

      integer ierr
      integer rank, pstates
      integer i
      double precision p, t, e

      call MPI_Init(ierr)
      if (ierr .ne. MPI_SUCCESS) then
         print *, 'MPI_Init failed:', ierr
         stop 1
      endif
      call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
      if (ierr .ne. MPI_SUCCESS) then
         print *, 'MPI_Comm_rank failed:', ierr
         call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
         stop 1
      endif

      pstates = smpi_get_host_nb_pstates()

      t = MPI_Wtime()

      print '(1a,F15.8,1a,i2,1a,i2,1a)', '[', t, '] [rank ', rank, '] ',
     &     pstates, ' pstates available'

      do i = 0, pstates - 1
         p = smpi_get_host_power_peak_at(i)
         print '(1a,F15.8,1a,i2,1a,F15.4)', '[', t, '] [rank ',
     &     rank, '] Power: ', p
      end do

      do i = 0, pstates - 1
         call smpi_set_host_pstate(i)
         t = MPI_Wtime()
         p = smpi_get_host_current_power_peak()
         print '(1a,F15.8,1a,i2,1a,i2,1a,F15.4)', '[', t, '] [rank ',
     &         rank,'] Current pstate: ', i,'; Current power: ', p

         e = 1e9
         call smpi_execute_flops(e)

         t = MPI_Wtime()
         e = smpi_get_host_consumed_energy()
         print '(1a,F15.8,1a,i2,1a,1a,F15.4)', '[', t, '] [rank ',
     &         rank, ']',' Energy consumed (Joules): ', e
      end do

      flush(6)

      call MPI_Finalize(ierr)
      if (ierr .ne. MPI_SUCCESS) then
         print *, 'MPI_Finalize failed:', ierr
         call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
         stop 1
      endif

      end program main
