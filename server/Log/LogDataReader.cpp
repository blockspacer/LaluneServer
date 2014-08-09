#ifndef _LOG_DLL_

#include "LogDataReader.h"

//�����������ͬ�����һ���ǰһ��

LogDataReader::LogDataReader(const std::string& data, bool enable_find_by_name)
{
	if (detail.ParseFromString(data))
	{
		if (enable_find_by_name)
		{
			for (auto it = detail.entry().begin(); it != detail.entry().end(); ++it)
			{
				if (it->has_entry_name())
				{
					name_map.insert(std::make_pair(it->entry_name(), &(*it)));
				}
			}
		}
	}
}

#endif