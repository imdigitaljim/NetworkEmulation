/*
Course: CNT5505 - Data Networks and Communications
Semester: Spring 2016
Name: James Bach, Becky Powell
*/

#ifndef _BRIDGE_H
#define _BRIDGE_H

#include "connection.h"
#include <unordered_map>
#define TTLMAX 360

class ConnectionEntry
{	
	public:
		ConnectionEntry(int = 0);
		int port;
		double TTL; 
};

class Bridge : public Connection
{
	
	public:	
		Bridge(string name, size_t ports);
		~Bridge(); 
		void ioListen();
		
	private:
		void checkExitServer();
		void checkNewConnections();
		void checkNewMessages();
		bool msgIsValid();
		void GenerateInfoFiles();
		void readMessage(int sock, int bytes); //de-layer the message
		
		string pFile, aFile; //port and address files
		list<int> conn_list;
		int open_port;
		size_t max_ports;
		size_t current_ports;
		string lan_name;
		IPAddr localIp;
		unordered_map<string, ConnectionEntry> connections;
};

#endif