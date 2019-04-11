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

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


//#define DEBUG 1


#define TOTAL_LEN (64*1024)
//#define TOTAL_LEN (512*1024)
#define TOTAL_DATA_SIZE (4*1024*1024)
//#define TOTAL_DATA_SIZE (64*1024*1024)
#define META_HEAD (TOTAL_DATA_SIZE/TOTAL_LEN)

pthread_t mythread;

static pthread_mutex_t mtx[2];
static pthread_cond_t cond[2];

static pthread_mutex_t mtx2[2];
static pthread_cond_t cond2[2];


static pthread_mutex_t mtx3;
static pthread_cond_t cond3;

static pthread_mutex_t mtx4[2];
static pthread_cond_t cond4[2];


static pthread_barrier_t barr;


int kick_trans = 1;
int op_switch = 0;


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
	int op_switch;
    int fd;

	char *buf;

};

struct sendBuf *sbuf[2];


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

void garbage_send_func2(void *data) {
	struct sendBuf *tbuf = (struct sendBuf*)data;
	int sockfd = tbuf->sockfd;

	int op = tbuf->op;
#ifdef DEBUG
    printf("wait op = %d, sockfd = %d\n", op, sockfd);
#endif
//	int buf_size = 65537;
	int buf_size = 2;
	char *buf = malloc(buf_size);
	memset(&buf[0], 1, 1);

	int len = buf_size-1;
	int offset = 0;

    do {
		int ret = 0;
		if(offset == 0) {
			do {
				ret = send(sockfd,buf, 1,0);
			} while (ret != 1);
			offset += 1;

			int sublen = 4;
			int myoffset = 0;
			int plen;
			plen = len;
			do {
				ret = send(sockfd, (char*)&plen+myoffset, sublen, 0);
				sublen -= ret;
				myoffset += ret;
			} while (sublen);
#ifdef DEBUG
			printf("####op = %d, ret = %d, already send num plen = %d\n", op, ret, plen);
#endif
		}
		ret = send(sockfd,buf + offset-1 , len,0);

		offset += ret;
	    len = len - ret;
#ifdef DEBUG
		printf("op = %d, start already send garbage len = %d sockfd = %d\n", op, offset, sockfd);
		printf("op = %d, start rest send garbage len = %d\n", op, len);
#endif
	} while (len);
	free(buf);
}

void *garbage_send_func(void *data)
{
	struct sendBuf *tbuf = (struct sendBuf*)data;
	int sockfd = tbuf->sockfd;

	int op = tbuf->op;
#ifdef DEBUG
    printf("wait op = %d, sockfd = %d\n", op, sockfd);
#endif
	int buf_size = 65537;
//	int buf_size = 2;
	char *buf = malloc(buf_size);
	memset(&buf[0], 1, 1);

	while(1) {

		pthread_mutex_lock(&mtx2[op]);
		do {
#ifdef DEBUG
            printf("op = %d, wait to send garbage!!!!\n", op);
#endif
			pthread_mutex_lock(&mtx4[op]);
			tbuf->kick = 0;
			pthread_cond_signal(&cond4[op]);
			pthread_mutex_unlock(&mtx4[op]);
			pthread_cond_wait(&cond2[op], &mtx2[op]);
			pthread_mutex_lock(&mtx4[op]);
			tbuf->kick = 1;
			pthread_mutex_unlock(&mtx4[op]);
			//pthread_mutex_unlock(&mtx2[op]);
		} while (tbuf->stop_send_garbage);
		pthread_mutex_unlock(&mtx2[op]);

while (tbuf->stop_send_garbage == 0) {
		int len = buf_size-1;
		int offset = 0;

        do {
			int ret = 0;
			if(offset == 0) {
				do {
					ret = send(sockfd,buf, 1,0);
				} while (ret != 1);
				offset += 1;

				int sublen = 4;
				int myoffset = 0;
				int *plen;
				plen = &len;
				do {
					ret = send(sockfd, (char*)plen+myoffset, sublen, 0);
					sublen -= ret;
					myoffset += ret;
				} while (sublen);
			}
			ret = send(sockfd,buf + offset -1 , len,0);

			offset += ret;
	    	len = len - ret;
#ifdef DEBUG
			printf("op = %d, start already send garbage len = %d sockfd = %d\n", op, offset, sockfd);
			printf("op = %d, start rest send garbage len = %d\n", op, len);
#endif
		} while (len);

//		usleep(100);
}

#ifdef DEBUG
		printf("after op = %d !!!!!!!!!!!!!\n", op);
#endif
	}

	printf("send okokokokokokok garbage_send_func\n");

}

void send_garbage(int op)
{
#ifdef DEBUG
    printf("try to send garbage!!!!\n");
#endif
	pthread_mutex_lock(&mtx2[op]);
	sbuf[op]->stop_send_garbage = 0;
	pthread_cond_signal(&cond2[op]);
	pthread_mutex_unlock(&mtx2[op]);



}

void stop_send_garbage(int op)
{
	pthread_mutex_lock(&mtx2[op]);
	sbuf[op]->stop_send_garbage = 1;
	pthread_mutex_unlock(&mtx2[op]);

#ifdef DEBUG
	printf("in stop_send_garbage now op = %d before recv\n", op);
#endif
	pthread_mutex_lock(&mtx4[op]);

	while(sbuf[op]->kick) {
		pthread_cond_wait(&cond4[op], &mtx4[op]);
	}
	pthread_mutex_unlock(&mtx4[op]);

#ifdef DEBUG
	int ret = 0;
	printf("in stop_send_garbage now op = %d after recv and recv %d bytes\n", op, ret);
#endif
}

void *trans_func(void *data)
{
	/*
	cpu_set_t cpuset;
//	int cpu = op;
	int cpu = 7;
	CPU_ZERO(&cpuset);
	CPU_SET(cpu, &cpuset);
	sched_setaffinity(0, sizeof(cpuset), &cpuset);
*/

	//int op = 0;
	int op = *(int*)data;
	while(1) {


//		send_garbage(op); //cocotion test

//		op = (op+1)%2;

		char *buf  = sbuf[op]->buf;
		int sockfd = sbuf[op]->sockfd;
		int head, tail = 0;
		int len = TOTAL_LEN;
		int bottom = 0;

		pthread_mutex_lock(&mtx[op]);
		while (sbuf[op]->head == sbuf[op]->tail) {
			if(sbuf[op]->bottom) break;
#ifdef DEBUG
			printf("wait op = %d, head = %d, tail = %d\n", op, sbuf[op]->head, sbuf[op]->tail);
#endif

/*
			if(sbuf[op]->op_switch) {
				pthread_mutex_unlock(&mtx[op]);
				op = (op+1)%2;
//				stop_send_garbage(op); //cocotion test
				sockfd = sbuf[op]->sockfd;
				buf  = sbuf[op]->buf;
				pthread_mutex_lock(&mtx[op]);
				continue;

			}
*/

			send_garbage(op);
			//garbage_send_func(sbuf[op]);
//			garbage_send_func2(sbuf[op]);
/*
			pthread_mutex_unlock(&mtx[op]);
			garbage_send_func2(sbuf[op]);
			usleep(1);

			op = (op+1)%2;
			sockfd = sbuf[op]->sockfd;
			buf  = sbuf[op]->buf;
			pthread_mutex_lock(&mtx[op]);
			//usleep(50);
			continue;
*/

			pthread_cond_wait(&cond[op], &mtx[op]);
			stop_send_garbage(op);
		//	pthread_mutex_unlock(&mtx[op]);
		//	pthread_mutex_lock(&mtx[op]);


#ifdef DEBUG
			printf("after wait op = %d, head = %d, tail = %d\n", op, sbuf[op]->head, sbuf[op]->tail);
#endif

		}

		head = sbuf[op]->head;
		tail = sbuf[op]->tail;
		bottom = sbuf[op]->bottom;

		int update_head = 0;


		if(bottom) {
			tail = bottom;
			update_head = 1;
#ifdef DEBUG
			printf("op = %d, bottom = %d\n", op, bottom);
#endif
		}

		pthread_mutex_unlock(&mtx[op]);

		int offset = 0;

		//int num = ((tail-head) >= TOTAL_LEN) ? TOTAL_LEN : (tail-head);
		int num = tail-head;
		int total_times = num/TOTAL_LEN;
		int i;

		char mmbuf;
    	memset(&mmbuf, 0, 1);

		for(i = 0; i < total_times; i++) {
			offset = 0;
			do {
				int ret = 0;
                if(offset == 0) {
					do {
				    	ret = send(sockfd, &mmbuf, 1,0);
					} while (ret != 1);
					offset +=1;

					int sublen = 4;
					int myoffset = 0;
					int *plen;
					plen = &len;
					do {
						ret = send(sockfd, (char*)plen+myoffset, sublen, 0);
						sublen -= ret;
						myoffset += ret;
					} while (sublen) ;
#ifdef DEBUG
					printf("cocotion op = %d, send len = %d, ret = %d\n",op,  len, ret);
#endif
                }
				ret = send(sockfd, buf+head + offset -1 , len,0);

#ifdef DEBUG

			printf("       cocotion test op = %d, ret = %d, offset = %d len = %d\n", op, ret, offset, len);
#endif
				offset += ret;
	    		len = len - ret;
			}while (len);

			len = TOTAL_LEN;

			head += len;
		}

		offset = 0;
		int rest = num - total_times*TOTAL_LEN;

    	memset(&mmbuf, 0, 1);

        if(rest) {
			do {
				int ret = 0;
                if(offset == 0) {
					do {
				    	ret = send(sockfd, &mmbuf, 1,0);
					} while (ret != 1);
					offset += 1;
#ifdef DEBUG
					printf("cocotion rest = %d send\n", rest);
#endif
					int sublen = 4;
					int myoffset = 0;
					int *plen;
					plen = &rest;

					do {
						ret = send(sockfd, (char*)plen+myoffset, sublen, 0);
						sublen -= ret;
						myoffset += ret;
					} while (sublen) ;

#ifdef DEBUG
					printf("cocotion op = %d, send rest len = %d, ret = %d\n", op, rest, ret);
#endif
				}
				ret = send(sockfd, buf+head + offset -1 ,rest,0);
				if(ret == -1)
					exit(1);
#ifdef DEBUG

                printf("        cocotion test rest op = %d, ret = %d, offset = %d len = %d\n", op, ret, offset, rest);
#endif
				offset += ret;
	    		rest = rest - ret;
			}while (rest);

			head = head + offset-1;
		}

        pthread_mutex_lock(&mtx[op]);
		sbuf[op]->head = head;
		pthread_mutex_unlock(&mtx[op]);

#ifdef DEBUG
		printf("~~~~op = %d, head= %d, tail = %d\n", op, head, sbuf[op]->tail);
#endif
		if(tail == head) {
			pthread_mutex_lock(&mtx[op]);
			if(update_head) {
				sbuf[op]->bottom = 0;
				sbuf[op]->head = 0;
			}
			pthread_cond_signal(&cond[op]);
			pthread_mutex_unlock(&mtx[op]);
		}
//		stop_send_garbage((op+1)%2); //cocotion test

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

	sbuf[op] = malloc(sizeof(struct sendBuf));
	sbuf[op]->buf = malloc(TOTAL_DATA_SIZE + META_HEAD);
	sbuf[op]->head = sbuf[op]->tail = sbuf[op]->kick = sbuf[op]->bottom = 0;

	sbuf[op]->op_switch = 0;

	int sockfd = socket_connect("172.31.3.2", port_num);
	int sockfd2 = socket_connect("172.31.3.2", port_num+1);

	sbuf[op]->sockfd2 = sockfd2;
	sbuf[op]->stop_send_garbage = 1;

	sbuf[op]->op = op;
	sbuf[op]->sockfd = sockfd;

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

//	sbuf[op]->sockfd = sockfd;


	pthread_t trans_thread;
//	if(op == 0) {

		//pthread_attr_t tattr;
		//int policy;
		//int ret;

		//pthread_attr_init(&tattr);

		//ret = pthread_attr_setschedpolicy(&tattr, SCHED_RR);


	//	pthread_t trans_thread;
//	pthread_create(&trans_thread, &tattr, trans_func, sbuf[op]);
//		pthread_create(&trans_thread, NULL, trans_func, sbuf[op]);
		pthread_create(&trans_thread, NULL, trans_func, &op);
//		pthread_create(&trans_thread, NULL, trans_func, &fd);

//	}

	mythread = trans_thread;

	int k = 0;
	while (fscanf(file, "%d", &num)!=EOF) {

		int mynumcount = 0;


		if(num < 2)
			num = 2;

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

#ifdef DEBUG
			printf("@@@@ op = %d, fucking num = %d\n", op, num);
#endif

			len = TOTAL_LEN;

			int tail;
			pthread_mutex_lock(&mtx[op]);
			tail = sbuf[op]->tail;
			if(tail < sbuf[op]->head) {
				while(tail+len >= sbuf[op]->head) {
#ifdef DEBUG
					printf("@@@@@@@@@@@@@ wait op = %d\n", op);
#endif
					pthread_cond_signal(&cond[op]);
					//pthread_cond_signal(&cond[(op+1)%2]);
					pthread_cond_wait(&cond[op], &mtx[op]);
					break;
				}
			}
			pthread_mutex_unlock(&mtx[op]);

			pthread_mutex_lock(&mtx[op]);
			tail = sbuf[op]->tail;

			if(tail + TOTAL_LEN >= TOTAL_DATA_SIZE) {
				sbuf[op]->bottom = tail;
				sbuf[op]->tail = 0;
				pthread_cond_signal(&cond[op]);

#ifdef DEBUG
				printf("op#####= %d, total_len#####, bottom = %d\n", op, tail);
#endif
			}

#ifdef DEBUG
			printf("@@@@@@@@@@@@@ op = %d, tail = %d, head = %d\n", op, sbuf[op]->tail, sbuf[op]->head);
#endif

			tail = sbuf[op]->tail;
			tail += len;
			sbuf[op]->tail = tail;
			pthread_cond_signal(&cond[op]);
			pthread_mutex_unlock(&mtx[op]);


			mynumcount+=len;
#ifdef DEBUG
			printf("@@@@@@ now op = %d, total produce num = %d\n", op, mynumcount);
#endif
		}

		offset = 0;
		int rest = num - total_times*TOTAL_LEN;
		if(rest) {
			int tail;
			pthread_mutex_lock(&mtx[op]);
			tail = sbuf[op]->tail;
			if(tail < sbuf[op]->head) {
				while(tail+rest >= sbuf[op]->head) {
#ifdef DEBUG
					printf("@@@@@@@@@@@@@ wait op = %d\n", op);
#endif
					pthread_cond_signal(&cond[op]);
					//pthread_cond_signal(&cond[(op+1)%2]);
					pthread_cond_wait(&cond[op], &mtx[op]);
					break;
				}
			}
			pthread_mutex_unlock(&mtx[op]);

			pthread_mutex_lock(&mtx[op]);
			tail = sbuf[op]->tail;
			if(tail + rest >= TOTAL_DATA_SIZE) {
				sbuf[op]->bottom = tail;
				sbuf[op]->tail = 0;
				pthread_cond_signal(&cond[op]);

#ifdef DEBUG
				printf("op#####= %d, rest##### = %d bottom = %d\n", op, rest, tail);
#endif
			}
#ifdef DEBUG
			printf("rest@@@@@@@@@@@@@ op = %d, tail = %d, head = %d\n", op, sbuf[op]->tail, sbuf[op]->head);
#endif
			tail = sbuf[op]->tail;
			tail += rest;
			sbuf[op]->tail = tail;
			pthread_cond_signal(&cond[op]);
			pthread_mutex_unlock(&mtx[op]);


			mynumcount+=rest;
#ifdef DEBUG
			printf("@@@@@@ now op = %d, total produce num = %d\n", op, mynumcount);
#endif
		}

#ifdef DEBUG
			printf("okok I recv now op = %d, head = %d, tail = %d, num = %d\n", op, sbuf[op]->head, sbuf[op]->tail, num);
#endif
		int ret;
		do {
			ret = recv(sockfd,receiveMessage,1,0);
		} while(ret != 1);

		gettimeofday(&end,NULL);
		timer = 1000000 * (end.tv_sec-start.tv_sec)+ end.tv_usec-start.tv_usec;

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
/*
	pthread_mutex_lock(&mtx[op]);
	sbuf[op]->op_switch = 1;
	printf("op = %d, send signal to QQQQ op_switch\n", op);
	pthread_cond_signal(&cond[op]);
	pthread_mutex_unlock(&mtx[op]);
*/

	pthread_barrier_wait(&barr);

	pthread_cancel(gthread);
	pthread_join(gthread, NULL);

	pthread_cancel(trans_thread);
	pthread_join(trans_thread, NULL);

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

	pthread_mutex_init(&mtx4[0], NULL);
	pthread_cond_init(&cond4[0], NULL);
	pthread_mutex_init(&mtx4[1], NULL);
	pthread_cond_init(&cond4[1], NULL);


	int port_num = atoi(argv[1]);


	pthread_attr_t tattr1, tattr2;
	int ret;
	pthread_attr_init(&tattr1);
	pthread_attr_init(&tattr2);
	ret = pthread_attr_setschedpolicy(&tattr1, SCHED_RR);
	ret = pthread_attr_setschedpolicy(&tattr2, SCHED_RR);


	pthread_barrier_init(&barr, NULL, 2);



	pthread_t appThread[2];

	//pthread_create(&appThread[0], &tattr1, func, &port_num);
	pthread_create(&appThread[0], &tattr1, func, &port_num);
	int port_num2 = port_num+11;

	pthread_create(&appThread[1], &tattr2, func, &port_num2);


	pthread_join(appThread[0], NULL);
	pthread_join(appThread[1], NULL);


	//pthread_cancel(mythread);
	//pthread_join(mythread, NULL);


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
