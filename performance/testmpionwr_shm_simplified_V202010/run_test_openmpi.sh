result=openmpi_test_realOS_`date "+%Y%m%d%H%M%S"`.log 
#echo $result
cat ./run_test_openmpi.sh >> $result
date1=`date`
#/usr/local/openmpi/bin/mpirun  --allow-run-as-root --cpu-list 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23 -n 23 ./emt1 5000000 | tee $result
/usr/local/openmpi/bin/mpirun  --allow-run-as-root  --rankfile rankfile -n 23 ./openmpi_emt    20000000 | tee $result
#taskset -c 1-10  mpirun -genv I_MPI_PIN_PROCESSOR_LIST=1-10 -n 8 ./emt1 2000000 | tee $result
#taskset -c 1-24  mpirun  -n 23 ./emt1 2000000 | tee $result
echo "program starts at $date1" >>$result
date2=`date`
echo "program ends at $date2" >>$result
date>>$result
echo "------- program run completely!--------------"
echo "log saved into $result"
