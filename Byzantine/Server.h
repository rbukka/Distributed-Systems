
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <iostream>
#include <string>
#include <vector>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <sys/time.h>
#include <math.h>
#include <map>
#include <algorithm>

#define HOSTNAME_LEN 40
#define MAX_HOSTS 1000
#define DEBUG 0
#define SIZE_ACK 8

#define ROUND_TIME 500
#define UDP_RETRANSMIT_TIMER 100
//#define ROUND_TIME 500
//#define UDP_RETRANSMIT_TIMER 100
#define TYPE_SIGN 1
#define TYPE_ACK 2
#define SLEEP_TIME 5
//#define SLEEP_TIME 0
#define MSG_SIZE 256
#define ATTACK "attack"
#define RETREAT "retreat"
#define S_LOCAL "127"

#include <openssl/rsa.h>
#include <openssl/evp.h>
#include <openssl/objects.h>
#include <openssl/x509.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>

#define SIG_SIZE 256 /* For 2048 bit RSA private key */

/*String compare overloaded*/

int compare(char *str1,const char* str2)
{
	//printf("In compare %d\n",strlen(str2));
	int len = strlen(str2);
	int ret = 0;
	int iter = 0;
	for(iter = 0;iter < len;iter++)
	{
	//	printf("str1[iter] ==> str2[iter] %c %c\n",str1[iter],str2[iter]);
		if(str1[iter] != str2[iter])
			ret = 1;
	//	printf("ret %d \n", ret);
	}

	return ret;
}


/* Get current time. */
struct timeval* get_now( struct timeval *time) {
	if ( gettimeofday( time, NULL ) != 0 ) {
		fprintf(stderr,"Can't get current time.\n");
	}
	
//	printf("Second %ld Micro %ld\n",time->tv_sec,time->tv_usec);

	return time;
}

/* Convert "struct timeval" to fractional seconds. */
double time_to_seconds ( struct timeval *tstart, struct timeval *tfinish ) {
	double t;

	t = ((tfinish->tv_sec - tstart->tv_sec) + (tfinish->tv_usec - tstart->tv_usec)/pow(10,6))*pow(10,3) ;
	return t;
}


struct sig {
		  uint32_t id; // the identifier of the signer
		  uint8_t signature[256]; // Since you have to use 2048 bits RSA private key
};

struct SignMessage{
		  uint32_t type; // Must be equal to 1
		  uint32_t total_sigs; // total number of signatures on the message
		  // (also indicates the round number)

		  uint32_t order; // the order (retreat = 0 and attack = 1)
		  struct sig sigs[]; // contains total_sigs signatures
}; 

typedef struct SignMessage SignedMessage;

typedef struct {
		  uint32_t type; // Must be equal to 2
		  uint32_t round; // round number
} Ack;


