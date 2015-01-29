#ifndef __Boids_AutoMatchServer_h_
#define __Boids_AutoMatchServer_h_

#include "NetLib/NetLib.h"
#include "battle.pb.h"
#include "boost/asio/deadline_timer.hpp"
#include "Timer.h"
#include <map>

struct IpPort
{
	std::string Ip;
	int port;

	IpPort()
	{
	}

	IpPort(const std::string& ip_, int port_) : Ip(ip_), port(port_)
	{
	}

	bool operator == (const IpPort& rhs)
	{
		return Ip == rhs.Ip && port == rhs.port;
	}

	bool operator < (const IpPort& rhs)
	{
		return Ip < rhs.Ip || Ip == rhs.Ip && port < rhs.port;
	}
};

struct PvPServerInfo
{
	IpPort id;
	NetLib_ServerSession_ptr session;
	std::shared_ptr<OneOffTimer> timer;
	int region;
	int priority;

	PvPServerInfo()
	{
	}
};

typedef std::vector<PvPServerInfo> ServerList;

typedef std::pair<int, std::string> MatchKey; //region, map_name
typedef std::pair<NetLib_ServerSession_ptr, std::string> WaitingUser; //session, user_id

struct GameInfo
{
	IpPort server;
	NetLib_ServerSession_ptr sessions[2]; //�������2�˾֣��˶������鿪���ͺ�
};

class AutoMatchServer
{
protected:
	std::map<int, ServerList*> servers_by_region; //�����ҷ�����
	std::map<MatchKey, WaitingUser> waiting_users; //����ƥ�䡣�Ժ�����а�����ƥ���ˣ������Ҫ�ģ�����������ʱ����
	std::map<std::string, GameInfo> games; //���ڴ�����Ϸ�ɹ������Ϸ�����Ϣ

public:
	~AutoMatchServer();

	//�����û�
	void MatchRequest(NetLib_ServerSession_ptr sessionptr, const boids::MatchRequest& user_req);

	//����ս������
	void CreateGameResponseGot(const boids::CreateGameResponse& res);
	void ServerRegister(NetLib_ServerSession_ptr sessionptr, const boids::PvPServerRegister& reg);
	void ServerHeartBeat(NetLib_ServerSession_ptr sessionptr, const boids::PvPServerHeartBeat& beat);
};

extern AutoMatchServer ams;

#endif