/*
Course: CNT5505 - Data Networks and Communications
Semester: Spring 2016
Name: James Bach, Becky Powell
*/


#ifndef _PCKET_H
#define _PCKET_H

#include <string>
#include <arpa/inet.h>
#include <vector>

#define VB "|"
#define PKTFIELDS 8

using namespace std;

typedef unsigned long PREAMBLE;
typedef unsigned long IPAddr;
typedef string MacAddr;

enum FRAMETYPE {IPFRAME, ARP_REQUEST, ARP_RESPONSE , ARP_QUEUED, NOFRAME};

class INET
{
	public:
		string ipv4_2_str(IPAddr x) const
		{
			char str[INET6_ADDRSTRLEN];	
			memset(str, 0, INET6_ADDRSTRLEN);
			inet_ntop(AF_INET, &x, str, INET6_ADDRSTRLEN);
			return string(str);
		}

		IPAddr str_2_ipv4(string x) const
		{
			IPAddr result;
			inet_pton(AF_INET, x.c_str(), &result);
			return result;
		}
};

class IP_Pkt: public INET // IP LAYER
{
	public:
		IP_Pkt() : dstip(0), srcip(0), nexthop(0), msg(""){}
		IP_Pkt(IPAddr d, IPAddr s, IPAddr nh, string data = ""): dstip(d), srcip(s), nexthop(nh), msg(data) {}
		IP_Pkt(string d, string s, string nh, string data) : msg(data)
		{
			dstip = str_2_ipv4(d);
			srcip = str_2_ipv4(s);
			nexthop = str_2_ipv4(nh);
		}
		string serialize() const
		{
			return string("|IP|" + ipv4_2_str(dstip) + VB + ipv4_2_str(srcip) + VB + ipv4_2_str(nexthop) + VB + msg);
		}
		IPAddr dstip;
		IPAddr srcip;
		IPAddr nexthop;
		string msg;

};

class Ethernet_Pkt : public INET //MAC LAYER
{
	public:	
		Ethernet_Pkt() : dst(""), src(""), type(NOFRAME), data(IP_Pkt()), iface_out(""){}
		Ethernet_Pkt(string s) : data()
		{
			try
			{
				deserialize(s);	
			}
			catch (const exception& e)
			{
				*this = Ethernet_Pkt();
			}
		}
		Ethernet_Pkt(MacAddr d, MacAddr s, FRAMETYPE t, IP_Pkt pkt, string ifc) 
		: dst(d), src(s), type(t), data(pkt), iface_out(ifc)  {} 
		string serialize() const
		{
			return string("|MAC|" + to_string(static_cast<int>(type)) + VB + dst + VB + src + data.serialize());
		}
		void deserialize(string pkt) 
		{
			vector<string> tokens;
			pkt.erase(0, 1);
			while (tokens.size() < PKTFIELDS)
			{
				size_t pos = pkt.find(VB);
				string t = pkt.substr(0, pos);
				tokens.push_back(t);
				pkt.erase(0, pos + 1);
			}
			type = static_cast<FRAMETYPE>(stoi(tokens[1]));
			dst = tokens[2];
			src = tokens[3];
			data = IP_Pkt(tokens[5], tokens[6], tokens[7], pkt);
		}
		MacAddr dst; /* destination address in net order */
		MacAddr src; /* source address in net order */
		FRAMETYPE type; /* enum FRAMETYPE {IPFRAME, ARP_REQUEST, ARP_RESPONSE , NOFRAME}; */
		IP_Pkt data;
		string iface_out;
};

#endif