#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>

#define FILEPATH "../../../tmp/output.txt"
#define PAYLOAD 1460

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

typedef struct dgram{
    int seqNum;                /* store sequence # */
    int dataSize;              /* datasize , normally it will be 1460*/
    char buf[PAYLOAD];            /* Data buffer */
}UDP;

char *nodes;
int start_ptr;
char *data;
struct stat st;

struct timeval start,stop;
int totaltime;

int filesize;
int sockfd;
struct sockaddr_in server;
socklen_t length = sizeof(server);
pthread_t thrd;

int totalPacketsArrived =0 , totalSeq=0;

void Error(const char *msg){
    perror(msg);
    exit(1);
}

int Search_missing(){
	int i;
	if( nodes == NULL ){
		return -1;
	}
	for ( i = start_ptr; i <= totalSeq ; i++){
		if (nodes[i] == 0 ){
			if (i!=totalSeq)
                start_ptr=(i+1);
			else
				start_ptr=0;
            return i;
		}
	}
	start_ptr=0;
	return -1;
}

void* missing(void* arg){
    int n=0;
    int miss_pack;
    printf("Sending missing packet information\n");
    while(1){
        if(totalPacketsArrived == totalSeq+1 ) {
            printf("Entire data received!!\n");
            pthread_exit(0);
        }
        usleep(10);
        pthread_mutex_lock(&lock);
        miss_pack = Search_missing();
        pthread_mutex_unlock(&lock);
        if(miss_pack >= 0 && miss_pack<=totalSeq){
            n = sendto(sockfd, &miss_pack ,sizeof(int),0,(struct sockaddr *)&server,length); 
            if (n < 0)
                Error("Error sending\n");
        }  
    }
}

int main(int argc, char *argv[])
{
    int i=0;
    int n;
    FILE *fp;
    int errno = 0 ;
    int seq=0;
    
    UDP recv_DG;
    int ack = -1;
    
    int datasize = 0;
    
    if (argc < 2) {
        fprintf(stderr,"Error, no port provided\n");
        exit(1);
    }
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        Error("Error opening socket\n");
    bzero((char *) &server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(atoi(argv[1]));
    
    long int buffer_size = 60000000;
    
    if(setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char *)&buffer_size, (long int)sizeof(buffer_size)) == -1){
        printf("Error with setsocket\n");
    }
    if(setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char *)&buffer_size, (long int)sizeof(buffer_size)) == -1){
        printf("Error with setsocket\n");
        
    }
    
    if (bind(sockfd, (struct sockaddr *) &server, sizeof(server)) < 0)
        Error("Error on binding\n");
    
    n = recvfrom(sockfd,&filesize,sizeof(filesize),0,(struct sockaddr *)&server,&length);
    
    totalSeq = ceil(filesize / PAYLOAD);
	printf("Total sequences to receive: %d\n",totalSeq);
    
    fp = fopen(FILEPATH , "w+");
    
    if((errno = pthread_create(&thrd, 0, missing, (void*)0 ))){
        fprintf(stderr, "pthread_create %s\n",strerror(errno));
        pthread_exit(0);
    }
    
    nodes = calloc(totalSeq , sizeof(char));
    gettimeofday(&start, NULL);
    
    int value = 0;
    while (1)
    {
        n = recvfrom(sockfd,&recv_DG,sizeof(UDP),0,(struct sockaddr *)&server,&length);
        
        if(n == 4){
            continue;
		}
        if (n < 0)
            Error("Error receiving\n");
        
        pthread_mutex_lock(&lock);
        if(recv_DG.seqNum >=0 && recv_DG.seqNum <=totalSeq){
            if( nodes[recv_DG.seqNum] != 0 ){
                value = 0;
            } else{
                nodes[recv_DG.seqNum] = 1;
                value = 1;
            }
        }
        else{value = 0;}

        if(value == 1){
            fseek( fp , PAYLOAD*recv_DG.seqNum , SEEK_SET  );
            fwrite(&recv_DG.buf , recv_DG.dataSize , 1 , fp);
            fflush(fp);
            totalPacketsArrived++;
        } 
        pthread_mutex_unlock(&lock);
        if(totalPacketsArrived == totalSeq+1 ){
        printf("We received the entire file.\n");
        for(i=0 ; i<5 ; i++){
            printf("Sending ack\n");
            n = sendto(sockfd,&ack,sizeof(int), 0,(struct sockaddr *) &server,length);
            if (n < 0)
                Error("Error sending\n");
        }   
        break;
    }
}

pthread_join(thrd, 0);

fclose(fp);

gettimeofday(&stop,NULL);
totaltime = (stop.tv_sec - start.tv_sec);
printf("Time taken to complete : %d sec\n",totaltime);

munmap(data, st.st_size);

close(sockfd);
return 0;
}