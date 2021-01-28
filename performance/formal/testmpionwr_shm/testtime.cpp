/*
 * testtime.cpp
 *
 *  Created on: 2019年2月15日
 *      Author: Lenovo
 */


/*
   电力系统实时仿真程序包括机电暂态(ST)、电磁暂态(EMT)等多种类型程序，具体见emt.h中SIMUTYPE_ALL定义。

   本程序模拟实时仿真中电磁暂态程序EMT，与SIMUTYPE_STCTRL、SIMUTYPE_STSUB、SIMUTYPE_EMTSUB、SIMUTYPE_EMTPHY、SIMUTYPE_EMTIO等类型的进程存在通信关系。
   本程序实现的是emt间通信方式之一，即各进程之间点对点通信。

   本程序可单独运行，调用方法：
# ./mpirun -np 6 emt
 */

//#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <i_malloc.h>
#include <time.h>



#define _GNU_SOURCE

#ifdef _LINUX
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sched.h>
#include <errno.h>
//mlock
#include <sys/mman.h>

//setrlimit
#include <sys/resource.h>
#else //_LINUX
#include <sys/timeb.h>
#endif //_LINUX

#ifdef _LINUX
pthread_t thread;
#endif //_LINUX


int main(int argc,char *argv[])
{

	time_t rawtime;
	int t=0;
				struct tm* timeinfo;
				time(&rawtime);
				printf("MPI proc: %d  bp1....\n",t);
				timeinfo = localtime(&rawtime);
				printf("MPI proc: %d  bp2....\n",t);
				printf("MPI proc: %d  bp3....\n",t);

				//fprintf(fp, sTime);
				printf("MPI proc: %d  bp4....\n",t);
				printf("MPI proc: %d  bp5....\n",t);
				printf("MPI proc: %d  bp6....\n",t);

				printf(asctime(timeinfo));

				return 0;
}


