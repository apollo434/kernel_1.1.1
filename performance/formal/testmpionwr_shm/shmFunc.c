#include <stddef.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "shmdata.h"
#include "shmFunc.h"
//#define _MuQ_SHM_DEB
void p(int semid,int sem_sigid);
void v(int semid,int sem_sigid);

union semun{
        int val;
        struct semid_ds *buf;
        unsigned short *array;
}; 

int shm_create(int key, int *shmid, int mode, void **shm) //mode 0 ��ʾ���������ڴ� 1 ��ʾ��ѯ�����ڴ� ; shm�ǹ����ڴ��Ӧ�ĵ�ַ
{
    struct shared_LC_Trans *shared = NULL;
    *shmid = shmget((key_t)key, sizeof(struct shared_LC_Trans), 0666 | IPC_CREAT);
    if (*shmid == -1)
    {
        fprintf(stderr, "shmat failed/n");
        return 1;
    }
    // �������ڴ����ӵ���ǰ���̵ĵ�ַ�ռ�
    (void *)*shm = shmat(*shmid, 0, 0);
    if ((void *)*shm == (void *)-1)
    {
        fprintf(stderr, "shmat failed\n");
        return 1;
    }
#ifdef _MuQ_SHM_DEB
    printf("\nMemory attached at %X\n", (int)*shm);
#endif

    // �����ڴ��ʼ��
    shared = (struct shared_LC_Trans *)*shm;
    if (mode == 0)
    {
        shared->i_written = 0;
        shared->j_written = 0;
    }
    else
    { //��ѯ�����ڴ�Ĳ���
    }
    return 0;
}

int sem_create(int key, int *semid) 
{
    /*�����ź����ĳ�ʼֵ��������Դ����*/
    union semun sem_u;    
    // ���������ڴ漰�ź���
    *semid = semget((key_t)key, 2, 0666 | IPC_CREAT); //�����ź���Ϊ2��һ���źż�������0���ź�����ʾWrite_Enable,1���ź�����ʾRead_Enable
    if (*semid == -1)
    {
        printf("creat sem is fail\n");
        return 1;
    }
    sem_u.val = 1;
    semctl(*semid, 0, SETVAL, sem_u); //��д�źų�ʼ��Ϊ��д
    sem_u.val = 0;
    semctl(*semid, 1, SETVAL, sem_u); //�ɶ������ʼ��Ϊ���ɶ�
    return 0;
}

void shm_send(double *buff, int length, int mode, void *shm, int isemid)
{
    int i;
    struct shared_LC_Trans *shared = (struct shared_LC_Trans *)shm;
    p(isemid, 0); //ͨ���ź����жϹ����ڴ����ǲ��ǿ�д

    // �����ڴ���д������
#ifdef _MuQ_SHM_DEB
    printf("Enter some text in i side: \n");
#endif
    memcpy(shared->i_text, &buff[0], length * sizeof(double));
    //printf("Subnet 1 is Sending: ");
    //for (i = 0;i<10;i++) printf("%f ",((double*)shared->i_text)[i]);
    //printf("\n");
    // 写完数据，设置written使共享内存段可读
    shared->i_written = 1;
    v(isemid, 1); //���ô����򣬸�֪�˹����ڴ����ѿɶ�
    return;
}

void shm_recv(double *buff, int length, int mode, void *shm, int isemid) 
{
    int i;
    struct shared_LC_Trans *shared = (struct shared_LC_Trans *)shm;
    p(isemid, 1); //ͨ���ź����жϹ����ڴ����ǲ��ǿɶ�
    // û�н������ڴ�д���ݣ������ݿɶ�ȡ
    memcpy(&buff[0], shared->i_text, length * sizeof(double));
    //cplusplus_rtclock_sleep(1);
    //usleep(1);
    //printf("Subnet 1 is Recving: ");
    //for (i = 0;i<10;i++) printf("%f ",((double*)shared->i_text)[i]);
    //printf("\n");
    // 读取完数据，设置written使共享内存段可写
    shared->i_written = 0;
    v(isemid, 0); //���ô����򣬸�֪�˹����ڴ����ѿ�д
    return;
}

int shm_close(int *shmid, int mode, void *shm)
{
    // �ѹ����ڴ�ӵ�ǰ�����з���
    if (shmdt(shm) == -1)
    {
        fprintf(stderr, "shmdt failed\n");
        return 0;
    }

    // ɾ�������ڴ�
    if (mode == 0)
    {
        if (shmctl(*shmid, IPC_RMID, 0) == -1)
        {
            fprintf(stderr, "shmctl(IPC_RMID) failed\n");
            return 0;
        }
    }

    return 1;
}

void del_semvalue(int semid)
{
    //ɾ���ź���
    union semun sem_union;
    if(semctl(semid,0,IPC_RMID,sem_union)==-1)
        fprintf(stderr,"Failed to delete semaphore\n");
}

/*�ź�����P����*/
void p(int semid,int sem_sigid)
{
    struct sembuf sem_p;
    sem_p.sem_num = sem_sigid;
    sem_p.sem_op = -1;
    if (semop(semid, &sem_p, 1) == -1)
        printf("p operation is fail.\n");
}

/*�ź�����V����*/
void v(int semid,int sem_sigid)
{
    struct sembuf sem_v;
    sem_v.sem_num = sem_sigid;
    sem_v.sem_op = 1;
    if (semop(semid, &sem_v, 1) == -1)
        printf("v operation is fail.\n");
}
