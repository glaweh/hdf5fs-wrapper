PROGRAM test_getenv
  use mpi
  
  CHARACTER(len=255) :: homedir,h5file,pmi_rank,h5base
  integer :: np,id,ierr
  
  CALL MPI_INIT(ierr)
  CALL MPI_COMM_SIZE (MPI_COMM_WORLD, np, ierr)
  CALL MPI_COMM_RANK (MPI_COMM_WORLD, id, ierr)



  CALL getenv("HOME", homedir)
  WRITE (*,*) TRIM(homedir)
  CALL getenv("H5FS_FILE", h5file)
  WRITE (*,*) TRIM(h5file)
  CALL getenv("H5FS_BASE", h5base)
  WRITE (*,*) TRIM(h5base)
  CALL getenv("MP_CHILD", pmi_rank)
  WRITE (*,*) TRIM(pmi_rank)

  testfile = TRIM(h5base) // '/fort.66'
  open(66, file=TRIM(h5base) // '/fort.66')
  write(66,*) homedir
  write(66,*) h5file
  write(66,*) pmi_rank
  close(66)

END PROGRAM
