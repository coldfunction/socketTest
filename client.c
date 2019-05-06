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


#define TOTAL_LEN (8*1024)
#define TOTAL_DATA_SIZE (8*1024*1024)

static pthread_mutex_t mtx[4];
static pthread_cond_t cond[4];

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


pthread_t mythread;
static pthread_barrier_t barr;

int TOTAL_VM = 4;



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

	int gonext;
};

struct sendBuf *sbuf[4];

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
	int buf_size = 65536;
//	int buf_size = 4;
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

	cpu_set_t cpuset;
	//	int cpu = op%2;
	int cpu = 7;
	CPU_ZERO(&cpuset);
	CPU_SET(cpu, &cpuset);
	sched_setaffinity(0, sizeof(cpuset), &cpuset);


	unsigned long timer = 0;

//	int op = 0;
	int op = *(int*)data;
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
/*		gettimeofday(&end,NULL);
		timer += 1000000 * (end.tv_sec-start.tv_sec)+ end.tv_usec-start.tv_usec;

		//printf("op = %d, cocotion test time = %ld\n", op, timer);

//		if(timer > 50) {
//		if(timer > 130 ) { // 4 VMs is ok
//		if(timer > 150 ) { // 2 VMs is ok
		if(timer > 180 ) {
			op = (op+1) % TOTAL_VM;
			timer = 0;
		}*/
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
	sbuf[op]->buf = malloc(TOTAL_DATA_SIZE);
	sbuf[op]->head = sbuf[op]->tail = sbuf[op]->kick = sbuf[op]->bottom = 0;


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


	pthread_attr_t tattr;
	pthread_attr_init(&tattr);
	pthread_attr_setschedpolicy(&tattr, SCHED_RR);


	pthread_t trans_thread;
//	if(op == 0) {
	pthread_create(&trans_thread, &tattr, trans_func, &op);
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

//			pthread_mutex_lock(&mtx[op]);

#ifdef DEBUG
			printf("@@@@@@@@@@@@@ op = %d, tail = %d, head = %d\n", op, sbuf[op]->tail, sbuf[op]->head);
#endif
			//tail = sbuf[op]->tail;
			//tail += len;
			sbuf[op]->tail += len;
//			pthread_mutex_unlock(&mtx[op]);


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
//			pthread_mutex_lock(&mtx[op]);
			sbuf[op]->tail += rest;
//			pthread_mutex_unlock(&mtx[op]);


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

	pthread_mutex_init(&mtx[0], NULL);
	pthread_cond_init(&cond[0], NULL);

	pthread_mutex_init(&mtx[1], NULL);
	pthread_cond_init(&cond[1], NULL);

	pthread_mutex_init(&mtx[2], NULL);
	pthread_cond_init(&cond[2], NULL);

    pthread_mutex_init(&mtx2[0], NULL);
	pthread_cond_init(&cond2[0], NULL);

	pthread_mutex_init(&mtx2[1], NULL);
	pthread_cond_init(&cond2[1], NULL);

	pthread_barrier_init(&barr, NULL, 4);


	int port_num = atoi(argv[1]);
	int port_num2 = port_num+11;
	int port_num3 = port_num+22;
	int port_num4 = port_num+33;

	pthread_t appThread[4];

	pthread_create(&appThread[0], NULL, func, &port_num);

	pthread_create(&appThread[1], NULL, func, &port_num2);

    pthread_create(&appThread[2], NULL, func, &port_num3);
    pthread_create(&appThread[3], NULL, func, &port_num4);




//	pthread_cancel(appThread[0]);
//	pthread_cancel(appThread[1]);
	pthread_join(appThread[0], NULL);
	pthread_join(appThread[1], NULL);
	pthread_join(appThread[2], NULL);
	pthread_join(appThread[3], NULL);



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
