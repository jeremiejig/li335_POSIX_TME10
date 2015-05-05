#include <myqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>


int main(int argc, char** argv) {

	char s[12], s2[12];
	unsigned a, i;
	mqd_t qid;
	
	qid = mq_open("/you_queue", O_CREAT|O_RDWR, 0666);
	
	printf("open\n");
	
	strcpy(s, "mymsg");
	
	printf("%%%%%%%% SEND %%%%%%%%\n");
	for (i = 0; i < 70; i++)
		if (fork()==0) {
			mq_send(qid, s, strlen(s)+1, (getpid()%32)+1);
			printf("SEND> msg prio %d\n", (getpid()%32)+1);
			exit(0);
		}
	
	printf("%%%%%%%% RECV %%%%%%%%\n");
	for (i = 0; i < 70; i++)
		if (fork()== 0) {
			mq_receive(qid, s2, 32, &a);
			printf("RECV> msg prio %d -- %s\n", a, s2);
			exit(0);
		}

	for (i = 0; i < 140; i++) {
		wait(0);
	}
	printf("%%%%%%%% ALL DONE %%%%%%%%\n");

	
	
	mq_close(qid);
	mq_unlink("/you_queue");
	
	return 0;

}
	