#include "GatewaySessionDelegate.h"
#include "ServerCommon.h"
#include "GatewayServer.h"
#include "Log/Log.h"

void GatewaySessionDelegate::RecvFinishHandler(NetLib_ServerSession_ptr sessionptr, char* data)
{
	if (SERVER_MSG_LENGTH(data) >= SERVER_MSG_HEADER_BASE_SIZE)
	{
		common::HeaderEx ex;
		if (ParseHeaderEx(data, ex))
		{
			int _id;
			if (ex.has_uid())
			{
				_id = ex.uid();
			}
			else if (ex.has_operation_id()) //���û��uid��operation_id����ôͨ���ǵ�½ǰ����Ϣ
			{
				_id = -(int)ex.operation_id(); //operation_id����tmp_id����gateway��map���Ǹ�����ʾ�ģ��Ժ�user_id�������֡���
			}
			else
			{
				LOGEVENTL("ERROR", "I don't know who I should send to");
				return;
			}

			NetLib_ServerSession_ptr user_session = GetSessionById(_id);
			if (user_session)
			{
				int len = SERVER_MSG_HEADER_BASE_SIZE + SERVER_MSG_DATA_LEN(data);
				char* send_buf = new char[len];
				SERVER_MSG_LENGTH(send_buf) = len;		// ���ݰ����ֽ�������msghead��
				SERVER_MSG_TYPE(send_buf) = SERVER_MSG_TYPE(data);
				SERVER_MSG_ERROR(send_buf) = 0;
				SERVER_MSG_HEADER_EX_LEN(send_buf) = 0;
				SERVER_MSG_RESERVED(send_buf) = 0;

				user_session->SendAsync(send_buf);
			}
			else
			{
				LOGEVENTL("ERROR", "user_session not found. " << _ln("id") << _id);
			}
		}
		else
		{
			LOGEVENTL("ERROR", "parse HeaderEx failed");
		}
	}
	else
	{
		LOGEVENTL("ERROR", "msg too short");
	}
}