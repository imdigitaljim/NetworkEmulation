/*
Course: CNT5505 - Data Networks and Communications
Semester: Spring 2016
Name: James Bach, Becky Powell
*/

#include "station.h"

#define MAXSQNUM 100000 
#define LOCALIP "0.0.0.0"

Station::Station(bool route, bool isDebug, string ifacefile, string rtablefile, string hostfile) : isRouter(route), DebugON(isDebug)
{
	/* extract information from tables*/
	populateInterfaces(ifacefile); 
	populateRouting(rtablefile);
	populateHosts(hostfile);
	if (DebugON) printTables();	
	/* connect to bridge(s) */
	connectbridges();	
}


void Station::connectbridges()
{
	int fd, fails = 0;
	for (auto it = iface_list.begin(); it != iface_list.end(); ++it)
	{
		pair<string, string> info = readLinks(it->lanname);
		string host = info.first, port = info.second;
		cout << "Connecting to bridge " << host << " : " << port << "..." << endl;
		sockaddr_in sa = getSockAddrInfo(htons(atoi(port.c_str())));
		if (inet_aton(host.c_str(), (in_addr *) &sa.sin_addr.s_addr))
		{
			if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == FAILURE) //build socket
			{
				cerr << "Failed to create socket" << endl;
			}
			if (connect(fd, (sockaddr *) &sa, sizeof(sa)) == FAILURE) //connect socket
			{
				close(fd);
				if (fails++ < MAXFAIL) //timeout/ reattempting
				{
					cerr << "Failed to Connect...Retrying..." << endl;				
					this_thread::sleep_for(chrono::seconds(TIMEOUT));
					it--; //try it again
				}
				else
				{
					cerr << "Connection to Interface Failed" << endl;
				}
				continue;
			}
			if (!isConnectionAccepted(fd)) cerr << "Connection Rejected from Bridge" << endl;
			cout << "... connected to lan '" << it->lanname << "'" << endl;
			connected_ifaces[it->ifacename] = fd;
		}		
	}
}

bool Station::isConnectionAccepted(int fd) 
//determine if the message from the bridge is accepted while not blocking
{
	int options, old_options, bytes_read;
	bool result = false;
	char buffer[CONNECTIONRESPONSE];
	memset(buffer, 0, CONNECTIONRESPONSE);
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

bool Station::ioListen()
//main looping listen on all sockets and inputs
{
	if (select(initReadSet(readset, connected_ifaces), &readset, NULL, NULL, NULL) == FAILURE) //initi read up to max socket for activity
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
			cin.ignore();
			getline(cin, user_msg);		
			Ethernet_Pkt pkt = buildMessagePkt(destination, user_msg);
			if (pkt.type == NOFRAME)
			{
				cerr << destination << ": host not found." << endl;
			}
			if (pkt.type != ARP_QUEUED)
			{
				sendPacket(pkt, getConnection(pkt));
			}
		}
		else if (command == "debug")
		{
			DebugON = !DebugON;
		}
		else if (command == "exit")
		{
			return false;
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
		else if (command == "connect")
		{
			connectbridges();
		}
		else
		{
			cerr << command << ": command not recognized" << endl;
		}
	
	} 
	
	unordered_map<string, int> closed_ifaces;
	for (auto it = connected_ifaces.begin(); it != connected_ifaces.end(); ++it)
	{
		if (FD_ISSET(it->second, &readset)) //server
		{
			char buffer[MSGMAX + 1];
			memset(buffer, 0, MSGMAX + 1);
			if (!readPreamble(it->second, buffer))
			{
				cerr << it->first << ": disconnected from server." << endl;
				closed_ifaces[it->first] = it->second;
				for (auto it2 = iface_list.begin(); it2 != iface_list.end(); ++it2)
				{
					if (it2->lanname == it->first)
					{
						for (auto it3 = arp_cache.begin(); it3 != arp_cache.end(); ++it3)
						{
							if (it3->macaddr == it2->macaddr)
							{
								arp_cache.erase(it3);
								break;
							}					
						}
						break;
					}			
				}
				continue;
			}
			else
			{
				char* msg = receivePacket(it->second, buffer); //reads packet into msg  	
				Ethernet_Pkt e(msg);

				if (ownsPacket(e) || (isRouter && (ipv4_2_str(e.data.nexthop) != LOCALIP)))
				{
					if (e.type == ARP_REQUEST)
					{
						Ethernet_Pkt response = buildReturnPkt(e);
						sendPacket(response, getConnection(response));
					}
					else if (e.type == ARP_RESPONSE)
					{
						SendAwaitingARP(e);
					}
					else if (isRouter && e.type == IPFRAME)
					{
						if (DebugON) cout << "ROUTING PACKET!" << endl;
						Ethernet_Pkt forward = buildRoutedPkt(e);
						sendPacket(forward, getConnection(forward));
						if (DebugON) printARPCache();
					}
					else if (e.type == IPFRAME)
					{
						cout << "(" << ipv4_2_str(e.data.srcip)<< "):" <<  e.data.msg << endl;
					}
				}		
				delete msg;
			}
		}
	}
	for (auto it = closed_ifaces.begin(); it != closed_ifaces.end(); ++it)
	{
		close(it->second);
		connected_ifaces.erase(it->first);
	}
	return true;
}

void Station::SendAwaitingARP(const Ethernet_Pkt& e)
{
	UpdateARPCache(e);
	for (auto it = arp_queue.begin(); it != arp_queue.end(); ++it)
	{	
		if (e.dst == it->src)
		{
			if (DebugON) cout << "ARP RESPONSE RECEIVED!" << endl << "... SENDING QUEUED MESSAGE(S)" << endl; 
			it->dst = e.src;
			sendPacket(*it, getConnection(*it));
			arp_queue.erase(it++);
		}
	}
}

int Station::getConnection(const Ethernet_Pkt& e) const
{
	for (auto it = connected_ifaces.begin(); it != connected_ifaces.end(); ++it)
	{
		if (it->first == e.iface_out)
		{
			return it->second;
		}
	}
	return FAILURE;
}
Ethernet_Pkt Station::buildRoutedPkt(const Ethernet_Pkt& e)
{
	string host, iface_out;
	MacAddr host_mac, peer_mac;
	IPAddr gatewayIP = 0, stationIP = 0;
	for (auto it = routing_table.begin(); it != routing_table.end(); ++it)
	{	
		if (ipv4_2_str(e.data.dstip & it->mask) == ipv4_2_str(it->destsubnet))
		{
			gatewayIP = it->nexthop;
			if (ipv4_2_str(gatewayIP) == LOCALIP)
			{
				gatewayIP = e.data.dstip;
			}
			iface_out = it->ifacename;
			break;
		}
	}
	if (DebugON) cout << "NEXT HOP: " << ipv4_2_str(gatewayIP) << " --- USING INTERFACE : " << iface_out << endl;
	for (auto it = iface_list.begin(); it != iface_list.end(); ++it)
	{
		if (iface_out == it->ifacename)
		{
			host_mac = it->macaddr;
			stationIP = it->ipaddr;
			break;
		}
	}
	for (auto it = arp_cache.begin(); it != arp_cache.end(); ++it)
	{
		if (ipv4_2_str(gatewayIP) == ipv4_2_str(it->ipaddr))
		{
			peer_mac = it->macaddr;	
			break;
		}
	}
	if (DebugON) cout << "SRC MAC: " << host_mac << " --- " << ipv4_2_str(stationIP) << endl;
	IP_Pkt ipPkt(e.data.dstip, e.data.srcip, gatewayIP, e.data.msg);
	if (peer_mac == NOENTRY) 
	{
		if (DebugON) cout << "DST MAC NOT FOUND!" << endl;
		bool isSent = false;
		for (auto it = arp_queue.begin(); it != arp_queue.end(); ++it)
		{
			if (gatewayIP == it->data.nexthop)
			{
				isSent = true;
				break;
			}
		}
		if (DebugON) cout << "ARP SENT, QUEUING MESSAGE AWAITING RESPONSE!" << endl;
		arp_queue.push_back(Ethernet_Pkt(NOENTRY, host_mac, IPFRAME, ipPkt, iface_out)); //push back awaiting arp response
		ipPkt.msg = NOENTRY; 															 //dont broadcast input
		if (isSent) return Ethernet_Pkt(NOENTRY, host_mac, ARP_QUEUED, ipPkt, iface_out); //if ARP is already pending just skip (dont duplicate ARP)
		return Ethernet_Pkt(NOENTRY, host_mac, ARP_REQUEST, IP_Pkt(e.data.dstip, stationIP, gatewayIP, NOENTRY), iface_out); 
	}
	if (DebugON) cout << "DST MAC: " << peer_mac << endl << "... SENDING MESSAGE" << endl;
	return Ethernet_Pkt(peer_mac, host_mac, IPFRAME, ipPkt, iface_out);
}


Ethernet_Pkt Station::buildReturnPkt(const Ethernet_Pkt& e)
{
	string iface_out;
	MacAddr host_mac;
	IPAddr gatewayIP = 0, stationIP = 0;
	if (DebugON) cout << "RECEIVED ARP REQUEST, SENDING RESPONSE" << endl;
	for (auto it = routing_table.begin(); it != routing_table.end(); ++it)
	{	
		if (ipv4_2_str(e.data.srcip & it->mask) == ipv4_2_str(it->destsubnet))
		{
			gatewayIP = it->nexthop;		
			iface_out = it->ifacename;
			break;
		}
	}
	for (auto it = iface_list.begin(); it != iface_list.end(); ++it)
	{
		if (iface_out == it->ifacename)
		{
			host_mac = it->macaddr;
			stationIP = it->ipaddr;
			break;
		}
	}
	UpdateARPCache(e);
	IP_Pkt ipPkt(e.data.srcip, stationIP, gatewayIP, NOENTRY); //ipPkt(dst, src, gateway, msg);
	return Ethernet_Pkt(e.src, host_mac, ARP_RESPONSE, ipPkt, iface_out);
}

void Station::UpdateARPCache(const Ethernet_Pkt& e)
{
	bool found = false;
	for (auto it = arp_cache.begin(); it != arp_cache.end(); ++it)
	{
		if (it->ipaddr == e.data.srcip)
		{
			it->macaddr = e.src;
			found = true;
		}
	}
	if (!found)
	{
		arp_cache.push_back(ARP_Entry(e.data.srcip, e.src));	
	}
}


bool Station::ownsPacket(const Ethernet_Pkt& e)
{
	for (auto it = iface_list.begin(); it != iface_list.end(); ++it)
	{
		if (e.dst == it->macaddr || ipv4_2_str(e.data.dstip) == ipv4_2_str(it->ipaddr))
		{
			return true;
		}
	}
	return false;
}

Ethernet_Pkt Station::buildMessagePkt(string dest, string msg)
{
	string host, iface_out;
	MacAddr host_mac, peer_mac;
	IPAddr destIP = 0, gatewayIP = 0, stationIP = 0;
	for (auto it = host_list.begin(); it != host_list.end(); ++it)
	{
		if (it->name == dest)
		{
			destIP = it->addr;
			host = dest;
			break;
		}
	}
	if (host == NOENTRY) return Ethernet_Pkt(); // return bad host	
	if (DebugON) cout << "FOUND HOST : " << host << " (" << ipv4_2_str(destIP) << ")" << endl;
	for (auto it = routing_table.begin(); it != routing_table.end(); ++it)
	{	
		if (ipv4_2_str(destIP & it->mask) == ipv4_2_str(it->destsubnet))
		{
			gatewayIP = it->nexthop;
			iface_out = it->ifacename;
			break;
		}
	}
	if (ipv4_2_str(gatewayIP) == LOCALIP)
	{
		for (auto it = routing_table.begin(); it != routing_table.end(); ++it)
		{	
			if (LOCALIP == ipv4_2_str(it->destsubnet))
			{
				gatewayIP = it->nexthop;
				iface_out = it->ifacename;
				break;
			}
		}
		
	}
	if (DebugON) cout << "NEXT HOP: " << ipv4_2_str(gatewayIP) << " --- USING INTERFACE : " << iface_out << endl; 
	for (auto it = iface_list.begin(); it != iface_list.end(); ++it)
	{
		if (iface_out == it->ifacename)
		{
			host_mac = it->macaddr;
			stationIP = it->ipaddr;
			break;
		}
	}
	for (auto it = arp_cache.begin(); it != arp_cache.end(); ++it)
	{
		if (ipv4_2_str(gatewayIP) == ipv4_2_str(it->ipaddr))
		{
			peer_mac = it->macaddr;	
			break;
		}
	}
	if (DebugON) cout << "SRC MAC: " << host_mac << " --- " << ipv4_2_str(stationIP) << endl;
	IP_Pkt ipPkt(destIP, stationIP, gatewayIP, msg);
	if (peer_mac == NOENTRY) 
	{
		if (DebugON) cout << "DST MAC NOT FOUND!" << endl;
		bool isSent = false;
		for (auto it = arp_queue.begin(); it != arp_queue.end(); ++it)
		{
			if (gatewayIP == it->data.nexthop)
			{
				isSent = true;
				break;
			}
		}
		
		arp_queue.push_back(Ethernet_Pkt(NOENTRY, host_mac, IPFRAME, ipPkt, iface_out)); //push back awaiting arp response
		ipPkt.msg = NOENTRY; 															 //dont broadcast input
		if (isSent) return Ethernet_Pkt(NOENTRY, host_mac, ARP_QUEUED, ipPkt, iface_out); //if ARP is already pending just skip (dont duplicate ARP)
		if (DebugON) cout << "ARP SENT, QUEUING MESSAGE AWAITING RESPONSE!" << endl;
		return Ethernet_Pkt(NOENTRY, host_mac, ARP_REQUEST, ipPkt, iface_out); 
	}
	if (DebugON) cout << "DST MAC: " << peer_mac << endl << "... SENDING MESSAGE" << endl;
	return Ethernet_Pkt(peer_mac, host_mac, IPFRAME, ipPkt, iface_out);
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
	string dhr("================================================================================\n");
	string hr("--------------------------------------------------------------------------------\n");
	cout << dhr << "ARP CACHE\n" << dhr;
	cout << setw(INET_MACSTRLEN ) << left << "MAC ADDRESS" << setw(INET_ADDRSTRLEN) << left << "IP ADDRESS" << endl;
	cout << hr;
	for (auto it = arp_cache.begin(); it != arp_cache.end(); ++it)
	{
		cout << setw(INET_MACSTRLEN) << left << it->macaddr;
		cout << setw(INET_ADDRSTRLEN) << left << ipv4_2_str(it->ipaddr) << endl;
	}
	cout << dhr << "ARP MESSAGES\n" << dhr;
	cout << setw(INET_ADDRSTRLEN ) << left << "DST IP ADDRESS" << setw(60) << left << "MESSAGE" << endl;
	cout << hr;
	for (auto it = arp_queue.begin(); it != arp_queue.end(); ++it)
	{
		cout << setw(INET_ADDRSTRLEN) << left << ipv4_2_str(it->data.srcip);
		cout << setw(60) << left << it->data.msg << endl;
	}
	
	
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
	for (auto it = connected_ifaces.begin(); it != connected_ifaces.end(); ++it)
	{
		close(it->second);
	}
}
