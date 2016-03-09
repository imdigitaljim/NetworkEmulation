/*
Course: CNT5505 - Data Networks and Communications
Semester: Spring 2016
Name: James Bach, Becky Powell
*/
#ifndef _SOCKCONN_H
#define _SOCKCONN_H
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <malloc.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <iostream>
#include <stdio.h>
#include <fcntl.h>
#include <cstring>
#include <string>
#include <signal.h>
#include <unistd.h>
#include <iomanip>
#include <sstream>
#include <limits.h>
#include <list>
#include <errno.h>
#include <fstream>
#include <chrono>
#include <thread>
#include <vector>
#include <utility>
#include <iomanip>

#define KEYBOARD 0
#define FAILURE -1
#define BACKLOG 5
#define MSGMAX sizeof(unsigned long int)
#define sockaddr_in struct sockaddr_in
#define addrinfo struct addrinfo
#define sockaddr struct sockaddr
#define in_addr struct in_addr
#define INET_MACSTRLEN 18


#define DEBUG true
#if DEBUG
#define DBGOUT(x) std::cout << "DEBUG:" << x << std::endl
#else
#define DBGOUT(x)
#endif

using namespace std;

typedef unsigned long IPAddr;
typedef string MacAddr;

enum ARPTYPE {ARP_REQUEST, ARP_RESPONSE};
enum FRAMETYPE {IPFRAME, ARPFRAME};

class Connection
{
	protected:
		string ipv4_2_str(IPAddr) const;
		IPAddr str_2_ipv4(string) const;
		int initReadSet(fd_set& rs, int ms, list<int> *cL = nullptr);
		addrinfo getHints(int flags);
		string ultostr(unsigned long int) const;
		sockaddr_in getSockAddrInfo(int port);
		void getMessageBuffer(int sock, int bytes);
		virtual void ioListen() = 0;
		
		int main_socket;
		fd_set readset;
		
		
		char buffer[MSGMAX + 1], *msg;				
};

class Host
{
	public:
		Host(string n, string ip) : addr(0)
		{
			name = n;
			inet_pton(AF_INET, ip.c_str(), &addr);
		}
		string name;
		IPAddr addr;
};

/* Structure for a routing table entry */
class Route
{
	public:
		Route(string dest, string hop, string mk, string name) : destsubnet(0), nexthop(0), mask(0)
		{
			inet_pton(AF_INET, dest.c_str(), &destsubnet);
			inet_pton(AF_INET, hop.c_str(), &nexthop);
			inet_pton(AF_INET, mk.c_str(), &mask);
			ifacename = name;
		}
		
		IPAddr destsubnet;
		IPAddr nexthop;
		IPAddr mask;
		string ifacename;
};

/* Structure to represent an interface */
class Interface
{
	public:
		Interface(string name, string ip, string mk, string mac, string lan) : ipaddr(0), mask(0)
		{
			ifacename = name;
			inet_pton(AF_INET, ip.c_str(), &ipaddr);
			inet_pton(AF_INET, mk.c_str(), &mask);
			macaddr = mac;
			lanname = lan;
		}
		string ifacename;
		IPAddr ipaddr;
		IPAddr mask;
		MacAddr macaddr;
		string lanname;
};

class Interface2Link{
	public:
		Interface2Link(string ifc, int fd) : ifacename(ifc), sockfd(fd){}
		string ifacename;
		int sockfd;
};

class ARP_Entry {
	public:
		ARP_Entry(IPAddr ip, string mac) : ipaddr(ip), macaddr(mac) {}
		IPAddr ipaddr;
		MacAddr macaddr;
};

class ARP_Pkt 
{
	public:
	  short op; /* op =0 : ARP request; op = 1 : ARP response */
	  IPAddr srcip;
	  MacAddr srcmac;
	  IPAddr dstip;
	  MacAddr dstmac;
};

class IP_Pkt
{
	public:
		IPAddr  dstip;
		IPAddr  srcip;
};


class Ethernet_Pkt 
{
	public:	
		MacAddr dst; /* destination address in net order */
		MacAddr src; /* source address in net order */
		FRAMETYPE type; /* enum FRAMETYPE {IPFRAME, ARPFRAME}; */
		string data; 

};

/*queue for ip packet that has not yet sent out*/
class ARPWait_Pkt
{
  IPAddr next_hop_ipaddr;
  IPAddr dst_ipaddr;
  string data;
};

/*queue to remember the packets we have received
typedef struct packet_queue
{
  char *packet;
  int  length;
  short counter;
} OLD_PACKETS;
*/



#endif