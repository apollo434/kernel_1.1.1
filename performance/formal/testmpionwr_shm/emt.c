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
#include <i_malloc.h>
#include "test.h"
#include "emt.h"
#include "Pub_Def.h"
#include "MPI_Comm.h"
#include <time.h>
#include "shmFunc.h"
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

#ifdef _LINUX
pthread_t thread;
#endif //_LINUX
#define COMM_PROC 7
#define NUM_PROC 23
#define NUM_DIV_PER 9
#define NUM_DIV (NUM_PROC-1-NUM_DIV_PER)/2*NUM_DIV_PER //需要满足整数要求
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
//#define _MPIISENDRECV
#define _MPI_SHM_PROXY



#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
 
#define MAP_LENGTH      (2*1024*1024)

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


static int page_id = 0;
void * alloc_one_2m_page(int cpuid)
{
     char filename[64];
     int fd;
     void * addr;
			  
     if (page_id > 1023)
	     return NULL;
     /* create a file in hugetlb fs */
     sprintf(filename, "/mnt/huge/%d_test_%d", cpuid, page_id++);
     fd = open(filename, O_CREAT | O_RDWR);
     if(fd < 0){
         printf("Err: %d", errno);
         return NULL;
    }  
				  
    /* map the file into address space of current application process */
    addr = mmap(0, MAP_LENGTH, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(addr == MAP_FAILED){
         printf("Err: %d", errno);
         close(fd);
         unlink(filename);
         return NULL;
    }  
					  
    close(fd);
    return addr;
}

int free_one_2m_page(void * addr)
{	
    munmap(addr, MAP_LENGTH);
    return 0;
}


char *gbuff1;
char *gbuff2;
char *gbuff3;
char *gbuff4;
char *gbuff5;

int iKsub;
int iMYID;

#ifdef _LINUX

static 	cpu_set_t set;
static 	cpu_set_t set_rtcore;
static  cpu_set_t get;
static 	cpu_set_t get_rtcore;
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
//double ISendBuf[BUFFSIZE];
//double IRecvBuf[BUFFSIZE];
double *ISendBuf = NULL;
double *IRecvBuf = NULL;
MessagePos Msgpos[NUM_DIV+1];
int stopprogram;
#ifdef SHM_EMTIO
struct shm_per_task shm_ctrl_per_task;
#endif

extern void EMT_DIV_GET(int *DIVI, int *DIVJ);
extern void cplusplus_rtclock_sleep(int usec);
int ProcIO;

#ifdef _windriver
extern void rtclock_gettime(unsigned long long *tt);
extern void clock_sleepto(unsigned long long tt);
extern void start_launch_on_core(int (*f)(void *), void *arg, unsigned char core);
#endif

#ifdef _LINUX
static int processcore = 0;

static 	cpu_set_t set;
static  cpu_set_t get;
#endif	

#if 1
void  sleep_rt (unsigned long long delay)
{
#if 0
   unsigned long long curr,target;
   int i;

   curr = currentcycles();
   //target = curr + (delay*2300000000);
   target = curr + (delay*2300000);


   do
   {
     curr = currentcycles();
   }while(curr < target);
#else
	sleep(delay);
#endif
   return;
}


void  usleep_rt (unsigned long long delay)
{
#if 0
   unsigned long long curr,target;
   int i;

   curr = currentcycles();
   //target = curr + (delay*2300000000);
   target = curr + (delay*2300);


   do
   {
     curr = currentcycles();
   }while(curr < target);
#else
	usleep(delay);
#endif
   return;
}


#endif

typedef unsigned long long cycles_t;
cycles_t currentcycles() {
    cycles_t hi,lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );;
}


void  cplusplus_rtclock_gettime (double *curr_time)
{
	unsigned long long curr;
#ifdef _windriver
	rtclock_gettime(&curr);
	*curr_time = 0.000000001*curr;
#else
//	*curr_time = MPI_Wtime();
    curr = currentcycles();
//    *curr_time = 0.000000001*(curr*0.434782);
    *curr_time = 0.00000000043*curr;
#endif
}


#ifdef STATISTICS_LOG /* log the time spent for each step */

double time_info_1[CALCU_NUM];
double time_info_2[CALCU_NUM];
double time_info_3[CALCU_NUM];
double time_info_4[CALCU_NUM];
double time_info_5[CALCU_NUM];
double time_info_6[CALCU_NUM];
double time_info_7[CALCU_NUM];
double time_info_8[CALCU_NUM];

void print_and_log(void)
{
	char str[2048];
	int i;
	int tmp;

	/* ouput the original data to file */
	if ((fp11 = fopen("time_info.txt", "w")) != NULL)
	{

		for (i = 0; i < CALCU_NUM; i++)
		{
			tmp = sprintf(str, "%d	%lld	%lld	%lld	%lld	%lld	%lld", i,
										time_info_1[i], time_info_2[i], time_info_3[i], time_info_4[i], time_info_5[i], time_info_6[i]);

			sprintf(&str[tmp], "	%lld	%lld	%lld	%lld	%lld	%lld\n",
							time_info_2[i] - time_info_1[i], time_info_3[i] - time_info_2[i], time_info_4[i] - time_info_3[i], time_info_5[i] - time_info_4[i], time_info_6[i] - time_info_5[i], time_info_6[i] - time_info_1[i]);

			// sprintf(str, "%d	%lld	%lld	%lld	%lld	%lld	%lld	%lld	%lld	%lld	%lld	%lld	%lld\n",i,
			// time_info_1[i],time_info_2[i],time_info_3[i],time_info_4[i],time_info_5[i],time_info_6[i],
			// time_info_2[i]-time_info_1[i],time_info_3[i]-time_info_2[i],time_info_4[i]-time_info_3[i],time_info_5[i]-time_info_4[i],time_info_6[i]-time_info_5[i],time_info_6[i]-time_info_1[i]);
			fprintf(fp11, str);
		}
		fclose(fp11);
	}
}
#endif /* STATISTICS_LOG */

void do_calc(int num)
{
	int i, j;

	int NUM_CALC = NUM_PROC+3;

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

#if 0
static void *_sock_client(__attribute__((unused)) void *arg)
#else
int _sock_client()
#endif
{
	printf("MPI %d thread sock_client starts\n", ProcInfo.Id);
	gstatus = gstart;

	sleep_rt(5);

	int n;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		printf("ERROR opening socket\n");
	}
	//server = gethostbyname(argv[1]);
	char *hostname = "localhost";
	server = gethostbyname(hostname);
	if (server == NULL)
	{
		printf("ERROR, no such host\n");
		exit(0);
	}
	bzero((char *)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr,
				(char *)&serv_addr.sin_addr.s_addr,
				server->h_length);
	serv_addr.sin_port = htons(PORT);
	int cnect = -1;
	int xa = 0;

	int ntmp = 0;

	while (cnect < 0)
	{
		cnect = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
		if (cnect >= 0)
			break;

		printf("ERROR connecting\n");
		xa++;
		if (xa > 10)
		{
			printf("not able to connect socket server, have to exit!\n\n");

			exit(-1);
		}
		usleep_rt(100);
	}

	//parseMessage(sent_buf,sent_length);

	char *sbuff = "hello this is from client\n";

	int x = 0;
	while (gstatus != gstop)
	{
		//printf("MPI %d thread sock_client starts in testing sleep loop times:%d\n",ProcInfo.Id,x);

		n = write(sockfd, sbuff, 256);
		if (n > 0)
		{
			printf("MPI No.%d sent out message by socket, time:%d\n", ProcInfo.Id, x);
		}

		else
		{
			printf("MPI No.%d sent out message by socket Error!\n", ProcInfo.Id);
		}

		sleep_rt(5);
		x++;
	}

	printf("MPI %d thread sock_client exit!\n", ProcInfo.Id);
	close(sockfd);
	return;
}
#if 0
static void *_sock_server(__attribute__((unused)) void *arg)
#else
int _sock_server()
#endif
{
	printf("MPI %d thread sock_server starts\n", ProcInfo.Id);
	gstatus = gstart;

	int opt = TRUE;
	int master_socket, addrlen, new_socket, client_socket[30],
			max_clients = 30, activity, i, valread, sd;
	int max_sd;
	struct sockaddr_in address;

	char buffer[1025]; //data buffer of 1K

	//set of socket descriptors
	fd_set readfds;

	//a message
	char *message = "This message is from socket server \r\n";

	//initialise all client_socket[] to 0 so not checked
	for (i = 0; i < max_clients; i++)
	{
		client_socket[i] = 0;
	}

	//create a master socket
	if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
	{
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	//set master socket to allow multiple connections ,
	//this is just a good habit, it will work without this
	if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,
								 sizeof(opt)) < 0)
	{
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	//type of socket created
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	//bind the socket to localhost port 8888
	if (bind(master_socket, (struct sockaddr *)&address, sizeof(address)) < 0)
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	printf("Listener on port %d \n", PORT);

	//try to specify maximum of 3 pending connections for the master socket
	if (listen(master_socket, 3) < 0)
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}

	//accept the incoming connection
	addrlen = sizeof(address);
	puts("Waiting for connections ...");

	while (gstatus != gstop)
	{
		//clear the socket set
		FD_ZERO(&readfds);

		//add master socket to set
		FD_SET(master_socket, &readfds);
		max_sd = master_socket;

		//add child sockets to set
		for (i = 0; i < max_clients; i++)
		{
			//socket descriptor
			sd = client_socket[i];

			//if valid socket descriptor then add to read list
			if (sd > 0)
				FD_SET(sd, &readfds);

			//highest file descriptor number, need it for the select function
			if (sd > max_sd)
				max_sd = sd;
		}

		//wait for an activity on one of the sockets , timeout is NULL ,
		//so wait indefinitely
		activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

		if ((activity < 0) && (errno != EINTR))
		{
			printf("select error");
		}

		//If something happened on the master socket ,
		//then its an incoming connection
		if (FD_ISSET(master_socket, &readfds))
		{
			if ((new_socket = accept(master_socket,
															 (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
			{
				perror("accept");
				exit(EXIT_FAILURE);
			}

			//inform user of socket number - used in send and receive commands
			//printf("New connection , socket fd is %d , ip is : %s , port : %d \n" ,
			//		new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

			printf("New connection , port:%d\n", ntohs(address.sin_port));
			//send new connection greeting message
			if (send(new_socket, message, strlen(message), 0) != strlen(message))
			{
				perror("send");
			}

			puts("Welcome message sent successfully");

			//add new socket to array of sockets
			for (i = 0; i < max_clients; i++)
			{
				//if position is empty
				if (client_socket[i] == 0)
				{
					client_socket[i] = new_socket;
					printf("Adding to list of sockets as %d\n", i);

					break;
				}
			}
		}

		//else its some IO operation on some other socket
		for (i = 0; i < max_clients; i++)
		{
			sd = client_socket[i];

			if (FD_ISSET(sd, &readfds))
			{
				//Check if it was for closing , and also read the
				//incoming message
				if ((valread = read(sd, buffer, 1024)) == 0)
				{
					//Somebody disconnected , get his details and print
					getpeername(sd, (struct sockaddr *)&address,
											(socklen_t *)&addrlen);
					printf("Host disconnected , ip %s , port %d \n",
								 inet_ntoa(address.sin_addr), ntohs(address.sin_port));

					//Close the socket and mark as 0 in list for reuse
					close(sd);
					client_socket[i] = 0;
				}

				//Echo back the message that came in
				else
				{
					//set the string terminating NULL byte on the end
					//of the data read
					buffer[valread] = '\0';

					printf("MPI No.%d socket server: receive messages:%s\n", ProcInfo.Id, buffer);
					send(sd, buffer, strlen(buffer), 0);
				}
			}
		}
	}

	close(sockfd);

	return 0;
}

static unsigned int pthread_rx_event_core = 0;
pthread_t pid_thread;
short pid_thread_create(void *pt)
{
	int core;
	int size;
#ifdef _windriver
	core = allocate_rtcore();
#else
	core = NUM_PROC + 2;
#endif

	if (core < 0)
	{
		printf("MPI No.%d PHY pthread_create(rx_event): allocate_rtcore failed,core = %d\n", ProcInfo.Id, core);
		return -1;
	}
	else
		printf("MPI No.%d pthread_create(rx_event): allocate_rtcore,core = %d\n", ProcInfo.Id, core);

	pthread_rx_event_core = core;
	if (pt == NULL)
		pt = _sock_client;

	pthread_create(&pid_thread, NULL, pt, &pthread_rx_event_core);

	printf("pthread_create returns 0, and its cpu no. is %d\n", pthread_rx_event_core);

	/*
	core = allocate_rtcore();

	if (core < 0)
	{
		fprintf(fdbg,"PHY pthread_create(rx_special): allocate_rtcore failed,core = %d\n",core);
		return -1;
	}
	else
		fprintf(fdbg,"pthread_create(rx_special): allocate_rtcore,core = %d\n",core);


	pthread_gpsTimer_core = core;
		pthread_create(&pid_thread2, NULL, __rx_special, &pthread_gpsTimer_core);
	 */

	return 0;
}

void emt_comm()
{
	int i, j;
	MPI_Status MpiStt;
	double buf[EMT_COMM_NUM];

	memset(buf, 0, EMT_COMM_NUM * sizeof(double));

	if (PRINT_DETAIL)
		printf("emtcalId=%d\n", ProcInfo.EMTCALId);

	if (ProcInfo.CommEMTCAL == MPI_COMM_NULL)
	{
		//while(1)
		{
			//printf("!!!!  prodID=%d return from emt_comm------\n",ProcInfo.Id);
			//sleep(15);
		}
		return;
	}

	/*
	while(1)
	{
		printf("in sleep ..... \n");
		sleep(5);
	}
	*/

	if (ProcInfo.EMTCALId < 0)
		return;

	// ������������ż�����̷�MPI��Ϣ
	for (i = 0; i < ProcInfo.NumProcEMTCAL; i++)
	{
		if (i != ProcInfo.EMTCALId)
		{
			MPI_Send(SendBuf, EMT_COMM_NUM, MPI_DOUBLE_PRECISION, i, i + 1, ProcInfo.CommEMTCAL);
		}
	}

	// ������������ż�����̽���MPI��Ϣ
	for (i = 0; i < ProcInfo.NumProcEMTCAL; i++)
	{
		if (i != ProcInfo.EMTCALId)
		{
			MPI_Recv(buf, EMT_COMM_NUM, MPI_DOUBLE_PRECISION, i, ProcInfo.EMTCALId + 1, ProcInfo.CommEMTCAL, &MpiStt);
		}

		//printf("ProcInfo.EMTCALID:%d,  buf:%f\n",ProcInfo.EMTCALId,buf);

		// �ۼ����е�ż�����̷��ؽ��
		for (j = 0; j < EMT_COMM_NUM; j++)
		{
			//printf("ProcInfo.EMTCALID:%d,  buf[%d]:%f\n",ProcInfo.EMTCALId,j,buf[j]);
			RecvBuf[j] += buf[j];
		}
	}
}

void emtphy_comm()
{

	int i, j;
	MPI_Status MpiStt;
	double buf[EMT_COMM_NUM];

	memset(buf, 0, EMT_COMM_NUM * sizeof(double));

	// ���������������ӿڽ��̷�MPI��Ϣ
	for (i = 0; i < ProcInfo.NumProcEMTPhy; i++)
	{
		MPI_Send(SendBuf, EMT_COMM_NUM, MPI_DOUBLE_PRECISION, ProcInfo.EMTPhyId[i], ProcInfo.EMTPhyId[i] + 1, MPI_COMM_WORLD);
	}

	// ���������������ӿڽ��̽���MPI��Ϣ
	for (i = 0; i < ProcInfo.NumProcEMTPhy; i++)
	{
		MPI_Recv(buf, EMT_COMM_NUM, MPI_DOUBLE_PRECISION, ProcInfo.EMTPhyId[i], ProcInfo.Id + 1, MPI_COMM_WORLD, &MpiStt);
	}

	// �ۼ����������ӿڽ��̷��ؽ��
	for (j = 0; j < EMT_COMM_NUM; j++)
		RecvBuf[j] += buf[j];
}

void emtio_comm()
{
	//printf("SHM_EMTIO is NOT defined \n!");

	/*
	MPI_Request MpiRqst;

	if (ProcInfo.NumProcEMTIO == 0) return;					// û��EMTIO���̣��˳�

	// ��IO���̷���MPI��Ϣ
	if(ProcInfo.Id != ProcInfo.EMTIOId)
		MPI_Isend(SendBuf, EMT_COMM_NUM, MPI_DOUBLE_PRECISION, ProcInfo.EMTIOId, ProcInfo.EMTIOId+1, MPI_COMM_WORLD, &MpiRqst);

	else
	{
		MPI_Irecv(RecvBuf,EMT_COMM_NUM, MPI_DOUBLE_PRECISION,)
		MPI_Recv(buf, EMT_COMM_NUM, MPI_DOUBLE_PRECISION, i, ProcInfo.EMTCALId+1, ProcInfo.CommEMTCAL, &MpiStt);
	}
	*/
	const int sendsize = 50;
	const int recsize = 50 * NUM_PROC * 2;
	double sendbuf[sendsize], recvbuf[recsize];

	if (ProcInfo.Id != ProcInfo.EMTIOId)
	{
		int j = 0;
		for (; j < sendsize; j++)
			sendbuf[j] = j * 11;
	}

	MPI_Gather(sendbuf, sendsize, MPI_DOUBLE_PRECISION, recvbuf, sendsize, MPI_DOUBLE_PRECISION, ProcInfo.EMTIOId, MPI_COMM_WORLD);

	/*
	if(ProcInfo.Id == ProcInfo.EMTIOId)
	{
		int j=0;
		for(;j<sendsize*ProcInfo.NumProcEMTCAL;j++)
			printf(" rec data[%d]=%f\n",j,recvbuf[j]);
		printf("---------------- \n");

	}
	*/
}
#if 0
void emtio_comm_shem(int step)
{
	//printf("SHM_EMTIO is defined \n!");

	//printf("emtio_comm share mem start\n!");

	int ind;
	struct shm_blk *pblk;

	if (ProcInfo.NumProcEMTIO == 0)
	{

		printf("ProcInfo.NumProcEMTIO=%d\n", ProcInfo.NumProcEMTIO);
		return; // û��EMTIO���̣��˳�
	}

	// ��IO���̷���MPI��Ϣ
	if (shm_ctrl_per_task.mem == NULL)
	{
		printf("shm_ctrl_per_task.mem = NULL\n");
		return;
	}

	//ind  = shm_ctrl_per_task.prod_ind;

	//pblk = (struct shm_blk *)((unsigned char*)shm_ctrl_per_task.mem + shm_ctrl_per_task.blk_size * ind);

	/*
	if (pblk->sem != SHM_READ_COMPLETE)
	{
		printf("write to emtio, overflow\n");
	}
	*/

	//printf("emt, data=%lf, sem=0x%08x", pblk->data[0], pblk->sem);

	if (ProcInfo.EMTCALId > -1)
	{
		if (PRINT_DETAIL)
			printf("EMTCALID>0   MPINo: %d    EMTCALId=%d\n", ProcInfo.Id, ProcInfo.EMTCALId);

		ind = ProcInfo.EMTCALId;
		//ind=0;

		pblk = (struct shm_blk *)((unsigned char *)shm_ctrl_per_task.mem + shm_ctrl_per_task.blk_size * ind);

		if (PRINT_DETAIL)
			printf("step:%d  emtio_comm---- data=%lf, sem=0x%08x\n", step, pblk->data[0], pblk->sem);

		/*
		while(pblk->sem != SHM_READ_COMPLETE)
		{
			printf("Proc %d  in sleep for wait SHM_READ_COMPLETE \n",ProcInfo.Id);
			sleep(1);
		}
		*/

		//if(pblk->sem == SHM_READ_COMPLETE)
		double sdata[EMT_COMM_NUM];
		if (1)
		{

			//memcpy(sdata, pblk->data, EMT_COMM_NUM * sizeof(double));

			int j = 0;
			for (; j < EMT_COMM_NUM; j++)
			{
				if (PRINT_DETAIL)
					printf("write data [%d]=%lf   pblk-data[%d]=%lf    \n", j, sdata[j], j, pblk->data[j]);
				sdata[j] = step * ind;
			}
			memcpy(pblk->data, sdata, EMT_COMM_NUM * sizeof(double));

			/*

			//while(pblk->sem != SHM_WRITE_COMPLETE );

			double sdata[EMT_COMM_NUM];
			int j=0;
			for(;j<EMT_COMM_NUM;j++)
			{
				sdata[j]=j;
			}
			memcpy(pblk->data, SendBuf, EMT_COMM_NUM * sizeof(double));
			pblk->sem = SHM_WRITE_COMPLETE;
			printf("EMTCALID>0   MPINo: %d  SHM_WRITE_COMPLETE done   \n",ProcInfo.Id);
			printf("pblk�� %x SHM_WRITE_COMPLETE ",(char*)pblk);
			*/
			pblk->sem = SHM_READ_COMPLETE;
		}
		else
		{
			printf("not SHM_WRITE_COMPLETE\n");
		}
	}
	else if (ProcInfo.EMTIOId > -1)
	{
		if (PRINT_DETAIL)
			printf("step:%d  EMTIOID>0   MPINo: %d\n", step, ProcInfo.Id);

		//ind=0;

		//pblk = (struct shm_blk *)((unsigned char*)shm_ctrl_per_task.mem + shm_ctrl_per_task.blk_size * ind);

		double sdata[EMT_COMM_NUM];
		/*
		int j=0;
		for(;j<EMT_COMM_NUM;j++)
		{
			sdata[j]=step;
			printf("write data[%d]=%lf  ",j,sdata[j]);

		}
		printf("\n");
		*/
		//memcpy(pblk->data, sdata, EMT_COMM_NUM * sizeof(double));
		//memcpy(sdata,pblk->data,EMT_COMM_NUM * sizeof(double));

		for (ind = 0; ind < ProcInfo.NumProcEMTCAL; ind++)
		{
			pblk = (struct shm_blk *)((unsigned char *)shm_ctrl_per_task.mem + shm_ctrl_per_task.blk_size * ind);
			memcpy(sdata, pblk->data, EMT_COMM_NUM * sizeof(double));

			if (PRINT_DETAIL)
			{
				printf("step:%d after memcpy ind:%d:  ", step, ind);
				int j;
				for (j = 0; j < EMT_COMM_NUM; j++)
				{
					printf("data[%d]=%lf  ", j, sdata[j]);
				}
				printf("\n");
			}
		}
		pblk->sem = SHM_WRITE_COMPLETE;

		/*
		sleep(2);

		int j=0;
		for(;j<ProcInfo.NumProcEMTCAL;j++)
		{
			ind=j;
			pblk = (struct shm_blk *)((unsigned char*)shm_ctrl_per_task.mem + shm_ctrl_per_task.blk_size * ind);

			/*
			while(pblk->sem != SHM_WRITE_COMPLETE)
			{
				printf("Proc %d   ind=%d  pblk:%x  in sleep for wait SHM_WRITE_COMPLETE \n",ProcInfo.Id,ind,(char*) pblk);
				sleep(1);
			}
			*/
		//if(pblk->sem == SHM_WRITE_COMPLETE )
		/*
			{
				printf("step:%d emtio_comm---- data=%lf, sem=0x%08x\n", step,pblk->data[0], pblk->sem);

				memcpy(RecvBuf,pblk->data,EMT_COMM_NUM * sizeof(double));

				int j=0;
				for(;j<EMT_COMM_NUM;j++)
					printf("read data [%d]=%lf\n",j,RecvBuf[j]);

				printf("EMTIOID>0   MPINo: %d  SHM_READ_COMPLETE done   \n",ProcInfo.Id);
				pblk->sem = SHM_READ_COMPLETE;
				printf("step: %d emtio_comm---- data=%lf, sem=0x%08x\n", step, pblk->data[0], pblk->sem);
			}

		}
		*/
	}

	//memcpy(pblk->data, "feedback for the shm...", EMT_COMM_NUM * sizeof(double));
	//pblk->sem = SHM_WRITE_COMPLETE;

	/*
	shm_ctrl_per_task.prod_ind++;
	if (shm_ctrl_per_task.prod_ind == shm_ctrl_per_task.blk_num)
	{
		shm_ctrl_per_task.prod_ind = 0;
	}
*/
	if (PRINT_DETAIL)
		printf("emtio_comm share mem done\n!");
}
#endif

int SendCheck(double* buf, double* validbuf, int length)//mode =  1: Recv mode, mode = 0: Send mode
{
	int i;
	for (i = 0; i<length; i++)
	{
		//检测是不是正确
		if ((buf[i] - validbuf[i])>1.0e-6)
		{
			return i;
		}
	}
	return -1;
}

void emt_comm_both_sides_shm(int step, int *DIVI, int *DIVJ, int *sendshmid, void **sendshm, int *sendsemid, int *recvshmid, void **recvshm, int *recvsemid)
{
	int i, j;
	MPI_Status MpiStt;
	MPI_Request request[(NUM_DIV_PER + 1) * 2];
	MPI_Status mpistatus[(NUM_DIV_PER + 1) * 2];
	double buf[EMT_COMM_NUM];
	memset(buf, 0, EMT_COMM_NUM * sizeof(double));
	int iNtmp_recv;
	int iNtmp_send;
	int mode;
//
#if 0
	if (step == 1)
	{
		//���ݷ���
		for (i = 0; i < NUM_DIV; i++)
		{
			if ((DIVI[i] == ProcInfo.EMTCALId) || (DIVJ[i] == ProcInfo.EMTCALId))
			{
				mode = 0;
				if (DIVI[i] == ProcInfo.EMTCALId)
				{
					iNtmp_send = i;
					iNtmp_recv = i + Reverse_ID;
				}
				else if (DIVJ[i] == ProcInfo.EMTCALId)
				{
					iNtmp_send = i + Reverse_ID;
					iNtmp_recv = i;
				}
				else
				{
				}
				shm_create(iNtmp_send, &sendshmid[i], mode, &sendshm[i]);
				shm_create(iNtmp_recv, &recvshmid[i], mode, &recvshm[i]);
				sem_create(iNtmp_send, &sendsemid[i]);
				sem_create(iNtmp_recv, &recvsemid[i]);
			}
		}
	}
#endif
	
	//���ݷ���
	int idx_buff = 0;
	int idx_request = 0;
	for (i = 0; i < NUM_DIV; i++)
	{
		if ((DIVI[i] == ProcInfo.EMTCALId) || (DIVJ[i] == ProcInfo.EMTCALId))
		{
//load the data in the Isendbuff
			for (int j = 0; j<EMT_COMM_NUM;j++)
			{
				ISendBuf[idx_buff+j] = (i*100+j)*step;
			}

			if (DIVI[i] == ProcInfo.EMTCALId)
			{
#ifdef _MPISENDRECV
#ifdef _MPIISENDRECV
				MPI_Isend(ISendBuf[idx_buff], EMT_COMM_NUM, MPI_DOUBLE_PRECISION, DIVJ[i], 33+i, ProcInfo.CommEMTCAL, &request[idx_request]);
#else
				MPI_Send(SendBuf, EMT_COMM_NUM, MPI_DOUBLE_PRECISION, DIVJ[i], 33, ProcInfo.CommEMTCAL);
#endif
#else
			int idx_add = MPI_EMT_Send(&ISendBuf[idx_buff],&IRecvBuf[idx_buff],EMT_COMM_NUM,MPI_DOUBLE_PRECISION,DIVJ[i],33,ProcInfo.CommEMTCAL,&request[idx_request]);
			idx_request += idx_add;
#endif
			}
			else if (DIVJ[i] == ProcInfo.EMTCALId)
			{
#ifdef _MPISENDRECV
#ifdef _MPIISENDRECV
				MPI_Isend(ISendBuf[idx_buff], EMT_COMM_NUM, MPI_DOUBLE_PRECISION, DIVI[i], 33+i, ProcInfo.CommEMTCAL, &request[idx_request]);
#else
				MPI_Send(SendBuf, EMT_COMM_NUM, MPI_DOUBLE_PRECISION, DIVI[i], 33, ProcInfo.CommEMTCAL);
#endif
#else
			int idx_add = MPI_EMT_Send(&ISendBuf[idx_buff],&IRecvBuf[idx_buff],EMT_COMM_NUM,MPI_DOUBLE_PRECISION,DIVI[i],33,ProcInfo.CommEMTCAL,&request[idx_request]);
			idx_request += idx_add;
#endif
			}
			idx_buff+=EMT_COMM_NUM;
			//if (idx_buff>=8) printf("send idx_buff is too much\n");
			//idx_request++;
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
				MPI_Irecv(IRecvBuf[idx_buff], EMT_COMM_NUM, MPI_DOUBLE_PRECISION, DIVJ[i], 33+i, ProcInfo.CommEMTCAL, &request[idx_request]);
#else
				MPI_Recv(RecvBuf, EMT_COMM_NUM, MPI_DOUBLE_PRECISION, DIVJ[i], 33, ProcInfo.CommEMTCAL, &MpiStt);
#endif
#else
				//shm_recv(RecvBuf, EMT_COMM_NUM, 0, recvshm[i], recvsemid[i]); //MuQ 181215
			MPI_EMT_Recv(&IRecvBuf[idx_buff],EMT_COMM_NUM, MPI_DOUBLE_PRECISION, DIVJ[i],33,ProcInfo.CommEMTCAL, &Msgpos[i],step); //MuQ 181215		
#endif
			}
			else if (DIVJ[i] == ProcInfo.EMTCALId)
			{
#ifdef _MPISENDRECV
#ifdef _MPIISENDRECV
				MPI_Irecv(IRecvBuf[idx_buff], EMT_COMM_NUM, MPI_DOUBLE_PRECISION, DIVI[i], 33+i, ProcInfo.CommEMTCAL, &request[idx_request]);
#else
				MPI_Recv(RecvBuf, EMT_COMM_NUM, MPI_DOUBLE_PRECISION, DIVI[i], 33, ProcInfo.CommEMTCAL, &MpiStt);
#endif
#else
			MPI_EMT_Recv(&IRecvBuf[idx_buff],EMT_COMM_NUM, MPI_DOUBLE_PRECISION, DIVI[i],33,ProcInfo.CommEMTCAL, &Msgpos[i],step); //MuQ 181215	
#endif
			}
			idx_buff+=EMT_COMM_NUM;
			//if (idx_buff>=8) printf("idx_buff is too much\n");
			//idx_request++;
		}
	}
	if ((ProcInfo.EMTCALId == 0)&&(step == 1))
	{
		printf("start to wait for MPI finished, with idx_request = %d.\n",idx_request);
	}
	if (idx_request > 0) MPI_Waitall(idx_request, &request[0], &mpistatus[0]);
	
	if ((ProcInfo.EMTCALId == 0)&&(step == 1))
	{
		printf("MPI finished, with idx_request = %d.\n",idx_request);
	}
	int Ierror = SendCheck(ISendBuf,IRecvBuf,idx_buff);
	if (Ierror != -1)
	{
		//数据传输错误
		printf(" Data Transfers with Error in Step %d in Ksub %d, the %dth data is error.\n", step,iKsub, Ierror);
		printf(" ISendBuff Data is: \n");
		for (int m = 0; m<idx_buff;m++)
		{
			printf("%d ",ISendBuf[m]);
		}
		printf("\n IRecvBuff Data is: \n");
		for (int m = 0; m<idx_buff;m++)
		{
			printf("%d ",IRecvBuf[m]);
		}	
	}

}

void init_gvar(int cpuid)
{
	//gbuff1 = malloc(SIZE_GVar_EMT);
	gbuff1 = alloc_one_2m_page(cpuid);
	if (gbuff1 == NULL)
	{
		printf("allocate mem for gbuff1 failed,and exit!\n");
		exit(-1);
	}
	printf("init_gval gbuff1=%lld\n", gbuff1);
	//gbuff2 = malloc(SIZE_GVar_EMT);
	gbuff2 = alloc_one_2m_page(cpuid);
	if (gbuff2 == NULL)
	{
		printf("allocate mem for gbuff2 failed,and exit!\n");
		exit(-1);
	}
	printf("init_gval gbuff2=%lld\n", gbuff2);
	//gbuff3 = malloc(SIZE_GVar_EMT);
	gbuff3 = alloc_one_2m_page(cpuid);
	if (gbuff3 == NULL)
	{
		printf("allocate mem for gbuff3 failed,and exit!\n");
		exit(-1);
	}
	printf("init_gval gbuff3=%lld\n", gbuff3);
	//gbuff4 = malloc(SIZE_GVar_EMT);
	gbuff4 = alloc_one_2m_page(cpuid);
	if (gbuff4 == NULL)
	{
		printf("allocate mem for gbuff4 failed,and exit!\n");
		exit(-1);
	}
	printf("init_gval gbuff4=%lld\n", gbuff4);
	//gbuff5 = malloc(SIZE_GVar_EMT);
	gbuff5 = alloc_one_2m_page(cpuid);
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
		//free(gbuff1);
		free_one_2m_page(gbuff1);
		gbuff1 = NULL;
	}
	printf("free  mem for gbuff1 vars successfully!\n");
	if (gbuff2 != NULL)
	{
		//free(gbuff2);
		free_one_2m_page(gbuff2);
		gbuff2 = NULL;
	}
	printf("free  mem for gbuff2 vars successfully!\n");
	if (gbuff3 != NULL)
	{
		//free(gbuff3);
		free_one_2m_page(gbuff3);
		gbuff3 = NULL;
	}
	printf("free  mem for gbuff3 vars successfully!\n");
	if (gbuff4 != NULL)
	{
		//free(gbuff4);
		free_one_2m_page(gbuff4);
		gbuff4 = NULL;
	}
	printf("free  mem for gbuff4 vars successfully!\n");
	if (gbuff5 != NULL)
	{
		//free(gbuff5);
		free_one_2m_page(gbuff5);
		gbuff5 = NULL;
	}
	printf("free  mem for gbuff5 vars successfully!\n");
	printf("free  mem for all global vars successfully!\n");
}

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
	for (i = 0; i < max-100; i = i + 100)
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

int update_buf()
{
	int i;

	for (i = 0; i < EMT_COMM_NUM; i++)
	{
		RecvBuf[i] = 0.0;

		//DataBuf[(step-1)*EMT_COMM_NUM+i] = SendBuf[i];
	}

	return 0;
}

int finalize_buf()
{
	int i, j;
	FILE *fp;
	char fname[128];

#ifndef RUN_LONG_TIME_TEST
	sprintf(fname, "emt_%02d.txt", ProcInfo.EMTCALId);
	if ((fp = fopen(fname, "w")) != NULL)
	{
		for (i = 0; i < nstep; i++)
		{
			for (j = 0; j < EMT_COMM_NUM; j++)
				fprintf(fp, "%14.6e, ", DataBuf[i * EMT_COMM_NUM + j]);
			fprintf(fp, "\n");
		}
		fclose(fp);
	}
#endif

	//delete[] DataBuf;
	free(DataBuf);

	return 0;
}

void DIVFileProduce(int* DIVI, int* DIVJ)
{
	FILE* fp = fopen("testmpionwr.DIV","w+");
	for (int i = 0; i < NUM_DIV; i++)
	{
		fprintf(fp, "1,%6d,%d,%6d,%6d,%6d,%6d,4,%6d,\n",i + 1,1,DIVI[i]+1,50,DIVJ[i]+1,50,EMT_COMM_NUM/6);
	}
	fclose(fp);
}

void EMT_DIV_GET(int *DIVI, int *DIVJ)
{
	int i;
	int startprocess =  (int)floor(1.0*(NUM_PROC-1-NUM_DIV_PER)/2);
	//int NUM_DIV = NUM_PROC-1-NUM_DIV_PER)/2*NUM_DIV_PER;

	for (i = 0; i < NUM_DIV; i++)
	{
		DIVI[i] = (int)floor((i / NUM_DIV_PER) + 1.0e-12); //i 0  j 200 190 180 170 160 150 140 130
		//i 1  j 199 189 179 169 159 149 139 129
		DIVJ[i] = startprocess+DIVI[i]+(i % NUM_DIV_PER);
		if (ProcInfo.EMTCALId == 0)	printf ("DIVI[%d] = %d,DIVJ[%d]=%d\n",i,DIVI[i],i,DIVJ[i]);
		//if (DIVJ[i] < 0 ) DIVJ[i] = ProcInfo.NumProcCAL - DIVI[i] - (i % NUM_DIV_PER) - NUM_DIV_PER;
	}
	DIVFileProduce(DIVI,DIVJ);
}



#ifdef _LINUX

int warm = WARM_COUNT;
//void *thread_emt(__attribute__((unused)) void *arg)
void thread_emt()
{

	double t1, t2, t3, t4, t5, t6, max_val = 0, val, cal, sum_cal = 0, avg_cal = 0, sum_val = 0, avg_val = 0,comm, sum_comm = 0, avg_comm = 0;
	unsigned long long step_all_max = 0, step_all_min = 0, step_calc_max = 0, step_calc_min = 0;
	unsigned long long step_comm_max = 0, step_comm_min = 0;
	char filename[40];
	int aa = 0;
	double *total_time = malloc((nstep - warm) * sizeof(double));

	memset(total_time, 0, ((nstep - warm) * sizeof(double)));
	//unsigned int core_id = *(unsigned int*)arg;
	//set_thread_affinity(core_id);
	FILE *fp;
	int n_overcal = 0;
	int n_overval = 0;
	int n_overcomm = 0;
	unsigned long pid = getpid();
	//sprintf(filename, "total_comm_%d.txt", ProcInfo.Id);
	//fp = fopen(filename, "wb");
#ifdef RUN_LONG_TIME_TEST
	//time_status_init();
#endif
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
	yj_performance_start();
#endif

	for (step = 1; step <= nstep; step++)
	{
		// if (ProcInfo.EMTCALId == 0)
		// {
		// 	if (step % 100 == 0)
		// 		printf("step %d.\n", step);
		// }
		cplusplus_rtclock_gettime(&t1);
		//		tn = t1 + tstep;
		//emt_comm();							// emt֮��MPIͨ�źͼ���
		//emt_comm_both_sides();
		//emt_comm_both_sides2();
		//		cplusplus_rtclock_gettime(&t2);
		//		emtphy_comm();						// emt��emtphy֮��MPIͨ��
		emt_comm_both_sides_shm(step, DIVI, DIVJ, sendshmid, sendshm, sendsemid, recvshmid, recvshm, recvsemid);
		cplusplus_rtclock_gettime(&t3);
		do_calc(iKsub);  //根据子网的子网号递减iKsubiKsub
//		do_calc(1024);  //根据子网的子网号递减iKsubiKsub
		update_gvar(0);
		cplusplus_rtclock_gettime(&t4);
		//emtio_comm();						// emt��emtio֮��MPIͨ��
		//emtio_comm_shem(step);
		cplusplus_rtclock_gettime(&t5);
		//update_buf();

		cplusplus_rtclock_gettime(&t6);

#ifdef STATISTICS_LOG
		time_info_1[step - 1] = t1;
		time_info_2[step - 1] = t2;
		time_info_3[step - 1] = t3;
		time_info_4[step - 1] = t4;
		time_info_5[step - 1] = t5;
		time_info_6[step - 1] = t6;
		time_info_7[step - 1] = step;
		time_info_7[step - 1] = pid;
#endif

		if (step > warm)
		{
			val = t6 - t1; //emt comm
			if (val > tmax_val)
			{
				tmax_val = val;
				step_all_max = step;
			}
			if (val > 1.3 * sum_val / (step - warm))
			{
				n_overval++;
			}
			if (val < tmin_val)
			{
				tmin_val = val;
				step_all_min = step;
			}
			sum_val += val;

			cal = t4 - t3; //cal value
			if (cal > tmax_cal)
			{
				tmax_cal = cal;
				step_calc_max = step;
			}
			if (cal > 1.3 * sum_cal / (step - warm))
			{
				n_overcal++;
			}
			if (cal < tmin_cal)
			{
				tmin_cal = cal;
				step_calc_min = step;
			}
			sum_cal += cal;

			comm = t3 - t1; //comm value
			if (comm > tmax_comm)
			{
				tmax_comm = comm;
				step_comm_max = step;
			}
			if (comm > 1.3 * sum_comm / (step - warm))
			{
				n_overcomm++;
			}
			if (comm < tmin_comm)
			{
				tmin_comm = comm;
				step_comm_min = step;
			}
			sum_comm += comm;

			//total_time[aa] = val;
			//aa++;
		}
		//MPI_Barrier(ProcInfo.CommCAL);			// ͬ�����MPI���̵�thread
		if (PRINT_DETAIL)
			printf("core=%d,step:%d,t3-t1:%f,t4-t3:%f,t5-t4:%f,t6-t4:%f,t6-t1:%f\n",
						 pid, step, t3 - t1, t4 - t3, t5 - t4, t6 - t4, t6 - t1);
	}
#ifdef _yuanji
	yj_performance_stop();
#endif


	MPI_Win_unLock();

	cplusplus_rtclock_gettime(&tc);

/* 	for (aa = 0; aa < nstep - warm; aa++)
	{
		fprintf(fp, "%lld\n", total_time[aa]);
	} */

	avg_val = sum_val / (nstep - warm);
	avg_cal = sum_cal / (nstep - warm);
	avg_comm = sum_comm / (nstep - warm);
	printf("core=%d,Id:%d CALC: Mean %f Max %f (%lld) Min %f (%lld) Overtime(%d) | Comm: Mean %f Max %f (%lld) Min %f (%lld) Overtime(%d) | TOT: Mean %f Max %f (%lld) Min %f (%lld) Overtime(%d) \n",
				 pid, ProcInfo.Id, avg_cal, tmax_cal, step_calc_max, tmin_cal, step_calc_min, n_overcal, avg_comm, tmax_comm, step_comm_max, tmin_comm, step_comm_min, n_overcomm,
				 avg_val, tmax_val, step_all_max, tmin_val, step_all_min, n_overval);

	// fclose(fp);
	stopprogram = 1;
	cplusplus_rtclock_sleep(1000);
	return;
}

#else //_LINUX

void thread_emt()
{

	//MPI_Barrier(ProcInfo.CommEMTCAL);			// ͬ�����MPI���̵�thread
	MPI_Barrier(MPI_COMM_WORLD);
	// �������ڿ���
	cplusplus_rtclock_gettime(&ts);
	for (step = 1; step <= nstep; step++)
	{

		printf("%step:    %d\n", step);

		emt_comm(); // emt֮��MPIͨ�źͼ���
								//emtphy_comm();						// emt��emtphy֮��MPIͨ��
								//do_calc(CalcNum);
								//update_gvar(-1);
								//emtio_comm();						// emt��emtio֮��MPIͨ��
								//update_buf();
	}
	cplusplus_rtclock_gettime(&tc);

	if (0 == ProcInfo.Id)
	{
		printf("Time step: %f\n", tstep);
		printf("Total steps: %d\n", nstep);
		printf("Elapsed time: %f\n", tc - ts);
	}
	isCalculationOver = 1;
}

#endif //_LINUX

#ifdef SHM_EMTIO
void init_shm(int no)
{

	no = 0;
	printf("init shm start---\n");
	int i;
	int shm_id;
	key_t key;
	char pathname[30];
	struct shm_blk *pblk;
	FILE *fp_shm;

	shm_ctrl_per_task.shm_size = SHM_SIZE_PER_TASK;
	shm_ctrl_per_task.mem = NULL;
	shm_ctrl_per_task.blk_num = SHM_BLOCK_NUM;
	shm_ctrl_per_task.blk_size = SHM_BLOCK_SIZE;
	shm_ctrl_per_task.prod_ind = 0;
	shm_ctrl_per_task.cons_ind = 0;

	sprintf(pathname, "emtio_shm_%d", no);

	fp_shm = fopen(pathname, "w");
	if (fp_shm == NULL)
	{
		printf("Can not open %s\n", pathname);
		return;
	}
	else
	{
		fclose(fp_shm);
	}

	key = ftok(pathname, 0x03);
	if (key == -1)
	{
		printf("ftok failed in emt, no=%d, errno=%d\n", no, errno);
		return;
	}

	shm_id = shmget(key, shm_ctrl_per_task.shm_size, 0666 | IPC_CREAT);
	if (shm_id == -1)
	{
		printf("shmget failed in emt, no=%d, errno=%d\n", no, errno);
	}
	else
	{
		shm_ctrl_per_task.mem = shmat(shm_id, NULL, 0);
		if (shm_ctrl_per_task.mem == NULL)
		{
			printf("shmat failed in emt, no=%d, errno=%d\n", no, errno);
		}
		else
		{

			for (i = 0; i < shm_ctrl_per_task.blk_num; i++)
			{
				pblk = (struct shm_blk *)((unsigned char *)shm_ctrl_per_task.mem + shm_ctrl_per_task.blk_size * i);

				//printf("set pblk->sem SHM_READ_COMPLETE\n");
				pblk->sem = SHM_READ_COMPLETE;
				//memset(pblk->data, 0, sizeof(double) * EMT_COMM_NUM);

				double x[EMT_COMM_NUM];
				int j = 0;
				for (j = 0; j < EMT_COMM_NUM; j++)
					x[j] = -j;
				memcpy(pblk->data, x, sizeof(double) * EMT_COMM_NUM);

				if (PRINT_DETAIL)
				{
					for (j = 0; j < EMT_COMM_NUM; j++)
						printf("pblk[%d]=%lf  ", j, pblk->data[j]);
				}
			}
		}
	}

	printf("init shm done---\n");

	return;
}

void deinit_shm(int no)
{

	no = 0;
	printf("MPI No.%d deinit_shm start\n", ProcInfo.Id);
	int shm_id;
	key_t key;
	char pathname[30];
	int ret;

	sprintf(pathname, "emtio_shm_%d", no);
	key = ftok(pathname, 0x03);
	if (key == -1)
	{
		printf("shmdt failed in emt1, no=%d, errno=%d\n", no, errno);
		return;
	}

	shm_id = shmget(key, 0, 0);
	if (shm_id == -1)
	{
		//printf("shmget failed in emt1, no=%d, errno=%d\n", no, errno);
		return;
	}

	ret = shmdt(shm_ctrl_per_task.mem);
	if (ret)
	{
		printf("shmdt failed in emt1, no=%d, errno=%d\n", no, errno);
		return;
	}

	ret = shmctl(shm_id, IPC_RMID, 0);
	if (ret == -1)
	{
		printf("shmctl failed in emt1, no=%d, errno=%d\n", no, errno);
	}

	return;
}

#endif

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

	//printf("size of GroupCAL:%d\n",size_t2);

	/*
	while(1)
	{
		printf("in sleep----\n");
		sleep(10);
	}
	*/

	/*
	if(Ntype==SIMUTYPE_EMTIO)
		while(1)
			sleep(1);

	*/
	/*
	printf("bk 2-------------------:   procInfo.Id=%d,  Ntypes=%d\n",ProcInfo.Id,Ntype);
	if(Ntype != SIMUTYPE_EMTIO)
	{
		printf("I am not SIMUTYPE_EMTIO\n");
		if(ProcInfo.CommCAL == MPI_COMM_NULL)
			printf("ProcInfo.CommCAL == MPI_COMM_NULL");
	     MPI_Comm_rank( ProcInfo.CommCAL, &ProcInfo.CALId);
	}
	else
	{
		printf("I am SIMUTYPE_EMTIO\n");
	}
	*/

	//printf("bk 3-------------------:   procInfo.Id=%d,  Ntypes=%d\n",ProcInfo.Id,Ntype);

	// ������̬ͨ����
	ProcInfo.NumProcST = 0;
	printf("ProcInfo.NumProc=%d\n", ProcInfo.NumProc);
	for (i = 0; i < ProcInfo.NumProc; i++)
	{
		//printf("IDataRecv[%d]=%d\n", i, IDataRecv[i]);
		if (IDataRecv[i] <= 100) // ���л�����̬����
		{
			ProcIdx[ProcInfo.NumProcST] = i;
			ProcInfo.NumProcST++;
		}
	}
	printf("init_comm ProcInfo.NumProcST=%d\n", ProcInfo.NumProcST);

	if (ProcInfo.NumProcST > 0)
	{
		//�����Ѵ��ڵĽ�����GroupCAL�����µĽ�����GroupST
		MPI_Group_incl(GroupCAL, ProcInfo.NumProcST, ProcIdx, &GroupST); //�γɻ�����̬������
		MPI_Comm_create(ProcInfo.CommCAL, GroupST, &ProcInfo.CommST);		 //�γɻ�����̬ͨ����
	}
	else
	{
		ProcInfo.CommST = MPI_COMM_NULL;
	}

	//printf("bk 4-------------------:   procInfo.Id=%d,  Ntypes=%d\n",ProcInfo.Id,Ntype);

	// �����̬ͨ����
	ProcInfo.NumProcEMT = 0;

	//int emtidx[100];
	for (i = 0; i < ProcInfo.NumProc; i++)
	{
		if ((IDataRecv[i] > 100) && (IDataRecv[i] <= 200))
		{

			//printf("procInfo.Id=%d  cal EMT Proc:  ProcInfo.NumProcEMT=%d\n",ProcInfo.Id,ProcInfo.NumProcEMT);
			//ProcIdx[ProcInfo.NumProcEMT] = i;
			ProcIdx[ProcInfo.NumProcEMT] = i;
			//printf("procInfo.Id=%d cal EMT Proc:  ProcIdx[%d]=%d\n",ProcInfo.Id,ProcInfo.NumProcEMT,i);
			ProcInfo.NumProcEMT++;
			//printf("procInfo.Id=%d cal EMT Proc:  ProcInfo.NumProcEMT=%d\n",ProcInfo.Id,ProcInfo.NumProcEMT);
		}
	}
	//printf("init_comm ProcInfo.NumProcEMT=%d\n",ProcInfo.NumProcEMT);

	//printf("bk 5-------------------:   procInfo.Id=%d,  Ntypes=%d\n",ProcInfo.Id,Ntype);
	//ProcInfo.CommEMT=MPI_COMM_NULL;
	//if (ProcInfo.NumProcEMT>0 && Ntype!=SIMUTYPE_EMTIO )
	//ProcInfo.NumProcEMT--;
	if (ProcInfo.NumProcEMT > 0)
	{
		//???????????????GroupCAL?????��??????GroupEMT
		MPI_Group_incl(GroupCAL, ProcInfo.NumProcEMT, ProcIdx, &GroupEMT); //?��????????????
		MPI_Comm_create(ProcInfo.CommCAL, GroupEMT, &ProcInfo.CommEMT);		 //?��???????????
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
		MPI_Comm_create(ProcInfo.CommEMT, GroupEMTCAL, &ProcInfo.CommEMTCAL);		 //?��???????????????
#ifdef _MPI_SHM_PROXY
		//start to increase the info of the CommEMTCal
		int iSubId = iKsub - 1;
		MPI_Allgather( &iSubId, 1, MPI_INTEGER, IDataRecv, 1, MPI_INTEGER, ProcInfo.CommEMTCAL);
		MPI_Group GroupEMTCAL2 = GroupEMTCAL;
		MPI_Group_incl(GroupEMTCAL2, ProcInfo.NumProcEMTCAL, IDataRecv, &GroupEMTCAL);		//形成电磁暂态子网进程
		MPI_Comm CommEMTCAL = ProcInfo.CommEMTCAL;
		MPI_Comm_create(CommEMTCAL, GroupEMTCAL, &ProcInfo.CommEMTCAL);	//形成电磁暂态子网通信体
		MPI_Group_free(&GroupEMTCAL2);
		MPI_EMTCAL_RANK(ProcInfo.CommEMTCAL,ProcInfo.NumProcEMTCAL,IDataRecv);

#endif		
	}
	else
	{
		ProcInfo.CommEMTCAL = MPI_COMM_NULL;
	}
	MPI_Comm_rank(ProcInfo.CommEMTCAL, &ProcInfo.EMTCALId);		

#ifdef SHM_EMTIO
	/* init shm used by emt to send data to emtio */
	printf("init_comm ProcInfo.NumProcEMTIO=%d\n", ProcInfo.NumProcEMTIO);
	if (ProcInfo.NumProcEMTIO > 0)
	{
		init_shm(ProcInfo.EMTCALId);
	}
	// ͳ�����������ӿڽ��̺�
	printf("init_comm ProcInfo.NumProcEMTPhy=%d\n", ProcInfo.NumProcEMTPhy);
	if (ProcInfo.NumProcEMTPhy > 0)
	{
		//ProcInfo.EMTPhyId = new int[ProcInfo.NumProcEMTPhy];
		ProcInfo.EMTPhyId = malloc(ProcInfo.NumProcEMTPhy * sizeof(int));
		j = 0;
		for (i = 0; i < ProcInfo.NumProc; i++)
		{
			if (SIMUTYPE_EMTPHY == IDataRecv[i])
			{
				ProcInfo.EMTPhyId[j] = i;
				j++;
			}
		}
	}
#endif

	//????????
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

void Socket_Comm()
{
	int i, j, Ntype;
	if (RUN_EMTIO == 1 && ProcInfo.Id == NUM_PROC)
	{
		Ntype = SIMUTYPE_EMTIO; //���������ͺ�
	}
	else if (RUN_EMTIO == 1 && ProcInfo.Id == NUM_PROC + 1)
	{
		Ntype = SIMUTYPE_GUI;

#if 0
		int nret = pid_thread_create(_sock_server);
#else
		int nret = _sock_server();
#endif

		if (nret < 0)
			return;

		/*
		// create sock
		int  n;
		int portno=8001;
		//struct sockaddr_in serv_addr;
		//struct hostent *server;

		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd < 0)
		{
			printf("ERROR opening socket\n");
		}
		//server = gethostbyname(argv[1]);
		char* hostname="localhost";
		server = gethostbyname(hostname);
		if (server == NULL) {
			printf("ERROR, no such host\n");
			exit(0);
		}
		bzero((char *) &serv_addr, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		bcopy((char *)server->h_addr,
				(char *)&serv_addr.sin_addr.s_addr,
				server->h_length);
		serv_addr.sin_port = htons(portno);

		printf("MPI no.%d  GUI socket after create socket connection!\n",ProcInfo.Id);
		//int cnect=-1;
		//int xa=0;

		/*

		if(0)
		{
			int ntmp=0;
			socklen_t alen=sizeof(int);
			getsockopt(sockfd,SOL_SOCKET,SO_SNDBUF,(char*)&ntmp,&alen);
			printf("SEND buff size��%d\n",ntmp);

			getsockopt(sockfd,SOL_SOCKET,SO_SNDLOWAT,(char*)&ntmp,&alen);
			printf("SEND LOWAT size��    %d\n",ntmp);

			//ntmp=1048*10;
			if(setsockopt(sockfd,SOL_SOCKET,SO_SNDBUF,(char*)&ntmp,sizeof(int))<0)
			{
				printf("chagne send BUFF size failed!\n");
			}

			getsockopt(sockfd,SOL_SOCKET,SO_SNDBUF,(char*)&ntmp,&alen);
			printf("now SEND buff size:    %d\n",ntmp);

		}
		while(cnect<0)
		{
			cnect=connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr));
			if(cnect >=0 )
				break;

			fprintf(fdbg, "ERROR connecting\n");
			fflush(fdbg);
			xa++;
			if(xa>10)
			{
				fprintf(fdbg, "not able to connect socket server, have to exit!\n\n");
				fflush(fdbg);
				exit(-1);
			}
			usleep(100);


		}
		*/
	}
	else
	{
		Ntype = SIMUTYPE_EMTSUB;
#if 0
		int nret = pid_thread_create(_sock_client);

			if(nret<0) return -1;
#else
		/* code */
		int nret = _sock_client();
#endif
	}
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

void *rt_thread(void *func)
{
	void (*pfun)();
	pfun = func;	
#ifdef _windriver
	set_thread_affinity(rt_core_id);
#else
	CPU_ZERO(&set_rtcore);
	CPU_SET(rt_core_id,&set_rtcore);
	if (pthread_setaffinity_np(pthread_self(),sizeof(set_rtcore),&set_rtcore) < 0)
	{
		fprintf(stderr,"set thread affinity failed\n");
	}
	else
	{
		printf("Info: Thread is binded core %d successfully.\n",rt_core_id);
	}
#endif
	pfun();
#ifdef _windriver
#else
	pthread_exit(NULL);
#endif
}

/* static void sig_usr(int signo)
{
	if (signo == SIGINT)
	{
		if (rt_core_id > 0)
		{
			release_rtcore(rt_core_id);
		}
		exit(0);
	}
} */

int create_rt_thread(void *func)
{
	int core, ret;
#ifdef _windriver
	core = allocate_rtcore();
#else
	MPI_Comm_rank(MPI_COMM_WORLD,&core);
	core = core + 1;
#endif
	if (core < 0)
	{
		printf("allocate_rtcore failed, core %d\n", core);
		return -1;
	}
	rt_core_id = core;
	printf("EMT is using core %d\n", core);

	ret = pthread_create(&thread, NULL, (void *)rt_thread, func);
	if (0 == ret)
	{
		printf("EMT: pthread_create returns %d, and its cpu no. is %d\n", ret, core);
	}
	else
	{
		printf("EMT: pthread_create returns %d\n", ret);
		return -2;
	}

	return 0;
}


void cplusplus_rtclock_sleep(int usec)
{
	unsigned long long begin, add;
	//rtclock_gettime(&begin);
#ifdef _windriver
	add = usec*1000;
	rtclock_gettime(&begin);
	begin += add;
	clock_sleepto(begin);
#else
//#if _yuanji
//	usleep_rt(usec);
//
//#else

        struct timeval tv1;
        struct timeval tv2;
        double tmp;
        double tmptheda;
        int i;
        gettimeofday(&tv1,NULL);
        gettimeofday(&tv2,NULL);

        while ((tv2.tv_usec - tv1.tv_usec)+(tv2.tv_sec - tv1.tv_sec)*1000000<usec)
        {
                for (i = 0;i<100;i++)
                {
                        tmptheda = asin(tmp);
                }
                gettimeofday(&tv2,NULL);
        }


//#endif
#endif
}


void cpu_setcore()
{	
	int core = 9 ;	
	int count=0;
	int i = 0;
	MPI_Status status;
	int m = 0;
	int dest = 0;
	int tag = 0;
	int source = 0;
	int NumProcessor = 0;  

	//int processcore;	
//	整体思路. MPI中的RANK号，每5个CPU进程的父进程，共用一个CPU实时核。其中第一个EMT进程去申请一个rtcore，然后后面4个EMT进程共用这个rtcore。
//	1. 第一个EMT进程申请一个rtcore(); //前5个进程使用独立的CPU，超过5个进程以后采用相同的CPU
	MPI_Comm_size(MPI_COMM_WORLD,&NumProcessor);
	MPI_Comm_rank(MPI_COMM_WORLD,&i);
	processcore = i + 1;
/*	if (i>=5)    //从第10个CPU开始父进程共用了
	{
		if (i%5 == 0)
		{
			processcore=allocate_rtcore();
			for(m = 1; m < 5;m++)
			{
				dest = i+m;
				tag = m;
				if (dest < NumProcessor)
				{
					MPI_Send(&processcore,1,MPI_INT,dest,tag,MPI_COMM_WORLD);
				}
			}
		}
		else
		{
			source = i-i%5;
			tag = i%5;
			MPI_Recv(&processcore,1,MPI_INT,source,tag,MPI_COMM_WORLD,&status);
		}
	}
	else
	{
		processcore=allocate_rtcore();
	}*/

	CPU_ZERO(&set);
	CPU_SET(processcore,&set);
	if(sched_setaffinity(getpid(),sizeof(set),&set)==-1)
	{
		printf("process bind cpu err\n");
		return;
	}
	else
	{
		printf("process bind cpu %d.\n",processcore);
	}
	return;
}

volatile int debug_break = 0;

static void sig_usr_debug(int signo)
{
        if (signo == SIGUSR1)
        {
		debug_break = 1;
		printf("LIZHAODBG: set debug_break %d\n", debug_break);
	}
}
int main(int argc, char *argv[])
{

	int core = 9;
	int count = 0;
	struct sched_param parm;
	struct rlimit rlimit_new;
	
	stopprogram = 0;

	//rt_init(5);
	// if (signal(SIGINT, sig_usr) == SIG_ERR)
	// {
	// 	printf("Cannot catch SIGINT\n");
	// }
#if 1
	if (signal(SIGUSR1, sig_usr_debug) ==  SIG_ERR) {

		printf("register SIGUSR1 ERROR\n");
	}
#endif

//	yj_performance_start();
	if (argc > 2)
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

	printf("test count =%u \n", nstep);
	tstep = 50000;

	isCalculationOver = 0;

	memset(&ProcInfo, 0, sizeof(struct SPROCINFO));

	ProcIO = -1; //非IO进程
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &ProcInfo.Id);
	MPI_Comm_size(MPI_COMM_WORLD, &ProcInfo.NumProc);
	//printf("current MPIRank is %d, the size of COMM_WORLD is %d;\n",ProcInfo.Id,ProcInfo.NumProc);
	ISendBuf = (double *)alloc_one_2m_page(ProcInfo.Id);
	IRecvBuf = (double *)alloc_one_2m_page(ProcInfo.Id);
#if 1
#ifdef _LINUX
	cpu_setcore();
#endif
#endif
	//MPI_Sharedmemory_init();
	MPI_Sharedmemory_init();
	init_comm();

	sleep_rt(10);

	init_buf();
	sleep_rt(10);
	init_gvar(ProcInfo.Id);

	while (debug_break == 0)  {
		sleep_rt(0);
	}


	printf("Go out debug break\n");
	MPI_Barrier(MPI_COMM_WORLD);

	printf("ProcInfo.Id:  %d  after MPI COMM WORLD Barrier\n", ProcInfo.Id);

#ifdef _LINUX

	//thread_emt((void *)0);
        //create_rt_thread(thread_emt);
	thread_emt();

#else //_LINUX

	//printf("linux is NOT defined~\n");
	thread_emt();

#endif //_LINUX
	while (stopprogram == 0)
	{
		cplusplus_rtclock_sleep(5000000);
	}
	printf("thread is finished in PID %d.\n", ProcInfo.Id);
	MPI_Barrier(MPI_COMM_WORLD);
	//printf("MPI proc: %d  after main MPI_Barrier....\n",ProcInfo.Id);

#ifdef _LINUX
#if 1
	if (0 == ProcInfo.Id)
	{
		if ((fp = fopen("log", "a")) != NULL)
		{
			char str[256], sTime[512];
			time_t rawtime;
			struct tm *timeinfo;
			//printf("MPI proc: %d  bp time1 \n",ProcInfo.Id);
			time(&rawtime);
			//printf("MPI proc: %d  bp time2 \n",ProcInfo.Id);
			timeinfo = localtime(&rawtime);
			//printf("MPI proc: %d  bp time3 \n",ProcInfo.Id);

			//sleep(1);
			//if(asctime(timeinfo) != NULL)
			//printf("%s\n",asctime(timeinfo));
			//strftime(sTime, 512, "%Y-%m-%d %H:%M:%S", timeinfo);
			//puts(sTime);
			//printf("MPI proc: %d  bp time4 \n",ProcInfo.Id);
			printf("%s\n", sTime);
			fprintf(fp, sTime);
			sprintf(str, "%lld  %lld  %lld  %d  %lf  %lf  %lf  %lf\n", tstep, nstep, tc - ts, cnt, tmax_ctrl, tmin_ctrl, tmax_cal, tmin_cal);
			fprintf(fp, str);
			//printf("MPI proc: %d  bp time5 \n",ProcInfo.Id);
			fclose(fp);
		}
	}

	if (-1 == ProcInfo.Id)
	{
		if ((fp = fopen("log", "a")) != NULL)
		{
			char str[256], sTime[128];
			time_t rawtime;
			struct tm *timeinfo;
			time(&rawtime);
			timeinfo = localtime(&rawtime);
			strftime(sTime, sizeof(sTime), "%Y-%m-%d %H:%M:%S    ", timeinfo);
			printf("%s\n", sTime);
			fprintf(fp, sTime);
			sprintf(str, "%lld  %lld  %lld  %d  %lf  %lf  %lf  %lf\n", tstep, nstep, tc - ts, cnt, tmax_ctrl, tmin_ctrl, tmax_cal, tmin_cal);
			fprintf(fp, str);
			fclose(fp);
		}
	}
#endif
#endif //_LINUX

	finalize_buf();

	//printf("MPI proc: %d  after finalize_buff....\n",ProcInfo.Id);
	finalize_comm();
	//printf("MPI proc: %d  after finalize_comm....\n",ProcInfo.Id);

	//release_gvar();

	MPI_Finalize();
	//printf("MPI proc: %d  after MPI_Finalize....\n",ProcInfo.Id);

	munlockall();

	//printf("MPI proc: %d  after munlockall....\n",ProcInfo.Id);
#ifdef _windriver
	if (pthread_rx_event_core != 0)
	{
		release_rtcore(pthread_rx_event_core);
		printf("\npthread_create(rx_event): release_rtcore,core = %d\n", pthread_rx_event_core);
		pthread_rx_event_core = 0;
	}

	release_rtcore(core);
#else
//	CPU_CLR(processcore,&set);
#endif

	printf("MPI proc: %d  after release_rtcore....\n", ProcInfo.Id);

	rt_core_id = 0;
	return 0;
}
