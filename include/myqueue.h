#define _XOPEN_SOURCE 700

#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <string.h>
#include <strings.h>



typedef char msg_data[32];

typedef int mqd_t;

typedef struct message {
	msg_data data;
	unsigned prio;
	size_t length;
} msg;
	
	
	
struct mq_attr {
	/*Mode de fonctionnement de la file (cf. flags dans la specification de mq_open)*/
	long	mq_flags;
	/*Nombre maximal de messages dans la file (par defaut : 32)*/
	long	mq_maxmsg;	
	/*Taille maximale d'un message (sizeof(message))*/
	long	mq_msgsize;	
	/*Nombre de messages contenus dans la file */
	long	mq_curmsgs;		
	/*Nom du semaphore sur lequel les processus en attente de message bloquent lorsque la file est vide, 
		et sur lequel les processus insérant un message bloquent lorsque la file est pleine*/
	char	blocked_writers[32];
	char	blocked_readers[32];
	/*Nb de pcs bloques sur la file*/
	int		nb_blocked_writers;	
	int		nb_blocked_readers;	
	/*Identifiant du processus en attente d'une notification lors de la prochaine arrivee de message*/
	pid_t	subscriber;
	/* Identifiant du signal envoye au processus 
		enregistre en attente de l'arrivee d'un message dans la file vide */
	int notification_sig;
	/*La file elle-meme*/
	msg queue[32];
};



#ifndef myqueue_h
#define myqueue_h

mqd_t mq_open(const char*, int, int);

int mq_close(mqd_t);
int mq_unlink(const char*);

int mq_getattr(mqd_t, struct mq_attr*);
int mq_setattr(mqd_t, struct mq_attr*, struct mq_attr*);

int mq_send(mqd_t, const char*, size_t, unsigned);

ssize_t mq_receive(mqd_t, char*, size_t, unsigned*);

mqd_t mq_notify(mqd_t, const struct sigevent*);

#endif