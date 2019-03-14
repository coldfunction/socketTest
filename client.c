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

	int port_num = atoi(argv[1]);

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
    info.sin_port = htons(port_num);


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
		fclose(file);
		return 0;
	}

	FILE *file2;


	file2 = fopen( "inputData_runtime.txt" , "r");
	if(!file2){
		free(buf);
		close(sockfd);
		fclose(file2);
		fclose(file);
		return 0;
	}
	int runningtime;
	fscanf(file2, "%d", &runningtime);

	int k = 0;
	while (fscanf(file, "%d", &num)!=EOF) {
		if(num == 0)
			num = 1;

		k++;
		usleep(runningtime);

		gettimeofday(&start,NULL);

		int total_times = num/TOTAL_LEN;

		srand(time(NULL));
		int a = rand()%5;

		int wantwait = 0;


		for(i = 0; i < total_times; i++) {
//
		if(rand()%2 == 0) {}
		else if(rand()%3 == 0) {
			usleep(10);
		}
		else if(rand()%4 == 0) {
			usleep(15);
		}
		else if(rand()%9 == 0) {
			usleep(20);
		}
		else if(k%11 == 0) {
			usleep(1);
		}
		else if(rand()%13 == 0) {
			usleep(80);
		}
		else {
			usleep(rand()%200);
		}

			do {

	    		int ret = send(sockfd,buf + offset, len,0);
				offset += ret;
	    		len = len - ret;

			}while (len);
			len = TOTAL_LEN;
			offset = 0;
		}


		int rest = num - total_times*TOTAL_LEN;
		if(rest) {
			do {
	    		int ret = send(sockfd,buf + offset, rest,0);
				offset += ret;
	    		rest = rest - ret;
			}while (rest);
		}

		recv(sockfd,receiveMessage,sizeof(receiveMessage),0);

		gettimeofday(&end,NULL);
		timer = 1000000 * (end.tv_sec-start.tv_sec)+ end.tv_usec-start.tv_usec;

		printf("%ld\n", num/timer);


		fscanf(file2, "%d", &runningtime);


		runningtime -= timer;
		if(runningtime <= 0)
			runningtime = 1;



	   	FILE *pFile;
   	   	char pbuf[200];
		pFile = fopen("mytransfer_rate.txt", "a");
    	if(pFile != NULL){
        	sprintf(pbuf, "%ld\n",num/timer);
        	fputs(pbuf, pFile);
    	}
    	else
        	printf("no profile\n");
    	fclose(pFile);




	}




    printf("close Socket\n");
	free(buf);
	close(sockfd);
	fclose(file);
	fclose(file2);
    return 0;
}
