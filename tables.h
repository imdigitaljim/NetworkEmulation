/*
Course: CNT5505 - Data Networks and Communications
Semester: Spring 2016
Name: James Bach, Becky Powell
*/
#ifndef _CONN_TABLES_H
#define _CONN_TABLES_H

#include "connection.h"

class Host : public INET
{
	public:
		Host(string n, string ip) : name(n), addr(0) 
		{
			inet_pton(AF_INET, ip.c_str(), &addr);
		}
		string name;
		IPAddr addr;
};

/* Structure for a routing table entry */
class Route : public INET
{
	public:
		Route(string dest, string hop, string mk, string name)
		: destsubnet(0), nexthop(0), mask(0), ifacename(name)
		{
			inet_pton(AF_INET, dest.c_str(), &destsubnet);
			inet_pton(AF_INET, hop.c_str(), &nexthop);
			inet_pton(AF_INET, mk.c_str(), &mask);			
		}
		IPAddr destsubnet;
		IPAddr nexthop;
		IPAddr mask;
		string ifacename;
};

/* Structure to represent an interface */
class Interface : public INET
{
	public:
		Interface(string name, string ip, string mk, string mac, string lan) 
		: ifacename(name), ipaddr(0), mask(0), macaddr(mac), lanname(lan)
		{
			inet_pton(AF_INET, ip.c_str(), &ipaddr);
			inet_pton(AF_INET, mk.c_str(), &mask);
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

class ARP_Entry : public INET
{
	public:
		ARP_Entry(IPAddr ip, MacAddr mac) : ipaddr(ip), macaddr(mac) {}
		IPAddr ipaddr;
		MacAddr macaddr;
};

#endif