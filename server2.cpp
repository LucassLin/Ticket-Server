#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <time.h> 
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/wait.h>
#include <sched.h>
#include "queue.h"

#include <iostream>

using namespace std;

pthread_mutex_t mutex;
pthread_cond_t cond;
queue* q;
int ticket = 50;


void doProcessing(int client){
    int buflen = 1024;
    char* buf = new char[buflen+1];
    string type = "";
    string number = "";
    int num = 0;
    int start = 0;
      // read a request
    memset(buf,0,buflen);
    int nread = recv(client,buf,buflen,0);
            if (nread == 0)
                exit(1);
            for(int i=0; i<nread; i++){
                if(*(buf+i) == '<'){
                    start = 1;
                    i++;
                }
                if(*(buf+i) == ',') start=0;
                if(start==1) type += *(buf+i);
                if(*(buf+i)==','){
                    i++;
                    while(*(buf+i)!='>'){
                        number += *(buf+i);
                        i++;
                    }
                }
            }
            num = stoi(number);

            // send a response
            string response;
            if(type=="plays"){
                response = "Purchase successfully!\n";
                ticket -= num;
                if(ticket<0){
                    response = "not enough tickets left.\n";
                    ticket += num; 
                }
                else cout << "Buy " << number << " " << type << " tickets, " << ticket <<" ticket left.\n";
            }
            else if(type == "moives"){
                string host = "localhost";
                struct hostent *hostEntry;
                hostEntry = gethostbyname(host.c_str());
                if (!hostEntry) {
                    cout << "No such host name: " << host << endl;
                    exit(-1);
                }

                // setup socket address structure
                int newport = 30000;
                struct sockaddr_in server_addr1;
                memset(&server_addr1,0,sizeof(server_addr1));
                server_addr1.sin_family = AF_INET;
                server_addr1.sin_port = htons(newport);
                memcpy(&server_addr1.sin_addr, hostEntry->h_addr_list[0], hostEntry->h_length);

                int server1 = socket(PF_INET,SOCK_STREAM,0);
                if (server1 < 0) {
                    perror("socket");
                    exit(-1);
                }

                // connect to server
                if (connect(server1,(const struct sockaddr *)&server_addr1,sizeof(server_addr1)) < 0) {
                    perror("connect");
                    exit(-1);
                }
                send(server1, buf, buflen, 0);
                memset(buf,0,buflen);
                int back = recv(server1,buf,buflen,0);
                cout << "forwarding message to another server...\n";
                for(int i=0; i<back; i++) response += *(buf+i);

            }
            else{
                response = "Error, Please enter correct type of ticket\n";
                cout << "There is no " << type <<" tickets.\n";
            }
            send(client, response.c_str(), response.length(), 0);
            close(client);
}

void queue_add(int value)
{
        /*Locks the mutex*/
        pthread_mutex_lock(&mutex);

        push(q, value);

        /*Unlocks the mutex*/
        pthread_mutex_unlock(&mutex);

        /* Signal waiting threads */
        pthread_cond_signal(&cond);
}

int queue_get()
{
       /*Locks the mutex*/
        pthread_mutex_lock(&mutex);

        /*Wait for element to become available*/
        while(empty(q) == 1)
        {
                printf("Thread %lu: \tWaiting for Connection\n", pthread_self());
                if(pthread_cond_wait(&cond, &mutex) != 0)
                {
                    perror("Cond Wait Error");
                }
        }

        /*We got an element, pass it back and unblock*/
        int val = peek(q);
        pop(q);

        /*Unlocks the mutex*/
        pthread_mutex_unlock(&mutex);

        return val;
}

static void* connectionHandler()
{
        int connfd = 0;

        /*Wait until tasks is available*/
        while(1)
        {
                connfd = queue_get();
                printf("Handler %lu: \tProcessing\n", pthread_self());
                /*Execute*/
                doProcessing(connfd);
        }
}

int main(int argc, char **argv)
{
    struct sockaddr_in server_addr,client_addr;
    socklen_t clientlen = sizeof(client_addr);
    int option, port, reuse;
    int server, client;
    int nread;

    q = createQueue(3);

            /*Initialize the mutex global variable*/
        pthread_mutex_init(&mutex,NULL);

        /*Declare the thread pool array*/
        pthread_t threadPool[3];

    // setup default arguments
    port = 32000;
      // setup socket address structure
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

      // create socket
    server = socket(AF_INET,SOCK_STREAM,0);
    if (!server) {
        perror("socket");
        exit(-1);
    }

      // set socket to immediately reuse port when the application closes
    reuse = 1;
    if (setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("setsockopt");
        exit(-1);
    }

      // call bind to associate the socket with our local address and
      // port
    if (bind(server,(const struct sockaddr *)&server_addr,sizeof(server_addr)) < 0) {
        perror("bind");
        exit(-1);
    }

      // convert the socket to listen for incoming connections
    if (listen(server,SOMAXCONN) < 0) {
        perror("listen");
        exit(-1);
    }

    for(int i = 0; i < 3; i++)
        {
                pthread_create(&threadPool[i], NULL, (void *(*)(void *))connectionHandler, (void *) NULL);
        }


        while(1)
        {
                queue_add(accept(server, (struct sockaddr*)NULL, NULL)); 

        }
    }