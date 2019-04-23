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
#define TOTAL_LEN2 (64*1024)
//#define TOTAL_LEN (512*1024)
//#define TOTAL_DATA_SIZE (4*1024*1024)
#define TOTAL_DATA_SIZE (128*1024*1024)
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


unsigned int totalcount = 0;
unsigned int totalwait = 0;


//int MAXNUM = 5000000;
#define MAXNUM 3500000
//int MAXNUM = 100;
unsigned int cycle = 0;


struct sendBuf
{
	volatile int head;
	volatile int tail;
//	int head;
//	int tail;
	int bottom;
	int sockfd;
	int sockfd2;
	int kick;
	volatile int stop_send_garbage;

	int op;
	int op_switch;
    int fd;

	int is_last_garbage;

	unsigned int shit;
	unsigned int good;


//	pthread_t producer_t;
	pid_t pid;
	int gonext;
	int wait_times;

	volatile int real_trans_bytes;
	int real_num;

	int garbage_num;
	int garbage_head;
	int garbage_tail;
	int garbage_state;


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
	int op = *(int*) data;
	struct sendBuf *tbuf = sbuf[op];

	//struct sendBuf *tbuf = (struct sendBuf*)data;
	int sockfd = tbuf->sockfd;

	//int op = tbuf->op;



#ifdef DEBUG
    printf("wait op = %d, sockfd = %d\n", op, sockfd);
#endif
//	int buf_size = 65537;
//	int buf_size = 32769;
//	int buf_size = 12769;
//	int buf_size = 8192;
//	int buf_size = 4096;
//	int buf_size = 2048;
//	int buf_size = 1024;
//	int buf_size = 512;
//	int buf_size = 2;
	int buf_size = tbuf->garbage_num;
//	int buf_size = 128;
//	int buf_size = 256;
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
//	int buf_size = 65537;
//	int buf_size = 2;
	int buf_size = 128;
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

pthread_mutex_lock(&mtx2[op]);
while (tbuf->stop_send_garbage == 0) {
pthread_mutex_unlock(&mtx2[op]);
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

  //      sbuf[op]->real_trans_bytes+=(len+5);
//    usleep(10);
pthread_mutex_lock(&mtx2[op]);
}
pthread_mutex_unlock(&mtx2[op]);

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
    if(sbuf[op]->stop_send_garbage)
        return;
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

	//while(sbuf[op]->kick);


#ifdef DEBUG
	int ret = 0;
	printf("in stop_send_garbage now op = %d after recv and recv %d bytes\n", op, ret);
#endif
}

void *trans_func(void *data)
{

	cpu_set_t cpuset;
//	int cpu = op;
	int cpu = 7;
	CPU_ZERO(&cpuset);
	CPU_SET(cpu, &cpuset);
	sched_setaffinity(0, sizeof(cpuset), &cpuset);

	int op = 0;
	while(1) {

//		send_garbage(op); //cocotion test

		op = (op+1)%2;

		int tail, head;

		if(!sbuf[op]->garbage_state)
		{
			pthread_mutex_lock(&mtx[op]);
			tail = sbuf[op]->tail;
			head = sbuf[op]->head;
			pthread_mutex_unlock(&mtx[op]);

#ifdef DEBUG
			printf("op start = %d, head is %d, tail is %d\n", op, head, tail);
			printf("op start = %d, ghead is %d, gtail is %d\n", op, sbuf[op]->garbage_head, sbuf[op]->garbage_tail);
#endif
			if(head == tail) {
				continue;
			}
		}
		else {
			tail = sbuf[op]->garbage_tail;
			head = sbuf[op]->garbage_head;

		}

        //stop_send_garbage(0);
        //stop_send_garbage(1);


//		printf("$$$$@@@cocotion op = %d, head = %d, tail = %d\n", op, head, tail);

//		printf("$$$$@@@cocotion op = %d, wait times = %d, head = %d, tail = %d\n", op, sbuf[op]->wait_times, head, tail);
		sbuf[op]->wait_times = 0;

		/*
		while(head == tail) {
			pthread_cond_wait(&cond[op], &mtx[op]);
//			continue;
			tail = sbuf[op]->tail;
			head = sbuf[op]->head;
		}
		pthread_mutex_unlock(&mtx[op]);
*/

		char *buf  = sbuf[op]->buf;
		int sockfd = sbuf[op]->sockfd;
		int len = TOTAL_LEN;
		int bottom = 0;

		//printf("op = %d, head = %d,  tail = %d\n", op, head, tail);

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
		//stop_send_garbage(op);
//        char *ptr;
        char mmbuf;
		if(sbuf[op]->garbage_state)
    		memset(&mmbuf, 1, 1);
		else
    		memset(&mmbuf, 0, 1);

		for(i = 0; i < total_times; i++) {
			offset = 0;
//			len+=1;

            //mmap((char*)(buf+head), TOTAL_LEN, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 1);
            //ptr = (char*)mmap(0, TOTAL_LEN+1, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    		//memset(ptr, 0, 1);
            /*mbuf[1] = buf+head;
			printf("op = %d, address test mbuf[1] = %p\n", op, mbuf[1]);
			printf("op = %d, address test buf+head = %p\n", op, buf+head);
			printf("op = %d, address test mbuf[0] = %p\n", op, mbuf[0]);
			printf("op = %d, address test mbuf = %p\n", op, mbuf);
			printf("op = %d, address test mbuf+1 = %p\n", op, mbuf+1);
			printf("op = %d, address test mbuf+2 = %p\n", op, mbuf+2);
*/
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
//				printf("before send content mbuf[0][0] = %c\n", mbuf[0][0]);
//				printf("before send content mbuf[1][0] = %c\n", mbuf[1][0]);
//				printf("before send content mbuf[1][1] = %c\n", *(mbuf[1]+1));
//				printf("before send content mbuf = %c\n", *(mbuf+2));
				//printf("before send content mbuf+offset = %c\n", *(mbuf+offset+3));
//				int ret = send(sockfd, mbuf + offset , len,0);
//				printf("before send content ptr+1 = %c\n", *(char*)(ptr+1));
//				printf("before send content ptr+1 = %c\n", *(char*)(ptr+1));
//				printf("before send content ptr[2] = %c\n", ptr[2]);
			//	printf("before send content buf+head = %c\n", *(char*)(buf+head));
				int ret = 0;
                if(offset == 0) {
				    //while ( ret = send(sockfd, &mmbuf, 1,0) == -1);
					do {
				    	ret = send(sockfd, &mmbuf, 1,0);
					} while (ret != 1);
					offset +=1;
					//if(ret == -1)
						//continue;

					//char *mylen;
					int sublen = 4;
					int myoffset = 0;
					//ret = send(sockfd, &len, 4, 0);
					//sprintf(mylen, "%d", len);
					int *plen;
					plen = &len;
					//mylen = (char*)plen;
					do {
						//ret = send(sockfd, mylen+myoffset, sublen, 0);
						ret = send(sockfd, (char*)plen+myoffset, sublen, 0);
						sublen -= ret;
						myoffset += ret;
					} while (sublen) ;
#ifdef DEBUG
					printf("cocotion op = %d, send len = %d, ret = %d\n",op,  len, ret);
#endif
                }
//				ret = send(sockfd, ptr + offset -1 , len,0);


				ret = send(sockfd, buf+head + offset -1 , len,0);

#ifdef DEBUG

			printf("       cocotion test op = %d, ret = %d, offset = %d len = %d\n", op, ret, offset, len);
#endif
				//				printf("send op = %d\n", op);
//				pthread_cond_signal(&cond3);
//				pthread_mutex_unlock(&mtx3);
				offset += ret;
	    		len = len - ret;
			}while (len);

			len = TOTAL_LEN;

			head += len;

			sbuf[op]->real_trans_bytes+=(len+5);
		}

//        munmap(ptr+1, TOTAL_LEN);

		offset = 0;
		int rest = num - total_times*TOTAL_LEN;
//		printf("cocotion in rest, rest = %d, num = %d, total_times = %d\n", rest, num, total_times);


		if(sbuf[op]->garbage_state)
    		memset(&mmbuf, 1, 1);
    	else
			memset(&mmbuf, 0, 1);

        if(rest) {
            //mmap(buf+head, rest, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 1);
            //ptr = mmap(0, TOTAL_LEN+1, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    		//memset(ptr, 0, 1);

       //     rest+=1;
//			mbuf[1] = buf+head;




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
	    		//int ret = send(sockfd, mbuf + offset, rest,0);
	    		//int ret = send(sockfd, ptr + offset, rest,0);
				int ret = 0;
                if(offset == 0) {
				    //while ( ret = send(sockfd, &mmbuf, 1,0) == -1);
					do {
				    	ret = send(sockfd, &mmbuf, 1,0);
					} while (ret != 1);
					//if(ret == -1)
						//continue;
					offset += 1;
#ifdef DEBUG
					printf("cocotion rest = %d send\n", rest);
#endif

					//ret = send(sockfd, &rest, 4, 0);

					//char *mylen;
					int sublen = 4;
					int myoffset = 0;
					//sprintf(mylen, "%d", rest);
					int *plen;
					plen = &rest;
					//mylen = (char*)plen;

					do {
						//ret = send(sockfd, mylen+myoffset, sublen, 0);
						ret = send(sockfd, (char*)plen+myoffset, sublen, 0);
						sublen -= ret;
						myoffset += ret;
					} while (sublen) ;

#ifdef DEBUG
					printf("cocotion op = %d, send rest len = %d, ret = %d\n", op, rest, ret);
#endif
				}
//				while ( ret = send(sockfd, buf+head + offset -1 ,rest,0) == -1);
				ret = send(sockfd, buf+head + offset -1 ,rest,0);
				if(ret == -1)
					exit(1);
				//	continue;

#ifdef DEBUG

                printf("        cocotion test rest op = %d, ret = %d, offset = %d len = %d\n", op, ret, offset, rest);
//				exit(1);
#endif

//				printf("send op rest= %d\n", op);
//				pthread_cond_signal(&cond3);
//				pthread_mutex_unlock(&mtx3);
				offset += ret;
	    		rest = rest - ret;
			}while (rest);

			head = head + offset-1;
			//head-=1;
			sbuf[op]->real_trans_bytes+=(offset-1+5);
		}

//        munmap(ptr+1, TOTAL_LEN);

		if(!sbuf[op]->garbage_state)
			sbuf[op]->head = head;
		else
			sbuf[op]->garbage_head = head;


#ifdef DEBUG
		printf("~~~~op = %d, head= %d, real tail = %d, real head = %d\n", op, head, sbuf[op]->tail, sbuf[op]->head);
		printf("~~~~op = %d, head= %d, now tail = %d, garbage state = %d\n", op, head,  tail, sbuf[op]->garbage_state);
		printf("~~~~op = %d, real trans bytes = %d, MAXNUM = %d\n", op, sbuf[op]->real_trans_bytes, sbuf[op]->real_num);
#endif


		if((!sbuf[op]->garbage_state) && tail == head) {
			if(tail < sbuf[op]->real_num) {
				sbuf[op]->garbage_num = sbuf[op]->real_num-tail;
#ifdef DEBUG
				printf("op = %d, garbage_num is %d\n", op, sbuf[op]->garbage_num);
#endif
			//	garbage_send_func2(&op);
				sbuf[op]->garbage_head = 0;
				sbuf[op]->garbage_tail = sbuf[op]->garbage_num;
				sbuf[op]->garbage_state = 1;
				//sbuf[op]->real_trans_bytes = 0;
				continue;
			}
			else {
				sbuf[op]->garbage_state = 1;
			}

//			sbuf[op]->head = 0;
//			sbuf[op]->tail = 0;
		}
		if(sbuf[op]->garbage_state && tail == head) {
#ifdef DEBUG
			printf("op = %d, garbage end head is %d\n", op, head);
#endif
			sbuf[op]->garbage_state = 0;


/*
            pthread_mutex_lock(&mtx2[op]);
			sbuf[op]->gonext = 1;
			pthread_cond_signal(&cond2[op]);
			pthread_mutex_unlock(&mtx2[op]);
*/

//			printf("cocotion test real trans_bytes = %d\n", sbuf[op]->real_trans_bytes);
			//sbuf[op]->real_trans_bytes = 0;
		}



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


//cocotion fucking
/*

			if(op == 0) {
			gettimeofday(&end2,NULL);
			unsigned int timer = 1000000 * (end2.tv_sec-start2.tv_sec)+ end2.tv_usec-start2.tv_usec;

			printf("want go next... op = %d, head = %d, tail = %d, time = %d\n", op, sbuf[op]->head, sbuf[op]->tail, timer);
			}
*/

//		stop_send_garbage((op+1)%2); //cocotion test
//		usleep(1);

	}
}

void *func(void *data)
{
	int *port = (int*)data;
	int port_num = *port;
	int op = port_num%2;

//	sbuf[op]->pid = getpid();

/*
	char vfname[100];
	sprintf(vfname, "/home/coldfunction/newnfs/socketTest/%d", port_num);
	int fd=open(vfname, O_CREAT|O_RDWR,0666);
	if(fd<0) {
		printf("failure to open\n");
		exit(0);
	}
	if(lseek(fd,TOTAL_LEN+1,SEEK_SET)==-1) {
		printf("Failure to lseek\n");
		exit(0);
	}
	if(write(fd, "",1) != 1) {
		printf("Failure on write\n");
		exit(0);
	}
*/



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

	sbuf[op]->op_switch = 0;
    //sbuf[op]->fd = fd;

	sbuf[op]->pid = getpid();

	int sockfd = socket_connect("172.31.3.2", port_num);
	int sockfd2 = socket_connect("172.31.3.2", port_num+1);

	sbuf[op]->sockfd2 = sockfd2;
	sbuf[op]->stop_send_garbage = 1;

	sbuf[op]->is_last_garbage = 0;
	sbuf[op]->shit = 0;
	sbuf[op]->good = 0;
	sbuf[op]->wait_times = 0;


	sbuf[op]->op = op;
	sbuf[op]->sockfd = sockfd;


	sbuf[op]->garbage_head = sbuf[op]->garbage_tail = 0;
	sbuf[op]->garbage_state = 0;
	sbuf[op]->gonext = 0;

    sbuf[op]->stop_send_garbage = 1;

//	pthread_t gthread;
//	pthread_create(&gthread, NULL, garbage_send_func, sbuf[op]);

    //Send a message to server
	//

	int len = TOTAL_LEN2;
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
	if(op == 0) {

		//pthread_attr_t tattr;
		//int policy;
		//int ret;

		//pthread_attr_init(&tattr);

		//ret = pthread_attr_setschedpolicy(&tattr, SCHED_RR);


	//	pthread_t trans_thread;
//	pthread_create(&trans_thread, &tattr, trans_func, sbuf[op]);
		pthread_create(&trans_thread, NULL, trans_func, NULL);
//		pthread_create(&trans_thread, NULL, trans_func, &fd);

	}

	mythread = trans_thread;

    int myMAXnum = MAXNUM;

	int k = 0;
	while (fscanf(file, "%d", &num)!=EOF) {

		int mynumcount = 0;


        cycle++;
        if(cycle % 100 == 0)
            myMAXnum = MAXNUM;

		if(num < 2)
			num = 2;

		if(num > myMAXnum) {
			myMAXnum = num;
			//MAXNUM += (MAXNUM/TOTAL_LEN+1)*5;
		}


		int sublen = 4;
		int myoffset = 0;
		int plen = myMAXnum;
		do {
			int ret = send(sockfd, (char*)(&plen)+myoffset, sublen, 0);
			sublen -= ret;
			myoffset += ret;
		} while (sublen) ;





		k++;
//		usleep(runningtime);


//		pthread_mutex_lock(&mtx[op]);
		//sbuf[op]->gonext = 0;
//		pthread_mutex_unlock(&mtx[op]);

		//sbuf[op]->real_trans_bytes = 0;

		sbuf[op]->real_num = myMAXnum;


		//sbuf[op]->gonext = 3333;

		gettimeofday(&start,NULL);
		if(op == 0) {
			gettimeofday(&start2,NULL);
		}

        sbuf[op]->real_trans_bytes = 0;

		int total_times = num/TOTAL_LEN2;

		srand(time(NULL));
		int a = rand()%5;

		int wantwait = 0;

		pthread_mutex_lock(&mtx[op]);

//		sbuf[op]->not_garbage_head = 0;
//		sbuf[op]->not_garbage_tail = num;


		sbuf[op]->tail = num;
		sbuf[op]->head = 0;
		//pthread_cond_signal(&cond[op]);
		//sbuf[op]->gonext = num;
		pthread_mutex_unlock(&mtx[op]);



//		printf("okok I recv now\n");
/*
		pthread_mutex_lock(&mtx[op]);
		kick_trans = 1;
		pthread_cond_signal(&cond[op]);
		pthread_mutex_unlock(&mtx[op]);
*/
//cocotion fucking test
//		if(op == 0)
#ifdef DEBUG
			printf("okok I recv now op = %d, head = %d, tail = %d, num = %d\n", op, sbuf[op]->head, sbuf[op]->tail, num);
#endif
		//recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
		int ret;
		do {
			ret = recv(sockfd,receiveMessage,1,0);
		} while(ret != 1);

		//int real_num = sbuf[op]->real_trans_bytes;

//        send_garbage(op);

        //pthread_mutex_lock(&mtx[op]);
		//pthread_mutex_unlock(&mtx[op]);


//		pthread_mutex_lock(&mtx2[op]);
//		if(!sbuf[op]->gonext)
//			pthread_cond_wait(&cond2[op], &mtx2[op]);
		int real_num = sbuf[op]->real_trans_bytes;
//		sbuf[op]->gonext = sbuf[op]->real_trans_bytes = 0;
//		pthread_mutex_unlock(&mtx2[op]);


//        send_garbage(op);
//		pthread_mutex_lock(&mtx[op]);
		//sbuf[op]->gonext = 1;
//		pthread_mutex_unlock(&mtx[op]);


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
/*
		printf("op = %d, what I think trans byte = %d\n", op, num);
		printf("op = %d, real trans byte = %d\n", op, sbuf[op]->real_trans_bytes);
		printf("op = %d, what I think transfer rate = %ld\n", op, num/timer);
		printf("op = %d, real transfer rate = %ld\n", op, sbuf[op]->real_trans_bytes/timer);
*/

		fscanf(file2, "%d", &runningtime);


		runningtime -= timer;
		if(runningtime <= 0)
			runningtime = 1;

//        printf("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ real time = %ld\n", timer);

		if(op%2 == 0) {
	   		FILE *pFile;
   	   		char pbuf[200];
			pFile = fopen("mytransfer_rate.txt", "a");
    		if(pFile != NULL){
//        		sprintf(pbuf, "%ld\n",num/timer);
        		sprintf(pbuf, "%ld\n",real_num/timer);
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
//        		sprintf(pbuf, "%ld\n",num/timer);
        		sprintf(pbuf, "%ld\n",real_num/timer);
        		fputs(pbuf, pFile);
    		}
    		else
        		printf("no profile\n");
    		fclose(pFile);
		}

        /*
	   	FILE *pFile;
   	   	char pbuf[200];
		pFile = fopen("inputnew.txt", "a");
    	if(pFile != NULL){
            int d = num/timer;
            if(d > 600 && d < 700) {
                //printf("%d\n", MAXNUM);
        	    sprintf(pbuf, "%d\n",MAXNUM);
//        		sprintf(pbuf, "%ld\n",real_num/timer);
        	    fputs(pbuf, pFile);
    	    }
        }
    	else
        	printf("no profile\n");
    	fclose(pFile);
        */

	}
/*
	pthread_mutex_lock(&mtx[op]);
	sbuf[op]->op_switch = 1;
	printf("op = %d, send signal to QQQQ op_switch\n", op);
	pthread_cond_signal(&cond[op]);
	pthread_mutex_unlock(&mtx[op]);
*/

	pthread_barrier_wait(&barr);

//	if(op == 0) {
//		pthread_cancel(trans_thread);
//		pthread_join(trans_thread, NULL);
//	}

//	pthread_cancel(gthread);
//	pthread_join(gthread, NULL);

    printf("close Socket\n");
	free(buf);
	free((void*)sbuf[op]);
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

	/*pthread_attr_init(&tattr1);
	pthread_attr_init(&tattr2);
	ret = pthread_attr_setschedpolicy(&tattr1, SCHED_RR);
	ret = pthread_attr_setschedpolicy(&tattr2, SCHED_RR);
*/

	pthread_barrier_init(&barr, NULL, 2);



	pthread_t appThread[2];

	//pthread_create(&appThread[0], &tattr1, func, &port_num);
//	pthread_create(&appThread[0], &tattr1, func, &port_num);
	pthread_create(&appThread[0], NULL, func, &port_num);
	int port_num2 = port_num+11;

	pthread_create(&appThread[1], NULL, func, &port_num2);

//	sbuf[port_num%2]->producer_t = appThread[0];
//	sbuf[port_num2%2]->producer_t = appThread[1];





//	pthread_cancel(appThread[0]);
//	pthread_cancel(appThread[1]);
	pthread_join(appThread[0], NULL);
	pthread_join(appThread[1], NULL);


	pthread_cancel(mythread);
	pthread_join(mythread, NULL);


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
