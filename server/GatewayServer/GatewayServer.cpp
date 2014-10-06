#include <boost/asio.hpp>
#include "include/ioservice_thread.h"
#include "Log/Log.h"
#include "NetLib/NetLib.h"
#include "memory.h"
#include "string.h"
#include <google/protobuf/stubs/common.h>
#include "MessageTypeDef.h"
#include "ServerCommonLib/ServerCommon.h"
#include "GatewaySessionDelegate.h"
#include "GatewayUserSessionDelegate.h"
#include "toBackEndDelegate.h"
#include <vector>
#include "include/utility1.h"

#define GATEWAY_OUTER_PORT (6677) //�������û���
#define GATEWAY_INNER_PORT (9432) //�ڲ�ͨ���õ�

ioservice_thread thread;

NetLib_Server_ptr server4user;

std::map<int, NetLib_ServerSession_ptr> user2session;
AvailableIDs<int> available_tmp_ids; //��ʵ��һ����Ҫ����ôһ��available_id��ֱ����operation_id�ӵ���0��ʼҲ���ԡ���idСһ�������ʡһ����������

int GetTmpUserId()
{
	return available_tmp_ids.getId();
}

void ReleaseTmpUserId(int _id)
{
	available_tmp_ids.releaseId(_id);
}

void UpdateUserSession(int uid, NetLib_ServerSession_ptr session)
{
	user2session[uid] = session;
}

void UserSessionLeft(int uid)
{
	user2session.erase(uid);
}

NetLib_ServerSession_ptr GetSessionById(int uid)
{
	auto it = user2session.find(uid);
	if (it == user2session.end())
	{
		return nullptr;
	}
	else
	{
		return it->second;
	}
}

class GatewayCommonLibDelegate : public CommonLibDelegate
{
public:
	void onInitialized()
	{
		server4user	= NetLib_NewServer<GatewayUserSessionDelegate>(&thread);

		//��ʱʱ��ÿ�����;���� TODO
		if (!server4user->StartTCP(GATEWAY_OUTER_PORT, 1, 120)) //�˿ڣ��߳�������ʱʱ��
		{
			LOGEVENTL("Error", "Server4User Start Failed !");

			//TODO ���������������˳�����
		}

		LOGEVENTL("Info", "Server4User Start Success");
	}

	void onConfigRefresh(const std::string& content)
	{

	}
};

int main(int argc, char* argv[])
{		
	//Check Memory Leaks
#if WIN32
	// Get the current bits
	int tmp = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);

	tmp |= _CRTDBG_LEAK_CHECK_DF;

	// Set the new bits
	_CrtSetDbgFlag(tmp);
#endif

	NETLIB_CHECK_VERSION;
    LogInitializeLocalOptions(true, true, "gateway");

	thread.start();

	NetLib_Server_ptr server = NetLib_NewServer<GatewaySessionDelegate>(&thread);

	//���Բ�ָ���˿� TODO (��Ҫ���ڲ��˿�)
	if (!server->StartTCP(GATEWAY_INNER_PORT, 1, 120)) //�˿ڣ��߳�������ʱʱ��
	{
		LOGEVENTL("Error", "Server Start Failed !");

		NetLib_Servers_WaitForStop();

		LogUnInitialize();

		google_lalune::protobuf::ShutdownProtobufLibrary();

		return 0;
	}
	
	LOGEVENTL("Info", "Server Start Success");

	GatewayCommonLibDelegate* cl_delegate = new GatewayCommonLibDelegate();
	InitializeCommonLib(thread, cl_delegate, GATEWAY_INNER_PORT, SERVER_TYPE_GATEWAY_SERVER, argc, argv);

	NetLibPlus_InitializeClients<toBackEndDelegate>();
	
	for (;;)
	{
		char tmp[200];
		if (scanf("%s", tmp) <= 0)
		{
			boost::this_thread::sleep(boost::posix_time::hours(1));
			continue;
		}

		if (strcmp(tmp, "stop") == 0)
		{
			if (server)
			{
				server->Stop();
				server.reset();
			}
		}
		else if (strcmp(tmp, "exit") == 0)
		{			
			break;
		}
	}

	NetLib_Servers_WaitForStop();

	LogUnInitialize();
	
	google_lalune::protobuf::ShutdownProtobufLibrary(); 

	return 0;
}

