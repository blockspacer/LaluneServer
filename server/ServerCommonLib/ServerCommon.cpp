#include "NetLib.h"
#include "include/ToAbsolutePath.h"
#include "ServerCommon.h"
#include "include/SimpleIni.h"
#include "include/utility1.h"
#include "include/ioservice_thread.h"
#include "Header.h"
#include "Log/Log.h"
#include <map>

CommonLibDelegate* __commonlib_delegate;
int __my_listening_port, __my_server_type;
NetLib_Client_ptr __conn2controlserver;

void SayHello2ControlServer(int is_server_start = 1)
{
	common::Hello hello;
	hello.set_my_listening_port(__my_listening_port);
	hello.set_server_type(__my_server_type);
	hello.set_is_server_start(1);
	SendMsg(CONTROL_SERVER_ID, MSG_TYPE_CONTROL_SERVER_SAY_HELLO, hello);
}

class Conn2ControlServerDelegate : public NetLib_Client_Delegate
{
public:
	void ReconnectedHandler(NetLib_Client_ptr clientptr)
	{
		SayHello2ControlServer(0);
	}

	void RecvFinishHandler(NetLib_Client_ptr clientptr, char* data)
	{
		if (SERVER_MSG_LENGTH(data) >= SERVER_MSG_HEADER_BASE_SIZE)
		{
			switch (SERVER_MSG_TYPE(data))
			{
			case MSG_TYPE_CONTROL_SERVER_ADDR_INFO_ADD:
			default:
				break;
			}
		}
		else
		{
			LOGEVENTL("ERROR", "message not enough size: " << SERVER_MSG_LENGTH(data));
		}
	}
};

void InitializeCommonLib(ioservice_thread& thread, CommonLibDelegate* d, int my_listening_port, int my_server_type, int argc, char* argv[])
{
	__commonlib_delegate = d;

	__my_listening_port = my_listening_port;
	__my_server_type = my_server_type;

	std::string control_server_ip = "192.168.1.16"; //Ĭ��ֵ
	int control_server_port = CONTROL_SERVER_DEFAULT_PORT;

	if (argc >= 3)
	{
		//�������������

		control_server_ip = argv[1];
		control_server_port = utility1::str2int(argv[2]);
	}
	else
	{
		//��ini�����
		CSimpleIni ini;
		if (ini.LoadFile(utility3::ToAbsolutePath("local_config.ini").c_str()) == SI_OK)
		{
			control_server_ip = ini.GetValue("ControlServer", "ControlServerIP", "192.168.1.16");
			control_server_port = ini.GetLongValue("ControlServer", "ControlServerPortP", 5432);
		}
	}

	_NetLibPlus_UpdateServerInfo(CONTROL_SERVER_ID, control_server_ip.c_str(), control_server_port, SERVER_TYPE_CONTROL_SERVER);

	SayHello2ControlServer();
}

void ReportLoad(float load_factor)
{
	common::ReportLoad load;
	load.set_load(load_factor);
	SendMsg(CONTROL_SERVER_ID, MSG_TYPE_CONTROL_SERVER_REPORT_LOAD, load);
}

//��Ҫ���Ի��˽⣺��protobufֱ�ӵ�struct�ö�����������ʧ����ʧ���
//TODO
//std::map<int, common::CorrespondingServer> corresponding_server_map;

//���費��Ƶ�����á�����ֻ����֪��CorrespondingServer���޸�ʱ�ŷ�����������Ҳֻ�����յ�CorrespondingServer�ǿ�ʱ��Update
/*
void UpdateCorrespondingServer(uint64_t user_id, const common::CorrespondingServer& cs)
{
	//����ֱ�Ӱ�һ�����ӽṹ��map���ˡ��Ż���ʱ����Կ��ǰ�����ĳ�ָ��ʲô�ģ�������unordered_map
	corresponding_server_map[user_id] = cs;
}
*/
