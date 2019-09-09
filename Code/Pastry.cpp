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
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "md5.h"
#include "RoutingManager.h"
#include "Pastry.h"

using namespace std;

#define BUF_SIZE 256
#define MAX_CLIENTS 10
#define MAX_DATA 50

PastryNode pNode;

PastryNode::PastryNode () {
    PORT=0;
    PORT_str="";
    ip_str="";
    nodeID="";
}

int main(int argc,char **argv)
{       
    string cmd;
    pthread_t thread_id=0;
    while(1) {
        cout<<"pastry $ ";
        cin>>cmd;
        if(cmd.compare("port")==0) {
            string tmp_port;
            cin>>tmp_port;
            pNode.setPort(tmp_port);
        } else if(cmd.compare("create")==0) {
            int tmp_port = pNode.getPort();
            int *port_pt;
            port_pt=&tmp_port;
            pthread_create(&thread_id, NULL, PastryNode::startServer, (void *)port_pt);
        } else if(cmd.compare("join")==0) {
            int port;
            string ip;
            cin>>ip>>port;
            pNode.join(ip, port);
        } else if(cmd.compare("put")==0) {
            string key, val;
            cin>>key>>val;
            pNode.put(key, val);
        } else if(cmd.compare("get")==0) {
            string key;
            cin>>key;
            pNode.get(key);
        } else if(cmd.compare("lset")==0) {
            pNode.rm.printLT();
        } else if(cmd.compare("dump")==0) {
            pNode.rm.printRT();
        } else if(cmd.compare("exit")==0) {
            cout<<"Bye!!"<<endl;
            break;
        } else {
            cout<<"Wrong command"<<endl;
        }
    }
    return 0;
}

void PastryNode::setPort(string tmp_port) {
    PORT_str=tmp_port;

    char tmp_arr[tmp_port.size()+1];
    copy(tmp_port.begin(), tmp_port.end(), tmp_arr);
    tmp_arr[tmp_port.size()]='\0';
    PORT=atoi(tmp_arr);

    ip_str=getip();
    nodeID=get_node_ID(ip_str, PORT_str);
    rm.setValues(nodeID, ip_str, PORT_str);
}

int PastryNode::getPort() {
    return PORT;
}

int PastryNode::connectToServer(char* argv1, int argv2) {
    int sock_fd;
    struct sockaddr_in servaddr;
    struct hostent *server;

    sock_fd=socket(AF_INET,SOCK_STREAM,0);
    if(sock_fd < 0) {
        cout<<"Error: "<<strerror(errno)<<endl;
    }

    bzero((char *)&servaddr,sizeof(servaddr));

    servaddr.sin_family=AF_INET;
    servaddr.sin_port=htons(argv2);

    server = gethostbyname(argv1);
    bcopy((char *) server->h_addr, (char *)&servaddr.sin_addr.s_addr, server->h_length);

    connect(sock_fd,(struct sockaddr *)&servaddr,sizeof(servaddr));

    return sock_fd;
}

void PastryNode::join(string ip, int port) {
    int sock_fd;
    string str, sendline;
    char recvline[BUF_SIZE];
    ssize_t ret_in;

    char ip_chararr[ip.size()+1];
    copy(ip.begin(), ip.end(), ip_chararr);
    ip_chararr[ip.size()] = '\0';
    sock_fd = connectToServer(ip_chararr, port);

    sendline.clear();
    sendline.append("SET#");
    sendline.append(pNode.getNodeID()+"#"+pNode.getIPStr()+"#"+pNode.getPortStr());
    send(sock_fd, sendline.c_str(), sendline.length() + 1, 0);
    bzero(recvline, BUF_SIZE);
    while((ret_in = recv(sock_fd, recvline, BUF_SIZE-1, 0)) > 0) {
        recvline[ret_in] = '\0';
        pNode.rm.addRowToRT(string(recvline));
        //cout<<recvline<<endl;
    }
    //pNode.rm.printRT();
    close(sock_fd);
}

void PastryNode::put(string key, string val) {
    int sock_fd;
    string sendline;

    char ip_chararr[ip_str.size()+1];
    copy(ip_str.begin(), ip_str.end(), ip_chararr);
    ip_chararr[ip_str.size()] = '\0';
    sock_fd = connectToServer(ip_chararr, PORT);

    sendline.clear();
    sendline.append("PUT#"+key+"#"+val);
    send(sock_fd, sendline.c_str(), sendline.length() + 1, 0);
    close(sock_fd);
}

void PastryNode::get(string key) {
    int sock_fd;
    string sendline;
    char recvline[BUF_SIZE];
    ssize_t ret_in;

    char ip_chararr[ip_str.size()+1];
    copy(ip_str.begin(), ip_str.end(), ip_chararr);
    ip_chararr[ip_str.size()] = '\0';
    sock_fd = connectToServer(ip_chararr, PORT);

    sendline.clear();
    sendline.append("GET#"+key);
    send(sock_fd, sendline.c_str(), sendline.length() + 1, 0);

    bzero(recvline, BUF_SIZE);
    while((ret_in = recv(sock_fd, recvline, BUF_SIZE-1, 0)) > 0) {
        recvline[ret_in] = '\0';
        cout<<recvline<<endl;
    }
    close(sock_fd);
}

void *PastryNode::startServer(void *lp) {
    int listen_fd;
    int *conn_fd;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;
    pthread_t threads[MAX_CLIENTS];
    int thread_no = 0;
    int port = *((int *)lp);
    string ip_str=pNode.getIPStr();
    string nodeID=pNode.getNodeID();
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    bzero(&serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htons(INADDR_ANY);
    serv_addr.sin_port = htons(port);
    bind(listen_fd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));

    listen(listen_fd, MAX_CLIENTS);
    //cout<<"\nStarted server"<<listen_fd<<" on port"<<port<<" with ip "<<ip_str<<" with nodeID "<<nodeID<<endl;
    clilen = sizeof(cli_addr);
    while(thread_no<MAX_CLIENTS) {
        conn_fd = (int*)malloc(sizeof(int));
        *conn_fd = accept(listen_fd, (struct sockaddr*) &cli_addr, &clilen);

        if (*conn_fd < 0) {
            perror("Error:");
        } else {
            pthread_create(&threads[thread_no], NULL, PastryNode::serveRequest, (void*)conn_fd);
            thread_no++;
        }
    }
    free(conn_fd);
    for(int i = 0; i < MAX_CLIENTS; i++) {
        pthread_join(threads[i], NULL);
    }
    pthread_exit(NULL);
}

void *PastryNode::serveRequest(void *lp) {
    int *fd = (int *)lp;
    int conn_fd = *fd;

    char recvline[BUF_SIZE];
    string receiveline_str, request_res;
    size_t pos;

    bzero(recvline, BUF_SIZE);
    recv(conn_fd, recvline, BUF_SIZE-1, 0);
    receiveline_str.assign(recvline);
    request_res.clear();

    if((pos = receiveline_str.find("SET#")) != string::npos) {
        //for join command received mesg format-> SET#<nodeID>#<ip>#<port>
        receiveline_str.replace(0, 4, "");
        receiveline_str.erase(receiveline_str.find_last_not_of(" \n\r\t")+1);

        request_res.assign("");

        char tmp_recvline[receiveline_str.size()+1];
        copy(receiveline_str.begin(), receiveline_str.end(), tmp_recvline);
        tmp_recvline[receiveline_str.size()]='\0';

        char * token = strtok(tmp_recvline, "#");
        string tmp_nodeID, tmp_ip, tmp_port;
        tmp_nodeID= string(token);
        token = strtok(NULL,"#");
        tmp_ip=string(token);
        token = strtok(NULL,"#");
        tmp_port=string(token);

        int sock_fd,sock_fd2,i;
        string sendline, sendline2;
        if(pNode.rm.inLSRange(tmp_nodeID)) {
            //inserted
            for(i=0;i<5;i++) {
                if(i!=2) {
                    cell tmp;
                    tmp=pNode.rm.getLS(i);
                    if(!tmp.nodeID.empty()) {
                        //char recvline[BUF_SIZE];
                        //ssize_t ret_in;

                        char ip_chararr[tmp.ip.size()+1];
                        copy(tmp.ip.begin(), tmp.ip.end(), ip_chararr);
                        ip_chararr[tmp.ip.size()] = '\0';

                        char port_int[tmp.port.size()+1];
                        copy(tmp.port.begin(), tmp.port.end(), port_int);
                        port_int[tmp.port.size()]='\0';

                        char ip_chararr2[tmp_ip.size()+1];
                        copy(tmp_ip.begin(), tmp_ip.end(), ip_chararr2);
                        ip_chararr2[tmp_ip.size()] = '\0';

                        char port_int2[tmp_port.size()+1];
                        copy(tmp_port.begin(), tmp_port.end(), port_int2);
                        port_int2[tmp_port.size()]='\0';

                        int port_no = atoi(port_int);
                        int port_no2 = atoi(port_int2);

                        sock_fd = pNode.connectToServer(ip_chararr, port_no);
                        sock_fd2 = pNode.connectToServer(ip_chararr2, port_no2);
                        sendline.clear();
                        sendline2.clear();
                        sendline.append("LFS#");
                        sendline2.append("LFS#");
                        sendline.append(tmp_nodeID+"#"+tmp_ip+"#"+tmp_port);
                        sendline2.append(tmp.nodeID+"#"+tmp.ip+"#"+tmp.port);
                        send(sock_fd, sendline.c_str(), sendline.length() + 1, 0);
                        send(sock_fd2, sendline2.c_str(), sendline2.length() + 1, 0);
                        close(sock_fd);
                        close(sock_fd2);
                    }
                }
            }
        } 
        request_res.append(pNode.rm.routeNode(tmp_nodeID, tmp_ip, tmp_port));

        //cout<<"Sending..."<<request_res<<endl;
        send(conn_fd, request_res.c_str(), request_res.length()+1, 0);
        //pNode.rm.printRT();
        close(conn_fd);
    } if((pos = receiveline_str.find("LFS#")) != string::npos) {
        //for updating leafset mesg format-> LFS#<nodeID>#<ip>#<port>
        //cout<<"received..."<<receiveline_str<<endl;
        receiveline_str.replace(0, 4, "");
        receiveline_str.erase(receiveline_str.find_last_not_of(" \n\r\t")+1);

        char tmp_recvline[receiveline_str.size()+1];
        copy(receiveline_str.begin(), receiveline_str.end(), tmp_recvline);
        tmp_recvline[receiveline_str.size()]='\0';

        char * token = strtok(tmp_recvline, "#");
        string tmp_nodeID, tmp_ip, tmp_port;
        tmp_nodeID= string(token);
        token = strtok(NULL,"#");
        tmp_ip=string(token);
        token = strtok(NULL,"#");
        tmp_port=string(token);

        pNode.rm.updateLeafSet(tmp_nodeID, tmp_ip, tmp_port);
        //pNode.rm.printRT();
    } if((pos = receiveline_str.find("PUT#")) != string::npos) {
        //for putting values-> PUT#<key>#<value>
        receiveline_str.replace(0, 4, "");
        receiveline_str.erase(receiveline_str.find_last_not_of(" \n\r\t")+1);

        char tmp_recvline[receiveline_str.size()+1];
        copy(receiveline_str.begin(), receiveline_str.end(), tmp_recvline);
        tmp_recvline[receiveline_str.size()]='\0';

        char * token = strtok(tmp_recvline, "#");
        string tmp_key, tmp_val;
        tmp_key=string(token);
        token = strtok(NULL,"#");
        tmp_val=string(token);

        string hash_key;
        hash_key.assign(get_hash_key(tmp_key));

        request_res.assign("");
        cell tmp;
        tmp=pNode.rm.routeKey(hash_key);
        if(tmp.nodeID.empty()) {
            //insert key value
            pNode.rm.insertKey(hash_key, tmp_val);
        } else {
            int sock_fd;

            char ip_chararr[tmp.ip.size()+1];
            copy(tmp.ip.begin(), tmp.ip.end(), ip_chararr);
            ip_chararr[tmp.ip.size()] = '\0';

            char port_int[tmp.port.size()+1];
            copy(tmp.port.begin(), tmp.port.end(), port_int);
            port_int[tmp.port.size()]='\0';

            int port_no = atoi(port_int);

            sock_fd = pNode.connectToServer(ip_chararr, port_no);

            request_res.append("PUT#"+tmp_key+"#"+tmp_val);
            //cout<<"Sending..."<<request_res<<endl;
            send(sock_fd, request_res.c_str(), request_res.length() + 1, 0);
            close(sock_fd);
        }
        close(conn_fd);

    } if((pos = receiveline_str.find("GET#")) != string::npos) {
        //for getting values-> GET#<key>#<value>
        receiveline_str.replace(0, 4, "");
        receiveline_str.erase(receiveline_str.find_last_not_of(" \n\r\t")+1);

        char tmp_recvline[receiveline_str.size()+1];
        copy(receiveline_str.begin(), receiveline_str.end(), tmp_recvline);
        tmp_recvline[receiveline_str.size()]='\0';

        char * token = strtok(tmp_recvline, "#");
        string tmp_key;
        tmp_key=string(token);

        string hash_key;
        hash_key.assign(get_hash_key(tmp_key));

        request_res.assign("");
        cell tmp;
        tmp=pNode.rm.routeKey(hash_key);
        if(tmp.nodeID.empty()) {
            //insert key value
            request_res.assign("");
            request_res.append(pNode.rm.getKeyValue(hash_key));
            send(conn_fd, request_res.c_str(), request_res.length()+1, 0);
            //cout<<"Sending back..."<<request_res<<endl;
        } else {
            int sock_fd;
            char recvline[BUF_SIZE];
            ssize_t ret_in=0;

            char ip_chararr[tmp.ip.size()+1];
            copy(tmp.ip.begin(), tmp.ip.end(), ip_chararr);
            ip_chararr[tmp.ip.size()] = '\0';

            char port_int[tmp.port.size()+1];
            copy(tmp.port.begin(), tmp.port.end(), port_int);
            port_int[tmp.port.size()]='\0';

            int port_no = atoi(port_int);

            sock_fd = pNode.connectToServer(ip_chararr, port_no);

            request_res.append("GET#"+tmp_key);
            //cout<<"Sending..."<<request_res<<endl;
            send(sock_fd, request_res.c_str(), request_res.length() + 1, 0);
            
            bzero(recvline, BUF_SIZE);
            if((ret_in = recv(sock_fd, recvline, BUF_SIZE-1, 0)) > 0) {
                recvline[ret_in] = '\0';
                //cout<<"sediiiinnnnggg..."<<recvline<<endl;
            }
            send(conn_fd, recvline, ret_in + 1, 0);
            close(sock_fd);
        }
        close(conn_fd);
    } else {
        request_res.assign("ERR#Invalid Format!!\nRequired: <filepath>\n");
        send(conn_fd, request_res.c_str(), request_res.length()+1, 0);
        close(conn_fd);
    }

    pthread_exit(NULL);
}

string PastryNode::getIPStr() {
    return ip_str;
}

string PastryNode::getNodeID() {
    return nodeID;
}

string PastryNode::getPortStr() {
    return PORT_str;
}

string getip() {
    struct ifaddrs * ifAddrStruct=NULL;
    struct ifaddrs * ifa=NULL;
    void * tmpAddrPtr=NULL;
    char addressBuffer[INET_ADDRSTRLEN];

    getifaddrs(&ifAddrStruct);

    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) {
            continue;
        }
        if (ifa->ifa_name[0]=='e' && ifa->ifa_addr->sa_family == AF_INET) { 
            tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
        }
    }
    if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);
    return string(addressBuffer);
}

string get_node_ID(string IP,string port) {
    string hash;

    char ip_addr[MAX_DATA];
    strcpy(ip_addr,IP.c_str());

    char port_no[MAX_DATA];
    strcpy(port_no,port.c_str());

    strcat(ip_addr,port_no);

    hash=md5(ip_addr);

    string first_two = hash.substr(0, 2);

    string hash_binary=GetBinaryStringFromHexString(first_two);

    string hash_base_4=GetBase4StringFromBinaryString(hash_binary);

    return hash_base_4.c_str(); 
}

string get_hash_key(string key)
{
    string hash;

    char hash_key[MAX_DATA];
    strcpy(hash_key,key.c_str());

    hash=md5(hash_key);

    string first_two = hash.substr(0, 2);

    string hash_binary=GetBinaryStringFromHexString(first_two);

    string hash_base_4=GetBase4StringFromBinaryString(hash_binary);
    //cout<<"in get_hash_key::hash_base_4::"<<hash_base_4<<endl;

    return hash_base_4.c_str();

}
