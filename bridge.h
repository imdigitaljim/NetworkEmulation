/*
Course: CNT5505 - Data Networks and Communications
Semester: Spring 2016
Name: James Bach, Becky Powell
*/

#ifndef _BRIDGE_H
#define _BRIDGE_H

#include "connection.h"

#define TTLMAX 7200

class ConnectionEntry
{	
	public:
		ConnectionEntry(int p = 0) : port(p), TTL(TTLMAX) {}
		int port;
		int TTL; 
};

class Bridge : public Connection
{
	
	public:	
		Bridge(bool isDebug, string name, size_t ports);
		~Bridge(); 
		bool ioListen();
		void TTLTimer();
		void printConnections() const;
	private:
		bool checkExitServer();
		void checkNewConnections();
		void checkNewMessages();

		void GenerateInfoFiles();
		
	
		/*id information about the bridge*/
		string pFile, aFile; //port and address files saved if needed
		int open_port; //port open
		IPAddr localIp;
		string lan_name;
		int main_socket;
		
		/*fd connections to read through*/
		size_t max_ports;
		size_t current_ports;
		
		thread ttlthread;
		bool DebugON;
		bool isStopped;
		mutex mtx;
		/*self learning MAC address*/
		unordered_map<MacAddr, ConnectionEntry> connections; 
};


#endif