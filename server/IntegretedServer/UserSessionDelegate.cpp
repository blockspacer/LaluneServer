#include "UserSessionDelegate.h"

void UserSessionDelegate::DisconnectedHandler(NetLib_ServerSession_ptr sessionptr, NetLib_Error error, int inner_error_code)
{
	//��ʱ��Ҫ������߼�Ӧ�ø��࣬��ֻ��ȡ��ƥ��
	ams.MatchCancel(sessionptr);
}