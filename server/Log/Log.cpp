#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread.hpp>
#include "internal.h"
#include "LogLevel.h"
#include "SimpleIni.h"
#include "LogFile.h"
#include <sstream>
#include <vector>
#include <memory>
#include "NetLib/NetLib.h"
#include "to_LS_ClientDelegate.h"
#include "Counter.h"
#include "ToAbsolutePath.h"
#include "../protobuf/log/log.pb.h"
#include "HeaderDefine.h"
#include "Log.h"

ioservice_thread __log_ioservice_th;

std::string __LS_ServerTypeNameForQuery = "ls";
std::string __LS_ServerTypeName = "ls";

boost::shared_mutex _log_shared_mutex;

std::string FileNamePrefix;

bool UserDefinedFilenamePrefix = false;

//û�е���LogInitializeLocalOptionsǰ��Ĭ��ֵ
bool EnableToConsole = true;
bool EnableToFile = true;
bool SplitByDate = true;
int LogTotalMegaBytesLimitWithinDir = 32 * 1024;
int LogTotalFileLimitWithinDir = 200;

unsigned global_LogLevel = LOGLV_INFO;
unsigned global_CountInterval = 24 * 3600;

unsigned global_EnableDetail = 1;
std::string global_index1, global_index2; //index1Ϊ��ʱ��д���ļ��������Log���ݴ�

std::map<std::string, LogOption> options_map;

LogFile file;

void SetLSServerTypeNameForQuery(const char* LS_ServerTypeNameForQuery)
{
	__LS_ServerTypeNameForQuery = LS_ServerTypeNameForQuery;
}

void SetLSServerTypeName(const char* LS_ServerTypeName)
{
	__LS_ServerTypeName = LS_ServerTypeName;
	__LS_ServerTypeNameForQuery = LS_ServerTypeName;
}

void _InitializeLogNetDelegate() //TODO: ��ioservice_thread�����ķ���
{
	//TODO TOMODIFY
	/*
	NetLibPlus_InitializeClients<to_LS_ClientsDelegate>(__LS_ServerTypeName.c_str());
	if (__LS_ServerTypeNameForQuery != __LS_ServerTypeName)
	{
		NetLibPlus_InitializeClients<to_LS_ClientsDelegate>(__LS_ServerTypeNameForQuery.c_str());
	}
	*/
};

void SendLoginToLS() 
{	
	_log_shared_mutex.lock_shared();

	kit::log::Login pb;
	pb.set_index1(global_index1);
	pb.set_index2(global_index2);

	_log_shared_mutex.unlock_shared();

	int pb_size = pb.ByteSize();
		
	char* send_buf = new char[CMDEX0_HEAD_SIZE + pb_size];
	CMD_SIZE(send_buf)			= CMDEX0_HEAD_SIZE + pb_size;					// ���ݰ����ֽ�������msghead��
	CMD_FLAG(send_buf)			= 0;							// ��־λ����ʾ�����Ƿ�ѹ�������ܵ�
	CMD_CAT(send_buf)			= CAT_LOGSVR;				// �������
	CMD_ID(send_buf)			= ID_LOGSVR_LOGIN;

	pb.SerializeWithCachedSizesToArray((google_lalune::protobuf::uint8*)CMDEX0_DATA(send_buf));

	//TODO TOMODIFY

	//TODO: Ӧ�ø�����ls��
	/*
	std::shared_ptr<NetLibPlus_Client> client = NetLibPlus_get_first_Client(__LS_ServerTypeName.c_str()); //LogOptionsֻ��дLog��Ч�����Բ��ö�ServerTypeForQuery��
	if (client) 
	{
		client->SendAsync(send_buf);
	}
	else
	{
		delete[] send_buf;
	}
	*/
}

void LogInitializeLocalOptions(bool enable_to_console, bool enable_to_file, const char* filename_prefix, bool split_by_date, int log_total_mega_bytes_limit_within_dir, int log_total_file_limit_within_dir, const char* config_file_path)
{
	boost::lock_guard<boost::shared_mutex> lock(_log_shared_mutex);

	FileNamePrefix = filename_prefix;
	EnableToConsole = enable_to_console;
	EnableToFile = enable_to_file;
	SplitByDate = split_by_date;
	LogTotalMegaBytesLimitWithinDir = log_total_mega_bytes_limit_within_dir;
	LogTotalFileLimitWithinDir = log_total_file_limit_within_dir;

	CSimpleIni ini;
	if (ini.LoadFile(utility3::ToAbsolutePath(config_file_path).c_str()) == SI_OK)
	{
		FileNamePrefix = ini.GetValue("Log", "FileNamePrefix", filename_prefix);
		EnableToConsole = ini.GetBoolValue("Log", "EnableConsole", EnableToConsole);
		EnableToFile = ini.GetBoolValue("Log", "EnableFile", EnableToFile);
		SplitByDate = ini.GetBoolValue("Log", "SplitByDate", SplitByDate);
		LogTotalMegaBytesLimitWithinDir = ini.GetLongValue("Log", "LogTotalMegaBytesLimitWithinDir", LogTotalMegaBytesLimitWithinDir);
		LogTotalFileLimitWithinDir = ini.GetLongValue("Log", "LogTotalFileLimitWithinDir", LogTotalFileLimitWithinDir);
	}
	
	if (FileNamePrefix.empty())
	{
		UserDefinedFilenamePrefix = false;
	}
	else
	{
		UserDefinedFilenamePrefix = true;
		FileNamePrefix = utility3::ToAbsolutePath(FileNamePrefix);
	}

	file.CloseIfNeeded(); //���ɵ�Log�ļ����ˣ���Ϊģ�������ˣ��������ĵ�һ��log�Ὺ�ļ��ġ�
}

void LogSetIdentifier(const char* ServerType, int ServerID)
{
	if (strlen(ServerType) == 0)
	{
		printf("[Log_Error] LogSetIdentifier: can't accept an empty string as ServerType.\n");
		return;
	}

	std::ostringstream os;
	os << ServerID;
	LogSetIndex12(ServerType, os.str().c_str());
}

void LogSetIndex12(const char* index1, const char* index2)
{
	{
		boost::lock_guard<boost::shared_mutex> lock(_log_shared_mutex);
		
		if (strlen(index1) == 0)
		{
			printf("[Log_Error] LogSetIndex12: can't accept an empty string as index1.\n");
			return;
		}

		global_index1 = index1;
		global_index2 = index2;

		if (! UserDefinedFilenamePrefix) //����û�û�����ⲿ�����ļ���ǰ׺������global_index1��global_index2ƴװһ��
		{
			if (global_index2.empty()) //Ĭ��ֵ�����ֱ�Ӳ���index2���ļ�����
			{
				FileNamePrefix = global_index1;
			}
			else
			{
				FileNamePrefix = global_index1 + "_" + global_index2;
			}

			FileNamePrefix = utility3::ToAbsolutePath(FileNamePrefix);
			
			file.CloseIfNeeded(); //���ɵ�Log�ļ����ˣ���Ϊƴװ���FileNamePrefix���ˣ��������ĵ�һ��log�Ὺ�ļ��ġ�	
		}

		__log_ioservice_th.start();	//�ظ�startû��ϵ
	}

	__log_ioservice_th.get_ioservice().post(boost::bind(&SendLoginToLS));
}

void LogUnInitialize()
{
	LogStopAllCounters();

	//TODO TOMODIFY
	/*
	NetLibPlus_UnInitializeClients(__LS_ServerTypeName.c_str());
	if (__LS_ServerTypeNameForQuery != __LS_ServerTypeName)
	{
		NetLibPlus_UnInitializeClients(__LS_ServerTypeNameForQuery.c_str());
	}
	*/

	__log_ioservice_th.stop_when_no_work(); //��䲻����LogStopAllCounters֮ǰִ��
	__log_ioservice_th.wait_for_stop(); //���ж���ʹ����ioservice: counter's timer / udpsocket_4log(DLL�汾) / udp_login_timer(DLL�汾) / SendLoginToLS
	
	//��wait_for_stop֮�����ͷŸ�����
}