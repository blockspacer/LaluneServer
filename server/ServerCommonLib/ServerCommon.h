#ifndef __SERVER_COMMON_H
#define __SERVER_COMMON_H

//������

#include <cstdint>
#include <map>

//����˰�ͷ����

#define SERVER_MSG_LENGTH(d) (*(uint32_t*)(d))
#define SERVER_MSG_TYPE(d) (*(uint32_t*)((d) + 4))
#define SERVER_MSG_ERROR(d) (*(uint8_t*)((d) + 8)) //��˷����ǰ�˷��񷵻صĴ�������д�����ôĬ�Ϻ����Protobuf��û���ˣ���������Լ��
#define SERVER_MSG_HEADER_EX_LEN(d) (*(uint8_t*)((d) + 9))
#define SERVER_MSG_RESERVED(d) (*(uint16_t*)((d) + 10))
#define SERVER_MSG_HEADER_EX(d) (*(uint16_t*)((d) + 12))
#define SERVER_MSG_DATA(d) (*(uint16_t*)((d) + 12 + MSG_HEADER_EX_LEN(d)))

#include "../protobuf/commonlib/HeaderEx.pb.h"

void UpdateAddrInfo(uint64_t user_id, const HeaderEx& header_ex);

enum SendFlag
{
	RequireAddrInfo = 0,
	OnlyUserId,
	NoHeaderEx
};

template<typename P>
bool SendMsg(uint32_t msg_type, uint64_t related_user_id, P proto, SendFlag flag = SendFlag::RequireAddrInfo)
{
	switch (flag)
	{
		case RequireAddrInfo:

	}

	//TOMODIFY
	int pb_size = proto.ByteSize();

	char* send_buf = new char[CMDEX0_HEAD_SIZE + pb_size];
	CMD_SIZE(send_buf) = CMDEX0_HEAD_SIZE + pb_size;					// ���ݰ����ֽ�������msghead��
	CMD_FLAG(send_buf) = 0;							// ��־λ����ʾ�����Ƿ�ѹ�������ܵ�
	CMD_CAT(send_buf) = CAT_LOGSVR;				// �������
	CMD_ID(send_buf) = CmdID;
	int tid = new_trans_id();
	CMDEX0_TRANSID(send_buf) = tid;
	{
		boost::lock_guard<boost::mutex> lock(_log_query_result_handlers_mutex);
		result_handlers[tid] = rh;
	}

	pb.SerializeWithCachedSizesToArray((google_lalune::protobuf::uint8*)CMDEX0_DATA(send_buf));

	std::shared_ptr<NetLibPlus_Client> ls_client = NetLibPlus_get_first_Client(__LS_ServerTypeNameForQuery.c_str());

	if (ls_client)
	{
		ls_client->SendAsync(send_buf);
	}
	else
	{
		delete[] send_buf;
	}
}

#endif