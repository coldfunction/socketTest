#define _GNU_SOURCE
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
#include <pthread.h>
#include <sched.h>


#define TOTAL_LEN (64*1024)
//#define TOTAL_LEN (512*1024)
#define TOTAL_DATA_SIZE (4*1024*1024)
//#define TOTAL_DATA_SIZE (64*1024*1024)
#define META_HEAD (TOTAL_DATA_SIZE/TOTAL_LEN)

static pthread_mutex_t mtx[2];
static pthread_cond_t cond[2];

//mtx[0] = PTHREAD_MUTEX_INITIALIZER;
//cond[0] = PTHREAD_COND_INITIALIZER;

//mtx[1] = PTHREAD_MUTEX_INITIALIZER;
//cond[1] = PTHREAD_COND_INITIALIZER;

static pthread_mutex_t mtx2[2];
static pthread_cond_t cond2[2];

//mtx2[0] = PTHREAD_MUTEX_INITIALIZER;
//cond2[0] = PTHREAD_COND_INITIALIZER;

//mtx2[1] = PTHREAD_MUTEX_INITIALIZER;
//cond2[1] = PTHREAD_COND_INITIALIZER;

static pthread_mutex_t mtx3;
static pthread_cond_t cond3;


int kick_trans = 0;


struct  timeval  start2;
struct  timeval  end2;



struct sendBuf
{
	int head;
	int tail;
	int bottom;
	int sockfd;
	int sockfd2;
	int kick;
	int stop_send_garbage;

	int op;

	char *buf;

};

struct sendBuf *sbuf[2];

/*
void send_garbage()
{
	pthread_t gthread;
	pthread_create(&gthread, NULL, trans_g_func, NULL);

}

void stop_send_garbage()
{


}
*/

int socket_connect(char *ip, int port_num)
{
    int sockfd = 0;
    sockfd = socket(AF_INET , SOCK_STREAM , 0);

    if (sockfd == -1){
        printf("Fail to create a socket.");
    }

    //socket connect

    struct sockaddr_in info;
    bzero(&info,sizeof(info));
    info.sin_family = PF_INET;

    info.sin_addr.s_addr = inet_addr(ip);
    info.sin_port = htons(port_num);


    int err = connect(sockfd,(struct sockaddr *)&info,sizeof(info));
    if(err==-1){
        printf("Connection error");
    }
	return sockfd;
}


void *garbage_send_func(void *data)
{
	//int sockfd = *(int*)data;
	struct sendBuf *tbuf = (struct sendBuf*)data;
	int sockfd = tbuf->sockfd2;

	int op = tbuf->op;

//	char buf[] = {"g"};
//	int buf_size = 65536;
	int buf_size = 4;
//	int buf_size = 131072;
	char *buf = malloc(buf_size);
    bzero(buf,buf_size);

	while(1) {

		do {
			pthread_mutex_lock(&mtx2[op]);
			pthread_cond_wait(&cond2[op], &mtx2[op]);
			pthread_mutex_unlock(&mtx2[op]);
		} while (tbuf->stop_send_garbage);

		int len = buf_size;
		int offset = 0;
		do {
			int ret = send(sockfd,buf + offset, len,0);
//			printf("start already send garbage len = %d\n", len);
			offset += ret;
	    	len = len - ret;
		} while (len);

	}

	printf("send okokokokokokok garbage_send_func\n");

}

void send_garbage(int op)
{
	pthread_mutex_lock(&mtx2[op]);
	sbuf[op]->stop_send_garbage = 0;
	pthread_cond_signal(&cond2[op]);
	pthread_mutex_unlock(&mtx2[op]);
}

void stop_send_garbage(int op)
{
	sbuf[op]->stop_send_garbage = 1;
}

void *trans_func(void *data)
{
//	struct sendBuf *tbuf = (struct sendBuf*)data;
//	int op = tbuf->op;

//	cpu_set_t cpuset;
//	int cpu = op%2;
//	int cpu = 7;
//	CPU_ZERO(&cpuset);
//	CPU_SET(cpu, &cpuset);
//	sched_setaffinity(0, sizeof(cpuset), &cpuset);

//	unsigned int sel = op;
/*

	int sockfd = tbuf->sockfd;
	char *buf  = tbuf->buf;
	int head, tail = 0;
	int len = TOTAL_LEN;
	int bottom = 0;
	static int clear_rest = 0;
*/
	int op = 0;
	while(1) {
//again:
		op = (op+1)%2;

		//printf("op = %d\n", op);

		char *buf  = sbuf[op]->buf;
		int sockfd = sbuf[op]->sockfd;
		int head, tail = 0;
		int len = TOTAL_LEN;
		int bottom = 0;

		char **mbuf;
		mbuf = (char**)malloc(2*sizeof(char*));
		mbuf[0] = (char*)malloc(sizeof(char));
		memset(&mbuf[0][0], 0, 1);

		pthread_mutex_lock(&mtx[op]);

		while (sbuf[op]->head == sbuf[op]->tail) {
			if(sbuf[op]->bottom) break;

//			send_garbage(op);
//			printf("after send garbage command stop garbage = %d\n", tbuf->stop_send_garbage);

//cocotion fucking
/*
			if(op == 0) {
			gettimeofday(&end2,NULL);
			unsigned int timer = 1000000 * (end2.tv_sec-start2.tv_sec)+ end2.tv_usec-start2.tv_usec;

			printf("wait op = %d, head = %d, tail = %d, time = %d\n", op, sbuf[op]->head, sbuf[op]->tail, timer);
			}
*/


			//			pthread_mutex_unlock(&mtx[op]);
//			goto again;
//
//
/*			if((sbuf[(op+1)%2]->head != sbuf[(op+1)%2]->tail) || sbuf[(op+1)%2]->bottom) {

				pthread_mutex_unlock(&mtx[op]);
				goto again;
			}
*/
/*
			pthread_mutex_lock(&mtx[(op+1)%2]);
			if((sbuf[(op+1)%2]->head != sbuf[(op+1)%2]->tail) || sbuf[(op+1)%2]->bottom) {
				pthread_mutex_unlock(&mtx[op]);

				op = (op+1)%2;
				//pthread_mutex_lock(&mtx[op]);
				sockfd = sbuf[op]->sockfd;

				break;
			}
			pthread_mutex_unlock(&mtx[(op+1)%2]);
*/
			pthread_cond_wait(&cond[op], &mtx[op]);
		}

//cocotion fucking
/*
		if(op == 0){
			gettimeofday(&end2,NULL);
			unsigned int timer = 1000000 * (end2.tv_sec-start2.tv_sec)+ end2.tv_usec-start2.tv_usec;
			if(timer > 8000) {

				printf("wait wait wait time = %d, head = %d, tail = %d\n", timer, sbuf[op]->head, sbuf[op]->tail);
			}
		}
*/
		head = sbuf[op]->head;
		tail = sbuf[op]->tail;
		bottom = sbuf[op]->bottom;

		int update_head = 0;


		if(bottom) {
			tail = bottom;
			update_head = 1;
//			printf("op = %d, bottom = %d\n", op, bottom);
		}

		pthread_mutex_unlock(&mtx[op]);

		int offset = 0;

		int num = ((tail-head) >= TOTAL_LEN) ? TOTAL_LEN : (tail-head);
		int total_times = num/TOTAL_LEN;
		int i;
/*
		if(tail == head) {
			pthread_mutex_lock(&mtx[op]);
			if(update_head) {
				sbuf[op]->bottom = 0;
				sbuf[op]->head = 0;
			}
			pthread_cond_signal(&cond[op]);
//			pthread_cond_signal(&cond3);
			pthread_mutex_unlock(&mtx[op]);
			//dododo
		}
*/
//		stop_send_garbage(op);

		for(i = 0; i < total_times; i++) {
			offset = 0;
			len+=1;
			mbuf[1] = buf+head;
			do {
//				sel++;
//				pthread_mutex_lock(&mtx3);
//				if(sel%2 == 0 && !kick_trans) {
//					kick_trans = 1;
//					printf("wait op = %d\n", op);
//					pthread_cond_wait(&cond3, &mtx3);
//				}
//				printf("send op = %d\n", op);
//				kick_trans = 0;

				//int ret = send(sockfd,buf + offset + head, len,0);
				//mbuf[1] = buf+head;
//				printf("before send content mbuf[1][0] = %c\n", mbuf[1][0]);
				int ret = send(sockfd, mbuf[0] + offset , len,0);


				printf("cocotion test op = %d, ret = %d, offset = %d len = %d\n", op, ret, offset, len);

				//				printf("send op = %d\n", op);
//				pthread_cond_signal(&cond3);
//				pthread_mutex_unlock(&mtx3);
				offset += ret;
	    		len = len - ret;
			}while (len);

			len = TOTAL_LEN;

			head += len;
		}

		offset = 0;
		int rest = num - total_times*TOTAL_LEN;

		if(rest) {
			rest+=1;
			mbuf[1] = buf+head;
			do {
//				sel++;
//				pthread_mutex_lock(&mtx3);
//				if(sel%2 == 0 && !kick_trans) {
//					kick_trans = 1;
//					pthread_cond_wait(&cond3, &mtx3);
//				}



//				kick_trans = 0;
	    		//int ret = send(sockfd,buf + offset + head, rest,0);


				//mbuf[1] = buf+head;
	    		int ret = send(sockfd, mbuf[0] + offset, rest,0);
				printf("cocotion test rest op = %d, ret = %d, offset = %d len = %d\n", op, ret, offset, rest);



//				printf("send op rest= %d\n", op);
//				pthread_cond_signal(&cond3);
//				pthread_mutex_unlock(&mtx3);
				offset += ret;
	    		rest = rest - ret;
			}while (rest);

			head+=offset;
			head-=1;
		}

		pthread_mutex_lock(&mtx[op]);
		sbuf[op]->head = head;
		pthread_mutex_unlock(&mtx[op]);

//		printf("~~~~op = %d, head= %d, tail = %d\n", op, head, sbuf[op]->tail);
/*
		pthread_mutex_lock(&mtx[op]);
		if(update_head) {
			sbuf[op]->bottom = 0;
			sbuf[op]->head = 0;
		}
		pthread_cond_signal(&cond[op]);
		pthread_cond_signal(&cond3);
		pthread_mutex_unlock(&mtx[op]);
		*/

		if(tail == head) {
			pthread_mutex_lock(&mtx[op]);
			if(update_head) {
				sbuf[op]->bottom = 0;
				sbuf[op]->head = 0;
			}
			pthread_cond_signal(&cond[op]);
//			pthread_cond_signal(&cond3);
			pthread_mutex_unlock(&mtx[op]);
			//dododo
		}

//cocotion fucking
/*

			if(op == 0) {
			gettimeofday(&end2,NULL);
			unsigned int timer = 1000000 * (end2.tv_sec-start2.tv_sec)+ end2.tv_usec-start2.tv_usec;

			printf("want go next... op = %d, head = %d, tail = %d, time = %d\n", op, sbuf[op]->head, sbuf[op]->tail, timer);
			}
*/

	}
}

void *func(void *data)
{
	int *port = (int*)data;
	int port_num = *port;
	int op = port_num%2;

/*
	cpu_set_t cpuset;
	int cpu = op;
//	int cpu = 7;
	CPU_ZERO(&cpuset);
	CPU_SET(cpu, &cpuset);
	sched_setaffinity(0, sizeof(cpuset), &cpuset);

*/



	//cocotion fucking test
	//printf("cocotion test port num = %d, op = %d\n", port_num, op);

	sbuf[op] = malloc(sizeof(struct sendBuf));
	sbuf[op]->buf = malloc(TOTAL_DATA_SIZE + META_HEAD);
	sbuf[op]->head = sbuf[op]->tail = sbuf[op]->kick = sbuf[op]->bottom = 0;

	int sockfd = socket_connect("172.31.3.2", port_num);
	int sockfd2 = socket_connect("172.31.3.2", port_num+1);

	sbuf[op]->sockfd2 = sockfd2;
	sbuf[op]->stop_send_garbage = 1;

	sbuf[op]->op = op;

	pthread_t gthread;
	pthread_create(&gthread, NULL, garbage_send_func, sbuf[op]);

    //Send a message to server
	//

	int len = TOTAL_LEN;
	char *buf;
	//buf = malloc(len);
	buf = sbuf[op]->buf;
	int i;
	for(i = 0; i < TOTAL_DATA_SIZE; i++)
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

	sbuf[op]->sockfd = sockfd;


	pthread_t trans_thread;
	if(op == 0) {

		//pthread_attr_t tattr;
		//int policy;
		//int ret;

		//pthread_attr_init(&tattr);

		//ret = pthread_attr_setschedpolicy(&tattr, SCHED_RR);


	//	pthread_t trans_thread;
//	pthread_create(&trans_thread, &tattr, trans_func, sbuf[op]);
		pthread_create(&trans_thread, NULL, trans_func, sbuf[op]);

	}

	int k = 0;
	while (fscanf(file, "%d", &num)!=EOF) {

		if(num == 0)
			num = 1;

		k++;
		usleep(runningtime);

		gettimeofday(&start,NULL);
		if(op == 0) {
			gettimeofday(&start2,NULL);
		}

		int total_times = num/TOTAL_LEN;

		srand(time(NULL));
		int a = rand()%5;

		int wantwait = 0;


		for(i = 0; i < total_times; i++) {

			offset = 0;

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

//cocotion fucking test
/*
			if(op == 0){
				gettimeofday(&end2,NULL);
				unsigned int timer = 1000000 * (end2.tv_sec-start2.tv_sec)+ end2.tv_usec-start2.tv_usec;
				printf("********************now already pass %d\n", timer);
			}
*/

			int start = sbuf[op]->tail;

			len = TOTAL_LEN;


			pthread_mutex_lock(&mtx[op]);
			if(start < sbuf[op]->head) {
				while(start+len > sbuf[op]->head) {
//					printf("@@@@@@@@@@@@@ wait op = %d\n", op);
					pthread_cond_wait(&cond[op], &mtx[op]);
					break;
				}
			}
			pthread_mutex_unlock(&mtx[op]);

			start += len;


			pthread_mutex_lock(&mtx[op]);

			if(start + TOTAL_LEN > TOTAL_DATA_SIZE) {
				sbuf[op]->bottom = start;
				sbuf[op]->tail = 0;
			}
			else
				sbuf[op]->tail = start;

//			printf("@@@@@@@@@@@@@ op = %d, tail = %d, head = %d\n", op, sbuf[op]->tail, sbuf[op]->head);
			pthread_cond_signal(&cond[op]);
			pthread_mutex_unlock(&mtx[op]);
		}

		offset = 0;
		int rest = num - total_times*TOTAL_LEN;
		if(rest) {


			int start = sbuf[op]->tail;


			pthread_mutex_lock(&mtx[op]);
			if(start < sbuf[op]->head) {
				while(start+rest > sbuf[op]->head) {
//					printf("@@@@@@@@@@@@@@ wait op = %d\n", op);
					pthread_cond_wait(&cond[op], &mtx[op]);
					break;
				}
			}
			pthread_mutex_unlock(&mtx[op]);

			start += rest;
			pthread_mutex_lock(&mtx[op]);

			if(start + TOTAL_LEN > TOTAL_DATA_SIZE) {
				sbuf[op]->bottom = start;
				sbuf[op]->tail = 0;
			}
			else
				sbuf[op]->tail = start;

//			printf("@@@@@@@@@@@@@ rest op = %d, tail = %d, head = %d\n", op, sbuf[op]->tail, sbuf[op]->head);
			pthread_cond_signal(&cond[op]);
			pthread_mutex_unlock(&mtx[op]);

		}
//		printf("okok I recv now\n");
/*
		pthread_mutex_lock(&mtx[op]);
		kick_trans = 1;
		pthread_cond_signal(&cond[op]);
		pthread_mutex_unlock(&mtx[op]);
*/
//cocotion fucking test
//		if(op == 0)
//			printf("okok I recv now head = %d, tail = %d\n", sbuf[0]->head, sbuf[0]->tail);
		recv(sockfd,receiveMessage,sizeof(receiveMessage),0);

		gettimeofday(&end,NULL);
		timer = 1000000 * (end.tv_sec-start.tv_sec)+ end.tv_usec-start.tv_usec;


		/*
//cocotion fucking test
		if(op%2 == 0) {
			//printf("%ld\n", num/timer);
			if(num/timer < 100) {
				printf("start====#############===============\n");
				printf("%ld\n", num/timer);
				printf("num = %d\n", num);
				printf("%ld\n", timer);
				printf("end======############=============\n");
			}
			else {
				printf("start@@@===================\n");
				printf("%ld\n", num/timer);
				printf("num = %d\n", num);
				printf("%ld\n", timer);
				printf("end====@@@@===============\n");
			}
		}
*/

		fscanf(file2, "%d", &runningtime);


		runningtime -= timer;
		if(runningtime <= 0)
			runningtime = 1;


		if(op%2 == 0) {
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
		if(op%2 == 1) {
	   		FILE *pFile;
   	   		char pbuf[200];
			pFile = fopen("mytransfer_rate2.txt", "a");
    		if(pFile != NULL){
        		sprintf(pbuf, "%ld\n",num/timer);
        		fputs(pbuf, pFile);
    		}
    		else
        		printf("no profile\n");
    		fclose(pFile);
		}
	}

	if(op == 0) {
		pthread_cancel(trans_thread);
		pthread_join(trans_thread, NULL);
	}

	pthread_cancel(gthread);
	pthread_join(gthread, NULL);

    printf("close Socket\n");
	free(buf);
	free(sbuf[op]);
	close(sockfd);
	close(sockfd2);
	fclose(file);
	fclose(file2);

}



int main(int argc , char *argv[])
{

	pthread_mutex_init(&mtx[0], NULL);
	pthread_cond_init(&cond[0], NULL);

	pthread_mutex_init(&mtx[1], NULL);
	pthread_cond_init(&cond[1], NULL);

	pthread_mutex_init(&mtx2[0], NULL);
	pthread_cond_init(&cond2[0], NULL);

	pthread_mutex_init(&mtx2[1], NULL);
	pthread_cond_init(&cond2[1], NULL);

	pthread_mutex_init(&mtx3, NULL);
	pthread_cond_init(&cond3, NULL);



	int port_num = atoi(argv[1]);

/*
	pthread_attr_t tattr1, tattr2;
	int ret;
	pthread_attr_init(&tattr1);
	pthread_attr_init(&tattr2);
	ret = pthread_attr_setschedpolicy(&tattr1, SCHED_FIFO);
	ret = pthread_attr_setschedpolicy(&tattr2, SCHED_FIFO);
*/


	pthread_t appThread[2];

	//pthread_create(&appThread[0], &tattr1, func, &port_num);
	pthread_create(&appThread[0], NULL, func, &port_num);
	int port_num2 = port_num+11;

	pthread_create(&appThread[1], NULL, func, &port_num2);




//	pthread_cancel(appThread[0]);
//	pthread_cancel(appThread[1]);
	pthread_join(appThread[0], NULL);
	pthread_join(appThread[1], NULL);

//////////////////////////////
/*
	sbuf = malloc(sizeof(struct sendBuf));


	sbuf->buf = malloc(TOTAL_DATA_SIZE);
	sbuf->head = sbuf->tail = sbuf->kick = sbuf->bottom = 0;


	int sockfd = socket_connect("172.31.3.2", port_num);
	int sockfd2 = socket_connect("172.31.3.2", port_num+1);

	sbuf->sockfd2 = sockfd2;
	sbuf->stop_send_garbage = 1;

	pthread_t gthread;
	pthread_create(&gthread, NULL, garbage_send_func, sbuf);


    //Send a message to server
	//

	int len = TOTAL_LEN;
	char *buf;
	//buf = malloc(len);
	buf = sbuf->buf;
	int i;
	for(i = 0; i < TOTAL_DATA_SIZE; i++)
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


	sbuf->sockfd = sockfd;

	pthread_t trans_thread;
	pthread_create(&trans_thread, NULL, trans_func, sbuf);



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

			offset = 0;

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


			int start = sbuf->tail;

			len = TOTAL_LEN;


			pthread_mutex_lock(&mtx);
			if(start < sbuf->head) {
				while(start+len > sbuf->head) {
					pthread_cond_wait(&cond, &mtx);
					break;
				}
			}
			pthread_mutex_unlock(&mtx);

			start += len;


			pthread_mutex_lock(&mtx);

			if(start + TOTAL_LEN > TOTAL_DATA_SIZE) {
				sbuf->bottom = start;
				sbuf->tail = 0;
			}
			else
				sbuf->tail = start;

			pthread_cond_signal(&cond);
			pthread_mutex_unlock(&mtx);
		}

		offset = 0;
		int rest = num - total_times*TOTAL_LEN;
		if(rest) {


			int start = sbuf->tail;


			pthread_mutex_lock(&mtx);
			if(start < sbuf->head) {
				while(start+rest > sbuf->head) {
					pthread_cond_wait(&cond, &mtx);
					break;
				}
			}
			pthread_mutex_unlock(&mtx);

			start += rest;
			pthread_mutex_lock(&mtx);

			if(start + TOTAL_LEN > TOTAL_DATA_SIZE) {
				sbuf->bottom = start;
				sbuf->tail = 0;
			}
			else
				sbuf->tail = start;

			pthread_cond_signal(&cond);
			pthread_mutex_unlock(&mtx);

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

	pthread_cancel(trans_thread);
	pthread_join(trans_thread, NULL);

	pthread_cancel(gthread);
	pthread_join(gthread, NULL);

    printf("close Socket\n");
	free(buf);
	free(sbuf);
	close(sockfd);
	close(sockfd2);
	fclose(file);
	fclose(file2);
	*/
    return 0;
}
