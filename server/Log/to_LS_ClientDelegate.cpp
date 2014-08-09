#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include "to_LS_ClientDelegate.h"
#include "LogQuery.h"
#include "../protobuf/src-gen/cpp/log/log.pb.h"
#include "HeaderDefine.h"
#include <map>
#include <set>
#include "internal.h"
#include "Counter.h"
#include "Log.h"

bool ProcessLogOptions(const char* pb_data, int pb_size)
{
	kit::log::LogOptions pb;
	if (pb.ParseFromArray(pb_data, pb_size))
	{
		boost::lock_guard<boost::shared_mutex> lock(_log_shared_mutex);

		global_LogLevel = pb.log_level();
		global_CountInterval = pb.count_interval();
		global_EnableDetail = pb.enable_detail();

		std::set<std::string> options_will_be_deleted;
		for (auto it_op0 = options_map.begin(); it_op0 != options_map.end(); ++it_op0)
		{
			options_will_be_deleted.insert(it_op0->first);
		}

		for (int i = 0; i < pb.log_option_size(); ++i)
		{
			const kit::log::LogOption& pb_lo = pb.log_option(i);
			LogOption lo;
			lo.LogLevel = pb_lo.log_level();
			lo.CountInterval = pb_lo.count_interval();
			lo.EnableDetail = pb_lo.enable_detail();
			options_will_be_deleted.erase(pb_lo.index3());

			auto it_op = options_map.find(pb_lo.index3());
			if (it_op != options_map.end()) //ԭ��options_map�����������ڿ����и���
			{
				if (it_op->second.CountInterval != lo.CountInterval)
				{
					UpdateCounterInterval(pb_lo.index3(), lo.CountInterval);
				}
				it_op->second = lo;
			}
			else //ԭ��options_map��û�������ô����µ�option��֮ǰ��Ĭ��ֵ����������ζ�Ÿ��¡�������Ҫ�Ǹ���Timer
			{
				options_map.insert(std::make_pair(pb_lo.index3(), lo));
				if (global_CountInterval != lo.CountInterval)
				{
					UpdateCounterInterval(pb_lo.index3(), lo.CountInterval);
				}
			}
		}

		for (auto it_opwd = options_will_be_deleted.begin(); it_opwd != options_will_be_deleted.end(); ++it_opwd)
		{
			auto it_op = options_map.find(*it_opwd);
			if (it_op != options_map.end())
			{
				if (it_op->second.CountInterval != global_CountInterval)
				{
					//CountInterval�仯�ˣ����Ĭ��ֵglobal_CountInterval��
					UpdateCounterInterval(it_op->first, global_CountInterval);
				}
				options_map.erase(it_op);
			}
		}

		return true;
	}
	else
	{
		LOGEVENTL("Log_Warn", "ProcessLogOptions, protobuf kit::log::LogOptions parse failed.");
		return false;
	}
}

#ifndef _LOG_DLL_

void to_LS_ClientsDelegate::ReconnectedHandler(std::shared_ptr<NetLibPlus_Client> clientptr)
{
	boost::shared_lock<boost::shared_mutex> lock(_log_shared_mutex);

	if (! global_index1.empty()) //�ǿձ���LogSetIdentifier���������ù�������Ҫ�ٷ�һ��Login�����������LogSetIdentifier�����þ����ˣ�LogSetIdentifier�����õ�ʱ���SendLoginToLS
	{
		SendLoginToLS();
	}
}

extern std::map<int, std::shared_ptr<ResultHandler>> result_handlers;
extern boost::mutex _log_query_result_handlers_mutex;

void to_LS_ClientsDelegate::RecvFinishHandler(std::shared_ptr<NetLibPlus_Client> clientptr, char* data)
{
	uint32_t uSize		  = CMD_SIZE(data);					// ���ݰ����ֽ�������msghead��

	if (uSize < CMDEX0_HEAD_SIZE) 
	{
		LOGEVENTL("UNEXPECTED", "to_ls_ClientDelegate::RecvFinishHandler, ls returned a packet with only size " << uSize);
		return; //���Ȳ���
	}
		
	if (CMD_CAT(data) != CAT_LOGSVR) 
	{
		LOGEVENTL("UNEXPECTED", "to_ls_ClientDelegate::RecvFinishHandler, unrecoginized CmdCat: " << log_::h16(CMD_CAT(data)));
		return;
	}

	switch (CMD_ID(data))
	{
		case ID_LOGSVR_QUERY_RESULT:
		case ID_LOGSVR_QUERY_RESULT_END:
			{
				//LOGEVENTL("Log_Debug", "QueryResult");

				kit::log::QueryResult pb;
				if (pb.ParseFromArray(CMDEX0_DATA(data), CMDEX0_DATA_SIZE(data)))
				{
					uint64_t tid = CMDEX0_TRANSID(data);
					std::shared_ptr<ResultHandler> result_handler;
					{
						boost::lock_guard<boost::mutex> lock(_log_query_result_handlers_mutex);
						auto it = result_handlers.find((int)tid);
						if (it != result_handlers.end())
						{
							result_handler = it->second;

							if (CMD_ID(data) == ID_LOGSVR_QUERY_RESULT_END)
							{
								result_handlers.erase(it); //12.08.17�޸ģ�֮ǰ����result_handlers���map�����С��erase֮���������滹����һ�ݣ����Բ��������ͷ�
							}
						}
					}
					if (result_handler)
					{
						//��̨���������Ҫ��
						for (int i = 0; i < pb.log_event_size(); ++i)
						{
							result_handler->handle_result(pb.log_event(i));
						}

						if (CMD_ID(data) == ID_LOGSVR_QUERY_RESULT_END) 
						{
							result_handler->result_end();
						}
					}
					else
					{
						LOGEVENTL("UNEXPECTED", "to_ls_ClientDelegate::RecvFinishHandler, TransID" << tid << " has no ResultHandler");
					}
				}
				else
				{
					LOGEVENTL("UNEXPECTED", "LS_ClientDelegate::RecvFinishHandler, protobuf kit::log::QueryResult parse fail");
				}
			}
			break;
		case ID_LOGSVR_LOGOPTIONS:
			ProcessLogOptions(CMDEX0_DATA(data), CMDEX0_DATA_SIZE(data));
			break;
		default:
			LOGEVENTL("UNEXPECTED", "to_ls_ClientDelegate::RecvFinishHandler, unrecoginized CmdID: " << hex16(CMD_ID(data)));
			break;
	}
}

#endif