/*
Course: CNT5505 - Data Networks and Communications
Semester: Spring 2016
Name: James Bach, Becky Powell
*/

#include "station.h"



Station::Station(bool isRouter, string ifacefile, string rtablefile, string hostfile)
{
	populateInterfaces(ifacefile);
	populateRouting(rtablefile);
	populateHosts(hostfile);
#if DEBUG
		printTables();	
#endif
	connectbridges();	
}


void Station::connectbridges()
{
	int fails = 0, success = 0;
	memset(buffer, 0, CONNECTIONRESPONSE);
	for (size_t i = 0; i < iface_list.size(); i++)
	{
		pair<string, string> info = readLinks(i);
		string host = info.first, port = info.second;
		sockaddr_in sa = getSockAddrInfo(htons(atoi(port.c_str())));
		if (inet_aton(host.c_str(), (in_addr *) &sa.sin_addr.s_addr))
		{
			if ((main_socket = socket(AF_INET, SOCK_STREAM, 0)) == FAILURE)
			{
				cerr << "Failed to create socket" << endl;
				exit(1);
			}
			if (connect(main_socket, (sockaddr *) &sa, sizeof(sa)) == FAILURE)
			{
				if (fails++ < MAXFAIL)
				{
					cerr << "Failed to Connect...Retrying..." << endl;				
					this_thread::sleep_for(chrono::seconds(TIMEOUT));
					i--;
				}
				else
				{
					cerr << "Connection to Interface Failed" << endl;
					exit(1);
				}
				continue;
			}
			if (!isConnectionAccepted()) cerr << "Connection Rejected from Bridge" << endl;
			success++;
		}		
	}
	if (!success) exit(1); 
}

bool Station::isConnectionAccepted() //non-blocking timeout
{
	int options, old_options, bytes_read;
	bool result = false;
	char buffer[CONNECTIONRESPONSE];
	if ((old_options = fcntl(main_socket, F_GETFL)) == FAILURE)
	{
		cerr << "Failed to get new socket connection options" << endl;
		return false;
	}
	options = old_options;
	options |= O_NONBLOCK;
	if (fcntl(main_socket, F_SETFL, options) == FAILURE)
	{
		cerr << "Failed to set new socket connection options" << endl;
		return false;
	}
	int fails = 0;
	while (fails < MAXFAIL)
	{
		this_thread::sleep_for(chrono::seconds(TIMEOUT));
		if ((bytes_read = read(main_socket, buffer, CONNECTIONRESPONSE)) > 0)
		{
			buffer[CONNECTIONRESPONSE - 1] = 0;
			result = (strcmp(buffer, "accept") == 0);
			DBGOUT("CONNECTION RESPONSE:" << buffer);
			break;
		}
		cerr << "Failed to Read...Retrying..." << endl;				
		fails++;
	}
	if (fcntl(main_socket, F_SETFL, old_options) == FAILURE)
	{
		cerr << "Failed to reset socket connection options" << endl;
		exit(1);
	}
	return result;
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
		string command; 
		cin >> command;
		if (command == "send")
		{
			string destination, user_msg;
			cin >> destination;
			getline(cin, user_msg);
			DBGOUT(destination << " " << user_msg);
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
				
			}
			
		}
		else
		{
			cerr << command << ": command not recognized" << endl;
		}
	
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
pair<string,string> Station::readLinks(int index) const
{
	char str[INET6_ADDRSTRLEN];
	memset(str,0,INET6_ADDRSTRLEN);
	string aFile = "." + iface_list[index].lanname + ".addr";
	string pFile = "." + iface_list[index].lanname + ".port"; 
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

void Station::printTables() const
{
	char str[INET6_ADDRSTRLEN];	
	memset(str, 0, INET6_ADDRSTRLEN);
	string dhr("================================================================================\n");
	string hr("--------------------------------------------------------------------------------\n");
	cout << dhr << "HOSTS\n" << dhr;
	cout << setw(INET_ADDRSTRLEN ) << left << "STATION NAME" << setw(INET_ADDRSTRLEN ) << left << "STATION IP" << endl;
	cout << hr;
	for (size_t i = 0; i < host_list.size(); i++)
	{
		inet_ntop(AF_INET, &host_list[i].addr, str, INET6_ADDRSTRLEN);
		cout << setw(INET_ADDRSTRLEN) << left << host_list[i].name << setw(INET_ADDRSTRLEN ) << left << str << endl;
		memset(str, 0, INET6_ADDRSTRLEN);
	}
	cout << dhr << "ROUTING TABLE\n" << dhr;
	cout << setw(INET_ADDRSTRLEN) << left << "DESTINATION IP" << setw(INET_ADDRSTRLEN) << left << "NEXT-HOP IP";
	cout << setw(INET_ADDRSTRLEN) << left << "SUBNET MASK" << setw(INET_ADDRSTRLEN) << left << "INTERFACE NAME" << endl;
	cout << hr;
	for (size_t i = 0; i < routing_table.size(); i++)
	{
		inet_ntop(AF_INET, &routing_table[i].destsubnet, str, INET6_ADDRSTRLEN);
		cout << setw(INET_ADDRSTRLEN) << left << str;
		memset(str, 0, INET6_ADDRSTRLEN);
		inet_ntop(AF_INET, &routing_table[i].nexthop, str, INET6_ADDRSTRLEN);
		cout << setw(INET_ADDRSTRLEN) << left << str;
		memset(str, 0, INET6_ADDRSTRLEN);
		inet_ntop(AF_INET, &routing_table[i].mask, str, INET6_ADDRSTRLEN);
		cout << setw(INET_ADDRSTRLEN) << left << str;
		cout << setw(INET_ADDRSTRLEN) << left << routing_table[i].ifacename << endl;
		memset(str, 0, INET6_ADDRSTRLEN);
	}
	cout << dhr << "INTERFACES\n" << dhr;
	cout << setw(INET_ADDRSTRLEN) << left << "STATION NAME" << setw(INET_ADDRSTRLEN) << left << "STATION IP";
	cout << setw(INET_ADDRSTRLEN) << left << "SUBNET MASK" << setw(INET_MACSTRLEN) << left << "MAC ADDRESS";
	cout << setw(INET_ADDRSTRLEN) << left << "LANNAME" << endl;
	cout << hr;
	for (size_t i = 0; i < iface_list.size(); i++)
	{
		inet_ntop(AF_INET, &iface_list[i].ipaddr, str, INET6_ADDRSTRLEN);
		cout << setw(INET_ADDRSTRLEN) << left << iface_list[i].ifacename;
		cout << setw(INET_ADDRSTRLEN) << left << str;
		memset(str, 0, INET6_ADDRSTRLEN);
		inet_ntop(AF_INET, &iface_list[i].mask, str, INET6_ADDRSTRLEN);
		cout << setw(INET_ADDRSTRLEN) << left << str;
		cout << setw(INET_MACSTRLEN) << left << iface_list[i].macaddr;
		cout << setw(INET_ADDRSTRLEN) << left << iface_list[i].lanname << endl;
		memset(str, 0, INET6_ADDRSTRLEN);
	}
	cout << dhr;
}

Station::~Station()
{
	close(main_socket);
}
