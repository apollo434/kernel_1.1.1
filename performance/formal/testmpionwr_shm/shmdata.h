#ifndef _SHMDATA_H_HEADER
#define _SHMDATA_H_HEADER
 
#define TEXT_SZ 2048
 
struct shared_LC_Trans
{
    int i_written; // ��Ϊһ����־����0����ʾ�ɶ���0����ʾ��д
    int j_written;
    char i_text[TEXT_SZ]; // ��¼д�� �� ��ȡ ���ı�
    char j_text[TEXT_SZ]; // ��¼д�� �� ��ȡ ���ı� 
};
 
#endif