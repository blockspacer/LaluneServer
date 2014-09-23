// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the NETLIB_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// NETLIB_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifndef _NETLIB_H_
#define _NETLIB_H_

#define _NETLIB_VERSION_ "1_02"

#ifndef _NOT_LINK_NETLIB_LIB_
#ifdef _DEBUG
#define _NETLIB_LIBNAME_ "NetLibd"
#else
#define _NETLIB_LIBNAME_ "NetLib"
#endif
#pragma comment(lib, _NETLIB_LIBNAME_)  //this is not cross-platform. you should write Makefile on linux.

#endif

#include <string>
namespace boost { namespace asio { class io_service; } }

//for windows:   Need VS2010(or above)
//for gnu linux: Need gcc4.6.3(or above)

#include "stdint.h"
#include "NetLib_Error.h"
#include <memory>


//=====NetLib_Flags=====
#define NETLIB_CLIENT_FLAG_KEEP_ALIVE						(0x00000001)	//���ڷ���sizeΪ4�İ���ʱ������������
#define NETLIB_FLAG_TCP_NODELAY								(0x00000002)
#define NETLIB_CLIENT_ENABLE_RECONNECT_ON_FIRST_CONNECT		(0x00000080)	//Ĭ���״����Ӳ��Զ��������������FLAG���״�����Ҳ���Զ�����
#define NETLIB_SERVER_LISTEN_KEEP_ALIVE_EVENT				(0x00000100)	//�յ�sizeΪ4�İ��ᴥ��ReceiveKeepAliveHandler���������flag�Ļ�ֻ�ǲ�����Handler����Ҳ��KeepAliveЧ����
																			//��������sizeΪ4�İ���������Server��ReceiveFinishHandler
//=====NetLib_Flags=====

void NetLib_Set_MaxPacketSize(unsigned int MaxPacketSize); //default: 512 * 1024
void NetLib_Set_Server_Recv_Buffer_Size(int Server_Recv_Buffer_Size); //default: 16 * 1024 * 1024
void NetLib_Set_Client_Recv_Buffer_Size(int Client_Recv_Buffer_Size); //default: 16 * 1024

#define NETLIBDataSize(A) (*(uint32_t*)(A))

class NetLib_Client_Interface
{
public:
	virtual void Disconnect() = 0; //��ε���Disconnect����������
	virtual bool IsConnected() = 0; //�������д�߼�ʱ���������÷����ķ���ֵ����Ϊ�ж�ǰ������״̬���ܷ����ı䡣

	//only ip is supported, domain or hostname is not supported. Resolver not implemented
	virtual void ConnectAsync(const char* ip, uint16_t port, uint64_t flags = 0) = 0;
	virtual void ConnectAsync(uint32_t ip, uint16_t port, uint64_t flags = 0) = 0;

	//'data' must be retained until SendFinishHandler has been called
	//data��ǰ4���ֽ��ǳ���
	virtual void SendAsync(const char* data, void* pHint = nullptr) = 0;

	virtual void SendCopyAsync(const char* data, void* pHint = nullptr) = 0; //you can release `data` after SendCopyAsync been executed. 7.19�޸�: Ҳ������pHint�������ҷ��ͽ����ᴥ��SendCopyFinishHandler

	//������������Ӧ��Connect֮ǰ����
	virtual void DisableReconnect() = 0;
	virtual void EnableReconnect(int reconnect_interval_ms = 5000, int max_continuous_retries = -1) = 0; 
	//����Ĭ��ΪEnable
	//reconnect_interval_ms��ʾÿ������ʧ�ܺ�ȴ����ٺ�����ٴγ�����������һ�ζ���ʱ������������
	//max_continuous_retries��ʾ����ʧ�ܶ��ٴκ�������
	//ֻ�����ŵ����Ӷ����˲Ż�������û���ϵ����Ӳ��ᷴ��������

	virtual void ResetFailedData() = 0;	
	
	virtual class boost::asio::io_service* GetWorkIoService() = 0;

	virtual void SetKeepAliveIntervalSeconds(int keepalive_interval_seconds = 240) = 0;
};

#define NetLib_Client_ptr std::shared_ptr<NetLib_Client_Interface>

class NetLib_Client_Delegate
{
public:
	virtual void ConnectedHandler(NetLib_Client_ptr clientptr) {} //������ǰ�ͷ�����Ҳ�ǿ��Եģ���ΪĬ�����Ϻ���Զ�����֮ǰ�ۻ�������

	//��Ϊ�ײ�ʵ�ֲ�ͬ��SendFinish�Ķ������TCP��UDP�ǲ�ͬ�ġ�UDP��ȷ�϶Է��յ��Ŵ���SendFinish��
	virtual void SendFinishHandler(NetLib_Client_ptr clientptr, char* data, void* pHint) { delete[] data; } //���������new[]������ڴ棬����д�˷���
	virtual void SendCopyFinishHandler(NetLib_Client_ptr clientptr, char* data, void* pHint) {}

	//RecvFinishHandlerһ�����أ�data�ͻᱻ�ͷţ����Ա�Ҫʱ���data���п���
	virtual void RecvFinishHandler(NetLib_Client_ptr clientptr, char* data) {} //ͨ������Ҫ��д�˷�����������ֻ��������ȫ��������

	//�շ����������ǻ����������Ƿ�enable��reconnect��Ҫ��DisconnectedHandlerִ����ϲŻῪʼ�������̡�
	virtual void DisconnectedHandler(NetLib_Client_ptr clientptr, NetLib_Error error, int inner_error_code, bool& will_reconnect) {};	
	//will_reconnect������ReconnectFailedHandler�Ĳ���������ͬ���ɶ���д��

	//ע��: ��UDPЭ���£�����ʧ�ܲ��ܱ�֤100%�Ƿ���ʧ�ܣ�Ҳ�п��ܶԷ��յ��˰���ֻ�Ǳ��ػ�û���ü��յ�ȷ�ϣ��Ͷ����ˡ�
	virtual bool SendFailedHandler(NetLib_Client_ptr clientptr, const char* data, void* pHint) { return true; } //������trueʱ����ʾ�����ɹ����ط��ð���������falseʱ��data��Ҫ���ͷ�
	virtual bool SendCopyFailedHandler(NetLib_Client_ptr clientptr, const char* data_copy, void* pHint) { return true; } //������trueʱ����ʾ�����ɹ����ط��ð���������Σ�data_copy������Ҫ���ͷ�
	virtual void FailedDataReleaseHandler(const char* data, void* pHint) { delete[] data; } //������ĸ��ʺܵ͡��������ڣ���Client�����ͷ�ʱ��ʧ�ܶ����������ݣ��������ݲ���copy������
	virtual void FailedDataReleaseHandler(NetLib_Client_ptr clientptr, const char* data, void* pHint) { delete[] data; } //��������Client������ResetFailedData()ʱ
	
	virtual void ReconnectedHandler(NetLib_Client_ptr clientptr) {} //�����ɹ�
	virtual void ReconnectFailedHandler(NetLib_Client_ptr clientptr, bool& will_continue_reconnect) {}  
	//��������ʧ�ܡ�
	//will_continue_reconnectΪtrue���ʾ����������������˵�����������Ѵﵽ�򳬹�max_continuous_retries��
	//�û�Ҳ�����ֶ���will_continue_reconnect��Ϊfalse����ʾ��������������
	//���߽�����false��will_continue_reconnect��Ϊtrue, ��ʾ����һ�Ρ����������������һ��ReconnectFailedHandler����ʱwill_continue_reconnect��Ȼ����false
	//���������Ͽ����ӻ���DisableReconnect�˵ģ�����will_continue_reconnect��ֵ��ʲô���������������
};

/*
NetLib_NewClient() returns a new intance of NetLib_Client_Imp:

class NetLib_Client_Imp Thread Safety:
	Distinct objects: Safe.
	Shared objects:	  Safe.

All the handlers share a single thread,
So long-time-consuming work is ok but not recommended to be called within the handler.
*/
NetLib_Client_ptr NetLib_NewClient(std::shared_ptr<NetLib_Client_Delegate> d);

//���ʹ���ⲿ�����ioservice�����Ҫ��֤��Client��ȫֹͣǰioservice���ܱ��ͷš�
NetLib_Client_ptr NetLib_NewClient(std::shared_ptr<NetLib_Client_Delegate> d, class ioservice_thread* thread); //dll�汾�Ļ�ֻ�ܴ�nullptr������������⣬��Ϊioservice_thread�Ǹ�������

template <typename ClientDelegate>
NetLib_Client_ptr NetLib_NewClient(ioservice_thread* ioservice = nullptr)
{
	return NetLib_NewClient(std::make_shared<ClientDelegate>(), ioservice);
}


//��һ����Ҫ���ñ��������������õĻ����ܻ���Щһ���Ե��ڴ�й© -- ���õĻ���������ֱ�����ж������ͷš�
//���뱣֤���ñ�����ǰ�Ѿ�û�л���������״̬��Client��������ͷŲ�����
//��������ʹ�õĻ���Ҫ�ȵ���CTRL_ReleaseConnection()��NetLibPlus_UnInitializeClients()
void NetLib_Clients_WaitForStop(); //������Handler�е��ã�������������


//======= Server =========




class NetLib_ServerSession_Interface
{	
public:	
	//'data' must be retained until SendFinishHandler has been called
	virtual void SendAsync(const char* data, void* pHint = nullptr) = 0;
	virtual void SendCopyAsync(const char* data, void* pHint = nullptr) = 0; //7.19�޸�: Ҳ������pHint�������ҷ��ͽ����ᴥ��SendCopyFinishHandler
	virtual bool GetRemoteAddress(char* ip, uint16_t& port) = 0; //ip must have a length of at least 16
	virtual void Disconnect() = 0;

	virtual bool GetRemoteAddress(std::string& ip, uint16_t& port) = 0;
	virtual uint32_t GetRemoteIPu() = 0;
	virtual std::string GetRemoteIP() = 0;
	virtual std::string GetLocalAddress() = 0;
};

#define NetLib_ServerSession_ptr std::shared_ptr<NetLib_ServerSession_Interface>

class NetLib_ServerSession_Delegate
{
public:
	virtual void ConnectedHandler(NetLib_ServerSession_ptr sessionptr) {};

	virtual void SendFinishHandler(NetLib_ServerSession_ptr sessionptr, char* data, void* pHint = nullptr) { delete[] data; } //���������new[]������ڴ棬����д�˷���
	virtual void SendCopyFinishHandler(NetLib_ServerSession_ptr sessionptr, char* data, void* pHint = nullptr) {}

	virtual void RecvKeepAliveHandler(NetLib_ServerSession_ptr sessionptr) {};
	//'data' will be released just after RecvFinishHandler returns
	virtual void RecvFinishHandler(NetLib_ServerSession_ptr sessionptr, char* data) = 0;

	//�������������ķ���ֵû������
	virtual bool SendFailedHandler(NetLib_ServerSession_ptr clientptr, const char* data, void* pHint = nullptr) { delete[] data; return true; } //`data` should be deleted
	virtual bool SendCopyFailedHandler(NetLib_ServerSession_ptr clientptr, const char* data_copy, void* pHint) { return true; } //`data_copy` should not be deleted.

	virtual void DisconnectedHandler(NetLib_ServerSession_ptr sessionptr, NetLib_Error error, int inner_error_code) {};

	//virtual void RecvKeepAliveHandler(NetLib_ServerSession_ptr sessionptr) = 0;
};

class NetLib_Server_Interface
{
public:
	//�ѹ�ʱ���൱��ֱ�ӵ���StartTCP
	virtual bool Start(int tcp_listen_port, int udp_listen_port, int work_thread_num = 1, int timeout_seconds = 300, uint64_t flags = 0) = 0;
	virtual bool StartTCP(int listen_port, int work_thread_num = 1, int timeout_seconds = 300, uint64_t flags = 0) = 0;
	//�ѹ�ʱ��ִ����ʲôҲ������
	virtual bool StartUDP(int listen_port, int work_thread_num = 1, int timeout_seconds = 300, uint64_t flags = 0) = 0;
	virtual void Stop() = 0;

	virtual class boost::asio::io_service* GetWorkIoService() = 0;

	virtual ~NetLib_Server_Interface() {}
};

#define NetLib_Server_ptr std::shared_ptr<NetLib_Server_Interface>

class NetLib_Server_Delegate
{
public:
	 virtual NetLib_ServerSession_Delegate* New_SessionDelegate() = 0; //factory function
	 virtual void Release_SessionDelegate(NetLib_ServerSession_Delegate* d)
	 {
		 delete d; //����ж��Session����Delegate���������ô����д�������
	 }
	 
	 virtual void ErrorHandler(NetLib_Error error, int error_code)
	 {
		 //printf("server produced an error: %d, %d\n", error, error_code);
	 }
};

NetLib_Server_ptr NetLib_NewServer(std::shared_ptr<NetLib_Server_Delegate> d);

NetLib_Server_ptr NetLib_NewServer(std::shared_ptr<NetLib_ServerSession_Delegate> d);

NetLib_Server_ptr NetLib_NewServer(std::shared_ptr<NetLib_Server_Delegate> d, class ioservice_thread* thread); //dll�汾�Ļ�ֻ�ܴ�nullptr������������⣬��Ϊioservice_thread�Ǹ�������

NetLib_Server_ptr NetLib_NewServer(std::shared_ptr<NetLib_ServerSession_Delegate> d, class ioservice_thread* thread);

template <typename SessionDelegate>
class NetLib_Server_Default_Delegate : public NetLib_Server_Delegate
{
public:
	NetLib_ServerSession_Delegate* New_SessionDelegate()
	{
		return new SessionDelegate();
	}
};

template <typename SessionDelegate>
NetLib_Server_ptr NetLib_NewServer(class ioservice_thread* thread = nullptr)
{
	return NetLib_NewServer(std::make_shared<NetLib_Server_Default_Delegate<SessionDelegate> > (), thread);
}

//��һ����Ҫ���ñ��������������õĻ����ܻ���Щһ���Ե��ڴ�й© -- ���õĻ���������ֱ���͸�Server��ص����ж������ͷš�
//����ǰ���뱣֤Server���Ѿ����ù�Stop���������ⲿ�Ѿ�û��Server��shared_ptr�����ã������һֱ�������Ƕ�
void NetLib_Servers_WaitForStop(); //������Handler�е��ã�����������

/*
NetLib_NewServer() returns a new intance of NetLib_Server_Imp:

class NetLib_Server_Imp Thread Safety:
	Distinct objects: Safe.
	Shared objects:	  Safe.

class NetLib_ServerSession_Interface Thread Safety:
	Distinct objects: Safe.
	Shared objects:	  Safe.
*/

bool NetLib_CheckVersion(const char* version);

#define NETLIB_CHECK_VERSION NetLib_CheckVersion(_NETLIB_VERSION_)

#endif
