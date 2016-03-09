/*
Course: CNT5505 - Data Networks and Communications
Semester: Spring 2016
Name: James Bach, Becky Powell
*/

#include "station.h"

#define NOENTRY ""

Station::Station(bool isRouter, string ifacefile, string rtablefile, string hostfile) : maxSock(0)
{
	populateInterfaces(ifacefile);
	populateRouting(rtablefile);
	populateHosts(hostfile);
#if DEBUG
	printTables();	
#endif
	connectbridges();	
}


void Station::connectbridges() //main socket needs to be a list of sockets
{
	int fd, fails = 0;
	memset(buffer, 0, CONNECTIONRESPONSE);
	for (auto it = iface_list.begin(); it != iface_list.end(); ++it)
	{
		pair<string, string> info = readLinks(it->lanname);
		string host = info.first, port = info.second;
		sockaddr_in sa = getSockAddrInfo(htons(atoi(port.c_str())));
		if (inet_aton(host.c_str(), (in_addr *) &sa.sin_addr.s_addr))
		{
			if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == FAILURE)
			{
				cerr << "Failed to create socket" << endl;
				exit(1);
			}
			if (connect(fd, (sockaddr *) &sa, sizeof(sa)) == FAILURE)
			{
				if (fails++ < MAXFAIL)
				{
					cerr << "Failed to Connect...Retrying..." << endl;				
					this_thread::sleep_for(chrono::seconds(TIMEOUT));
					it--;
				}
				else
				{
					cerr << "Connection to Interface Failed" << endl;
					exit(1);
				}
				continue;
			}
			if (!isConnectionAccepted(fd)) cerr << "Connection Rejected from Bridge" << endl;
			connected_ifaces.push_back(Interface2Link(it->ifacename,fd));
		}		
	}
	for (auto it = connected_ifaces.begin(); it != connected_ifaces.end(); ++it)
	{
		if (maxSock < it->sockfd)
		{
			maxSock = it->sockfd;
		}
	}
	if (maxSock == 0)exit(1); 
}

bool Station::isConnectionAccepted(int fd) //non-blocking timeout
{
	int options, old_options, bytes_read;
	bool result = false;
	char buffer[CONNECTIONRESPONSE];
	if ((old_options = fcntl(fd, F_GETFL)) == FAILURE)
	{
		cerr << "Failed to get new socket connection options" << endl;
		return false;
	}
	options = old_options;
	options |= O_NONBLOCK;
	if (fcntl(fd, F_SETFL, options) == FAILURE)
	{
		cerr << "Failed to set new socket connection options" << endl;
		return false;
	}
	int fails = 0;
	while (fails < MAXFAIL)
	{
		this_thread::sleep_for(chrono::seconds(TIMEOUT));
		if ((bytes_read = read(fd, buffer, CONNECTIONRESPONSE)) > 0)
		{
			buffer[CONNECTIONRESPONSE - 1] = 0;
			result = (strcmp(buffer, "accept") == 0);
			DBGOUT("CONNECTION RESPONSE:" << buffer);
			break;
		}
		cerr << "Failed to Read...Retrying..." << endl;				
		fails++;
	}
	if (fcntl(fd, F_SETFL, old_options) == FAILURE)
	{
		cerr << "Failed to reset socket connection options" << endl;
		exit(1);
	}
	return result;
}


void Station::ioListen()
{
	if (select(initReadSet(readset, maxSock), &readset, NULL, NULL, NULL) == FAILURE) //read up to max socket for activity
	{
		cerr << "Select failed" << endl;
		exit(1);
	}
	if (FD_ISSET(KEYBOARD, &readset)) //keyboard
	{
		string command; 
		cin >> command;
		if (command == "send")
		{
			string destination, user_msg;
			cin >> destination;
			getline(cin, user_msg);
			DBGOUT(destination << user_msg);
			
			string send_msg = buildMessagePkt(destination);
			if (send_msg == NOENTRY)
			{
				cerr << destination << ": HOST NOT FOUND" << endl;
			}
			else if (send_msg == "ARPREQUEST")
			{
					//queue this packet and send ARP;
			}
			else
			{
				
			}
			//DBGOUT(send_msg);
			//send message to user by name (build packet)
			
			//string send_msg = ultostr(user_msg.length()) + user_msg;
			//write(main_socket, send_msg.c_str(), send_msg.length());
		}
		else if (command == "show")
		{
			string input;
			cin >> input;
			if (input == "host")
			{
				printTables();
			}
			else if (input == "arp")
			{
				printARPCache();
			}
			
		}
		else
		{
			cerr << command << ": command not recognized" << endl;
		}
	
	}
	for (auto it = connected_ifaces.begin(); it != connected_ifaces.end(); ++it)
	{
		if (FD_ISSET(it->sockfd, &readset)) //server
		{
			int bytes_read;
			if ((bytes_read = read(it->sockfd, buffer, MSGMAX)) == 0)
			{
				cerr << it->ifacename << "Disconnected from server." << endl;
				connected_ifaces.erase(it--);
			}
			else
			{
				getMessageBuffer(it->sockfd, bytes_read);
				cout << msg << endl;
				delete msg;
			}
		}
	}
	
}

string Station::buildMessagePkt(string dest)
{
	string result, iface_out;
	MacAddr host_mac, peer_mac;
	IPAddr destIP, gatewayIP;
	for (auto it = host_list.begin(); it != host_list.end(); ++it)
	{
		if (it->name == dest)
		{
			destIP = it->addr;
			result = dest;
			DBGOUT("HOST: " << result);
			break;
		}
	}
	if (result == NOENTRY) return NOENTRY;
	for (auto it = routing_table.begin(); it != routing_table.end(); ++it)
	{	
		if (ipv4_2_str(destIP & it->mask) == ipv4_2_str(it->destsubnet))
		{
			gatewayIP = it->nexthop;
			iface_out = it->ifacename;
			DBGOUT("GATEWAYIP: " << ipv4_2_str(gatewayIP) << " ON " << iface_out);
			break;
		}
	}
	for (auto it = iface_list.begin(); it != iface_list.end(); ++it)
	{
		if (it->ifacename == iface_out)
		{
			host_mac = it->macaddr;
			DBGOUT("INTERFACE: " << host_mac);
			break;
		}
	}
	for (auto it = arp_cache.begin(); it != arp_cache.end(); ++it)
	{
		if (it->ipaddr == destIP)
		{
			peer_mac = it->macaddr;
		}
	}
	if (peer_mac == NOENTRY) return "ARPREQUEST";
	//build ip/ethernet packets
	return result;
}
pair<string,string> Station::readLinks(string name) const
{
	char str[INET6_ADDRSTRLEN];
	memset(str,0,INET6_ADDRSTRLEN);
	string aFile = "." + name + ".addr";
	string pFile = "." + name + ".port"; 
	readlink(aFile.c_str(), str, INET6_ADDRSTRLEN);
	string host(str);
	memset(str,0,INET6_ADDRSTRLEN);
	readlink(pFile.c_str(), str, 5);
	string port(str);
	return make_pair(host, port);
}


void Station::populateHosts(string hostfile)
{
	fstream fs(hostfile.c_str());
	string name, ip;
	while (fs >> name)
	{
		fs >> ip;
		host_list.push_back(Host(name, ip));
	}
	fs.close();
}

void Station::populateRouting(string rtablefile)
{
	fstream fs(rtablefile.c_str());
	string dest, hop, mask, iface;
	while (fs >> dest)
	{
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
	while (fs >> iface)
	{
		fs >> ip;
		fs >> mk;
		fs >> mac;
		fs >> lan;
		iface_list.push_back(Interface(iface, ip, mk, mac, lan));
	}
	fs.close();	
}

void Station::printARPCache() const
{
	
}

void Station::printTables() const
{
	string dhr("================================================================================\n");
	string hr("--------------------------------------------------------------------------------\n");
	cout << dhr << "HOSTS\n" << dhr;
	cout << setw(INET_ADDRSTRLEN ) << left << "STATION NAME" << setw(INET_ADDRSTRLEN) << left << "STATION IP" << endl;
	cout << hr;
	for (auto it = host_list.begin(); it != host_list.end(); ++it)
	{
		cout << setw(INET_ADDRSTRLEN) << left << it->name;
		cout << setw(INET_ADDRSTRLEN ) << left << ipv4_2_str(it->addr) << endl;
	}
	cout << dhr << "ROUTING TABLE\n" << dhr;
	cout << setw(INET_ADDRSTRLEN) << left << "DESTINATION IP" << setw(INET_ADDRSTRLEN) << left << "NEXT-HOP IP";
	cout << setw(INET_ADDRSTRLEN) << left << "SUBNET MASK" << setw(INET_ADDRSTRLEN) << left << "INTERFACE NAME" << endl;
	cout << hr;
	for (auto it = routing_table.begin(); it != routing_table.end(); ++it)
	{
		cout << setw(INET_ADDRSTRLEN) << left << ipv4_2_str(it->destsubnet);
		cout << setw(INET_ADDRSTRLEN) << left << ipv4_2_str(it->nexthop);
		cout << setw(INET_ADDRSTRLEN) << left << ipv4_2_str(it->mask);
		cout << setw(INET_ADDRSTRLEN) << left << it->ifacename << endl;
	}
	cout << dhr << "INTERFACES\n" << dhr;
	cout << setw(INET_ADDRSTRLEN) << left << "STATION NAME" << setw(INET_ADDRSTRLEN) << left << "STATION IP";
	cout << setw(INET_ADDRSTRLEN) << left << "SUBNET MASK" << setw(INET_MACSTRLEN) << left << "MAC ADDRESS";
	cout << setw(INET_ADDRSTRLEN) << left << "LANNAME" << endl;
	cout << hr;
	for (auto it = iface_list.begin(); it != iface_list.end(); ++it)
	{
		cout << setw(INET_ADDRSTRLEN) << left << it->ifacename;
		cout << setw(INET_ADDRSTRLEN) << left << ipv4_2_str(it->ipaddr);
		cout << setw(INET_ADDRSTRLEN) << left << ipv4_2_str(it->mask);
		cout << setw(INET_MACSTRLEN) << left << it->macaddr;
		cout << setw(INET_ADDRSTRLEN) << left << it->lanname << endl;
	}
	cout << dhr;
}

Station::~Station()
{
	close(main_socket);
}
