#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>

#define TOTAL_LEN (64*1024)
#define TOTAL_DATA_SIZE (4*1024*1024)


int main(int argc , char *argv[])
{

	//socket build
    int sockfd = 0;
	int len = TOTAL_LEN;
    sockfd = socket(AF_INET , SOCK_STREAM , 0);

    if (sockfd == -1){
        printf("Fail to create a socket.");
    }

    //socket connect

    struct sockaddr_in info;
    bzero(&info,sizeof(info));
    info.sin_family = PF_INET;

    //localhost test
    info.sin_addr.s_addr = inet_addr("172.31.3.2");
    info.sin_port = htons(8701);


    int err = connect(sockfd,(struct sockaddr *)&info,sizeof(info));
    if(err==-1){
        printf("Connection error");
    }


    //Send a message to server
	//

	char *buf;
	buf = malloc(len);
	int i;
	for(i = 0; i < len; i++)
		buf[i] = '@';

//    char message[] = {"Hi there"};
    char receiveMessage[100] = {};
	int offset = 0;

	struct  timeval  start;
	struct  timeval  end;
	unsigned long timer;


	int num;
	FILE * file;
	file = fopen( "inputData.txt" , "r");
	if(!file){
		free(buf);
		close(sockfd);
		return 0;
	}



	while (fscanf(file, "%d", &num)!=EOF) {
		if(num == 0)
			num = 1;

		srand(time(NULL));
		int a = rand()%5000;

		//5000~6000 wait
//		usleep(8000);
		usleep(4000+a);
//		usleep(9000-num/1000);


		gettimeofday(&start,NULL);

		int total_times = num/TOTAL_LEN;

		for(i = 0; i < total_times; i++) {
		//for(i = 0; i < TOTAL_DATA_SIZE/TOTAL_LEN; i++) {
//			int wantwait = num/1000/total_times - rand()%70;
			int wantwait = 400/total_times;
			if(wantwait <= 0) wantwait = 10;
			usleep(wantwait);
			do {
				//usleep(50);

	    		int ret = send(sockfd,buf + offset, len,0);
				offset += ret;
	    		len = len - ret;
//			printf("send len = %d\n", ret);
//			printf("rest len = %d\n", len);

			}while (len);
//			printf("i = %d times\n", i);
			len = TOTAL_LEN;
			offset = 0;
		}


		int rest = num - total_times*TOTAL_LEN;
//		printf("cocotion test fucking rest = %d\n", rest);
		if(rest) {
			do {
	    		int ret = send(sockfd,buf + offset, rest,0);
				offset += ret;
	    		rest = rest - ret;
//				printf("cocotion testing fucking still rest = %d\n", rest);


			}while (rest);
		}

//		printf("ok wait ack@@!!!!\n");

		recv(sockfd,receiveMessage,sizeof(receiveMessage),0);

		gettimeofday(&end,NULL);
		timer = 1000000 * (end.tv_sec-start.tv_sec)+ end.tv_usec-start.tv_usec;


    	//printf("%s",receiveMessage);
    	//printf("close Socket\n");
//    	printf("timer = %ld us\n",timer);

		//printf("average trans rate = %ld\n", num/timer);
		printf("%ld\n", num/timer);



	}




    printf("close Socket\n");
	free(buf);
	close(sockfd);
    return 0;
}
