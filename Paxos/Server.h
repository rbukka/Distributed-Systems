
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

#define BACKLOG 5
#define HOSTNAME_LEN 80
#define MAX_HOSTS 10
#define DEBUG 1
#define SIZE_ACK 8

#define ROUND_TIME 100000
#define UDP_RETRANSMIT_TIMER 5000
//#define ROUND_TIME 0.5
//#define UDP_RETRANSMIT_TIMER 0.1
#define TYPE_DM 1
#define TYPE_ACK 2
#define TYPE_SEQ_MSG 3

#define MAX_PACKET_SIZE 20
#define MAX_SIZE 64000

using namespace std;



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

/* Get current time. */
struct timeval* get_now( struct timeval *time) {
	if ( gettimeofday( time, NULL ) != 0 ) {
		fprintf(stderr,"Can't get current time.\n");
	}
	
	//printf("Second %ld Micro %ld\n",time->tv_sec,time->tv_usec);

	return time;
}

/* Convert "struct timeval" to fractional seconds. */
double time_to_seconds ( struct timeval *tstart, struct timeval *tfinish ) {
	double t;

	t = ((tfinish->tv_sec - tstart->tv_sec) + (tfinish->tv_usec - tstart->tv_usec)/pow(10,6))*pow(10,3) ;
	return t;
}

//=============================NEW FOR PROJECT 3==================================

#define TYPE_CLIENT_UPDATE 1
#define TYPE_VIEW_CHANGE 2
#define TYPE_VC_PROOF 3
#define TYPE_PREPARE 4
#define TYPE_PROPOSAL 5
#define TYPE_ACCEPT 6
#define TYPE_GLOBALLY_ORDERED_UPDATE 7
#define TYPE_PREPARE_OK 8

//Map to hold client ID to IP address Mapping
map<int,string> client_id_to_ip;

//Map to hold client id and update timer
map<int,time_t> update_time_for_timer;

//Map to store Hostname to IP address
map<string,string> host_to_id;

//Map to store Hostname to id
map<string,int> hostname_to_id;

//Socket variables for servers
int paxos_sockfd;

//Socket for client
int client_sockfd;

struct sockaddr_in servaddr,cliaddr;

//Paxos Port number
int paxos_port;

//Server Port number
int server_port;

//Progress Timer
struct timeval curr;

typedef struct {
uint32_t type; // must be equal to 1
uint32_t client_id;
uint32_t server_id;
uint32_t timestamp;
uint32_t update;
}Client_Update;

//Map to hold client id to client update
map<int,Client_Update*> client_id_to_update;

typedef struct {
uint32_t type; // must be equal to 2
uint32_t server_id;
uint32_t attempted;
}View_Change;

typedef struct {
uint32_t type; // must be equal to 3
uint32_t server_id;
uint32_t installed;
}VC_Proof;

typedef struct {
uint32_t type; // must be equal to 4
uint32_t server_id;
uint32_t view;
uint32_t local_aru;
}Prepare;

typedef struct {
uint32_t type; // must be equal to 5
uint32_t server_id;
uint32_t view;
uint32_t seq;
Client_Update update;
}Proposal;

typedef struct {
uint32_t type; // must be equal to 6
uint32_t server_id;
uint32_t view;
uint32_t seq;
}Accept;

typedef struct {
uint32_t type; // must be equal to 7
uint32_t server_id;
uint32_t seq;
Client_Update update;
}Globally_Ordered_Update;

typedef struct {
uint32_t type; // must be equal to 8
uint32_t server_id;
uint32_t view;
//
//// The following fields form the data_list
//// structure mentioned in the paper
////
//// a list of proposals
uint32_t total_proposals; // total number of proposals in this message
//// a list of globally ordered updates
uint32_t total_globally_ordered_updates; // total number of globally ordered updates

Proposal *proposals;
////
/// in this message
Globally_Ordered_Update *globally_ordered_updates;
}Prepare_OK;

void Send_VC_Proof();

typedef struct{
	Proposal* latest_proposal_accepted;
	map<int,Accept*> *Accepts;
	Globally_Ordered_Update* g_ordered_update;
}Global_Slot;

void host_network(Client_Update* c)
{	
	kprintf("In host_network Client_Update");
	c->type = htonl(c->type);
	c->client_id = htonl(c->client_id);
	c->server_id = htonl(c->server_id);
	c->timestamp = htonl(c->timestamp);
	c->update = htonl(c->update);
}

void network_host(Client_Update* c)
{		
		kprintf("In network_host Client_Update");
		c->type = ntohl(c->type);
		c->client_id = ntohl(c->client_id);
		c->server_id = ntohl(c->server_id);
		c->timestamp = ntohl(c->timestamp);
		c->update = ntohl(c->update);
}

void host_network(View_Change* p)
{	
		kprintf("In host_network View_Change");
		p->type = htonl(p->type);
		p->server_id = htonl(p->server_id);
		p->attempted = htonl(p->attempted);
}

void network_host(View_Change* p)
{
		kprintf("In network_host View_Change");
		p->type = ntohl(p->type);	
		p->server_id = ntohl(p->server_id);
		p->attempted = ntohl(p->attempted);
}

void host_network(VC_Proof* p)
{
		kprintf("In host_network VC_Proof");
		p->type = htonl(p->type);
		p->server_id = htonl(p->server_id);
		p->installed = htonl(p->installed);
}	

void network_host(VC_Proof* p)
{
		kprintf("In network_host VC_Proof");
		p->type = ntohl(p->type);
		p->server_id = ntohl(p->server_id);
		p->installed = ntohl(p->installed);
}
void host_network(Proposal* p)
{
	  kprintf("In host_network Proposal");	
		p->type = htonl(p->type);
		p->server_id = htonl(p->server_id);
		p->view = htonl(p->view);
		p->seq = htonl(p->seq);
		host_network(&(p->update));
}

void network_host(Proposal* p)
{
		kprintf("In network_host Proposal");
		p->type = ntohl(p->type);
		p->server_id = ntohl(p->server_id);
		p->view = ntohl(p->view);
		p->seq = ntohl(p->seq);
		network_host(&(p->update));
}

void host_network(Prepare* p)
{
		kprintf("In host_network Prepare");
		p->type = htonl(p->type);
		p->server_id = htonl(p->server_id);
		p->view = htonl(p->view);
		p->local_aru = htonl(p->local_aru);
}

void network_host(Prepare* p)
{
		kprintf("In network_host Prepare");
		p->type = ntohl(p->type);
		p->server_id = ntohl(p->server_id);
		p->view = ntohl(p->view);
		p->local_aru = ntohl(p->local_aru);
}

void host_network(Accept* p)
{
		kprintf("In network_host Accept");
		p->type = htonl(p->type);
		p->server_id = htonl(p->server_id);
		p->view = htonl(p->view);
		p->seq = htonl(p->seq);
}

void network_host(Accept* p)
{
		kprintf("In network_host Accept");
		p->type = ntohl(p->type);
		p->server_id = ntohl(p->server_id);
		p->view = ntohl(p->view);
		p->seq = ntohl(p->seq);
}

void host_network(Globally_Ordered_Update* p)
{
		kprintf("In host_network Globally Ordered Update");
		p->type = htonl(p->type);
		p->server_id = htonl(p->server_id);
		p->seq = htonl(p->seq);
		host_network(&(p->update));
}

void network_host(Globally_Ordered_Update* p)
{
		kprintf("In network_host Globally Ordered Update");	
		p->type = ntohl(p->type);
		p->server_id = ntohl(p->server_id);
		p->seq = ntohl(p->seq);
		network_host(&(p->update));
}

void host_network(Prepare_OK* p)
{
		kprintf("In host_network Prepare_OK");
		p->type = htonl(p->type);
		p->server_id = htonl(p->server_id);
		p->view = htonl(p->view);
	
		int iter;
		for(iter=0;iter<p->total_proposals;iter++)
		{
			host_network(&p->proposals[iter]);
		}

		p->total_proposals = htonl(p->total_proposals);

		for(iter=0;iter<p->total_globally_ordered_updates;iter++)
		{
			host_network(&p->globally_ordered_updates[iter]);
		}
		
		p->total_globally_ordered_updates = htonl(p->total_globally_ordered_updates);
}

void network_host(Prepare_OK* p)
{
		kprintf("In network_host Prepare_OK");
		p->type = ntohl(p->type);
		p->server_id = ntohl(p->server_id);
		p->view = ntohl(p->view);
		p->total_globally_ordered_updates = ntohl(p->total_globally_ordered_updates);
		p->total_proposals = ntohl(p->total_proposals);

		int iter;

		for(iter=0;iter<p->total_proposals;iter++)
			network_host(&p->proposals[iter]);

		for(iter=0;iter<p->total_globally_ordered_updates;iter++)
			network_host(&p->globally_ordered_updates[iter]);
}

//============Server State Variables =====================//

typedef enum
{
	LEADER_ELECTION,
	REG_LEADER,
	REG_NONLEADER
}State; //Server state

State my_state = LEADER_ELECTION;

int My_server_id; //a unique identifier for this server
int N = 0; //Total servers in the system

//===========END Server State Variables =======================//


//===========View State Variables =========================//


int	Last_Attempted; //the last view this server attempted to install
int Last_Installed; //the last view this server installed
map<int,View_Change*> VC;	//array of View_Change messages, indexed by server_id


//===========END View State Variables ========================//

//==========Prepare Phase Variables =========================//

Prepare* last_prepare; //the prepare message from the last preinstalled view, if received.
map<int,Prepare_OK*> Prepare_oks; //array of Prepare_OK messages received, indexed by server_id

//=========END Prepare Phase Variables =====================//

//=========Global Ordering Variables ======================//

int Local_Aru; // the local aru value of this server
int Last_Proposed; //last sequence number proposed by the leader
map<int,Global_Slot*> global_history;

//=========END Global Ordering Variables ==================//

//=========Timers Variables================================//

int Progress_Timer = 300;
double Update_Timer	 = 150.0;
int progressTimer  =   0;

//=========END Timers Variables============================//

//=========Client Handling Variables=======================//

queue<Client_Update*> Update_Queue; //queue of Client_Update messages
map<int,int> Last_Executed;			//array of timestamps, indexed by client_id
map<int,int> Last_Enqueued;			//array of timestamps, indexed by client_id
map<int,Client_Update*> Pending_Updates;	//array of Client_Update messages, indexed by client_id

//=========END Client Handling Variables===================//

bool Conflict(char* mesg,int type)
{
	kprintf("In Conflict");
	switch(type)
	{
		case TYPE_VIEW_CHANGE:
		{
				View_Change* v_change = (View_Change*)mesg;
				int server_id = v_change->server_id;
				int attempted = v_change->attempted;
				if(server_id == My_server_id)
					return true;
				if(my_state != LEADER_ELECTION)
					return true;
				if(progressTimer == 1)
					return true;
				if(v_change->attempted < Last_Installed)
					return true;
				return false;
		}

		case TYPE_VC_PROOF:
		{
			VC_Proof* vc_proof = (VC_Proof*)mesg;
			if(vc_proof->server_id == My_server_id)
				return true;
			if(my_state != LEADER_ELECTION)
				return true;
			return false;
		}

		case TYPE_PREPARE:
		{
			Prepare* prepare = (Prepare*)mesg;
			if(prepare->server_id == My_server_id)
				return true;
			if(prepare->view != Last_Attempted)
				return true;
			return false;
		}

		case TYPE_PREPARE_OK:
		{
			Prepare_OK* prepare_ok = (Prepare_OK*)mesg;
			if(my_state != LEADER_ELECTION)
				return true;
			if(prepare_ok->view != Last_Attempted)
				return true;
			return false;
		}
		
		case TYPE_PROPOSAL:
		{
			Proposal* proposal = (Proposal*)mesg;
			if(proposal->server_id == My_server_id)
				return true;
			if(my_state != REG_NONLEADER)
				return true;
			if(proposal->view != Last_Installed)
				return true;
			return false;
		}

		case TYPE_ACCEPT:
		{
			Accept* accept = (Accept*)mesg;
			if(accept->server_id != My_server_id)
				return true;
			if(accept->view != Last_Installed)
				return true;			
			if(global_history.find(accept->seq) != global_history.end() && global_history[accept->seq])
			{
				if(global_history[accept->seq]->latest_proposal_accepted)
				{
					if(global_history[accept->seq]->latest_proposal_accepted->view != accept->view)
						return true;
				}
			}
			return false;
		}
	}
}

void update_data_structures(void* mesg,int type)
{
	kprintf("In update_data_structures");
	switch(type)
	{
		case TYPE_VIEW_CHANGE:
		{	
			View_Change* v_change = (View_Change*)mesg;
			if(VC.find(v_change->server_id) == VC.end())
				VC[v_change->server_id] = v_change;
			break;
		}
		case TYPE_PREPARE:
		{
			last_prepare = (Prepare*)mesg;
			break;
		}
		case TYPE_PREPARE_OK:
		{
			Prepare_OK* prepare_ok = (Prepare_OK*)mesg;
			if(Prepare_oks.find(prepare_ok->server_id) == Prepare_oks.end())
			{
				Prepare_oks[prepare_ok->server_id] =  prepare_ok;
			}
			int i=0;
			for(i=0;i<prepare_ok->total_proposals;i++)
			{
					update_data_structures((void *)&(prepare_ok->proposals[i]),TYPE_PROPOSAL);		
			}
			for(i=0;i<prepare_ok->total_globally_ordered_updates;i++)
			{
					update_data_structures((void *)&(prepare_ok->globally_ordered_updates[i]),TYPE_GLOBALLY_ORDERED_UPDATE);
			}
			break;
		}
		case TYPE_PROPOSAL:
		{
			Proposal* proposal = (Proposal*)mesg;
			
			if(global_history.find(proposal->seq) != global_history.end() && !(global_history[proposal->seq]))
			{
				global_history[proposal->seq] = (Global_Slot*) malloc(sizeof(Global_Slot));
				memset(global_history[proposal->seq],0,sizeof(Global_Slot));
			}
			
			if(global_history.find(proposal->seq) != global_history.end() && global_history[proposal->seq])
			{
				if(global_history[proposal->seq]->g_ordered_update == NULL)
				{
					if(global_history[proposal->seq]->latest_proposal_accepted != NULL)
					{
						if(proposal->view > global_history[proposal->seq]->latest_proposal_accepted->view)
						{
							global_history[proposal->seq]->latest_proposal_accepted = proposal;
							if(global_history[proposal->seq]->Accepts != NULL)
								global_history[proposal->seq]->Accepts->clear();
						}
					}
				}
			}
			
			
			break;
		}
		case TYPE_ACCEPT:
		{
			Accept* accept = (Accept*)mesg;
		
			if(global_history.find(accept->seq) == global_history.end() && !(global_history[accept->seq])) 
			{
				global_history[accept->seq] = (Global_Slot*)malloc(sizeof(Global_Slot));
				memset(global_history[accept->seq],0,sizeof(Global_Slot));
			}


			if(global_history.find(accept->seq) != global_history.end() && global_history[accept->seq] &&  global_history[accept->seq]->g_ordered_update != NULL)
				return;
			if(global_history.find(accept->seq) != global_history.end() && global_history[accept->seq] && global_history[accept->seq]->Accepts != NULL  &&global_history[accept->seq]->Accepts->size() >= N/2)
				return;
			if(global_history.find(accept->seq) != global_history.end() && global_history[accept->seq] && global_history[accept->server_id] != NULL)
				return;
			if(global_history.find(accept->seq) != global_history.end() && global_history[accept->seq])
				global_history[accept->seq]->Accepts->insert(std::pair<int,Accept*>(accept->server_id,accept));
			break;
		}
		case TYPE_GLOBALLY_ORDERED_UPDATE:
		{
			Globally_Ordered_Update* global = (Globally_Ordered_Update*)mesg;
		
			if(global_history.find(global->seq) == global_history.end() && !(global_history[global->seq]))
			{
				global_history[global->seq] = (Global_Slot*)malloc(sizeof(Global_Slot));
				memset(global_history[global->seq],0,sizeof(Global_Slot));
			}

			if(global_history.find(global->seq) != global_history.end() && global_history[global->seq] && global_history[global->seq]->g_ordered_update ==	NULL)
				global_history[global->seq]->g_ordered_update	= global;
	
		}	
	}
	
}

bool View_Prepared_Ready(int view)
{	
	kprintf("In View_Prepared_Ready");
	int count=0;
	for(map<int,Prepare_OK*>::iterator it=Prepare_oks.begin();it!=Prepare_oks.end();it++)
	{
		Prepare_OK* p = (Prepare_OK*)(it->second);
		if(p->view == view)
			count++;
	}
	if(count>floor(N/2)+1)
		return true;
	else
		return false;
}


bool Preinstall_Ready(int view)
{
	kprintf("In Preinstall_Ready");
	int count=0;
	for(map<int,View_Change*>::iterator it=VC.begin();it!=VC.end();it++)
	{
		View_Change* vc = (View_Change*)(it->second);
		if(vc->attempted == view)
			count++;			
	}

	kprintf("VC count is: ",count);

	if(count>floor(N/2)+1)
		return true;
	else
		return false;		
}

Prepare* Construct_Prepare(int Last_Installed,int aru)
{
	kprintf("In Construct_Prepare");
	Prepare* p = (Prepare*) malloc(sizeof(Prepare));
	p->type = TYPE_PREPARE;
	p->server_id = My_server_id;
	p->view = Last_Installed;
	p->local_aru = aru;
	return p;
	
}

Globally_Ordered_Update* Construct_Globally_Ordered_Update(int seq,int server_id,Client_Update update)
{
	kprintf("In Construct_Globally_Ordered_Update");
	Globally_Ordered_Update* g_update = (Globally_Ordered_Update*)malloc(sizeof(Globally_Ordered_Update));
	g_update->seq = seq;
	g_update->server_id = server_id;
	memcpy(&(g_update->update),&update,sizeof(update));
	return g_update;
}

void Construct_DataList(int aru,vector<Proposal*> *proposal_list,vector<Globally_Ordered_Update*> *global_list)
{
	kprintf("In Construct_DataList");
	proposal_list->clear();
	global_list->clear();

	for(map<int,Global_Slot*>::iterator it=global_history.begin();it!=global_history.end();it++)
	{
		if(it->first > aru)
		{
			Global_Slot* temp = (Global_Slot*)(it->second);
			if(temp->latest_proposal_accepted != NULL)
				proposal_list->push_back(temp->latest_proposal_accepted);
			if(temp->g_ordered_update != NULL)
				global_list->push_back(temp->g_ordered_update);

		}
	}
}

Prepare_OK* Construct_Prepare_OK(int Last_Installed,vector<Proposal*> *proposal_list,vector<Globally_Ordered_Update*> *global_list)
{
	kprintf("In Construct_Prepare_OK");
	Prepare_OK* prepare_ok = (Prepare_OK*)malloc(sizeof(Prepare_OK)+(global_list->size())*sizeof(Globally_Ordered_Update)+(proposal_list->size())*sizeof(Proposal));
	prepare_ok->type = TYPE_PREPARE_OK;
	prepare_ok->view = Last_Installed;
	prepare_ok->total_globally_ordered_updates=global_list->size();
	prepare_ok->total_proposals = proposal_list->size();
	
	Proposal* proposals = (Proposal*)malloc(sizeof(Proposal)*prepare_ok->total_proposals);
	int i;
	for(i=0;i<prepare_ok->total_proposals;i++)
	{
		memcpy(&proposals[i],proposal_list->at(i),sizeof(Proposal));
	}

	prepare_ok->proposals = proposals;
	
	Globally_Ordered_Update* global = (Globally_Ordered_Update*)malloc(sizeof(Globally_Ordered_Update)*prepare_ok->total_globally_ordered_updates);

	for(i=0;i<prepare_ok->total_globally_ordered_updates;i++)
	{
		memcpy(&global[i],global_list->at(i),sizeof(Globally_Ordered_Update));
	}
	
	prepare_ok->globally_ordered_updates = global;

	return prepare_ok;
}


void Shift_to_Prepare_Phase()
{
	kprintf("In Shift_to_Prepare_Phase");
	Last_Installed = Last_Attempted;

	Send_VC_Proof();

	Prepare* c_p = Construct_Prepare(Last_Installed,Local_Aru);
	update_data_structures(c_p,TYPE_PREPARE);
	vector<Proposal*> *proposal_list;
	vector<Globally_Ordered_Update*> *global_list;
	Construct_DataList(Local_Aru,proposal_list,global_list);
	
	Prepare_OK* p_ok = Construct_Prepare_OK(Last_Installed,proposal_list,global_list);
	Prepare_oks[My_server_id] = p_ok;

	Last_Enqueued.clear();

	host_network(c_p);
	
	//Send Prepare to all servers.
	for(map<string,string>::iterator it=host_to_id.begin(); it != host_to_id.end(); it++)
	{
		cliaddr.sin_family = AF_INET;
		cliaddr.sin_addr.s_addr = inet_addr(it->second.c_str());
		cliaddr.sin_port = htons(paxos_port);
		int rval = sendto(paxos_sockfd,c_p,sizeof(Prepare),0,(struct sockaddr *)&cliaddr,sizeof(cliaddr));								
		kprintf("Sent message ",it->second.c_str());
	}
	network_host(c_p);
}

View_Change* Construct_VC(int Last_Attempted)
{
	kprintf("In Construct_VC");
	
	View_Change* vc = (View_Change*)malloc(sizeof(View_Change));
	vc->type = TYPE_VIEW_CHANGE;
	vc->attempted = Last_Attempted;
	vc->server_id = My_server_id;
	return vc;
}

bool Enqueue_Update(Client_Update* U)
{
	kprintf("In Enqueue_Update");

	if(U->timestamp <= Last_Executed[U->client_id])
		return false;
	
	if(U->timestamp <= Last_Enqueued[U->client_id])
		return false;

	Update_Queue.push(U);
	Last_Enqueued[U->client_id] = U->timestamp;
	return true;
}

//check if client update is bound
bool Check_Bound(Client_Update* U)
{
	kprintf("In Check_Bound");
	int u = U->update;

	for(map<int,Global_Slot*>::iterator it = global_history.begin(); it!=global_history.end(); it++)
	{
		if(it->second->g_ordered_update != NULL)
		{
			int g_u = it->second->g_ordered_update->update.update;
			if(u==g_u)
				return true;
		}
	}

	return false;
}


bool Check_if_in_update_queue(Client_Update* U)
{
	kprintf("In Check_if_in_update_queue");
	stack<Client_Update*> temp_queue;
	bool found = false;

	while(!Update_Queue.empty())
	{
		Client_Update* temp = Update_Queue.front();
		temp_queue.push(temp);
		Update_Queue.pop();
		if(temp->client_id == U->client_id && temp->server_id == U->server_id && temp->timestamp == U->timestamp && temp->update == U->update)
			found = true;
	}

	while(!temp_queue.empty())
	{
		Client_Update* temp = temp_queue.top();
		Update_Queue.push(temp);
		temp_queue.pop();
	}

	return found;
}

void Add_to_Pending_Updates(Client_Update* U)
{
	kprintf("In Add_to_Pending_Updates");
	Pending_Updates[U->client_id] = U;
	//Update timer
	time_t currenttime;
	time(&currenttime);
	update_time_for_timer[U->client_id] =currenttime;
}

void Remove_Bound_Updates_From_Queue()
{
	kprintf("In Remove_Bound_Updates_From_Queue");
	stack<Client_Update*> temp_queue;

	while(!Update_Queue.empty())
	{
		int f = 0;
		Client_Update* temp = Update_Queue.front();
		if(Check_Bound(temp) || temp->timestamp <= Last_Executed[temp->client_id] || ( temp->timestamp <= Last_Enqueued[temp->client_id] && temp->server_id!= My_server_id))
		{
			f = 1;
			if(temp->timestamp > Last_Enqueued[temp->client_id])
				Last_Enqueued[temp->client_id] = temp->timestamp;
		}
		
		if(f==1)
			temp_queue.push(temp);
		
		Update_Queue.pop();

	}

	while(!temp_queue.empty())
	{
		Client_Update* temp = temp_queue.top();
		Update_Queue.push(temp);
		temp_queue.pop();
	}
	
}

void Enqueue_Unbound_Pending_Updates()
{
	kprintf("In Enqueue_Unbound_Pending_Updates");
	for(map<int,Client_Update*>::iterator it = Pending_Updates.begin(); it != Pending_Updates.end(); it++)
	{
		Client_Update* c_u = it->second;
		if(Check_Bound(c_u))
		{
				if(Check_if_in_update_queue(c_u))
					Enqueue_Update(c_u);
		}
	}
}

Proposal* Construct_Proposal(int My_server_id,int view,int seq,Client_Update u)
{
	kprintf("In Construct_Proposal");
	Proposal* p = (Proposal*)malloc(sizeof(Proposal));
	p->type = TYPE_PROPOSAL;
	p->server_id = My_server_id;
	p->view = view;
	p->seq = seq;
	memcpy(&(p->update),&u,sizeof(Client_Update));

	return p;
}

bool Globally_Ordered_Ready(int seq)
{
	kprintf("In Globally_Ordered_Ready");
	
	if(global_history.find(seq) != global_history.end() && global_history[seq])
	{
		if(global_history[seq]->latest_proposal_accepted != NULL)
		{
			if(global_history[seq]->Accepts != NULL)
			{
				map<int,Accept*> *accepts = global_history[seq]->Accepts;
				map<int,int> view_count; //view,count
				for(map<int,Accept*>::iterator it=accepts->begin(); it!=accepts->end(); it++)
				{
					int view = it->second->view;
					map<int,int>:: iterator it;
					it = view_count.find(view);

					if(it != view_count.end())
						view_count[view] ++;
					else
						view_count[view] = 0;
				}

				for(map<int,int>:: iterator it = view_count.begin(); it!= view_count.end(); it++)
				{
					if(it->second >= N/2)
						return true;
				}
			}
		}
	}
	return false;
}
void Send_Proposal()
{
	kprintf("In Send_Proposal");
	int seq = Last_Proposed + 1;
	Client_Update u;

	if(global_history.find(seq) != global_history.end() && global_history[seq])
	{
		if(global_history[seq]->g_ordered_update != NULL)
		{
			Last_Proposed++;
			Send_Proposal();
		}
	}

	if(global_history.find(seq) != global_history.end() && global_history[seq])
	{
		if(global_history[seq]->latest_proposal_accepted != NULL)
		{
			memcpy(&u,&(global_history[seq]->latest_proposal_accepted->update),sizeof(Client_Update));
		}
	  else if(Update_Queue.empty())
		{
			return;
		}
		else
		{
			Client_Update* c_update = Update_Queue.front();
			memcpy(&u,&(c_update),sizeof(Client_Update));
			Update_Queue.pop();
		}
	}
	else if(Update_Queue.empty())
	{
		return;
	}
	else
	{
		Client_Update* c_update = Update_Queue.front();
		memcpy(&u,&(c_update->update),sizeof(Client_Update));
		Update_Queue.pop();
	}
	//view doubtful
	Proposal* proposal = Construct_Proposal(My_server_id,Last_Installed,seq,u);
	update_data_structures(proposal,TYPE_PROPOSAL);
	Last_Proposed = seq;

	host_network(proposal);
	//Send Proposal to all servers.
	for(map<string,string>::iterator it=host_to_id.begin(); it != host_to_id.end(); it++)
	{
		cliaddr.sin_family = AF_INET;
		cliaddr.sin_addr.s_addr = inet_addr(it->second.c_str());
		cliaddr.sin_port = htons(paxos_port);
		int rval = sendto(paxos_sockfd,proposal,sizeof(Proposal),0,(struct sockaddr *)&cliaddr,sizeof(cliaddr));								
		kprintf("Sent message ",it->second.c_str());
	}
	network_host(proposal);
}


void Advance_Aru()
{
	kprintf("In Advance_Aru");
	int i = Local_Aru + 1;
	while(1)
	{	
			if(global_history.find(i) != global_history.end() && global_history[i])
			{
				if(global_history[i]->g_ordered_update != NULL)
				{
					Client_Update cli_update;
					memcpy(&cli_update,&(global_history[i]->g_ordered_update->update),sizeof(Client_Update));

					int seq = global_history[i]->g_ordered_update->seq;
	
					//Upon executing a client update
					{

						fprintf(stderr,"%d: Executed update %d from client %d with seq %d and view %d",My_server_id,cli_update.update,cli_update.client_id,seq,Last_Installed);

						if(cli_update.server_id == My_server_id)
						{
							//Reply to client
							//
							kprintf("Sending reply to client");
						

							if(client_id_to_ip.find(cli_update.client_id) != client_id_to_ip.end())
							{
								string ip = client_id_to_ip[cli_update.client_id];
								int rval = send(client_sockfd, "Hello, world!", 13, 0);
								kprintf("Sent reply to client");
							}
							else
							{
								kprintf("Error sending reply to client");
							}

							if(Pending_Updates.find(cli_update.client_id) != Pending_Updates.end() && Pending_Updates[cli_update.client_id])
							{
								update_time_for_timer.erase(cli_update.client_id);
								free(Pending_Updates[cli_update.client_id]);
								Pending_Updates.erase(cli_update.client_id);
							}
							Last_Executed[cli_update.client_id] = cli_update.timestamp;
						}
						
						if(my_state != LEADER_ELECTION)
						{
							//Restart Progress Timer
							get_now(&curr);

						}
						if(my_state == REG_LEADER)
						{	
							Send_Proposal();
						}	
					}
					Local_Aru++;
					i++;
				}
			}
			else
				return;
	}
}


void Client_Update_Handler(Client_Update* U)
{
	kprintf("In Client_Update_Handler");

	if(my_state == LEADER_ELECTION)
	{
		if(U->server_id != My_server_id)
			return;
		if(Enqueue_Update(U))
			Add_to_Pending_Updates(U);
	}

	if(my_state == REG_NONLEADER)
	{
		if(U->server_id == My_server_id)
			Add_to_Pending_Updates(U);
		
		//Send to Leader : U
		int leader = Last_Installed % N;
		int size = sizeof(Client_Update);
		string leader_ip;
		for(map<string,int>::iterator it = hostname_to_id.begin(); it != hostname_to_id.end(); it++)
		{
			if(it->second == leader)
			{
				string leader_host_name = it->first;
				if(host_to_id.find(leader_host_name) != host_to_id.end())
				{
					leader_ip = host_to_id[leader_host_name];
					kprintf("In Client update handler IP",leader_ip);
				}
			}
		}
		cliaddr.sin_family = AF_INET;
		cliaddr.sin_addr.s_addr = inet_addr(leader_ip.c_str());
		cliaddr.sin_port = htons(paxos_port);
		host_network(U);
		int rval = sendto(paxos_sockfd,U,size,0,(struct sockaddr *)&cliaddr,sizeof(cliaddr));								
		kprintf("Sent message ",leader_ip.c_str());																
		network_host(U);									

	}

	if(my_state == REG_LEADER)
	{
		if(Enqueue_Update(U))
		{
			if(U->server_id == My_server_id)
			{
				Add_to_Pending_Updates(U);
			}
			//Send_Proposal
			Send_Proposal();
		}
	}

	}

void Shift_to_Reg_Leader()
{	
	kprintf("In Shift_to_Reg_Leader");

	fprintf(stderr,"%d: Server %d is the new leader of the view %d \n", My_server_id, Last_Installed % N, Last_Installed);

	my_state = REG_LEADER;
	Enqueue_Unbound_Pending_Updates();
	Remove_Bound_Updates_From_Queue();
	Last_Proposed = Local_Aru;
	Send_Proposal();
}

void Shift_to_Reg_Non_Leader()
{
	kprintf("In Shift_to_Reg_Non_Leader");

	my_state = REG_NONLEADER;
	Last_Installed = Last_Attempted;
	
	Send_VC_Proof();

	fprintf(stderr,"%d: Server %d is the new leader of the view %d \n",My_server_id, Last_Installed % N, Last_Installed);
	
	while(!Update_Queue.empty())
	{
		Update_Queue.pop();
	}

	kprintf("Size update queue:",Update_Queue.size());
}





void Shift_to_Leader_Election(int view)
{
	kprintf("In Shift_to_Leader_Election");

	my_state = LEADER_ELECTION;

	VC.clear();
	Prepare_oks.clear();
	last_prepare = NULL;
	Last_Enqueued.clear();
	Last_Attempted = view;

	View_Change* vc = Construct_VC(Last_Attempted);


	host_network(vc);
	//To do
	//send vc to all servers
	
	for(map<string,string>::iterator it=host_to_id.begin(); it != host_to_id.end(); it++)
	{
		cliaddr.sin_family = AF_INET;
		cliaddr.sin_addr.s_addr = inet_addr(it->second.c_str());
		cliaddr.sin_port = htons(paxos_port);
		int rval = sendto(paxos_sockfd,vc,sizeof(View_Change),0,(struct sockaddr *)&cliaddr,sizeof(cliaddr));								
		kprintf("Sent message ",it->second.c_str());
	}
	
	network_host(vc);
	//Apply vc to data structures
	update_data_structures(vc,TYPE_VIEW_CHANGE);

}

void Send_VC_Proof()
{
	kprintf("In VC_Proof");
	VC_Proof* vc_p = (VC_Proof*)malloc(sizeof(VC_Proof));
	vc_p->type = TYPE_VC_PROOF;
	vc_p->server_id = My_server_id;
	vc_p->installed = Last_Installed;

	host_network(vc_p);
	
	for(map<string,string>::iterator it=host_to_id.begin(); it != host_to_id.end(); it++)
	{
		cliaddr.sin_family = AF_INET;
		cliaddr.sin_addr.s_addr = inet_addr(it->second.c_str());
		cliaddr.sin_port = htons(paxos_port);
		int rval = sendto(paxos_sockfd,vc_p,sizeof(VC_Proof),0,(struct sockaddr *)&cliaddr,sizeof(cliaddr));								
		kprintf("Sent message ",it->second.c_str());
	}	
	network_host(vc_p);

}

void Leader_Election()
{
	kprintf("In Leader_Election");
	if(progressTimer == 0)
		Shift_to_Leader_Election(Last_Attempted+1);	
}


	

