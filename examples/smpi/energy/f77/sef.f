! Copyright (c) 2013-2014. The SimGrid Team.
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
      call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)

      pstates = smpi_get_host_nb_pstates()

      t = MPI_Wtime()

      print *, '[', t, '] [rank ', rank, ']',
     &     pstates, ' pstates available'

      do i = 0, pstates - 1
         p = smpi_get_host_power_peak_at(i)
         print *, '[', t, '] [rank ', rank, '] Power: ', p
      end do

      do i = 0, pstates - 1
         call smpi_set_host_power_peak_at(i)
         t = MPI_Wtime()
         p = smpi_get_host_current_power_peak()
         print *, '[', t, '] [rank ', rank, '] Current pstate: ', i,
     &        '; Current power: ', p

         e = 1e9
         call smpi_execute_flops(e)

         t = MPI_Wtime()
         e = smpi_get_host_consumed_energy()
         print *, '[', t, '] [rank ', rank, ']',
     &        ' Energy consumed (Joules): ', e
      end do

      call MPI_Finalize(ierr)

      end program main
