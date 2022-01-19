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
# define max 100
using namespace std;
pthread_mutex_t m;
struct details {
  int account;
  float amount;
} ;
int base_number=100; //Count of account number will start from 100

void *backend_server(void *arg)
{
    struct details act_details[max];    
    int front_socket = *(int *)arg;
    char buff1[1024];
    while(true)
    {
        bzero(buff1, 1024);
        int r1 = read(front_socket, buff1, sizeof(buff1));
        cout << endl;
        if(r1!=-1 && strcmp(buff1,"VOTE")==0)
        {
           L: cout << "Message from Coordinator:" << buff1 << endl;

            //logic to make decision based on random number: START
            time_t timer;
            srand((unsigned)time(&timer));
            int abortval = (rand() % 2)-1;
            cout<<endl;
            cout << "Random Number Generated to show Abort:"<<abortval<<endl;;
            if (abortval == 0)
            {
                cout<<endl;
                cout<<"Server Message: ABORT"<<endl;
                int s1 = write(front_socket, "ABORT", 256);
            }

            if (r1 != 0 && abortval!=0) 
            {
                cout<<endl;
                cout<<"Server Message: READY"<<endl;
                int s1 = write(front_socket, "READY", 256);
            }
        }
        
        //logic to make decision based on random number: END
        char decision[256];
        bzero(decision, 256);
        int r2 = read(front_socket, decision, sizeof(decision));
        if(strcmp(decision,"VOTE")==0)
            goto L;

        cout << endl
             << "Decision by Coordinator:" << decision<< endl;

        char transaction[256];
        
        int r3 = read(front_socket, transaction, sizeof(transaction));

        char transaction_tobe_processed[256];
        
        string processed_transaction;
        
        if ((strcmp(decision, "ABORT")) == 0)
        {
            bzero(transaction, 256);
            bzero(transaction_tobe_processed, 256);
            bzero(decision, 256);
            bzero(buff1,1024);
            cout<<"\nTransaction aborted\n";
        }
        else if((strcmp(decision, "COMMIT")) == 0  && ((strcmp(decision, "ABORT")) != 0))
        {   
            int i=0;
            char *c;
            float balance;
            c = strtok(transaction, " ");
            if (c!= NULL)
            {

                //performs required operations when a transaction contains "CREATE"
                if (strcmp(c, "CREATE") == 0)
                {
                    pthread_mutex_lock(&m);
                    balance = atof(strtok(NULL, " "));
                    act_details[i].amount = balance;
                    act_details[i].account = base_number;
                    base_number = base_number + 1;
                    string act = to_string(act_details[i].account);
                    string msg = "Account created: ";
                    processed_transaction=msg+act;
                    int s1 = send(front_socket, &processed_transaction, sizeof(processed_transaction), 0);
                    pthread_mutex_unlock(&m);
                    cout <<"\nCreate transaction processed.\n"<<processed_transaction;
                    processed_transaction.clear(); act.clear(); msg.clear();
                    i++;
                }
                else if (strcmp(c, "UPDATE") == 0)
                {
                    int found=0;
                    int act_index=0;
                    pthread_mutex_lock(&m);
                    int account_number = atoi(strtok(NULL, " "));
                    float amount_received = atof(strtok(NULL, " "));

                    for(int p=0;p<max;p++)
                    {
                        if (act_details[p].account==account_number)
                        {
                            found++;
                            act_index=p;
                        }
                    }
                        if (found==1)
                        {
                            string act = to_string(act_details[act_index].account);
                            string amt = to_string(act_details[act_index].amount);
                            string msg = "Account number: ";
                            string msg2= " , Updated Amount: ";
                            processed_transaction = msg+act+msg2+amt;
                            int s2 = send(front_socket, &processed_transaction, sizeof(processed_transaction), 0);
                            cout<<"\nUpdate Transaction Successfull."<<processed_transaction << endl;
                            act.clear();amt.clear();msg.clear(); msg2.clear();processed_transaction.clear();
                        }
                        else
                        {
                            string act = to_string(act_details[i].account);
                            string msg = "This account number doesn't exist: ";
                            processed_transaction = msg+act;
                            int s3 = send(front_socket, &processed_transaction, sizeof(processed_transaction), 0);
                            cout<<"\nAccount Number doesn't exist." <<processed_transaction<< endl;
                            act.clear();msg.clear();processed_transaction.clear();
                        }
                    

                    pthread_mutex_unlock(&m);
                }
                else if (strcmp(c, "QUERY") == 0)
                {

                    pthread_mutex_lock(&m);
                    int act_found=0;
                    int index=0;
                    int intput_account = atoi(strtok(NULL, " ")); 
                    for(int j=0;j<max;j++)
                    {
                        if (act_details[j].account==intput_account)
                        {
                            act_found++;
                            index=j;
                        }
                    }
                        if(act_found!=0)
                        {
                            cout<<"act_details[i].account"<<act_details[index].account<<endl;
                            cout<<"intput_account"<<intput_account<<endl;
                            string act = to_string(act_details[index].account);
                            string amt = to_string(act_details[index].amount);
                            string msg = "Account number: ";
                            string msg2= " ,Amount: ";
                            processed_transaction = msg+act+msg2+amt;
                            int s4 = send(front_socket, &processed_transaction, sizeof(processed_transaction), 0);
                            cout<<"\nQUERY transaction porcessed."<<processed_transaction << endl;
                            act.clear();amt.clear();msg.clear(); msg2.clear();processed_transaction.clear();
                        }
                        else
                        {
                            string act = to_string(intput_account);
                            string msg = "Account number not found: ";
                            processed_transaction = msg+act;
                            int s5 = send(front_socket, &processed_transaction, sizeof(processed_transaction), 0);
                            cout<< "Transaction cannot be processed: Account number doesn't exist"<<processed_transaction << endl;
                            act.clear();msg.clear();processed_transaction.clear();
                        }
                    

                    pthread_mutex_unlock(&m);
                }


            }
            
        }

    }
}

int main(int argc, char *argv[])
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
    front_server.sin_port = htons(atoi(argv[1]));
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
                pthread_create(&newthread, NULL, backend_server, &coordinator_socket);
            }
    }
    close(coordinator_socket);    
    close(listening);

    return 0;
}