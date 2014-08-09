#pragma once

#include <boost/date_time.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include "const_time.h"

class ptime2 : private boost::posix_time::ptime
{
public:
	ptime2(const boost::posix_time::ptime& rhs) : ptime(rhs)
	{
	}

	ptime2(const boost::gregorian::date& d) : ptime(d)
	{
	}

	uint64_t get_u64()
	{
		return time_.time_count();
	}

	static ptime create_ptime(uint64_t t)
	{
		if (t < TIME64_NEGATIVE_INFINITY) t = TIME64_NEGATIVE_INFINITY; //1900���
		if (t > TIME64_POSITIVE_INFINITY) t = TIME64_POSITIVE_INFINITY; //8099���
		//���������Χ�ڵĻ���ת��ptime����û���⣬���õ�ʱ����ܱ������߲���δ�������Ϊ

		return ptime(time_rep_type(t));
	}
};

class time_utility
{
public:
	//Release�治����ô��year() month() day()����ֵ����%d��iostream�ܴ򣬵��������ֱ�Ӵ�����ptimeһ�����·���Ӣ��
	/*
	static std::string ptime_to_string(boost::posix_time::ptime t)
	{
		auto date = t.date();
		auto time_of_day = t.time_of_day();
		char time_str[20];
		sprintf(time_str, "%04d-%02d-%02d %02d:%02d:%02d", date.year(), date.month(), date.day(), 
										time_of_day.hours(), time_of_day.minutes(), time_of_day.seconds());
	
		return time_str;
	}*/
	static void ptime_to_string_full(uint64_t t, char* buf) //buf����27�ֽ�(��ĩβ0)
	{
		tm pt_tm = boost::posix_time::to_tm(ptime2::create_ptime(t));
		sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d.%06d", 
			pt_tm.tm_year + 1900, pt_tm.tm_mon + 1, pt_tm.tm_mday, 
			pt_tm.tm_hour, pt_tm.tm_min, pt_tm.tm_sec, t % 1000000);
	}
	static std::string ptime_to_string_full(uint64_t t)
	{
		char time_str[27];
		ptime_to_string_full(t, time_str);
		return time_str;
	}
	static void ptime_to_string_full(boost::posix_time::ptime t, char* buf) //buf����27�ֽ�(��ĩβ0)
	{
		tm pt_tm = boost::posix_time::to_tm(t);
		sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d.%06d", 
			pt_tm.tm_year + 1900, pt_tm.tm_mon + 1, pt_tm.tm_mday, 
			pt_tm.tm_hour, pt_tm.tm_min, pt_tm.tm_sec, ptime2(t).get_u64() % 1000000);
	}
	static std::string ptime_to_string_full(boost::posix_time::ptime t)
	{
		char time_str[27];
		ptime_to_string_full(t, time_str);
		return time_str;
	}
	static void ptime_to_string2(boost::posix_time::ptime t, char* buf) //buf����20�ֽ�(��ĩβ0)
	{
		tm pt_tm = boost::posix_time::to_tm(t);
		sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d", pt_tm.tm_year + 1900, pt_tm.tm_mon + 1, pt_tm.tm_mday, pt_tm.tm_hour, pt_tm.tm_min, pt_tm.tm_sec);
	}
	static std::string ptime_to_string2(boost::posix_time::ptime t)
	{
		char time_str[20];
		ptime_to_string2(t, time_str);
		return time_str;
	}
	static void ptime_to_string3(boost::posix_time::ptime t, char* buf) //buf����18�ֽ�(��ĩβ0)
	{
		tm pt_tm = boost::posix_time::to_tm(t);
		sprintf(buf, "%04d-%02d-%02d %02d%02d%02d", pt_tm.tm_year + 1900, pt_tm.tm_mon + 1, pt_tm.tm_mday, pt_tm.tm_hour, pt_tm.tm_min, pt_tm.tm_sec);
	}
	static std::string ptime_to_string3(boost::posix_time::ptime t)
	{
		char time_str[18];
		ptime_to_string3(t, time_str);
		return time_str;
	}
	static void ptime_to_string4(boost::posix_time::ptime t, char* buf) //buf����25�ֽ�(��ĩβ0)
	{
		tm pt_tm = boost::posix_time::to_tm(t);
		sprintf(buf, "%04d-%02d-%02d %02d%02d%02d_%d", pt_tm.tm_year + 1900, pt_tm.tm_mon + 1, pt_tm.tm_mday, pt_tm.tm_hour, pt_tm.tm_min, pt_tm.tm_sec, ptime2(t).get_u64() % 1000000);
	}
	static std::string ptime_to_string4(boost::posix_time::ptime t)
	{
		char time_str[25];
		ptime_to_string4(t, time_str);
		return time_str;
	}
	static std::string ptime_to_string(boost::posix_time::ptime t)
	{
		return boost::posix_time::to_simple_string(t);
	}

	static uint64_t string_to_ptime_u(const std::string& s)
	{
		try
		{
			if (s.size() <= 10)
			{
				return ptime2(boost::gregorian::from_simple_string(s)).get_u64();
			}
			else
			{
				return ptime2(boost::posix_time::time_from_string(s)).get_u64();
			}
		}
		catch (std::exception e)
		{
			return 0;
		}
	}

	static time_t string_to_timet(const std::string& s)
	{
		tm tm1;
		sscanf(s.c_str(), "%4d-%2d-%2d %2d:%2d:%2d", &tm1.tm_year, &tm1.tm_mon, &tm1.tm_mday, &tm1.tm_hour, &tm1.tm_min, &tm1.tm_sec);
		tm1.tm_year -= 1900;
		tm1.tm_mon --;
		tm1.tm_isdst = -1;

		return mktime(&tm1);
	}

	static boost::posix_time::ptime timet_to_ptime_local(time_t t)
	{
		return boost::posix_time::ptime_from_tm(*localtime(&t));
	}
};

//�������Ϊ�˼�����ǰ�Ĵ���
//#define ptime_to_string(PT) time_utility::ptime_to_string(PT)