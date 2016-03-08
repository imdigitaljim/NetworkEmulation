/*
Course: CNT5505 - Data Networks and Communications
Semester: Spring 2016
Name: James Bach, Becky Powell
*/

#ifndef _STATION_H
#define _STATION_H

#define MAXHOSTS 32
#define MAXINTER 32

#include "connection.h"

typedef unsigned long IPAddr;
typedef string MacAddr;

class Host
{
	public:
		Host(string n, string ip)
		{
			cout << "ADDING HOST: " << n << " " << ip << endl;
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
		Route(string dest, string hop, string mk, string name)
		{
			cout << "ADDING ROUTE: " << dest << " " << hop << " " << mk << " " << name << endl;	
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
		Interface(string name, string ip, string mk, string mac, string lan)
		{
			cout << "ADDING INTERFACE: " << name << " " << mk << " " << ip << " " << mac << " " << lan << endl;
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

class Station : public Connection
{
	
	public:
		Station(bool isRouter, string iface, string rtable, string hostfile);
		~Station();
		
		void ioListen();
		
	private:
		void populateHosts(string hostfile);
		void populateRouting(string rtable);
		void populateInterfaces(string iface);
	
		string server_ip;
		list<Host> host_list;
		list<Route> routing_table;
		list<Interface> iface_list;
};
#endif


/* REMAINING UNUSED/UNCONVERTED TEMPLATE CODE */

/* IP.H */

/* ARP packet types */
#define ARP_REQUEST 0
#define ARP_RESPONSE 1

/* IP protocol types */
#define PROT_TYPE_UDP 0
#define PROT_TYPE_TCP 1
#define PROT_TYPE_OSPF 2







/* ETHER.H */

#define PEER_CLOSED 2
#define TYPE_IP_PKT 1
#define TYPE_ARP_PKT 0

/* structure of an ethernet pkt */
typedef struct __etherpkt 
{
  /* destination address in net order */
  MacAddr dst;

  /* source address in net order */
  MacAddr src;

  /************************************/
  /* payload type in host order       */
  /* type = 0 : ARP frame             */
  /* type = 1 : IP  frame             */
  /************************************/
  short  type;
  
  /* size of the data in host order */
  short   size;

  /* actual payload */
  char *  dat;

} EtherPkt;
