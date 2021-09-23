! This file created from test/mpi/f77/coll/uallreducef.f with f77tof90
! -*- Mode: Fortran; -*-
!
!  (C) 2003 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
!
! Test user-defined operations.  This tests a simple commutative operation
!
      subroutine uop( cin, cout, count, datatype )
      use mpi
      integer cin(*), cout(*)
      integer count, datatype
      integer i

      if (datatype .ne. MPI_INTEGER) then
         print *, 'Invalid datatype (',datatype,') passed to user_op()'
         return
      endif

      do i=1, count
         cout(i) = cin(i) + cout(i)
      enddo
      end

      program main
      use mpi
      external uop
      integer ierr, errs
      integer count, sumop, i, size
      integer, DIMENSION(:), ALLOCATABLE :: vin, vout
      integer comm
      integer status

      errs = 0
      ALLOCATE(vin(65000), STAT=status)
      ALLOCATE(vout(65000), STAT=status)

      call mtest_init(ierr)
      call mpi_op_create( uop, .true., sumop, ierr )

      comm = MPI_COMM_WORLD
      call mpi_comm_size( comm, size, ierr )
      count = 1
      do while (count .lt. 65000)
         do i=1, count
            vin(i) = i
            vout(i) = -1
         enddo
         call mpi_allreduce( vin, vout, count, MPI_INTEGER, sumop,  &
      &                       comm, ierr )
!         Check that all results are correct
         do i=1, count
            if (vout(i) .ne. i * size) then
               errs = errs + 1
               if (errs .lt. 10) print *, "vout(",i,") = ", vout(i)
            endif
         enddo
         count = count + count
      enddo

      call mpi_op_free( sumop, ierr )
      DEALLOCATE(vout)
      DEALLOCATE(vin)
      call mtest_finalize(errs)
      call mpi_finalize(ierr)
      end
