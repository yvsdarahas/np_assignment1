#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>
#include <netdb.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
/* You will to add includes here */

// Included to get the support library
#include <calcLib.h>

// Enable if you want debugging to be printed, see examble below.
// Alternative, pass argument during compilation '-DDEBUG'
#define DEBUG

using namespace std;
int client_sock;

void checkJobbList(int signum)
{
    char timeout_msg[10] = "ERROR TO\n";
    if (send(client_sock, timeout_msg, strlen(timeout_msg), 0) < 0)
        printf("\n Error send\n");
    close(client_sock);
}

int main(int argc, char *argv[])
{
    /*
    Read first input, assumes <ip>:<port> syntax, convert into one string (Desthost) and one integer (port).
    Atm, works only on dotted notation, i.e. IPv4 and DNS. IPv6 does not work if its using ':'.
    */
    char delim[] = ":";
    char *Desthost = strtok(argv[1], delim);
    char *Destport = strtok(NULL, delim);
    // *Desthost now points to a sting holding whatever came before the delimiter, ':'.
    // *Dstport points to whatever string came after the delimiter.

    /* Do magic */
    int port = atoi(Destport);
    #ifdef DEBUG
        printf("Host %s, and port %d.\n", Desthost, port);
    #endif

    //Getting the address info(Ipv4/Ipv6/Dns)
    //Useful when dns host is used
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int addr_info = getaddrinfo(Desthost, Destport, &hints, &res);
    if (addr_info != 0){
        printf("\n Error in getting client's info \n");
        exit(1);
    }

    // Initialise socket
    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0)
    {
        printf("\n Error creating socket \n");
        exit(1);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(Desthost);

    // Bind socket
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("\n Error in binding \n");
        exit(1);
    }

    // Listen to the clients
    if ((listen(sockfd, 5)) != 0)
    {
        printf("Listen failed \n");
        exit(1);
    }

    struct itimerval alarmTime;
    alarmTime.it_interval.tv_sec = 5;
    alarmTime.it_interval.tv_usec = 5;
    alarmTime.it_value.tv_sec = 5;
    alarmTime.it_value.tv_usec = 5;

    while (1)
    {
        socklen_t server_addr_size;
        server_addr_size = sizeof(struct sockaddr_in);

        // Accept the client connection here
        client_sock = accept(sockfd, (struct sockaddr *)&server_addr, &server_addr_size);
        if (client_sock < 0)
        {
            printf("\n Not accepted \n");
            exit(1);
        }

        /* Regiter a callback function, associated with the SIGALRM signal, which will be raised when the alarm goes of */
        signal(SIGALRM, checkJobbList);
        setitimer(ITIMER_REAL, &alarmTime, NULL); // Start/register the alarm.

        const char *arith[] = {"add", "sub", "mul", "div", "fadd", "fsub", "fmul", "fdiv"};

        // Send the supported protocol of the server to client
        char server_msg[] = "TEXT TCP 1.0\n";
        strcat(server_msg, "\n");
        int send_len = send(client_sock, server_msg, sizeof(server_msg), 0);
        if (send_len < 0)
        {
            printf("\n Error send\n");
            exit(1);
        }

        // If client accepts the protocol it replies with "OK"
        // If "OK" then server randomly creates 2 values of float or integer based on operation
        char from_client[256];
        memset(from_client, 0, sizeof(from_client));
        int recv_len = recv(client_sock, &from_client, sizeof(from_client), 0);
        if (recv_len < 0)
        {
            printf("\n Error recv\n");
            exit(1);
        }

        int random, o1, o2, result;
        float f1, f2, fresult;
        const char *array[3];
        srand(time(0));
        random = rand() % 8;
        array[0] = arith[random];

        if (strcmp(from_client, "OK\n") == 0)
        {
            if (array[0][0] != 'f')
            {
                o1 = rand() % 100;
                o2 = rand() % 100;
                memset(server_msg, 0, sizeof(server_msg));
                sprintf(server_msg, "%s %d %d\n", array[0], o1, o2);
            }

            else
            {
                f1 = (float)rand() / (float)(RAND_MAX)*100;
                f2 = (float)rand() / (float)(RAND_MAX)*100;
                memset(server_msg, 0, sizeof(server_msg));
                snprintf(server_msg, 256, "%s %8.8g %8.8g\n", array[0], f1, f2);
            }
        }
        else
        {
            memset(server_msg, 0, sizeof(server_msg));
            strcat(server_msg, "ERROR\n");
        }

        // Send the string of format "<val1> <val2> <opeartion>" to the client
        send_len = send(client_sock, server_msg, strlen(server_msg), 0);
        if (send_len < 0)
        {
            printf("\n Error send\n");
            exit(1);
        }

        // Calculate the result of generated operation for comparing it with the client's result
        char serv_result[256];

        if (array[0][0] != 'f')
        {
            if (strcmp("add", array[0]) == 0)
                result = o1 + o2;
            else if (strcmp("sub", array[0]) == 0)
                result = o1 - o2;

            else if (strcmp("mul", array[0]) == 0)
                result = o1 * o2;
            else if (strcmp("div", array[0]) == 0)
                result = o1 / o2;
            sprintf(serv_result, "%d\n", result);
        }
        else
        {
            if (strcmp("fadd", array[0]) == 0)
                fresult = f1 + f2;
            else if (strcmp("fsub", array[0]) == 0)
                fresult = f1 - f2;
            else if (strcmp("fmul", array[0]) == 0)
                fresult = f1 * f2;
            else if (strcmp("fdiv", array[0]) == 0)
                fresult = f1 / f2;
            sprintf(serv_result, "%8.8g\n", fresult);
        }
        memset(from_client, 0, sizeof(from_client));

        // Receive a result from the client
        recv_len = recv(client_sock, &from_client, sizeof(from_client), 0);
        if (recv_len < 0)
        {
            printf("\n Error recv\n");
            exit(1);
        }

        // Compare the actual result with the client's result
        // If result is true then send "OK" or send "Error"
        if (array[0][0] == 'f')
        {
            float fres_client = atof(from_client);
            memset(server_msg, 0, sizeof(server_msg));
            if (abs(fres_client - fresult) < 0.0001)
                strcat(server_msg, "OK\n");
            else
                strcat(server_msg, "Error\n");
        }
        else
        {
            int res_client = atoi(from_client);
            memset(server_msg, 0, sizeof(server_msg));
            if (abs(res_client - result) < 0.0001)
                strcat(server_msg, "OK\n");
            else
                strcat(server_msg, "Error\n");
        }

        send_len = send(client_sock, server_msg, strlen(server_msg), 0);
        if (send_len < 0)
        {
            printf("\n Error send\n");
            exit(1);
        }

        // Disconnect with the client after the process
        alarm(0);
        close(client_sock);
        sleep(1);
    }

    close(sockfd);
}