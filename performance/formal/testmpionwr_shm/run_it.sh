#!/bin/bash

taskset -c 1-23 mpirun -genv I_MPI_PIN_PROCESSOR_LIST=1-23 -np 23 ./emt1 200000 &
#taskset -c 1-23 mpirun -genv I_MPI_PIN_PROCESSOR_LIST=1-23 -np 23 ./emt1 10000000 &

sleep 60

taskset -pc 0 `ps -ef | grep mpiexec | awk '/genv/ {print $2}'`
taskset -pc 0 `ps -ef | grep pmi_proxy | awk '/control-port/ {print $2}'`
killall -USR1 emt1

