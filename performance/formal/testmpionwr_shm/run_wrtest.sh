yu=$((`grep ^-IB launch.cfg|wc -l`%2))
echo "$yu"
if [ "$yu" -eq "1" ] ;then
	echo "the number of MPI proc should be even!"
	exit
fi
result=test_wr_`date "+%Y%m%d%H%M%S"`.log 
#echo $result
mpirun -configfile launch.cfg |tee $result
date>>$result
echo "------- program run completely!--------------"
echo "log saved into $result"
