#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/stat.h>
#include <pthread.h>
#include <time.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>

#define FILENAME "../../../tmp/data.txt"
#define PAYLOAD 1460

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

int filesize = 0;
int errno= -1;
char *data;
char *nodes;

struct sockaddr_in server;
struct stat st;
int start_ptr;

int totalPacketsArrived =0 , totalSeq=0;

struct timeval start , stop;
int totaltime;
socklen_t length = sizeof(server);

typedef struct dgram{
    int seqNum;                /* store sequence # */
    int dataSize;              /* datasize , normally it will be 1460*/
    char buf[PAYLOAD];            /* Data buffer */
}UDP;

pthread_t thrd;
int sockfd;

void error(const char *msg){
    perror(msg);
    exit(0);
}


void *missing(void* arg){
    int totalseq, last_pack, dup,n=0;
    UDP send_DG ,  recvDG;
    int packetmiss;
    
    printf("Receiving missing packet information\n");
    while (1) {
        n = recvfrom(sockfd,&packetmiss,sizeof(int),0,(struct sockaddr *)&server,&length);
        if(n<0){
            error("Error receiving\n");
        }
        if(packetmiss < 0){
            printf("Entire data received\n");
            pthread_exit(0);
        }
        totalseq =  filesize / PAYLOAD;
        last_pack = filesize % PAYLOAD;
        
        dup = PAYLOAD;
        if(last_pack != 0 && totalseq ==  packetmiss){
        	dup = last_pack;
        }
        pthread_mutex_lock(&lock);
        memcpy(send_DG.buf , &data[packetmiss*PAYLOAD], dup );
        pthread_mutex_unlock(&lock);
        
        send_DG.seqNum = packetmiss;
        send_DG.dataSize = dup;
        
        n = sendto(sockfd,&send_DG,sizeof(UDP), 0,(struct sockaddr *) &server,length);
        if (n < 0)
            error("Error sending\n");       
    }
}

int main(int argc, char *argv[]){

    int seq = 0 , datasize =0 ;
    int ack = -1;
    UDP recvDG;
    FILE* fp = NULL;
    int n;
    struct hostent *ser;
    int i;
    
    if (argc < 3) {
        fprintf(stderr,"usage %s hostname port\n", argv[0]);
        exit(0);
    }
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket\n");
    ser = gethostbyname(argv[1]);
    if (ser == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &server, sizeof(server));
    server.sin_family = AF_INET;
    bcopy((char *)ser->h_addr, (char *)&server.sin_addr.s_addr,ser->h_length);
    server.sin_port = htons(atoi(argv[2]));
    
    long int buffer_size = 60000000;
    
    if(setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char *)&buffer_size, (long int)sizeof(buffer_size)) == -1){
        printf("error with setsocket levels\n");
    }
    if(setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char *)&buffer_size, (long int)sizeof(buffer_size)) == -1){
        printf("error with setsocket levels\n");
    }
    
	if ((fp = fopen(FILENAME, "r")) == NULL){
        error("Error opening file\n");
	}
	
	if(stat(FILENAME,&st)==0){
        filesize=st.st_size;
        printf("The size of this file is %d.\n", filesize);
	}

    for(i=0 ; i<5 ; i++){
        n = sendto(sockfd,&filesize,sizeof(filesize), 0,(struct sockaddr *) &server,sizeof(server));
        printf("Sending filesize\n");
        if (n < 0)
            error("Error sending\n");
    }
    pthread_mutex_lock(&lock);
    
    int fd;
    fd = open(FILENAME, O_RDONLY);
    if(fd < 0 ){
        error("Error: while opening the file\n");
    }
    
    if(fstat(fd,&st) < 0){
        error("ERROR: while file status\n");
    }
    
    data = mmap((caddr_t)0, st.st_size , PROT_READ , MAP_SHARED , fd , 0 ) ;
    
    if(data == MAP_FAILED ){
        error("ERROR : mmap \n");
    }
    datasize = st.st_size;;
    pthread_mutex_unlock(&lock);
    
    if((errno = pthread_create(&thrd, 0, missing, (void*)0 ))){
        fprintf(stderr, "pthread_create %s\n",strerror(errno));
        pthread_exit(0);
    }
    
    gettimeofday(&start, NULL);
    
    while (datasize > 0) {
        int chunk , left;
        UDP send_DG;
        memset(&send_DG , 0 , sizeof(UDP));
        
        left = datasize;
        chunk = PAYLOAD;
        
        if(left - chunk < 0){
            chunk = left;
        } else {
            left = left - chunk;
        }
        pthread_mutex_lock(&lock);
        memcpy(send_DG.buf , &data[seq*PAYLOAD] , chunk);
        pthread_mutex_unlock(&lock);
        
        send_DG.seqNum = seq;
        send_DG.dataSize = chunk;
        usleep(10);
        sendto(sockfd , &send_DG , sizeof(send_DG) , 0 , (struct sockaddr *) &server,sizeof(server));
        seq++;
        datasize -= chunk; 
    }
    
    pthread_join(thrd, 0);
        
    gettimeofday(&stop, NULL);
    totaltime = (stop.tv_sec - start.tv_sec);
    
    printf("Time taken to complete : %d sec\n",totaltime);
    munmap(data, st.st_size);
    
    fclose(fp);
    close(sockfd);
    return 0;
}
