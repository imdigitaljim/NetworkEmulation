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
	connectbridges();	
}


void Station::connectbridges()
{
	int fails = 0;
	char buffer[CONNECTIONRESPONSE];
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
				if (timeout(fails))	i--;
				continue;
			}
			read(main_socket, buffer, CONNECTIONRESPONSE); //potentially non-blocking implementation needed?
			buffer[CONNECTIONRESPONSE - 1] = 0;
			if (strcmp(buffer, "reject") == 0) 
			{
				if (timeout(fails)) i--;
				continue;
			}
		}		
	}
}

bool Station::timeout(int& count) const
{
	count++;
	if (count < MAXFAIL)
	{
		cerr << "Failed to Connect...Retrying..." << endl;				
		this_thread::sleep_for(chrono::seconds(TIMEOUT));
		return true;
	}
	cerr << "Failed to Connect" << endl;
	return false;
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
pair<string,string> Station::readLinks(int index) const
{
	char str[INET6_ADDRSTRLEN];
	memset(str,0,INET6_ADDRSTRLEN);
	string aFile = "." + iface_list[index].lanname + ".addr";
	string pFile = "." + iface_list[index].lanname + ".port"; 
	cout << aFile << " " << pFile << endl;
	readlink(aFile.c_str(), str, INET6_ADDRSTRLEN);
	string host(str);
	memset(str,0,INET6_ADDRSTRLEN);
	readlink(pFile.c_str(), str, 5);
	string port(str);
	cout << "host: " << host << " :port: " << port << endl;
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

Station::~Station()
{
	close(main_socket);
}
