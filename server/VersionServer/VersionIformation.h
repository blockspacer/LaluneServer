#ifndef __VERSION_IFORMATION_
#define __VERSION_IFORMATION_
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/date_time.hpp>
#include <string>
#include <vector>
#include <iostream>
#include <vector>
#include <boost/foreach.hpp>
using namespace boost::property_tree;
using namespace boost::gregorian;
using namespace boost;
using namespace std;
class VersionIformation
{
public:
	 vector<string> file_path;//��Ҫ���µ��ļ���
	 vector<string> file_url;//��Ҫ���µ��ļ�·��
	 string now_version;//���ڵİ汾��
	 string next_version;//��һ���İ汾��
	static vector<string> GetVerctor(const string &str);//�����ַ����ֽ�



};
extern vector<VersionIformation> version_infor;
#endif
