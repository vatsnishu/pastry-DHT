#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <cstdio>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string>
#include <map>
#include <vector>
#include <ctime>
#include <fstream>
#include <arpa/inet.h>
#include <pthread.h>
#include "RoutingManager.h"

using namespace std;

#define BUF_SIZE 256
#define MAX_CLIENTS 10

pthread_mutex_t lock;

RoutingManager::RoutingManager () {
    nodeid="";
    ip="";
    port="";
    int i,j;
    pthread_mutex_lock(&lock);
    for (i=0;i<4;i++) {
        for(j=0;j<4;j++) {
            RoutingTable[i][j].nodeID.assign("");
            RoutingTable[i][j].ip.assign("");
            RoutingTable[i][j].port.assign("");
        }
    }
    pthread_mutex_unlock(&lock);
    for(i=0;i<5;i++) {
        LeafSet[i].nodeID.assign("");
        LeafSet[i].ip.assign("");
        LeafSet[i].port.assign("");
    }
}

void RoutingManager::setValues(string id, string tmp_ip, string tmp_port) {
    nodeid=id;
    ip=tmp_ip;
    port=tmp_port;
    int i,pre,j;
    j=0;
    pre=id[j]-'0';
    pthread_mutex_lock(&lock);
    for (i=0;i<4;i++) {
        RoutingTable[i][pre].nodeID.assign(nodeid);
        RoutingTable[i][pre].ip.assign(ip);
        RoutingTable[i][pre].port.assign(port);
        j++;
        pre=id[j]-'0';
    }
    pthread_mutex_unlock(&lock);
    LeafSet[2].nodeID.assign(nodeid);
    LeafSet[2].ip.assign(ip);
    LeafSet[2].port.assign(port);
}

string RoutingManager::routeNode(string tmp_nodeID, string tmp_ip, string tmp_port) {
    int flag;
    flag=updateLeafSet(tmp_nodeID, tmp_ip, tmp_port);
    string res;
    res.assign(nodeid+"#"+ip+"#"+port);
    res.append(updateRT(tmp_nodeID, tmp_ip, tmp_port));
    return res;
}

cell RoutingManager::getLS(int i) {
    if(!LeafSet[i].nodeID.empty()) {
        return LeafSet[i];
    }
    cell tmp;
    tmp.nodeID="";
    tmp.ip="";
    tmp.port="";
    return tmp;
}

int RoutingManager::updateLeafSet(string tmp_nodeID, string tmp_ip, string tmp_port) {

    if(tmp_nodeID.compare(nodeid)>0) {
        //cout<<tmp_nodeID<<nodeid<<"greater set1"<<endl;
        if(LeafSet[3].nodeID.empty()) {
            LeafSet[3].nodeID.assign(tmp_nodeID);
            LeafSet[3].ip.assign(tmp_ip);
            LeafSet[3].port.assign(tmp_port);
        } else if(LeafSet[4].nodeID.empty()) {
            if(LeafSet[3].nodeID.compare(tmp_nodeID)>0) {
                LeafSet[4].nodeID.assign(LeafSet[3].nodeID);
                LeafSet[4].ip.assign(LeafSet[3].ip);
                LeafSet[4].port.assign(LeafSet[3].port);
                LeafSet[3].nodeID.assign(tmp_nodeID);
                LeafSet[3].ip.assign(tmp_ip);
                LeafSet[3].port.assign(tmp_port);
            } else {
                LeafSet[4].nodeID.assign(tmp_nodeID);
                LeafSet[4].ip.assign(tmp_ip);
                LeafSet[4].port.assign(tmp_port);
            }
        } else {
            if(LeafSet[3].nodeID.compare(tmp_nodeID)>0) {
                updateRTafterLSModification(LeafSet[4].nodeID, LeafSet[4].ip, LeafSet[4].port);
                LeafSet[4].nodeID.assign(LeafSet[3].nodeID);
                LeafSet[4].ip.assign(LeafSet[3].ip);
                LeafSet[4].port.assign(LeafSet[3].port);
                LeafSet[3].nodeID.assign(tmp_nodeID);
                LeafSet[3].ip.assign(tmp_ip);
                LeafSet[3].port.assign(tmp_port);
            } else if(LeafSet[4].nodeID.compare(tmp_nodeID)>0) {
                updateRTafterLSModification(LeafSet[4].nodeID, LeafSet[4].ip, LeafSet[4].port);
                LeafSet[4].nodeID.assign(tmp_nodeID);
                LeafSet[4].ip.assign(tmp_ip);
                LeafSet[4].port.assign(tmp_port);
            } else {
                return 0;
            }
        }
    } else if(tmp_nodeID.compare(nodeid)<0) {
        //cout<<tmp_nodeID<<nodeid<<"lesser set"<<endl;
        if(LeafSet[1].nodeID.empty()) {
            LeafSet[1].nodeID.assign(tmp_nodeID);
            LeafSet[1].ip.assign(tmp_ip);
            LeafSet[1].port.assign(tmp_port);
        } else if(LeafSet[0].nodeID.empty()) {
            if(LeafSet[1].nodeID.compare(tmp_nodeID)<0) {
                LeafSet[0].nodeID.assign(LeafSet[1].nodeID);
                LeafSet[0].ip.assign(LeafSet[1].ip);
                LeafSet[0].port.assign(LeafSet[1].port);
                LeafSet[1].nodeID.assign(tmp_nodeID);
                LeafSet[1].ip.assign(tmp_ip);
                LeafSet[1].port.assign(tmp_port);
            } else {
                LeafSet[0].nodeID.assign(tmp_nodeID);
                LeafSet[0].ip.assign(tmp_ip);
                LeafSet[0].port.assign(tmp_port);
            }
        } else {
            if(LeafSet[1].nodeID.compare(tmp_nodeID)<0) {
                updateRTafterLSModification(LeafSet[0].nodeID, LeafSet[0].ip, LeafSet[0].port);
                LeafSet[0].nodeID.assign(LeafSet[1].nodeID);
                LeafSet[0].ip.assign(LeafSet[1].ip);
                LeafSet[0].port.assign(LeafSet[1].port);
                LeafSet[1].nodeID.assign(tmp_nodeID);
                LeafSet[1].ip.assign(tmp_ip);
                LeafSet[1].port.assign(tmp_port);
            } else if(LeafSet[0].nodeID.compare(tmp_nodeID)<0) {
                updateRTafterLSModification(LeafSet[0].nodeID, LeafSet[0].ip, LeafSet[0].port);
                LeafSet[0].nodeID.assign(tmp_nodeID);
                LeafSet[0].ip.assign(tmp_ip);
                LeafSet[0].port.assign(tmp_port);
            } else {
                return 0;
            }
        }
    }
    updateRTafterLSModification(tmp_nodeID, tmp_ip, tmp_port);
    return 1;
}

void RoutingManager::updateRTafterLSModification(string tmp_nodeID, string tmp_ip, string tmp_port) {
    int pre,i;
    pre=0;
    char recvdNode_chararr[tmp_nodeID.size()+1];
    copy(tmp_nodeID.begin(), tmp_nodeID.end(), recvdNode_chararr);
    recvdNode_chararr[tmp_nodeID.size()]='\0';

    char node_chararr[nodeid.size()+1];
    copy(nodeid.begin(), nodeid.end(), node_chararr);
    node_chararr[nodeid.size()]='\0';
    while(pre<4 && node_chararr[pre]==recvdNode_chararr[pre]) {
        pre++;
    }
    if(pre<4) {
        pthread_mutex_lock(&lock);
        int next_digit_recvd=recvdNode_chararr[pre]-'0';
        if(RoutingTable[pre][next_digit_recvd].nodeID.empty()) {
            RoutingTable[pre][next_digit_recvd].nodeID.assign(tmp_nodeID);
            RoutingTable[pre][next_digit_recvd].ip.assign(tmp_ip);
            RoutingTable[pre][next_digit_recvd].port.assign(tmp_port);
        } else if(!inLSRange(tmp_nodeID)) {
            RoutingTable[pre][next_digit_recvd].nodeID.assign(tmp_nodeID);
            RoutingTable[pre][next_digit_recvd].ip.assign(tmp_ip);
            RoutingTable[pre][next_digit_recvd].port.assign(tmp_port);
        }
        pthread_mutex_unlock(&lock);
    }
}

string RoutingManager::updateRT(string tmp_nodeID, string tmp_ip, string tmp_port) {
    int pre,i;
    string ret_str;
    pre=0;
    char recvdNode_chararr[tmp_nodeID.size()+1];
    copy(tmp_nodeID.begin(), tmp_nodeID.end(), recvdNode_chararr);
    recvdNode_chararr[tmp_nodeID.size()]='\0';

    char node_chararr[nodeid.size()+1];
    copy(nodeid.begin(), nodeid.end(), node_chararr);
    node_chararr[nodeid.size()]='\0';
    while(pre<4 && node_chararr[pre]==recvdNode_chararr[pre]) {
        pre++;
    }
    char digit[2];
    digit[0]=pre+'0';
    digit[1]='\0';
    ret_str.assign("#"+string(digit));
    if(pre<4) {
        pthread_mutex_lock(&lock);
        int next_digit_recvd=recvdNode_chararr[pre]-'0';
        int next_digit_node = node_chararr[pre] - '0';
        if(RoutingTable[pre][next_digit_recvd].nodeID.empty()) {
            RoutingTable[pre][next_digit_recvd].nodeID.assign(tmp_nodeID);
            RoutingTable[pre][next_digit_recvd].ip.assign(tmp_ip);
            RoutingTable[pre][next_digit_recvd].port.assign(tmp_port);
        } else if(!inLSRange(tmp_nodeID)) {
            RoutingTable[pre][next_digit_recvd].nodeID.assign(tmp_nodeID);
            RoutingTable[pre][next_digit_recvd].ip.assign(tmp_ip);
            RoutingTable[pre][next_digit_recvd].port.assign(tmp_port);
        }
        for(i=0;i<4;i++) {
            if(i==next_digit_recvd) {
                ret_str.append("# # # ");
            } else if(i==next_digit_node) {
                ret_str.append("#"+nodeid+"#"+ip+"#"+port);
            } else {
                if(RoutingTable[pre][i].nodeID.empty()) {
                    ret_str.append("# # # ");
                } else {
                    ret_str.append("#"+RoutingTable[pre][i].nodeID+"#"+RoutingTable[pre][i].ip+"#"+RoutingTable[pre][i].port);
                }
            }
        }
        pthread_mutex_unlock(&lock);

    }
    return ret_str;
}

void RoutingManager::addRowToRT(string recv_str) {
    char recv_arr[recv_str.size()+1];
    copy(recv_str.begin(), recv_str.end(), recv_arr);
    recv_arr[recv_str.size()]='\0';

    int pre,i;

    char * token = strtok(recv_arr, "#");
    string tmp_id, tmp_ip, tmp_port;
    tmp_id.assign(string(token));
    token = strtok(NULL,"#");
    tmp_ip.assign(string(token));
    token = strtok(NULL,"#");
    tmp_port.assign(string(token));
    token = strtok(NULL,"#");
    pre=atoi(token);
    if(pre!=-1) {
        i=0;
        token = strtok(NULL,"#");
        pthread_mutex_lock(&lock);
        while(i<4 && token!=NULL) {
            if(strcmp(token," ")==0) {
                token = strtok(NULL,"#");
                token = strtok(NULL,"#");
                token = strtok(NULL,"#");
            } else {
                RoutingTable[pre][i].nodeID.assign(string(token));
                token = strtok(NULL,"#");
                RoutingTable[pre][i].ip.assign(string(token));
                token = strtok(NULL,"#");
                RoutingTable[pre][i].port.assign(string(token));
                token = strtok(NULL,"#");
            }
            i++;
        }
        pthread_mutex_unlock(&lock);
    }
    updateLeafSet(tmp_id, tmp_ip, tmp_port);
}

void RoutingManager::printRT() {
    int i,j;
    cout<<"\nRouting Table:\n"<<endl;
    cout<<"-0\t1\t2\t3\t-"<<endl;
    for(i=0;i<4;i++) {
        cout<<"-";
        for(j=0;j<4;j++) {
            cout<<RoutingTable[i][j].nodeID<<"\t";
        }
        cout<<"-"<<endl;
    }
}

void RoutingManager::printLT() {
    int i;
    cout<<"\nLeafSet:\n";
    for(i=0;i<5;i++) {
        cout<<i<<"-->"<<LeafSet[i].nodeID<<endl;
    }
}

bool RoutingManager::inLSRange(string tmp_nodeID) {
    if(tmp_nodeID.compare(nodeid)>0) {
        //cout<<tmp_nodeID<<nodeid<<"greater set2"<<endl;
        if(LeafSet[3].nodeID.empty() || LeafSet[4].nodeID.empty())
            return true;
        else if(LeafSet[4].nodeID.compare(tmp_nodeID)>0) {
            return true;
        } else
            return false;
    } else if(tmp_nodeID.compare(nodeid)<0) {
        if(LeafSet[1].nodeID.empty() || LeafSet[0].nodeID.empty())
            return true;
        else if(LeafSet[0].nodeID.compare(tmp_nodeID)<0)
            return true;
        else
            return false;
    }
    return false;
}

bool RoutingManager::inLSBounds(string tmp_key) {
    string max, min;
    if(!LeafSet[4].nodeID.empty()) {
        max.assign(LeafSet[4].nodeID);
    } else if(!LeafSet[3].nodeID.empty()) {
        max.assign(LeafSet[3].nodeID);
    } else {
        max.assign(LeafSet[2].nodeID);
    }

    if(!LeafSet[0].nodeID.empty()) {
        min.assign(LeafSet[0].nodeID);
    } else if(!LeafSet[1].nodeID.empty()) {
        min.assign(LeafSet[1].nodeID);
    } else {
        min.assign(LeafSet[2].nodeID);
    }

    if(tmp_key.compare(max)>=0) {
        return false;
    }
    if(tmp_key.compare(min)<=0) {
        return false;
    }
    return true;
}

cell RoutingManager::routeKey(string tmp_key) {
    int pre,i;
    cell tmp;
    tmp.nodeID.assign("");
    tmp.ip.assign("");
    tmp.port.assign("");

    if(nodeid.compare(tmp_key)==0) {
        return tmp;
    }

    int min=9999;

    if(inLSBounds(tmp_key)) {
        for(i=0;i<5;i++) {
            if(!LeafSet[i].nodeID.empty() && absdiff(LeafSet[i].nodeID, tmp_key)<min) {
                tmp.nodeID.assign(LeafSet[i].nodeID);
                tmp.ip.assign(LeafSet[i].ip);
                tmp.port.assign(LeafSet[i].port);
                min=absdiff(LeafSet[i].nodeID, tmp_key);
            }
        }
        if(LeafSet[2].nodeID.compare(tmp.nodeID)==0) {
            tmp.nodeID.assign("");
            tmp.ip.assign("");
            tmp.port.assign("");
        }
        return tmp;
    } else {
        pre=0;

        char key_chararr[tmp_key.size()+1];
        copy(tmp_key.begin(), tmp_key.end(), key_chararr);
        key_chararr[tmp_key.size()]='\0';

        char node_chararr[nodeid.size()+1];
        copy(nodeid.begin(), nodeid.end(), node_chararr);
        node_chararr[nodeid.size()]='\0';

        while(pre<4 && node_chararr[pre]==key_chararr[pre]) {
            pre++;
        }
        if(pre<4) {
            pthread_mutex_lock(&lock);
            int next_digit_recvd= key_chararr[pre]-'0';
            //cout<<"debug"<<pre<<" "<<next_digit_recvd<<endl; 
            if(!RoutingTable[pre][next_digit_recvd].nodeID.empty()) {
                tmp.nodeID.assign(RoutingTable[pre][next_digit_recvd].nodeID);
                tmp.ip.assign(RoutingTable[pre][next_digit_recvd].ip);
                tmp.port.assign(RoutingTable[pre][next_digit_recvd].port);
            }
            pthread_mutex_unlock(&lock);
        }
    }
    return tmp;
}

void RoutingManager::insertKey(string tmp_key, string tmp_val) {
    keys.insert ( pair<string, string>(tmp_key, tmp_val) );
    //cout<<"inserted"<<keys[tmp_key]<<" in node "<<nodeid<<endl;
}

string RoutingManager::getKeyValue(string tmp_key) {
    if(keys.find(tmp_key)==keys.end()) {
        return string("");
    } else {
        return keys[tmp_key];
    }
}

int RoutingManager::absdiff(string a, string b) {
    char a_chararr[a.size()+1];
    copy(a.begin(), a.end(), a_chararr);
    a_chararr[a.size()]='\0';

    char b_chararr[b.size()+1];
    copy(b.begin(), b.end(), b_chararr);
    b_chararr[b.size()]='\0';

    int aa,bb;
    aa=atoi(a_chararr);
    bb=atoi(b_chararr);

    if(aa>bb)
        return aa-bb;
    else
        return bb-aa;
}
