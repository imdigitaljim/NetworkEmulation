/*
Course: CNT5505 - Data Networks and Communications
Semester: Spring 2016
Name: James Bach, Becky Powell
*/

#include "station.h"

using namespace std;

int main(int argc, char** argv)
{	
	if (argc != 5)
	{
		cerr << "usage: " << argv[0] << " <-no -route> <interface> <routingtable> <hostname>" << endl;
		exit(1);
	}
	/* initialization of hosts, interface, and routing tables */
	Station conn(argv[1], argv[2]);	
	/* hook to the lans that the station should connected to
	* note that a station may need to be connected to multilple lans */
	while(true)
	{
	/* monitoring input from users and bridges
	* 1. from user: analyze the user input and send to the destination if necessary
	* 2. from bridge: check if it is for the station. Note two types of data
	* in the ethernet frame: ARP packet and IP packet.
	* for a router, it may need to forward the IP packet*/
		
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
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
*/