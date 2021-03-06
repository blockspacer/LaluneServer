#ifndef __CONTROL_SERVER_H_
#define __CONTROL_SERVER_H_

#include <string>
#include "include/ioservice_thread.h"
#include "commonlib/CommonLib.pb.h"
#include "NetLib/NetLib.h"
#include <boost/asio.hpp>
#include "ControlServerConfig.h"
#include "include/utility1.h"
#include "include/utility2.h"

extern ioservice_thread thread;

extern std::string config_file_path;
void LoadConfig();

extern bool during_startup;

extern AvailableIDs<uint32_t> available_ids;

void ServerTimeout(IPPort ip_port, const boost::system::error_code& error);

class ServerInfo
{
public:
	common::AddressInfo addr;

	ServerInfo(const std::pair<int, int>& ip_port, int server_type) : _timer(thread.get_ioservice())
	{
		addr.set_ip(ip_port.first);
		addr.set_port(ip_port.second);
		addr.set_server_id(available_ids.getId());
		addr.set_server_type(server_type);
		
		refresh();
	}

	void refresh()
	{
		_timer.expires_from_now(boost::posix_time::seconds(config.timeout_sec));
		_timer.async_wait(boost::bind(&ServerTimeout, std::make_pair(addr.ip(), addr.port()), boost::asio::placeholders::error));
	}

	virtual ~ServerInfo()
	{
		LOGEVENTL("Debug", "ServerInfo deconstruction.");
	}

protected:
	boost::asio::deadline_timer _timer;
};

extern std::map<IPPort, NetLib_ServerSession_ptr> server2session;
extern std::map<IPPort, ServerInfo*> servers_info;

typedef std::set<IPPort> server_group;
extern std::map<int, server_group*> server_groups;

void GenerateAddressList(common::AddressList& list);

template<typename P>
void informAddressInfo(P addr_list, int msg_type)
{
	for (auto session : server2session)
	{
		ReplyMsg(session.second, msg_type, addr_list);
	}
}

#endif