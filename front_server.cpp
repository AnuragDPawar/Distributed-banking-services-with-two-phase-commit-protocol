#include <arpa/inet.h>
#include <libexplain/read.h>
#include <netdb.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <string>
using namespace std;
int back_end_servers[]={54000,54001,54002};


void *co_ordinator(void *arg)
{
    int client_socket = *(int *)arg;
    cout << "Client connected"
         << " " << client_socket << " "
         << "Thread ID"
         << " " << pthread_self() << endl;
    
    int back_server[3];
    int back_server_port[3];
    back_server_port[0] = htons(back_end_servers[0]);
    back_server_port[1] = htons(back_end_servers[1]);
    back_server_port[2] = htons(back_end_servers[2]);
    int total_servers = 0;
    int online_servers[3] = {0};

        for (int i = 0; i < 3; i++)
    {
        struct sockaddr_in backend_server;
        backend_server.sin_family = AF_INET;
        backend_server.sin_addr.s_addr = inet_addr("127.0.0.1");
        backend_server.sin_port = ((back_server_port[i]));

        //array of sockets to connect to multiple servers one by one

        back_server[i] = socket(AF_INET, SOCK_STREAM, 0);
        if (back_server[i] < 0)
        {
            cout << "Coordinator thread started: Socket creation failed for backend server" << endl;
            return 0;
        }
        else
        {
            cout << "Coordinator thread started: Socket created for backend server" << endl;

        }

        //Connect to all back end servers 
        if (connect(back_server[i], (struct sockaddr *)&backend_server, sizeof(backend_server)) >= 0)
        {
            online_servers[i] = 1;
            total_servers++;
        }
    }

     while (true)
    {

        char transaction_request[1024];
        char transaction_response[1024];
        //read request from client
        int read_from_client = read(client_socket, transaction_request, sizeof(transaction_request));
        cout << "Transaction requested by client: " << transaction_request << endl;

        if (strcmp(transaction_request, "QUIT") == 0)
        {
            cout<<"\nClient terminated the connection\n";
            close(client_socket);
            break;
        }
        //ask the backend servers to provide their decison
        if(read_from_client>0)
        {
            for (int j = 0; j < 3; j++)
            {
                if (online_servers[j] == 1)
                {
                    int status = send(back_server[j], "VOTE", 256, 0); //sending VOTE to backend servers
                    if(status==-1)
                    {
                        cout<<"Error in sending VOTE message to backend servers"<<endl;
                    }
                }
            }
        }
        cout<<"\nBack-end servers asked for a VOTE."<<endl;

        struct timeval time_out;
        time_out.tv_sec = 2;
        time_out.tv_usec = 0;
        online_servers[3] = {0};
        char response_from_back_server[1024];
        int count = 0;
        int abort_count = 0;

        for (int i = 0; i < 3; i++)
        {
            //https://linux.die.net/man/2/setsockopt
            //https://stackoverflow.com/questions/4181784/how-to-set-socket-timeout-in-c-when-making-multiple-connections/4182564#4182564
            if (setsockopt(back_server[i], SOL_SOCKET, SO_RCVTIMEO, (char *)&time_out, sizeof(time_out)) ==-1)
            {
                cout << "Timeout error" << endl;
                return 0;
            }
            while (recv(back_server[i], response_from_back_server, 256, 0) > 0)
            {
                //checking if any of the backend server is saying ABORT
                if (strcmp(response_from_back_server, "ABORT") == 0)
                {
                    abort_count++;
                }
                count++; //this will catch the number of active servers
                online_servers[i] = 1;
            }
        }
        if (abort_count > 0)
        {
            char abort_msg[] = "ABORT";
            for (int p = 0; p < 3; p++)
            {
                send(back_server[p], abort_msg, sizeof(abort_msg), 0); //Informing backend servers that transaction is aborted
            }
            int s = send(client_socket,"Transaction aborted", 1024, 0); //Informing client that transaction is aborted
            cout << "Transaction Aborted" << endl;
        }
        else
        {
            cout << "Active backend servers: " << count;

            if (count == 0)
            {
                cout << "\nNo active backend servers" << endl;
                string inactive_msg = "Service not available [backend server unavailable]";
                int send_to_client = send(client_socket, &inactive_msg, sizeof(inactive_msg), 0);
            }
            else
            {
                if (count <= total_servers) 
                {
                    char buff2[256];
                    int p = 0;
                    while (recv(back_server[p], buff2, 256, 0) != 0)
                    {
                        if (p == 3)
                        {
                            break;
                        }
                        if (online_servers[p] == 1)
                        {
                            int s3 = send(back_server[p], "COMMIT", 256, 0); //sending final decision COMMIT to all available backend servers

                            total_servers++;
                        }
                        p++;
                    }
                    char buffer3[256];
                    total_servers = 0;
                    for (int q = 0; q < 3; q++)
                    {

                        if (recv(back_server[q], buffer3, 256, 0) != 0)
                        {
                            if (online_servers[q] == 1)
                            {
                                //sending client's transaction request to all the available backend servers
                                int s4 = send(back_server[q], transaction_request, sizeof(transaction_request), 0); 
                                total_servers++;
                            }
                        }
                        else
                        {
                            online_servers[q] = 0;
                        }
                    }
                    //To receive the processed transaction from the backend servers
                    total_servers = 0;

                    char buff4[1024];
                    bzero(transaction_request, 1024);
                    for (int i = 0; i < 3; i++)
                    {

                        if (recv(back_server[i], transaction_response, 1024, 0) != 0)
                        {
                            if (online_servers[i] == 1)
                            {
                                total_servers++;
                            }
                        }
                        else
                        {
                            online_servers[i] = 0;
                        }
                    }
                    cout <<transaction_response<< endl;
                    int msg_to_client = send(client_socket, transaction_response, sizeof(transaction_response), 0);
                    cout<<transaction_request<<endl;
                }
            }
        bzero(transaction_request, 256);
        bzero(transaction_response, 256);
        bzero(response_from_back_server, 256);


        }

    }
    close(client_socket);

}

int main()
{
//Create a server socket
    int listening = 0;
    listening = socket(AF_INET, SOCK_STREAM, 0);
    if (listening == -1) {
        cerr << "socket not created\n";
    }

    else {
        cout << "Socket created with FD: " << listening << "\n";
    }

    int reuse_address = 1;
    //Below code is referred from: https://pubs.opengroup.org/onlinepubs/000095399/functions/setsockopt.html
    //To reuse the address
    /*if(setsockopt(listening, SOL_SOCKET, SO_REUSEADDR, &reuse_address, sizeof(reuse_address)) != 0){
		cout<<"Failed to reuse the address"<<endl;
	}*/
    //To reuse the port
    if (setsockopt(listening, SOL_SOCKET, SO_REUSEPORT, &reuse_address, sizeof(reuse_address)) != 0) {
        cout << "Failed to reuse the port" << endl;
    }

    //Bind socket on ip & port
    sockaddr_in front_server;
    front_server.sin_family = AF_INET;
    front_server.sin_port = htons(53000);
    inet_pton(AF_INET, "127.0.0.1", &front_server.sin_addr);

    if (bind(listening, (sockaddr *)&front_server, sizeof(front_server)) == -1) {
        cerr << "Binding failed\n";
    }

    //Make the socket listen
    if (listen(listening, 4) == -1) {
        cerr << "Listening failed\n";
    }

    //accpet the connection
    sockaddr_in client;
    socklen_t clientsize = sizeof(client);
    char host[NI_MAXHOST];
    char svc[NI_MAXSERV];
    int coordinator_socket;
    pthread_t newthread;
  
    while (coordinator_socket = accept(listening, (struct sockaddr *)&client, (socklen_t *)&clientsize)) 
    {
            if (coordinator_socket == -1) 
            {
                cerr << "Unable to connect with back-end server\n";
                continue;
            } else 
            {
                pthread_create(&newthread, NULL, co_ordinator, &coordinator_socket);
            }
    }
    close(coordinator_socket);    
    close(listening);

    return 0;
}
