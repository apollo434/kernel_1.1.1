for i in {1..3}
	do
		result=test_wr_`date "+%Y%m%d%H"`.log.$i 
#echo $result
rm -rf /var/rtcore_map
echo "No ${i} is started!---------------------------"
mpirun -np 150 ./emt1 |tee $result
#mpirun -np 150 ./emt1
date>>$result
echo "------- program run completely ${i}!--------------"
echo "log saved into $result"
sleep 30s
done
