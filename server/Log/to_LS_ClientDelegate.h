#pragma once

bool ProcessLogOptions(const char* pb_data, int pb_size); //����ȷ�����л���return true

#ifndef _LOG_DLL_

#include "NetLib/NetLib.h"
#include "internal.h"

class to_LS_ClientsDelegate : public NetLibPlus_Client_Delegate
{
public:
	void RecvFinishHandler(std::shared_ptr<NetLibPlus_Client> clientptr, char* data);

	void ReconnectedHandler(std::shared_ptr<NetLibPlus_Client> clientptr);
};

#endif