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
#include <stdlib.h>
#include <iostream>
#include <stdio.h>
#include <fcntl.h>
#include <cstring>
#include <string>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <iomanip>
#include <sstream>
#include <limits.h>
#include <list>
#include <errno.h>
#include <fstream>
#include <chrono>
#include <thread>
#include <utility>
#include <iomanip>
#include <unordered_map>
#include <mutex>


#include "packet.h"

#define KEYBOARD 0
#define FAILURE -1
#define BACKLOG 5
#define MSGMAX sizeof(unsigned long int)
#define sockaddr_in struct sockaddr_in
#define addrinfo struct addrinfo
#define sockaddr struct sockaddr
#define in_addr struct in_addr
#define INET_MACSTRLEN 18
#define NOENTRY ""

#define DEBUG false
#if DEBUG
#define DBGOUT(x) std::cout << "DEBUG:" << x << std::endl
#else
#define DBGOUT(x)
#endif

using namespace std;

class Connection : public INET
{
	protected:

		int initReadSet(fd_set& rs, const unordered_map<string,int>& connections, int mainSock = -1); // for bridge mainsock passed through
		addrinfo getHints(int flags);
		string ultostr(unsigned long int) const;
		sockaddr_in getSockAddrInfo(int port);
		virtual bool ioListen() = 0;
		
		
		void sendPacket(const Ethernet_Pkt& e, int fd);
		char* receivePacket(int sock, char* buffer);
		bool readPreamble(int fd, char* buffer);
		unordered_map<string, int> connected_ifaces; //interface name (or port) = fd
		fd_set readset;
				
};




#endif