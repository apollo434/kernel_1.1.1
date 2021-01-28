#include "mpi.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <iostream>
#include <algorithm>
#include <functional>
#include <iterator>

#define RECVBUFFSIZE 100000
#define RECVPIDSIZE 50000
#define MAXPROCNUM 1000
#include "MPI_Comm_def.h"
#include "IOComm_Proxy.h"
using namespace std;

//char IOSendbuff[RECVBUFFSIZE];
//char* IOSendbuffptr;
//char IORecvbuff[RECVBUFFSIZE];
//char* IORecvbuffptr;
//int RecvPackageNum;
//MPICOMM_PACKAGE PIDRecvPackage[MAXRECVPACKAGENUM];  //PID接收的Message位置认为是固定且定长的
extern MPICommInfo g_MPICommInfo;
//char* ADPSSinterfaceSendBuff[RECVBUFFSIZE];
//IOBuffInfo SendIO[MAXSOCKETCPU];   //可以动态分配

extern BuffInfo *MPISendBuff;
extern BuffInfo *MPIRecvBuff[MAXSOCKETCPU];

extern vector<vector<MPICOMM_PACKAGE> > *MPIMessage;
MPI_Request *PIDreqs; //物理进程的MPI状态
MPI_Request *PIDrq;

//MPICOMM_PACKAGE MpCon_EMT;
//
////内存空间
//MPICOMM_PACKAGE MpCon_IO[MAXSOCKETCPU];
//MPICOMM_PACKAGE MpMon_IO[MAXSOCKETCPU];
BuffInfo PIDRecvInfo[MAXRECVPACKAGENUM];
BuffInfo PIDSendInfo[MAXRECVPACKAGENUM];
BuffInfo MpCon_IO[MAXSOCKETCPU];
BuffInfo MpCon_EMT;
BuffInfo MpMon_IO[MAXSOCKETCPU];
BuffInfo MpMon_EMT;
MPICOMM_PACKAGE MpMon_Message[MAXSOCKETCPU];

vector<MessagePos> PIDMessagePos;
vector<MessagePos> DIDMessagePos;
int PIDRank[MAXPROCNUM]; //记录PIDRank的信息
int PIDJob[MAXPROCNUM];

void BuffInfo::allocate_mem(int size, int mode)
{
	this->startaddress = calloc(size, sizeof(char));
	if (!this->startaddress)
	{
		this->currentaddress = this->startaddress;
		this->MaxLength = size;
		this->mode = mode;
	}
	else
	{
		//错误处理，无法分配内存
	}
}

void BuffInfo::reallocate_mem(int size, int mode)
{
	void *newaddress = calloc(size, sizeof(char));
	if (!newaddress)
	{
		memcpy(newaddress, this->startaddress, this->length);
		this->currentaddress = (char*) newaddress + ((char*)this->currentaddress - (char*)this->startaddress);
		this->startaddress = newaddress;
		this->MaxLength = size;
	}
	else
	{
		//错误处理，无法分配内存
	}
}

void BuffInfo::elem_pushback(void *databuff, int length)
{
	int lengthtmp = this->length + length;
	if (lengthtmp < MaxLength)
	{
		memcpy(this->currentaddress, databuff, length);
		this->currentaddress = &((char *)this->currentaddress)[length];
		this->iNum++;
		this->length = lengthtmp;
	}
	else if (mode == BUFFALLOCATEABLE)
	{
		reallocate_mem(lengthtmp + 1, mode);
		memcpy(this->currentaddress, databuff, length);
		this->currentaddress = &((char *)this->currentaddress)[length];
		this->iNum++;
		this->length = lengthtmp;
	}
	else
	{
		//错误处理，待补
	}
}

void BuffInfo::elem_pushbackwithoutaddiNum(void *databuff, int length)
{
	this->length += length;
	if (length < MaxLength)
	{
		memcpy(this->currentaddress, databuff, length);
		this->currentaddress = &((char *)this->currentaddress)[length];
	}
	else if (mode == BUFFALLOCATEABLE)
	{
		reallocate_mem(length + 1, mode);
		memcpy(this->currentaddress, databuff, length);
		this->currentaddress = &((char *)this->currentaddress)[length];
	}
	else
	{
		//错误处理，待补
	}
}

void BuffInfo::MessageExtract(int iProc, vector<MPICOMM_PACKAGE> &vecRecvPage)
{
	char *Recvdataptr;
	int i = 0;
	int HeadRead;
	MPICOMM_PACKAGE RecvPackage;
	Recvdataptr = (char *)startaddress;
	int ExtractLength = 0;

	while (ExtractLength < this->length)
	{
		HeadRead = *(int *)Recvdataptr;
		if (HeadRead != 0xFFFF0000)
		{
			//解帧错误
			//ErrorStru Errtmp = {0};
			printf("Receive Package %d Header is broken.\n", i);
			break;
		}
		else
		{
			RecvPackage.header = HeadRead;
			RecvPackage.source = ((int *)Recvdataptr)[1];
			RecvPackage.dest = ((int *)Recvdataptr)[2];
			RecvPackage.length = ((int *)Recvdataptr)[3];
			RecvPackage.typ = ((int *)Recvdataptr)[4];
			RecvPackage.tag = ((int *)Recvdataptr)[5];
			RecvPackage.comm = ((MPI_Comm *)Recvdataptr)[6];
			Recvdataptr += FRAMEHEADERLEN;
			RecvPackage.dataptr = Recvdataptr;
			Recvdataptr += RecvPackage.length;
			vecRecvPage.push_back(RecvPackage);
		}
		i++;
		ExtractLength += RecvPackage.length + FRAMEHEADERLEN;
	}

	int IORecvPackageNum = i; //记录总帧数
	if (IORecvPackageNum > MAXRECVPACKAGENUM)
	{
		//报数太多了
		//ErrorStru Errtmp = {0};
		printf("Receive Packages is more than the maximum %d", MAXRECVPACKAGENUM);
	}
	return;
}

void BuffInfo::RecvBuffFlush()
{
	iNum = (*buffaddress).iNum;
	length = (*buffaddress).length;
	MaxLength = (*buffaddress).MaxLength;
	mode = (*buffaddress).mode;         //普通模式，绑定	
}

void BuffInfo::clear()
{
	iNum = 0;
	length = 0;
	currentaddress = startaddress;
	//MaxLength = (*buffaddress).MaxLength;
	//mode = (*buffaddress).mode;         //普通模式，绑定	
}

//************************************
// Method:    PackageDivision
// FullName:  PackageDivision
// Access:    public
// Returns:   void
// Qualifier: //接收以后，统一分割报文
// Auther:    Qing Mu
// Data:      2019/07/17
//************************************
void PackageDivision(int step) //接收以后，统一分割报文
{
	if (step <= MPIBUFFBLOCKNUM) //MPIMessage每一个都要初始化
	{
		MPICOMM_PACKAGE tmp;
		vector<MPICOMM_PACKAGE> MPITmp(20, tmp);
		(*MPIMessage).resize(g_MPICommInfo.iNumCommProc, MPITmp);
	}
	if (g_MPICommInfo.iMYID_SocketComm == g_MPICommInfo.iProcIO_SocketComm) //IO进程需要对每一个内存都进行报文分割
	{
		for (int i = 0; i < g_MPICommInfo.iNumCommProc; i++)
		{
			if (i != g_MPICommInfo.iMYID_SocketComm)
			{
				//1. MPIMessage clear 内存
				(*MPIMessage)[i].clear();
				//2. 读取缓存区内的长度和其他信息
				(*MPIRecvBuff[i]).RecvBuffFlush();
				//3. Message分离
				(*MPIRecvBuff[i]).MessageExtract(i, (*MPIMessage)[i]);
			}
		}
	}
	else //EMT进程只对IO进程进行报文分割
	{
		if (step <= MPIBUFFBLOCKNUM)
		{
			for (int i = 0; i < g_MPICommInfo.iNumCommProc; i++)
			{
				if (i != g_MPICommInfo.iMYID_SocketComm)
				{
				//1. MPIMessage clear 内存
				(*MPIMessage)[i].clear();
				//2. 读取缓存区内的长度和其他信息
				(*MPIRecvBuff[i]).RecvBuffFlush();
				//3. Message分离
				(*MPIRecvBuff[i]).MessageExtract(i, (*MPIMessage)[i]);
				}
			}
		}
#if 0		
		(*MPIRecvBuff[g_MPICommInfo.iProcIO_SocketComm]).MessageExtract(g_MPICommInfo.iProcIO_SocketComm,(*MPIMessage)[g_MPICommInfo.iProcIO_SocketComm]);
#endif
	}
	return;
}
#if 0
void RecvPackageExtract(MessagePos& RecvPos,char* databuff,int len,int proc_source ,int tag, MPI_Comm comm,int typ)//消息搜集
{
	if (g_SimSt.iKloop == 1)     //初始化时确定报文的位置
	{
		MPICOMM_PACKAGE RecvPackage;
		int ierr = RecvPackage.checkandrecv(databuff,len,proc_source,tag,comm,typ,RecvPos);
	}
	else
	{
		(*MPIMessage)[RecvPos.iProc][RecvPos.iPackage].recv(databuff,len,proc_source,tag,comm);
	}
}


/////////////////////////////////////IO 进程的数据处理///////////////////////////////////////////////
void ADPSSInterfaceMessagePosition(vector<MessagePos>&idx,int typ)
{
	idx.clear();
	MessagePos tmp;
	for (int i = 0; i <(*MPIMessage).size(); i++)
	{
		//PackageDivision(); //接收以后，统一分割报文
		for (int j=0; j<(*MPIMessage)[i].size();j++)
		{
			if ((*MPIMessage)[i][j].typ == typ)
			{
				tmp.iProc = i;
				tmp.iPackage = j;
			}
			idx.push_back(tmp);
		}
	}
	return;
}


void ADPSSInterfaceDispatch()  //各子网传输到IO进程的信号，进行分拆分配并找到发送的物理接口的进程号 RecvPackage[i].dest 对侧的进程号
{
	vector<MessagePos>::iterator iterPIDMessagePos;
	if (g_SimSt.iKloop == 1)   //初始化时
	{
		ADPSSInterfaceMessagePosition(PIDMessagePos,PIDInfo);
		ADPSSInterfaceMessagePosition(DIDMessagePos,DIDInfo);
		memset(&PIDRank[0],0,MAXPROCNUM*sizeof(int));
		memset(&PIDJob[0],0,MAXPROCNUM*sizeof(int));

		int iPIDProcNum = 0;
		for (iterPIDMessagePos=PIDMessagePos.begin(); iterPIDMessagePos!= PIDMessagePos.end();iterPIDMessagePos++)
		{
			int SendPIDProc = (*MPIMessage)[iterPIDMessagePos->iProc][iterPIDMessagePos->iPackage].dest;
			int* tmp=find(&PIDJob[0],&PIDJob[iPIDProcNum],SendPIDProc);
			if (tmp != &PIDJob[iPIDProcNum])
			{
				iPIDProcNum++;
				PIDRank[SendPIDProc] = iPIDProcNum;
				PIDJob[iPIDProcNum] = SendPIDProc;
			}
			else
			{
				int iNum = tmp - &PIDJob[0];  //记录是第几块内存
				PIDRank[SendPIDProc] = iNum;
			}
		}
		for (iterPIDMessagePos=DIDMessagePos.begin(); iterPIDMessagePos!= DIDMessagePos.end();iterPIDMessagePos++)
		{
			int SendDIDProc = (*MPIMessage)[iterPIDMessagePos->iProc][iterPIDMessagePos->iPackage].dest;
			int* tmp=find(&PIDJob[0],&PIDJob[iPIDProcNum],SendDIDProc);
			if (tmp != &PIDJob[iPIDProcNum])
			{
				iPIDProcNum++;
				PIDRank[SendDIDProc] = iPIDProcNum;
				PIDJob[iPIDProcNum] = SendDIDProc;
			}
			else
			{
				int iNum = tmp - &PIDJob[0];  //记录是第几块内存
				PIDRank[SendDIDProc] = iNum;
			}
		}
		g_MPICommInfo.iNumADPSSInterface = iPIDProcNum;
		PIDreqs = (MPI_Request*) calloc(iPIDProcNum * 2 + 1,sizeof(MPI_Request));  //Isend + Irecv
		PIDrq = PIDreqs;
	}

	//1.根据报文类型和Dest口整理发送数据
	for (iterPIDMessagePos=PIDMessagePos.begin(); iterPIDMessagePos!= PIDMessagePos.end();iterPIDMessagePos++)
	{
		int SendPIDProc = (*MPIMessage)[iterPIDMessagePos->iProc][iterPIDMessagePos->iPackage].dest;
		(*MPIMessage)[iterPIDMessagePos->iProc][iterPIDMessagePos->iPackage].sendanyBuff(PIDSendInfo[PIDRank[SendPIDProc]]);
	}
	for (iterPIDMessagePos=DIDMessagePos.begin(); iterPIDMessagePos!= DIDMessagePos.end();iterPIDMessagePos++)
	{
		int SendDIDProc = (*MPIMessage)[iterPIDMessagePos->iProc][iterPIDMessagePos->iPackage].dest;
		(*MPIMessage)[iterPIDMessagePos->iProc][iterPIDMessagePos->iPackage].sendanyBuff(PIDSendInfo[PIDRank[SendDIDProc]]);
	}

	//1. 物理进程收发数据
	for (int i = 0; i < g_MPICommInfo.iNumADPSSInterface; i++)
	{
		int source = PIDJob[i];
		MPI_Irecv(PIDRecvInfo[i].startaddress, PIDRecvInfo[i].length,MPI_BYTE,source,DEFAULTMPITAG, MPI_COMM_WORLD, PIDrq++);
	}
	for (int i = 0; i < g_MPICommInfo.iNumADPSSInterface; i++)
	{
		int dest = PIDJob[i];
		MPI_Isend(PIDSendInfo[i].startaddress, PIDSendInfo[i].length,MPI_BYTE,dest,DEFAULTMPITAG, MPI_COMM_WORLD, PIDrq++);
	}
}

void ADPSSInterfaceGather()  //物理进程发送到IO进程的信号，进行分拆并注入到需要发送的队列 SendIO[SendMPIProc].Package.dest 对侧的进程号
{
	int HeadRead;
	MPICOMM_PACKAGE RecvPackage;
	//1.等待接收物理进程的信号
	MPI_Waitall(g_MPICommInfo.iNumADPSSInterface, PIDreqs, MPI_STATUS_IGNORE); //等待所有SendRecv完成
	PIDrq = PIDreqs;

	//假设每一个与核交互的Package的位置已经明确了。
	for (int i = 0; i < g_MPICommInfo.iNumADPSSInterface; i++)
	{
		//PackageDivision(); //接收以后，统一分割报文
#if 0
		int RecvPackageNum = *(int*)&PIDRecvInfo[i].startptr;  //内存的第一个地址写入总帧数（int数）
		int DispatchPackageNum = 0;
		do 
		{
			HeadRead = *(int*)PIDRecvInfo[i].currentaddress;
			if (HeadRead!=0xFFFF0000)
			{
				//解帧错误
				ErrorStru Errtmp = {0};
				printf("Receive Package %d Header is broken.\n",i);
				break;
			}
			else
			{
				RecvPackage.header = HeadRead;
				RecvPackage.source = ((int*)PIDRecvInfo[i].currentaddress)[1];
				RecvPackage.dest = ((int*)PIDRecvInfo[i].currentaddress)[2];
				RecvPackage.length = ((int*)PIDRecvInfo[i].currentaddress)[3];
				RecvPackage.typ = ((int*)PIDRecvInfo[i].currentaddress)[4];
				RecvPackage.tag = ((int*)PIDRecvInfo[i].currentaddress)[5];
				RecvPackage.comm = ((MPI_Comm*)PIDRecvInfo[i].currentaddress)[6];
				PIDRecvInfo[i].currentaddress = (char*)PIDRecvInfo[i].currentaddress + FRAMEHEADERLEN;
				RecvPackage.dataptr = PIDRecvInfo[i].currentaddress;
				//PIDRecvInfo[i].currentaddress = (char*)PIDRecvInfo[i].currentaddress + RecvPackage.length;
				RecvPackage.send(PIDRecvInfo[i].currentaddress,RecvPackage.length,RecvPackage.dest,RecvPackage.tag,RecvPackage.comm,PIDInfo);
				//MPICOMM_PACKAGE_Pushback(SendIO[SendMPIProc].Package[SendIO[SendMPIProc].iNum].dest,SendIO[SendMPIProc].iNum - 1);  //数据装载到发送内存区
			}
			DispatchPackageNum++;
		}while(i<DispatchPackageNum);
#else
		(*MPISendBuff).elem_pushback(PIDRecvInfo[i].startaddress,PIDRecvInfo[i].length);
#endif
	}
}

//void MPICOMM_PACKAGE_Pushback(int SendMPIProc, int LastNum)
//{
//	int CurrentNum = LastNum + 1;
//	char* currentptr = SendIO[SendMPIProc].Package[LastNum].dataptr+SendIO[SendMPIProc].Package[LastNum].length;
//	memcpy(currentptr,&SendIO[SendMPIProc].Package[CurrentNum],FRAMEHEADERLEN);
//	currentptr += FRAMEHEADERLEN;
//	memcpy(currentptr,SendIO[SendMPIProc].Package[CurrentNum].dataptr,SendIO[SendMPIProc].Package[CurrentNum].length);
//	currentptr += SendIO[SendMPIProc].Package[CurrentNum].length;
//}

//************************************
// Method:    MpConBuffInfo
// FullName:  MpConBuffInfo
// Access:    public 
// Returns:   void
// Qualifier: //buff初始化
// Auther:    Qing Mu
// Data:      2019/07/22
//************************************
void MpConBuffInfo() //buff初始化
{
	int buffsize = floor(MAXMPCONCHN*sizeof(StruConInfo)*1.1);
	for (int i = 0; i < MAXSOCKETCPU; i++)
	{
		MpCon_IO[i].allocate_mem(buffsize,BUFFMODE); //默认的Con的缓冲区大小为10e3
	}
	return;
}

//************************************
// Method:    MpConDispatch
// FullName:  MpConDispatch
// Access:    public 
// Returns:   void
// Qualifier: //变量拆分的函数
// Parameter: StruConInfo MpCon
// Auther:    Qing Mu
// Data:      2019/07/20
//************************************
void MpConDispatch(StruConInfo MpCon) //变量拆分的函数
{
	MPICOMM_PACKAGE MPIPackageTmp;
	int Id[MAXSOCKETCPU] = {0};
	if (g_SimSt.iKloop == 1)
	{
		MpConBuffInfo();
	}
	for(int i=0;i<MpCon.VarNum;i++)
	{
		int SendMPIProc  = g_MPICommInfo.rank_socketcomm[MpCon.Var[i].Ksub];
		MpCon_IO[SendMPIProc].elem_pushback(&MpCon.Var[i],sizeof(StruConInfo));
		MpCon.Var[i].Val=g_SimSt.dTnow; //控制生效的时刻
		Id[SendMPIProc] = 1;   //表示已经有新信息了。
	}
	for (int i = 0; i < MAXSOCKETCPU; i++)
	{
		if (Id[i] == 1)
		{
			MPIPackageTmp.send(MpCon_IO[i].startaddress,i,MpCon_IO[i].length,MPCONInfo,DEFAULTMPITAG,g_MPICommInfo.SocketComm); //SocketComm 通讯域
		}
	}
	return;
}

//************************************
// Method:    MpMonBuffInfo
// FullName:  MpMonBuffInfo
// Access:    public 
// Returns:   void
// Qualifier: //buff初始化
// Auther:    Qing Mu
// Data:      2019/07/22
//************************************
void MpMonBuffInfo() //buff初始化
{
	int buffsize = floor(MAXMPMONCHN*sizeof(StruConInfo)*1.1);
	for (int i = 0; i < MAXSOCKETCPU; i++)
	{
		MpMon_IO[i].allocate_mem(buffsize,BUFFMODE); //默认的Con的缓冲区大小为10e3
	}
	return;
}

//************************************
// Method:    MpMonDispatch
// FullName:  MpMonDispatch
// Access:    public 
// Returns:   void
// Qualifier: //变量拆分的函数
// Parameter: StruMonInfo MpMon
// Auther:    Qing Mu
// Data:      2019/07/20
//************************************
void MpMonDispatch(StruMonInfo MpMon) //变量拆分的函数
{
	MPICOMM_PACKAGE MPIPackageTmp;
	int Id[MAXSOCKETCPU] = {0};
	if (g_SimSt.iKloop == 1)
	{
		MpConBuffInfo();
	}
	for(int i=0;i<MpMon.VarNum;i++)
	{
		int SendMPIProc  = g_MPICommInfo.rank_socketcomm[MpMon.Var[i].Ksub];
		MpMon.Var[i].Val = MpMon_IO[SendMPIProc].length/sizeof(StruMonInfo);  //先把索引的位置储处在MpMon.Var[i].Val上。
		MpMon_IO[SendMPIProc].elem_pushback(&MpMon.Var[i],sizeof(StruConInfo));
		Id[SendMPIProc] = 1;   //表示已经有新信息了。
	}
	for (int i = 0; i < MAXSOCKETCPU; i++)
	{
		if (Id[i] == 1)
		{
			MPIPackageTmp.send(MpCon_IO[i].startaddress,i,MpCon_IO[i].length,MPCONInfo,DEFAULTMPITAG,g_MPICommInfo.SocketComm); //SocketComm 通讯域
		}
	}
	return;
}



//************************************
// Method:    MpMonGather
// FullName:  MpMonGather
// Access:    public 
// Returns:   void
// Qualifier: //变量聚合的函数
// Parameter: StruMonInfo MpMon
// Parameter: StruMonRes & MpMonRes
// Auther:    Qing Mu
// Data:      2019/07/20
//************************************
void MpMonGather(StruMonInfo MpMon,StruMonRes& MpMonRes) //变量聚合的函数
{
	StruMonVar* pMonVar = NULL;
	//报文先找到位置
	for(int i=0;i<g_MPICommInfo.iNumEMTProc;i++)
	{
		int ierr = MpMon_Message[i].check(i,DEFAULTMPITAG,g_MPICommInfo.SocketComm,MPMONInfo);
		if (ierr != 0)
		{
			//错误处理
			return;
		}
	}
	for(int i=0;i<MpMon.VarNum;i++)
	{
		int SendMPIProc  = g_MPICommInfo.rank_socketcomm[MpMon.Var[i].Ksub];
		int idx = floor(MpMon.Var[i].Val + EPSINON);
		pMonVar = &((StruMonVar*)MpMon_Message[SendMPIProc].dataptr)[idx];
		MpMonRes.Val[i] = pMonVar->Val;
	}
	return;
}
#endif