#include "UpdateVersion.h"
#include "ServerCommon.h"
#include "MessageTypeDef.h"
UpdateVersion::UpdateVersion()
{
}

UpdateVersion::~UpdateVersion()
{
}
bool UpdateVersion::SendIformation()
{
	ptree pt;
	struct node{
		string now_ve;
		string next_ve;
		string path;
		string url;
	};
	vector<node> version;
	try{
		read_json("version_control.txt", pt);
	}
	catch (ptree_error & e) {
		return 1;
	}
	ptree pt_root;
	pt_root = pt.get_child("root");  // get_child�õ��������   



	BOOST_FOREACH(boost::property_tree::ptree::value_type &v, pt_root)  //����ÿ��ptree
	{
		

		node temp_node;
		temp_node.now_ve = v.second.get<string>("now_ve");
		temp_node.next_ve = v.second.get<string>("next_ve");
		temp_node.path = v.second.get<string>("path");
		temp_node.url = v.second.get<string>("url");
		version.push_back(temp_node);
		//cout << v.second.get<string>("now_ve") << endl;
		//cout << v.second.get<string>("next_ve") << endl;
		//cout << v.second.get<string>("path") << endl;
		//cout << v.second.get<string>("url") << endl;
	}
	node temp_node;
	temp_node.now_ve = pro_version;
	temp_node.next_ve = now_version;
	int i;
	string file_name1 = "";
	for (i = 0; i < file_name.size(); i++)//�ļ�ƴ��
	{

		file_name1 += (file_name[i]+"|");
	}
	temp_node.path = file_name1 ;
	string file_url1 = "";
	for (i = 0; i < file_url.size(); i++)//urlƴ��
	{
		file_url1 += (file_url[i] + "|");
	}
	temp_node.url = file_url1;
	version.push_back(temp_node);
	ptree temp_pt, temp_pt1;
	for (i = 0; i < version.size(); i++)
	{
		temp_pt.put<string>("now_ve", version[i].now_ve);
		temp_pt.put<string>("next_ve", version[i].next_ve);
		temp_pt.put<string>("path", version[i].path);
		temp_pt.put<string>("url", version[i].url);
		temp_pt1.push_back(make_pair("", temp_pt));
	}
	pt.put_child("root", temp_pt1);

	std::stringstream s2;
	write_json(s2, pt);
	std::string outstr = s2.str();
	//cout << outstr << endl;
	write_json("version_control.txt", pt);
	

	return true;
}
bool UpdateVersion::Uploading()
{
	int i;
	string url;
	for (i = 0; i < file_name.size(); i++)
	{
		url=UploadingOne(file_name[i]);
		file_url.push_back(url);
	}
	return true;
}
string UpdateVersion::UploadingOne(const string name)
{
	string url="";
	return url;

}
bool UpdateVersion::Input()
{
	string str;
	cout << "ǰһ���汾��" << endl;
	cin >> pro_version;
	cout << "��ǰ�汾��" << endl;
	cin >> now_version;
	cout << "�����ļ���������end����" << endl;
	while (cin>>str&&str!="end")
	{
		file_name.push_back(str);
	}
	return true;
	
}