F90?=gfortran
MPIF90?=mpif90

all: test_env test_env_mpi

test_env: test_env.f90
	$(F90) -o $@ $<

test_env_mpi: test_env_mpi.f90
	$(MPIF90) -o $@ $<

clean:
	rm -f test_env test_env_mpi

.PHONY: clean
