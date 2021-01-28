
#define CalcNum			30
#define EMT_COMM_NUM		24


#define PRINT_DETAIL 0
#define _ADDTIMEBUFF


enum SIMUTYPE_ALL
{
	// 1~100Ϊ�������
	SIMUTYPE_STCTRL=1,		// ��������
	SIMUTYPE_STSUB,			// ��������
	SIMUTYPE_STUD,			// ����UDģ��
	SIMUTYPE_STMAT,			// ����Matlabģ��
	SIMUTYPE_STPHY,			// ���������ӿ�

	// 101~200Ϊ��Ž���
	SIMUTYPE_EMTSUB=101,	// �������
	SIMUTYPE_EMTUD,			// ���UDģ��
	SIMUTYPE_EMTMAT,		// ���Matlabģ��
	SIMUTYPE_EMTPHY,		// ��������ӿ�
	
	// 201����ΪIO����
	SIMUTYPE_STIO=201,		// ����IO����
	SIMUTYPE_EMTIO,			// ���IO����

	SIMUTYPE_GUI=301

};

// MPIͨ������Ϣ
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
} ProcInfo;


//#define SHM_EMTIO

#ifdef SHM_EMTIO
#define SHM_READ_COMPLETE	0x5A5A5A5A
#define SHM_WRITE_COMPLETE	0xA5A5A5A5

#define SHM_SIZE_PER_TASK   4096
#define SHM_BLOCK_SIZE		256
#define SHM_BLOCK_NUM		(SHM_SIZE_PER_TASK/SHM_BLOCK_SIZE)

struct shm_blk
{
	unsigned int sem;
	double data[EMT_COMM_NUM];
};

struct shm_per_task
{
	int shm_size;	/* shm size used by shmget */
	void *mem;		/* address of shm */
	int blk_num;	/* shm_size is divided into blk_num blocks */
	int blk_size;	/* size of each block */

	int prod_ind;	/* block index for producer task */
	int cons_ind;	/* block index for consumer task */
};

#endif

extern struct sockaddr_in;
extern struct hostent;

struct sockaddr_in serv_addr;
struct hostent *server;
int sockfd;

enum GSTATUS
{
	ginit=0,
	gstart=1,
	gstop=2
};
