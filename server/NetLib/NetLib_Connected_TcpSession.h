#ifndef _NETLIB_CONNECTED_TCPSESSION_H_
#define _NETLIB_CONNECTED_TCPSESSION_H_

#include "NetLib.h"
#include "NetLib_Session_Owner.h"
#include "NetLib_Packet.h"
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/locks.hpp>
#include <memory>
#include <queue>
#include "../Log/Log.h"

using namespace boost::asio::ip;

//�����������Owner���ͷţ�Owner��ȵ�Connected_TcpSessionû���κ�pending����֮����ͷ���������ȷ���ͷ�ʱ���������ⲿ��ʵ����Ҫ��shared_ptr
class NetLib_Connected_TcpSession
{
protected:
	std::shared_ptr<NetLib_Session_Owner> owner; //session����ȥ�ͷ�owner. ֻ��owner�����ֶ��ͷ�session
	boost::asio::io_service& boostioservice;
	bool m_isconnected;	

	tcp::socket* tcpsocket;	
	std::queue<netlib_packet*> async_send_queue;
	uint32_t receiving_data_size;
	std::shared_ptr<char> receiving_data;

	void tcp_connect_async();
	void tcp_connected_handler(boost::system::error_code& error);

	void _send_async_from_queue(); //�ڲ�������������

	void send_packet_finish(char* data, void* pHint);
	void send_packet_copy_finish(char* data, void* pHint);
	void tcp_send_async_finished(const boost::system::error_code& error, std::size_t bytes_transferred);
	
	void tcp_receive_async();	
	void tcp_header_recv(const boost::system::error_code& error);
	void tcp_recv_async_finished(const boost::system::error_code& error);
	
	void set_socket(tcp::socket* tcpsocket);

public:
	void close_session_and_handle_error(NetLib_Error error, const boost::system::error_code& boost_error);

	//should be called after construction
	void init(std::shared_ptr<NetLib_Session_Owner> _owner) 
	{
		owner = _owner;
	}

	uint32_t get_remote_ip();
	bool get_remote_address(char* ip, uint16_t& port);
	bool get_remote_address(std::string& ip, uint16_t& port);
	std::string get_local_address();
	void start();
	void send_async(const char* data, bool copy = false, void* pHint = nullptr);

	void process_failed_packets(); //ͬ������
	void process_failed_packet(netlib_packet* packet);

	bool is_connected() //��������������û�����壬Ҫ��ͬ������Ļ�����is_connected�������
	{		
		return m_isconnected;
	}
	
	void disconnect(NetLib_Error error = close_by_local);

	boost::asio::io_service& GetIoService()
	{
		return boostioservice;
	}

public:	
	virtual ~NetLib_Connected_TcpSession();
	NetLib_Connected_TcpSession(boost::asio::io_service & ioservice); //�����ʹ�ã������и������̳�һ��
	NetLib_Connected_TcpSession(std::shared_ptr<NetLib_Session_Owner> _owner, boost::asio::io_service & ioservice, tcp::socket& tcpsocket); //�ͻ���ʹ��
};


#endif