#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
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
    struct packet p3;
    int loss = 0;
    int newSocket, telnetSocket, sproxySocket;
    char buffer[1024], buffer1[1024];
    struct sockaddr_in serverAddr, serverAddr1, clientAddr;
    int addr_size;
    telnetSocket = socket(AF_INET, SOCK_STREAM, 0);
    serverAddr1.sin_family = AF_INET;
    serverAddr1.sin_port = htons(atoi(argv[1]));
    serverAddr1.sin_addr.s_addr = htonl(INADDR_ANY);
    memset(&(serverAddr1.sin_zero), '\0', 8);
    bind(telnetSocket, (struct sockaddr *) &serverAddr1, sizeof(struct sockaddr));
    if(listen(telnetSocket,5)==0)
        printf("Cproxy is listening from user\n");
    newSocket = accept(telnetSocket, (struct sockaddr *) &clientAddr, &addr_size);
    printf("connection with telent\n");

    sproxySocket = socket(AF_INET, SOCK_STREAM, 0);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(atoi(argv[3]));
    serverAddr.sin_addr.s_addr = inet_addr(argv[2]);
    //serverAddr.sin_addr.s_addr = inet_addr("192.168.6.3");
    memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);
    if (connect(sproxySocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr))<0){
        perror("connection failed");
        exit(1);
    }

    int rv, n, len;
    fd_set readfds, heartbeat;
    struct timeval tv;
    FD_ZERO(&readfds);
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    while(1){
    FD_SET(newSocket, &readfds);
    FD_SET(sproxySocket, &readfds);
    if (newSocket > sproxySocket)
        n = newSocket + 1;
    else
        n = sproxySocket + 1;

    rv = select(n, &readfds, NULL, NULL, &tv);

    if (rv == -1) {
    perror("error in select()"); // error occurred in select()
    } 
    else if (rv == 0) {
    p3.type = 1;
    send(sproxySocket, &p3, sizeof p3, 0);
    printf("Sending heartbeat\n");
    loss = loss + 1;
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    if(loss>3){
    printf("Connection failed, try to reconnect!\n");
    close(sproxySocket);
    sproxySocket = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(sproxySocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr))<0){
        printf("connection failed\n");
    }
    loss = 0;
    }
    
}
    else {
    if (FD_ISSET(newSocket, &readfds)) {
       if((p1.length = recv(newSocket, p1.payload, 1024,0)) > 0){
                p1.type = 2;
                send(sproxySocket, &p1, sizeof p1, 0);
            }
    }
    else if (FD_ISSET(sproxySocket, &readfds)) {
        if((len = recv(sproxySocket, &p2, sizeof p2, 0))>0){
                if(p2.type == 1){
                    printf("Receiving ACK back\n");
                    loss = 0;
                }
                else if(p2.type == 2){
                    send(newSocket, p2.payload, p2.length, 0);
                }
                else if(p2.type == 3){
                    close(newSocket);
                    newSocket = accept(telnetSocket, (struct sockaddr *) &clientAddr, &addr_size);
                    printf("connection with telent\n");
                }
    }
    else{
        
    }
   
}
}
}
    printf("Connection ends\n");
    close(newSocket);
    close(sproxySocket);
    close(telnetSocket);
    return 0;
}
                                            

