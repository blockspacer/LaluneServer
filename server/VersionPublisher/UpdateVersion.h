#ifndef __UPADATE_VERSION_H
#define __UPADATE_VERSION_H

#include <string>
#include <iostream>
#include <vector>
#include "Version.pb.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/date_time.hpp>

#include <boost/foreach.hpp>
using namespace boost::property_tree;
using namespace boost::gregorian;
using namespace boost;
using namespace std;
class UpdateVersion
{
public:
	UpdateVersion();
	~UpdateVersion();
	bool SendIformation(const string &file_information);//������Ϣ��controlserver
	bool Uploading();//�ϴ�ȫ���ļ�
	string UploadingOne(const string name);//�ϴ�һ���ļ�������url
	bool Input();
	bool DelFile();//ɾ�������ļ�
	string op_command;

private:
	vector<string> file_name;//��Ҫ���»�ɾ�����ļ���
	vector<string> file_url;//��Ҫ���µ��ļ�·�����ձ�ʾɾ��
	string pro_version;//ǰһ���汾��
	string now_version;//���ڵİ汾��
	string common_url;


};



#endif