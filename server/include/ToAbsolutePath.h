#pragma once

#include <string>
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#else
#include "unistd.h"
#endif

class utility3
{
public:
	static std::string ToAbsolutePath(std::string relative_path) //����Ѿ��Ǿ���·������ֱ�ӷ��أ����ʧ�ܣ��򷵻�ԭ·����
	{
#ifdef WIN32
		if (relative_path.size() >= 2 && relative_path[1] == ':')
#else
		if (relative_path.size() >= 1 && relative_path[0] == '/') //�������·������ѭ���ָ�ʽ����Ķ����ϣ�����\/xxx���������
#endif
		{
			return relative_path;
		}
		
#define MAX_PATH_LEN (2000) //����·���ܳ��������2000
		char app_absolute_path[MAX_PATH_LEN]; 

		int path_len;

#ifdef WIN32
		if( !GetModuleFileNameA( NULL, app_absolute_path, MAX_PATH_LEN ) )
		{
			return relative_path;
		}
		path_len = strlen(app_absolute_path);
#else
        path_len = readlink( "/proc/self/exe", app_absolute_path, MAX_PATH_LEN );
        if ( path_len < 0 )
        {
			return relative_path;
        }
#endif

		int last_slash_pos;
        for(last_slash_pos = path_len; last_slash_pos >= 0; last_slash_pos--)
        {
            if (app_absolute_path[last_slash_pos] == '/' || app_absolute_path[last_slash_pos] == '\\')
            {
                break;
            }
        }

		if (last_slash_pos < 0) //û�ҵ�slash
		{
			return relative_path;
		}
		else
		{
			return std::string(app_absolute_path, 0, last_slash_pos + 1) + relative_path;
		}
	}
};