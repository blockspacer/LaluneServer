#ifndef __INNER_SERVER_SESSION_DELEGATE_H_
#define __INNER_SERVER_SESSION_DELEGATE_H_

#include "ServerHeaderDef.h"
#include "AutoMatchServer.h"

class InnerServerSessionDelegate : public NetLib_ServerSession_Delegate
{
public:

	BEGIN_HANDLER

	HANDLE_MSG(boids::MessageType::PVP_SERVER_CREATE_GAME_REQUEST, boids::CreateGameResponse, ams.CreateGameResponseGot) //ս���������ɹ���ʧ�ܸ�ƥ���
	HANDLE_MSG_SESSION(boids::MessageType::PVP_SERVER_REGISTER_REQUEST, boids::PvPServerRegister, ams.ServerRegister) //��ƥ���ע��ս����
	HANDLE_MSG(boids::MessageType::PVP_SERVER_HEART_BEAT, boids::PvPServerHeartBeat, ams.ServerBeat)

	END_HANDLER(InnerServerSessionDelegate)
};

#endif