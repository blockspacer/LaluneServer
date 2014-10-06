#include "GatewayUserSessionDelegate.h"
#include "ServerCommon.h"
#include "MessageTypeDef.h"
#include "Log/Log.h"
#include "GatewayServer.h"

void GatewayUserSessionDelegate::ConnectedHandler(NetLib_ServerSession_ptr sessionptr)
{

}

void GatewayUserSessionDelegate::DisconnectedHandler(NetLib_ServerSession_ptr sessionptr)
{
	int _id = sessionptr->GetAttachedData();
	if (_id != 0)
	{
		UserSessionLeft(_id);
		if (_id < 0)
		{
			ReleaseTmpUserId(-_id); //��Ϊtmp_id�浽map����ȡ���õ�
		}
	}
}

void GatewayUserSessionDelegate::RecvFinishHandler(NetLib_ServerSession_ptr sessionptr, char* data)
{
	if (MSG_LENGTH(data) >= MSG_HEADER_BASE_SIZE)
	{
		int server_type = MSG_TYPE(data) / MSG_TYPE_SPAN;
		switch (server_type)
		{
		case SERVER_TYPE_GATEWAY_SERVER:
			//��Ҫ�Լ������
			LOGEVENTL("Warn", "ServerType=GatewayServer haven't implemented");
			break;
		case SERVER_TYPE_BASIC_INFO_SERVER:
		case SERVER_TYPE_LEAGUE_SERVER:
		case SERVER_TYPE_ASYNC_BATTLE_SERVER:
			//��Ҫ��ϣ��
			LOGEVENTL("Warn", "ServerType haven't implemented");
			break;
		default:
			//ת����
			{
				//���ﻹ�д��Ż������÷��ظ�map�������������ѡ�þ����ˡ����ǿ��ܻ��и�������Gatewayר�õķ���ʵ����ServerCommon��
				std::map<int, NetLibPlus_ServerInfo> info = NetLibPlus_getClientsInfo(server_type);
				if (info.size()) //TODO ��������ʱ����һ�����ˣ�֮���ٸġ���
				{
					common::HeaderEx ex;
					char* send_buf;
					if (server_type == SERVER_TYPE_VERSION_SERVER || server_type == SERVER_TYPE_LOGIN_SERVER) //�������������ڵ�½��ǰ������Ҫ����operation_id������֪���ظ��ĸ��û���������Ļ��Ǽ�user_id
					{
						int tmp_id = sessionptr->GetAttachedData();
						if (tmp_id == 0)
						{
							tmp_id = GetTmpUserId();
							sessionptr->SetAttachedData(tmp_id);
							UpdateUserSession(-tmp_id, sessionptr); //tmp_id��map�����ø�����ʾ�ģ��Ժ�������user_id������
						}
						ex.set_operation_id(tmp_id);
					}
					else
					{
						int uid = sessionptr->GetAttachedData();
						if (uid == 0)
						{
							LOGEVENTL("ERROR", "UNEXPECTED. Attached uid = 0. " << _ln("MsgType") << MSG_TYPE(data));
							return;
						}
						ex.set_uid(uid);
					}
					int ex_len = ex.ByteSize();
					int len = SERVER_MSG_HEADER_BASE_SIZE + ex_len + MSG_DATA_LEN(data);
					send_buf = new char[len];
					SERVER_MSG_LENGTH(send_buf) = len;
					SERVER_MSG_TYPE(send_buf) = MSG_TYPE(data);
					SERVER_MSG_ERROR(send_buf) = 0;
					SERVER_MSG_HEADER_EX_LEN(send_buf) = ex_len;
					SERVER_MSG_RESERVED(send_buf) = 0;

					ex.SerializeWithCachedSizesToArray((google_lalune::protobuf::uint8*)SERVER_MSG_AFTER_HEADER_BASE(send_buf));
					memcpy(SERVER_MSG_DATA(send_buf), MSG_DATA(data), MSG_DATA_LEN(data));

					auto client = NetLibPlus_getClient(info.begin()->first);
					if (client)
					{
						client->SendAsync(send_buf);
					}
					else
					{
						delete[] send_buf;
					}
				} //else�����������log�ˣ��Ͳ�����
			}
			break;
		}
	}
}