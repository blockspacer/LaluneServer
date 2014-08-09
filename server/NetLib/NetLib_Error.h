#ifndef _NETLIB_ERROR_H_
#define _NETLIB_ERROR_H_

enum NetLib_Error{
	no_error = -1, //ö��ֵno_error�����ڲ�ʹ�ã����ⲿ��˵��һ������Failed���������Ǹ���
	close_by_local = 0,
	client_connect_error = 2,
	client_connect_error_std_ex = 3,
	client_cancel_connect_by_local = 4,
	client_send_error = 10,
	client_recv_error,
	server_tcp_open_fail = 20,
	server_tcp_bind_fail,
	server_tcp_listen_fail,
	server_tcp_accept_error,	
	tcp_disconnect_by_remote = 31,
	session_timeout = 35,
	packet_too_big = 40,
	packet_too_small
};

#endif