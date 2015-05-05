#include <myqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>


void notified(int sig) {
	if (sig == SIGUSR1)
		printf("Msg in queue !\n");
}


int main(int argc, char** argv) {

	char s[12], s2[12];
	unsigned a, i;
	mqd_t qid;
	struct sigevent my_sigevent;
	struct sigaction my_sigact;
	sigset_t mask, mask_act;
	
	qid = mq_open("/notif_queue", O_CREAT|O_RDWR, 0666);
	
	printf("%%%%%%%% OPEN %%%%%%%%\n");
	
	sigfillset(&mask);
	sigprocmask(SIG_SETMASK, &mask, 0);
	
	
	printf("%%%%%%%% SUBSCRIBE %%%%%%%%\n");
	sigemptyset(&mask_act);
	my_sigact.sa_flags = 0;
	my_sigact.sa_mask = mask_act;
	my_sigact.sa_handler = notified;
	sigaction(SIGUSR1, &my_sigact, 0);
	sigaction(SIGUSR2, &my_sigact, 0);
	my_sigevent.sigev_notify = SIGEV_SIGNAL;
	my_sigevent.sigev_signo = SIGUSR1;
	mq_notify(qid, &my_sigevent);

	if (fork()== 0) {
		mq_receive(qid, s2, 32, &a);
		printf("%d> RECV> msg prio %d -- %s\n", getpid(), a, s2);
		exit(0);
	}
	
	if (fork()==0) {
		strcpy(s, "mymsg");
		mq_send(qid, s, strlen(s)+1, 1);
		printf("%d> SEND> msg prio %d\n", getpid(), 1);
		mq_send(qid, s, strlen(s)+1, 2);
		printf("%d> SEND> msg prio %d\n", getpid(), 2);
		kill(getppid(), SIGUSR2);
		exit(0);
	} else {
		sigdelset(&mask, SIGUSR2);
		sigsuspend(&mask);
		sigdelset(&mask, SIGUSR1);
		sigsuspend(&mask);
	}
	
	
	for (i = 0; i < 2; i++) {
		wait(0);
	}
	
	mq_receive(qid, s2, 32, &a);
	printf("%d> RECV> msg prio %d -- %s\n", getpid(), a, s2);

	mq_close(qid);
	mq_unlink("/notif_queue");
	
	return 0;

}
	