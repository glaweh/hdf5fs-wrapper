SHELL:=/bin/bash
OCTOPUS?=$(HOME)/build/octopus/usr/bin/octopus
WRAPPER_DIR?=$(HOME)/build/hdf5fs-wrapper

%/: %.h5
	mkdir $@
	cd $@ && $(WRAPPER_DIR)/h5fs-unpack ../$<

%.size_md5sum: %/
	test -d $<
	./size_md5.sh $< > $@

all: restart restart-nowrap

start: start.h5 start.size_md5sum
start.h5: 06-restart.03-gs_mixvhxc.inp $(WRAPPER_DIR)/h5fs-wrap $(OCTOPUS) $(WRAPPER_DIR)/h5fs-repack
	ln -nsf $< inp
	rm -f scratch_*.h5
	H5FS_BASE=workdir mpirun -np 1 $(WRAPPER_DIR)/h5fs-wrap $(OCTOPUS) |& tee start.log
	rm -f $@
	$(WRAPPER_DIR)/h5fs-repack $@ scratch_*.h5

restart: restart.h5 restart.size_md5sum
restart.h5: 06-restart.04-restart_gs_mixvhxc.inp start.h5
	ln -nsf $< inp
	rm -f scratch_*.h5
	H5FS_BASE=workdir H5FS_RO=start.h5 mpirun -np 1 $(WRAPPER_DIR)/h5fs-wrap $(OCTOPUS) |& tee restart.log
	$(WRAPPER_DIR)/h5fs-repack $@ start.h5 scratch_*.h5

start-nowrap: workdir-start-nowrap workdir-start-nowrap.size_md5sum
workdir-start-nowrap: 06-restart.03-gs_mixvhxc.inp $(OCTOPUS)
	ln -nsf $< inp
	rm -rf workdir
	mpirun -np 1 $(OCTOPUS) |& tee start-nowrap.log
	rm -rf $@
	mv workdir $@

restart-nowrap: workdir-restart-nowrap workdir-restart-nowrap.size_md5sum
workdir-restart-nowrap: 06-restart.04-restart_gs_mixvhxc.inp workdir-start-nowrap
	ln -nsf $< inp
	rm -f workdir
	cp -a workdir-start-nowrap workdir
	mpirun -np 1 $(OCTOPUS) |& tee restart-nowrap.log
	rm -rf $@
	mv workdir $@

clean:
	rm -rf exec/ scratch_*.h5 start.h5 start.log restart.h5 restart.log workdir-start-nowrap workdir-restart-nowrap start-nowrap.log restart-nowrap.log workdir restart restart.size_md5sum workdir-restart-nowrap.size_md5sum inp

.PHONY: clean start restart start-nowrap restart-nowrap
