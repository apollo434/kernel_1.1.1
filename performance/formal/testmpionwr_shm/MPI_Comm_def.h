#ifndef _MPI_COMM_DEF_H__
#define _MPI_COMM_DEF_H__
#include "Pub_Def.h"
#define MAXSOCKETCPU 50
#define MAXMPIPROC 512
#define MAXRECVPACKAGENUM 100
#define DEFAULTMPITAG 0   //Ĭ��û��Tag�����ݴ����Tag��־0
#define FRAMEHEADERLEN 28
#define MPIBUFFBLOCKNUM 2
//MPI��ʶ
struct SMPIID {
	int MyId;                           //������id
	int NumProcALL;                     //��������
}; 

//�����������
enum SIMUTYPE_ALL
{
	SIMUTYPE_STCTRL=1,
	SIMUTYPE_STSUB,
	SIMUTYPE_STUD,
	SIMUTYPE_STMAT,
	SIMUTYPE_STPHY,
	SIMUTYPE_EMTSUB=101,
	SIMUTYPE_EMTUD,
	SIMUTYPE_EMTMAT,
	SIMUTYPE_EMTPHY,
	SIMUTYPE_EMTDid=105,
	SIMUTYPE_EMTFPGA=111,
	SIMUTYPE_STIO=201,
	SIMUTYPE_EMTIO=202,
	SIMUTYPE_FPGA=191  //MuQ 10.18�޸�
};

//����ṹ���¼�뱾�����ӿڽ���ͨѶ�ļ��������Ϣ
struct SSIMPROCINFO {
	int Id;
	int nIn;
	int nOut;
	double Data[MAXRECV];
};



//������ͨѶ����ı�ʾ
typedef struct 
{
	struct SMPIID CommMPIId;
	struct SMPIID CommEMTId;
	struct SMPIID CommEMTCALId;
	struct SMPIID CommSTId;
	struct SMPIID CommCALId;
	struct SMPIID CommEMTCALGrpId;
//#ifdef _MULTIRATE_NEW
	struct SMPIID CommEMTCALGrp_TimelevelId[MAXTIMESTEP_LEVEL];
//#endif
	struct SMPIID CommEMTFPGA;
	struct SMPIID CommSOCKETCOMM;       //��Socket�ϵ����н���
	struct SMPIID CommSocketEMTCal;     //��Socket�ϵ�EMT����
}CommMPIInfo;


//MPI���̺�ͨ�����ʶ
typedef struct{
	//vector<SSIMPROCINFO> SimProc;		//�ͱ�����ͨ�ŵĵ���������̱�ʶ��
	MPI_Comm CommEMT;                   //�����̬ͨ����
	MPI_Comm CommEMTCAL;               //�����̬����ͨ����
	MPI_Comm CommST;                    //������̬ͨ����
	MPI_Comm CommCAL;
	//	MPI_Comm CommEMTFPGA;               //FPGA���������ͨѶ��
	MPI_Comm CommEMTCALGrp;             //���ͨѶ��������������
	MPI_Comm CommEMTCALGrp_TimeLevel[MAXTIMESTEP_LEVEL];  //�������ڵĲ�ͬʱ��߶ȵĽ�����
	//  ����
	MPI_Comm CommSocketEMTCal;
	//MPI_Comm SocketComm;
}MPI_Comm_Stru;

class MPICOMM_PACKAGE
{
public:
	int header;
	int source;
	int dest;
	int length;
	int typ;
	int tag;
	MPI_Comm comm;
	void* dataptr;
	//������MPIMessage�Ĳ�ѯƥ����
	//void ElementPushback(void* databuff,int length);
	void init(char* databuff,int dest ,int length,int typ,int tag, MPI_Comm comm);
	void send(void* databuff,int length,int dest ,int tag, MPI_Comm comm, int typ);
    void sendanyBuff();
	int recv(void* databuff,int length,int source,int tag, MPI_Comm comm);
	int checkandrecv(void* databuff,int length,int source,int tag, MPI_Comm comm,int typ, MessagePos& pos);
	int check(int source, int tag, MPI_Comm comm,int typ, MessagePos& pos);
	void copy(MPICOMM_PACKAGE&);
};

class MPICommInfo
{
public:
	int iNumEMTProc;
	int iNumCommProc;
	int iNumTotolProc;         //�ܽ�����
	int iNumADPSSInterface;    //��ʾ�ⲿ�����˼���ADPSS�Ľӿڽ���
	int job_socketcomm[MAXSOCKETCPU];     //MPI��SOCKET�ϵĽ��̵�ԭʼ���̺�(MPI_COMM_WORLD)
	int rank_socketcomm[MAXMPIPROC];      //MPIԭʼ���̶�Ӧ���µ�MPIͨѶ��SOCKET�ںţ�SOCKETCOMM��
	int rank_emtcalcomm[MAXMPIPROC];      //MPIԭʼ��emtcal���ж�Ӧ��SOCKET�ڲ���ͨѶ��
	int EMTsub_commworld[MAXMPIPROC];     //����MPI���̵ĵ��������
	int Socketid_commworld[MAXMPIPROC];   //����MPI������Ҫ�󶨵ĵ�Socketid
	int Proctyp_commworld[MAXMPIPROC];    //����MPI������Ҫ�󶨵Ľ�������
	int CPUid_commworld[MAXMPIPROC];      //����MPI������Ҫ�󶨵�CPU�˺�
	CommMPIInfo commMPIInfo;   //MPIInfo����Ϣ
	MPI_Comm_Stru commMPICommStru;
	MPI_Comm SocketComm;
	MPI_Win win;
	int iMYID_SocketComm;
	int iSocketid;
	int iCPUid;                //CPU��id��
	int iProcIO_SocketComm;    //��ʾIO������SocketComm�е�Rank��
	int iIsProcIO;
	int iProcIO_COMMWORLD;     //��ʾIO������MPICOMMWORLD�е�Rank��
	void* MPISendAddress;
	void* MPIRecvAddress[MAXSOCKETCPU];
	void* MPISendAddressMem[MPIBUFFBLOCKNUM];  //ԭʼ���ڴ�ӳ�����ʼ��ַ
	void* MPIRecvAddressMem[MPIBUFFBLOCKNUM][MAXSOCKETCPU]; //ԭʼ���ڴ�ӳ�����ʼ��ַ
};

#endif
