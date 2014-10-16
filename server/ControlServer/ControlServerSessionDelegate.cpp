#include "ControlServerSessionDelegate.h"
#include "ServerCommonLib/ServerCommon.h"
#include "Log/Log.h"
#include "MessageTypeDef.h"
#include "controlserver/ControlServer.pb.h"
#include "ControlServer.h"
#include <boost/bind.hpp>
#include "include/utility1.h"
#include "include/utility2.h"
#include "ControlServerConfig.h"
#include "Config.h"
#include <boost/asio.hpp>

void ControlServerSessionDelegate::ConnectedHandler(NetLib_ServerSession_ptr sessionptr)
{
	//��ʱû��ʲôҪ���ġ���Ϊ����֮�󻹵÷����Լ��Ķ˿ڲ�������
	//LOGEVENTL("DEBUG", "connected");
}

void ControlServerSessionDelegate::DisconnectedHandler(NetLib_ServerSession_ptr sessionptr)
{
	long attached_data = sessionptr->GetAttachedData();
	if (attached_data) //���Ϊ0����˵���ڷ�Hello��ǰ
	{
		//ֻҪά��server2session�ͺã�servers_info��server_groups���г�ʱ�¼�ά��
		server2session.erase(*(IPPort*)attached_data);
		delete (IPPort*)sessionptr->GetAttachedData();
	}
}

void ControlServerSessionDelegate::RecvKeepAliveHandler(NetLib_ServerSession_ptr sessionptr)
{
	//ˢ�³�ʱʱ��
	long attached_data = sessionptr->GetAttachedData();
	if (attached_data) //���Ϊ0����˵���ڷ�Hello��ǰ
	{
		IPPort ip_port = *(IPPort*)attached_data;
		auto info_it = servers_info.find(ip_port);
		if (info_it == servers_info.end())
		{
			LOGEVENTL("ERROR", "a server exist in session2server, but not in servers_info. " << log_::n("ip") << utility2::toIPs(ip_port.first) << log_::n("port") << ip_port.second);
		}
		else
		{
			info_it->second->refresh();
		}
	}
}

//RecvFinishHandlerһ�����أ�data�����ݾͻᱻ�ͷ�
void ControlServerSessionDelegate::RecvFinishHandler(NetLib_ServerSession_ptr sessionptr, char* data)
{
	if (SERVER_MSG_LENGTH(data) >= SERVER_MSG_HEADER_BASE_SIZE)
	{
		switch (SERVER_MSG_TYPE(data))
		{
		case MSG_TYPE_CONTROL_SERVER_INITIALIZE:
			{
				common::Initialize init;
				if (ParseMsg(data, init))
				{
					//ȡ��Ӧ�������ļ���������

					common::RefreshConfig rc;
					rc.set_server_type(init.server_type());
					for (auto kvp : configs)
					{
						if (kvp.first.first == init.server_type())
						{
							common::ConfigFile* file = rc.add_file();
							file->set_file_name(kvp.first.second);
							file->set_content(*kvp.second);
						}
					}
													   
					ReplyMsg(sessionptr, MSG_TYPE_REFRESH_CONFIG, rc);
				}												   
			}
			break;
		case MSG_TYPE_CONTROL_SERVER_SAY_HELLO:
			{
				common::Hello hello;
				if (ParseMsg(data, hello))
				{
					IPPort* ip_port = new IPPort(sessionptr->GetRemoteIPu(), hello.my_listening_port());

					//ά��session����ַ��ӳ��
					sessionptr->SetAttachedData((long)ip_port);
					server2session[*ip_port] = sessionptr;

					if (hello.is_server_start() == 1)
					{
						auto info_it = servers_info.find(*ip_port);
						if (info_it == servers_info.end())
						{
							ServerInfo* info = new ServerInfo(*ip_port, hello.server_type());
							servers_info.insert(std::make_pair(*ip_port, info));

							//ά��sessions_groupby_servertype
							auto it_group = server_groups.find(hello.server_type());
							if (it_group != server_groups.end())
							{
								it_group->second->insert(*ip_port);
							}

							common::HelloResult hello_result;
							hello_result.set_server_id(info->addr.server_id());
							ReplyMsg(sessionptr, MSG_TYPE_CONTROL_SERVER_SAY_HELLO_RESULT, hello_result);

							if (!during_startup)
							{
								informAddressInfo(info->addr, MSG_TYPE_CONTROL_SERVER_ADDR_INFO_ADD);
								LOGEVENTL("INFO", "Send add to all. " << _ln("server_id") << info->addr.server_id()
									<< _ln("IP") << utility2::toIPs(info->addr.ip()) << _ln("Port") << info->addr.port()
									<< _ln("server_type") << hello.server_type());
							}
						}
						else
						{
							//������ˣ�˵����ͬһ̨�����������ˡ���Ϊֻ��������ʱ�������(is_server_startΪ1)��
							//server_id�Ͳ��ñ��ˣ�����server_typeҲ����䡣��ôֻҪˢtimer�ͺ���
							//��ʱ���ܸ������յ�RESTART��������ʲô����
							info_it->second->refresh();

							common::HelloResult hello_result;
							hello_result.set_server_id(info_it->second->addr.server_id());
							ReplyMsg(sessionptr, MSG_TYPE_CONTROL_SERVER_SAY_HELLO_RESULT, hello_result);

							if (!during_startup)
							{
								common::ServerId _id;
								_id.set_server_id(info_it->second->addr.server_id());
								informAddressInfo(_id, MSG_TYPE_CONTROL_SERVER_ADDR_INFO_RESTART);
								LOGEVENTL("INFO", "Send restart to all. " << _ln("server_id") << _id.server_id());
							}
						}

						if (!during_startup)
						{
							//�����������˵���Ϣ
							common::AddressList addr_list;
							GenerateAddressList(addr_list);
							ReplyMsg(sessionptr, MSG_TYPE_CONTROL_SERVER_ADDR_INFO_REFRESH, addr_list);
							LOGEVENTL("INFO", "Send list to the new one. " << _ln("IP") << utility2::toIPs(ip_port->first) << _ln("Port") << ip_port->second);
						}
					}
					else
					{
						LOGEVENTL("INFO", "Receive Hello but not server_start. " << _ln("IP") << utility2::toIPs(ip_port->first) << _ln("Port") << ip_port->second);
					}
				}
			}
			break;
		case MSG_TYPE_CONTROL_SERVER_REPORT_LOAD:
			//TODO
			break;
		case MSG_TYPE_CMD2SERVER:
			{
				//�Ӻ�̨�����������̨�Ǳ߻����и�Ȩ�޵���֤��TODO
				common::Cmd2Server cmd;
				if (ParseMsg(data, cmd))
				{
					auto it_group = server_groups.find(cmd.to_server_type());
					if (it_group != server_groups.end())
					{
						cmd.clear_to_server_type();
						for (auto it_server = it_group->second->begin(); it_server != it_group->second->end(); ++it_server)
						{
							auto it_session = server2session.find(*it_server);
							if (it_session != server2session.end())
							{
								ReplyMsg(it_session->second, MSG_TYPE_CMD2SERVER, cmd);
							}
						}
					}
				}
			}
			break;		
		case MSG_TYPE_REFRESH_CONFIG:
			{
				//�Ӻ�̨������ˢ�����ļ������̨�Ǳ߻����и�Ȩ�޵���֤��TODO
				common::RefreshConfig rc;
				if (ParseMsg(data, rc))
				{
					auto it_group = server_groups.find(rc.server_type());
					if (it_group != server_groups.end())
					{
						for (int i = 0; i < rc.file_size(); ++i)
						{
							LOGEVENTL("INFO", "new config, " << _ln("server_type") << rc.server_type() << _ln("file_name") << rc.file(i).file_name());

							writeConfig(rc.server_type(), rc.file(i).file_name(), rc.file(i).content());

							//������ظ�����

							//TOMODIFY
							rc.clear_server_type();
							for (auto it_server = it_group->second->begin(); it_server != it_group->second->end(); ++it_server)
							{
								auto it_session = server2session.find(*it_server);
								if (it_session != server2session.end())
								{
									ReplyMsg(it_session->second, MSG_TYPE_REFRESH_CONFIG, rc);
								}
							}
						}
						ReplyEmptyMsg(sessionptr, MSG_TYPE_REFRESH_CONFIG_RESPONSE);
					}
				}
			}
			break;
		case MSG_TYPE_CONTROL_SERVER_FETCH_CONFIG_REQUEST:
			{
				control_server::FetchConfigRequest fc;
				if (ParseMsg(data, fc))
				{
					common::ConfigFile cf;
					cf.set_file_name(fc.file_name());
					std::string* content = readConfig(fc.server_type(), fc.file_name());
					if (content == nullptr)
					{
						cf.set_content("");
					}
					else
					{
						cf.set_content(*content);
					}
					ReplyMsg(sessionptr, MSG_TYPE_CONTROL_SERVER_FETCH_CONFIG_RESPONSE, cf);
				}
			}
			break;
		case MSG_TYPE_CONTROL_SERVER_CMD:
			if (sessionptr->GetRemoteIP() == "127.0.0.1")
			{
				control_server::Command cmd;
				if (ParseMsg(data, cmd))
				{
					if (cmd.command_name() == "reload")
					{
						LOGEVENTL("Info", "Reload config: " << config_file_path);
						LoadConfig();
						
						control_server::CommandResult cmd_result;
						cmd_result.set_result("config reloaded.");
						ReplyMsg(sessionptr, MSG_TYPE_CONTROL_SERVER_CMD_RESULT, cmd_result);
					}
					else if (cmd.command_name() == "refresh_config")
					{
						LOGEVENTL("Info", "Refresh all the configs to these servers");
						initializeConfigs();

						common::RefreshConfig rc;
						rc.set_server_type(init.server_type());

						//TOMODIFY

						for (auto kvp : configs)
						{
							if (kvp.first.first == init.server_type())
							{
								common::ConfigFile* file = rc.add_file();
								file->set_file_name(kvp.first.second);
								file->set_content(*kvp.second);
							}
						}

						ReplyMsg(sessionptr, MSG_TYPE_REFRESH_CONFIG, rc);

						control_server::CommandResult cmd_result;
						cmd_result.set_result("all the configs have been refreshed.");
						ReplyMsg(sessionptr, MSG_TYPE_CONTROL_SERVER_CMD_RESULT, cmd_result);
					}
					else
					{
						control_server::CommandResult cmd_result;
						cmd_result.set_result("CommandName not recognized: " + cmd.command_name());
						ReplyMsg(sessionptr, MSG_TYPE_CONTROL_SERVER_CMD_RESULT, cmd_result);
					}
				}
				else
				{
					control_server::CommandResult cmd_result;
					cmd_result.set_result("Protobuf parse failed");
					ReplyMsg(sessionptr, MSG_TYPE_CONTROL_SERVER_CMD_RESULT, cmd_result);
				}
			}
			else
			{
				LOGEVENTL("WARN", "None-local address tries to send command to ControlServer. IP: " << sessionptr->GetRemoteIP());
			}
			break;
		default:
			LOGEVENTL("WARN", "MsgType not recognized: " << SERVER_MSG_TYPE(data));
			break;
		}
	}
	else
	{
		LOGEVENTL("ERROR", "message not enough size: " << SERVER_MSG_LENGTH(data));
	}
}