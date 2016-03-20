/*
Course: CNT5505 - Data Networks and Communications
Semester: Spring 2016
Name: James Bach, Becky Powell
*/

#include "bridge.h"


Bridge::Bridge(string name, size_t ports) : current_ports(0), ttlthread(&Bridge::TTLTimer, this), isStopped(false)
{	
	max_ports = ports;
	lan_name = name;
	int optval = true;
	char hostname[HOST_NAME_MAX];
	sockaddr_in serv_addr = getSockAddrInfo(htons(0));
	addrinfo hints = getHints(AI_CANONNAME | AI_PASSIVE), *serv_info, *p;
	socklen_t salen = sizeof(serv_addr);
	memset(hostname, 0, HOST_NAME_MAX);
	
	if (gethostname(hostname, HOST_NAME_MAX) == FAILURE)
	{
		cerr << "Failed to get hostname" << endl;
		exit(1);
	}
	if (getaddrinfo(hostname, NULL, &hints, &serv_info) == FAILURE) {
		cerr << "Failed to get address info" << endl;
		exit(1);
	}
	for (p = serv_info; p != NULL; p = p->ai_next)
	{
		if ((main_socket = socket(AF_INET, SOCK_STREAM, 0)) == FAILURE)
		{
			continue;
		}
		if (setsockopt(main_socket, SOL_SOCKET,	SO_REUSEADDR, &optval, sizeof(int)) == FAILURE)
		{
			cerr << "Socket options failed." << endl;
			exit(1);
		}
		if (bind(main_socket, p->ai_addr, p->ai_addrlen) == FAILURE)
		{
			close(main_socket);
			continue;
		}
		break;
	}
	sockaddr_in *ipv4 = (sockaddr_in*)p->ai_addr;
	localIp = ipv4->sin_addr.s_addr;
	if (p == NULL)
	{
		cerr << "Failed to bind." << endl;
		exit(1);
	}
	freeaddrinfo(serv_info);
	if (listen(main_socket, BACKLOG) == FAILURE)
	{
		cerr << "Listen failed." << endl;
		exit(1);
	}
	if (getsockname(main_socket, (sockaddr *)&serv_addr, &salen) == FAILURE)
	{
		cerr << "No socket created!" << endl;
		exit(1);
	}
	open_port = ntohs(serv_addr.sin_port);
	GenerateInfoFiles();
}

void Bridge::GenerateInfoFiles()
{
	string file_prefix = "." + lan_name + ".";
	pFile = file_prefix + "port";
	aFile = file_prefix + "addr";
	symlink(to_string(open_port).c_str(), pFile.c_str());
	symlink(ipv4_2_str(localIp).c_str(), aFile.c_str());	
	//DBGOUT("CREATED FILE " << pFile << " POINTING TO " << to_string(open_port).c_str());
	//DBGOUT("CREATED FILE " << aFile << " POINTING TO " << ipv4_2_str(localIp).c_str());
}

bool Bridge::checkExitServer()
{
	if (FD_ISSET(KEYBOARD, &readset)) //keyboard
	{
		string input;
		getline(cin, input);
		if (input == "exit")
		{
			return true;
		}
		if (input == "ttl")
		{
			printConnections();
		}
	}
	return false;
}

void Bridge::checkNewConnections()
{
	if (FD_ISSET(main_socket, &readset))
	{
		int options, cd;
		addrinfo hints = getHints(AI_CANONNAME | AI_PASSIVE);
		addrinfo *serv_info;
		sockaddr_in client_addr;
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
		if (getaddrinfo(ipv4_2_str(client_addr.sin_addr.s_addr).c_str(), NULL, &hints, &serv_info) == FAILURE)
		{
			cerr << "Failed to get address info " << endl;
			exit(1);
		}
		if (current_ports < max_ports)
		{
			current_ports++;
			DBGOUT("PORTS AVAILABLE: " << max_ports - current_ports);
			cout << "connect from '" + string(serv_info->ai_canonname) + "' at '" + to_string(ntohs(client_addr.sin_port)) + "'" << endl;	
			connected_ifaces[to_string(ntohs(client_addr.sin_port))] = cd;
			printConnections();
			string response("accept");
			write(cd, response.c_str(), response.length());	
		}
		else
		{
			string response("reject");
			write(cd, response.c_str(), response.length());		
			close(cd);
		}
	
		freeaddrinfo(serv_info);
	}
}

void Bridge::checkNewMessages()
{
	unordered_map<string, int> closed_ifaces;
	for (auto it = connected_ifaces.begin(); it != connected_ifaces.end(); ++it)
	{
		
		if (FD_ISSET(it->second, &readset))
		{	
			sockaddr_in client_addr;
			socklen_t calen = sizeof(client_addr);
			char buffer[MSGMAX + 1];
			memset(buffer, 0, MSGMAX + 1);
			if (!readPreamble(it->second, buffer))
			{
				//disconnect
				current_ports--;
				getpeername(it->second, (sockaddr*)&client_addr, &calen);
				cout << "(" << to_string(ntohs(client_addr.sin_port)) << ")" << string(inet_ntoa(client_addr.sin_addr)) << " has disconnected." << endl;
				closed_ifaces[it->first] = it->second;
				DBGOUT("PORTS AVAILABLE: " << max_ports - current_ports);
				
			}
			else
			{
				getpeername(it->second, (sockaddr*)&client_addr, &calen);
				char* msg = receivePacket(it->second, buffer); //reads packet into msg  	
				Ethernet_Pkt e(msg);
				//DBGOUT("FORWARDING PACKET" << e.serialize());
				mtx.lock();
				connections[e.src] = ConnectionEntry(it->second); //adds AND refreshes entry
				printConnections();
				//DBGOUT("e.dst is " << e.dst);
				//DBGOUT("e.src is " << e.src);
				if (e.dst != NOENTRY && connections.count(e.dst) > 0) // it knows the destination
				{
					//DBGOUT("FOUND MAC - SENDING MSG TO:" << e.dst << " ON " << connections[e.dst].port);
					sendPacket(e, connections[e.dst].port);
				}
				else //broadcast message
				{
					//DBGOUT("MAC NOT FOUND - BROADCASTING MSG");
					for (auto it2 = connected_ifaces.begin(); it2 != connected_ifaces.end(); it2++)
					{
						if (it->second != it2->second)
						{
							sendPacket(e, it2->second);	
						}
					}
					DBGOUT("END OF BROADCASTING MSG");
				}
				mtx.unlock();				
				delete msg;
			}
		}
	}
	for (auto it = closed_ifaces.begin(); it != closed_ifaces.end(); ++it)
	{
		DBGOUT("REMOVING CLOSED CONNECTIONS: " << it->first);
		close(it->second);
		connected_ifaces.erase(it->first);
	}
}

void Bridge::printConnections() const
{
	string dhr("=======================================================\n");
	string hr("--------------------------------------------------------\n");
	cout << dhr << "BROADCAST CONNECTION LIST\n" << dhr;
	cout << setw(7) << left << "FD" << endl;
	cout << hr;
	for (auto it = connected_ifaces.begin(); it != connected_ifaces.end(); ++it)
	{
		cout << setw(7) << left << it->second << setw(20) << it->first << endl;
	}
	cout << dhr;
	cout << dhr << "MAC CACHE\n" << dhr;
	cout << setw(INET_MACSTRLEN) << left << "MAC ADDRESS" << setw(7) << left << "FD" << setw(5) << "TTL" << endl;
	cout << hr;

	for (auto it = connections.begin(); it != connections.end(); ++it)
	{
		cout << setw(INET_MACSTRLEN) << left << it->first;
		cout << setw(7) << left << it->second.port;
		cout << setw(5) << left << it->second.TTL << endl;
	}
	cout << dhr;
}


void Bridge::TTLTimer()
{
	bool isRunning = true;
	while (isRunning)
	{
		this_thread::sleep_for(std::chrono::seconds(1));
		mtx.lock();
		unordered_map<MacAddr, ConnectionEntry> remove;
		for (auto it = connections.begin(); it != connections.end(); ++it)
		{
			if (--(it->second.TTL) <= 0)
			{
				remove[it->first] = it->second;
			}
		}
		for (auto it = remove.begin(); it != remove.end(); ++it)
		{
			connections.erase(it->first);
		}
		isRunning = !isStopped;
		mtx.unlock();
		
	}
}

bool Bridge::ioListen()
{
	if (select(initReadSet(readset, connected_ifaces, main_socket), &readset, NULL, NULL, NULL) == FAILURE) //read up to max socket for activity
	{
		cerr << "Select failed" << endl;
		exit(1);
	}
	if (checkExitServer()) return false;
	checkNewConnections();
	checkNewMessages();
	return true;
}

Bridge::~Bridge()
{
	/* add file removal for cwd of .lan-name.addr/port*/
	for (auto it = connected_ifaces.begin(); it != connected_ifaces.end(); it++)
	{
		close(it->second);
	}
	close(main_socket);
	mtx.lock();
	isStopped = true;
	mtx.unlock();
	ttlthread.join();	
}