#include<stdio.h>
#include<string.h>    //strlen
#include<stdlib.h>    //strlen
#include<sys/socket.h>
#include<arpa/inet.h> //inet_addr
#include<unistd.h>    //write
#include<pthread.h> //for threading , link with lpthread

//thread function
void *connection_handler(void *);

//connected clients
int client_list[100];
int client_count = 0;

int create_socket(){
    
    int sock;
    
    //Create socket
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1)
    {
        printf("Could not create socket\n");
    }
    puts("Socket created");
    
    return sock;
}

int listen_clients(int socket_desc){
    
    struct sockaddr_in server , client;
    int client_sock , c , *new_sock;
    
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( 8888 );
    
    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        perror("Bind failed. Error");
        return 1;
    }
    puts("Bind done");
    
    //Listen
    listen(socket_desc , 3);
    
    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
    
    while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
    {
        puts("Connection accepted");
        
        client_list[client_count] = client_sock;
        client_count++;
        
        pthread_t sniffer_thread;
        
        new_sock = malloc(1);
        *new_sock = client_sock;
        
        if( pthread_create( &sniffer_thread , NULL ,  connection_handler , (void*) new_sock) < 0)
        {
            perror("could not create thread");
            return 1;
        }
        
        //Now join the thread , so that we dont terminate before the thread
        pthread_join( sniffer_thread , NULL);
        puts("Handler assigned");
    }
    
    if (client_sock < 0)
    {
        perror("accept failed");
        return 1;
    }
    return 0;
}

void remove_from_client_list(int sock){
    int i=0, tmp_sock = 0;
    
    for(i=0;i<client_count;i++){
        tmp_sock = client_list[i];
        if(tmp_sock == sock){
            client_list[i] = -1;
            client_count --;
            break;
        }
    }
}

void initialize_client_list(){
    int i = 0;
    for(i=0;i<100;i++){
        client_list[i] = -1;
    }
}

int main(int argc , char *argv[])
{
    initialize_client_list();
    int socket_desc;
    
    socket_desc = create_socket();
    listen_clients(socket_desc);
    
    return 0;
}

/*
 * This will handle connection for each client
 * */
void *connection_handler(void *socket_desc)
{
    //Get the socket descriptor
    int sock = *(int*)socket_desc, i=0;
    long read_size;
    char client_message[8];

    //Receive a message from client
    while( (read_size = recv(sock , client_message , 7 , 0)) > 0 )
    {
        //Send the message back to client
        for(i=0;i<client_count;i++){
            if(client_list[i] != -1){
                write(client_list[i] , client_message , strlen(client_message));
            }
        }
    }
    
    if(read_size == 0)
    {
        puts("Client disconnected");
        remove_from_client_list(sock);
        fflush(stdout);
    }
    else if(read_size == -1)
    {
        perror("Recv failed");
    }
    
    //Free the socket pointer
    free(socket_desc);
    
    return 0;
}