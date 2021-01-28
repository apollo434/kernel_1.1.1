#pragma once

#ifndef _MPI_COMM_H__
#define _MPI_COMM_H__

//#include "MPI_Comm_def.h"

#ifdef __cplusplus
extern "C" 
{
#endif

#define MPI_CommEMT 100
#define MPI_CommEMTCAL 101
#define MPI_CommST 102
#define MPI_CommCAL 103
#define MPI_CommEMTCALGRP 104
#define MPI_OutofComm -1    //�����̲�����ĳһ��ͨѶ��ʱ

#if 0
extern CommMPIInfo* getStaticCommMPI(); 

//MPI Base Function Initialization
extern void MPI_EMT_Start(int,int);

//�γ�ͨѶ��
extern int FormComm();
//�γ�EMT����ͨѶ��
extern int FormEMTGrpComm();

extern MPI_Comm returnMPI_Comm(int MPI_Typ);
#endif

extern int MPI_EMT_Send(void* sendbuf,void* recvbuff,int count,MPI_Datatype datatype,int iDest,int tag,MPI_Comm comm,MPI_Request* request);
extern void MPI_EMT_Recv(void* buf,int count,MPI_Datatype datatype,int iSource,int tag,MPI_Comm comm,MessagePos* RecvPos, int step);//MuQ 181215;

extern void MPI_Sharedmemory_init();
extern void MPI_Sync_Step(int step);
extern void BindBuff_step(int step);
extern void Translate_Ranks(MPI_Comm comm1, int n_partners, int *partners, int *partners_map, MPI_Comm comm2);
extern void MPI_Win_Lock();
extern void MPI_Win_unLock();
extern void MPI_EMTCAL_RANK(MPI_Comm comm1, int n_partners, int *partners);


#ifdef __cplusplus
};
#endif 

#endif