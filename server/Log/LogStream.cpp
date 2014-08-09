#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/bind.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/system/error_code.hpp>
#include "LogStream.h"
#include "internal.h"
#include "LogLevel.h"
#include "LogFile.h"
#include "Log.h"
#include <map>
#include "Counter.h"

LogStream::LogStream(const char* index3) : m_index3(index3), owns_lock(true)
{
	_log_shared_mutex.lock_shared();

	auto it = options_map.find(index3);
	if (it == options_map.end())
	{
		if (global_index1.empty())
		{
			op.LogLevel = global_LogLevel;
			op.CountInterval = 0;
			op.EnableDetail = 0;
		}
		else
		{
			op.LogLevel = global_LogLevel;
			op.CountInterval = global_CountInterval;
			op.EnableDetail = global_EnableDetail;
		}
	}
	else
	{
		op = it->second;
	}
}

LogStream::~LogStream()
{
	if (owns_lock)
	{
		_log_shared_mutex.unlock_shared();
	}
}

bool LogStream::IsEnable() const
{
	return global_LogLevel <= op.LogLevel;
}

void LogStream::FormatToString(std::string& formatted_log) const
{
	char left_part[28];
	left_part[0] = '[';
	time_utility::ptime_to_string_full(t, left_part + 1);

	formatted_log = left_part;
	formatted_log += "] [";
	formatted_log += m_index3;
	formatted_log += "] ";
	formatted_log += ToString(); // std::string �� += �������õ��ǵײ��append�������� = p1 + p2 + p3 + p4������
}

void LogStream::FormatToStringComplete(std::string& formatted_log) const
{
	char buf[300];
	sprintf(buf, "[%s] [%s] [%s] [%s] ", time_utility::ptime_to_string_full(t).c_str(), global_index1.c_str(), global_index2.c_str(), m_index3.c_str());
	formatted_log = buf;
	formatted_log += ToString();
}

void LogStream::LogToLocal()
{	
	if (EnableToConsole || EnableToFile)
	{
		std::string formatted_log;
		FormatToString(formatted_log);

		if (EnableToConsole)
		{
			printf("%s\n", formatted_log.c_str());
		}

		if (EnableToFile && FileNamePrefix.size()) //FileNamePrefix�򲻴��ļ�������LogInitializeLocalOptions��LogSetIndex12����Ҫ��һ�������ã��Ż���ļ�
		{	
			file.PrintLine(formatted_log);
		}
	}
}

void LogStream::Log(bool IsLocal)
{
	RecordTime();

	LogToLocal();

	_log_shared_mutex.unlock_shared(); //���治�ٻ����ȫ�ֱ����ˣ�����ǰ����
	owns_lock = false;

	if (! IsLocal)
	{
		if (op.CountInterval) //���global_index1��û�и�ֵ����LogStream�ڹ����ʱ��ͻ��CountInterval��Ϊ0
		{
			IncreaseCounter(m_index3, op.CountInterval);
		}

		if (op.EnableDetail) //���global_index1��û�и�ֵ����LogStream�ڹ����ʱ��ͻ��EnableDetail��Ϊ0
		{
			//���ﲻ����_log_shared_mutex�������Ϊ������ܽ�NetLib���������������������ܵ��½���˳��һ�¶�������
			//�ڽ���˳���ϣ���������NetLib��������Log����

			LogToNet(m_index3);
		}
	}
}