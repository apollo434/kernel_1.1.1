//////////////////////////////////////////////////////////////////////
// ��Ȩ (C), 1988-1999, XXXX��˾
// �� �� ��: Pub_Def.h
// ��    ��:       �汾:        ʱ��: // ���ߡ��汾���������
// ��    ��: ȫ�ֺ궨��
//           ȫ�ֳ�������
//           �����Զ�������
//           ͨ���ṹ�嶨��
// ��    ������ #include <filename.h> ��ʽ�����ñ�׼���ͷ�ļ������������ӱ�׼��Ŀ¼��ʼ������
//           �� #include "filename.h" ��ʽ�����÷Ǳ�׼���ͷ�ļ��������������û��Ĺ���Ŀ¼��ʼ������
// �޸ļ�¼:     // �޸���ʷ��¼�б���ÿ���޸ļ�¼Ӧ�����޸����ڡ��޸�
                 // �߼��޸����ݼ���  
// 1. ʱ��:2014.8.4
//    ����:����Ӣ
//    �޸�����:MAXSTRLEN 256 �޸�Ϊ MAXSTRLEN 512
//    ԭ��*.TB2�ļ�һ�м�¼�ַ�������Խ��
// 2. ʱ��:2014.12.29
//    ����:����Ӣ
//    �޸�����:MAXSTRLEN 512 �޸�Ϊ MAXSTRLEN 2048
//    ԭ��*.GEN�ļ�һ�м�¼�ַ�������Խ��
// 3. ʱ��:2015.4.26
//    ����:����Ӣ
//    �޸�����:EPSINON 1.0e-10�޸�Ϊ EPSINON 1.0e-14
//    ԭ�򣺵�Ԫ��������Сʱ������ΪС���ص���·���迹���������ʽΪ��Сֵ����1e-12������Ӧ����Ϊ��������
// 4. ʱ��:2015.4.29
//    ����:����Ӣ
//    �޸�����:MAXNODE 2000�޸�Ϊ MAXNODE 4000
//             MAXNY 32000�޸�Ϊ  MAXNY 128000
// 5. ʱ��:2015.8.14
//    ����:����Ӣ
//    �޸�����:EPSINON 1.0e-14�޸�Ϊ EPSINON 1.0e-16
//    ԭ����step_GOV1()���ж�dAVZ<=dEKʱ��dAVZΪ1.0e-15��dEK=0.0ʱ���жϴ���
// 6. ʱ��:2016.11.12
//    ����:����Ӣ
//    �޸�����:OutVarStru������iVArrSTNo��iVArrDim����
//    ԭ�����������ά��������
// 7. ʱ��:2018.4.12
//    ����:����Ӣ
//    �޸�����:���ӳ�ʼ������Ԫ�������ڲ�����ȷ����һ�����̽�֧��1��INIԪ�����������޸�ת������ֱ��ģ��
// 8. ʱ��:2018.5.8
//    ����:����Ӣ
//    �޸�����:���������ַ������ȶ���MIDSTRLEN��SHORTSTRLEN�����ֲ�ͬ��;���ַ���
//    ԭ������ڵ���������ڵ�����ռ���ڴ�̫��
//////////////////////////////////////////////////////////////////////
#ifndef _PUB_DEF_H__
#define _PUB_DEF_H__


#ifdef __cplusplus

extern "C" 
{
#endif

// ȫ�ֺ궨��
#ifndef __cplusplus
// ����Сֵ
#ifndef min
	#define min(a, b)  ((a) < (b) ? (a) : (b))
#endif
// �����ֵ
#ifndef max
	#define max(a, b)  ((a) > (b) ? (a) : (b))
#endif
#endif
// ������������������룩
#define NINT(a) ((a >= 0)? (int)(a+0.5) : (int)(a-0.5)) // ͬFortran����IDNINT

// ȫ�ֳ�������
// �������������ȣ�
#define EPSINON 1.0e-16 // 1.0e-10-->1.0e-14-->1.0e-16

#define EPSINONEQU 1.0e-10 // 1.0e-10-->1.0e-14-->1.0e-16
// ��ֵ����
#define VALUPLIMIT 1.0e+10
// Բ����
#define PI 3.1415926535897932
// �м�������ά��
#ifdef _REALTIME
#define MAXMIDVAR 5000
#define MAXCOMMVAR 1000
#else
#define MAXMIDVAR 5000
#define MAXCOMMVAR 3000
#endif
// �ڵ������Ŀ
// �ڵ������Ŀ
#ifdef _WINDOWS
#define MAXNODE 8000
#else
#ifndef _REALTIME
#define MAXNODE 40000  //15000//������23000//����+������40000
#else
#define MAXNODE 3000
#endif
#endif
#ifdef _WINDOWS
#define MAXNY 115200  //115200
#else
#ifndef _REALTIME
#define MAXNY 2352000  //115200
#else
#define MAXNY 115200  //115200
#endif
#endif
// �ڵ���������
#define MAXOUTLINE 1000
// ���������Ŀ
#define MAXSUB 1000
// �ַ�����󳤶�
#define MAXSTRLEN 2048
// �м䳤���ַ�������
#define MIDSTRLEN 512 
// �̳����ַ�������
#define SHORTSTRLEN 256 
// �ļ������������
#define MAXCOL 100
// LU�ֽ��U���������Ŀ
#define MAXNU 50000
// ��
#define TRUE 1
// ��
#define FALSE 0
// Y��Խ�Ԫ����ֵ
#define ADDYVAL 1.0e-12
// ��ʼ������Ԫ����Ŀ
#define MAXINI 1  // �ڲ�����ȷ����һ�����̽�֧��1��INIԪ��
// ������ʼ������Ԫ��֧���ı�����Ŀ
#define MAXINIVAR 500


//���ݴ�������ֵ����Сֵ
#ifdef _WINDOWS
#define MAXRECV 50000
#define MAXSEND 50000
#else
#ifndef _REALTIME
#define MAXRECV 1000000
#define MAXSEND 1000000
#else
#define MAXRECV 5000
#define MAXSEND 5000
#endif
#endif

//�����������ڵ�����ST����
#define MAXST 200

//���������ڵ���
#define MAXBUS MAXNODE
#define MAXBUS_FPGA 100

#define MAXSUB_GRP 100   //�������е������������
#define	MAXGRP MAXSUB

#define MAXBUS_TLI 11    //FPGA�����ߴ�С�����ӿڵ�ѹ�ģ���󲻳���10ά��3���ӿڣ�
//һ������������Ľӿ�����
#define MAXITF 600
#define MAXITFBUS 600   //����һ�������еĽڵ���������ĵ�������ڵ���
#define MAXDIV 100
#define MAXLCDIV 50     //�����е����LC�����������50��

//Ƶ�������·���������
#define MAX_FDLINE 30

//      ����״̬��־
#define MARK_RUN 1    //1 �������ڽ���
#define MARK_INIT 0   //0 ����û�п�ʼ
#define MARK_END 2    //2 �������

#define CPU_FREQ 2.2e3          //CPU Ƶ������ʱ�����
#define MAXTIMESTEP_LEVEL 4     //�������ڵĲ�ͬ�����������

//FPGA-STS ����ϵͳ�����Ԫ����������
	//�������������FPGA�ĸ���
#define MAX_FPGA_NUM 7

	//�����е�����MMC_FPGA�ű���
#define MAX_FPGA_MMCNARM 24

#define MAX_FPGA_COMMCHANNEL 100 //1400�ֽڣ�16��֡ͷ��+ 4 �ֽڣ�֡��+��Ч���ݳ��ȣ�+ 9*8 �ֽڣ��ڵ��ѹ�� + 110 *8����Ч���ݣ�+ 110 *4����ַ��

#define MAX_STS_SLA 200
#define MAX_MMC_SLA 120
#define MAX_STS_IGBT 60
#define MAX_MMC_IGBT 60
#define MAX_STS_PWM 60
#define MAX_MMC_PWM 60
#define MAX_STS_CVS 10
#define MAX_STS_CIS 10
#define MAX_STS_SCV 40
#define MAX_STS_SCI 40
#define MAX_STS_AO 24
#define MAX_STS_AI 24
#define MAX_STS_DI 48
#define MAX_STS_STB 24
#define MAX_STS_BRK 5
#define MAX_STS_LC 3
#define MAX_STS_NAC 64
#define MAX_STS_NB 10
#define TagCHG 20    //����״̬���ݱ�ǩ

#define EMTSUB_EACHNODE 150 //IB����ϵͳ�ĵ��ڵ��ϵ���������
#define DEFAULTMPITAG 0   //Ĭ��û��Tag�����ݴ����Tag��־0
//Message type
#define PIDInfo 01
#define DIDInfo 02
#define MPCMDInfo 03
#ifdef _REALTIME
#define MAXMPCONCHN 200
#else
#define MAXMPCONCHN 1000
#endif
#define MPCONInfo 04
#ifdef _REALTIME
#define MAXMPMONCHN 300
#else
#define MAXMPMONCHN 1000
#endif
#define MPMONInfo 05

// �����Զ�������
// �̶���С���ַ�����
typedef char FixedString[MAXSTRLEN];
typedef char MidString[MIDSTRLEN];
typedef char ShortString[SHORTSTRLEN];

// ͨ�ýṹ����
// �����ṹ
typedef struct
{
	double dRms;      // ��Чֵ
	double dTheta;	  // ���
}PhasorStru;

// �����ṹ
typedef struct
{
	double dRe;      // ʵ��
	double dIm;	     // �鲿
}ComplexStru;


// ����������ṹ
//typedef struct
//{
//	double dP0;      // �й����ʳ�ֵ
//	double dQ0;	     // �޹����ʳ�ֵ
//}GenFLStru;

// Ԫ�������ṹ
typedef struct
{
	int iKsubSource; //�źŵ���ʼ����
	int iKsubDest;   //�źŵ�Ŀ������
	int iCTyp;	// Ԫ������
	int iCNo;	// Ԫ�����
	int iVNo;	// �������
	int iVArrSTNo;  // ���Ԫ������������е���ʼ���        //!!penghy!!20161112!!//����
	int iVArrDim;   // ��iVArrSTNo��ſ�ʼ��Ҫ���������ά�� //!!penghy!!20161112!!//����
	int iType;      //�ź����ͣ�Char�� 2 ���� 1 ������ 0
}CVarIndexStru;

// ��ת��Ϊ��������Ԫ��������Ԫ���ṹ
typedef struct
{
	int	iNum;		// Ԫ����Ŀ
	int	*piIndexNo;// ����Ԫ��ת��Ϊ��������Ԫ��ʱ����A���ڵ���Ԫ���еı��
}ThrPhaseConvtStru;

// ����������Ϣ�ṹ
typedef struct  
{
	int iPulse;	    // ��������(01�ź�)
	double dTTag;	// ����ֶ�
}TrigPulseStru;

// ������Ԫ����Ϣ�ṹ
typedef struct  
{
	int iNo;	    // �кŻ��к�
	double dVal;	// ֵ
}YElmStru;

//penghy//2018-2-11//���������������Чֵ�洢�ṹ
typedef struct
{									
	PhasorStru Pha;        // ������������Чֵ�����
	double dABC[4];          // ����˲ʱֵ
	double *pdABC1Cy;     // һ���ܲ��ڵ�˲ʱֵ��ά��[iNDTCycle*3+1]
	                      // �洢�ṹΪ��0��VA1��VB1��VC1��...VAn��VBn��VCn
	ComplexStru CABCRI[4]; // ��������ʵ�鲿
}PhaCalStru;


typedef struct 
{
	int iProc;
	int iPackage;
}MessagePos;

#ifdef _ADDOUT
//penghy/2018-4-27//���������������Чֵ�洢�ṹ
typedef struct
{									
	PhasorStru Pha;        // ������������Чֵ�����
	double *pdVal1Cy;     // һ���ܲ��ڵ�˲ʱֵ��ά��[iNDTCycle+1]
	// �洢�ṹΪ��0��V1��V2��...Vn
	ComplexStru CRI; // ����ʵ�鲿
}SPRMSCalStru;
#endif
typedef double udouble;

#ifdef __cplusplus
}
#endif


#endif