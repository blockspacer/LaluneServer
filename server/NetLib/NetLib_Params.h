#ifndef _NETLIB_PARAMS_
#define _NETLIB_PARAMS_

extern int UDP_REPLY_INTERVAL_MS; // (90)
extern int UDP_MAX_CONSECUTIVE_RESEND_TIMES; // (5) ////同一包连续RESEND超过该次数则认为失败

#define UDP_DEFAULT_PACKET_TIMEOUT_INTERVAL_MS (500) //Timeout初始值
#define UDP_MINIMAL_PACKET_TIMEOUT_INTERVAL_MS (500)
#define UDP_MAXIMUM_PACKET_REPLY_INTERVAL_MS (460) //得比UDP_MINIMAL_PACKET_TIMEOUT_INTERVAL_MS小一些，否则连续收到包容易超时。

#define __MSS	(1500)
#define STARTUP_CWND_SIZE	(__MSS)
#define MINIMAL_CWND_SIZE	(__MSS)
#define INITIAL_CONGESTION_THRESHOLD (66560)

#define BASIC_TIMEOUT_INTERVAL (140) //避免根据EstimateRTT和DevRTT算出来的Timeout小于ReplyInterval

extern unsigned int MAX_PACKET_SIZE;

extern int SERVER_RECV_BUFFER_SIZE;

extern int CLIENT_RECV_BUFFER_SIZE;

#endif