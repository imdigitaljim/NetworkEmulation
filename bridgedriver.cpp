/*
Course: CNT5505 - Data Networks and Communications
Semester: Spring 2016
Name: James Bach, Becky Powell
*/

#include "bridge.h"
#include <thread>

using namespace std;

size_t getPorts(string port);
bool isValidName(string name);

/* bridge : recvs pkts and relays them*/
int main(int argc, char** argv)
{
	if (argc != 3)
	{
		cerr << "usage: " << argv[0] << " <lan-name> <max-port>" << endl;
		exit(1);
	}

	string name_str(argv[1]);
	size_t port_count = getPorts(string(argv[2]));
	if (!isValidName(name_str))
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
		if (!conn.ioListen()) break;
	}
	return 0;
}
size_t getPorts(string port)
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

bool isValidName(string name)
{
	char buff[true];
	string aFile = "." + name + ".addr";
	string pFile = "." + name + ".port";
	return (readlink(aFile.c_str(), buff, true) == FAILURE) 
		&& (readlink(pFile.c_str(), buff, true) == FAILURE);
}


