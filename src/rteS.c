/** 
 *      Author : Chares Moustakas <cmoustakas@ece.auth.gr> || <charesmoustakas@gmail.com>
 *      AEM    : 8860
 *      Prof   : Nikolaos Pitsianis <pitsiani@ece.auth.gr>
 *      Prof   : Dimitrios Floros   <fcdimitr@auth.gr>
 *      
 *      Description: 
 *              Real Time Embedded System Project , the below code is about to be executed on Raspberry Pi Zero W .
 *      Compilation: 
 *       Ubuntu Disco Dingo 9.1 OS , gcc 8.3 compiler and gdb debugger, with adittional Cross-compilation tool.
 *
 */


#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<signal.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<time.h>
#include<stdbool.h>
#include<sys/time.h>
#include<pthread.h>
#include<time.h>

#define BACKLOG 1
#define MY_AEM 8860
#define LIST_LEN 30
#define SERVER_FLAG 0
#define CLIENT_FLAG 1
#define STAT_LEN 5

int port = 2288 ;
bool connection = false;

char infoPacket[272] ; //= "7592_8998_1387909800_Test";
char historyMessg[2000][272];
int historyOfAEM[2000];
int lastMssg = 0;

int keepServerStatistics[STAT_LEN];
int serverLine = 0;
int serverAvrg = 0;
int serverStdDeviation = 0;

int keepClientStatistics[STAT_LEN];
int clientLine = 0;
int clientAvrg = 0;
int clientStdDeviation = 0;


char messgForMe[100][272];
int myMssg = 0;

int historyOfHandshakes[LIST_LEN];
int handshakeCounter = 0;

char delim[] = "_";
bool communicationFLag = true;
char *prefixIP = "10.0.";
pthread_t thread[LIST_LEN-1];
pthread_t serverThr ;



//          FUNCTIONS : 

void *clientThread(void* thread_args);
void *serverThread();
void addDotToString(char* s);
bool handshake(int aem);
bool messageForMe(char packet[272]);
void keepStatistics(int flag,int timeStamp);
void calculateStatistics(int flag);
void generate_message(int signlNum);
bool checkForDuplicates(char packet[272]);



//           MAIN : 

int ctr = 0;


int main(int argc,char**argv){
    
    int sockfd =  socket(AF_INET,SOCK_STREAM,0); //Create the socket ---> ATTRIBUTES : IPV4-TCP protocol
    struct sockaddr_in serverAddress;
    
    serverAddress.sin_family = AF_INET;   
    serverAddress.sin_port = htons(port);
    
    char *bruteForceAEM ;
    char *hostIP; 
    
    
    /**
     *                                  [+][+]    Server Part Of Code  [+][+]
     *                              This Thread Routine is Executed for Ever 
     *                              Always Listen for Somebody's Communication-Request.  
     */
    
    pthread_create(&serverThr,NULL,&serverThread,NULL);

    
    /**                          
     *                                  [+][+]     Client Part Of Code   [+][+]
     *                              Initialize As threads As The Lenght of My AEM List.
     *                              In Order to Parallelize Searching for Available Raspberry Server Routine.
     **/    
    struct sigaction sign ;
    struct itimerval timer ;
 
    memset(&sign,0,sizeof(sign));
    sign.sa_handler = &generate_message;
    
    srand(time(NULL));
    int randTime = 10;//(rand()%(241) + 60  ;


    sigaction(SIGALRM,&sign,NULL);
    timer.it_value.tv_usec = 0; 
    timer.it_value.tv_sec = 10;
    timer.it_interval.tv_usec = 0;
    timer.it_interval.tv_sec = 10;
    setitimer(ITIMER_REAL,&timer,NULL);
    
    while(1){                       /**  INSERT THE WAITING LOOP */
    
        if(communicationFLag && infoPacket[0]!=0){
            

            /** 
             *               If Info Packet Has Arrived
             *              Then, Cancel The TIMER Until Threads Do Their Jobs
             
            timer.it_value.tv_usec = 0; 
            timer.it_value.tv_sec = 0;
            timer.it_interval.tv_usec = 0;
            timer.it_interval.tv_sec = 0;
            setitimer(ITIMER_REAL,&timer,NULL);
            **/


            char tempInfoPack[272] ;
            for(int i =0;i<strlen(infoPacket);i++) tempInfoPack[i] = infoPacket[i];  // Fill the Temp Array in Order to Parse Arguments Securely;

            
            
            bruteForceAEM = malloc(5*sizeof(char));
            hostIP = malloc(11*sizeof(char));

            /** 
                    BruteForce all Available Server Addresses via Multithreading[+] 
                    But first Check if Sender Inside the Packet is Available For Communication 
            **/
        
            char* receiverAEM ;
            receiverAEM = malloc(5*sizeof(char));
            receiverAEM = strtok(tempInfoPack,delim);
            receiverAEM = strtok(NULL,delim);
            historyOfAEM[lastMssg] = atoi(receiverAEM);
           
            
            addDotToString(receiverAEM);
            strcpy(hostIP,prefixIP);
            strcat(hostIP,receiverAEM);

            serverAddress.sin_addr.s_addr =inet_addr(hostIP);
            
            
            struct timeval timeout; // Set Timeout 
            timeout.tv_sec = 5;
            timeout.tv_usec = 0;
            setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO,&timeout,sizeof(struct timeval));
            if(connect(sockfd,(struct sockaddr*)&serverAddress,sizeof(serverAddress)) == -1){
            
            int counterAEM = MY_AEM - (LIST_LEN)/2;

                for(int i =0;i<LIST_LEN;i++){
                    if(i != MY_AEM) pthread_create(&thread[i],NULL,&clientThread,((void*)counterAEM+i));
                }
               
                
                for(int i =0;i<LIST_LEN;i++)if(i!=MY_AEM){pthread_join(thread[i],NULL);}
               
            }else send(sockfd,infoPacket,272,0);
            communicationFLag = !communicationFLag ;

            // Reset Timer Until New Packet Comes 
            
            timer.it_value.tv_usec = 0; 
            timer.it_value.tv_sec = 10;
            timer.it_interval.tv_usec = 0;
            timer.it_interval.tv_sec =0;
            setitimer(ITIMER_REAL,&timer,NULL);
            
        }
    }
    return 0;
}





void* clientThread(void* thread_args){
    if(infoPacket[0] != 0){

        char *bruteForceAEM = malloc(5*sizeof(char)) ;
        char *hostIP = malloc(11*sizeof(char)) ; 
    
        int bforce = (int*)thread_args;

        int sockfd =  socket(AF_INET,SOCK_STREAM,0); //Create the socket ---> ATTRIBUTES : IPV4-TCP protocol
        struct sockaddr_in clientAddress;
    
        sprintf(bruteForceAEM,"%d",bforce);
        addDotToString(bruteForceAEM);
        strcpy(hostIP,prefixIP);
        strcat(hostIP,bruteForceAEM);
    
        //if(bforce==8867){hostIP = "192.168.2.8";} // Just Testing [+]


        clientAddress.sin_family = AF_INET;   
        clientAddress.sin_port = htons(port);
        clientAddress.sin_addr.s_addr =inet_addr(hostIP);            
    
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO,&timeout,sizeof(struct timeval));
    
          //printf("[+]Checking for IP : %s \n",hostIP);
        if(connect(sockfd,(struct sockaddr*)&clientAddress,sizeof(clientAddress))==-1)return NULL; 
    
        else{ 
        
            send(sockfd,infoPacket,272,0);
            printf("[+] Sent Packet To IP : %s  \n",hostIP);
            
            memset(infoPacket,0,272*sizeof(infoPacket[0])); 
            

            struct timeval t;
            gettimeofday(&t,NULL);
            float timeStamp =  (double)(t.tv_sec*1000000 + t.tv_usec);
            keepStatistics(CLIENT_FLAG,(int)timeStamp); // Code doesnt keep Statistics For Handshake Situation 

            if(handshake(bforce)){ // If we Havent Communicate Again Take All the Messages You Should Have
            
           
                historyOfHandshakes[handshakeCounter] = bforce ;
                handshakeCounter++;
            
                for(int i =0;i<LIST_LEN;i++){
                
                    if(historyOfAEM[i] == 0) break;
                    else if (bforce == historyOfAEM[i]){
                    char tempPacket[272];
                    for(int j=0;j<272;j++)tempPacket[j] = historyMessg[i][j]; 
                    printf("[+][+] HandShake Transfer \n");
                    send(sockfd,tempPacket,272,0);

                    }
                }
            }   
        
           
        }
        close(sockfd);
    }
}


void *serverThread(){

    char recvPack[272];
    int sockfd =  socket(AF_INET,SOCK_STREAM,0); //ipv4
    struct sockaddr_in serverAddress;
    

    serverAddress.sin_family = AF_INET;                   // use ipv4 
    serverAddress.sin_addr.s_addr =  INADDR_ANY;         // Communicate with anyone 
    serverAddress.sin_port = htons(port);               // Host to Network Order  Port 
    
    bind(sockfd,(struct sockaddr*)&serverAddress,sizeof(serverAddress));  // Get Ready for Handshake 
    listen(sockfd,BACKLOG);   
    printf("Listenning for RaspBerries, port : 2288 [+] \n ");
    
    while(1){    
        
        int clientSock = accept(sockfd,NULL,NULL);          // Stay IDLE until Somebody ask for Communication  
        
        printf("Connection Captured [+]  \n");
        recv(clientSock,recvPack,271,0);
        
        memset(infoPacket,0,272*sizeof(infoPacket[0])); // Keep The Rubbish Out 

        for(int i =0;i<strlen(recvPack);i++)infoPacket[i] = recvPack[i];
        printf("[+][+] Received Message : %s \n",infoPacket);

        struct timeval t;
        gettimeofday(&t,NULL);
        float timeStamp =  (double)(t.tv_sec*1000000 + t.tv_usec);
        
        keepStatistics(SERVER_FLAG,(int)timeStamp);
       
        /**          
        *           Check If the Message was For Me and Keep It To my Message History Array 
        *           If Not keep It in General History Array 
        */
        
        char tempPacket[272];
        for(int j=0;j<272;j++)tempPacket[j] = infoPacket[j]; 
        
        if(!messageForMe(tempPacket)){
            memset(infoPacket,0,sizeof(infoPacket[0])*272);
            communicationFLag = !communicationFLag; 
           
            if(lastMssg == 2000)lastMssg = 0;
            
            for(int i=0;i<strlen(infoPacket);i++) historyMessg[lastMssg][i] = infoPacket[i] ; //Keep history of Messages
            lastMssg++;
            
        }
        else{
            /* 
            *                    Maybe A handShake Situation 
            *                   Checking for Duplicates [+][+][+]
            */
            if(!checkForDuplicates(tempPacket)){ // If There is No Duplicate Register New Message To messgForMe Array
                if(myMssg == 100)myMssg = 0;
                for(int i=0;i<strlen(infoPacket);i++)messgForMe[myMssg][i] = infoPacket[i]; //Keep history Of My Messages 
                myMssg++;
            }
        } 
        ctr++;
    }
    return 0;
}

bool handshake(int aem){
    for(int i =0;i<LIST_LEN;i++){
        if(aem == historyOfHandshakes[i])return false;
    }
    return true;
    
}

bool messageForMe(char packet[272]){
    
    char* receiverAEM ;
    receiverAEM = malloc(5*sizeof(char));
    receiverAEM = strtok(packet,delim);
    receiverAEM = strtok(NULL,delim);
    
    if(receiverAEM == NULL)return false;

    if(MY_AEM == atoi(receiverAEM)) return true;
    else return false;

}

void keepStatistics(int flag,int timeStamp){
    if(flag == SERVER_FLAG){
        if(keepServerStatistics[serverLine] = 0)keepServerStatistics[serverLine] = timeStamp; // if zero then just register timeStamp 
        else{ //if Not then find time difference between two continuesly Recvs
            keepServerStatistics[serverLine] = timeStamp - keepServerStatistics[serverLine] ;
            serverLine ++;
        }
        if(serverLine == STAT_LEN)calculateStatistics(flag);
    
    }
    else{
        if(keepClientStatistics[clientLine] == 0)keepClientStatistics[clientLine] = timeStamp;
        else{
            keepClientStatistics[clientLine] = timeStamp - keepClientStatistics[clientLine];
            clientLine++;
        }

        if(clientLine == STAT_LEN)calculateStatistics(flag);
    }

}



void calculateStatistics(int flag){
    if(flag == CLIENT_FLAG){
        clientAvrg = 0;
        // Calculate Average : 
        for(int i =0 ;i<STAT_LEN;i++)clientAvrg +=  keepClientStatistics[i];
        clientAvrg = clientAvrg/STAT_LEN;
        
        //Calculate Standard Deviation:
        for(int i=0;i<STAT_LEN;i++)clientStdDeviation += (keepClientStatistics[i] - clientAvrg )*(keepClientStatistics[i] - clientAvrg );
        clientStdDeviation = clientStdDeviation/(STAT_LEN-1);
        
        clientLine = 0;

        printf("Client Average-Time difference : %d microseconds\n",clientAvrg);
        printf("Client StdDeviation-Time  : %d \n",clientStdDeviation);

    }else{
        serverAvrg = 0;
        // Calculate Average : 
        for(int i =0 ;i<STAT_LEN;i++)serverAvrg +=  keepServerStatistics[i];
        serverAvrg = serverAvrg/STAT_LEN;
        
        //Calculate Standard Deviation:
        for(int i=0;i<STAT_LEN;i++)serverStdDeviation += (keepServerStatistics[i] - serverAvrg )*(keepServerStatistics[i] - serverAvrg );
        serverStdDeviation = serverStdDeviation/(STAT_LEN-1);
        
        serverLine = 0;

        printf("Server Average-Time difference : %d microseconds \n",serverAvrg);
        printf("Server StdDeviation-Time  : %d   \n",serverStdDeviation);
    }
}

void addDotToString(char* s){
    for(int i = 4;i>2;i--){
        s[i] = s[i-1];
    }
    s[2] = '.';
    s[5] = 0;    // Terminate The Parsed String .
}

bool checkForDuplicates(char packet[272]){
    int d = 0;
    for(int i = 0;i<100;i++){
        
        if(messgForMe[i][0] == 0) return false;
        
        for(int j = 0;j<272;j++){
            if(packet[j] == messgForMe[i][j])d++;
        }
        
        if(d == 272)return true;
        else d=0;
    }
    return false;
}


void generate_message(int signlNum) // If my Server Thread didnt Listen anything for a long time Period Generate a random Packet   
{
    if(infoPacket[0]!=0)return ;
    else{
        //char* packet = (char*)malloc(sizeof(char)*272) ;
    
        char senderAEM[5] =  "8860";
    /** strcat(senderAEM,"_");
        
        srand(time(NULL));
        int rAEM = 8835 ; //(rand()%(LIST_LEN)) + MY_AEM - (LIST_LEN/2) ;


        char recvAEM[5] ;
        sprintf(recvAEM,"%d",rAEM);
        strcat(recvAEM,"_");

                
        struct timeval time;
        gettimeofday(&time,NULL);
        float timeStamp = (double)(time.tv_sec);
        char timeStampStr[100] ;
        sprintf(timeStampStr,"%d",(int)timeStamp);
    
        strcpy(packet,senderAEM);
        //strcat(packet,recvAEM);
        //strcat(packet,timeStampStr);
        strcat(packet,"_HelloFriend") ;
        
      **/  

        char *msg_to_return = malloc(272*sizeof(char));
	
	    char lin_tmstmp[41];
	    sprintf(lin_tmstmp, "%d", (int)time(NULL));
	
	    strcpy(msg_to_return, senderAEM);
	    strcat(msg_to_return, "_");
	    strcat(msg_to_return, "8835"); //or rAEM 
	    strcat(msg_to_return, "_");
	    strcat(msg_to_return, lin_tmstmp);
	    strcat(msg_to_return, "_");
	    strcat(msg_to_return, "hello_friend");
	
        memset(infoPacket,0,272*sizeof(infoPacket[0]));
        for(int i =0;i<272;i++)infoPacket[i] = msg_to_return[i];


        
        printf("Sent Random Packet : %s  \n\n",infoPacket);
        communicationFLag = true;
    
    
    
    
    }

}