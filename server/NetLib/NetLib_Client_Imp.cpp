#include "NetLib_Client_Imp.h"
#include "NetLib_Connected_TcpSession.h"
#include <boost/bind.hpp>
#include <boost/thread/locks.hpp>
#include <boost/array.hpp>
#include "NetLib_Params.h"
#include "Log/Log.h"

NetLib_Client_Imp::NetLib_Client_Imp(std::shared_ptr<NetLib_Client_Delegate> d, boost::asio::io_service & ioservice) : theDelegate(d), boostioservice(ioservice), tcpsocket(ioservice), 
	m_last_error(no_error), m_last_internal_error(0), 
	m_enable_reconnect(true), m_manually_disconnect(false), m_will_reconnect_if_disconnected(false), m_in_disconnect_process(false),
	m_reconnect_interval_ms(5000), m_max_continuous_retries(-1), m_currently_retries(0), reconnect_retry_timer(ioservice), keep_alive_timer(ioservice), m_keepalive_interval_seconds(240)
{
}

NetLib_Client_Imp::~NetLib_Client_Imp() //���ⲿ��֤��������á�(post����)
{										//��Ҫ������õ�ԭ���������лص�FailedDataReleaseHandler��
	//֮���Բ�ֱ�ӵ���ResetFailedData()����Ϊ�˱��������post��
	while (! failed_data_queue.empty())
	{
		if (failed_data_queue.front().copy)
		{
			delete[] failed_data_queue.front().data;
		}
		else
		{
			theDelegate->FailedDataReleaseHandler(failed_data_queue.front().data, failed_data_queue.front().pHint);
		}
		failed_data_queue.pop();
	}
}

void NetLib_Client_Imp::ResetFailedData()
{		
	boost::lock_guard<boost::recursive_mutex> lock(client_mutex);

	while (! failed_data_queue.empty())
	{
		if (failed_data_queue.front().copy)
		{
			delete[] failed_data_queue.front().data;
		}
		else
		{
			boostioservice.post(boost::bind(&NetLib_Client_Delegate::FailedDataReleaseHandler, theDelegate.get(), shared_from_this(), failed_data_queue.front().data, failed_data_queue.front().pHint));
			//��ʱ������Ѿ���NetLib_Client_Imp�������������ˣ�����Ҳ����shared_from_this()��
			//ʹ���������Handler���������Ҳ������һ��delete[] data��
		}
		failed_data_queue.pop();
	}
}

boost::recursive_mutex& NetLib_Client_Imp::get_mutex()
{
	return client_mutex;
}

void NetLib_Client_Imp::Disconnect()
{	
	std::shared_ptr<NetLib_Connected_TcpSession> connected_session_keep_alive = connected_session;

	boost::lock_guard<boost::recursive_mutex> lock(client_mutex);
	
	m_manually_disconnect = true; //Disconnect���Բ���ϱ��ε��������̣������ٲ������������������ˡ�������ܳ��֡���ͣʧ�ܲ�ͣ����ͣ��ͣ�����������
	m_will_reconnect_if_disconnected = false;

	bool perform_disconnect = false;
	if (!m_in_disconnect_process) //�������Զ�������ʱ�����Disconnect()���������ָ����߼�
	{
		if (connected_session_keep_alive && connected_session_keep_alive->is_connected() )
		{
			perform_disconnect = true;
			connected_session_keep_alive->disconnect();
		}
	}
	else
	{
		LOGEVENTL("NetLib_Info", "the client you call Disconnect() is already disconnecting.");
	}

	if (!perform_disconnect)
	{
		handle_error(client_cancel_connect_by_local, 0);

		boost::system::error_code ignored_error;

		tcpsocket.close(ignored_error);

		if (ignored_error)
		{
			LOGEVENTL("Warn", "when close a un-connected tcp client, a error occurs: " << ignored_error.value());
		}

		reconnect_retry_timer.cancel(ignored_error);
	}
}

bool NetLib_Client_Imp::IsConnected()
{	
	std::shared_ptr<NetLib_Connected_TcpSession> connected_session_keep_alive = connected_session;

	if (connected_session_keep_alive)
	{
		return connected_session_keep_alive->is_connected();
	}
	return false;
}

void NetLib_Client_Imp::reconnect_timer_pulse(std::shared_ptr<NetLib_Client_Imp> keep_alive, const boost::system::error_code& error)
{
	if (!error) //
	{		
		boost::lock_guard<boost::recursive_mutex> lock(client_mutex);
		m_currently_retries ++;
		connect_async();
	}
}

//ͬһʱ�䣬ֻ����һ���߳̽���disconnected�ķ����塣ͨ��bool m_in_disconnect_process����֤��
void NetLib_Client_Imp::disconnected(std::shared_ptr<NetLib_Client_Imp> keep_alive)
{
	//LOGEVENTL("disconnected", log_::n("ptr") << hex((std::size_t)keep_alive.get()) << log_::n("delegate") << hex((std::size_t)theDelegate.get()));

	//�������������ⲿ��֤�̰߳�ȫ���ⲿ���뱣֤ͬһʱ�䱾�����漰�ı�������󶼲����޸ģ������޸���Ҳ��Ӱ�졣ҲҪ��֤���������������޸Ķ���粻��������Ԥ�ڵ�Ӱ�졣
	//������pending_op == 0�˲Ž��������Щ�첽�����϶�����ͱ������໥Ӱ���ˡ�ֻҪ��֤�ֶ����õķ����ͱ��������ڵĴ��벻��ͻ���ɡ�����Ҳ����˵���ͱ������϶������໥Ӱ��ķ��������Բ�ʹ��incre/decre pending_op��һ��

	bool will_continue_reconnect = m_will_reconnect_if_disconnected && (m_max_continuous_retries == -1 || m_currently_retries < m_max_continuous_retries); //��������������������δ���������������

	if (m_currently_retries == 0) //�ն��ߣ���û��������
	{
		theDelegate->DisconnectedHandler(shared_from_this(), m_last_error, m_last_internal_error, will_continue_reconnect); 

		if (connected_session)
		{
			connected_session->process_failed_packets(); 
		}
		else
		{
			//���������ʵ�����׷�����ConnectAsyncû�ɹ��ͻ�������
		}
	}
	else //�Ѿ��������������ˣ����������ʧ��
	{
		//LOGEVENTL("_reconn_fail", log_::n("ptr") << hex((std::size_t)keep_alive.get()) << _ln("error") << m_last_error << _ln("internal_error") << m_last_internal_error);

		theDelegate->ReconnectFailedHandler(shared_from_this(), will_continue_reconnect); //���Handler���ܻ��޸�will_continue_reconnect��ֵ
	}
	
	//LOGEVENTL("disconnected_half", log_::n("ptr") << hex((std::size_t)keep_alive.get()) << log_::n("connected_session") << hex((std::size_t)connected_session.get()));

	connected_session.reset(); //cut the relationship between the session and its owner. otherwise the shared_ptrs formed a cycle and can't release.
	
	//���Client���ֶ�Disconnect��Release����m_will_reconnect_if_disconnected��false�������ٴ�����
	//m_will_reconnect_if_disconnected��ǿ��������ʹwill_continue_reconnect�����ˣ����m_will_reconnect_if_disconnected��false��Ҳû����
	if (m_will_reconnect_if_disconnected && will_continue_reconnect)
	{
		if (m_currently_retries || (m_currently_retries == 0 && (m_flags & NETLIB_CLIENT_ENABLE_RECONNECT_ON_FIRST_CONNECT)))
		{
			//LOGEVENTL("Info", "will reconnect later");
			//���״ζ��ߣ����ߵ�һ������ʧ��Ȼ����Ҫ����(�������ѡ��)����Ҫ��ʱһ��ʱ��������
			reconnect_retry_timer.expires_from_now(boost::posix_time::milliseconds(m_reconnect_interval_ms));
			reconnect_retry_timer.async_wait(boost::bind(&NetLib_Client_Imp::reconnect_timer_pulse, this, shared_from_this(), boost::asio::placeholders::error));
		}
		else //�ն��ߣ���û�����������Ҳ����״����ӣ������������
		{
			//LOGEVENTL("Info", "reconnect at once");
			m_currently_retries ++;
			connect_async();
			//connect_async�п��ܽ���decrease_pending_ops_count��п��ܻ�postһ��disconnected������ioserviceֻ��һ���߳�����ʱ����post�����disconnected��������ȵ�ǰ�ķ�����ִ������ܽ���
		}
	}
	else
	{
		m_in_disconnect_process = false;
		m_will_reconnect_if_disconnected = false; //��������
		m_currently_retries = 0; //�������Դ�������Ϊ�������Ҳ��������ʶ�Ƿ���������
	}

	//NetLib_Client_Imp��һ�����ܵ��ͷŵ㡣ֻҪ�����������ʱ�����ü���Ϊ0����Ϊǰ�����ǰ�connected_session��reset��

	//LOGEVENTL("disconnected_finish", log_::n("ptr") << hex((std::size_t)keep_alive.get()) << log_::n("delegate") << hex((std::size_t)theDelegate.get()));
}

void NetLib_Client_Imp::decrease_pending_ops_count() //������ÿ��pending_op�������ã���Ϊ���ܴ���connected_session.reset()��������client���ͷ�
{	
	boost::lock_guard<boost::recursive_mutex> lock(client_mutex);

	m_pending_ops_count --;
	//LOGEVENTL("pending_ops_count", "decrease to " << m_pending_ops_count);
	if (m_pending_ops_count == 0 && m_last_error != no_error) //�д���Ŵ���disconnected��û��error����m_pending_ops_count��Ϊ0�������������udp tcp���ӵ��л����м������0�����
	{
		m_in_disconnect_process = true; 

		//ֻ�н�������disconnected������ĩβ�Ὣm_in_disconnect_process��Ϊfalse. ������;��ConnectAsync���š�SendAsync��Disconnect����connected_session�ǶϿ�״̬���ǲ�������ִ�еġ�
		//ֻҪ�Ǽ�������������m_pending_ops_count�Ͳ����ٶ����ӣ�����ط�Ҳ�����ٴν������������Զ�������ʧ�ܿ��ܻ��ٽ����
		//���ֻҪ��֤ioserviceֻ��һ���߳����ܣ�disconnected������ͬһʱ��ֻ���һ�Ρ�
		boostioservice.post(boost::bind(&NetLib_Client_Imp::disconnected, this, shared_from_this())); //post��Ϊ�˳��׳��������ܵ��������滹��û����
	}
}

//���еĶ��ߡ�����ʧ�ܶ��������������մ���DisconnectedHandler�����������decrease_pending_ops_count����Ҫ��֤ÿ��handle_error()����decrease_pending_ops_count���á�
void NetLib_Client_Imp::handle_error(NetLib_Error error, int error_code)  //as long as connected_sessions is connected, the session_container is kept alive
{	
	boost::lock_guard<boost::recursive_mutex> lock(client_mutex);

	//����������ܽ����
	//���ڸ÷��������е����߶���ֱ�ӻ��ӽ�NetLib_Client_Imp keep_alive��������������Ͳ���keep_alive��
	if (m_last_error == no_error) //ͨ�����������ֻ֤��һ�Ρ���m_last_error�������ٵ�����client��idle��(û��pending_ops��)������
	{
		m_last_error = error;
		m_last_internal_error = error_code;

		boost::system::error_code ignored_error;
		keep_alive_timer.cancel(ignored_error);
	}
}

void NetLib_Client_Imp::send_keep_alive(std::shared_ptr<NetLib_Client_Imp> keep_alive, const boost::system::error_code& error) //send_keep_alive��shared_ptr��keep_alive��ȫ����һ����
{
	if (!error)
	{
		char send_buf[4];
		*(uint32_t*)send_buf = 4;
		SendCopyAsync(send_buf);

		send_keep_alive_in_future();
	}
	else
	{
		decrease_pending_ops_count();
	}
}

void NetLib_Client_Imp::send_keep_alive_in_future()
{
	boost::system::error_code ignored_error;
	keep_alive_timer.expires_from_now(boost::posix_time::seconds(m_keepalive_interval_seconds), ignored_error);
	keep_alive_timer.async_wait(boost::bind(&NetLib_Client_Imp::send_keep_alive, this, shared_from_this(), boost::asio::placeholders::error));
}

void NetLib_Client_Imp::connected_handler() //�÷���ֻ�������ڵ���
{	
	if (m_enable_reconnect)
	{
		m_will_reconnect_if_disconnected = true; //��Ҫ������m_will_reconnect_if_disconnected�ĵط���
	}

	if (m_currently_retries == 0) //��ʾ���ⲿ����ConnectAsync�����ӳɹ�
	{		
		boostioservice.post(boost::bind(&NetLib_Client_Delegate::ConnectedHandler, theDelegate.get(), shared_from_this())); //���滹����������ֻ��ͨ��post��������
	}
	else //��ʾ�Զ������ɹ�
	{		
		LOGEVENTL("NetLib_Info", "(" << hex((std::size_t)this) << ") NetLib_Client reconnected."); 

		m_in_disconnect_process = false;
		m_currently_retries = 0;
		boostioservice.post(boost::bind(&NetLib_Client_Delegate::ReconnectedHandler, theDelegate.get(), shared_from_this())); //���滹����������ֻ��ͨ��post��������
	}

	if (connected_session)
	{
		if (! failed_data_queue.empty())
		{			
			//����֮ǰʧ�ܵ�����
			LOGEVENTL("NL_ResendFailedData", log_::n("ptr") << hex((std::size_t)this) << log_::n("queue_size") << failed_data_queue.size());

			do
			{
				connected_session->send_async(failed_data_queue.front().data, failed_data_queue.front().copy, failed_data_queue.front().pHint); //SendFinish��SendFailed���Connected�ٴ�����ֻҪioservice������ִ�ж����е������
				failed_data_queue.pop();
			} while (! failed_data_queue.empty());
		}

		//�����Ҫkeep_alive����������ʱ��
		if (m_flags & NETLIB_CLIENT_FLAG_KEEP_ALIVE)
		{
			increase_pending_ops_count();
			send_keep_alive_in_future();
		}
	}
	else
	{
		LOGEVENTL("Error", "ConnectedSession is empty in connected_handler. Client ptr: " << log_::h((std::size_t)this));
	}
}

void NetLib_Client_Imp::tcp_connect_async()
{	
	//LOGEVENTL("NetLib_Trace", "tcp_connect_async");
	boost::lock_guard<boost::recursive_mutex> lock(client_mutex);
	try
	{
		boost::asio::ip::tcp::endpoint endpoint;
		if (m_dest_ip_u == 0)
		{
			endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(m_dest_ip), m_tcp_port);
		}
		else
		{
			endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v4(m_dest_ip_u), m_tcp_port);
		}

		increase_pending_ops_count();

		tcpsocket.async_connect(endpoint, 
			boost::bind(&NetLib_Client_Imp::tcp_connected_handler, this, shared_from_this(), boost::asio::placeholders::error));
	}
	catch (std::exception e)
	{		
		LOGEVENTL("Debug", "tcpsocket.async_connect throws exception: " << e.what());

		std::shared_ptr<NetLib_Client_Imp> keep_alive = shared_from_this(); //why this? 11.28
		handle_error(client_connect_error_std_ex, 0);

		decrease_pending_ops_count();
	}
}

void NetLib_Client_Imp::tcp_connected_handler(std::shared_ptr<NetLib_Client_Imp> keep_alive, const boost::system::error_code& error)
{	
	boost::lock_guard<boost::recursive_mutex> lock(client_mutex);
	if (error)
	{
		handle_error(client_connect_error, error.value());
	}
	else
	{		
		if (m_manually_disconnect) //�����ϣ�ȴ�����Ѿ����ֶ��Ͽ��ˣ��͵���û���������������Ӧ���Ѿ�������client_cancel_connect_by_local��
		{
			if (m_last_error != client_cancel_connect_by_local)
			{
				LOGEVENTL("Error", "UNEXPECTED. m_last_error got: " << m_last_error << ", expected client_cancel_connect_by_local(" << client_cancel_connect_by_local << ")");
			}
		}
		else
		{
			if (m_flags & NETLIB_FLAG_TCP_NODELAY)
			{
				boost::asio::ip::tcp::no_delay option(true); 
				boost::system::error_code ignored_error;
				tcpsocket.set_option(option, ignored_error); 
			}

			boost::asio::socket_base::receive_buffer_size option(CLIENT_RECV_BUFFER_SIZE);
			boost::system::error_code error4recv_buf;
			tcpsocket.set_option(option, error4recv_buf);
			if (error4recv_buf)
			{
				LOGEVENTL("Warn", "(tcp client) set_option(receive_buffer_size) error: " << error4recv_buf.value());
			}

			if (connected_session && connected_session->is_connected())
			{
				LOGEVENTL("Fatal", "tcp_connected_handler was triggered when member class connected_session are still connected.");
			}
			else //��δ���ӣ�����һ��connected_session�ѶϿ�
			{
				connected_session = std::shared_ptr<NetLib_Connected_TcpSession> (new NetLib_Connected_TcpSession(shared_from_this(), boostioservice, tcpsocket));
				connected_session->start();

				connected_handler();
			}
		}
	}
	decrease_pending_ops_count();
}

void NetLib_Client_Imp::connect_async()
{
	//���ﲻ�ı���m_will_reconnect_if_disconnected��ֵ����Ϊ�״��������������ܽ����

	boost::lock_guard<boost::recursive_mutex> lock(client_mutex);

	if (m_manually_disconnect)
	{
		LOGEVENTL("NetLib_Debug", "will connect_async, but m_manually_disconnect is true. nothing will be done.");
		//��������������Զ������ļ���У��ⲿ������Disconnect�ˡ����Ҳ���������ˡ�m_last_errorӦ����һ�ε�error��������client_cancel_connect_by_local
	}
	else
	{
		m_last_error = no_error;
		m_last_internal_error = 0;

		tcp_connect_async();
	}
}

void NetLib_Client_Imp::_connect_async(const char* ip_s, uint32_t ip_u, uint16_t port, uint64_t flags)
{
	std::shared_ptr<NetLib_Client_Imp> keep_alive = shared_from_this(); //�����������ʱ��this�����ڣ����ܽ�����this�Ѿ��ͷ��ˡ�
	boost::lock_guard<boost::recursive_mutex> lock(client_mutex);
	if (m_pending_ops_count != 0 || m_in_disconnect_process)
	{
		LOGEVENTL("Error", "Don't call ConnectAsync on a non-idle client ! Nothing happened.");
	}
	else
	{
		if (ip_s)
		{
			LOGEVENTL("NetLib_Info", log_::n("ptr") << log_::h((std::size_t)this) << ", ConnectAsync: "  //ֱ�Ӵ�ָ��gcc���Լ���0x���Ͳ�ͳһ�ˣ�����ת���ٴ�
				<< log_::n("ip") << ip_s << log_::n("port") << port << log_::n("flags") << log_::h(flags));

			m_dest_ip = ip_s;
			m_dest_ip_u = 0;
		}
		else
		{
			LOGEVENTL("NetLib_Info", log_::n("ptr") << log_::h((std::size_t)this) << ", ConnectAsync: "  //ֱ�Ӵ�ָ��gcc���Լ���0x���Ͳ�ͳһ�ˣ�����ת���ٴ�
				<< log_::n("ip") << ip_u << log_::n("port") << port << log_::n("flags") << log_::h(flags));

			m_dest_ip_u = ip_u;
		}
		m_manually_disconnect = false;

		m_tcp_port = port;
		m_flags = flags;
		if (m_flags & NETLIB_CLIENT_ENABLE_RECONNECT_ON_FIRST_CONNECT)
		{
			m_will_reconnect_if_disconnected = true; //�״�����Ҳ��������
		}
		else
		{
			m_will_reconnect_if_disconnected = false; //�״����ӣ�Ĭ�ϲ����������������
		}

		connect_async();
	}
}

void NetLib_Client_Imp::ConnectAsync(const char* ip, uint16_t port, uint64_t _flags)
{	
	_connect_async(ip, 0, port, _flags);
}

void NetLib_Client_Imp::ConnectAsync(uint32_t ip, uint16_t port, uint64_t _flags)
{
	_connect_async(nullptr, ip, port, _flags);
}

void NetLib_Client_Imp::SendAsyncFailed(std::shared_ptr<NetLib_Client_Imp> keep_alive, const char* data, void* pHint)
{
	SendFailedHandler(data, pHint);
}

void NetLib_Client_Imp::SendCopyAsyncFailed(std::shared_ptr<NetLib_Client_Imp> keep_alive, const char* data_copy, void* pHint)
{
	if (!SendCopyFailedHandler(data_copy, pHint))
	{
		delete[] data_copy; //��Handler����false��ʱ�򣬰Ѹ�new������copyɾ��
	}
}

void NetLib_Client_Imp::SendAsync(const char* data, void* pHint)
{	
	//ԭ��������Client sharedһ�ݣ����������ò���Ҫ��ȥ���ˡ����������̲߳���ͬһ��shared_ptr����һ���߳���SendAsync����һ���߳�ͬʱreset�����shared_ptr�����������Ż������
	client_mutex.lock();
	if (connected_session && connected_session->is_connected())
	{
		connected_session->send_async(data, false, pHint);
		client_mutex.unlock();
	}
	else
	{
		client_mutex.unlock();
		boostioservice.post(boost::bind(&NetLib_Client_Imp::SendAsyncFailed, this, shared_from_this(), data, pHint)); //Handler����ֱ�ӻ��ӵľ���post���ܵ��ã������п��ܵ�������
	}
}

void NetLib_Client_Imp::SendCopyAsync(const char* data, void* pHint)
{	
	//ԭ��������Client sharedһ�ݣ����������ò���Ҫ��ȥ���ˡ����������̲߳���ͬһ��shared_ptr����һ���߳���SendCopyAsync����һ���߳�ͬʱreset�����shared_ptr�����������Ż������

	//���������ȿ���һ�ݡ���Ϊ��ʹ�ǽ������else�����copyҲ���ܻᱻʹ�ã���Ϊ�в��뵽failed_data_queue�Ŀ���
	char* data_copy = new char[*(uint32_t*)data];
	memcpy(data_copy, data, *(uint32_t*)data);

	client_mutex.lock();
	if (connected_session && connected_session->is_connected())
	{
		connected_session->send_async(data_copy, true, pHint);
		client_mutex.unlock();
	}
	else
	{
		client_mutex.unlock();
		boostioservice.post(boost::bind(&NetLib_Client_Imp::SendCopyAsyncFailed, this, shared_from_this(), data_copy, pHint)); //Handler����ֱ�ӻ��ӵľ���post���ܵ��ã������п��ܵ�������
	}
}

void NetLib_Client_Imp::SendFailedHandler(const char* data, void* pHint)
{
	if (theDelegate->SendFailedHandler(shared_from_this(), data, pHint))
	{	
		//LOGEVENTL("NetLib_Trace", "SendFailed and Push into failed_data_queue. data: 0x" << std::hex << (std::size_t)data);

		boost::lock_guard<boost::recursive_mutex> lock(client_mutex);		

		failed_data_queue.push( netlib_packet(data, false, pHint));

		//LOGEVENTL("NetLib_Trace", "failed_data_queue.back().data: 0x" << std::hex << (std::size_t)failed_data_queue.back().data);
	}
}

bool NetLib_Client_Imp::SendCopyFailedHandler(const char* data_copy, void* pHint)
{
	if (theDelegate->SendCopyFailedHandler(shared_from_this(), data_copy, pHint))
	{			
		//LOGEVENTL("NetLib_Trace", "SendCopyFailed and Push into failed_data_queue. data_copy: 0x" << std::hex << (std::size_t)data_copy);
		boost::lock_guard<boost::recursive_mutex> lock(client_mutex);
		failed_data_queue.push( netlib_packet(data_copy, true, pHint));
		return true;
	}
	else
	{
		return false;
	}
}

//������ǰ��
void NetLib_Client_Imp::DisableReconnect()
{
	boost::system::error_code ignored_error;

	boost::lock_guard<boost::recursive_mutex> lock(client_mutex);

	reconnect_retry_timer.cancel(ignored_error);
	m_enable_reconnect = false;
	m_will_reconnect_if_disconnected = false;	
	m_max_continuous_retries = 0;
}

//EnableReconnect����Ҫ������ǰ�ã�����֮�������þ�����
void NetLib_Client_Imp::EnableReconnect(int reconnect_interval_ms, int max_continuous_retries)
{
	boost::lock_guard<boost::recursive_mutex> lock(client_mutex);
	m_enable_reconnect = true;
	m_reconnect_interval_ms = reconnect_interval_ms;
	m_max_continuous_retries = max_continuous_retries;
}

boost::asio::io_service* NetLib_Client_Imp::GetWorkIoService()
{
	return &boostioservice;
}

void NetLib_Client_Imp::SetKeepAliveIntervalSeconds(int keepalive_interval_seconds)
{
	m_keepalive_interval_seconds = keepalive_interval_seconds;
}