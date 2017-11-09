# @ shell=/bin/bash
#
# @ job_name =test
# @ error   = $(job_name).err.$(jobid)
# @ output  = $(job_name).out.$(jobid)
# @ job_type = parallel
# @ node_usage= shared
# @ node = 1
# @ tasks_per_node =  4
# @ resources = ConsumableCpus(1)
# @ network.MPI = sn_all,not_shared,us
# @ wall_clock_limit =00:05:00
## @ requirements = (Feature=="gpu")
# @ notification = never
# @ queue

#export H5FS_FILE=/u/mjr/scratch_${MP_CHILD}.h5
#echo "H5FS_FILE: $H5FS_FILE"
#poe /u/mjr/a.out
poe wrapper.sh ./a.out

