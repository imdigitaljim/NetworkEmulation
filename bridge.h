#ifndef _BRIDGE_H
#define _BRIDGE_H

#include "connection.h"
#include <fstream>

class Bridge : public Connection
{
	
	public:	
		Bridge(string name, size_t ports);
		~Bridge();
		
		void ioListen();
		string GetName() const;
		size_t GetPortCount() const;
	private:
		void checkExitServer();
		void checkNewConnections();
		void checkNewMessages();
		bool msgIsValid();
		
		list<int> conn_list;
		int open_port;
		size_t num_ports;
		string lan_name;
		
};

#endif