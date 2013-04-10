#include <stdio.h>
#include <map>
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
#include <stack>

using namespace std;

typedef struct {
uint32_t type; // must be equal to 5
uint32_t server_id;
uint32_t view;
uint32_t seq;
uint32_t update;
}Proposal;

struct Accept{
uint32_t type; // must be equal to 6
uint32_t server_id;
uint32_t view;
uint32_t seq;
};

typedef struct {
uint32_t type; // must be equal to 7
uint32_t server_id;
uint32_t seq;
uint32_t update;
}Globally_Ordered_Update;

typedef struct{
	Proposal* latest_proposal_accepted;
	map<int,Accept*> Accepts;
	Globally_Ordered_Update* g_ordered_update;
}Global_Slot;




int main()
{
	Accept* accept = (Accept*)malloc(sizeof(Accept));
	accept->type = 2;

	printf("Maralo\n");
	accept->type = 8;
	Global_Slot* glb = new Global_Slot;
	
	//glb->Accepts.insert(pair<int, Accept*>(1,accept));
	glb->Accepts[1] = accept;

	if(!glb->latest_proposal_accepted){
		printf("Is null\n");
	}

	if(!glb->g_ordered_update){
	printf("glob is null\n");
	}
	
	printf("%d\n",glb->Accepts[1]->type);

	long int i = -313123123;

	if(i<0)
		printf("i \n");
	

	//glb->Accepts[1] = *accept;
	return 0;	
}
