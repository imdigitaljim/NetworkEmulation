/*
Course: CNT5505 - Data Networks and Communications
Semester: Spring 2016
Name: James Bach, Becky Powell
*/

#ifndef _STATION_H
#define _STATION_H

#define MAXHOSTS 32
#define MAXINTER 32
#define PORTMAX
#define CONNECTIONRESPONSE 7
#define TIMEOUT 2
#define MAXFAIL 5

#include "connection.h"


class Station : public Connection
{
	
	public:
		Station(bool isRouter, string iface, string rtable, string hostfile);
		~Station();
		
		void ioListen();
		void connectbridges();
	private:
		void populateHosts(string hostfile);
		void populateRouting(string rtable);
		void populateInterfaces(string iface);
		pair<string,string> readLinks(int index) const;
		bool timeout(int& count) const;
		
		string server_ip;
		vector<Host> host_list;
		vector<Route> routing_table;
		vector<Interface> iface_list;
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
