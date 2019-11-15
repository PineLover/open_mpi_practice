#!/bin/sh
#
foldername="ppm_example/small/"
exec_file="mpirun -np 8 -mca btl ^openib -hostfile host pb5_mpi"

for LINE in `ls ppm_example/small/`
    do
        #filename= basename $LINE
        echo $LINE
        #echo "./ppm_example/large/"$LINE
        #`mpirun -np 8 -mca btl ^openib -hostfile host pb5_mpi` <<< "./ppm_example/large/"$LINE
    done

