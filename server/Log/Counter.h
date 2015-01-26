#pragma once
#include <memory>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/bind.hpp>
#include <stdint.h>
#include "../include/ptime2.h"
#include "internal.h"
#include "Log.h"

class Counter : public std::enable_shared_from_this<Counter>
{
protected:
	boost::asio::deadline_timer t;
	std::string m_index3;
	unsigned m_interval_secs;
	uint64_t m_start_time;

public:
	void refresh_timer()
	{
		m_start_time = ptime2(boost::posix_time::microsec_clock::local_time()).get_u64();
		boost::system::error_code error;
		t.expires_from_now(boost::posix_time::seconds(m_interval_secs), error);
		t.async_wait(boost::bind(&Counter::timer_expired, this, this->shared_from_this(), boost::asio::placeholders::error));
	}

protected:
	void timer_expired(std::shared_ptr<Counter> keep_alive, const boost::system::error_code& error)
	{		
		//cnt_valueΪ0Ҳ�򡣱�������ı����ȱ�С��弸��ֵΪ0����Ҳ���ѿռ䡣
		_LOG("_cnt_" + global_index1, global_index2, m_index3, cnt_value << m_start_time); //��ûˢ��timer֮ǰ��m_start_time����һ�ε�ʱ��

		if (! error) //cancel����һ��expire��LOG�ռǣ����ǲ���ˢ��timer
		{
			refresh_timer();
		}

		cnt_value = 0;
	}

public:
	uint32_t cnt_value;

	Counter(boost::asio::io_service& ioservice, const std::string& index3, unsigned interval_secs) : 
		t(ioservice), cnt_value(1), m_index3(index3), m_interval_secs(interval_secs)
	{
	}

	void ChangeInterval(unsigned int new_interval)
	{
		m_interval_secs = new_interval;

		//interval�仯��timerҪˢ��һ�¡����������ˢ�£���ԭ1���timer�����ڸĳ�1�����ˣ����Ҫ1���Ÿġ�
		//�����refresh_timer()����һ�������ܸ���m_start_time
		boost::system::error_code error;
		t.expires_from_now(boost::posix_time::seconds(m_interval_secs), error);
		t.async_wait(boost::bind(&Counter::timer_expired, this, this->shared_from_this(), boost::asio::placeholders::error));
	}

	void Cancel()
	{
		boost::system::error_code ignored_error;
		t.cancel(ignored_error);
	}
};

extern std::map<std::string, std::shared_ptr<class Counter>> __log_counters;
extern boost::mutex __log_counters_mutex;

void UpdateCounterInterval(const std::string& index3, unsigned int NewInterval);
void IncreaseCounter(const std::string& index3, unsigned int CountInterval);
void LogStopAllCounters();