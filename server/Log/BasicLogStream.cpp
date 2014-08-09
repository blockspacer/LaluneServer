#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/asio/placeholders.hpp>
#include "ptime2.h"
#include "../protobuf/src-gen/cpp/log/log.pb.h"
#ifndef _LOG_DLL_
#include "NetLib/NetLib.h"
#endif
#include "internal.h"
#include "BasicLogStream.h"
#include "HeaderDefine.h"

uint64_t __last_t = 0;
boost::mutex __last_t_mutex;

#ifdef _LOG_DLL_
std::shared_ptr<boost::asio::ip::udp::socket> udpsocket_4log;
boost::asio::ip::udp::endpoint udpendpoint_4log;
boost::mutex _udpsocket_mutex;
#endif

void BasicLogStream::RecordTime()
{	
	//������Ȳ�Ҫ����ô�ߣ���ȡʱ����������Ż�����һ����ʼֵ��GetTickCount��ӡ�(Ҳ�Ż���������)
	t = ptime2(boost::posix_time::microsec_clock::local_time()).get_u64();

	{
		boost::lock_guard<boost::mutex> lock(__last_t_mutex);
		if (t <= __last_t)
		{
			t = __last_t + 1;
		}
		__last_t = t;
	}
}

void BasicLogStream::LogToNet(const std::string& index3, const std::string& index1, const std::string& index2)
{
	kit::log::LogDetail detail_pb;
	for (auto it = lexical_entries.begin(); it != lexical_entries.end(); ++it)
	{			
		kit::log::LexicalEntry* entry = detail_pb.add_entry();
		if (it->has_name())
		{
			entry->set_entry_name(it->name.v);
		}
		entry->set_value_type(it->value_type);
		switch (it->value_type)
		{
			case LexicalEntryValue::int_value_type:
				entry->set_i(it->ivalue);
				break;
			case LexicalEntryValue::uint_value_type:
			case LexicalEntryValue::uint_value_type_display_as_hex:
			case LexicalEntryValue::uint16_value_type_display_as_hex:
				entry->set_u(it->ivalue);
				break;
			case LexicalEntryValue::string_value_type:				
			case LexicalEntryValue::wstring_value_type:
				entry->set_s(it->svalue);
				break;
			case LexicalEntryValue::bytes_value_type_no_display:
			case LexicalEntryValue::bytes_value_type_display_as_hex:
				entry->set_b(it->bytes, it->bytes_length);
				break;
			case LexicalEntryValue::double_value_type:
				entry->set_d(it->dvalue);
				break;
			default:
				break;
		}
	}

	kit::log::LogEvent pb;
	if (!index1.empty())
	{
		pb.set_index1(index1);
		pb.set_index2(index2);
	}
	pb.set_index3(index3);
	pb.set_time(t);
	pb.set_data(detail_pb.SerializeAsString());

#ifndef _LOG_DLL_
	int pb_size = pb.ByteSize();
	char* send_buf = new char[CMDEX0_HEAD_SIZE + pb_size];
	CMD_SIZE(send_buf)			= CMDEX0_HEAD_SIZE + pb_size;					// ���ݰ����ֽ�������msghead��
	CMD_FLAG(send_buf)			= 0;							// ��־λ����ʾ�����Ƿ�ѹ�������ܵ�
	CMD_CAT(send_buf)			= CAT_LOGSVR;				// �������
	CMD_ID(send_buf)			= ID_LOGSVR_LOG;

	pb.SerializeWithCachedSizesToArray((google::protobuf::uint8*)CMDEX0_DATA(send_buf));

	//TODO: �ĳ�1000����һ��֮���
	std::shared_ptr<NetLibPlus_Client> c = NetLibPlus_get_first_Client(__LS_ServerTypeName.c_str());
	if (c) 
	{
		c->SendAsync(send_buf);
	}
	else
	{
		delete[] send_buf;
	}
#else
	if (udpsocket_4log)
	{
		if (index1.empty()) //��global_index1, global_index2����proto���Ȼ���������Log�Ѿ�ָ����index1,index2���Ǿ��ò���global_index1��
		{
			pb.set_index1(global_index1);
			pb.set_index2(global_index2);
		}
		int pb_size = pb.ByteSize();
		char* send_buf = new char[2 + pb_size];

		*(uint16_t*)send_buf = ID_LOGSVR_LOG;

		pb.SerializeWithCachedSizesToArray((google::protobuf::uint8*)(send_buf + 2));

		boost::system::error_code error;
		boost::lock_guard<boost::mutex> lock(_udpsocket_mutex);

		udpsocket_4log->send_to(boost::asio::buffer(send_buf, 2 + pb_size), udpendpoint_4log, 0, error);  //���0�ľ������岻������������������Ļ�Ĭ��������Ҳ��0
		delete[] send_buf;
	}
#endif
}