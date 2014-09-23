#include "NetLib_Connected_TcpSession.h"
#include "NetLib_Params.h"
#include <boost/bind.hpp>
#include <boost/array.hpp>
#include "Log/Log.h"

NetLib_Connected_TcpSession::NetLib_Connected_TcpSession(boost::asio::io_service & ioservice) : boostioservice(ioservice), m_isconnected(true)
{
}

NetLib_Connected_TcpSession::NetLib_Connected_TcpSession(std::shared_ptr<NetLib_Session_Owner> _owner, boost::asio::io_service & ioservice, tcp::socket& _socket) : 
	boostioservice(ioservice), m_isconnected(true), tcpsocket(&_socket)
{
	init(_owner);
}

NetLib_Connected_TcpSession::~NetLib_Connected_TcpSession()
{	
	//LOGEVENTL("NetLib_Trace", "NetLib_Connected_TcpSession deconstruction. " << _ln("owner_ptr") << hex(owner.get()) << _ln("self_ptr") << hex((std::size_t)this));
	LOGEVENTL("~ConnectedSession", _ln("owner") << hex((std::size_t)owner.get()) << _ln("self") << hex((std::size_t)this));
}

void NetLib_Connected_TcpSession::set_socket(tcp::socket* _socket)
{
	tcpsocket = _socket;
}

void NetLib_Connected_TcpSession::start()
{	
	boost::lock_guard<boost::recursive_mutex> lock(owner->get_mutex());
	owner->increase_pending_ops_count(); //��������ˣ��Ͳ�����tcp_receive_async����ÿ�μ���
	tcp_receive_async();
/*
	boost::asio::socket_base::receive_buffer_size option;
	boost::system::error_code error;
	tcpsocket->get_option(option, error);
	LOGEVENTL("TCPRecvBuffer", option.value());*/
}

void NetLib_Connected_TcpSession::tcp_receive_async()
{
	boost::lock_guard<boost::recursive_mutex> lock(owner->get_mutex());
	boost::asio::async_read(*tcpsocket,
		boost::asio::buffer((char*)(&receiving_data_size), 4),
		boost::asio::transfer_at_least(4),
		boost::bind(&NetLib_Connected_TcpSession::tcp_header_recv, this, boost::asio::placeholders::error));
}

void NetLib_Connected_TcpSession::tcp_header_recv(const boost::system::error_code& error)
{
	boost::lock_guard<boost::recursive_mutex> lock(owner->get_mutex());
	if (error || !m_isconnected)
	{
		if (error == boost::asio::error::eof || error == boost::asio::error::connection_reset || error == boost::asio::error::connection_refused)
		{
			close_session_and_handle_error(tcp_disconnect_by_remote, error);
		}
		else
		{
			close_session_and_handle_error(client_recv_error, error); 
		}
		owner->decrease_pending_ops_count(); //ֻ��error��ʱ����Ҫdecrease����Ϊ����ʱ�򶼻������
	}
	else
	{
		if (receiving_data_size > MAX_PACKET_SIZE)
		{
			close_session_and_handle_error(packet_too_big, error);
			owner->decrease_pending_ops_count(); //ֻ��error��ʱ����Ҫdecrease����Ϊ����ʱ�򶼻�����ա�add @ 11.28����ǰ��������
			return;
		}

		if (receiving_data_size < 4) 
		{
			close_session_and_handle_error(packet_too_small, error); 
			owner->decrease_pending_ops_count(); //ֻ��error��ʱ����Ҫdecrease����Ϊ����ʱ�򶼻�����ա�add @ 11.28����ǰ��������
			return;
		}

		//��ʵ���ﲻ��shared_ptrҲ���ԡ���shared_ptr�Ļ���Ҫע�����ǰ�ͷ�
		receiving_data = std::shared_ptr<char>(new char[receiving_data_size], [](char* d){delete[] d;} );
		memcpy(receiving_data.get(), &receiving_data_size, 4);

		boost::asio::async_read(*tcpsocket,
			boost::asio::buffer(receiving_data.get() + 4, receiving_data_size - 4),
			boost::asio::transfer_at_least(receiving_data_size - 4),
			boost::bind(&NetLib_Connected_TcpSession::tcp_recv_async_finished, this, boost::asio::placeholders::error));
	}
}

void NetLib_Connected_TcpSession::tcp_recv_async_finished(const boost::system::error_code& error)
{	
	//��������������recv���������ڲ����ε��õģ�ֻҪ�÷����ڵ�����ֱ�ӷ��ʵı�����ֻ��recvϵ�з����йأ��Ͳ����������޸ġ�
	//���Ҳ�����keep_alive�ˣ�Ҳ���õ�����;���ͷ�
	//ֻ��m_isconnected����recv��صģ���m_isconnected����ν

	if (error || !m_isconnected)
	{		
		if (error == boost::asio::error::eof || error == boost::asio::error::connection_reset || error == boost::asio::error::connection_refused)
		{
			close_session_and_handle_error(tcp_disconnect_by_remote, error);
		}
		else
		{
			close_session_and_handle_error(client_recv_error, error); 
		}
		owner->decrease_pending_ops_count(); //ֻ��error��ʱ����Ҫdecrease����Ϊ����ʱ�򶼻������
	}
	else
	{
		owner->RecvFinishHandler(receiving_data.get()); //���Handler����post�������Ǵ�������������һ����shared_ptr���ᱻ�ͷ�

		tcp_receive_async();
	}
}

void NetLib_Connected_TcpSession::send_packet_copy_finish(char* data, void* pHint)
{
	owner->SendCopyFinishHandler(data, pHint);
	delete[] data;

	owner->decrease_pending_ops_count();
}

void NetLib_Connected_TcpSession::send_packet_finish(char* data, void* pHint)
{
	owner->SendFinishHandler(data, pHint);

	owner->decrease_pending_ops_count();
}

void NetLib_Connected_TcpSession::tcp_send_async_finished(const boost::system::error_code& error, std::size_t bytes_transferred)
{	
	boost::lock_guard<boost::recursive_mutex> lock(owner->get_mutex());
	if (error || !m_isconnected)
	{
		close_session_and_handle_error(client_recv_error, error);		
		owner->decrease_pending_ops_count();
	}
	else
	{	
		netlib_packet* front_packet = async_send_queue.front();

		if (front_packet->copy)
		{
			boostioservice.post(boost::bind(&NetLib_Connected_TcpSession::send_packet_copy_finish, this, front_packet->data, front_packet->pHint));
		}
		else
		{
			boostioservice.post(boost::bind(&NetLib_Connected_TcpSession::send_packet_finish, this, front_packet->data, front_packet->pHint));
			//why post: 
			//1). we should let the async_send_queue.front() be released first. avoid SendFinishHandler calls something and affect async_send_queue
			//2). Handler����Ӧ�����ڣ���������
		}

		delete front_packet;
		async_send_queue.pop();

		if (async_send_queue.size() > 0)
		{
			//������pending_ops
			_send_async_from_queue();
		}
	}
}

void NetLib_Connected_TcpSession::disconnect(NetLib_Error error)
{
	boost::system::error_code boost_empty_error;
	close_session_and_handle_error(error, boost_empty_error);
}

//��ʵ����������Բ���keep_alive������ֻҪ��֤�������е����߶��Ѿ���connected_session��keep_alive�ˣ����Ҷ�������ֱ�ӵ��ö���post���ü��ɡ�
//�����Ǽ���keep_alive�ˣ������߼����ӣ����������ʧ����ʲô
void NetLib_Connected_TcpSession::close_session_and_handle_error(NetLib_Error error, const boost::system::error_code& boost_error)
{		
	boost::lock_guard<boost::recursive_mutex> lock(owner->get_mutex());
	if (m_isconnected)
	{		
		m_isconnected = false;	//����������ӳ�ʼ��Ϊtrue֮���ܹ�ֻ��仯һ��

		if (error != close_by_local && !tcpsocket->is_open())
		{
			LOGEVENTL("Debug", "close_session, but tcpsocket->is_open() already false.");
		}

		//���������ǲ���open����shutdown��closeһ��
		boost::system::error_code ignored_error;
		tcpsocket->shutdown(boost::asio::socket_base::shutdown_both, ignored_error);
		tcpsocket->close(ignored_error);
			
		//server����session_detail��owner, serversession����session_detail��owner
			
		//���ﲻ����post����Ϊ����client��˵������Ҫ��֤error�ȱ���¼��Ȼ���ٴ���decrease_pending_ops����post�Ļ�˳���û����֤��
		owner->handle_error(error, boost_error.value()); //һ�������У�ֻ�е�һ��error��Ҫ����¼�������error���˾�����
	}
}

//����������ü�������Ϊ��ֻ����m_isconnected=false֮��Żᱻ���ã���˲����ܱ�send_async������
//�Ѿ���˼������������Ϊ�����лص�Ҫִ�С���
void NetLib_Connected_TcpSession::process_failed_packets()
{	
	/*if (async_send_queue.size())
	{
		LOGEVENTL("NetLib_Debug", "TcpSession failed_data: " << async_send_queue.size());
	}*/

	while (async_send_queue.size())
	{
		process_failed_packet(async_send_queue.front());
		async_send_queue.pop();
	}
}

void NetLib_Connected_TcpSession::process_failed_packet(netlib_packet* packet)
{
	if (packet->copy)
	{
		if (!owner->SendCopyFailedHandler(packet->data, packet->pHint)) //����true��ʾ��Ҫ�ط�����ɾ������false��ʾ����ɾ��
		{
			//LOGEVENTL("NetLib_Trace", "data_copy: 0x" << std::hex << (std::size_t)packet->data << " deleted.");
			delete[] packet->data;
		}
	}
	else
	{
		owner->SendFailedHandler(packet->data, packet->pHint);
	}
	delete packet;
}

void NetLib_Connected_TcpSession::_send_async_from_queue()
{	
	boost::lock_guard<boost::recursive_mutex> lock(owner->get_mutex());
	
	owner->increase_pending_ops_count();

	netlib_packet* front_packet = async_send_queue.front();

	boost::asio::async_write(*tcpsocket, boost::asio::buffer(front_packet->data, front_packet->data_size),
		boost::bind(&NetLib_Connected_TcpSession::tcp_send_async_finished, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

//���������ִ�е�ʱ����Ϊ�����������Լ���һֱ��m_isconnected״̬(��Ȼ�ײ��п��ܶ���)�������õ��Ļ��m_isconnected=false֮�����Ĳ�����process_failed_packets������š�
void NetLib_Connected_TcpSession::send_async(const char* data, bool copy, void* pHint) //�÷����϶���ͨ��shared_ptr���õģ�����keep_alive
{		
	boost::lock_guard<boost::recursive_mutex> lock(owner->get_mutex());

	netlib_packet* packet = new netlib_packet(data, copy, pHint);				
	async_send_queue.push(packet);
	if (async_send_queue.size() == 1)
	{
		//async_send can only be called after the previous one has been finished
		//if size() > 1, means there are packet(s) still be sending, then we needn't do any thing, just push the packet into queue.

		_send_async_from_queue();

		//boostioservice.post(boost::bind(&NetLib_Connected_TcpSession::send_async_from_queue, this, shared_from_this())); 
		//why post: avoid RecvFinishHandler() (or other handler) indirectly calls send_async_from_queue() before the handler finish
	}
}

bool NetLib_Connected_TcpSession::get_remote_address(char* ip, uint16_t& port)
{	
	boost::lock_guard<boost::recursive_mutex> lock(owner->get_mutex());
	boost::system::error_code error;
	const tcp::endpoint& ep = tcpsocket->remote_endpoint(error);
	if (error)
	{
		return false;
	}
	strcpy(ip, ep.address().to_string(error).c_str());
	if (error)
	{
		return false;
	}
	port = ep.port();
	return true;
}

bool NetLib_Connected_TcpSession::get_remote_address(std::string& ip, uint16_t& port)
{	
	boost::lock_guard<boost::recursive_mutex> lock(owner->get_mutex());
	boost::system::error_code error;
	const tcp::endpoint& ep = tcpsocket->remote_endpoint(error);
	if (error)
	{
		return false;
	}
	ip = ep.address().to_string(error);
	if (error)
	{
		return false;
	}
	port = ep.port();
	return true;
}

uint32_t NetLib_Connected_TcpSession::get_remote_ip()
{
	boost::lock_guard<boost::recursive_mutex> lock(owner->get_mutex());
	boost::system::error_code error;
	const tcp::endpoint& ep = tcpsocket->remote_endpoint(error);
	if (error)
	{
		return 0;
	}
	return ep.address().to_v4().to_ulong();
}

std::string NetLib_Connected_TcpSession::get_local_address()
{
	boost::system::error_code error;
	const tcp::endpoint& ep = tcpsocket->local_endpoint(error);
	if (error)
	{
		return "";
	}
	else
	{
		return ep.address().to_string(error);
	}
}