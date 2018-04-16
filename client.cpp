#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string>
#include <fstream>
#include <iostream>

using namespace std;

class client{

private:
	int port, server;
	struct sockaddr_in server_addr;
	char* buf;
	string host = "localhost";

public:
	client(){
		buf = new char[1024];
		memset(buf,0,1024);
	}

	~client(){
		close(server);
	}

	void Init(int port){
		struct hostent *hostEntry;
    	hostEntry = gethostbyname(host.c_str());
    	if (!hostEntry) {
        	cout << "No such host name: " << host << endl;
        	exit(-1);
    	}
		memset(&server_addr,0,sizeof(server_addr));
    	server_addr.sin_family = AF_INET;
    	server_addr.sin_port = htons(port);
    	memcpy(&server_addr.sin_addr, hostEntry->h_addr_list[0], hostEntry->h_length);

    	server = socket(PF_INET,SOCK_STREAM,0);
    	if (server < 0) {
        	perror("socket");
        	exit(-1);
    	}
    	if (connect(server,(const struct sockaddr *)&server_addr,sizeof(server_addr)) < 0) {
        	perror("connect");
        	exit(-1);
    	}
    	cout << "What ticket would you like to buy and how many do you want?\n";
	}

	void BuyTicket(){
		string line;
		cout << "Enter <TicketType, #of tickets>\n";
		getline(cin,line);
			if(line=="exit"){
            	close(server);
            	exit(1);
        	}
        	send(server, line.c_str(), line.length(), 0);
        	memset(buf,0,1024);
        	recv(server,buf,1024,0);
        	cout << buf <<endl;
		close(server);
	}
};

int main(){
	client* c = new client();
	srand (time(NULL));
	int random = rand() % 2;
	int portnumber;
	if(random==1) portnumber=30000;
	else portnumber=32000;
	c->Init(portnumber);
	c->BuyTicket();
}