#include "bridge.h"


Bridge::Bridge(string name, size_t ports) : num_ports(ports)
{
	int optval = true;
	char hostname[HOST_NAME_MAX];
	sockaddr_in serv_addr = getSockAddrInfo(htons(0));
	addrinfo hints = getHints(AI_CANONNAME | AI_PASSIVE), *serv_info;
	socklen_t salen = sizeof(serv_addr);
	memset(hostname, 0, HOST_NAME_MAX);
	main_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (setsockopt(main_socket, SOL_SOCKET,	SO_REUSEADDR, &optval, sizeof(optval)) == FAILURE)
	{
		cerr << "Socket options failed." << endl;
		exit(1);
	}
	if (bind(main_socket, (sockaddr *) &serv_addr, sizeof(serv_addr)) == FAILURE)
	{
		cerr << "Bind failed." << endl;
		exit(1);
	}
	if (listen(main_socket, BACKLOG) == FAILURE)
	{
		cerr << "Listen failed." << endl;
		exit(1);
	}
	if (gethostname(hostname, HOST_NAME_MAX) == FAILURE)
	{
		cerr << "Failed to get hostname" << endl;
		exit(1);
	}	
	if (getaddrinfo(hostname, NULL, &hints, &serv_info) == FAILURE) {
		cerr << "Failed to get address info" << endl;
		exit(1);
	}
	if (getsockname(main_socket, (sockaddr *)&serv_addr, &salen) == FAILURE)
	{
		cerr << "No socket created!" << endl;
		exit(1);
	}
	open_port = ntohs(serv_addr.sin_port);
	cout << "admin: started server on '" << serv_info->ai_canonname;
	cout << "' at '" << open_port << "'" << endl;
	freeaddrinfo(serv_info);
}

void Bridge::checkExitServer()
{
	if (FD_ISSET(KEYBOARD, &readset)) //keyboard
	{
		string input;
		getline(cin, input);
		if (input == "exit")
		{
			exit(0);
		}
	}
}

void Bridge::checkNewConnections()
{
	if (FD_ISSET(main_socket, &readset))
	{
		int options, cd;
		addrinfo hints = getHints(AI_CANONNAME | AI_PASSIVE);
		addrinfo *serv_info;
		sockaddr_in client_addr;
		char ipstr[INET6_ADDRSTRLEN];
		socklen_t calen = sizeof(client_addr);
		if ((cd = accept(main_socket, (sockaddr* ) &client_addr, &calen)) == FAILURE)
		{
			cerr << "Connection failed to connect" << endl;
			exit(1);
		}
		if ((options = fcntl(cd, F_GETFL)) == FAILURE)
		{
			cerr << "Failed to get new socket connection options" << endl;
			exit(1);
		}
		options |= O_NONBLOCK;
		if (fcntl(cd, F_SETFL, options) == FAILURE)
		{
			cerr << "Failed to set new socket connection options" << endl;
			exit(1);
		}
		if (getpeername(cd, (sockaddr *)&client_addr, &calen) == FAILURE)
		{
			cerr << "No socket found!" << endl;
			exit(1);
		}
		inet_ntop(AF_INET, &client_addr.sin_addr, ipstr, sizeof(ipstr));
		if (getaddrinfo(ipstr, NULL, &hints, &serv_info) == FAILURE)
		{
			cerr << "Failed to get address info " << endl;
			exit(1);
		}
		conn_list.push_back(cd);
		cout << "admin: connect from '" + string(serv_info->ai_canonname) + "' at '" + to_string(ntohs(client_addr.sin_port)) + "'" << endl;	
		string admin_msg = "admin: connected to server on '" + string(serv_info->ai_canonname) + "' at '" + to_string(open_port) + "' thru '" + to_string(ntohs(client_addr.sin_port)) + "'";
		string send_msg = ultostr(admin_msg.length()) + admin_msg;
		for (auto it = conn_list.begin(); it != conn_list.end(); it++)
		{
			write(*it, send_msg.c_str(), send_msg.length());			
		}
		freeaddrinfo(serv_info);
	}
}

void Bridge::checkNewMessages()
{
	for (auto it = conn_list.begin(); it != conn_list.end(); ++it)
	{
		if (FD_ISSET(*it, &readset))
		{	
			int bytes_read;
			sockaddr_in client_addr;
			socklen_t calen = sizeof(client_addr);
			memset(buffer, 0, MSGMAX + 1);
			if ((bytes_read = read(*it, buffer, MSGMAX)) == 0)
			{
				//disconnect
				getpeername(*it, (sockaddr*)&client_addr, &calen);
				string admin_msg = "admin: (" + to_string(ntohs(client_addr.sin_port)) + ")" + string(inet_ntoa(client_addr.sin_addr)) + " has disconnected. ";
				cout << admin_msg  << endl;
				close(*it);
				it = conn_list.erase(it);
				string send_msg = ultostr(admin_msg.length()) + admin_msg;
				for (auto it2 = conn_list.begin(); it2 != conn_list.end(); it2++)
				{
					write(*it2, send_msg.c_str(), send_msg.length());	
				}					
			}
			else
			{
				getpeername(*it, (sockaddr*)&client_addr, &calen);			
				getMessageBuffer(*it, bytes_read);	
				if (msgIsValid())
				{
					string peer_msg = "(" + to_string(ntohs(client_addr.sin_port)) + ") " + string(inet_ntoa(client_addr.sin_addr)) + ": " + string(msg);
					cout << peer_msg << endl;
					string send_msg = ultostr(peer_msg.length()) + peer_msg;
					for (auto it2 = conn_list.begin(); it2 != conn_list.end(); it2++)
					{
						if (*it != *it2)
						{
							write(*it2, send_msg.c_str(), send_msg.length());	
						}
					}
				}
				
				delete msg;
			}
		}
	}
}

bool Bridge::msgIsValid()
{
	return !(strcmp(msg, " ") == 0 || strcmp(msg, "\n") == 0 ||
	   strcmp(msg, "\r") == 0 || strlen(msg) == 0);
}


void Bridge::ioListen()
{
	if (select(initReadSet(readset, main_socket, &conn_list), &readset, NULL, NULL, NULL) == FAILURE) //read up to max socket for activity
	{
		cerr << "Select failed" << endl;
		exit(1);
	}
	checkExitServer();
	checkNewConnections();
	checkNewMessages();
}



string Bridge::GetName() const
{
	return lan_name;
}

size_t Bridge::GetPortCount() const
{
	return num_ports;
}


Bridge::~Bridge()
{
	for (auto it = conn_list.begin(); it != conn_list.end(); it++)
	{
		close(*it);
	}
	close(main_socket);
}
