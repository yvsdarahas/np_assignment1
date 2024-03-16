#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <math.h>
#include <regex.h>
/* You will to add includes here */

// Enable if you want debugging to be printed, see examble below.
// Alternative, pass CFLAGS=-DDEBUG to make, make CFLAGS=-DDEBUG
#define DEBUG


// Included to get the support library
#include <calcLib.h>

int main(int argc, char *argv[]){

	/*
		Read first input, assumes <ip>:<port> syntax, convert into one string (Desthost) and one integer (port).
		Atm, works only on dotted notation, i.e. IPv4 and DNS. IPv6 does not work if its using ':'.
	*/
	char delim[]=":";
	char *Desthost=strtok(argv[1],delim);
	char *Destport=strtok(NULL,delim);
	// *Desthost now points to a sting holding whatever came before the delimiter, ':'.
	// *Dstport points to whatever string came after the delimiter.

	/* Do magic */
	int port=atoi(Destport);
	#ifdef DEBUG
		printf("Host %s, and port %d.\n",Desthost,port);
	#endif

	//Getting the address info(Ipv4/Ipv6/Dns)
	//Useful when dns host is used
	struct addrinfo hints, *res;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	int addr_info = getaddrinfo(Desthost, Destport, &hints, &res);
	if (addr_info != 0){
		printf("\n Error in getting Server's info \n");
		exit(1);
	}

	//Initialising Socket
	int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sockfd < 0){
		printf("\n Error creating socket \n");
		exit(1);
	}

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = inet_addr(Desthost);

	//Connection to the server is established here
	int connection = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (connection < 0){
		perror("\n Error in connection");
		exit(1);
	}

	//If server successfully accepts the connection then it sends its version as "TEXT TCP 1.0"
	//If connection is not successful client receives a buffer of length -1 i.e "Error"
	char from_server[256], to_server[256];
	int recv_len = recv(sockfd, &from_server, sizeof(from_server), 0);
	if (recv_len < 0) {
		printf("\n Error recv\n");
		exit(1);
	}

	//If the clients accepts this protocol then it sends message "OK" or it sends message "Error"
	regex_t regex;
	int rc = regcomp(&regex, "TCP 1.0", 0);
	rc = regexec(&regex, from_server, 0, NULL, 0);
	if (rc != 0) {
		printf("\n No supported protocol \n");
		exit(1);
	}
	printf("\n %s", from_server);
	memset(to_server, 0, sizeof(to_server));
	strcat(to_server, "OK\n");
	int send_len = send(sockfd, to_server, strlen(to_server), 0);
	if (send_len < 0) {
		printf("\n Error send\n");
		exit(1);
	}

	//Error handling of received buffer from server
	memset(from_server, 0, sizeof(from_server));
	recv_len = recv(sockfd, &from_server, sizeof(from_server), 0);
	if (recv_len < 0) {
		printf("\n Error recv\n");
		exit(1);
	}

	//Here server sends the 2 values and its operation to do <value1> <value2> <operation>
	//The values and the operation are obtained from the string received from server
	printf(" The received operation is : %s", from_server);
	char *oper[3];
	int i = 0;
	oper[i] = strtok(from_server, " ");
	while (oper[i] != NULL) {
	    oper[++i] = strtok(NULL, " ");
	}
	int o1 = atoi(oper[1]);
	int o2 = atoi(oper[2]);
	float f1 = atof(oper[1]);
	float f2 = atof(oper[2]);
	int result;
	float fresult;

	//Calculate the result of two value obtained from the server based on operation

	//If it is integer operation add, sub, mul, div
	if (oper[0][0] != 'f') {
		if (strcmp("add", oper[0]) == 0) {
			result = o1 + o2;
		}
		else if (strcmp("sub", oper[0]) == 0) {
			result = o1 - o2;
		}
		else if (strcmp("mul", oper[0]) == 0) {
			result = o1 * o2;
		}
		else if (strcmp("div", oper[0]) == 0) {
			result = o1 / o2;
		}
		memset(to_server, 0, sizeof(to_server));
		sprintf(to_server, "%d\n", result);
		printf("\n Result : %d \n", result);
	}

	//If it is float operation fadd, fsub, fmul, fdiv
	else {
		if (strcmp("fadd", oper[0]) == 0) {
			fresult = f1 + f2;
		}
		else if (strcmp("fsub", oper[0]) == 0) {
			fresult = f1 - f2;
		}
		else if (strcmp("fmul", oper[0]) == 0) {
			fresult = f1 * f2;
		}
		else if (strcmp("fdiv", oper[0]) == 0) {
			fresult = f1 / f2;
		}
		memset(to_server, 0, sizeof(to_server));
		sprintf(to_server, "%8.8g\n", fresult);
		printf("\n Result : %8.8g \n", fresult);
	}

	//Send the result to the server in the form "result\n"
	send_len = send(sockfd, to_server, strlen(to_server), 0);
	if (send_len < 0) {
		printf("\n Error send\n");
		exit(1);
	}

	//Receive message "OK" if the result is correct or message "Error" if result is wrong
	memset(from_server, 0, sizeof(from_server));
	recv_len = recv(sockfd, &from_server, sizeof(from_server), 0);
	if (recv_len < 0) {
		printf("\n Error recv\n");
		exit(1);
	}
	printf("\n Server response : %s", from_server);
	close(sockfd);
	return 0;
  
}
