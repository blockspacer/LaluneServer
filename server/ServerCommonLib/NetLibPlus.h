#pragma once

#include "NetLib.h"
#include <memory>
#include <stdint.h>
#include <string>
#include "NetLib_Error.h"

//�÷���ͨ����ControlServerConnector���ã�Ҳ�������е���
NETLIB_API void _NetLibPlus_UpdateServerInfo(int ServerID, const char* Ip, int Port, const char* ServerType);

class NetLibPlus_Client
{
public:
	virtual void SendAsync(const char* data, void* pHint = nullptr) = 0;
	virtual void SendCopyAsync(const char* data) = 0;
	virtual int GetRemoteServerID() const = 0; 
	virtual void GetRemoteServerAddress(std::string& IP, int& port) = 0;
};

class NetLibPlus_Client_Delegate
{
public:	
	virtual void RecvFinishHandler(std::shared_ptr<NetLibPlus_Client> clientptr, char* data) {}

	virtual void ConnectedHandler(std::shared_ptr<NetLibPlus_Client> clientptr) {}
	virtual void ReconnectedHandler(std::shared_ptr<NetLibPlus_Client> clientptr) {}
	virtual void DisconnectedHandler(std::shared_ptr<NetLibPlus_Client> clientptr, NetLib_Error error, int inner_error_code) {} //����������Ҫ��֪ͨ�ã���������������߼�����
	//DisconnectedHandler������clientptr�ڵ�ָ��ֵҲ����䣬�õ���ͬһ��

	virtual void SendFinishHandler(std::shared_ptr<NetLibPlus_Client> clientptr, char* data, void* pHint) { delete[] data; }

	virtual bool SendFailedHandler(std::shared_ptr<NetLibPlus_Client> clientptr, const char* data, void* pHint) { return true; } //����true��ʾ���ڲ��ط�������false��ʾ�ڲ����ط�
	virtual bool SendCopyFailedHandler(std::shared_ptr<NetLibPlus_Client> clientptr, const char* data) { return true; } //����true��ʾ���ڲ��ط�������false��ʾ�ڲ����ط�

	virtual void FailedDataReleaseHandler(const char* data, void* pHint) { delete[] data; } //������ĸ��ʺܵ͡��������ڣ���Client�����ͷ�ʱ��ʧ�ܶ����������ݣ��������ݲ���copy�����ġ�
	//FailedDataReleaseHandler�����������������ڱ�ֱ�ӵ��ã������ڽ���Ҫ����
};

//����ֵ��֤���ǿ�shared_ptr�������ServerID��Ӧ����Ϣ��û�л�ȡ������ô���İ��������ʧ�ܶ����ServerID��Ӧ����Ϣ�ı��ˣ���ͬһ��ClientҲ���ճ���
NETLIB_API std::shared_ptr<NetLibPlus_Client> NetLibPlus_getClient(int ServerID);

//����ֵ����Ϊ��shared_ptr
NETLIB_API std::shared_ptr<NetLibPlus_Client> NetLibPlus_get_first_Client(const char* ServerType);

//����ֵ����Ϊ��shared_ptr������������get_first_Client������Ӱ�졣��һ�ε���get_next_Client�᷵�ص�һ��Client��������һ���ֻ�ӵ�һ����ʼ
NETLIB_API std::shared_ptr<NetLibPlus_Client> NetLibPlus_get_next_Client(const char* ServerType);

typedef struct tagNetLibPlus_ServerInfo
{
	std::string IP;
	int port;
	std::string ServerType;
	int ServerID;	
} NetLibPlus_ServerInfo;

#ifdef _STATIC_NETLIB_

#include <vector>

class NetLibPlus_Clients : public std::vector<std::shared_ptr<NetLibPlus_Client> >
{
public:
	void SendCopyAsync(const char* data);
};

NETLIB_API std::shared_ptr<NetLibPlus_Clients> NetLibPlus_getClients(const char* ServerType);

#include <map>

NETLIB_API std::map<int, NetLibPlus_ServerInfo> NetLibPlus_getClientsInfo(const char* ServerType);

#endif

//dll�汾�Ļ�ioservice_thֻ�ܴ�nullptr��
NETLIB_API void NetLibPlus_InitializeClients(const char* ServerType, std::shared_ptr<NetLibPlus_Client_Delegate> d, class ioservice_thread* ioservice_th = nullptr, uint64_t flags = 0); //����ĳ�����ͷ�������Delegate

NETLIB_API void NetLibPlus_InitializeClients(std::shared_ptr<NetLibPlus_Client_Delegate> d, class ioservice_thread* ioservice_th = nullptr, uint64_t flags = 0); //����ȫ�ֵ�Delegate�������Ǹ����͵�Delegate

template <typename Delegate>
void NetLibPlus_InitializeClients(const char* ServerType, class ioservice_thread* ioservice_th = nullptr, uint64_t flags = 0) //����ĳ�����ͷ�������Delegate
{
	NetLibPlus_InitializeClients(ServerType, std::make_shared<Delegate>(), ioservice_th, flags);
}

template <typename Delegate>
void NetLibPlus_InitializeClients(class ioservice_thread* ioservice_th = nullptr, uint64_t flags = 0) //����ȫ�ֵ�Delegate�������Ǹ����͵�Delegate
{
	NetLibPlus_InitializeClients(std::make_shared<Delegate>(), ioservice_th, flags);
}

NETLIB_API void NetLibPlus_UnInitializeClients(const char* ServerType); //����һ������ӹرա�ͨ����һЩһ���Ե����ӣ���ȡ��Ϣ��Ͳ������ˡ����������һ�����ӵ�ĳЩ���ӵĵ�ַ������������ֻ��ٴ����ϡ�

NETLIB_API void NetLibPlus_UnInitializeClients();

NETLIB_API void NetLibPlus_DisableClients(); //���������Disable����ʾ��ʹ��NetLibPlus��Clients��ͬʱ�������ʱ����Ҫ����NetLibPlus_UnInitializeClients


//==============================================================


