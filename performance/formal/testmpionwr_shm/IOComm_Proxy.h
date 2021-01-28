#ifndef _IOCOMM_PROXY_H
#define _IOCOMM_PROXY_H
#define BUFFMODE 02 //����ģʽ
#define NOBUFFMODE 01 //�޻�����
#define BUFFALLOCATEABLE 03 //����չ����ģʽ


class IOBuffInfo
{
public:
	int iNum;
	MPICOMM_PACKAGE Package[MAXRECVPACKAGENUM];
};

class BuffInfo
{
public:
	void* startaddress;
	int length;
	int mode;  //��ǰ��������ģʽ   01=��ͨģʽ��û���Լ��Ļ�����  02=����ģʽ�����Լ��Ļ����� 03=����չ����ģʽ�����Լ��Ŀ���չ���幦��
	int iNum; //���е�Message������
	int MaxLength;  //��������ԭʼ��С
	void* currentaddress; //��Ч��������ǰ��ַ
	BuffInfo* buffaddress;
	void allocate_mem(int size,int mode);  //�Լ�����һ�黺����,��СΪsize��ģʽΪmode
	void release_mem();   //�ͷ�һ�黺����;
	void check();         //�����������;
	void elem_pushback(void* databuff,int length);
	void elem_pushbackwithoutaddiNum(void* databuff,int length);
	void reallocate_mem(int size,int mode);
	void MessageExtract(int iProc,std::vector <MPICOMM_PACKAGE>&);
	void RecvBuffFlush();
	void clear();
};

#ifdef __cplusplus
extern   "C"   {
#endif
extern void PackageDivision(int step); //接收以后，统一分割报文
//void MpConDispatch(StruConInfo MpCon); //������ֵĺ���
//void MpMonDispatch(StruMonInfo MpMon); //������ֵĺ���
//void MpMonGather(StruMonInfo MpMon,StruMonRes& MpMonRes); //�����ۺϵĺ���
#ifdef __cplusplus
}
#endif
#endif