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


//#define TOTAL_LEN (64*1024)
#define TOTAL_LEN (8*1024)
//#define TOTAL_LEN (512*1024)
#define TOTAL_DATA_SIZE (8*1024*1024)
//#define TOTAL_DATA_SIZE (64*1024*1024)
#define META_HEAD (TOTAL_DATA_SIZE/TOTAL_LEN)

int TOTAL_VM = 4;

pthread_t mythread;

static pthread_mutex_t mtx[4];
static pthread_cond_t cond[4];

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

    int gonext;

};

struct sendBuf *sbuf[4];


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
	int buf_size = 65537;
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
	char *buf = malloc(buf_size);
	memset(&buf[0], 1, 1);

	while(1) {

		pthread_mutex_lock(&mtx2[op]);
		do {
            printf("op = %d, wait to send garbage!!!!\n", op);
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
			printf("op = %d, start already send garbage len = %d sockfd = %d\n", op, offset, sockfd);
			printf("op = %d, start rest send garbage len = %d\n", op, len);
		} while (len);
}

		printf("after op = %d !!!!!!!!!!!!!\n", op);
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

	printf("in stop_send_garbage now op = %d before recv\n", op);

	pthread_mutex_lock(&mtx4[op]);

	while(sbuf[op]->kick) {
		pthread_cond_wait(&cond4[op], &mtx4[op]);
	}
	pthread_mutex_unlock(&mtx4[op]);

	//while(sbuf[op]->kick);


	int ret = 0;
	printf("in stop_send_garbage now op = %d after recv and recv %d bytes\n", op, ret);
}

void *trans_func(void *data)
{
/*
	cpu_set_t cpuset;
	//	int cpu = op%2;
	int cpu = 7;
	CPU_ZERO(&cpuset);
	CPU_SET(cpu, &cpuset);
	sched_setaffinity(0, sizeof(cpuset), &cpuset);
*/

	unsigned long timer = 0;

	int op = 0;
	while(1) {

		//op = (op+1) % TOTAL_VM;

		char *buf  = sbuf[op]->buf;
		int sockfd = sbuf[op]->sockfd;
		int head, tail = 0;
		int len = TOTAL_LEN;
		int bottom = 0;
/*
		int inloop = 0;

		pthread_mutex_lock(&mtx[op]);
		while (sbuf[op]->head == sbuf[op]->tail) {
#ifdef DEBUG
			printf("wait op = %d, head = %d, tail = %d\n", op, sbuf[op]->head, sbuf[op]->tail);
#endif
			inloop++;
			printf("op = %d, inloop = %d\n", op, inloop);
			printf("wait op = %d, head = %d, tail = %d\n", op, sbuf[op]->head, sbuf[op]->tail);

//		    pthread_cond_signal(&cond[op]);
			pthread_mutex_unlock(&mtx[op]);

			op = (op+1) % TOTAL_VM;
			sockfd = sbuf[op]->sockfd;
			buf  = sbuf[op]->buf;
			//usleep(5);
			pthread_mutex_lock(&mtx[op]);
		}

		inloop = 0;
*/
		pthread_mutex_lock(&mtx[op]);

		head = sbuf[op]->head;
		tail = sbuf[op]->tail;

		sbuf[op]->gonext = 0;

		//int update_head = 0;

		pthread_mutex_unlock(&mtx[op]);

		int offset = 0;

		int num = ((tail-head) >= TOTAL_LEN) ? TOTAL_LEN : (tail-head);
		int total_times = num/TOTAL_LEN;
		int i;

	struct  timeval  start;
	struct  timeval  end;
	gettimeofday(&start,NULL);



		for(i = 0; i < total_times; i++) {
			offset = 0;
			do {
				int ret = 0;
				ret = send(sockfd, buf+head + offset , len,0);

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
        if(rest) {
			do {
				int ret = 0;
				ret = send(sockfd, buf+head + offset ,rest,0);
				if(ret == -1)
					exit(1);
#ifdef DEBUG

                printf("        cocotion test rest op = %d, ret = %d, offset = %d len = %d\n", op, ret, offset, rest);
#endif
				offset += ret;
	    		rest = rest - ret;
			}while (rest);
			head = head + offset;
		}


        pthread_mutex_lock(&mtx[op]);
//		printf("~~~~op = %d, head= %d, tail = %d, local head = %d, gonext = %d\n", op, sbuf[op]->head, sbuf[op]->tail, head, sbuf[op]->gonext);

		if(!sbuf[op]->gonext)
			sbuf[op]->head = head;
		//else
        	//sbuf[op]->gonext = 0;
		pthread_mutex_unlock(&mtx[op]);

#ifdef DEBUG
		printf("~~~~op = %d, head= %d, tail = %d\n", op, head, sbuf[op]->tail);
#endif
		gettimeofday(&end,NULL);
		timer += 1000000 * (end.tv_sec-start.tv_sec)+ end.tv_usec-start.tv_usec;

		//printf("op = %d, cocotion test time = %ld\n", op, timer);

//		if(timer > 50) {
		if(timer > 100) {
			op = (op+1) % TOTAL_VM;
			timer = 0;
		}
	}
}

void *func(void *data)
{
	int *port = (int*)data;
	int port_num = *port;
	int op = port_num % TOTAL_VM;

    printf("cocotion test op = %d, port_num = %d\n", op, port_num);


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

    sbuf[op]->gonext = 0;

	int sockfd = socket_connect("172.31.3.2", port_num);

	sbuf[op]->stop_send_garbage = 1;

	sbuf[op]->op = op;
	sbuf[op]->sockfd = sockfd;

	printf("cocotion test sockfd = %d\n", sockfd);

    //Send a message to server
	//

	int len = TOTAL_LEN;
	char *buf;
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

	pthread_t trans_thread;
	if(op == 0) {
		pthread_create(&trans_thread, NULL, trans_func, sbuf[op]);
	}

	mythread = trans_thread;

	int k = 0;
	while (fscanf(file, "%d", &num)!=EOF) {

		int mynumcount = 0;


		if(num < 2)
			num = 2;

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

#ifdef DEBUG
			printf("@@@@ op = %d, fucking num = %d\n", op, num);
#endif

			len = TOTAL_LEN;

			int tail;

			pthread_mutex_lock(&mtx[op]);

#ifdef DEBUG
			printf("@@@@@@@@@@@@@ op = %d, tail = %d, head = %d\n", op, sbuf[op]->tail, sbuf[op]->head);
#endif
			//tail = sbuf[op]->tail;
			//tail += len;
			sbuf[op]->tail += len;
			pthread_mutex_unlock(&mtx[op]);


			mynumcount+=len;
#ifdef DEBUG
			//printf("@@@@@@ now op = %d, total produce num = %d\n", op, mynumcount);
			printf("@@@@@@ now op = %d, tail = %d\n", op, sbuf[op]->tail);
#endif
		}

		offset = 0;
		int rest = num - total_times*TOTAL_LEN;
		if(rest) {

#ifdef DEBUG
			printf("rest@@@@@@@@@@@@@ op = %d, tail = %d, head = %d\n", op, sbuf[op]->tail, sbuf[op]->head);
#endif
			pthread_mutex_lock(&mtx[op]);
			sbuf[op]->tail += rest;
			pthread_mutex_unlock(&mtx[op]);


			mynumcount+=rest;
#ifdef DEBUG
			//printf("@@@@@@ now op = %d, total produce num = %d\n", op, mynumcount);
			printf("@@@@@@ now op = %d, tail = %d\n", op, sbuf[op]->tail);
#endif

		}
#ifdef DEBUG
			printf("okok I recv now op = %d, head = %d, tail = %d, num = %d\n", op, sbuf[op]->head, sbuf[op]->tail, num);
#endif
		int ret;
		do {
			ret = recv(sockfd,receiveMessage,1,0);
		} while(ret != 1);

		//while(sbuf[op]->head != sbuf[op]->tail) {
		//	printf("fucking shit!!!!op = %d, port_num = %d, head = %d, tail = %d\n", op, port_num, sbuf[op]->head, sbuf[op]->tail);

		//exit(1);
		//}

		//if(sbuf[op]->tail - sbuf[op]->head > 65536) {
		//	printf("fucking shit!!!!op = %d, port_num = %d, head = %d, tail = %d\n", op, port_num, sbuf[op]->head, sbuf[op]->tail);
		//}


//		while(sbuf[op]->head != sbuf[op]->tail) ;

		pthread_mutex_lock(&mtx[op]);
        sbuf[op]->gonext = 1;
//		pthread_cond_wait(&cond[op], &mtx[op]);
//		if(sbuf[op]->head != num)
//			printf("fucking shit!!!! head = %d, tail = %d\n", sbuf[op]->head, sbuf[op]->tail);

        sbuf[op]->tail = sbuf[op]->head = 0;
		pthread_mutex_unlock(&mtx[op]);

//		printf("@@@@@@ now op = %d, after recv okokokok head = %d, tail = %d \n", op, sbuf[op]->head, sbuf[op]->tail);

		gettimeofday(&end,NULL);
		timer = 1000000 * (end.tv_sec-start.tv_sec)+ end.tv_usec-start.tv_usec;



		if(op % TOTAL_VM == 0) {
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
		else if(op % TOTAL_VM == 1) {
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
        else if(op % TOTAL_VM == 2) {
	   		FILE *pFile;
   	   		char pbuf[200];
			pFile = fopen("mytransfer_rate3.txt", "a");
    		if(pFile != NULL){
        		sprintf(pbuf, "%ld\n",num/timer);
        		fputs(pbuf, pFile);
    		}
    		else
        		printf("no profile\n");
    		fclose(pFile);
		}
        else if(op % TOTAL_VM == 3) {
	   		FILE *pFile;
   	   		char pbuf[200];
			pFile = fopen("mytransfer_rate4.txt", "a");
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

    printf("close Socket\n");
	free(buf);
	free(sbuf[op]);
	close(sockfd);
	fclose(file);
	fclose(file2);

}



int main(int argc , char *argv[])
{

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
	TOTAL_VM = atoi(argv[2]);

	pthread_attr_t tattr[TOTAL_VM];

    int i;
    int newport[TOTAL_VM];
    for(i = 0; i < TOTAL_VM; i++) {
	    pthread_mutex_init(&mtx[i], NULL);
	    pthread_cond_init(&cond[i], NULL);

        pthread_attr_init(&tattr[i]);
		pthread_attr_setschedpolicy(&tattr[i], SCHED_RR);
        newport[i] = port_num + i*11;
    }

	pthread_barrier_init(&barr, NULL, TOTAL_VM);



	pthread_t appThread[TOTAL_VM];

    for(i = 0; i < TOTAL_VM; i++) {
        pthread_create(&appThread[i], &tattr[i], func, &newport[i]);
    }



//	pthread_cancel(appThread[0]);
//	pthread_cancel(appThread[1]);

    for(i = 0; i < TOTAL_VM; i++)  {
        pthread_join(appThread[i], NULL);
    }

	pthread_cancel(mythread);
	pthread_join(mythread, NULL);

    return 0;
}
