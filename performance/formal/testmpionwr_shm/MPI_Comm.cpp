//////////////////////////////////////////////////////////////////////
// 版权 (C), 1988-1999, XXXX公司
// 文 件 名: MPI_Comm.c
// 作    者:        版本:       时间:
// 描    述:  MPI通讯结构构成
//            包含子程序：
//                       initNet_Standard()
//                       initNet_GI0() 
//                       initElements()
//                       getNewNoElements() 
//                       creatIYJYYElements()
// 其    他：用 #include <filename.h> 格式来引用标准库的头文件（编译器将从标准库目录开始搜索）
//           用 #include "filename.h" 格式来引用非标准库的头文件（编译器将从用户的工作目录开始搜索）         
// 修改记录:       // 历史修改记录
// 1. 时间:
//    作者:
//    修改内容:
// 2. ...
//////////////////////////////////////////////////////////////////////
#include "mpi.h"
#ifdef _WINDOWS
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <iostream>
#include <algorithm>
#include <functional>
#include <iterator>
#include <iostream>

#include <cmath>

#define _MPI_SHM_PROXY
#include "Pub_Def.h"
#include "MPI_Comm_def.h"
#include "IOComm_Proxy.h"
#include "MPI_Comm.h"

#define MAXPROCNUM 1000
//Message type
#define PIDInfo 01
#define DIDInfo 02
#define MPCMDInfo 03
#define MPCONInfo 04
#define MPMONInfo 05
#define EMTDIV4Info 06
#define EMTDIV7Info 07

#define MPIEMTBUFFSIZE 80000 //80K  9*25*6*8=
#define MPIIOBUFFSIZE  100000 //240K

#define _MPI_SHM_PROXY
#define _DBG_LEVEL 4

using namespace std;

CommMPIInfo s_CommMPIInfo;
MPICommInfo g_MPICommInfo;        //s_MPICommInfo记录了整个通讯系统的所有通讯信息

BuffInfo* MPISendBuff;
BuffInfo* MPIRecvBuff[MAXSOCKETCPU];
BuffInfo* MPISendBuffAll[MPIBUFFBLOCKNUM];
BuffInfo* MPIRecvBuffAll[MPIBUFFBLOCKNUM][MAXSOCKETCPU];
MPICOMM_PACKAGE EMTSend;
vector<vector<MPICOMM_PACKAGE> >* MPIMessage;
vector<vector<vector<MPICOMM_PACKAGE> > > MPIMessage_Mem; //

extern int ProcIO;  //仿真进程对应的IO进程号
extern int iMYID;
extern int iKsub;

struct SPROCINFO {
	MPI_Comm CommCAL;
	MPI_Comm CommST;
	MPI_Comm CommEMT;
	MPI_Comm CommEMTCAL;	
	
	int NumProc;
	int NumProcCAL;
	int NumProcST;
	int NumProcEMT;
	int NumProcEMTCAL;
	int NumProcEMTPhy;
	int NumProcEMTIO;

	int Id;					// �����������н����е�ID
	int CALId;				// ��������CommCAL���е�ID
	int EMTId;				// ��������CommEMT���е�ID
	int EMTCALId;			// ��������CommEMTCAL���е�ID

	int* EMTPhyId;			// �����̶�Ӧ��EMTPhy����ID
	int EMTIOId;			// �����̶�Ӧ��EMTIO����ID
	int SimType;
};
extern struct SPROCINFO ProcInfo;
//MPI_Comm_Stru ProcEMT;            //通讯体赋值


//---------------------------------类定义-------------------------------------------------
//************************************
// Method:    PackageForm
// FullName:  MPICOMM_PACKAGE::PackageForm
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: char * databuff
// Parameter: int length
// Parameter: int
// Parameter: int tag
// Parameter: MPI_Comm comm
// Auther:    Qing Mu
// Data:      2019/07/17
//************************************
//void MPICOMM_PACKAGE::SendPackageForm(char* databuff,int length,int dest ,int tag, MPI_Comm comm, int typ)
//{
//	this->header = 0xFFFF0000;
//	this->source =  g_ParInfo.iMYID; //当前的系统的MPI进程号
//	this->dest = dest;
//	this->length = length;
//	this->typ  = typ;  //发送报文的类型
//	this->tag = tag;
//	this->comm = comm;
//	this->dataptr = databuff;
//}

void MPICOMM_PACKAGE::send(void* databuff,int length,int dest ,int tag, MPI_Comm comm, int typ)
{
	//1. 建立发送实例的初始化信息
	this->header = 0xFFFF0000;
	int i;
	MPI_Comm_rank(comm,&i);
	this->source =  i;
	this->dest = dest;
	this->length = length;
	this->typ  = typ;  //发送报文的类型
	this->tag = tag;
	this->comm = comm;
	this->dataptr = databuff;
	(*MPISendBuff).elem_pushback(&header,FRAMEHEADERLEN);
	(*MPISendBuff).elem_pushbackwithoutaddiNum(this->dataptr,length);
	this->dataptr = (*MPISendBuff).currentaddress;
}

void MPICOMM_PACKAGE::sendanyBuff()   //把MPIPackage中的信息直接写入一个Buff中，这种情况是MPIPackage的数据地址和报文信息已经初始化好了的情况。
{
	//1. 建立发送实例的初始化信息
	(*MPISendBuff).elem_pushback(&header,FRAMEHEADERLEN);
	(*MPISendBuff).elem_pushbackwithoutaddiNum(this->dataptr,length);
	this->dataptr = (*MPISendBuff).currentaddress;
}

void MPICOMM_PACKAGE::init(char* databuff,int dest ,int length,int typ,int tag, MPI_Comm comm)
{
	this->header = 0xFFFF0000;
	int i;
	MPI_Comm_rank(comm,&i);
	this->source =  i;
	this->dest = dest;
	this->length = length;
	this->typ  = typ;  //发送报文的类型
	this->tag = tag;
	this->comm = comm;
	this->dataptr = databuff;
}

int MPICOMM_PACKAGE::checkandrecv(void* databuff,int length,int source,int tag, MPI_Comm comm,int typ,MessagePos& pos)
{
	int ierr = this->check(source,tag, comm,typ,pos);
	if (ierr == 0)
	{
		ierr = this->recv(databuff,length,source,tag,comm);
	}
	return ierr;
}

struct SourceCompare: binary_function<MPICOMM_PACKAGE, int,bool> 
{
	bool operator()( MPICOMM_PACKAGE &value, int source) const
	{
		if (value.dest== source)
			return true;
		else
			return false;
	}
};

struct TypCompare: binary_function<MPICOMM_PACKAGE, int,bool> 
{
	bool operator()( MPICOMM_PACKAGE &value, int source) const
	{
		if (value.typ== source)
			return true;
		else
			return false;
	}
};

int MPICOMM_PACKAGE::check(int source, int tag, MPI_Comm comm,int typ, MessagePos& pos)
{
	int RecvRank = 0;
	int DestInfo = 0; //报文的目标地址
	vector<MPICOMM_PACKAGE>::iterator MPIMessageite;
	if (comm == g_MPICommInfo.SocketComm)
	{
		RecvRank = source;
		DestInfo = g_MPICommInfo.iMYID_SocketComm;  //SocketComm下的进程号
	}
	else if (comm == MPI_COMM_WORLD)
	{
		RecvRank = g_MPICommInfo.rank_socketcomm[source];
		DestInfo = iMYID;  //MPI_COMM_WORLD下的进程号
	}
	else
	{
		Translate_Ranks(comm,1,&source,&RecvRank,g_MPICommInfo.SocketComm);
		Translate_Ranks(g_MPICommInfo.SocketComm,1,&g_MPICommInfo.iMYID_SocketComm,&DestInfo,comm);
	}
	if ((typ == MPCONInfo)||(typ == MPMONInfo))
	{
		MPIMessageite = find_if((*MPIMessage)[RecvRank].begin(),(*MPIMessage)[RecvRank].end(),bind2nd(TypCompare(),MPCONInfo));
	}
	else
	{
		for (MPIMessageite = (*MPIMessage)[RecvRank].begin();MPIMessageite != (*MPIMessage)[RecvRank].end(); ++MPIMessageite)
		{
			if ((MPIMessageite->dest == DestInfo)&&(MPIMessageite->typ == typ)&&(MPIMessageite->tag == tag))
			{
				break;
			}
		}
	}
	if (MPIMessageite !=(*MPIMessage)[RecvRank].end())
	{
		//找到了相关的Message地址;
		copy(*MPIMessageite);
		pos.iProc = RecvRank;
		pos.iPackage =  MPIMessageite - (*MPIMessage)[RecvRank].begin();
	}
	else
	{
		//没有找到相关的Message;
		//错误处理待补
		return 1;
	}
	return 0;
}

void MPICOMM_PACKAGE::copy(MPICOMM_PACKAGE& a)
{
	header = a.header;
	source = a.source;
	dest = a.dest;
	length = a.length;
	typ = a.typ;
	tag = a.tag;
	dataptr = a.dataptr;
	comm = a.comm;
}

int MPICOMM_PACKAGE::recv(void* databuff,int length,int source,int tag, MPI_Comm comm)
{
	//不check直接传输
	int ierr = 0;
	if ((this->source == source)&&(this->tag == tag)&&(this->length == length))
	{
		memcpy(databuff,this->dataptr,length);
	}
	else
	{
		//错误处理
		ierr = 1;
	}
	return ierr;
}

void Translate_Ranks(MPI_Comm comm1, int n_partners, int *partners, int *partners_map, MPI_Comm comm2)
{
	MPI_Group world_group, shared_group;

	/* create MPI groups for global communicator and shm communicator */
	MPI_Comm_group(comm1, &world_group);
	MPI_Comm_group(comm2, &shared_group);
	MPI_Group_translate_ranks(world_group, n_partners, partners, shared_group, partners_map);
}

void MPI_EMTCAL_RANK(MPI_Comm comm1, int n_partners, int *partners)
{
	//1.
	int* IDRecvBuff = (int*) calloc(n_partners + 1,sizeof(int));

	Translate_Ranks(comm1, n_partners,partners,IDRecvBuff,g_MPICommInfo.SocketComm);

	//2.
	for (int i = 0; i< n_partners;i++)
	{
		g_MPICommInfo.rank_emtcalcomm[partners[i]] = IDRecvBuff[i];
	}
	printf("the current Process Ksub is %d, the correspending Socketrank is %d.\n",iKsub, g_MPICommInfo.rank_emtcalcomm[iKsub - 1]);
}
//-----------------------------MPI3的接口和映射问题-----------------------------//
void MPI_Memary_Alloc(MPI_Comm shmcomm,void* membuffptr[MPIBUFFBLOCKNUM], MPI_Win& win,int size)  //MPI内存的申请
{
	void *basmemptr;
	int blocksize = floor(size/MPIBUFFBLOCKNUM+1.0e-9);
	MPI_Win_allocate_shared(size * sizeof(char), 1, MPI_INFO_NULL, shmcomm, /* inputs to MPI-3 SHM collective */
		&basmemptr, &win);
	for (int i = 0; i<MPIBUFFBLOCKNUM; i++)
	{
		membuffptr[i] = &((char*)basmemptr)[blocksize*i];//printf 地址
#if _DBG_LEVEL >=3
		printf("shared memory address is %llx and the stored membuffptr is %llx in Block %d in Process %d.\n",basmemptr,membuffptr[i],i,ProcInfo.Id);
#endif
	}
}

//-----------------------------MPI3的共享地址的查询-----------------------------//
void Get_Link_Ptrs(MPI_Win win,void* (&basmemptr)[MPIBUFFBLOCKNUM][MAXSOCKETCPU])  //MPI3的共享地址的查询
{
	int i,dsp_unit;
	MPI_Aint sz;
	int rank;
	int blocksize;
	if (g_MPICommInfo.iIsProcIO == 1)
	{
		blocksize = MPIIOBUFFSIZE;
	}
	else
	{
		blocksize = MPIEMTBUFFSIZE;
	}

	for (int i = 0; i < g_MPICommInfo.iNumCommProc; i++)
	{
		void* Memoryaddress;
		MPI_Win_shared_query( win,i,&sz,&dsp_unit, &Memoryaddress); //把所有共享内存的地址都获取到
		for (int j = 0; j<MPIBUFFBLOCKNUM; j++)
		{
			basmemptr[j][i] = (char*)Memoryaddress+blocksize*j;//printf 地址p 
#if _DBG_LEVEL >=3
			printf("shared memory address is %llx and the stored membuffptr is %llx in ProcId %d in Process %d.\n",basmemptr[j][i],Memoryaddress,i,ProcInfo.Id);
#endif
		}
	}
	return;
}

//---------------------------MPI3的通讯域划分------------------------------------//
void MPIcomm_sharedmem_Build(int socketid,MPI_Comm comm,MPI_Comm* shmcomm)  //MPI3的共享内存的通讯体构建
{
	int ikey;
	MPI_Comm_rank(comm,&ikey);
	MPI_Comm_split(comm,socketid,ikey,shmcomm);
	int ranknum = 0;
	MPI_Comm_size(*shmcomm,&ranknum);
	MPI_Barrier(*shmcomm);
	int iRank = 0;
	MPI_Comm_rank(*shmcomm,&iRank);
	if (iRank == 0)
	{
		printf("MPIComm_sharedmem has been established.\nsocketid is %d, shmcomm size is %d\n", socketid, ranknum);
	}
}

void BindMPIWin(void* RecvAddress,BuffInfo* BuffInfoptr,int size)     //绑定BuffInfo
{
	BuffInfoptr->buffaddress = (BuffInfo*)RecvAddress;
	BuffInfoptr->startaddress = (char*)RecvAddress+sizeof(BuffInfo);
	BuffInfoptr->currentaddress = BuffInfoptr->startaddress;
	BuffInfoptr->iNum = 0;
	BuffInfoptr->length = 0;
	BuffInfoptr->MaxLength = size;
	BuffInfoptr->mode = NOBUFFMODE;         //普通模式，绑定
}

void BindBuff_init()
{
	int size;
	vector<vector<MPICOMM_PACKAGE> > a;
	//5.0 初始化MPIMessage_Mem
	for (int j = 0;j<MPIBUFFBLOCKNUM;j++)
	{
		MPIMessage_Mem.push_back(a);
	}

	//5.1 调用地址分配命令：
	for (int j = 0;j<MPIBUFFBLOCKNUM;j++)
	{
		int sendtmp = (j + MPIBUFFBLOCKNUM)%MPIBUFFBLOCKNUM; 
		int receivetmp = (j + MPIBUFFBLOCKNUM - 1)%MPIBUFFBLOCKNUM; 
		g_MPICommInfo.MPISendAddress = g_MPICommInfo.MPISendAddressMem[sendtmp];
		memcpy(&g_MPICommInfo.MPIRecvAddress[0],g_MPICommInfo.MPIRecvAddressMem[receivetmp],sizeof(void*)*MAXSOCKETCPU);
		for (int i = 0;i<g_MPICommInfo.iNumCommProc;i++)
		{
			if (i == g_MPICommInfo.iMYID_SocketComm)
			{
				int size = 0;
				if (i == g_MPICommInfo.iProcIO_SocketComm)
				{
					size = MPIIOBUFFSIZE;
				}
				else
				{
					size = MPIEMTBUFFSIZE;
				}
				MPISendBuffAll[sendtmp] = new(g_MPICommInfo.MPISendAddress)BuffInfo();
				BindMPIWin(g_MPICommInfo.MPISendAddress,MPISendBuffAll[sendtmp],size-sizeof(BuffInfo)-1);
			}
			else
			{
				if (i == g_MPICommInfo.iProcIO_SocketComm)
				{
					size = MPIIOBUFFSIZE;
				}
				else
				{
					size = MPIEMTBUFFSIZE;
				}
				MPIRecvBuffAll[receivetmp][i] = new BuffInfo();
				BindMPIWin(g_MPICommInfo.MPIRecvAddress[i],MPIRecvBuffAll[receivetmp][i],size-sizeof(BuffInfo)-1);
			}
		}
	}
	BindBuff_step(MPIBUFFBLOCKNUM);  // 初始化加第一步barrier前写0片,初始化加第一步barrier前读1片	
	return;
}

void MPI_Sync_Step(int step)
{
	//1. MPI信息同步
      //MPI_Barrier(MPI_COMM_WORLD); /* time barrier to make sure all ranks have updated their info */	
	  MPI_Win_sync(g_MPICommInfo.win); /* memory fence to sync node exchanges */
      //if (u != -1)
      //{
      MPI_Barrier(g_MPICommInfo.SocketComm); /* time barrier to make sure all ranks have updated their info */
     // memcpy(mem,localmem,10000);
     // MPI_Barrier(shmcomm);
      //}
      MPI_Win_sync(g_MPICommInfo.win);

	//2. Buff 切换
	BindBuff_step(step); 
	//3. 报文的分割处理
	PackageDivision(step);

}

void MPI_Win_Lock()
{
	MPI_Win_lock_all(MPI_MODE_NOCHECK, g_MPICommInfo.win);
}

void MPI_Win_unLock()
{
	MPI_Win_unlock_all(g_MPICommInfo.win);
}

void BindBuff_step(int step)
{
	int size;
	//5.1 调用地址分配命令：
	int sendtmp = step%MPIBUFFBLOCKNUM; // 第一步barrier后加第二步Barrier前写1片
	int receivetmp = (step - 1)%MPIBUFFBLOCKNUM; // 第一步barrier后加第二步Barrier前读0片
	g_MPICommInfo.MPISendAddress = g_MPICommInfo.MPISendAddressMem[sendtmp];
	memcpy(&g_MPICommInfo.MPIRecvAddress[0],g_MPICommInfo.MPIRecvAddressMem[receivetmp],sizeof(void*)*MAXSOCKETCPU);

	MPISendBuff = MPISendBuffAll[sendtmp];
	//clear Buff Content
	(*MPISendBuff).clear();
	for (int i = 0;i<g_MPICommInfo.iNumCommProc;i++)
	{
		MPIRecvBuff[i] = MPIRecvBuffAll[receivetmp][i];
	}

	MPIMessage = &MPIMessage_Mem[receivetmp];

	return;
}

////////////////////////////////////////////////////////////////////////
// 函 数 名:          // strtok_uni
// 描    述:          // strtok_s 和 strtok_r的统一平台问题
////////////////////////////////////////////////////////////////////////
char* strtok_uni(char* Dest, const char* Source,char** Type)
{
	char* stringout;
#ifdef _WINDOWS
	stringout = strtok(Dest,Source);
#else
	stringout = strtok(Dest,Source);
#endif
	return stringout;
}

//////////////////////////////////////////////////////////////////////
// 函 数 名:          // getFileLineNum
// 描    述:          // 获取文件有效行数
// 输入参数:          // pfile：文件指针。(FILE *)
// 输出参数:          // 无
// 返 回 值:          // i：文件行数。(int)
// 其    他:          // 其它说明
//////////////////////////////////////////////////////////////////////
int getFileLineNum(FILE *pfile)
{
	int i = 0;
	int iflag = 0;
	char mystring[4096];
	char *pcnext = NULL;

	// 统计文件有效行数
	while( !feof(pfile) )
	{
		char *a = fgets(mystring,MAXSTRLEN,pfile);
		if( a == NULL ) continue;
		iflag = atoi(strtok_uni(mystring,",",&pcnext));
		if( iflag != 0 ) // 该行记录有效，统计行数加1
		{
			i=i+1;
		}		
	}
	// 将pfile指示的文件中的位置指针置于文件开头位置，并清除文件结束标志和错误标志
	rewind(pfile);

	// 返回文件有效行数
	return i;
}

void CPUAlloc_readIn()
{
	// 临时变量
	FixedString pcOpenFile;  //打开的文件
	FILE *fp = NULL;
	FixedString strline;
	int iflag = 0;
	char *pcnext = NULL;
	int i = 0;
	int iEMT_SubNum = 0;
	fp = fopen("CPUAlloc.INF","r+");
	if( !fp )
	{
		printf("Warning：Can't find CPUAlloc.INF!\n");
	}
	else
	{		    
		int iNum=getFileLineNum(fp);
		// 开始读入元件信息
		if (iNum == 0)
		{

		}
		else
		{
			// 读数据文件
			while( !feof(fp) )
			{
				char *a=fgets(strline,MAXSTRLEN,fp);
				if( a == NULL ) continue;
				iflag = atoi((const char*)strtok_uni(strline,",",&pcnext));
				if( iflag == 0 ) // 该行记录无效，退出本次循环
				{
					continue;
				}
				g_MPICommInfo.CPUid_commworld[i] = atoi(strtok_uni(NULL,",",&pcnext));     //分配的核号
				g_MPICommInfo.Socketid_commworld[i] = atoi(strtok_uni(NULL,",",&pcnext));     //分配的Socket号
				int MPIrank = atoi(strtok_uni(NULL,",",&pcnext));     //分配的核号
				char* tmp = strtok_uni(NULL,",",&pcnext);
				g_MPICommInfo.EMTsub_commworld[i] = atoi(strtok_uni(NULL,",",&pcnext));     //分配的子网号
				g_MPICommInfo.Proctyp_commworld[i] = atoi(strtok_uni(NULL,",",&pcnext));     //进程类型
				if ((g_MPICommInfo.EMTsub_commworld[i] > 0)&&((g_MPICommInfo.Proctyp_commworld[i] > 100) &&(g_MPICommInfo.Proctyp_commworld[i] < 200)))
				{
					iEMT_SubNum++;
				}
				i++; // i统计元件数目
			} 
		} // end of while( !feof(fp) ) 
		fclose(fp);
		g_MPICommInfo.iNumTotolProc = i;
		g_MPICommInfo.iNumEMTProc = iEMT_SubNum;
	}

	//本进程信息赋值。
	g_MPICommInfo.iSocketid = g_MPICommInfo.Socketid_commworld[ProcInfo.Id];
	g_MPICommInfo.iCPUid = g_MPICommInfo.CPUid_commworld[ProcInfo.Id];
#ifdef _MPI_SHM_PROXY
	iKsub = g_MPICommInfo.EMTsub_commworld[ProcInfo.Id];
	cout<<"g_ParInfo.iKsub = "<<iKsub<<" ProcInfo.Id = " << ProcInfo.Id <<endl;
#endif
	for (int i = 0;i<g_MPICommInfo.iNumTotolProc;i++)
	{
		g_MPICommInfo.rank_socketcomm[i] = -1;  //no jobs
		g_MPICommInfo.job_socketcomm [i] = -1;
	}
	g_MPICommInfo.iProcIO_COMMWORLD = ProcIO;
	if (g_MPICommInfo.iProcIO_COMMWORLD == ProcInfo.Id) g_MPICommInfo.iIsProcIO = 1;
	else g_MPICommInfo.iIsProcIO = 0;
}

//---------------------------MPI3的通讯域初始化------------------------------------//
void MPI_Sharedmemory_init()  //MPI3的共享内存的初始化
{

	int size = 0;
	//0. 记录当前子网的MYID号
	MPI_Comm_rank(MPI_COMM_WORLD,&ProcInfo.Id);
	//1. read CPUAlloc.INF文件
	CPUAlloc_readIn();

	//2. build new shared comm
	MPIcomm_sharedmem_Build(g_MPICommInfo.iSocketid,MPI_COMM_WORLD,&g_MPICommInfo.SocketComm);  //MPI3的共享内存的通讯体构建

	MPI_Comm_size(g_MPICommInfo.SocketComm,&g_MPICommInfo.iNumCommProc);

	//3. translate the Ranks from rank MPI_COMM_WORLD to rank shared comm
	printf("step 3 is starting;\n");
	int* rankorg = new int[g_MPICommInfo.iNumCommProc];
	int* ranknew = new int[g_MPICommInfo.iNumCommProc];
	for (int i = 0; i<g_MPICommInfo.iNumCommProc; i++)
	{
		rankorg[i] = -1;  //初始化为 - 1；
		ranknew[i] = -1;  //初始化为 - 1；
	}
	MPI_Allgather(&ProcInfo.Id, 1, MPI_INT, rankorg, 1, MPI_INT, g_MPICommInfo.SocketComm);
	Translate_Ranks(MPI_COMM_WORLD, g_MPICommInfo.iNumCommProc,rankorg, ranknew, g_MPICommInfo.SocketComm);
	for (int i = 0;i<g_MPICommInfo.iNumCommProc;i++)
	{
		g_MPICommInfo.rank_socketcomm[rankorg[i]] = ranknew[i];
		g_MPICommInfo.job_socketcomm [ranknew[i]] = rankorg[i];
	}
	g_MPICommInfo.iMYID_SocketComm = g_MPICommInfo.rank_socketcomm[ProcInfo.Id];
#if 0	
	g_MPICommInfo.iProcIO_SocketComm = g_MPICommInfo.rank_socketcomm[g_MPICommInfo.iProcIO_COMMWORLD];
#else
	g_MPICommInfo.iProcIO_SocketComm = -1;
#endif

	//4. 共享内存分配
	printf("step 4 is starting;\n");
	if (g_MPICommInfo.iIsProcIO == 1)
	{
		size = MPIIOBUFFSIZE*MPIBUFFBLOCKNUM;
	}
	else
	{
		size = MPIEMTBUFFSIZE*MPIBUFFBLOCKNUM;
	}
	char* MemoryAddress = NULL;
	MPI_Memary_Alloc(g_MPICommInfo.SocketComm,g_MPICommInfo.MPISendAddressMem, g_MPICommInfo.win,size);  //MPI内存的申请
#if _DBG_LEVEL >=3
		printf("shared memory address is %llx and the next membuffptr is %llx in Process %d.\n",g_MPICommInfo.MPISendAddressMem[0],g_MPICommInfo.MPISendAddressMem[1],ProcInfo.Id);
#endif


	//5. 获取所有访问地址
	Get_Link_Ptrs(g_MPICommInfo.win,g_MPICommInfo.MPIRecvAddressMem);  //MPI3的共享地址的查询

	//6. 把所有的BuffInfo与获取的地址进行绑定映射
	BindBuff_init();

	return;
}

int MPI_EMT_Send(void* sendbuff,void* recvbuff,int count,MPI_Datatype datatype,int iDest,int tag,MPI_Comm comm,MPI_Request* request)
{
	int rank = g_MPICommInfo.rank_emtcalcomm[iDest];
	if (rank == MPI_UNDEFINED)   //区外通信
	{
		MPI_Irecv(recvbuff,count,MPI_DOUBLE,iDest,33,comm,request++);
		MPI_Isend(sendbuff,count,MPI_DOUBLE,iDest,33,comm,request++); //MuQ 181215
		return 2; //request add 2
	}
	else  if (rank != MPI_UNDEFINED)   //区内通信
	{
		int disp_unit;
		MPI_Type_size( MPI_DOUBLE, &disp_unit ); 
		EMTSend.send(sendbuff,count*disp_unit,rank,33,g_MPICommInfo.SocketComm,EMTDIV4Info);
		return 0;
	}
	return 0;
	
}

void MPI_EMT_Recv(void* buf,int count,MPI_Datatype datatype,int iSource,int tag,MPI_Comm comm,MessagePos* RecvPos, int step) //MuQ 181215
{
	int rank = g_MPICommInfo.rank_emtcalcomm[iSource];
	if (rank == MPI_UNDEFINED)   //区外通信
	{

	}
	else if (rank != MPI_UNDEFINED)   //区内通信
	{
		int disp_unit;
		MPI_Type_size( MPI_DOUBLE, &disp_unit ); 
		if (step == 1)     //初始化时确定报文的位置
		{
			MPICOMM_PACKAGE RecvPackage;
			int ierr = RecvPackage.checkandrecv(buf,count*disp_unit,rank,tag,g_MPICommInfo.SocketComm,EMTDIV4Info,*RecvPos);
		}
		else
		{
			(*MPIMessage)[(*RecvPos).iProc][(*RecvPos).iPackage].recv(buf,count*disp_unit,rank,tag,g_MPICommInfo.SocketComm);
		}
	}
}

int isIOProc()
{
	return g_MPICommInfo.iIsProcIO;
}



#if 0
//-----------元件接口函数定义---------------------------------------
//******以下接口函数供需要用到分网元件变量的接口函数调用
//////////////////////////////////////////////////////////////////////
// 函 数 名:          // getStaticCommMPI
// 描    述:          // 返回静态变量s_CommMPIInfo地址
// 输入参数:          // 无
// 输出参数:          // 无
// 返 回 值:          // s_CommMPIInfo地址
// 其    他:          // 无
//////////////////////////////////////////////////////////////////////
CommMPIInfo* getStaticCommMPI()
{
	return &s_CommMPIInfo;
}

//////////////////////////////////////////////////////////////////////
// 函 数 名:          // returnMPI_Comm
// 描    述:          // 返回通讯体的静态地址
// 输入参数:          // 无
// 输出参数:          // 无
// 返 回 值:          // MPI_Comm地址
// 其    他:          // 无
//////////////////////////////////////////////////////////////////////
MPI_Comm returnMPI_Comm(int MPI_Typ)
{
	MPI_Comm tmp;
	switch (MPI_Typ)
	{
	case MPI_CommST: tmp = ProcEMT.CommST;
		break;
	case MPI_CommEMT: tmp = ProcEMT.CommEMT;
		break;
	case MPI_CommEMTCAL: tmp = ProcEMT.CommEMTCAL;
		break;
	case MPI_CommCAL: tmp = ProcEMT.CommCAL;
		break;
	case MPI_CommEMTCALGRP: tmp = ProcEMT.CommEMTCALGrp;
		break;
	default: tmp = MPI_COMM_NULL;
	}
	return tmp;
}

//////////////////////////////////////////////////////////////////////
// 函 数 名:          // MPI_EMT_Start()
// 描    述:          // MPI Base Function Initialization
// 输入参数:          // 无
// 输出参数:          // 无
// 返 回 值:          // 无
// 其    他:          // 无
//////////////////////////////////////////////////////////////////////
void MPI_EMT_Start(int n,int r)
{
	int iTmp;
	s_CommMPIInfo.CommMPIId.MyId = r;
	s_CommMPIInfo.CommMPIId.NumProcALL = n;
	g_ParInfo.iMYID = s_CommMPIInfo.CommMPIId.MyId;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 函 数 名:          // FormComm()
// 描    述:          // Comm group form
// 输入参数:          // 无
// 输出参数:          // 无
// 返 回 值:          // Error Tag
// 其    他:          // 无
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int FormComm()
{
	int *IDataRecv;
	int *PCbuffer;
	int i, Ntype;

	IDataRecv = new int[s_CommMPIInfo.CommMPIId.NumProcALL];
	PCbuffer = new int[s_CommMPIInfo.CommMPIId.NumProcALL];

	s_CommMPIInfo.CommCALId.NumProcALL = 0;
	s_CommMPIInfo.CommEMTId.NumProcALL = 0;
	s_CommMPIInfo.CommEMTCALId.NumProcALL = 0;
	s_CommMPIInfo.CommSTId.NumProcALL = 0;

	s_CommMPIInfo.CommCALId.MyId = 0;
	s_CommMPIInfo.CommEMTId.MyId = 0;
	s_CommMPIInfo.CommEMTCALId.MyId = 0;
	s_CommMPIInfo.CommSTId.MyId = 0;

	MPI_Group GroupAll,GroupST,GroupEMT,GroupEMTCAL,GroupCAL,GroupEMTFPGA;		//全部进程组、机电暂态进程组、电磁暂态进程组、电磁暂态子网进程组、计算进程组
	printSimMsg("******Formcomm() begin******\n");


	//----- 形成机电暂态、电磁暂态计算的通信体
	MPI_Comm_group(MPI_COMM_WORLD, &GroupAll);

	// 每个进程都发送自己的类型号，并收集所有进程的类型号

	Ntype=SIMUTYPE_EMTSUB;   //10.14 修改为计算进程

	printSimMsg("******Formcomm() begin waiting ******\n");

	MPI_Allgather( &Ntype, 1, MPI_INTEGER, IDataRecv, 1, MPI_INTEGER, MPI_COMM_WORLD);

	printSimMsg("******Formcomm() end waiting ******\n");

	// 计算进程通信体
	int NumProcCAL = 0;
	for (i=0; i<s_CommMPIInfo.CommMPIId.NumProcALL; i++)
	{
		if (IDataRecv[i]<=200)
		{
			PCbuffer[NumProcCAL] = i;
			NumProcCAL = NumProcCAL + 1;
		}
	}
	if (NumProcCAL>0)
	{
		MPI_Group_incl(GroupAll, NumProcCAL, PCbuffer, &GroupCAL);		//形成计算进程组
		MPI_Comm_create(MPI_COMM_WORLD, GroupCAL, &ProcEMT.CommCAL);	//形成计算通信域
		printSimMsg("Comm CAL is created!\n");
		MPI_Comm_size(ProcEMT.CommCAL,&s_CommMPIInfo.CommCALId.NumProcALL);
		MPI_Comm_rank(ProcEMT.CommCAL,&s_CommMPIInfo.CommCALId.MyId);
	}
	else
	{
		ProcEMT.CommCAL = MPI_COMM_NULL;
	}
	printSimMsg("Number of CAL proc: %d\n", s_CommMPIInfo.CommCALId.NumProcALL);

	// 机电暂态通信体
	int NumProcST = 0;

	for (i=0; i<s_CommMPIInfo.CommMPIId.NumProcALL; i++)
	{
		if (IDataRecv[i]<=100)
		{
			if (IDataRecv[i]==SIMUTYPE_STCTRL) g_ParInfo.iIDPSASP = i;
			//Debug_pause(1);
			PCbuffer[NumProcST] = i;
			NumProcST = NumProcST + 1;            
		}
	}

	if (NumProcST>0)
	{
		//将已存在的进程组GroupCAL中NumProcST个的生成新的进程组GroupST
		MPI_Group_incl(GroupCAL, NumProcST, PCbuffer, &GroupST);		//形成机电暂态进程组

		MPI_Comm_create(ProcEMT.CommCAL, GroupST, &ProcEMT.CommST);				//形成机电暂态通信域		

		printSimMsg("Comm ST is created!\n");

		//MPI_Comm_size(ProcEMT.CommST,&s_CommMPIInfo.CommSTId.NumProcALL);
		s_CommMPIInfo.CommSTId.NumProcALL = NumProcST;
		s_CommMPIInfo.CommSTId.MyId = MPI_UNDEFINED; //非本通讯体进程
		//MPI_Comm_rank(ProcEMT.CommST,&s_CommMPIInfo.CommSTId.MyId);
	}
	else
	{
		ProcEMT.CommST = MPI_COMM_NULL;
	}
	printSimMsg("Number of ST Proc: %d\n", s_CommMPIInfo.CommSTId.NumProcALL);
	// 电磁暂态通信体
	int NumProcEMT = 0;
	for (i=0; i<s_CommMPIInfo.CommMPIId.NumProcALL; i++)
	{
		if ((IDataRecv[i]>100)&&(IDataRecv[i]<=200))
		{
			PCbuffer[NumProcEMT] = i;
			NumProcEMT = NumProcEMT + 1;
		}
	}
	// 电磁暂态子网进程组中的进程号要与子网号相匹配
	if (NumProcEMT>0)
	{
		//将已存在的进程组GroupCAL中NumProcEMT个的生成新的进程组GroupEMT
		MPI_Group_incl(GroupCAL, NumProcEMT, PCbuffer, &GroupEMT);		//形成电磁暂态进程组
		MPI_Comm_create(ProcEMT.CommCAL, GroupEMT, &ProcEMT.CommEMT);			//形成电磁暂态通信域
		printSimMsg("Comm EMT is created!\n");
		MPI_Comm_size(ProcEMT.CommEMT,&s_CommMPIInfo.CommEMTId.NumProcALL);
		MPI_Comm_rank(ProcEMT.CommEMT,&s_CommMPIInfo.CommEMTId.MyId);
	}
	else
	{
		ProcEMT.CommEMT=MPI_COMM_NULL;
	}
	printSimMsg("Number of EMT proc: %d\n", s_CommMPIInfo.CommEMTId.NumProcALL);

	//电磁暂态子网通信体（仅电磁暂态程序本身）
	int NumProcEMTCAL = 0;								//电磁暂态子网数
	for (i=0; i<s_CommMPIInfo.CommMPIId.NumProcALL; i++)
	{
		if (SIMUTYPE_EMTSUB == IDataRecv[i])
		{
			PCbuffer[NumProcEMTCAL] = i-NumProcST;		//该子网进程在电磁暂态进程组中的标识号，默认不同类型进程的启动顺序与COMMSIMU.PMN中定义的顺序一致
			NumProcEMTCAL = NumProcEMTCAL + 1;
		}
	}
	if (NumProcEMTCAL>0)
	{
		//将已存在的进程组GroupEMT中NumProcEMTCAL个的生成新的进程组GroupEMTCAL
		MPI_Group_incl(GroupEMT, NumProcEMTCAL, PCbuffer, &GroupEMTCAL);		//形成电磁暂态子网进程
		MPI_Comm_create(ProcEMT.CommEMT, GroupEMTCAL, &ProcEMT.CommEMTCAL);	//形成电磁暂态子网通信体
		printSimMsg("Comm EMT is created!\n");
#ifdef _MPI_SHM_RPOXY
		//start to increase the info of the CommEMTCal
		int iSubId = g_ParInfo.iKsub;
		MPI_Allgather( &iSubId, 1, MPI_INTEGER, IDataRecv, 1, MPI_INTEGER, &ProcEMT.CommEMTCAL);
		MPI_Group GroupEMTCAL2 = GroupEMTCAL;
		MPI_Group_incl(GroupEMTCAL2, NumProcEMTCAL, IDataRecv, &GroupEMTCAL);		//形成电磁暂态子网进程
		MPI_Comm CommEMTCAL = ProcEMT.CommEMTCAL;
		MPI_Comm_create(CommEMTCAL, GroupEMTCAL, &ProcEMT.CommEMTCAL);	//形成电磁暂态子网通信体
		MPI_Group_free(&GroupEMTCAL2);
#endif
		MPI_Comm_size(ProcEMT.CommEMTCAL,&s_CommMPIInfo.CommEMTCALId.NumProcALL);
		MPI_Comm_rank(ProcEMT.CommEMTCAL,&s_CommMPIInfo.CommEMTCALId.MyId);
		g_ParInfo.iNsub = s_CommMPIInfo.CommEMTCALId.NumProcALL;
#ifndef _MPI_SHM_RPOXY
		g_ParInfo.iKsub = s_CommMPIInfo.CommEMTCALId.MyId + 1;
		cout<<"g_ParInfo.iKsub = "<<g_ParInfo.iKsub<<endl;
#endif
	}
	else
	{
		ProcEMT.CommEMTCAL=MPI_COMM_NULL;
	}
	printSimMsg("Number of EMTCAL proc: %d\n", s_CommMPIInfo.CommEMTCALId.NumProcALL);

	//释放进程组
	MPI_Group_free(&GroupAll);
	if(NumProcCAL>0) MPI_Group_free(&GroupCAL);
	if(NumProcST>0) MPI_Group_free(&GroupST);
	if(NumProcEMT>0) MPI_Group_free(&GroupEMT);
	if(NumProcEMTCAL>0) MPI_Group_free(&GroupEMTCAL);
	//	if(NumProcEMTFPGA>0) MPI_Group_free(&GroupEMTFPGA);

	printSimMsg("Form communicator success!\n");

	delete[] IDataRecv;
	delete[] PCbuffer;

	return 0;
}


//************************************************************************
//*	函数名：FormEMTGrpComm
//*	输入：
//*	功能：
//*		形成EMT子网通讯组。
//*	修改：
//*		2007-01-15	15:14:00	by ZhangXing
//************************************************************************/
int FormEMTGrpComm()
{
	int *IDataRecv;
	int *IDataRecv_Map;
	int *PCbuffer;
	int i, Ntype;
	MPI_Group GroupEMTCAL,GroupEMTCALGrp;		//全部进程组、机电暂态进程组、电磁暂态进程组、电磁暂态子网进程组、计算进程组


	IDataRecv = new int[s_CommMPIInfo.CommEMTCALId.NumProcALL];
	IDataRecv_Map = new int[s_CommMPIInfo.CommEMTCALId.NumProcALL];
	PCbuffer = new int[s_CommMPIInfo.CommEMTCALId.NumProcALL];

	s_CommMPIInfo.CommEMTCALGrpId.NumProcALL = 0;
	s_CommMPIInfo.CommEMTCALGrpId.MyId = 0;

	printSimMsg("******Form EMTGroup comm() begin******\n");
	//----- 形成电磁暂态计算通信体
	if (s_CommMPIInfo.CommEMTCALId.NumProcALL == 0) return 0;
	//每一个CommEMTCAL进程都发送自己的控制号，并收集所有进程的控制号	
	if (g_ParInfo.iIsKsub_EMTCon != 1) Ntype = 0;
	else Ntype = EMT_CON_KSUB;
	MPI_Allgather( &Ntype, 1, MPI_INTEGER, IDataRecv, 1, MPI_INTEGER, ProcEMT.CommEMTCAL);
	for (i=0; i<s_CommMPIInfo.CommEMTCALId.NumProcALL; i++)
	{     		
		if (IDataRecv[i]==EMT_CON_KSUB)
		{
			g_ParInfo.iKsub_EMTCon = i+1;
		}
	}

	//每一个CommEMTCAL进程都发送自己的组号，并收集所有进程的组号
	if (g_ParInfo.iIssubGrp_EMTCon == 1)
	{
		Ntype=-g_ParInfo.iKGrp;   //10.14 修改为计算进程
	}
	else
	{
		Ntype=g_ParInfo.iKGrp;   //10.14 修改为计算进程
	}
	MPI_Comm_group(ProcEMT.CommEMTCAL, &GroupEMTCAL);
	// 每个CommEMTCAL进程都发送自己的组号，并收集所有进程的组号
	MPI_Allgather( &Ntype, 1, MPI_INTEGER, IDataRecv, 1, MPI_INTEGER, ProcEMT.CommEMTCAL);

	// 每个CommEMTCAL进程都发送自己的在MPI_COMM_WORLD中的ID号，并收集所有进程的组号
	MPI_Allgather( &g_ParInfo.iMYID, 1, MPI_INTEGER, IDataRecv_Map, 1, MPI_INTEGER, ProcEMT.CommEMTCAL);

	// 计算进程通信体

	//数组排序，并形成新的映射
	//SortArrayInt(IDataRecv,s_CommMPIInfo.CommEMTCALId.NumProcALL,IDataRecv_Map);

	int NumGrp = 0;
	int NumGrpSub = 0;
	for (i=0;i<=g_ParInfo.iNGrp;i++)
	{
		g_ParInfo.iKsubGrp_EMTConArray[i]= -1;  //初始化的时候初始化为-1；
	}

	for (i=0; i<s_CommMPIInfo.CommEMTCALId.NumProcALL; i++)
	{ 
		if (IDataRecv[i] < 0)
		{
			g_ParInfo.iKsubGrp_EMTConArray[(int)floor(abs(IDataRecv[i])+1.0e-6)] = IDataRecv_Map[i];
		}
	}
	for (i=0; i<s_CommMPIInfo.CommEMTCALId.NumProcALL; i++)
	{     		
		if (abs(IDataRecv[i])==g_ParInfo.iKGrp)
		{
			PCbuffer[NumGrpSub] = i;
			NumGrpSub = NumGrpSub + 1;
			if (IDataRecv[i]==-g_ParInfo.iKGrp)
			{
				g_ParInfo.iKsubGrp_EMTCon = NumGrpSub;
			}
		}
	}
	if (NumGrpSub>0)
	{
		MPI_Group_incl(GroupEMTCAL, NumGrpSub, PCbuffer, &GroupEMTCALGrp);		//形成计算进程组
		MPI_Comm_create(ProcEMT.CommEMTCAL, GroupEMTCALGrp, &ProcEMT.CommEMTCALGrp);	//形成计算通信域
		printSimMsg("Comm EMTCALGrp is created!\n");
		MPI_Comm_size(ProcEMT.CommEMTCALGrp,&s_CommMPIInfo.CommEMTCALGrpId.NumProcALL);
		MPI_Comm_rank(ProcEMT.CommEMTCALGrp,&s_CommMPIInfo.CommEMTCALGrpId.MyId);
		g_ParInfo.iNSubGrp = s_CommMPIInfo.CommEMTCALGrpId.NumProcALL;
		g_ParInfo.iKsubGrp = s_CommMPIInfo.CommEMTCALGrpId.MyId + 1;

		//存储子网组内所有子网对应的子网号
		g_ParInfo.iGrpSubArray = new int [NumGrpSub+1];
		g_ParInfo.isub_tocon_ratio = new int [NumGrpSub+1]; //同子网组内子网间通讯的速率;
		g_ParInfo.isub_commInd = new int [NumGrpSub+1];     //该子网对同子网组内其他子网的通讯标志
		g_ParInfo.isub_commSend = new int [NumGrpSub+1];    //该子网对同子网组内其他子网的发送标志
		g_ParInfo.iSub_mstep = new int [NumGrpSub+1];

		for (i = 1;i<=NumGrpSub;i++)
		{
			g_ParInfo.iGrpSubArray[i]=PCbuffer[i-1]+1;
		}
	}
	else
	{
		ProcEMT.CommEMTCALGrp = MPI_COMM_NULL;
		g_ParInfo.iNSubGrp = 0;
		g_ParInfo.iKsubGrp = 0;
	}
	printSimMsg("Number of EMTCALGrp proc: %d\n", s_CommMPIInfo.CommEMTCALGrpId.NumProcALL);

	//释放进程组
	if(s_CommMPIInfo.CommEMTCALId.NumProcALL>0)MPI_Group_free(&GroupEMTCAL);
	if(NumGrpSub>0) MPI_Group_free(&GroupEMTCALGrp);
	//	if(NumProcEMTFPGA>0) MPI_Group_free(&GroupEMTFPGA);

	printSimMsg("Form Group communicator success!\n");

	delete[] IDataRecv;
	delete[] IDataRecv_Map;
	delete[] PCbuffer;

	return 0;
}

////************************************************************************
////*	函数名：MF_InitMPIFIFO
////*	输入：
////*	功能：
////*		在通信域icomm上初始化MPIFIFO。
////*	修改：
////*		2007-01-15	15:14:00	by ZhangXing
////************************************************************************/
//int MF_InitMPIFIFO(MPI_Comm icomm)
//{
//	int* iIOProcNo;
//
//	int IOMark,SimProcNum;
//
//	//初始化IOProcNo数组
//	iIOProcNo = new int[s_CommMPIInfo.CommMPIId.NumProcALL];
//	int NumProcALL = s_CommMPIInfo.CommMPIId.NumProcALL;
//	int myid = s_CommMPIInfo.CommMPIId.MyId;
//	for (int i=0; i<NumProcALL;i++)
//	{
//		iIOProcNo[i]=-1;
//	}
//
//	int Ierrno = MF_ReadCfgFile(iIOProcNo);
//
//	if(Ierrno==0)
//	{//stop '读取MPIFIFO配置文件出错!'
//
//		if(iIOProcNo[myid]==myid)
//		{//当前进程为IO进程，搜索其对应的计算进程
//			IOMark=1;
//			SimProcNum=0;
//			for (int i=0; i<NumProcALL;i++)
//			{					
//				if((iIOProcNo[i]==myid)&&(i!=myid)) SimProcNum=SimProcNum+1;
//			}
//			int* VsimProc = new int[SimProcNum];
//			int j = 1;
//			for (int i=0; i<NumProcALL;i++)
//			{	
//				if((iIOProcNo[i]==myid)&&(i!=myid))
//				{
//					VsimProc[j]=i;
//					j=j+1;
//				}
//			}
//
//			cout<<"IO Proc"<<myid<<SimProcNum<<endl;
//		}
//		else                         //当前进程为计算进程，记录其对应的IO进程
//		{
//			int PROCIO=iIOProcNo[myid];
//			cout<<"Calc Proc"<<myid<<PROCIO;
//		}
//	}
//	return 0;
//}
//
////************************************************************************
////*	函数名：MF_ReadCfgFile()
////*	输入：
////*	功能：
////*		在通信域icomm上初始化MPIFIFO。
////*	修改：
////*		2007-01-15	15:14:00	by ZhangXing
////************************************************************************/
//int MF_ReadCfgFile(int* iIOProcNo)
//{
//	FixedString strline;
//	FixedString sInPath[MAXSUB+1];
//		int ISO;
//		int i = 0;
//		int proc1,proc2;
//
//		ISO=0;
//		int Ierrno=0;
//
//		FILE *fp = NULL;
//		char *pcFileName = NULL;
//		char *pcnext;
//
//		cout<< "Proc"<<s_CommMPIInfo.CommMPIId.MyId<<": MF_ReadCfgFile start ---------------"<<endl;
//		
//		pcFileName = "MPIFIFO.CFG";
//		fp = fopen(pcFileName,"r");
//		if( !fp )
//		{
//			////dealSimMsg(FN_IN,0,ERROR_FILEOPEN,WS_FATAL,DOUT_SCREEN,"Input data file alloc_in.pth ");
//			//ErrorTmp.iENo   = ERROR_FILEOPEN;
//			//ErrorTmp.iECTyp = FN_IN;
//			//ErrorTmp.iECNo  = 0;
//			//setSimError(&ErrorTmp);
//			return 1;
//		}
//		else
//		{
//			//g_ParInfo.iNsub=getFileLineNum(fp);
//		}
//
//		fgets(strline,MAXSTRLEN,fp);
//
//		i = 1;
//
//		// strtok_uni以逗号为分隔符读取变量，moveStrTokens移除字符串首末端空格和单引号
//		strcpy(sInPath[i],moveStrTokens(strtok_uni(strline,",",&pcnext)));
//			int iT = atoi(strtok_uni(NULL,",",&pcnext));
//			int iDT =atoi(strtok_uni(NULL,",",&pcnext));
//			int iNPCCAL =atoi(strtok_uni(NULL,",",&pcnext));
//			int iNPCIO =atoi(strtok_uni(NULL,",",&pcnext));
//			int iLenBuff =atoi(strtok_uni(NULL,",",&pcnext));
//			int iCIP =atoi(strtok_uni(NULL,",",&pcnext));
//			int inport =atoi(strtok_uni(NULL,",",&pcnext));
//			i++;
//    
//		if(iNPCIO<=0)
//		{
//			Ierrno=2;
//			return 1;
//		}
//
//		while( !feof(fp) )
//		{
//		fgets(strline,MAXSTRLEN,fp);
//
//		// strtok_uni以逗号为分隔符读取变量，moveStrTokens移除字符串首末端空格和单引号
//		strcpy(sInPath[i],moveStrTokens(strtok_uni(strline,",",&pcnext)));
//					int proc1 = atoi(strtok_uni(NULL,",",&pcnext));
//			int proc2 = atoi(strtok_uni(NULL,",",&pcnext));
//			iIOProcNo[proc1+1]=proc2;
//			iIOProcNo[proc2+1]=proc2;
//
//			i++;
//		}
//		fclose(fp);
//			return 0;
//}
#endif


