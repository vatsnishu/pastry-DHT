#ifndef PASTRY_H_INCLUDED
#define PASTRY_H_INCLUDED
#include<string>
#include "RoutingManager.h"

using namespace std;

class PastryNode {
    string nodeID;
    int PORT;
    string PORT_str;
    string ip_str;
public:
    RoutingManager rm;
    PastryNode();
    void setPort(string);
    int getPort();
    static void *startServer(void *);
    int connectToServer(char* , int);
    static void *serveRequest(void *);
    void join(string ip, int port);
    string getIPStr();
    string getPortStr();
    string getNodeID();
    void put(string, string);
    void get(string);
};

string getip();
string get_node_ID(string, string);
string get_hash_key(string);
#endif
