#pragma once

#include "SemanticStream.h"

class BasicLogStream : public SemanticStream //ֻ�м�¼ʱ�䣬�������������Ĺ���
{
protected:
	uint64_t t;

public:
	void RecordTime();
	void RecordTime(uint64_t time) //�ⲿ����ʱ��
	{
		t = time;
	}

	void LogToNet(const std::string& index3, const std::string& index1 = "", const std::string& index2 = ""); 
	//index3����ָ����ǰ���߿�������(ǰ�������յĻ��������ȷ���Login��), ��Ҫ�еĻ�����һ����
};
