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

extern MPICommInfo g_MPICommInfo;

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