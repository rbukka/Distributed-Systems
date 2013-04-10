
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
#include <queue>
#include <fstream>
#include <set>
#include <stdlib.h>
#include <stdio.h>
#include <sys/fcntl.h>

#define DEBUG 1
#define IP_SIZE 15
#define HOSTNAME_LEN 80
#define CLIENT_UPDATE_LEN 20
#define BACKLOG 5

#define TYPE_CLI_UPDATE 1

using namespace std;



typedef struct {
uint32_t type; // must be equal to 1
uint32_t client_id;
uint32_t server_id;
uint32_t timestamp;
uint32_t update;
} Client_Update;

void kprintf(const char* s)
{
	if(DEBUG)
		fprintf(stderr,"%s\n", s);
}

void kprintf(const char *s,int value)
{
	if(DEBUG)
		fprintf(stderr,"%s %d\n",s,value);
}

void kprintf(const char *s,char* value)
{
	if(DEBUG)
		fprintf(stderr,"%s %s\n",s,value);
}

void kprintf(const char *s,string value)
{
	if(DEBUG)
		fprintf(stderr,"%s %s\n",s,value.c_str());
}



	

