#include "InnerServerSessionDelegate.h"
#include "ServerHeaderDef.h"
#include "MessageTypeDef.h"
#include "Log/Log.h"
#include "AutoMatchServer.h"

void InnerServerSessionDelegate::RecvFinishHandler(NetLib_ServerSession_ptr sessionptr, char* data)
{
	BEGIN_SWITCH

	HANDLE_MSG(MSG_TYPE_PVP_SERVER_CREATE_GAME_RESPONSE, boids::CreateGameResponse, ams.CreateGameResponseGot) //ս���������ɹ���ʧ�ܸ�ƥ���
	HANDLE_MSG(MSG_TYPE_PVP_SERVER_REGISTER, boids::PvPServerRegister, ams.ServerRegister) //��ƥ���ע��ս����
	HANDLE_MSG(MSG_TYPE_PVP_SERVER_HEART_BEAT, boids::PvPServerHeartBeat, ams.ServerBeat)

	END_SWITCH("InnerServerSessionDelegate")
}