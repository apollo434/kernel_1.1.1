/*
   ����ϵͳʵʱ����������������̬(ST)�������̬(EMT)�ȶ������ͳ��򣬾����emt.h��SIMUTYPE_ALL���塣

   ������ģ��ʵʱ�����е����̬����EMT����SIMUTYPE_STCTRL��SIMUTYPE_STSUB��SIMUTYPE_EMTSUB��SIMUTYPE_EMTPHY��SIMUTYPE_EMTIO�����͵Ľ��̴���ͨ�Ź�ϵ��
   ������ʵ�ֵ���emt��ͨ�ŷ�ʽ֮һ����������֮���Ե�ͨ�š�

   ������ɵ������У����÷�����
# ./mpirun -np 6 emt
 */
#define _GNU_SOURCE
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#include "test.h"
#include "emt.h"
#include "Pub_Def.h"
#include "MPI_Comm.h"
#include <time.h>

#ifndef _ARM
	#include <i_malloc.h>
#endif
	

#ifdef _windriver
	#include "windriver.h"
#endif

#ifdef _LINUX
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sched.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
//mlock
#include <sys/mman.h>

//setrlimit
#include <sys/resource.h>
#include <signal.h>
#else //_LINUX
#include <sys/timeb.h>
#endif //_LINUX

//int shield(int rankid, int size, int cpuid);
#define DEBUG 0

#ifdef _LINUX
pthread_t thread;
#endif //_LINUX
#define COMM_PROC 7
//#define NUM_PROC 23
//#define NUM_DIV_PER 9
//#define NUM_DIV (NUM_PROC - 1 - NUM_DIV_PER) / 2 * NUM_DIV_PER //需要满足整数要求
int NUM_PROC=23;
int NUM_DIV_PER=9;
int NUM_DIV;
#define Reverse_ID 1000
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
//#include <iostream>
#include <netdb.h>
#include <sys/syscall.h>
//#define _MPISENDRECV 1
//#define _MPIISENDRECV 1
#define _MPI_SHM_PROXY

#ifdef _JN_OS
	int shield(int rankid, int size, int cpuid);
#endif
char *gbuff1;
char *gbuff2;
char *gbuff3;
char *gbuff4;
char *gbuff5;

int iKsub;
int iMYID;

#ifdef _LINUX

static cpu_set_t set;
static cpu_set_t set_rtcore;
static cpu_set_t get;
static cpu_set_t get_rtcore;
#endif

#define SIZE_GVar_EMT 512 * 1024
//#define SIZE_GVar_EMT 1024

int isCalculationOver;

double ts, te, tn, tstep, tc;
double tmax_ctrl = 0, tmin_ctrl = 1000000;
double tmax_val = 0, tmin_val = 100000000, tmax_cal = 0, tmin_cal = 100000000;
double tmax_comm = 0, tmin_comm = 100000000;
unsigned long long step, nstep;
int cnt = 0;
FILE *fp;
static int rt_core_id = 0;
pthread_t thread;
FILE *fp11;
#define BUFFSIZE 160000
double SendBuf[EMT_COMM_NUM], RecvBuf[EMT_COMM_NUM], *DataBuf;
double ISendBuf[BUFFSIZE];
double IRecvBuf[BUFFSIZE];
//MessagePos Msgpos[NUM_DIV + 1];
MessagePos Msgpos[256];
int stopprogram;
#ifdef SHM_EMTIO
struct shm_per_task shm_ctrl_per_task;
#endif

extern void EMT_DIV_GET(int *DIVI, int *DIVJ);
extern void DIVFileProduce(int *DIVI, int *DIVJ);
extern void cplusplus_rtclock_sleep(int usec);
extern int SendCheck(double *buf, double *validbuf, int length);

/*
extern int gettid();
extern void finalize_comm();
extern void init_comm();
extern void init_buf();
extern void init_gvar();
extern void release_gvar();
*/
int ProcIO;

#ifdef _windriver
extern void rtclock_gettime(unsigned long long *tt);
extern void clock_sleepto(unsigned long long tt);
extern void start_launch_on_core(int (*f)(void *), void *arg, unsigned char core);
#endif

#ifdef _yuanji
//extern void yj_performance_start(void);
//extern void yj_performance_stop(void);
void yj_performance_start(void)
{
    int fd;
    char stop[]={"1"};

    fd=open("/sys/kernel/yjdbg_flags",O_CREAT|O_RDWR,S_IRWXU);
    write(fd,stop,strlen(stop));

    return;
}

void yj_performance_stop(void)
{
    int fd;
    char start[]={"0"};

    fd=open("/sys/kernel/yjdbg_flags",O_CREAT|O_RDWR,S_IRWXU);
    write(fd,start,strlen(start));

    return;
}

#endif


#ifdef _LINUX
static int processcore = 0;

static cpu_set_t set;
static cpu_set_t get;
#endif

void cplusplus_rtclock_gettime(double *curr_time)
{
	unsigned long long curr;
#ifdef _windriver
	rtclock_gettime(&curr);
	*curr_time = 0.000000001 * curr;
#else
	*curr_time = MPI_Wtime();
#endif
}

void do_calc(int num)
{
	int i, j;

	int NUM_CALC = NUM_PROC + 3;

	int calc_time = floor((NUM_CALC - num));

	double *tmp1 = (double *)gbuff1;
	double *tmp2 = (double *)gbuff2;
	double *tmp3 = (double *)gbuff3;
	double *tmp4 = (double *)gbuff4;
	double *tmp5 = (double *)gbuff5;

	for (i = 0; i < calc_time; i++)
	{
		for (j = 0; j < EMT_COMM_NUM; j++)
		{
			tmp1[j] = sin(tmp1[j] * 3.1415926 + 0.5);
			tmp2[j] = sin(tmp2[j] * 3.1415926 + 0.5);
		}
	}
	//update_gvar(-1);
}
static int gstatus = ginit;

#define TRUE 1
#define FALSE 0
#define PORT 8888

int update_gvar(int trig)
{
	int x; // random number
	//printf("function update_gvar ----\n");
	if (trig == -1)
	{
		double dtt = 0;
		cplusplus_rtclock_gettime(&dtt);
		x = (int)dtt % 10;
		//printf("function update_gvar, tt:%lld\n",tt);

		//printf("function update_gvar, tt%10=%d\n",x);
	}
	else
	{
		x = trig;
	}

	int i, j;
	int *tmp1 = (int *)gbuff1;
	int *tmp2 = (int *)gbuff2;
	int *tmp3 = (int *)gbuff3;
	int *tmp4 = (int *)gbuff4;
	int *tmp5 = (int *)gbuff5;
	int max = SIZE_GVar_EMT / sizeof(int);

	//printf("function udpate_gvar max=%d\n",max);
	for (i = 0; i < max - 100; i = i + 100)
	{
		//printf("function udpate_gvar i=%d\n",i);
		tmp1[i] = x;
		tmp2[i] = x;
		tmp3[i] = x;
		tmp4[i] = x;
		tmp5[i] = x;
	}

	return 0;
}

void emt_comm_both_sides_shm(int step, int *DIVI, int *DIVJ, int *sendshmid, void **sendshm, int *sendsemid, int *recvshmid, void **recvshm, int *recvsemid)
{
	//printf("emt_comm_both_size_shm begin:\n");
	int i, j;
	MPI_Status MpiStt;
	MPI_Request request[(NUM_DIV_PER + 1) * 2];
	MPI_Status mpistatus[(NUM_DIV_PER + 1) * 2];
	double buf[EMT_COMM_NUM];
	memset(buf, 0, EMT_COMM_NUM * sizeof(double));
	int iNtmp_recv;
	int iNtmp_send;
	int mode;

	int idx_buff = 0;
	int idx_request = 0;
	for (i = 0; i < NUM_DIV; i++)
	{
		//printf("check p 1----\n");
		if ((DIVI[i] == ProcInfo.EMTCALId) || (DIVJ[i] == ProcInfo.EMTCALId))
		{
			//load the data in the Isendbuff
			//printf("check p 2 ----\n");
			for (int j = 0; j < EMT_COMM_NUM; j++)
			{

				ISendBuf[idx_buff + j] = (i * 100 + j) * step;
				if(DEBUG)
					printf("ISendBuf[%d]=%lf\n",idx_buff + j,ISendBuf[idx_buff + j]);
			}

			if (DIVI[i] == ProcInfo.EMTCALId)
			{
				if(DEBUG)
					printf("check p4 ----------\n");
#ifdef _MPISENDRECV
				if(DEBUG)
				    printf("check p 5\n");
#ifdef _MPIISENDRECV
				

				if(DEBUG)
				{
					printf("MPI_Isend(buf:%d,count:%d,type:MPI_DOUBLE_PRECISION,source:%d,dest:%d,tag:%d)\n ",ISendBuf+idx_buff,EMT_COMM_NUM,ProcInfo.EMTCALId,DIVJ[i],33+i,ProcInfo.CommEMTCAL);
					int xx=0;
					for(;xx<EMT_COMM_NUM;xx++)
						printf(" %d:%lf   ",xx+1,*(ISendBuf+idx_buff+xx));
					printf("\n");

				}
				MPI_Isend((void*)(ISendBuf+idx_buff), EMT_COMM_NUM, MPI_DOUBLE_PRECISION, DIVJ[i], 33 + i, ProcInfo.CommEMTCAL, &request[idx_request]);
#else
				MPI_Send(SendBuf, EMT_COMM_NUM, MPI_DOUBLE_PRECISION, DIVJ[i], 33, ProcInfo.CommEMTCAL);
#endif
#else
				int idx_add = MPI_EMT_Send(&ISendBuf[idx_buff], &IRecvBuf[idx_buff], EMT_COMM_NUM, MPI_DOUBLE_PRECISION, DIVJ[i], 33, ProcInfo.CommEMTCAL, &request[idx_request]);
				idx_request += idx_add;
#endif
			}
			else if (DIVJ[i] == ProcInfo.EMTCALId)
			{
#ifdef _MPISENDRECV
#ifdef _MPIISENDRECV
                if(DEBUG)
				{
					printf("MPI_Isend(buf:%d,count:%d,type:MPI_DOUBLE_PRECISION,dest:%d,tag:%d)\n ",ISendBuf+idx_buff,EMT_COMM_NUM,DIVI[i],33+i,ProcInfo.CommEMTCAL);
					int xx=0;
					for(;xx<EMT_COMM_NUM;xx++)
					printf(" %d:%lf   ",xx+1,*(ISendBuf+idx_buff+xx));
					printf("\n");
				}
				MPI_Isend((void*)(ISendBuf+idx_buff), EMT_COMM_NUM, MPI_DOUBLE_PRECISION, DIVI[i], 33 + i, ProcInfo.CommEMTCAL, &request[idx_request]);
#else
				MPI_Send(SendBuf, EMT_COMM_NUM, MPI_DOUBLE_PRECISION, DIVI[i], 33, ProcInfo.CommEMTCAL);
#endif
#else
				int idx_add = MPI_EMT_Send(&ISendBuf[idx_buff], &IRecvBuf[idx_buff], EMT_COMM_NUM, MPI_DOUBLE_PRECISION, DIVI[i], 33, ProcInfo.CommEMTCAL, &request[idx_request]);
				idx_request += idx_add;
#endif
			}
			idx_buff += EMT_COMM_NUM;
		}
	}

	//MPI共享内存的同步区域
	MPI_Sync_Step(step);

	//���ݽ���
	idx_buff = 0;
	for (i = 0; i < NUM_DIV; i++)
	{
		if ((DIVI[i] == ProcInfo.EMTCALId) || (DIVJ[i] == ProcInfo.EMTCALId))
		{
			if (DIVI[i] == ProcInfo.EMTCALId)
			{
#ifdef _MPISENDRECV
#ifdef _MPIISENDRECV

				MPI_Irecv((void*)(IRecvBuf+idx_buff), EMT_COMM_NUM, MPI_DOUBLE_PRECISION, DIVJ[i], 33 + i, ProcInfo.CommEMTCAL, &request[idx_request]);
				if(DEBUG)
				{
					printf("MPI_Irecv(buf:%d,count:%d,type:MPI_DOUBLE_PRECISION,source:%d,dest:%d,tag:%d)\n ",IRecvBuf+idx_buff,EMT_COMM_NUM,DIVJ[i],ProcInfo.EMTCALId,33+i,ProcInfo.CommEMTCAL);
					int xx=0;
					for(;xx<EMT_COMM_NUM;xx++)
						printf(" %d:%lf   ",xx+1,*(IRecvBuf+idx_buff+xx));
					printf("\n");
				}
#else
				MPI_Recv(RecvBuf, EMT_COMM_NUM, MPI_DOUBLE_PRECISION, DIVJ[i], 33, ProcInfo.CommEMTCAL, &MpiStt);
#endif
#else
				//shm_recv(RecvBuf, EMT_COMM_NUM, 0, recvshm[i], recvsemid[i]); //MuQ 181215
				MPI_EMT_Recv(&IRecvBuf[idx_buff], EMT_COMM_NUM, MPI_DOUBLE_PRECISION, DIVJ[i], 33, ProcInfo.CommEMTCAL, &Msgpos[i], step); //MuQ 181215
#endif
			}
			else if (DIVJ[i] == ProcInfo.EMTCALId)
			{
#ifdef _MPISENDRECV
#ifdef _MPIISENDRECV

				MPI_Irecv((void*)(IRecvBuf+idx_buff), EMT_COMM_NUM, MPI_DOUBLE_PRECISION, DIVI[i], 33 + i, ProcInfo.CommEMTCAL, &request[idx_request]);
				if(DEBUG)
				{
					printf("MPI_Irecv(buf:%d,count:%d,type:MPI_DOUBLE_PRECISION,source:%d,dest:%d,tag:%d)\n ",IRecvBuf+idx_buff,EMT_COMM_NUM,DIVI[i],ProcInfo.EMTCALId,33+i);
					int xx=0;
					for(;xx<EMT_COMM_NUM;xx++)
						printf(" %d:%lf   ",xx+1,*(IRecvBuf+idx_buff+xx));
					printf("\n");
				}
#else
				MPI_Recv(RecvBuf, EMT_COMM_NUM, MPI_DOUBLE_PRECISION, DIVI[i], 33, ProcInfo.CommEMTCAL, &MpiStt);
#endif
#else
				MPI_EMT_Recv(&IRecvBuf[idx_buff], EMT_COMM_NUM, MPI_DOUBLE_PRECISION, DIVI[i], 33, ProcInfo.CommEMTCAL, &Msgpos[i], step); //MuQ 181215
#endif
			}
			idx_buff += EMT_COMM_NUM;
			//if (idx_buff>=8) printf("idx_buff is too much\n");
			//idx_request++;
		}
	}
	if ((ProcInfo.EMTCALId == 0) && (step == 1))
	{
		printf("start to wait for MPI finished, with idx_request = %d.\n", idx_request);
	}
	if (idx_request > 0)
		MPI_Waitall(idx_request, &request[0], &mpistatus[0]);

	if ((ProcInfo.EMTCALId == 0) && (step == 1))
	{
		printf("MPI finished, with idx_request = %d.\n", idx_request);
	}
	//double tx=0;
	//cplusplus_rtclock_gettime(&tx);
	/*
	int Ierror = SendCheck(ISendBuf, IRecvBuf, idx_buff);
	if (Ierror != -1)
	{
		//数据传输错误
		printf(" ID:%lf Data Transfers with Error in Step %d in Ksub %d, the %dth data is error.\n", tx,step, iKsub, Ierror);
		printf(" ID:%lf ISendBuff Data is: \n",tx);
		for (int m = 0; m < idx_buff; m++)
		{
			printf("%d ", ISendBuf[m]);
		}
		printf(" \n ID:%lf ISendBuff Data print done: \n",tx);
		printf("\n ID:%lf IRecvBuff Data is: \n",tx);
		for (int m = 0; m < idx_buff; m++)
		{
			printf("%d ", IRecvBuf[m]);
		}
		printf("\n ID:%lf IRecvBuff Data print done: \n",tx);
	}
	*/
}

int warm = WARM_COUNT;
//void *thread_emt(__attribute__((unused)) void *arg)

#define  CN  50000000
double time1[CN],time2[CN],time3[CN],t_com[CN],t_cal[CN],time6[CN];

unsigned long int  n_overcal = 0;
unsigned long int n_overval = 0;
unsigned long int n_overcomm = 0;
unsigned long pid;
void thread_emt()
{
	//printf("start to run threat_emt:\n");

	//double t1, t2, t3, t4, t5, t6,max_val = 0, val, cal, sum_cal = 0, avg_cal = 0, sum_val = 0, avg_val = 0, comm, sum_comm = 0, avg_comm = 0;
	//unsigned long long step_all_max = 0, step_all_min = 0, step_calc_max = 0, step_calc_min = 0;
	//unsigned long long step_comm_max = 0, step_comm_min = 0;
	char filename[40];
	int aa = 0;
	double *total_time = malloc((nstep - warm) * sizeof(double));

	memset(total_time, 0, ((nstep - warm) * sizeof(double)));
	//unsigned int core_id = *(unsigned int*)arg;
	//set_thread_affinity(core_id);
	FILE *fp;

	pid = getpid();
	//sprintf(filename, "total_comm_%d.txt", ProcInfo.Id);
	//fp = fopen(filename, "wb");
#ifdef RUN_LONG_TIME_TEST
	//time_status_init();
#endif
	printf(" Warm system %d times\n",  warm);
	//MPI_Barrier(ProcInfo.CommCAL);			// ͬ�����MPI���̵�thread

	//MPI_Barrier(MPI_COMM_WORLD);

	// �������ڿ���
	cplusplus_rtclock_gettime(&ts);
	//DIVI??DIVJ???????��???????????
	int *DIVI = calloc(1000, sizeof(int));
	int *DIVJ = calloc(1000, sizeof(int));
	int *sendshmid = calloc(1000, sizeof(int));
	int *recvshmid = calloc(1000, sizeof(int));
	int *sendsemid = calloc(1000, sizeof(int));
	int *recvsemid = calloc(1000, sizeof(int));

	void **sendshm = calloc(1000, sizeof(void *));
	void **recvshm = calloc(1000, sizeof(void *));

	//????DIV????????
	EMT_DIV_GET(DIVI, DIVJ);

	MPI_Win_Lock();
#ifdef _yuanji
//        yj_performance_start();
#endif
	long int eff_steps;

    double* t1=time1;
	double* t2=time2;
	double* t3=time3;
#ifdef _JN_OS   
	printf("shield(%d, %d, %d)\n", ProcInfo.Id, ProcInfo.NumProc, ProcInfo.Id + 1);
	shield(ProcInfo.Id, ProcInfo.NumProc, ProcInfo.Id + 1);
#endif
	for (step = 1; step <= nstep; step++)
	{
		if(DEBUG)
		{
			if(step>3)
			 	break;
		}
		//printf("step:%d\n",step);

		cplusplus_rtclock_gettime(t1);
		//printf("before emt commn:%d\n",step);
		emt_comm_both_sides_shm(step, DIVI, DIVJ, sendshmid, sendshm, sendsemid, recvshmid, recvshm, recvsemid);
		//printf("after emt commn:%d\n",step);
		cplusplus_rtclock_gettime(t2);
		do_calc(iKsub); //根据子网的子网号递减iKsubiKsub
		update_gvar(0);
		cplusplus_rtclock_gettime(t3);

		t1++;
		t2++;
		t3++;
/*
#ifdef _ADDTIMEBUFF
		double tcal = t4 - t3;
		if (tcal < avg_cal + 0.00001) //缓冲10us
		{
			int usec = floor((avg_cal + 0.00001 - tcal) * 1e6);
			cplusplus_rtclock_sleep(usec);
		}
#endif
*/

		//cplusplus_rtclock_gettime(t5);
		//cplusplus_rtclock_gettime(t6);

/*

        eff_steps=step-warm;
		val = t6 - t1; //emt comm
		if ((val > tmax_val) && (step > warm))
		{
			tmax_val = val;
			step_all_max = step;
		}

		if(eff_steps > 0)
			sum_val += val;

		if (step >warm  && (val > 1.3 * sum_val / eff_steps) )
		{
			n_overval++;
			//printf("eff_steps=%ld\n",eff_steps);
			//printf("n_overal =%d step=%ld   val=%lf   avg_val=%lf  avg_val*1.3=%lf\n",n_overval,step,val,sum_val/eff_steps, 1.3 * sum_val / eff_steps);
		}
		if ((val < tmin_val) && (step > warm))
		{
			tmin_val = val;
			step_all_min = step;
		}



		cal = t4 - t3; //cal value
		if ((cal > tmax_cal) && (step > warm))
		{
			tmax_cal = cal;
			step_calc_max = step;
		}

		
		if(step > warm)
			sum_cal += cal;
		if ((step > warm) && (cal > 1.3 * sum_cal / eff_steps ))
		{
			n_overcal++;
		}
		if ((cal < tmin_cal) && (step > warm))
		{
			tmin_cal = cal;
			step_calc_min = step;
		}


		comm = t3 - t1; //comm value

		if(step > warm)
		{
			sum_comm += comm;
			//avg_val = sum_val /( step-warm);
			//avg_cal = sum_cal / ( step-warm);
			//avg_comm = sum_comm / ( step-warm);
		}

		if ((step > warm) &&   (comm > tmax_comm) )
		{
			tmax_comm = comm;
			step_comm_max = step;
		}
		if ((step > warm)   && (comm > 1.3 * sum_comm / eff_steps)  )
		{
			n_overcomm++;
		}
		if ( (step > warm)  && (comm < tmin_comm))
		{
			tmin_comm = comm;
			step_comm_min = step;
		}


*/
		//if (PRINT_DETAIL)
			//printf("core=%d,step:%d,t3-t1:%f,t4-t3:%f,t5-t4:%f,t6-t4:%f,t6-t1:%f\n",
				 //  pid, step, t3 - t1, t4 - t3, t5 - t4, t6 - t4, t6 - t1);
	}
	
#ifdef _yuanji    
//        yj_performance_stop();   
#endif	

	MPI_Win_unLock();

	//cplusplus_rtclock_gettime(&tc);

	//avg_val = sum_val / (nstep - warm);
	//avg_cal = sum_cal / (nstep - warm);
	//avg_comm = sum_comm / (nstep - warm);
	/*printf("core=%d,Id:%d CALC: Mean %f Max %f (%lld) Min %f (%lld) Overtime(%lld) | Comm: Mean %f Max %f (%lld) Min %f (%lld) Overtime(%lld) | TOT: Mean %f Max %f (%lld) Min %f (%lld) Overtime(%lld) \n",
		   pid, ProcInfo.Id, avg_cal, tmax_cal, step_calc_max, tmin_cal, step_calc_min, n_overcal, avg_comm, tmax_comm, step_comm_max, tmin_comm, step_comm_min, n_overcomm,
		   avg_val, tmax_val, step_all_max, tmin_val, step_all_min, n_overval);
		   
    */
	printdata();

	stopprogram = 1;
	cplusplus_rtclock_sleep(1000);
	return;
}

int printdata()
{

	int i=warm;
	double avg_com=0,avg_cal=0,avg_total=0;
	double avg_com_thresh=0,avg_cal_thresh=0,avg_total_thresh=0;

	tmax_cal=0;
	long int step_calc_max=0;
	long int step_calc_min=0;
	long int step_comm_max=0;
	long int step_comm_min=0;
	long int step_all_max=0;
	long int step_all_min=0;

	for(;i<nstep;i++)
	{
		t_com[i]=time2[i]-time1[i];
		t_cal[i]=time3[i]-time2[i];
		avg_com+=t_com[i];
		avg_cal+=t_cal[i];

		if(t_cal[i]>tmax_cal)
		{
			tmax_cal=t_cal[i];
			step_calc_max=i;
		}

		if(t_cal[i]<tmin_cal)
		{
			tmin_cal=t_cal[i];
			step_calc_min=i;
		}
		if(t_com[i]>tmax_comm)
	    {
			tmax_comm=t_com[i];
			step_comm_max=i;

		}
		if(t_com[i]<tmin_comm)
	    {
			tmin_comm=t_com[i];
			step_comm_min=i;

		}

		if(t_cal[i]+t_com[i]>tmax_val)
		{
			tmax_val=t_cal[i]+t_com[i];
			step_all_max=i;
		}

		if(t_cal[i]+t_com[i]<tmin_val)
		{
			tmin_val=t_cal[i]+t_com[i];
			step_all_min=i;
		}
		

	}
	avg_com=avg_com/(i-warm);
	avg_cal=avg_cal/(i-warm);
	avg_total=avg_com+avg_cal;
	avg_com_thresh=1.3*avg_com;
	avg_cal_thresh=1.3*avg_cal;
	avg_total_thresh=1.3*avg_total;

	for(i=warm;i<nstep;i++)
	{

		if(t_com[i]>avg_com_thresh)
		   n_overcomm++;
		
		if(t_cal[i]>avg_cal_thresh )
			n_overcal++;

		if(t_com[i]+t_cal[i]>avg_total_thresh)
			n_overval++;

	}
	printf("core=%d,Id:%d CALC: Mean %f Max %f (%lld) Min %f (%lld) Overtime(%lld) | Comm: Mean %f Max %f (%lld) Min %f (%lld) Overtime(%lld) | TOT: Mean %f Max %f (%lld) Min %f (%lld) Overtime(%lld) \n",
		   pid, ProcInfo.Id, avg_cal, tmax_cal, step_calc_max, tmin_cal, step_calc_min, n_overcal, avg_com, tmax_comm, step_comm_max, tmin_comm, step_comm_min, n_overcomm,
		   avg_total, tmax_val, step_all_max, tmin_val, step_all_min, n_overval);


   

}

int main(int argc, char *argv[])
{

	int core = 9;
	int count = 0;
	struct sched_param parm;
	struct rlimit rlimit_new;

	stopprogram = 0;

   if(argc > 4)
   {
	   count = atoi(argv[1]);
	   warm=atoi(argv[2]);
	   NUM_PROC=atoi(argv[3]);
	   NUM_DIV_PER=atoi(argv[4]);
	   printf("count=%d, warm=%d  NUM_PROC=%d  NUM_DIV_PER=%d\n",count,warm,NUM_PROC,NUM_DIV_PER);
   }

   	else if (argc > 2)
	{
		count = atoi(argv[2]);
		warm = atoi(argv[3]);
		printf("count=%d  warm =%d\n", count, warm);
	}
	else if (argc > 1)
	{
		count = atoi(argv[1]);
	}
	if (count > 0)
		nstep = count;
	else
		nstep = CALCU_NUM;

	NUM_DIV=(NUM_PROC - 1 - NUM_DIV_PER) / 2 * NUM_DIV_PER;

	printf("test count =%u \n", nstep);
	tstep = 50000;

	isCalculationOver = 0;

	memset(&ProcInfo, 0, sizeof(struct SPROCINFO));

	ProcIO = -1; //非IO进程
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &ProcInfo.Id);
	MPI_Comm_size(MPI_COMM_WORLD, &ProcInfo.NumProc);
	//printf("current MPIRank is %d, the size of COMM_WORLD is %d;\n",ProcInfo.Id,ProcInfo.NumProc);

	MPI_Sharedmemory_init();
	init_comm();

	sleep(10);

	init_buf();
	sleep(10);
	init_gvar();

	MPI_Barrier(MPI_COMM_WORLD);

	printf("ProcInfo.Id:  %d  after MPI COMM WORLD Barrier\n", ProcInfo.Id);

	yj_performance_start();  
	thread_emt();
	yj_performance_stop();

	while (stopprogram == 0)
	{
		cplusplus_rtclock_sleep(5000000);
	}
	printf("thread is finished in PID %d.\n", ProcInfo.Id);
	MPI_Barrier(MPI_COMM_WORLD);

	//finalize_buf();

	finalize_comm();

	MPI_Finalize();

	munlockall();

	printf("MPI proc: %d  after release_rtcore....\n", ProcInfo.Id);

	release_gvar();

	rt_core_id = 0;

	return 0;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  支撑函数 //
//  无需关注 //
//
//
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void init_gvar()
{
	gbuff1 = malloc(SIZE_GVar_EMT);
	if (gbuff1 == NULL)
	{
		printf("allocate mem for gbuff1 failed,and exit!\n");
		exit(-1);
	}
	printf("init_gval gbuff1=%lld\n", gbuff1);
	gbuff2 = malloc(SIZE_GVar_EMT);
	if (gbuff2 == NULL)
	{
		printf("allocate mem for gbuff2 failed,and exit!\n");
		exit(-1);
	}
	printf("init_gval gbuff2=%lld\n", gbuff2);
	gbuff3 = malloc(SIZE_GVar_EMT);
	if (gbuff3 == NULL)
	{
		printf("allocate mem for gbuff3 failed,and exit!\n");
		exit(-1);
	}
	printf("init_gval gbuff3=%lld\n", gbuff3);
	gbuff4 = malloc(SIZE_GVar_EMT);
	if (gbuff4 == NULL)
	{
		printf("allocate mem for gbuff4 failed,and exit!\n");
		exit(-1);
	}
	printf("init_gval gbuff4=%lld\n", gbuff4);
	gbuff5 = malloc(SIZE_GVar_EMT);
	if (gbuff5 == NULL)
	{
		printf("allocate mem for gbuff5 failed,and exit!\n");
		exit(-1);
	}
	printf("init_gval gbuff5=%lld\n", gbuff5);
	printf("allocate mem for all global vars successfully!\n");

	memset(gbuff1, 0, SIZE_GVar_EMT);
	memset(gbuff2, 0, SIZE_GVar_EMT);
	memset(gbuff3, 0, SIZE_GVar_EMT);
	memset(gbuff4, 0, SIZE_GVar_EMT);
	memset(gbuff5, 0, SIZE_GVar_EMT);
}

void release_gvar()
{
	printf("release_gvar gbuff1=%lld\n", gbuff1);
	printf("release_gvar gbuff2=%lld\n", gbuff2);
	printf("release_gvar gbuff3=%lld\n", gbuff3);
	printf("release_gvar gbuff4=%lld\n", gbuff4);
	printf("release_gvar gbuff5=%lld\n", gbuff5);

	if (gbuff1 != NULL)
	{
		free(gbuff1);
		gbuff1 = NULL;
	}
	printf("free  mem for gbuff1 vars successfully!\n");
	if (gbuff2 != NULL)
	{
		free(gbuff2);
		gbuff2 = NULL;
	}
	printf("free  mem for gbuff2 vars successfully!\n");
	if (gbuff3 != NULL)
	{
		free(gbuff3);
		gbuff3 = NULL;
	}
	printf("free  mem for gbuff3 vars successfully!\n");
	if (gbuff4 != NULL)
	{
		free(gbuff4);
		gbuff4 = NULL;
	}
	printf("free  mem for gbuff4 vars successfully!\n");
	if (gbuff5 != NULL)
	{
		free(gbuff5);
		gbuff5 = NULL;
	}
	printf("free  mem for gbuff5 vars successfully!\n");
	printf("free  mem for all global vars successfully!\n");
}



int init_buf()
{
	int i;

	for (i = 0; i < EMT_COMM_NUM; i++)
	{
		SendBuf[i] = (1.0 + ProcInfo.EMTCALId) * i;

		//printf("init buff SendBuf[%d]=%f\n",i,SendBuf[i]);
		RecvBuf[i] = 0.0;
	}

	//DataBuf = new double[EMT_COMM_NUM * nstep];
	//	DataBuf = malloc(EMT_COMM_NUM * nstep * sizeof(double));

	return 0;
}

int SendCheck(double *buf, double *validbuf, int length) //mode =  1: Recv mode, mode = 0: Send mode
{
	int i;
	for (i = 0; i < length; i++)
	{
		//检测是不是正确
		if ((buf[i] - validbuf[i]) > 1.0e-6)
		{
			return i;
		}
	}
	return -1;
}

void DIVFileProduce(int *DIVI, int *DIVJ)
{
	FILE *fp = fopen("testmpionwr.DIV", "w+");
	for (int i = 0; i < NUM_DIV; i++)
	{
		fprintf(fp, "1,%6d,%d,%6d,%6d,%6d,%6d,4,%6d,\n", i + 1, 1, DIVI[i] + 1, 50, DIVJ[i] + 1, 50, EMT_COMM_NUM / 6);
	}
	fclose(fp);
}

void EMT_DIV_GET(int *DIVI, int *DIVJ)
{
	int i;
	int startprocess = (int)floor(1.0 * (NUM_PROC - 1 - NUM_DIV_PER) / 2);
	//int NUM_DIV = NUM_PROC-1-NUM_DIV_PER)/2*NUM_DIV_PER;

	for (i = 0; i < NUM_DIV; i++)
	{
		DIVI[i] = (int)floor((i / NUM_DIV_PER) + 1.0e-12); //i 0  j 200 190 180 170 160 150 140 130
		//i 1  j 199 189 179 169 159 149 139 129
		DIVJ[i] = startprocess + DIVI[i] + (i % NUM_DIV_PER);
		if (ProcInfo.EMTCALId == 0)
			printf("DIVI[%d] = %d,DIVJ[%d]=%d\n", i, DIVI[i], i, DIVJ[i]);
		//if (DIVJ[i] < 0 ) DIVJ[i] = ProcInfo.NumProcCAL - DIVI[i] - (i % NUM_DIV_PER) - NUM_DIV_PER;
	}
	DIVFileProduce(DIVI, DIVJ);
}

// �������н��̵����ͣ����ɲ�ͬ��ͨ����
int init_comm()
{
	int *IDataRecv;
	int *ProcIdx;
	int i, j, Ntype;

	//IDataRecv = new int[ProcInfo.NumProc];
	IDataRecv = malloc(ProcInfo.NumProc * sizeof(int));
	//ProcIdx = new int[ProcInfo.NumProc];
	ProcIdx = malloc(ProcInfo.NumProc * sizeof(int));

	if (RUN_EMTIO == 1 && ProcInfo.Id == NUM_PROC)
	{
		Ntype = SIMUTYPE_EMTIO; //���������ͺ�
	}
	else if (RUN_EMTIO == 1 && ProcInfo.Id == NUM_PROC + 1)
	{
		Ntype = SIMUTYPE_GUI;
	}
	else
	{
		Ntype = SIMUTYPE_EMTSUB;
	}

	MPI_Group GroupAll, GroupST, GroupEMT, GroupEMTCAL, GroupCAL; //ȫ�������顢������̬�����顢�����̬�����顢�����̬���������顢���������

	MPI_Comm_group(MPI_COMM_WORLD, &GroupAll);

	// ÿ�����̶������Լ������ͺţ����ռ����н��̵����ͺ�

	ProcInfo.SimType = Ntype;

	printf("procInfo.Id=%d,  Ntypes=%d\n", ProcInfo.Id, Ntype);
	MPI_Allgather(&Ntype, 1, MPI_INTEGER, IDataRecv, 1, MPI_INTEGER, MPI_COMM_WORLD);

	//printf("after MPI_ALLgather:%d \n",ProcInfo.Id);
	// ���м������ͨ����
	ProcInfo.NumProcCAL = 0;
	ProcInfo.NumProcEMTIO = 0;
	for (i = 0; i < ProcInfo.NumProc; i++)
	{
		if (IDataRecv[i] <= 200) // ���м������
		{
			ProcIdx[ProcInfo.NumProcCAL] = i;
			ProcInfo.NumProcCAL++;
		}

		//printf("IDataRecv[%d] = %d\n", i, IDataRecv[i]);

		if (IDataRecv[i] == SIMUTYPE_EMTIO) // һ������ֻ��1��emtio����
		{
			ProcInfo.EMTIOId = i;

			printf("set ProcInfo.EMTIOId=%d\n", i);
			ProcInfo.NumProcEMTIO++;
		}
	}
	printf("init_comm ProcInfo.Id=%d\n", ProcInfo.Id);

	//printf("bk 1-------------------:   procInfo.Id=%d,  Ntypes=%d\n",ProcInfo.Id,Ntype);

	int tmp = 0;
	for (; tmp < ProcInfo.NumProcCAL; tmp++)
		// printf("bk 1.1:   procInfo.Id=%d,  ProcIdx[%d]=%d\n",ProcInfo.Id,tmp,ProcIdx[tmp]);

		//if (ProcInfo.NumProcCAL>0  && Ntype != SIMUTYPE_EMTIO)
		if (ProcInfo.NumProcCAL > 0)
		{
			MPI_Group_incl(GroupAll, ProcInfo.NumProcCAL, ProcIdx, &GroupCAL); //�γɼ��������

			//printf("GroupCAL:%0x  ProcInfo.NumProcCAL=%d  \n",GroupCAL,ProcInfo.Id);
			MPI_Comm_create(MPI_COMM_WORLD, GroupCAL, &ProcInfo.CommCAL); //�γɼ���ͨ����
																		  //printf("GroupCAL:%0x  ProcInfo.NumProcCAL=%d\n ",GroupCAL,ProcInfo.Id);
		}
		else
		{
			ProcInfo.CommCAL = MPI_COMM_NULL;
		}

	if (ProcInfo.CommCAL != MPI_COMM_NULL)

		MPI_Comm_rank(ProcInfo.CommCAL, &ProcInfo.CALId);

	int size_t2;
	MPI_Group_size(GroupCAL, &size_t2);

	// ������̬ͨ����
	ProcInfo.NumProcST = 0;
	printf("ProcInfo.NumProc=%d\n", ProcInfo.NumProc);
	for (i = 0; i < ProcInfo.NumProc; i++)
	{
		if (IDataRecv[i] <= 100) // ���л�����̬����
		{
			ProcIdx[ProcInfo.NumProcST] = i;
			ProcInfo.NumProcST++;
		}
	}
	printf("init_comm ProcInfo.NumProcST=%d\n", ProcInfo.NumProcST);

	if (ProcInfo.NumProcST > 0)
	{
		MPI_Group_incl(GroupCAL, ProcInfo.NumProcST, ProcIdx, &GroupST); //�γɻ�����̬������
		MPI_Comm_create(ProcInfo.CommCAL, GroupST, &ProcInfo.CommST);	 //�γɻ�����̬ͨ����
	}
	else
	{
		ProcInfo.CommST = MPI_COMM_NULL;
	}

	// �����̬ͨ����
	ProcInfo.NumProcEMT = 0;

	//int emtidx[100];
	for (i = 0; i < ProcInfo.NumProc; i++)
	{
		if ((IDataRecv[i] > 100) && (IDataRecv[i] <= 200))
		{
			ProcIdx[ProcInfo.NumProcEMT] = i;
			ProcInfo.NumProcEMT++;
		}
	}

	if (ProcInfo.NumProcEMT > 0)
	{
		MPI_Group_incl(GroupCAL, ProcInfo.NumProcEMT, ProcIdx, &GroupEMT); //?��????????????
		MPI_Comm_create(ProcInfo.CommCAL, GroupEMT, &ProcInfo.CommEMT);	   //?��???????????
	}
	else
	{
		ProcInfo.CommEMT = MPI_COMM_NULL;
	}

	if (ProcInfo.CommEMT != MPI_COMM_NULL)

		MPI_Comm_rank(ProcInfo.CommEMT, &ProcInfo.EMTId);

	int size_t;
	MPI_Group_size(GroupEMT, &size_t);

	//printf("size of EMT:  %d\n",size_t);

	/*
	while(1)
	{
		printf("in sleep----\n");
		sleep(10);
	}
*/

	//printf("bk 6-------------------:   procInfo.Id=%d,  Ntypes=%d\n",ProcInfo.Id,Ntype);
	// �����̬����ͨ���壨�������̬��������
	ProcInfo.NumProcEMTCAL = 0; //�����̬������
	ProcInfo.NumProcEMTPhy = 0; //�����̬�����ӿ���Ŀ
	for (i = 0; i < ProcInfo.NumProc; i++)
	{
		if (SIMUTYPE_EMTSUB == IDataRecv[i])
		{
			ProcIdx[ProcInfo.NumProcEMTCAL] = i - ProcInfo.NumProcST; //????????????????????????��??????????????????????????emt.h?��??????????
			ProcInfo.NumProcEMTCAL++;
		}
		if (SIMUTYPE_EMTPHY == IDataRecv[i])
		{
			ProcInfo.NumProcEMTPhy++;
		}
	}
	//printf("init_comm ProcInfo.NumProcEMTCAL=%d\n",ProcInfo.NumProcEMTCAL);

	if (ProcInfo.NumProcEMTCAL > 0)
	{
		//???????????????GroupEMT?????��??????GroupEMTCAL
		MPI_Group_incl(GroupEMT, ProcInfo.NumProcEMTCAL, ProcIdx, &GroupEMTCAL); //?��????????????????
		MPI_Comm_create(ProcInfo.CommEMT, GroupEMTCAL, &ProcInfo.CommEMTCAL);	 //?��???????????????
#ifdef _MPI_SHM_PROXY
		//start to increase the info of the CommEMTCal
		int iSubId = iKsub - 1;
		MPI_Allgather(&iSubId, 1, MPI_INTEGER, IDataRecv, 1, MPI_INTEGER, ProcInfo.CommEMTCAL);
		MPI_Group GroupEMTCAL2 = GroupEMTCAL;
		MPI_Group_incl(GroupEMTCAL2, ProcInfo.NumProcEMTCAL, IDataRecv, &GroupEMTCAL); //形成电磁暂态子网进程
		MPI_Comm CommEMTCAL = ProcInfo.CommEMTCAL;
		MPI_Comm_create(CommEMTCAL, GroupEMTCAL, &ProcInfo.CommEMTCAL); //形成电磁暂态子网通信体
		MPI_Group_free(&GroupEMTCAL2);
		MPI_EMTCAL_RANK(ProcInfo.CommEMTCAL, ProcInfo.NumProcEMTCAL, IDataRecv);

#endif
	}
	else
	{
		ProcInfo.CommEMTCAL = MPI_COMM_NULL;
	}
	MPI_Comm_rank(ProcInfo.CommEMTCAL, &ProcInfo.EMTCALId);

	MPI_Group_free(&GroupAll);
	if (ProcInfo.NumProcCAL > 0)
		MPI_Group_free(&GroupCAL);
	if (ProcInfo.NumProcST > 0)
		MPI_Group_free(&GroupST);
	if (ProcInfo.NumProcEMT > 0)
		MPI_Group_free(&GroupEMT);
	if (ProcInfo.NumProcEMTCAL > 0)
		MPI_Group_free(&GroupEMTCAL);

	//delete[] IDataRecv;
	free(IDataRecv);
	//delete[] ProcIdx;
	free(ProcIdx);

	printf("emt, NumProc: %d, NumProcCAL: %d, NumProcST: %d, NumProcEMT: %d, NumProcEMTCAL: %d, NumProcEMTPhy: %d, NumProcEMTIO: %d\n", ProcInfo.NumProc, ProcInfo.NumProcCAL, ProcInfo.NumProcST, ProcInfo.NumProcEMT, ProcInfo.NumProcEMTCAL, ProcInfo.NumProcEMTPhy, ProcInfo.NumProcEMTIO);
	printf("emt, Id: %d, CALId; %d, EMTId: %d, EMTCALId: %d\n", ProcInfo.Id, ProcInfo.CALId, ProcInfo.EMTId, ProcInfo.EMTCALId);

	return 0;
}

int finalize_comm()
{
	//if (ProcInfo.NumProcEMTPhy>0) delete[] ProcInfo.EMTPhyId;
	if (ProcInfo.NumProcEMTPhy > 0)
		free(ProcInfo.EMTPhyId);

#ifdef SHM_EMTIO
	if (ProcInfo.NumProcEMTIO > 0)
	{
		deinit_shm(ProcInfo.EMTCALId);
	}
#endif

	return 0;
}

void cplusplus_rtclock_sleep(int usec)
{
	unsigned long long begin, add;
	//rtclock_gettime(&begin);
#ifdef _windriver
	add = usec * 1000;
	rtclock_gettime(&begin);
	begin += add;
	clock_sleepto(begin);
#else
	struct timeval tv1;
	struct timeval tv2;
	double tmp;
	double tmptheda;
	int i;
	gettimeofday(&tv1, NULL);
	gettimeofday(&tv2, NULL);

	while ((tv2.tv_usec - tv1.tv_usec) + (tv2.tv_sec - tv1.tv_sec) * 1000000 < usec)
	{
		for (i = 0; i < 100; i++)
		{
			tmptheda = asin(tmp);
		}
		gettimeofday(&tv2, NULL);
	}
#endif
}
