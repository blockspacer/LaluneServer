#include "NetLib.h"
#include "NetLibPlus.h"
#include <queue>
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include "NetLib/NetLib_Packet.h"

class NetLibPlus_Client_Imp : public NetLibPlus_Client, public NetLib_Client_Delegate, public std::enable_shared_from_this<NetLibPlus_Client_Imp>
{
private:
	std::shared_ptr<NetLibPlus_Client_Delegate> m_delegate;
	int m_ServerID;
	std::string m_RemoteServerIP;
	int m_RemoteServerPort;
	NetLib_Client_ptr m_client;
	std::queue< netlib_packet > failed_data_queue;
	boost::shared_mutex client_mutex; //����m_client����ʹ���б��ͷ�
	boost::mutex queue_mutex; //����queue

	void SendFinishHandler(NetLib_Client_ptr clientptr, char* data, void* pHint);
	void RecvFinishHandler(NetLib_Client_ptr clientptr, char* data);

	void add_failed_data(const char* data, bool is_copy, void* pHint = nullptr);

	bool SendFailedHandler(NetLib_Client_ptr clientptr, const char* data, void* pHint);

	void SendTheCopyAsync(const char* data);

	void ConnectedHandler(NetLib_Client_ptr clientptr);
	void ReconnectedHandler(NetLib_Client_ptr clientptr);
	void ReconnectFailedHandler(NetLib_Client_ptr clientptr, bool& will_continue_reconnect);
	void DisconnectedHandler(NetLib_Client_ptr clientptr, NetLib_Error error, int inner_error_code, bool& will_reconnect);

	void ResendFailedData();

public:
	NetLibPlus_Client_Imp(int ServerID);
	virtual ~NetLibPlus_Client_Imp();
	void InitializeDelegate(std::shared_ptr<NetLibPlus_Client_Delegate> d); //����Ҫ�ڵ�һ��ResetClient֮ǰ����
	void ResetClient(const char* ip, uint16_t tcp_port, class ioservice_thread* ioservice_th = nullptr, uint64_t flags = 0);
	void ReleaseClient();
	void SendAsync(const char* data, void* pHint = nullptr);
	void SendCopyAsync(const char* data);
	int GetRemoteServerID() const;
	void GetRemoteServerAddress(std::string& IP, int& port);
};