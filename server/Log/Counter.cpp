#include "Counter.h"

std::map<std::string, std::shared_ptr<Counter>> __log_counters;
bool __log_counters_stopped = false;
boost::mutex __log_counters_mutex;

void UpdateCounterInterval(const std::string& index3, unsigned int NewInterval)
{
	boost::lock_guard<boost::mutex> lock(__log_counters_mutex);
	auto it = __log_counters.find(index3);
	if (it != __log_counters.end()) //���е�timer�Ÿ��£���û������timer�Ͳ��ùܣ�֮������timerʱ��Ȼ�����µ�interval
	{
		if (NewInterval)
		{
			it->second->ChangeInterval(NewInterval);
		}
		else //NewIntervalΪ0��ʾȡ��Timer
		{
			it->second->Cancel();
			__log_counters.erase(it);
		}
	}
}

void IncreaseCounter(const std::string& index3, unsigned int CountInterval)
{
	boost::lock_guard<boost::mutex> lock(__log_counters_mutex);
	auto it = __log_counters.find(index3);
	if (it == __log_counters.end())
	{
		if (! __log_counters_stopped)
		{
			auto c = std::make_shared<Counter>(__log_ioservice_th.get_ioservice(), index3, CountInterval);
			c->refresh_timer();
			__log_counters.insert(std::make_pair(index3, c));
		}
	}
	else
	{
		it->second->cnt_value ++;
	}
}

void LogStopAllCounters()
{	
	boost::lock_guard<boost::mutex> lock(__log_counters_mutex);
		
	for (auto it = __log_counters.begin(); it != __log_counters.end(); ++it)
	{
		it->second->Cancel();
	}
	__log_counters.clear();
	__log_counters_stopped = true;
}