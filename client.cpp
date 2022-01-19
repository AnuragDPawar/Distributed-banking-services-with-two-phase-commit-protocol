#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string>
#include <stdio.h>
#include <string.h>

using namespace std;

void check_error_in_sending(int x)
{
    if(x==-1)
        {
            cout<<"Error in sending\n";
        }
}

void response_from_server(int x)
{
    char buff[1024];    
    bzero(buff, 1024);
    sleep(5);
    int read_tran = read(x, buff, sizeof(buff));

        if (strcmp(buff, "ABORT") == 0)
        {
            cout << "Transaction was aborted by the server." << endl;
        }
        else
        {
            cout<<"Response from server: "<<endl;
            cout <<buff<<endl;            
        }
    
}

int main()
{   
    //creatomg socket
    int csock = socket(AF_INET, SOCK_STREAM,0);
    if (csock == -1)
    {
        cerr << "socket not created\n";
    }

    int port = 54004;
    string IP = "127.0.0.1";
    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(53000);
    inet_pton(AF_INET, IP.c_str(), &hint.sin_addr);

    //connecting to the server
    int connreq = connect(csock,(sockaddr *)&hint, sizeof(sockaddr_in));
    if(connreq==-1)
    {
        cout<<"Error while connecting to front-end server\n";
    }
    int choice,account;
    int amount;
    string amt,act,transaction,request;
    string space=" ";

    while (true)
    {
       
        cout<<"Please select transaction to be performed\n";
        cout<<"1. CREATE\n";
        cout<<"2. UPDATE\n";
        cout<<"3. QUERY\n";
        cout<<"4. QUIT\n";
        cin>>choice;
        //send to front end server
        switch (choice)
        {
        case 1:
        {
            cout<<"Please enter the initial amount\n";
            amt.clear();transaction.clear();request.clear();
            cin>>amount;
            amt=to_string(amount);
            transaction="CREATE ";
            request=transaction+amt;
            int request_1 = write(csock, request.c_str(), request.size()+1);
            check_error_in_sending(request_1);
            cout<<request<<endl;
            response_from_server(csock);
            break;
        }

        case 2:
        {
            cout<<"Please enter the account number and the amount\n";
            amt.clear();act.clear();transaction.clear();request.clear();
            cin>>account>>amount;
            act=to_string(account);
            amt=to_string(amount);
            transaction="UPDATE ";            
            request=transaction+act+space+amt;
            int request_2 = write(csock, request.c_str(), request.size()+1);
            check_error_in_sending(request_2);
            cout<<request<<endl;
            response_from_server(csock);
            break;
        }

        case 3:
        {
            cout<<"Please enter the account number\n";
            act.clear();transaction.clear();request.clear();
            cin>>account;
            act=to_string(account);
            transaction="QUERY ";
            request=transaction+act;
            int request_3 = write(csock, request.c_str(), request.size()+1);
            check_error_in_sending(request_3);
            cout<<request<<endl;
            response_from_server(csock);
            break;
        }

        case 4:
            exit(0);
            break;
        
        default: cout<<"Enter correct chice\n";
            break;
        }

    }

    close(csock);
    
    return 0;
}
