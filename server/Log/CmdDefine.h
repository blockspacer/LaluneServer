#pragma once

#define CAT_LOGSVR	0x4150 //"Log�����������Ϣ"

#define ID_LOGSVR_LOGIN 0x01 //"��½Log��������Ŀǰ��Ҫ���ύindex1��index2����Ϣ��������֮���Log��ʹ����ͬ��index1��index2�������Ժ�ÿ�ζ���"
#define ID_LOGSVR_LOG 0x02 //"��Log"
#define ID_LOGSVR_LOGIN_FAIL  		0x03 //"��½Log������ʧ��"
#define ID_LOGSVR_LOGOPTIONS  		0x04 //"Log����������Logѡ��ĸ���"
#define ID_LOGSVR_NO_PRIVILEDGE  	0x05 //"��Ӧ������Ȩ�޲���"
#define ID_LOGSVR_QUERY_123  		0x10 //"��ѯLog������Ϊ��index1/index2/index3"
#define ID_LOGSVR_QUERY_123p  		0x11 //"��ѯLog������Ϊ��index1/index2/index3_prefix"
#define ID_LOGSVR_QUERY_13  			0x12 //"��ѯLog������Ϊ��index1/index3"
#define ID_LOGSVR_QUERY_13p  		0x13 //"��ѯLog������Ϊ��index1/index3_prefix"
#define ID_LOGSVR_QUERY_1p3p  		0x14 //"��ѯLog������Ϊ��index1_prefix/index3_prefix"
#define ID_LOGSVR_QUERY_1p3  		0x15 //"��ѯLog������Ϊ��index1_prefix/index3"
#define ID_LOGSVR_QUERY_RESULT  		0x20 //"Log��������Log��ѯ�߷���Log(����"
#define ID_LOGSVR_QUERY_RESULT_END  	0x21 //"Log��������Log��ѯ�߷���Log(���һ����"
#define ID_LOGSVR_SHOW_QUERIES  		0x30 //"��ʾ��������ִ�л�ȴ�ִ�еĲ�ѯ"
#define ID_LOGSVR_KILL_QUERY  		0x31 //"Killָ��TransactionID�Ĳ�ѯ"