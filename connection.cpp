/*
Course: CNT5505 - Data Networks and Communications
Semester: Spring 2016
Name: James Bach, Becky Powell
*/

#include "connection.h"


string Connection::ultostr(unsigned long int x) const
{
	stringstream ss;
	string result;
	ss << setfill('0') << setw(MSGMAX) << to_string(x);
	return ss.str();
}
void Connection::sendPacket(const Ethernet_Pkt& e, int fd)
{
	if (fd == FAILURE)
	{
		cerr << "(sendpacket) NO AVAILABLE CONNECTIONS" << endl;
		return;
	}
	string msg = e.serialize();
	string send_msg = ultostr(msg.length()) + msg;
	DBGOUT("SENDING PACKET TO: " << fd << endl << send_msg);
	write(fd, send_msg.c_str(), send_msg.length());
	
}

bool Connection::readPreamble(int fd, char* buffer)
{
	int bytes_read = read(fd, buffer, MSGMAX);
	return bytes_read > 0;
}

char* Connection::receivePacket(int sock, char* buffer)
{
	buffer[MSGMAX] = 0;			
	PREAMBLE len = strtoul(buffer, NULL, 10);
	DBGOUT("RECEIVED MSG OF LENGTH " << len);
	char* msg = new char[len + 1];
	memset(msg, 0, len + 1);
	read(sock, msg, len);
	msg[len] = 0;
	DBGOUT("***" << msg << "***");
	return msg;
}

/*
bool Connection::msgIsValid(char* msg) const 
{
	return !(strcmp(msg, " ") == 0 
	|| strcmp(msg, "\n") == 0 
	|| strcmp(msg, "\r") == 0 
	|| strlen(msg) == 0);
}
*/

int Connection::initReadSet(fd_set& rs, int ms, list<int> *cL)
{
	int maxSocket = ms;
	FD_ZERO(&rs);
	FD_SET(ms, &rs);
	FD_SET(KEYBOARD, &rs);
	if (cL != nullptr)
	{
		for (auto it = cL->begin(); it != cL->end(); it++)
		{
			FD_SET(*it, &rs); //set fd_set
			maxSocket = max(maxSocket, *it); //get a max to socket number
		}
	}
	return maxSocket + 1;
}

addrinfo Connection::getHints(int flags)
{
	addrinfo h;
	memset(&h, 0, sizeof(h));
	h.ai_family = AF_UNSPEC;
	h.ai_socktype = SOCK_STREAM;
	h.ai_flags = AI_PASSIVE | flags;
	return h;
}

sockaddr_in Connection::getSockAddrInfo(int port)
{
	sockaddr_in sa;
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = port;
	sa.sin_addr.s_addr = htonl(INADDR_ANY);
	return sa;
}
