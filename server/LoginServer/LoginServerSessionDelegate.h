#ifndef __LOGIN_SERVER_SESSION_DELEGATE_H_
#define __LOGIN_SERVER_SESSION_DELEGATE_H_

#include "NetLib/NetLib.h"

class LoginServerSessionDelegate : public NetLib_ServerSession_Delegate
{
public:
	void RecvFinishHandler(NetLib_ServerSession_ptr sessionptr, char* data);
};

#endif