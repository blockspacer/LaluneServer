#pragma once

#include "LogOption.h"
#include "BasicLogStream.h"

class LOG_API LogStream : public BasicLogStream //���д���Ļ/���ļ�/�������/ͳ�� �ȹ���
{
protected:
	const std::string m_index3;
	LogOption op;
	bool owns_lock;

	void LogToLocal();

	void FormatToString(std::string& formatted_log) const;
	void FormatToStringComplete(std::string& formatted_log) const;

public:
	LogStream(const char* index3);
	virtual ~LogStream();

	bool IsEnable() const;
	void Log(bool IsLocal = false); //Local�����ִ�д���Ļ�����ļ��Ĳ���
};