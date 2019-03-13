#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define TOTAL_LEN (64*1024)
#define TOTAL_DATA_SIZE (4*1024*1024)

int main(int argc , char *argv[])

{
    //socket build
//    char inputBuffer[256] = {};
	char *inputBuffer;
	int len = TOTAL_LEN;
	inputBuffer = malloc(len);

    char message[] = {"Hi,this is server okokokok.\n"};
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
    serverInfo.sin_port = htons(8701);
    bind(sockfd,(struct sockaddr *)&serverInfo,sizeof(serverInfo));
    listen(sockfd,5);

	//int total = TOTAL_DATA_SIZE;
    while(1){
        forClientSockfd = accept(sockfd,(struct sockaddr*) &clientInfo, &addrlen);

		int num;
		FILE * file;
		file = fopen( "inputData.txt" , "r");
		if(!file){
			free(inputBuffer);
			close(sockfd);
			return 0;
		}
		while (fscanf(file, "%d", &num)!=EOF) {
			if(num == 0)
				num = 1;

			int total = num;
			len = TOTAL_LEN;

			if(total < TOTAL_LEN) {
				len = total;
			}


//			printf("cocotion  test total = %d\n", total);
			int ret, offset = 0;
			while ( ret = recv(forClientSockfd,inputBuffer+offset,len,0) ) {
				offset += ret;
				len = len - ret;
//			printf("cocotion recv len = %d\n", ret);
				if(len == 0) {
					total-=offset;
					//printf("cocotion test rest recv = %d\n", total);
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
//        printf("Get:%s\n",inputBuffer);
    	}
	}
	free(inputBuffer);
	close(sockfd);
    return 0;
}
