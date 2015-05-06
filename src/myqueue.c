#include "myqueue.h"
#include <stdlib.h>
#include <sys/mman.h>
#include <errno.h>

#define BLOCKED_WRITERS_PATH blockwrit_
#define BLOCKED_READERS_PATH blockread_
#define QUEUE_SEM_PATH queuesem_
#define SEARCH_SEM_PATH searchsem_

struct localinfo_s {
	int mq_flags;
	sem_t *blocked_writers;
	sem_t *blocked_readers;
	sem_t *queue_sem;
	sem_t *search_sem;
};

void dump_queue(struct mq_attr* attr);

int started = 0;
//Contient surtout le flag local associé à la file (localinfo)
struct localinfo_s localinfo[20];
//Projection du shm sur hashLink[fd]
struct mq_attr *hashLink[20];

char genpathed[32];

char *genPath(const char* pattern, int i) {
	snprintf(genpathed, 32, "%s%i", pattern, i);
	return genpathed;
}

mqd_t mq_open(const char* path, int flags, int mode){
	int fd, i;
	struct mq_attr* attr;

	if(!started) {
		for( i = 0; i < 20 ; i++ )
			hashLink[i]=NULL;
		started=1;
	}

	if((fd = shm_open(path, flags, mode)) == -1)
		return (mqd_t) -1;

	if(flags & O_CREAT && ftruncate(fd, sizeof(struct mq_attr)))
		return (mqd_t) -1;

	if((attr = mmap(NULL, sizeof(struct mq_attr), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED)
		return (mqd_t) -1;

	if(flags & O_CREAT) {
		//Setting up message queue
		attr->mq_maxmsg = 32;
		attr->mq_msgsize = sizeof(msg);
		attr->mq_curmsgs = 0;
		attr->subscriber = 0;
		attr->first = NULL;
		//attr->last = NULL;

		//All the msg struct isn't alloc at first.
		for(i = 0; i < attr->mq_maxmsg ; i++)
			attr->queue[i].used = 0;

		/**	Creating semaphore.	**/
		//Writer initialised at maxmsg
		for(i = 0; SEM_FAILED==(localinfo[fd].blocked_writers = sem_open(genPath("BLOCKED_WRITERS_PATH",i),
						O_CREAT | O_EXCL, 0600, attr->mq_maxmsg))
				&& i < 1000; i++ )
			if(localinfo->blocked_writers == SEM_FAILED && errno != EEXIST)
				return -1;
		strcpy(attr->blocked_writers, genPath("BLOCKED_WRITERS_PATH",i));
		//Reader initialised at 0
		for(i = 0; SEM_FAILED==(localinfo[fd].blocked_readers = sem_open(genPath("BLOCKED_READERS_PATH",i), O_CREAT | O_EXCL, 0600, 0))
				&& i < 1000; i++ )
			if(localinfo->blocked_readers == SEM_FAILED && errno != EEXIST)
				return -1;
		strcpy(attr->blocked_readers, genPath("BLOCKED_READERS_PATH",i));
		//Mutex for queue operation (send, receive and notify)
		for(i = 0; SEM_FAILED==(localinfo[fd].queue_sem       = sem_open(genPath("QUEUE_SEM_PATH",i), O_CREAT | O_EXCL, 0600, 1))
				&& i < 1000; i++ )
			if(localinfo->queue_sem == SEM_FAILED && errno != EEXIST)
				return -1;
		strcpy(attr->queue_sem, genPath("QUEUE_SEM_PATH",i));
		//Mutex for allocation of msg in the shared memory.
		for(i = 0; SEM_FAILED==(localinfo[fd].search_sem      = sem_open(genPath("SEARCH_SEM_PATH",i) , O_CREAT | O_EXCL, 0600, 1))
				&& i < 1000; i++ )
			if(localinfo->search_sem == SEM_FAILED && errno != EEXIST)
				return -1;
		strcpy(attr->search_sem, genPath("SEARCH_SEM_PATH",i));
		errno = 0;
	} else {
		if(SEM_FAILED == (localinfo->blocked_writers = sem_open(attr->blocked_writers, 0)))
			return -1;
		if(SEM_FAILED == (localinfo->blocked_readers = sem_open(attr->blocked_readers, 0)))
			return -1;
		if(SEM_FAILED == (localinfo->queue_sem = sem_open(attr->queue_sem, 0)))
			return -1;
		if(SEM_FAILED == (localinfo->search_sem = sem_open(attr->search_sem, 0)))
			return -1;
	}

	//Setting local info
	localinfo[fd].mq_flags = O_NONBLOCK & flags;
	hashLink[fd] = attr;

	return (mqd_t) fd;
}

int mq_close(mqd_t mqd) {
	struct mq_attr *attr = hashLink[mqd];
	if(attr == NULL) {
		errno = EBADF;
		return -1;
	}
	attr = NULL;
	return close(mqd);
}
int mq_unlink(const char* path) {
	//Unlink sem.
	return shm_unlink(path);
}

int mq_getattr(mqd_t mqd, struct mq_attr* v){
	struct mq_attr *attr = hashLink[mqd];
	if(attr == NULL) {
		errno = EBADF;
		return -1;
	}
	v->mq_flags = localinfo[mqd].mq_flags & O_NONBLOCK; // Well normally there is only NONBLOCK;
	v->mq_maxmsg = attr->mq_maxmsg;
	v->mq_msgsize = attr->mq_msgsize;
	v->mq_curmsgs = attr->mq_curmsgs;
	return 0;
}

int mq_setattr(mqd_t mqd, struct mq_attr* newattr, struct mq_attr* oldattr){
	struct mq_attr *attr = hashLink[mqd];
	if(attr == NULL) {
		errno = EBADF;
		return -1;
	}
	if(newattr->mq_flags & ~O_NONBLOCK) {
		errno = EINVAL;
		return -1;
	}
	if(oldattr != NULL)
		mq_getattr(mqd, oldattr);
	localinfo[mqd].mq_flags = newattr->mq_flags;
	return 0;
}

int mq_send(mqd_t mqd, const char* msg_ptr, size_t msg_len, unsigned msg_prio){
	struct mq_attr *attr = hashLink[mqd];
	int i;
	msg *msgtosend;
	msg *insert;
	pid_t subscriber = 0;
	int notification_sig;
	if(attr == NULL) {
		errno = EBADF;
		return -1;
	}
	//Non Bloquing mode, see sem_trywait(3). set errno to EACCES if cannot acquire lock.
	if(localinfo[mqd].mq_flags & O_NONBLOCK && sem_trywait(localinfo[mqd].blocked_writers)) {
		errno = EACCES;
		return -1;
	}
	//Bloquing mode.
	if(!(localinfo[mqd].mq_flags & O_NONBLOCK) && sem_wait(localinfo[mqd].blocked_writers)){
		perror("mq_send");
		return -1;
	}

	//Beginning msgtosend = shm_malloc();
	if(sem_wait(localinfo[mqd].search_sem)){
		perror("mq_send");
		return -1;
	}
	for(i = 0; attr->queue[i].used == 1; i++);
	msgtosend = &(attr->queue[i]);
	msgtosend->used = 1;
	if(sem_post(localinfo[mqd].search_sem)){
		perror("mq_send");
		return -1;
	}
	//End of shm_malloc()
	//
	//Preparing the message.
	msgtosend->length = msg_len < attr->mq_msgsize ? msg_len : attr->mq_msgsize ;
	msgtosend->prio = msg_prio;
	msgtosend->next = NULL;
	memcpy(msgtosend->data, msg_ptr, msgtosend->length);
	//Mutex on queue
	if(sem_wait(localinfo[mqd].queue_sem)){
		perror("mq_send");
		return -1;
	}
	//Search where to insert.
	for(insert = attr->first;
			insert != NULL && insert->next != NULL
		       	&& (insert->next->prio >= msgtosend->prio );
			insert = insert->next) ;
	//Inserting
	if(insert == NULL)
		attr->first = msgtosend;
	else if(insert == attr->first) {
		if(insert->prio < msgtosend->prio) {
			msgtosend->next = insert;
			attr->first = msgtosend;
		} else {
			msgtosend->next = insert->next;
			insert->next = msgtosend;
		}
	} else {
		msgtosend->next = insert->next;
		insert->next = msgtosend;
	}
	// Debug queue.
	//dump_queue(attr);
	//
	// If queue empty preparing to send signal.
	if(attr->mq_curmsgs++ == 0) {
		subscriber = attr->subscriber;
		notification_sig = attr->notification_sig;
	}
	attr->subscriber = 0;
	//Releasing mutex
	if(sem_post(localinfo[mqd].queue_sem)){
		perror("mq_send");
		return -1;
	}
	//releasing a resource to the receivers.
	if(sem_post(localinfo[mqd].blocked_readers)){
		perror("mq_send");
		return -1;
	}
	if(subscriber != 0)
		kill(subscriber, notification_sig);

	return 0;
}

ssize_t mq_receive(mqd_t mqd, char* msg_ptr, size_t msg_len, unsigned* msg_prio){
	struct mq_attr *attr = hashLink[mqd];
	msg *msgread;
	ssize_t sizeRead = msg_len;
	if(attr == NULL) {
		errno = EBADF;
		return -1;
	}
	//Non Bloquing mode, see sem_trywait(3). set errno to EACCES if cannot acquire lock.
	if(localinfo[mqd].mq_flags & O_NONBLOCK && sem_trywait(localinfo[mqd].blocked_readers)) {
		errno = EACCES;
		return -1;
	}
	//Bloquing mode
	if(!(localinfo[mqd].mq_flags & O_NONBLOCK) && sem_wait(localinfo[mqd].blocked_readers)) {
		perror("mq_receive");
		return -1;
	}
	//Mutex on queue
	if(sem_wait(localinfo[mqd].queue_sem)) {
		perror("mq_receive");
		return -1;
	}
	//Retrieve the first message.
	msgread = attr->first;
	attr->first = attr->first->next;
	attr->mq_curmsgs--;
	//Releasing Mutex
	if(sem_post(localinfo[mqd].queue_sem)) {
		perror("mq_receive");
		return -1;
	}

	//Reading the msg.
	if(msgread->length < msg_len)
		sizeRead = msgread->length;
	memcpy(msg_ptr, msgread->data, sizeRead);
	if(msg_prio != NULL)
		*msg_prio = msgread->prio;
	//Releasing resource shm_free(msgread)
	msgread->used = 0;
	//Releasing a resource to writer
	if(sem_post(localinfo[mqd].blocked_writers)) {
		perror("mq_receive");
		return -1;
	}
	return sizeRead;
}

mqd_t mq_notify(mqd_t mqd, const struct sigevent* sevp){
	struct mq_attr *attr = hashLink[mqd];
	if(attr == NULL) {
		errno = EBADF;
		return -1;
	}
	if(attr->subscriber) {
		errno = EBUSY;
		return -1;
	}
	if(sevp->sigev_notify & ~(SIGEV_NONE | SIGEV_SIGNAL)) {
		errno = EINVAL;
		return -1;
	}
	if(sevp->sigev_notify & SIGEV_SIGNAL && (sevp->sigev_signo < 1 || sevp->sigev_signo > 31 
				|| sevp->sigev_signo == SIGKILL || sevp->sigev_signo == SIGSTOP)) {
		errno = EINVAL;
		return -1;
	}
	//Subscribing to the queue.
	sem_wait(localinfo[mqd].queue_sem);
	attr->subscriber = getpid();
	attr->notification_sig = sevp->sigev_signo;
	sem_post(localinfo[mqd].queue_sem);
	return (mqd_t) 0;
}

void dump_queue(struct mq_attr *attr) {
	msg *msg;

	for(msg = attr->first; msg != NULL; msg = msg->next)
		printf("\t(prio: %d)",msg->prio);
	printf("\n");
}


