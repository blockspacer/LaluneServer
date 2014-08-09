#ifndef _NETLIB_SESSION_OWNER_H_
#define _NETLIB_SESSION_OWNER_H_

#include <memory>
#include "NetLib_Error.h"
#include <boost/thread/recursive_mutex.hpp>

class NetLib_Session_Owner //session usually maintain a share_ptr to session_owner
{
	friend class NetLib_Connected_TcpSession;
	friend class NetLib_Client_Imp;
protected:
	virtual boost::recursive_mutex& get_mutex() = 0;
	
	int m_pending_ops_count;

	void increase_pending_ops_count()  //�÷��������������ⲿ��֤������
	{
		m_pending_ops_count ++;
	} 
	virtual void decrease_pending_ops_count() {}
	virtual void handle_error(NetLib_Error error, int internal_error_code) = 0;

	//��������������õ�ʱ��֤�˲������owner��ʵ������ֱ�ӵ����ϲ��delegate�Ϳ���
	virtual void SendFinishHandler(char* data, void* pHint) = 0; //���лᱻpostִ�еķ�����Ҫkeep_aliveһ�ݱ�����
	virtual void SendCopyFinishHandler(char* data, void* pHint) = 0; //SendCopyFinish���������ᱻpost����������һ��ᱻpostִ�С�

	//'data' will be released just after RecvFinishHandler returns
	virtual void RecvFinishHandler(char* data) = 0;

	virtual void SendFailedHandler(const char* data, void* pHint) = 0;
	virtual bool SendCopyFailedHandler(const char* data_copy, void* pHint) = 0; //����false���ʾdata_copy����ɾ��

public:
	NetLib_Session_Owner() : m_pending_ops_count(0) {}
};
#endif