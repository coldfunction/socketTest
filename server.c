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
//#define TOTAL_LEN (512*1024)
#define TOTAL_DATA_SIZE (4*1024*1024)
//#define TOTAL_DATA_SIZE (64*1024*1024)
#define META_HEAD (32)


void *garbage_recv_func(void *data)
{
	int port_num = *(int*)data;
	port_num+=1;

//	int buf_size = 65536;
	int buf_size = 4;
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
	inputBuffer = malloc(len+1);

    char message[] = {"Hi,this is server okokokok.\n"};


	pthread_t gthread;
	pthread_create(&gthread, NULL, garbage_recv_func, &port_num);




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


//	char **mbuf;
//	mbuf = (char**)malloc(2*sizeof(char*));
//	mbuf[0] = (char*)malloc(sizeof(char));
//	memset(&mbuf[0][0], 0, 1);

//	mbuf[1] = (char*)malloc(sizeof(char)*len);
//	mbuf[1][0] = 'X';
// 	printf("@Get:%c\n",mbuf[1][0]);
	memset(&inputBuffer[0], 0, 1);

	//int total = TOTAL_DATA_SIZE;
	int garbage = 0;
	int pre_len;
	FILE * file;
	int head = 1;
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
			int ret, offset = 0;

/*			if(garbage)	 {
				num = pre_len;
				garbage = 0;
//				goto again;
			}*/
			if(num < 2)
				num = 2;

			int total = num;



			len = TOTAL_LEN;

			printf("this time len is num = %d ########################\n", num);


			if(total < TOTAL_LEN) {
				len = total;
			}


//			printf("cocotion  test total = %d\n", total);
		//	int ret, offset = 0;
			//while ( ret = recv(forClientSockfd,inputBuffer+offset,len,0) ) {

			//int garbage = 0;

			//len+=1;
//again:
			//mbuf[1] = inputBuffer;
			garbage = 0;
			head = 1;
			while (1){
				//mbuf[1] = inputBuffer+offset;
				//ret = recv(forClientSockfd,inputBuffer+offset,len,0);
				//mbuf[1] = inputBuffer+offset;
//				printf("cocotion test offset = %d, len = %d\n", offset, len);
				//ret = recv(forClientSockfd,mbuf+offset,len,0);

				if(head) {
//					ret = recv(forClientSockfd,inputBuffer,1,0);
					ret = recv(forClientSockfd,inputBuffer,1,0);
					printf("cocotion first head recv len = %d\n", ret);
					head = 0;
					if(ret == 0) break;


					if(ret == -1) {
						printf("@@@error error recv cocotion test offset = %d\n", offset);
						exit(1);
					}
					if(inputBuffer[0]) {
						printf("!!!!!!Garbage packet!!!!!Please drop it!!!!!\n");
						garbage = 1;
					//	pre_len = num;
					//total++;
//					memset(&mbuf[0][0], 0, 1);
						memset(&inputBuffer[0], 0, 1);
						//exit(1);
					}
					if(garbage) {
						printf("!!!!!!Garbage packet!!!!!Please drop it!!!!!\n");
						int glen = TOTAL_LEN;
						int goffset = 0;
						do {
							ret = recv(forClientSockfd,inputBuffer+1+goffset,glen,0);
							glen -= ret;
							goffset += ret;
						} while (glen);
						//len = total = TOTAL_LEN;
						head = 1;
						garbage = 0;
        				send(forClientSockfd,message,sizeof(message),0);
						continue;
					}
				}



			//	ret = recv(forClientSockfd,inputBuffer+offset,len,0);
//				printf("cocotion recv len = %d\n", ret);
 			    //printf("Get:%c\n",mbuf[1][0]);

				//if(mbuf[0][0]) {
				ret = recv(forClientSockfd,inputBuffer+1+offset,len,0);
				//if(ret == 0) break;



				offset += ret;
				len = len - ret;

				printf("cocotion total = %d\n", total);
				printf("cocotion offset = %d\n", offset);

				printf("cocotion recv len = %d\n", ret);
				printf("cocotion rest len = %d\n", len);

				if(len == 0) {
					total-=offset;
					//total+=1;
					//total+=garbage;
					printf("cocotion test rest total = %d\n", total);
					if(total == 0)
						break;
					else {
						head = 1;
						garbage = 0;
						offset = 0;
						len = TOTAL_LEN;
						if(total < TOTAL_LEN) {
							len = total;
						}
						//len = len + 1;
					}
				}
			}
			printf("cocotion test ok I recv all\n");

			//if(garbage) continue;
//			int i;
		//for(i = 0; i < TOTAL_LEN; i++) {
		//	if(inputBuffer[i] != '@' ) break;
		//}
		//if(i != TOTAL_LEN) {
		//	printf("i = %d\n", i);
		//	message[0] = '@';
		//}
		//usleep(100);
        	send(forClientSockfd,message,sizeof(message),0);
			printf("cocotion test ok go next\n");
        //printf("Get:%s\n",inputBuffer);
//        printf("Get:%s\n",mbuf[1]);
 //       printf("Get:%c\n",mbuf[1][0]);
    	}
	}

	pthread_cancel(gthread);
	pthread_join(gthread, NULL);

	free(inputBuffer);
	close(sockfd);
    return 0;
}
