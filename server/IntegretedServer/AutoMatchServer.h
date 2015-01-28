#ifndef __Boids_AutoMatchServer_h_
#define __Boids_AutoMatchServer_h_

#include "battle.pb.h"
#include "boost/asio/deadline_timer.hpp"

struct IpPort
{
	std::string Ip;
	int port;
};

class PvPServerInfo
{
	boost::asio::deadline_timer timer;
	int region;
	int priority;
};

class AutoMatchServer
{
protected:
	
public:
	//�����û�
	void MatchRequest(const boids::MatchRequest& user_req);

	//����ս������
	void CreateGameResponseGot(const boids::CreateGameResponse& res);
	void ServerRegister(const boids::PvPServerRegister& reg);
	void ServerHeartBeat(const boids::PvPServerHeartBeat& beat);
};

extern AutoMatchServer ams;

#endif