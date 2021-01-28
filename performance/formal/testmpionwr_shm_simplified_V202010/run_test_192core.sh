#$1: total process num
#$2: DIV NUM process num
#$3: count of running 
#$4: warm count
if [ $# != 4 ]; then 
	echo "Usage: "  
	echo "  #1:total process num  "
	echo "  #2:DIV num"
        echo "  #3: count of running "
	echo "  #4: warm count"
        exit 1;
fi
result=intel_mpi_test_realOS_`date "+%Y%m%d%H%M%S"`.log 
#echo $result
date1=`date`
cat ./run_test.sh >>$result 
t2=1
t1=$[$1+$t2]
t3=$[$t1+$t2]
echo "Command:   taskset -c 0-$t1 mpirun  -genv I_MPI_DEBUG=+10,time,rank  -genv I_MPI_PIN_PROCESSOR_LIST=2-$t1 -genv I_MPI_DEBUG_OUTPUT=./de.log -n $1 ./intelmpi_emt   $3 $4 $1 $2 | tee $result"
taskset -c 0-$t1 mpirun   -genv I_MPI_PIN_PROCESSOR_LIST=2-$t1 -genv I_MPI_DEBUG_OUTPUT=./de.log -n $1 ./intelmpi_emt   $3 $4 $1 $2 | tee $result
#taskset -c 0-$t1 mpirun  -genv I_MPI_DEBUG=+10,time,rank  -genv I_MPI_PIN_PROCESSOR_LIST=2-$t1 -genv I_MPI_DEBUG_OUTPUT=./de.log -n $1 ./intelmpi_emt   $3 $4 $1 $2 | tee $result
#taskset -c 1-10  mpirun -genv I_MPI_PIN_PROCESSOR_LIST=1-10 -n 8 ./emt1 2000000 | tee $result
#taskset -c 1-24  mpirun  -n 23 ./emt1 2000000 | tee $result
echo "program starts at $date1" >>$result
date2=`date`
echo "program ends at $date2" >>$result
date>>$result
echo "------- program run completely!--------------"
echo "log saved into $result"
