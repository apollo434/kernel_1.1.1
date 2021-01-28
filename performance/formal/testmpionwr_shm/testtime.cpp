/*
 * testtime.cpp
 *
 *  Created on: 2019��2��15��
 *      Author: Lenovo
 */


/*
   ����ϵͳʵʱ����������������̬(ST)�������̬(EMT)�ȶ������ͳ��򣬾����emt.h��SIMUTYPE_ALL���塣

   ������ģ��ʵʱ�����е����̬����EMT����SIMUTYPE_STCTRL��SIMUTYPE_STSUB��SIMUTYPE_EMTSUB��SIMUTYPE_EMTPHY��SIMUTYPE_EMTIO�����͵Ľ��̴���ͨ�Ź�ϵ��
   ������ʵ�ֵ���emt��ͨ�ŷ�ʽ֮һ����������֮���Ե�ͨ�š�

   ������ɵ������У����÷�����
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


