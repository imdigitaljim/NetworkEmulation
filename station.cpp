/*
Course: CNT5505 - Data Networks and Communications
Semester: Spring 2016
Name: James Bach, Becky Powell
*/

#include "station.h"

void Station::populateHosts(string hostfile)
{
	fstream fs(hostfile.c_str());
	string name, ip;
	while (!fs.eof())
	{
		fs >> name;
		fs >> ip;
		host_list.push_back(Host(name, ip));
	}
	fs.close();
}
void Station::populateRouting(string rtablefile)
{
	fstream fs(rtablefile.c_str());
	string dest, hop, mask, iface;
	while (!fs.eof())
	{
		fs >> dest;
		fs >> hop;
		fs >> mask;
		fs >> iface;
		routing_table.push_back(Route(dest, hop, mask, iface));
	}
	fs.close();
}
void Station::populateInterfaces(string ifacefile)
{
	fstream fs(ifacefile.c_str());
	string iface, ip, mk, mac, lan;
	while (!fs.eof())
	{
		fs >> iface;
		fs >> ip;
		fs >> mk;
		fs >> mac;
		fs >> lan;
		iface_list.push_back(Interface(iface, ip, mk, mac, lan));
	}
	fs.close();
	
}
Station::Station(bool isRouter, string ifacefile, string rtablefile, string hostfile)
{
	populateInterfaces(ifacefile);
	populateRouting(rtablefile);
	populateHosts(hostfile);
	return;
	
	const char* host = "aksudhaskdu";
	const char* port = "12355";
	addrinfo *server_info;
	sockaddr_in sa = getSockAddrInfo(htons(atoi(port))), *h;
	if (inet_aton(host, (in_addr *) &sa.sin_addr.s_addr))
	{
		if ((main_socket = socket(AF_INET, SOCK_STREAM, 0)) == FAILURE)
		{
			cerr << "Failed to create socket" << endl;
			exit(1);
		}
		if (connect(main_socket, (sockaddr *) &sa, sizeof(sa)) == FAILURE)
		{
			cerr << "Failed to Connect!" << endl;
			exit(1);
		}
	}
	else
	{
		addrinfo hints = getHints(AI_PASSIVE);
		if (getaddrinfo(host, "http", &hints, &server_info) == FAILURE)
		{
			cerr << host << ": failed to resolve" << endl;
			exit(1);
		}	
		addrinfo *ai_ptr;
		for(ai_ptr = server_info; ai_ptr != NULL; ai_ptr = ai_ptr->ai_next) 
		{
			h = (sockaddr_in *) ai_ptr->ai_addr;
			server_ip = inet_ntoa(h->sin_addr);	
			if (inet_aton(server_ip.c_str(), (in_addr *) &sa.sin_addr.s_addr))
			{
				if ((main_socket = socket(AF_INET, SOCK_STREAM, 0)) == FAILURE)
				{
					continue;
				}

				if (connect(main_socket, (sockaddr *) &sa, sizeof(sa)) == FAILURE)
				{
					continue;
				}
				
				break; //successful connect
			}
		}
		if (ai_ptr == NULL)
		{
			cerr << "Could not connect" << endl;
			exit(1);
		}
		freeaddrinfo(server_info);
	}
}

void Station::ioListen()
{
	if (select(initReadSet(readset, main_socket), &readset, NULL, NULL, NULL) == FAILURE) //read up to max socket for activity
	{
		cerr << "Select failed" << endl;
		exit(1);
	}
	if (FD_ISSET(KEYBOARD, &readset)) //keyboard
	{
		string user_msg;
		getline(cin, user_msg);
		string send_msg = ultostr(user_msg.length()) + user_msg;
		write(main_socket, send_msg.c_str(), send_msg.length());
	}
	if (FD_ISSET(main_socket, &readset)) //server
	{
		int bytes_read;
		if ((bytes_read = read(main_socket, buffer, MSGMAX)) == 0)
		{
			cerr << "Disconnected from server." << endl;
			exit(1);
		}
		else
		{
			getMessageBuffer(main_socket, bytes_read);
			cout << msg << endl;
			delete msg;
		}
	}
}

Station::~Station()
{
	close(main_socket);
}
