#include "NetLib_Params.h"

int UDP_REPLY_INTERVAL_MS = 90;  //12.07.23 ��120��Ϊ90
//Ĭ����һ��ʱ������reply�����������յ�����ܿ�İ�������ʡһ��������reply��
//������ͷ���Ҫ����֪��SendFinish�Ļ�����ôӦ�����ֵ��С����С���ڳ������͵����кô�����ΪREPLY��ʱ�����ӳ���RTT(Round Trip Time)

int UDP_MAX_CONSECUTIVE_RESEND_TIMES = 5; //ͬһ������RESEND�����ô�������Ϊʧ��

unsigned int MAX_PACKET_SIZE = (512 * 1024);

int SERVER_RECV_BUFFER_SIZE	= 16 * 1024 * 1024;

int CLIENT_RECV_BUFFER_SIZE = 16 * 1024;