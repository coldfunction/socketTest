#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>


#define TOTAL_LEN (64*1024)
//#define TOTAL_LEN (8*1024)
#define TOTAL_DATA_SIZE (8*1024*1024)


void *garbage_recv_func(void *data)
{
	int port_num = *(int*)data;
	port_num+=1;

	int buf_size = 65536;
//	int buf_size = 4;
//	int buf_size = 131072;
//	char buf[10];
	char *buf = malloc(buf_size);

    int sockfd = 0,forClientSockfd = 0;
	sockfd = socket(AF_INET , SOCK_STREAM , 0);

    if (sockfd == -1){
        printf("Fail to create a socket.");
    }

	//socket connect
    struct sockaddr_in serverInfo,clientInfo;
    int addrlen = sizeof(clientInfo);
    bzero(&serverInfo,sizeof(serverInfo));

    serverInfo.sin_family = PF_INET;
    serverInfo.sin_addr.s_addr = inet_addr("172.31.3.2");
    serverInfo.sin_port = htons(port_num);
    bind(sockfd,(struct sockaddr *)&serverInfo,sizeof(serverInfo));
    listen(sockfd,5);
    while(1){
        forClientSockfd = accept(sockfd,(struct sockaddr*) &clientInfo, &addrlen);
		int ret, offset = 0;
		int len = buf_size;
		while ( ret = recv(forClientSockfd,buf+offset,len,0) ) {
			offset += ret;
			len = len-ret;
			//printf("@@@@@@@@@@@@@@@!!!!!!!!!!!@@@@@@@@@@cocotion recv len = %d\n", ret);
		}
	}
}



int main(int argc , char *argv[])

{
	int port_num = atoi(argv[1]);
    //socket build
//    char inputBuffer[256] = {};
	char *inputBuffer;
	int len = TOTAL_LEN;
	inputBuffer = malloc(len);

    char message[] = {"Hi,this is server okokokok.\n"};


//	pthread_t gthread;
//	pthread_create(&gthread, NULL, garbage_recv_func, &port_num);




    int sockfd = 0,forClientSockfd = 0;
	sockfd = socket(AF_INET , SOCK_STREAM , 0);

    if (sockfd == -1){
        printf("Fail to create a socket.");
    }

    //socket connect
    struct sockaddr_in serverInfo,clientInfo;
    int addrlen = sizeof(clientInfo);
    bzero(&serverInfo,sizeof(serverInfo));

    serverInfo.sin_family = PF_INET;
    serverInfo.sin_addr.s_addr = inet_addr("172.31.3.2");
    serverInfo.sin_port = htons(port_num);
    bind(sockfd,(struct sockaddr *)&serverInfo,sizeof(serverInfo));
    listen(sockfd,5);



	//int total = TOTAL_DATA_SIZE;
	FILE * file;
    while(1){
        forClientSockfd = accept(sockfd,(struct sockaddr*) &clientInfo, &addrlen);

		int num;
		//FILE * file;
		file = fopen( "inputData.txt" , "r");
		if(!file){
			free(inputBuffer);
			close(sockfd);
			fclose(file);
			return 0;
		}
		while (fscanf(file, "%d", &num)!=EOF) {

			int totalcount = 0;

			//if(num == 0)
				//num = 1;
			if(num < 2)
				num = 2;

			int total = num;
			len = TOTAL_LEN;

			printf("this time len is num = %d ########################\n", num);


			if(total < TOTAL_LEN) {
				len = total;
			}


//			printf("cocotion  test total = %d\n", total);
			int ret, offset = 0;
			while ( ret = recv(forClientSockfd,inputBuffer+offset,len,0) ) {
				offset += ret;
				len = len - ret;
				totalcount+=ret;
				if(len == 0) {
					total-=offset;
			//		printf("cocotion test rest recv = %d\n", total);
					if(total == 0)
						break;
					else {
						offset = 0;
						len = TOTAL_LEN;
						if(total < TOTAL_LEN) {
							len = total;
						}
					}
				}
			}
			//printf("cocotion test ok I recv all\n");
//			if(num != totalcount)
//				printf("totalcount = %d num = %d\n", totalcount, num);
//			int i;
		//for(i = 0; i < TOTAL_LEN; i++) {
		//	if(inputBuffer[i] != '@' ) break;
		//}
		//if(i != TOTAL_LEN) {
		//	printf("i = %d\n", i);
		//	message[0] = '@';
		//}
		//usleep(100);
//        	send(forClientSockfd,message,sizeof(message),0);
			do {
        		ret = send(forClientSockfd,message,1,0);
			} while (ret != 1);
//        printf("Get:%s\n",inputBuffer);
//			printf("okok after send, I will go next num = %d port num = %d\n", num, port_num);
    	}
	}

//	pthread_cancel(gthread);
//	pthread_join(gthread, NULL);

	free(inputBuffer);
	close(sockfd);
    return 0;
}

