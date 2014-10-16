#include "Config.h"
#include "include/utility1.h"
#include "include/ptime2.h"
#include "rapidjson.h"
#include "document.h"
#include "stringbuffer.h"
#include "prettywriter.h"
#include "ControlServerConfig.h"
#include "Log/Log.h"
#include "MessageTypeDef.h"
#include "include/ToAbsolutePath.h"

//������ֹ�ϵ�������ļ��Ĵ��Ŀ¼������������޸ģ���Ҫ����Ӧ��Ŀ¼Ҳ���޸ġ�
std::string GetServerTypeWrittenName(int server_type)
{
	switch (server_type)
	{
	case SERVER_TYPE_GATEWAY_SERVER:
		return "gateway";
	case SERVER_TYPE_VERSION_SERVER:
		return "version_server";
	case SERVER_TYPE_ACCOUNT_SERVER:
		return "account_server";
	case SERVER_TYPE_BASIC_INFO_SERVER:
		return "basic_info_server";
	case SERVER_TYPE_LEAGUE_SERVER:
		return "league_server";
	case SERVER_TYPE_NOTICE_SERVER:
		return "notice_server";
	case SERVER_TYPE_RANK_SERVER:
		return "rank_server";
	case SERVER_TYPE_ASYNC_BATTLE_SERVER:
		return "async_battle_server";
	case SERVER_TYPE_REPLAY_SERVER:
		return "replay_server";
	case SERVER_TYPE_STAT_SERVER:
		return "stat_server";
	case SERVER_TYPE_AUTO_MATCH_SERVER:
		return "auto_match_server";
	case SERVER_TYPE_SYNC_BATTLE_SERVER:
		return "sync_battle_server";
	case SERVER_TYPE_CONTROL_SERVER:
		return "control_server";
	case SERVER_TYPE_BACKGROUND:
		return "background";
	default:
		return "default";
	}
}

std::map<std::pair<int, std::string>, std::string*> configs;

std::string* readConfig(int server_type, const std::string& file_name)
{
	auto it_config = configs.find(std::make_pair(server_type, file_name));
	if (it_config != configs.end())
	{
		return it_config->second;
	}

	return nullptr;
}

void writeConfig(int server_type, const std::string& file_name, const std::string& content)
{
	//дһ����Ϊ��ʷ
	file_utility::writeFile(utility3::ToAbsolutePath("configs/" + GetServerTypeWrittenName(server_type) +
		"/[" + time_utility::ptime_to_string4(boost::posix_time::microsec_clock::local_time()) + "] " + file_name), content);

	//�浽��ǰ�ļ�
	file_utility::writeFile(utility3::ToAbsolutePath("configs/" + GetServerTypeWrittenName(server_type) + "/" + file_name), content);

	//�浽�ڴ�map
	auto it_config = configs.find(std::make_pair(server_type, file_name));
	if (it_config != configs.end())
	{
		delete it_config->second;
	}
	else
	{
		//д��control_server.json

		std::string content; 
		file_utility::readFile(config.server_configs_list_file, content);

		rapidjson::Document document;		
		document.Parse<0>(content.c_str());
		if (document.IsArray())
		{
			rapidjson::Value v(rapidjson::kObjectType);
			v.AddMember("server_type", server_type, document.GetAllocator());
			std::string server_type_name = GetServerTypeWrittenName(server_type); 
			//�ӵ��ַ������������ڱ���Ҫ��json��Щ���Ҫ��������ʱ��ͻ���ʵ��Ѿ��ͷŵĿռ䣬����/u0000����ʵ���Ƿ������ˡ�
			//���������в���ֱ�Ӵ�GetServerTypeWrittenName().c_str()
			//Ҫ��rapidjson����������Ū��kStringType��Value����Ҫ��allocator
			v.AddMember("server_type_name", rapidjson::GenericStringRef<char>(server_type_name.c_str(), server_type_name.size()), document.GetAllocator());
			v.AddMember("file_name", rapidjson::GenericStringRef<char>(file_name.c_str(), file_name.size()), document.GetAllocator());
			document.PushBack(v, document.GetAllocator());
			rapidjson::StringBuffer buffer;
			rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
			document.Accept(writer);

			file_utility::writeFile(config.server_configs_list_file + ".bak", content);
			file_utility::writeFile(config.server_configs_list_file, buffer.GetString());
		}
		else
		{
			LOGEVENTL("ERROR", config.server_configs_list_file << " not an array. At least an empty array is needed.");
		}
	}

	configs[std::make_pair(server_type, file_name)] = new std::string(content);
}

void initializeConfigs()
{
	//�ĳɾ���·��
	config.server_configs_list_file = utility3::ToAbsolutePath(config.server_configs_list_file);

	std::string content;
	file_utility::readFile(config.server_configs_list_file, content);

	rapidjson::Document document;
	document.Parse<0>(content.c_str());

	if (document.IsArray())
	{
		unInitializeConfigs();
		configs.clear();

		for (int i = 0; i < document.Size(); ++i)
		{
			int server_type;
			if (document[i]["server_type"].IsNumber())
			{
				server_type = document[i]["server_type"].GetInt();
			}
			else
			{
				continue;
			}
			
			std::string file_name;
			if (document[i]["file_name"].IsString())
			{
				file_name = document[i]["file_name"].GetString();
			}
			else
			{
				continue;
			}

			std::string* content = new std::string();
			file_utility::readFile(utility3::ToAbsolutePath("configs/" + GetServerTypeWrittenName(server_type) + "/" + file_name), *content);
			configs.insert(std::make_pair(std::make_pair(server_type, file_name), content));
		}
	}
	else
	{
		LOGEVENTL("ERROR", config.server_configs_list_file << " not an array. At least an empty array is needed.");
	}
}

void unInitializeConfigs()
{
	for (auto it = configs.begin(); it != configs.end(); ++it)
	{
		delete it->second;
	}
}