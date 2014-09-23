#include "NetLib_ServerSession_Imp.h"
#include "NetLib_Server_Imp.h"
#include "Log/Log.h"
#include <boost/bind.hpp>

NetLib_ServerSession_Imp::NetLib_ServerSession_Imp(NetLib_ServerSession_Delegate* d, std::shared_ptr<NetLib_Server_Delegate> sd, boost::asio::io_service& ioservice, int timeout_seconds): 
	theDelegate(d), serverDelegate(sd), boostioservice(ioservice), m_keep_alive_timer(boostioservice), m_timeout_seconds(timeout_seconds)
{
}

NetLib_ServerSession_Imp::~NetLib_ServerSession_Imp()
{
	serverDelegate->Release_SessionDelegate(theDelegate);
	
	//LOGEVENTL("NetLib_Trace", "NetLib_ServerSession_Imp deconstruction " << _ln("ptr") << hex((std::size_t)this));
}

boost::recursive_mutex& NetLib_ServerSession_Imp::get_mutex()
{
	return session_mutex;
}

void NetLib_ServerSession_Imp::refresh_timeout_timer() //session�����󣬸÷������ٻᱻserver����һ�Σ�Ȼ��ÿ���յ�����ˢ�³�ʱʱ��
{
	m_keep_alive_timer.expires_from_now(boost::posix_time::seconds(m_timeout_seconds));
	//LOGEVENTL("NetLib_Trace", "ServerSession timeout_handler refreshed");
	m_keep_alive_timer.async_wait(boost::bind(&NetLib_ServerSession_Imp::timeout_handler, this, shared_from_this(), boost::asio::placeholders::error));

	//���timer��session_detailû�й�ϵ������increasing_pending_ops_count������ServerSession���pending_op
}

//session_detailҪkeep_alive, ServerSession_Imp����ҲҪkeep_alive����Ϊ���Ǹ��첽���õķ���
void NetLib_ServerSession_Imp::timeout_handler(std::shared_ptr<NetLib_ServerSession_Imp> keep_alive, const boost::system::error_code& error) 
{
	if (!error)
	{		
		boost::lock_guard<boost::recursive_mutex> lock(session_mutex);

		if (session_detail && session_detail->is_connected())
		{
			session_detail->disconnect(session_timeout);
		} //���session_detail�Ѿ����ͷ��ˣ��Ǿ�˵�������Ѿ���Ϊ���ԭ����ˣ������ٴ�����ʱ��Session����Client��һ���������ڶϿ�ǰ�϶�����session_detail��
	}
	else
	{
		//LOGEVENTL("NetLib_Trace", "ServerSession timeout_handler canceled");
	}
}

//'data' must be retained until SendFinishHandler has been called
void NetLib_ServerSession_Imp::SendAsync(const char* data, void* pHint)
{	
	session_mutex.lock(); //���뱣֤m_isconnectedΪtrue֮�󣬲��ٽ�send_async����send_async�ᵼ������pending_ops����ʱ��pending_ops��������0���ᵼ�¶���ͷ�

	if (session_detail && session_detail->is_connected())
	{
		session_detail->send_async(data, false, pHint);
		session_mutex.unlock();
	}
	else
	{
		session_mutex.unlock();
		boostioservice.post(boost::bind(&NetLib_ServerSession_Delegate::SendFailedHandler, theDelegate, shared_from_this(), data, pHint)); //Handler����ֱ�ӻ��ӵľ���post���ܵ��ã������п��ܵ�������
	}
}

void NetLib_ServerSession_Imp::SendCopyAsyncFailed(std::shared_ptr<NetLib_ServerSession_Imp> keep_alive, const char* data_copy, void* pHint)
{
	theDelegate->SendCopyFailedHandler(keep_alive, data_copy, pHint);
	delete[] data_copy;
}

void NetLib_ServerSession_Imp::SendCopyAsync(const char* data, void* pHint)
{		
	char* data_copy = new char[*(uint32_t*)data];
	memcpy(data_copy, data, *(uint32_t*)data);

	session_mutex.lock(); //���뱣֤m_isconnectedΪtrue֮�󣬲��ٽ�send_async����send_async�ᵼ������pending_ops����ʱ��pending_ops��������0���ᵼ�¶���ͷ�

	if (session_detail && session_detail->is_connected())
	{
		session_detail->send_async(data_copy, true, pHint);		
		session_mutex.unlock();
	}
	else
	{		
		session_mutex.unlock();
		boostioservice.post(boost::bind(&NetLib_ServerSession_Imp::SendCopyAsyncFailed, this, shared_from_this(), data_copy, pHint)); //Handler����ֱ�ӻ��ӵľ���post���ܵ��ã������п��ܵ�������
	}
}

bool NetLib_ServerSession_Imp::GetRemoteAddress(char* ip, uint16_t& port)
{	
	//����Ҳ���ܱ�֤session_detail�����ͷţ����keep_aliveһ��
	std::shared_ptr<NetLib_Connected_TcpSession> session_detail_keep_alive = session_detail;

	if (session_detail_keep_alive)
	{
		return session_detail_keep_alive->get_remote_address(ip, port);
	}
	else
	{
		return false;
	}
}

bool NetLib_ServerSession_Imp::GetRemoteAddress(std::string& ip, uint16_t& port)
{
	boost::lock_guard<boost::recursive_mutex> lock(session_mutex);

	if (session_detail)
	{
		return session_detail->get_remote_address(ip, port);
	}
	else
	{
		return false;
	}
}

std::string NetLib_ServerSession_Imp::GetRemoteIP()
{
	boost::lock_guard<boost::recursive_mutex> lock(session_mutex);

	if (session_detail)
	{
		std::string ip;
		uint16_t port;
		if (session_detail->get_remote_address(ip, port))
		{
			return ip;
		}
	}
	return "";
}

uint32_t NetLib_ServerSession_Imp::GetRemoteIPu()
{
	boost::lock_guard<boost::recursive_mutex> lock(session_mutex);

	if (session_detail)
	{
		return session_detail->get_remote_ip();
	}
	return 0;
}

std::string NetLib_ServerSession_Imp::GetLocalAddress()
{
	boost::lock_guard<boost::recursive_mutex> lock(session_mutex);

	if (session_detail)
	{
		return session_detail->get_local_address();
	}
	else
	{
		return "";
	} 
}

void NetLib_ServerSession_Imp::Disconnect()
{	
	boost::lock_guard<boost::recursive_mutex> lock(session_mutex);

	if (session_detail)
	{
		//LOGEVENTL("NetLib_Trace", "manually disconnect a ServerSession");
		session_detail->disconnect();
	}
	else
	{
		//LOGEVENTL("NetLib_Trace", "manually disconnect a ServerSession, but session_detail has been released already.");
	}
}

//��pending_ops����0ʱִ��
void NetLib_ServerSession_Imp::disconnected()
{	
	{
		boost::lock_guard<boost::recursive_mutex> lock(session_mutex);

		session_detail->process_failed_packets();
		session_detail.reset(); //������Ȼ�ж���session_detail��server_session����ϵ�������ᵼ��server_session�ͷš�server_session�ڽ�������erase_session����ɾ�����һ�����ú󣬲Żᱻ�ͷ�
	}
	
	server->erase_session(shared_from_this());
}

//ֻ���һ��
void NetLib_ServerSession_Imp::handle_error(NetLib_Error error, int error_code) 
{
	boost::system::error_code ignored_error;
	m_keep_alive_timer.cancel(ignored_error); //��Ȼ����������Ҳ��cancel�����ǻᵼ��ServerSession���ͷ�ʱ���session_detail�ٺþã���Ϊ��һ��ServerSession��keepalive��timer��

	boostioservice.post(boost::bind(&NetLib_ServerSession_Delegate::DisconnectedHandler, theDelegate, shared_from_this(), error, error_code));	
}

void NetLib_ServerSession_Imp::decrease_pending_ops_count() //������ÿ��pending_op��������
{	
	boost::lock_guard<boost::recursive_mutex> lock(session_mutex);

	m_pending_ops_count --;
	//LOGEVENTL("pending_ops_count", _ln("decrease_to") << m_pending_ops_count << _ln("ptr") << hex((std::size_t)this));
	if (m_pending_ops_count == 0)
	{
		//��ʱ�����session��pending_ops�Ѿ�������������		
		//post��Ϊ�˳���, ͬʱҲʹ��Server Stop��ʱ��iterator����close��һ���͵���ʧЧ
		boostioservice.post(boost::bind(&NetLib_ServerSession_Imp::disconnected, this)); //post��Ϊ�˳��׳��������ܵ��������滹��û����
	}
}

void NetLib_ServerSession_Imp::SendFinishHandler(char* data, void* pHint)
{
	theDelegate->SendFinishHandler(shared_from_this(), data, pHint);
}

void NetLib_ServerSession_Imp::SendCopyFinishHandler(char* data, void* pHint)
{
	theDelegate->SendCopyFinishHandler(shared_from_this(), data, pHint);
}

//'data' will be released just after RecvFinishHandler returns
void NetLib_ServerSession_Imp::RecvFinishHandler(char* data) //��Щ�������ϲ㶼��֤�˱�����������;�ͷ�
{
	refresh_timeout_timer();

	if (*(uint32_t*)data == 4)
	{
		/*if (m_flags & NETLIB_SERVER_LISTEN_KEEP_ALIVE_EVENT)
		{
			theDelegate->RecvKeepAliveHandler(shared_from_this());
		}*/
	}
	else
	{
		theDelegate->RecvFinishHandler(shared_from_this(), data);
	}
}

void NetLib_ServerSession_Imp::SendFailedHandler(const char* data, void* pHint)
{
	theDelegate->SendFailedHandler(shared_from_this(), data, pHint);
}

bool NetLib_ServerSession_Imp::SendCopyFailedHandler(const char* data_copy, void* pHint)
{
	theDelegate->SendCopyFailedHandler(shared_from_this(), data_copy, pHint);
	return false;
}