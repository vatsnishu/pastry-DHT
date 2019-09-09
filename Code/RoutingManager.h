#ifndef ROUTINGMANAGER_H_INCLUDED
#define ROUTINGMANAGER_H_INCLUDED
#include<string>
#include<map>
#include<pthread.h>

using namespace std;

struct cell {
    string nodeID;
    string ip;
    string port;
};

class RoutingManager {
    string nodeid; 
    string port;
    string ip;
    cell RoutingTable [4][4];
    cell LeafSet[5];
    map <string, string> keys;
public:
    RoutingManager();
    void setValues(string , string, string);
    string routeNode(string, string, string);
    string updateRT(string, string, string);
    int updateLeafSet(string, string, string);
    void updateRTafterLSModification(string, string, string);
    void addRowToRT(string);
    void printRT();
    void printLT();
    cell getLS(int);
    bool inLSRange(string);
    cell routeKey(string);
    void insertKey(string, string);
    string getKeyValue(string);
    int absdiff(string, string);
    bool inLSBounds(string);
};
extern pthread_mutex_t lock;
#endif
