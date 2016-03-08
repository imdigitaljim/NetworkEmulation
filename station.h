/*
Course: CNT5505 - Data Networks and Communications
Semester: Spring 2016
Name: James Bach, Becky Powell
*/

#ifndef _STATION_H
#define _STATION_H

#include "connection.h"


class Station : public Connection
{
	
	public:
		Station(const char* host, const char *port);
		~Station();
		
		void ioListen();
		
	private:
		string server_ip;
};
#endif