//////////////////////////////////////////////////////////////////////
// ��Ȩ (C), 1988-1999, XXXX��˾
// �� �� ��: shmFunc.h
// ��    ��:        �汾:       ʱ��:
// ��    ��: �����ýӿں������� 
// ��    ������ #include <filename.h> ��ʽ�����ñ�׼���ͷ�ļ������������ӱ�׼��Ŀ¼��ʼ������
//           �� #include "filename.h" ��ʽ�����÷Ǳ�׼���ͷ�ļ��������������û��Ĺ���Ŀ¼��ʼ������
// �޸ļ�¼:     // �޸���ʷ��¼�б�ÿ���޸ļ�¼Ӧ�����޸����ڡ��޸�
// �߼��޸����ݼ���  
// 1. ʱ��:
//    ����:
//    �޸�����:
// 2. ...
//////////////////////////////////////////////////////////////////////
#ifndef _SHMFUNC_H__
#define _SHMFUNC_H__
//���ص�ǰʱ��
#ifdef __cplusplus
extern "C"
{
#endif

	extern int shm_create(int key, int *shmid, int mode, void **shm); //mode 0 表示创建共享内存 1 表示查询共享内存 ; shm是共享内存对应的地址
	extern void shm_send(double *buff, int length, int mode, void *shm, int isemid);
	extern void shm_recv(double *buff, int length, int mode, void *shm, int isemid); 
    extern int shm_close(int* shmid,int mode,void* shm);
	extern int sem_create(int key, int *semid);
	extern void del_semvalue(int semid);
#ifdef __cplusplus
};
#endif
#endif