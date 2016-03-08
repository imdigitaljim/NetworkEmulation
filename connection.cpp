/*
Course: CNT5505 - Data Networks and Communications
Semester: Spring 2016
Name: James Bach, Becky Powell
*/

#include "connection.h"

string Connection::ultostr(unsigned long int x)
{
	stringstream ss;
	string result;
	ss << setfill('0') << setw(MSGMAX) << to_string(x);
	return ss.str();
}

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

void Connection::getMessageBuffer(int sock, int bytes)
{
	buffer[MSGMAX] = 0;			
	unsigned long len = strtoul(buffer, NULL, 10);
	msg = new char[len + 1];
	memset(msg, 0, len + 1);
	read(sock, msg, len);
	msg[len] = 0;
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
