      integer mpi_comm_world
      parameter (mpi_comm_world = 0)

      integer mpi_max, mpi_min, mpi_sum
      parameter (mpi_max = 1, mpi_sum = 2, mpi_min = 3)

      integer mpi_byte, mpi_integer, mpi_real,
     >                  mpi_double_precision,  mpi_complex,
     >                  mpi_double_complex
      parameter (mpi_double_precision = 1,
     $           mpi_integer = 2, 
     $           mpi_byte = 3, 
     $           mpi_real= 4, 
     $           mpi_complex = 5,
     $           mpi_double_complex = 6)

      integer mpi_any_source
      parameter (mpi_any_source = -1)

      integer mpi_err_other
      parameter (mpi_err_other = -1)

      double precision mpi_wtime
      external mpi_wtime

      integer mpi_status_size
      parameter (mpi_status_size=3)
