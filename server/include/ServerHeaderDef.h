#ifndef __Boids_Server_Header_def_h_
#define __Boids_Server_Header_def_h_

//����˰�ͷ����

#include <cstdint>
#include "boids.pb.h"
#include "NetLib/NetLib.h"
#include "Log/Log.h"

#define SHOW_PACKET
//#define SHOW_PACKET LOGEVENTL("DEBUG", "Receive: " << log_::bytes_display(data, MSG_LENGTH(data)));

#define MSG_LENGTH(d) (*(uint32_t*)(d))
#define MSG_DATA(d) ((uint32_t*)(d + 4))
#define MSG_HEADER_LEN (4)
#define MSG_DATA_LEN(d) (MSG_LENGTH(d) - MSG_HEADER_LEN)

#define PARSE_EXECUTE(DATA, PROTO, FUNC) \
{ \
	PROTO __p; \
	if (__p.ParseFromString(DATA)) { \
		FUNC(__p); \
	}\
	else \
	{ \
		LOGEVENTL("Error", "Parse " << #PROTO << " failed"); \
	} \
}

//�������и�sessionptr����
#define PARSE_EXECUTE_SESSION(DATA, PROTO, FUNC) \
{ \
	PROTO __p; \
	if (__p.ParseFromString(DATA)) { \
		FUNC(sessionptr, __p); \
	}\
	else \
	{ \
		LOGEVENTL("Error", "Parse " << #PROTO << " failed"); \
	} \
}

#define UNRECOGNIZE(title, t) LOGEVENTL("ERROR", title << ": unrecognized msg type: " << t);
#define MSG_TOO_SHORT(title, len) LOGEVENTL("ERROR", title << ": msg too short, got: " << len << ", expect at least: " << MSG_HEADER_LEN);

//��������˻��и�data����
#define BEGIN_HANDLER \
void RecvFinishHandler(NetLib_ServerSession_ptr sessionptr, char* data) { \
	SHOW_PACKET; \
	if (MSG_LENGTH(data) >= MSG_HEADER_LEN) \
	{ \
		boids::BoidsMessageHeader __msg; \
		if (__msg.ParseFromArray(MSG_DATA(data), MSG_DATA_LEN(data))) \
		{ \
			switch (__msg.type()) \
			{

//��������˻��и�data����
#define HANDLE_MSG(T, PROTO, FUNC) \
			case T: \
				PARSE_EXECUTE(data, PROTO, FUNC); \
				break;

//��������˻��и�data������sessionptr����
#define HANDLE_MSG_SESSION(T, PROTO, FUNC) \
			case T: \
				PARSE_EXECUTE_SESSION(data, PROTO, FUNC); \
				break;

//�����BEGIN_SWITCH����ʹ��
#define END_HANDLER(classname) \
			default: \
				UNRECOGNIZE(#classname, __msg.type()); \
				break; \
			} \
		} \
	} \
	else \
	{ \
		MSG_TOO_SHORT(#classname, MSG_LENGTH(data)); \
	} \
}

template<typename P>
void ReplyMsg(NetLib_ServerSession_ptr sessionptr, boids::MessageType msg_type, P& proto) //��ͷ��UserID�İ汾
{
	boids::BoidsMessageHeader proto_with_header;
	proto_with_header.set_type(msg_type);
	proto.SerializeToString(proto_with_header.mutable_data());

	int proto_size = proto_with_header.ByteSize();

	char* send_buf = new char[MSG_HEADER_LEN + proto_size];
	MSG_LENGTH(send_buf) = MSG_HEADER_LEN + proto_size;

	proto_with_header.SerializeWithCachedSizesToArray((google_lalune::protobuf::uint8*)MSG_DATA(send_buf));

	sessionptr->SendAsync(send_buf);
}

#endif