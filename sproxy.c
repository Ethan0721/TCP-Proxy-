#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <sys/select.h>
struct packet{
    int type; // 1: heartbeat 2: message
    char payload[1024];
    int ack;
    int length;
};

int main(int argc, char * argv[]){
    struct packet p1;
    struct packet p2;
    int loss = 0;
    int welcomeSocket, newSocket, clientSocket;
    char buffer[1024], buffer1[1024];
    struct sockaddr_in serverAddr, serverAddr1, clientAddr;
    int addr_size;
    welcomeSocket = socket(AF_INET, SOCK_STREAM, 0);
    serverAddr1.sin_family = AF_INET;
    serverAddr1.sin_port = htons(atoi(argv[1]));
    serverAddr1.sin_addr.s_addr = htonl(INADDR_ANY);
    memset(&(serverAddr1.sin_zero), '\0', 8);
    bind(welcomeSocket, (struct sockaddr *) &serverAddr1, sizeof(struct sockaddr));
    if(listen(welcomeSocket,5)==0)
        printf("Listening\n");
    newSocket = accept(welcomeSocket, (struct sockaddr *) &clientAddr, &addr_size);
    printf("connection with cproxy\n");

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(23);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);
    if (connect(clientSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr))<0){
        perror("connection failed");
        exit(1);
    }

    int rv, len, n;
    fd_set readfds;
    struct timeval tv;
    FD_ZERO(&readfds);
    tv.tv_sec = 1;
    tv.tv_usec = 0;
     while(1){
    FD_SET(newSocket, &readfds);
    FD_SET(clientSocket, &readfds);
    if (newSocket > clientSocket)
        n = newSocket + 1;
    else
        n = clientSocket + 1;
    
    rv = select(n, &readfds, NULL, NULL, &tv);
    if (rv == -1) {
    perror("error in select()"); // error occurred in select()
    } 
    else if (rv == 0) {  
    tv.tv_sec = 1;
    tv.tv_usec = 0; 
    loss = loss + 1;
    if(loss > 3){
    printf("Connection failed, open a new port waiting for connection!\n");
    close(newSocket);
    newSocket = accept(welcomeSocket, (struct sockaddr *) &clientAddr, &addr_size);
    printf("reconnection with cproxy\n");
    loss = 0;
}
    } else {
    if (FD_ISSET(newSocket, &readfds)) {
        if((len = recv(newSocket, &p1, sizeof p1, 0)) > 0){
            if(p1.type == 1){
                send(newSocket, &p1, sizeof p1, 0);
                printf("Reciving heartbeat from cproxy\n");
                printf("Sending ACK back\n");
                loss = 0;
            }
            else if(p1.type == 2){
                send(clientSocket, p1.payload, p1.length, 0);
            }
            }

    }
    else if (FD_ISSET(clientSocket, &readfds)) {
        if(( p2.length = recv(clientSocket, p2.payload, 1024, 0))>0){
            p2.type = 2;
            send(newSocket, &p2, sizeof p2, 0);
            }
        else{
            p2.type = 3;
            send(newSocket, &p2, sizeof p2, 0);
            close(clientSocket);
            clientSocket = socket(AF_INET, SOCK_STREAM, 0);
            if (connect(clientSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr))<0){
                perror("connection failed");
                exit(1);
    }
        }
    }
}
}
    printf("Connection ends\n");
    close(welcomeSocket);
    close(newSocket);
    close(clientSocket);
    return 0;
}
