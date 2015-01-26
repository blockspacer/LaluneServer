#pragma once

#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include "../include/ptime2.h"
#include <string>
#include <stdio.h>
#include <boost/filesystem.hpp>
//#include <iostream>

class LogFile
{
protected:
	FILE* _file;
	std::string current_date; //��Ϊ����������ļ����ļ�����һ���֡��µ�һ�쵽��ʱ������������жϳ��仯���Ӷ��л������ļ�
	boost::mutex mutex;

	void GenerateFileName(char* file_name) //��Ҫ���ڵ���
	{
		std::string time;
		if (SplitByDate) //��ģʽ��������Ϊ�ļ�����ÿ��һ���Զ������ļ�
		{
			time = current_date;
		}
		else //��ģʽ��ʱ��Ϊ�ļ�����ÿ��Log��ʼ����һ���ļ�������;�����ָ
		{
			time = time_utility::ptime_to_string4(boost::posix_time::microsec_clock::local_time());
		}

		sprintf(file_name, "%s_%s.log", FileNamePrefix.c_str(), time.c_str());
	}

	inline void _CloseIfNeeded() //���ڵ���
	{
		if (_file)
		{	
			fclose(_file);
			_file = nullptr;
		}
	}

public:
	LogFile() : _file(nullptr)
	{
	}

	inline void CloseIfNeeded()
	{
		boost::lock_guard<boost::mutex> lock(mutex);

		_CloseIfNeeded();
	}

	virtual ~LogFile()
	{
		CloseIfNeeded();
	}

	void PrintLine(const std::string& formatted_log)
	{		
		boost::lock_guard<boost::mutex> lock(mutex);		
		
		if (SplitByDate)
		{
			if (current_date != formatted_log.substr(1, 10))
			{
				current_date = formatted_log.substr(1, 10);
				_CloseIfNeeded();
			}
		}
		
		if (! _file) //����ļ���û������һ���ļ���������ļ����̿��ܻὨ���ļ���
		{
			//���ļ�֮ǰ�����ͬĿ¼�µ�Log�ܴ�С�Ƿ񳬹����ơ�����<=0���ʾ�����ơ����������޸�ʱ������ɾ����Log��ֱ�������ޡ�
			
			if (LogTotalMegaBytesLimitWithinDir > 0 || LogTotalFileLimitWithinDir > 0) //����һ���������㣬��Ҫ��ȥɨ�ļ�
			{
				//DEBUG
				//std::cout << LogTotalMegaBytesLimitWithinDir << ", " << LogTotalFileLimitWithinDir << std::endl;

				boost::filesystem::path dir;
				//ʵ����win�µĸ�Ŀ¼Ҫð�ź����иܲ���Ϸ�������������ֻҪ�ѺϷ����г�������(�Ѿ���·���ҳ���)�����ðѷǷ��ĸ��е���boost���еġ�
				if ((FileNamePrefix.size() >= 2 && FileNamePrefix[1] == ':') || (FileNamePrefix.size() >= 1 && FileNamePrefix[0] == '/'))
				{
					dir = FileNamePrefix;
				}
				else
				{
					//���·������������Ӹ���
					dir = ".";
					dir /= FileNamePrefix;
				}

				std::string actual_FileNamePrefix = dir.leaf().string();

				if (! is_directory(dir))
				{
					dir.remove_filename();
				}

				std::vector<std::pair<boost::filesystem::path, uintmax_t> > matched_files;
				uintmax_t total_size = 0;

				if (is_directory(dir))
				{
					boost::filesystem::directory_iterator it(dir), eod;
					for (; it != eod; ++it)
					{
						if (is_regular_file(*it) && it->path().extension() == ".log" && it->path().leaf().string().compare(0, actual_FileNamePrefix.size(), actual_FileNamePrefix) == 0)
						{
							boost::system::error_code error;
							uintmax_t size = file_size(it->path(), error);
							if (!error)
							{
								//DEBUG
								//std::cout << "matched file: " << it->path().string() << " " << size << std::endl;
								matched_files.push_back(std::make_pair(*it, size));
								total_size += size;
							}
						}
					}

					int file_count = matched_files.size();

					uintmax_t total_bytes_limit = LogTotalMegaBytesLimitWithinDir;
					total_bytes_limit *= 1024 * 1024;

					//std::cout << "total: " << total_bytes_limit << std::endl;

					//������֮һ���㣬��Ҫ��ȥɾ�ļ�
					if (total_bytes_limit > 0 && total_size > total_bytes_limit || LogTotalFileLimitWithinDir > 0 && file_count > LogTotalFileLimitWithinDir)
					{
						//std::cout << "delete files." << std::endl;
						sort(matched_files.begin(), matched_files.end()); //����ֱ����ô�Ⱦ��ǰ�����ʱ������ġ��е㲻�Ͻ��������б���ļ�Ҳ�������FileNamePrefix�Ļ�˳��ͻ᲻��ȫ��ȷ�����������������
						auto will_delete_it = matched_files.begin();
						for ( ; will_delete_it != matched_files.end(); ++will_delete_it)
						{
							total_size -= will_delete_it->second;
							file_count --;

							//�������������㣬�����break
							if ((total_bytes_limit <= 0 || total_size <= total_bytes_limit) && (file_count <= 0) || (file_count <= LogTotalFileLimitWithinDir))
							{
								break;
							}
						}
						//��will_delete_itǰ��һλ����ʱ��ɾ��will_delete_it��ǰһ���ļ�Ϊֹ
						if (will_delete_it != matched_files.end()) //�����ȣ�˵������Щ�ļ���ɾ��Ҳ���㲻������������
						{
							will_delete_it ++;
						}
												
						boost::system::error_code error;
						for (auto it0 = matched_files.begin(); it0 != will_delete_it; ++it0)
						{
							remove(it0->first, error);
						}
					}
					else
					{
						//std::cout << "no need to delete files." << std::endl;
					}
				}
			}

			char file_name[300];
			GenerateFileName(file_name);
			_file = fopen(file_name, "a");

			if (! _file) //�������û���ɣ���������ΪȨ�޲������Ӳ������������Ŀ¼������
			{
				printf("ERROR: can't write to file %s\n", file_name);
				return;
			}
		}
		
		fprintf(_file, "%s\n", formatted_log.c_str());
		fflush(_file);
	}
};

extern LogFile file;