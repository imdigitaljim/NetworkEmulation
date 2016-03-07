/*
Course: CNT5505 - Data Networks and Communications
Semester: Spring 2016
Name: James Bach, Becky Powell
*/

#include "bridge.h"

using namespace std;

/* bridge : recvs pkts and relays them*/
int main(int argc, char** argv)
{
	if (argc != 3)
	{
		cerr << "usage: " << argv[0] << " <lan-name> <max-port>" << endl;
		exit(1);
	}
	/* create the symbolic links to its address and port number
	* so that others (stations/routers) can connect to it */
	string port_str(argv[2]);
	size_t port_count;
	try
	{
		port_count = stoi(port_str); //will throw exception if invalid
		if (port_count <= 0)
		{
			throw; //throw exception anyways
		}
	}
	catch (const exception& e)
	{
		cerr << "Invalid Port" << endl;
		exit(1);
	}
	cout << "port_count: " << port_count << endl;
	Bridge conn(string(argv[1]), port_count);
	while(true)
	{
		/* listen to the socket.
		* two cases:
		* 1. connection open/close request from stations/routers
		* 2. regular data packets */
		conn.ioListen();
	}
	return 0;
}

/*
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <malloc.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
*/