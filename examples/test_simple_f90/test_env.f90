PROGRAM test_getenv
  ! use mpi
  
  CHARACTER(len=255) :: homedir,h5file,pmi_rank,h5base,testfile
  integer :: np,id,ierr
  
  ! CALL MPI_INIT(ierr)
  ! CALL MPI_COMM_SIZE (MPI_COMM_WORLD, np, ierr)
  ! CALL MPI_COMM_RANK (MPI_COMM_WORLD, id, ierr)



  CALL getenv("HOME", homedir)
  WRITE (*,*) TRIM(homedir)
  CALL getenv("H5FS_FILE", h5file)
  WRITE (*,*) TRIM(h5file)
  CALL getenv("H5FS_BASE", h5base)
  WRITE (*,*) 'h5b: "', TRIM(h5base), '"'
  if (len_trim(h5base)==0) h5base="."

  testfile = TRIM(h5base) // '/fort.66'
  ! CALL getenv("MP_CHILD", pmi_rank)
  ! WRITE (*,*) TRIM(pmi_rank)

  open(66, file=testfile)
  write(66,*) homedir
  write(66,*) h5file
  ! write(66,*) pmi_rank
  close(66)

END PROGRAM
