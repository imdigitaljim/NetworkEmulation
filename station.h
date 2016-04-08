/*
Course: CNT5505 - Data Networks and Communications
Semester: Spring 2016
Name: James Bach, Becky Powell
*/

#ifndef _STATION_H
#define _STATION_H

#define MAXHOSTS 32
#define MAXINTER 32
#define PORTMAX 10
#define CONNECTIONRESPONSE 7
#define TIMEOUT 2
#define MAXFAIL 5


#include "connection.h"
#include "tables.h"
#define MAXTHREAD 5

class Station : public Connection
{
	
	public:
		Station(bool isRouter, bool isDebug, string iface, string rtable, string hostfile);
		~Station();
		
		bool ioListen();
		void printTables() const;
		void printARPCache() const;

	private:
		Ethernet_Pkt buildMessagePkt(string dest, string msg);
		Ethernet_Pkt buildReturnPkt(const Ethernet_Pkt& e);
		Ethernet_Pkt buildRoutedPkt(const Ethernet_Pkt& e);
		
		
		void SendAwaitingARP(const Ethernet_Pkt& e);
		void UpdateARPCache(const Ethernet_Pkt& e);
		
		void populateHosts(string hostfile);
		void populateRouting(string rtable);
		void populateInterfaces(string iface);
		void connectbridges();
		
		pair<string,string> readLinks(string name) const;
		bool isConnectionAccepted(int fd);
		
		bool ownsPacket(const Ethernet_Pkt& e);
		int getConnection(const Ethernet_Pkt& e) const;		
		
		const bool isRouter;					
		bool DebugON;
		
		list<Host> host_list;
		list<Route> routing_table;
		list<Interface> iface_list;
		list<ARP_Entry> arp_cache;
		list<Ethernet_Pkt> arp_queue;
};


#endif

