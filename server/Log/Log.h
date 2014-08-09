#pragma once

#include "Log_Dll.h"

#ifndef _LOG_DLL_

#ifdef _DEBUG
#define _LOG_LIBNAME_ "Logd"
#else
#define _LOG_LIBNAME_ "Log"
#endif

#pragma comment(lib, _LOG_LIBNAME_)  //this is not cross-platform. you should write Makefile on linux.

#endif

#include "LogStream.h"
#include <sstream>

#ifdef WIN32
#define _FUNCTION_AND_LINE_ log_::n("Function") << __FUNCTION__ << log_::n("Line") << __LINE__ << ", "
#define _FUNCTION_AND_LINE_NOCOMMA log_::n("Function") << __FUNCTION__ << log_::n("Line") << __LINE__
#else
#define _FUNCTION_AND_LINE_ log_::n("Function") << __func__ << log_::n("Line") << __LINE__ << ", "
#define _FUNCTION_AND_LINE_NOCOMMA log_::n("Function") << __func__ << log_::n("Line") << __LINE__
#endif

#define _ln(NAME) (log_::n(NAME))

#define _LOG(I1, I2, I3, C) \
{							\
	BasicLogStream ___bls;  \
	___bls << C;			\
	___bls.RecordTime();	\
	___bls.LogToNet(I3, I1, I2);	\
}

//I1��I2, I3ȷ��ʱ��TҪ��֤����˳������������׻�����ͬʱֻ��һ���˶�һ��(I1, I2, I3)����д��
#define _LOGT(I1, I2, I3, T, C) \
{							\
	BasicLogStream ___bls;  \
	___bls << C;			\
	___bls.RecordTime(T);	\
	___bls.LogToNet(I3, I1, I2);	\
}

#define LOGEVENT(I3, C)		\
{							\
	LogStream ___ls(I3);	\
	if (___ls.IsEnable())	\
	{						\
		___ls << C;			\
		___ls.Log();		\
	}						\
}

#define LOGLIST(I3, C, V)	\
{							\
	LogStream ___ls(I3);	\
	if (___ls.IsEnable())	\
	{						\
		___ls << C;			\
		for (auto __it = V.begin(); __it != V.end(); ++__it)	\
		{					\
			if (__it != V.begin())	\
			{				\
				___ls << ", ";	\
			}				\
			___ls << *__it;	\
		}					\
		___ls.Log();		\
	}						\
}

#define LOGVECTOR(I3, C, V)	LOGLIST(I3, C, V)

#define LOGMAP(I3, C, V)	\
{							\
	LogStream ___ls(I3);	\
	if (___ls.IsEnable())	\
	{						\
		___ls << C;			\
		for (auto __it = V.begin(); __it != V.end(); ++__it)	\
		{					\
			___ls << _ln(__it->first) << __it->second;	\
		}					\
		___ls.Log();		\
	}						\
}

#define LOGEVENTFL(I3, C)	LOGEVENT(I3, _FUNCTION_AND_LINE_ << C)

#define LOGLISTFL(I3, C, V)	\
{							\
	LogStream ___ls(I3);	\
	if (___ls.IsEnable())	\
	{						\
		___ls << _FUNCTION_AND_LINE_ << C;	\
		for (auto __it = V.begin(); __it != V.end(); ++__it)	\
		{					\
			if (__it != V.begin())	\
			{				\
				___ls << ", ";	\
			}				\
			___ls << *__it;	\
		}					\
		___ls.Log();		\
	}						\
}

#define LOGVECTORFL(I3, C, V)	LOGLISTFL(I3, C, V)

#define LOGMAPFL(I3, C, V)	\
{							\
	LogStream ___ls(I3);	\
	if (___ls.IsEnable())	\
	{						\
		___ls << _FUNCTION_AND_LINE_ << C;	\
		for (auto __it = V.begin(); __it != V.end(); ++__it)	\
		{					\
			___ls << _ln(__it->first) << __it->second;	\
		}					\
		___ls.Log();		\
	}						\
}

#define LOGEVENT_(C) LOGEVENT("", C)

#define LOGEVENTSTR(I3, C)	\
{							\
	LogStream ___ls(I3);	\
	if (___ls.IsEnable())	\
	{						\
		std::ostringstream os;	\
		os << C;			\
		___ls << os.str();	\
		___ls.Log();		\
	}						\
}

#define LOGEVENTL(I3, C)	\
{							\
	LogStream ___ls(I3);	\
	if (___ls.IsEnable())	\
	{						\
		___ls << C;			\
		___ls.Log(true);	\
	}						\
}

#define LOGLISTL(I3, C, V)	\
{							\
	LogStream ___ls(I3);	\
	if (___ls.IsEnable())	\
	{						\
		___ls << C;			\
		for (auto __it = V.begin(); __it != V.end(); ++__it)	\
		{					\
			if (__it != V.begin())	\
			{				\
				___ls << ", ";	\
			}				\
			___ls << *__it;	\
		}					\
		___ls.Log(true);	\
	}						\
}

#define LOGVECTORL(I3, C, V)	LOGLISTL(I3, C, V)

#define LOGMAPL(I3, C, V)	\
{							\
	LogStream ___ls(I3);	\
	if (___ls.IsEnable())	\
	{						\
		___ls << C;			\
		for (auto __it = V.begin(); __it != V.end(); ++__it)	\
		{					\
			if (__it != V.begin())	\
			{				\
				___ls << ", ";	\
			}				\
			___ls << *__it;	\
		}					\
		___ls.Log(true);		\
	}						\
}

#define LOGEVENTLFL(I3, C)	LOGEVENTL(I3, _FUNCTION_AND_LINE_ << C)

#define LOGLISTLFL(I3, C, V)	\
{							\
	LogStream ___ls(I3);	\
	if (___ls.IsEnable())	\
	{						\
		___ls << _FUNCTION_AND_LINE_ << C;	\
		for (auto __it = V.begin(); __it != V.end(); ++__it)	\
		{					\
			if (__it != V.begin())	\
			{				\
				___ls << ", ";	\
			}				\
			___ls << *__it;	\
		}					\
		___ls.Log(true);		\
	}						\
}

#define LOGVECTORLFL(I3, C, V)	LOGLISTLFL(I3, C, V)

#define LOGMAPLFL(I3, C, V)		\
{							\
	LogStream ___ls(I3);	\
	if (___ls.IsEnable())	\
	{						\
		___ls << _FUNCTION_AND_LINE_ << C;	\
		for (auto __it = V.begin(); __it != V.end(); ++__it)	\
		{					\
			___ls << _ln(__it->first) << __it->second;	\
		}					\
		___ls.Log(true);		\
	}						\
}

#define LOGEVENTL_(C) LOGEVENTL("", C)

#define LOGEVENTLSTR(I3, C)	\
{							\
	LogStream ___ls(I3);	\
	if (___ls.IsEnable())	\
	{						\
		std::ostringstream os;	\
		os << C;			\
		___ls << os.str();	\
		___ls.Log(true);	\
	}						\
}

#define LOGBYTES(T, D, L) LOGEVENT(T, log_::b(D, L))
#define LOGBYTES_DISPLAY(T, D, L) LOGEVENT(T, log_::bd(D, L))

#define __LOGCMD(LOGFUNC, CMDBUF)	\
{								\
	uint32_t ___uSize = *(uint32_t*)(CMDBUF);	\
	if (___uSize >= 12)							\
	{											\
		char ___LogTypeBuf[13];					\
		sprintf(___LogTypeBuf, "_CMD%04x%04x", *(uint16_t*)((char*)CMDBUF + 8), *(uint16_t*)((char*)CMDBUF + 10));	\
		LOGFUNC(___LogTypeBuf, CMDBUF, ___uSize);	\
	}											\
	else										\
	{											\
		LOGFUNC("_ICMD", CMDBUF, ___uSize);	\
	}											\
}

#define LOGCMD(CMDBUF) __LOGCMD(LOGBYTES, CMDBUF)
#define LOGCMD_DISPLAY(CMDBUF) __LOGCMD(LOGBYTES_DISPLAY, CMDBUF)

//logfilename_prefix�𳬹�200�ֽڣ���������������
LOG_API void LogInitializeLocalOptions(bool enable_to_console = false, bool enable_to_file = false, const char* logfilename_prefix = "", bool split_by_date = true, 
	int log_total_mega_bytes_limit_within_dir = 32 * 1024, int log_total_file_limit_within_dir = 200, const char* config_file_path = "logger.ini"); //��������ļ�����ֵ�����������ļ��е�ֵ���ǲ�����ֵ

#ifndef _LOG_DLL_

void SetLSServerTypeNameForQuery(const char* LS_ServerTypeNameForQuery); //һ�����ServerTypeName��"ls"����Ҳ���ܲ��ǡ���������Ҫ��CtrlLogin֮ǰ�����á�
void SetLSServerTypeName(const char* LS_ServerTypeName); //һ�����ServerTypeName��"ls"����Ҳ���ܲ��ǡ��������Ḳ��LS_ServerTypeNameForQuery����������Ҫ��CtrlLogin֮ǰ�����á�

void _InitializeLogNetDelegate(); //��CtrlSvrConnector��CtrlLogin֮ǰ������

#else

//ֻ��DLL�汾�����������DLL�汾��ʹ��NetLib(��Ϊһ���õĻ���DLL���������ˣ�̫�ƣ������и��˲���������������)��ʹ������UDP
//��Ҫ���ͻ���ʹ�á����к�������ص�һ���������ַ����
//����������ж�̨�����ɿͻ���ѡ��һ̨��Set��Logģ��
LOG_API void SetLogServerAddress(const char* IP, int port); 

#endif

//����������ǰ(ȷ�е�˵����index1�ǿ�ǰ)������ͨ��LOGEVENT�����������İ������ԣ�Ҳ����ͳ�ƣ�ֻ�Ǵ���Ļ��ֱ������������ȷ�����÷������Ա���������
//�ң����filename_prefixΪ�գ���ô����������ǰ��LogҲ������ļ�����Ϊ��֪��Log��Ŀ�ĵ��ļ���

LOG_API void LogSetIndex12(const char* index1, const char* index2); //index1 + index2�𳬹�199�ֽڣ���������������

void LogSetIdentifier(const char* ServerType = "", int ServerID = -1);  //�����������Ͼ���LogSetIndex12����ServerType��Ϊindex1����ServerID��Ϊindex2��ServerIDΪ-1ʱindex2Ϊ���ַ���

//�������������Ҫ�������ü����������������һ�����ñ�ж�ص�ʱ���ִ�С���ʱû�йܣ���һ�ε��þ�����ִ���ˡ�
LOG_API void LogUnInitialize();
