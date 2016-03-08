/*
Course: CNT5505 - Data Networks and Communications
Semester: Spring 2016
Name: James Bach, Becky Powell
*/

#include "bridge.h"
#include <dirent.h>
#define Directory struct dirent 

using namespace std;

size_t GetPorts(string port);
bool IsValidName(string name);

/* bridge : recvs pkts and relays them*/
int main(int argc, char** argv)
{
	if (argc != 3)
	{
		cerr << "usage: " << argv[0] << " <lan-name> <max-port>" << endl;
		exit(1);
	}

	string name_str(argv[1]);
	size_t port_count = GetPorts(string(argv[2]));
	if (!IsValidName(name_str))
	{
		cerr << name_str << ": already exists!" << endl;
		exit(1);
	}
	Bridge conn(name_str, port_count);
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
size_t GetPorts(string port)
{
	size_t num;
	try
	{
		num = stoi(port); //will throw exception if invalid
		if (num <= 0)
		{
			throw; //throw exception anyways
		}
	}
	catch (const exception& e)
	{
		cerr << "Invalid Port" << endl;
		exit(1);
	}
	return num;
}

bool IsValidName(string name)
{
	string info_file = "." + name + ".";
	DIR *d;
	Directory *dir;
	d = opendir(".");
	if (d)
	{
		while((dir = readdir(d)) != NULL)
		{
			string cwdfile(dir->d_name);		
			if (cwdfile.length() > info_file.length())
			{
				if (cwdfile.substr(0, info_file.length()) == info_file)
				{
					return false;
				}		
			}
		}	
	}
	return true;
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