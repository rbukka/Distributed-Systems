/* UDP server */
#include "Server.h"

using namespace std;

//Maintains the total number of processes in the system.
int global_proc_counter;

//Maintains a map of hostname to its id in the sig structure.
map<char *,int> hostname_to_sigid;

//Maintains a map of the host id to its X509 key used to generate key.
map<int, X509 *> hostid_to_x509;

//Maintains a map of hostname to its id.
map<string,int> hostname_aux_to_sigid;


void host_network(SignedMessage* msg)
{
	msg->type = htonl(msg->type);
	msg->order = htonl(msg->order);

	int maxiter = msg->total_sigs;
	int iter;
	
	for(iter=0;iter<maxiter;iter++)
	{
			msg->sigs[iter].id = htonl(msg->sigs[iter].id);
	}

	msg->total_sigs = htonl(msg->total_sigs);
}

void network_host(SignedMessage* msg,int round)
{
	msg->type = ntohl(msg->type);
	msg->total_sigs = ntohl(msg->total_sigs);
	msg->order = ntohl(msg->order);

	int maxiter=0;
	if(msg->total_sigs>round+1)
		maxiter = round+1;
	else
		maxiter = msg->total_sigs;

	int iter = 0;
	for(iter=0;iter<maxiter;iter++)
	{
			msg->sigs[iter].id = ntohl(msg->sigs[iter].id);
			if(msg->sigs[iter].id < 0 || msg->sigs[iter].id > global_proc_counter-1)
			{
				msg = NULL;
				return;
			}
	}


}

void host_network_ack(Ack* msg)
{
	msg->type = htonl(msg->type);
	msg->round = htonl(msg->round);
}

void network_host_ack(Ack* msg)
{
	msg->type = ntohl(msg->type);
	msg->round = ntohl(msg->round);
}




//Loads all the signatures of each of the hosts into the map hostid_to_x509
void load_signatures()
{
	map<string , int>::iterator it;
	for(it = hostname_aux_to_sigid.begin(); it != hostname_aux_to_sigid.end(); it++){		
		
		//Build the certificate name of the host
		char certfile[80];
		strcpy(certfile,"cert_");
		strcat(certfile,it->first.c_str());
		strcat(certfile,".pem");
	
		X509 * x509;

		FILE*	fp = fopen (certfile, "r");
		if (fp == NULL)
		{
			fprintf(stderr,"Cannot open %s\n ",certfile);
		}

		//Read x509
		x509 = PEM_read_X509(fp, NULL, NULL, NULL);
		fclose (fp);
		if (x509 == NULL) {
			ERR_print_errors_fp (stderr);
			fprintf(stderr,"%s\n","Error in x509");
		}

		fprintf(stderr,"load_signatures id ==> %d\n",it->second);

		//insert into the hostid_to_x509 map
		hostid_to_x509.insert(pair<int, X509 *>(it->second,x509));
		
	}
				
}

//Method to insert a message in the reciever map to be sent in next round
int verify(SignedMessage* msg, map < char *,SignedMessage *> &msg_to_list_processes,char* myhostname)
{

		map<char* , int>::iterator it;

		//Insert into reciever_msg_to_list_processes except me.
		for(it = hostname_to_sigid.begin(); it != hostname_to_sigid.end(); it++){
				if(strcmp(it->first,myhostname) != 0)
					msg_to_list_processes.insert(pair< char *,SignedMessage *>(it->first,msg));
		}	

		int t_sigs = msg->total_sigs;
		int iter = 0;
		map<char *,int>::iterator it_temp;
		map<char *,SignedMessage *>::iterator msg_it_temp;
	
		//Remove all hosts in reciever_msg_to_list_processes which already have the message.
		for(iter=0;iter<t_sigs;iter++)
		{
			int id = msg->sigs[iter].id;
			for(it_temp = hostname_to_sigid.begin(); it_temp != hostname_to_sigid.end(); it_temp++)
			{
				if(it_temp->second == id)
				{
					for(msg_it_temp = msg_to_list_processes.begin(); msg_it_temp != msg_to_list_processes.end(); msg_it_temp++)
					{
						if(strcmp(it_temp->first,msg_it_temp->first) == 0)
							msg_to_list_processes.erase(msg_it_temp->first);
					}
				}
			}
			
		}

	return 1;
}

//Method to verify the signatures of a message recieved.
int verify_new_msg(SignedMessage *mesg)
{
	
	int iter=0;
	int t_max = mesg->total_sigs-1;
	
	EVP_MD_CTX md_ctx;
	EVP_PKEY * pkey;
	int sig_len;
	int err;
	unsigned char sig_buf [SIG_SIZE];

	for(iter=mesg->total_sigs-1;iter>0;iter--)
	{
		int curr_id = mesg->sigs[iter].id;
		

		X509* x509;
		if(hostid_to_x509.find(curr_id) != hostid_to_x509.end())
			x509 = hostid_to_x509.at(curr_id);
		else
			return 0;
		
//		char *data = (char *)(malloc(SIG_SIZE*t_max));
//		char *data = (char *)malloc(sizeof(struct sig)*t_max);
//		memcpy(data,mesg->sigs,SIG_SIZE*t_max);
//		memcpy(data,mesg->sigs,sizeof(struct sig)*t_max);

		t_max--;
	
		if (x509 == NULL) {
			ERR_print_errors_fp (stderr);
			fprintf(stderr,"Error in x509\n");
			return 0;
		}
		pkey=X509_get_pubkey(x509);
	
		if (pkey == NULL) {
			ERR_print_errors_fp (stderr);
			fprintf(stderr,"Error in pkey \n");
			return 0;
		}
	
		EVP_VerifyInit (&md_ctx, EVP_sha1());
//		EVP_VerifyUpdate (&md_ctx, data, (SIG_SIZE*t_max));
//		EVP_VerifyUpdate (&md_ctx, data, sizeof(struct sig)*t_max);
		
		EVP_VerifyUpdate (&md_ctx, mesg->sigs[iter-1].signature, SIG_SIZE);

		sig_len = SIG_SIZE;
		err = EVP_VerifyFinal (&md_ctx, mesg->sigs[iter].signature,SIG_SIZE, pkey);
		EVP_PKEY_free (pkey);
	
		if (err != 1) {
			ERR_print_errors_fp (stderr);
			fprintf(stderr,"Error in verify %d \n",iter );
			return 0;
		}
	}
	
	if(iter==0)
	{
		int curr_id = mesg->sigs[iter].id;
		X509* x509;
		x509 = hostid_to_x509.at(curr_id);
		
		if (x509 == NULL) {
			ERR_print_errors_fp (stderr);
			fprintf(stderr,"Error in x509\n");
			return 0;
		}
		pkey=X509_get_pubkey(x509);
	
		if (pkey == NULL) {
			ERR_print_errors_fp (stderr);
			fprintf(stderr,"Error in pkey \n");
			return 0;
		}
	
		EVP_VerifyInit (&md_ctx, EVP_sha1());
		EVP_VerifyUpdate(&md_ctx,&mesg->order,sizeof(uint32_t));
		sig_len = SIG_SIZE;
		err = EVP_VerifyFinal (&md_ctx, mesg->sigs[iter].signature,SIG_SIZE, pkey);
		EVP_PKEY_free (pkey);
	
		if (err != 1) {
			ERR_print_errors_fp (stderr);
			fprintf(stderr,"Error in verify\n");
			return 0;
		}
	
	}
	
	return 1;
}

int choice(vector<int> liet_val)
{
	fprintf(stderr,"Size of set of values %ld\n",liet_val.size());
	if(liet_val.size()==0 || liet_val.size()>=2)
		return 0;
	else if(liet_val.size()==1)
	{
		if(liet_val.at(0) == 1)
			return 1;
		else
			return 0;
	}
}


SignedMessage* sign_order(int order,char* myhostname,char* dup_myhostname)
{
	SignedMessage* temp_signed = (SignedMessage *)malloc(sizeof(SignedMessage)+sizeof(struct sig));
	temp_signed->order = order;
	temp_signed->type = TYPE_SIGN;
	temp_signed->total_sigs = 1;	

	int curr_index = temp_signed->total_sigs-1;	
	//To do: Add a signature in the signature array
	map<char* , int>::iterator it;
	for(it = hostname_to_sigid.begin(); it != hostname_to_sigid.end(); it++){
				if(strcmp(it->first,myhostname) == 0)
				{
						temp_signed->sigs[temp_signed->total_sigs-1].id = it->second;
				}
	}			

	int err;
	unsigned int sig_len;
	unsigned char sig_buf [SIG_SIZE];
	
	char certfile[80];
	strcpy(certfile,"cert_");
	strcat(certfile,dup_myhostname);
	strcat(certfile,".pem");


	char keyfile[80];
	strcpy(keyfile,"key_");
	strcat(keyfile,dup_myhostname);
	strcat(keyfile,".pem");


	EVP_MD_CTX md_ctx;
	EVP_PKEY * pkey;
	FILE * fp;
	X509 * x509;
	
	// Just load the crypto library error strings 
	ERR_load_crypto_strings();
	
	// Read private key 
	fp = fopen (keyfile, "r");
	
	if (fp == NULL) 
	{
		fprintf(stderr,"Error in sign_order file not found %s\n",certfile);
		return NULL;
	}

	pkey = PEM_read_PrivateKey(fp, NULL, NULL, NULL);
	fclose (fp);
	
	if (pkey == NULL) {
		fprintf(stderr,"pkey is null\n");
		ERR_print_errors_fp (stderr);
		return NULL;
	}
	// Do the signature 
	EVP_SignInit (&md_ctx, EVP_sha1());	
	EVP_SignUpdate (&md_ctx, &order,sizeof(uint32_t));

	sig_len = SIG_SIZE;

	err = EVP_SignFinal (&md_ctx, sig_buf, &sig_len, pkey);

	if (err != 1) {
		fprintf(stderr,"Error in signing\n");
		ERR_print_errors_fp(stderr);
		return NULL;
	}
	EVP_PKEY_free (pkey);
	memcpy(temp_signed->sigs[curr_index].signature,sig_buf,SIG_SIZE);	

	return temp_signed;
	

}

SignedMessage* sign_message(SignedMessage* msg,char* myhostname,char* dup_myhostname)
{
	SignedMessage* temp_signed = (SignedMessage *)malloc(sizeof(SignedMessage)+(msg->total_sigs+1)*sizeof(struct sig));
	temp_signed->order = msg->order;
	temp_signed->type = TYPE_SIGN;
	temp_signed->total_sigs = msg->total_sigs+1;
	
	memcpy(temp_signed->sigs,msg->sigs,(sizeof(struct sig))*msg->total_sigs);

	//To do copy all the signature from msg into temp_signed
	/*
	char *data = (char *)malloc((msg->total_sigs)*sizeof(struct sig));
	memcpy(data,msg->sigs,(msg->total_sigs)*sizeof(struct sig));
	*/
	int curr_index = temp_signed->total_sigs-1;

	fprintf(stderr,"Curr_index %d\n",curr_index);
	
	//To do: Add a signature in the signature array
	map<char* , int>::iterator it;
	for(it = hostname_to_sigid.begin(); it != hostname_to_sigid.end(); it++){
				if(strcmp(it->first,myhostname) == 0)
				{
						temp_signed->sigs[temp_signed->total_sigs-1].id = it->second;
				}
	}			

	int err;
	unsigned int sig_len;
	unsigned char sig_buf [SIG_SIZE];
	
	char certfile[80];
	strcpy(certfile,"cert_");
	strcat(certfile,dup_myhostname);
	strcat(certfile,".pem");

	char keyfile[80];
	strcpy(keyfile,"key_");
	strcat(keyfile,dup_myhostname);
	strcat(keyfile,".pem");

	EVP_MD_CTX md_ctx;
	EVP_PKEY * pkey;
	FILE * fp;
	X509 * x509;
	
	// Just load the crypto library error strings 
	ERR_load_crypto_strings();
	
	// Read private key 
	fp = fopen (keyfile, "r");
	
	if (fp == NULL) 
	{
		fprintf(stderr,"Error in file %s\n",keyfile);
		return NULL;
	}

	pkey = PEM_read_PrivateKey(fp, NULL, NULL, NULL);
	fclose (fp);
	
	if (pkey == NULL) {
		fprintf(stderr,"Error in pkey\n");
		ERR_print_errors_fp (stderr);
		return NULL;
	}
	// Do the signature 
	EVP_SignInit (&md_ctx, EVP_sha1());
//	EVP_SignUpdate (&md_ctx, data, (msg->total_sigs)*sizeof(struct sig));

	EVP_SignUpdate (&md_ctx, msg->sigs[msg->total_sigs-1].signature, SIG_SIZE);
	sig_len = SIG_SIZE;
	sig_len = SIG_SIZE;
	err = EVP_SignFinal (&md_ctx, temp_signed->sigs[curr_index].signature, &sig_len, pkey);

	if (err != 1) {
		ERR_print_errors_fp(stderr);
		fprintf(stderr,"Error in signing\n");
		return NULL;
	}
	EVP_PKEY_free (pkey);
//	memcpy(temp_signed->sigs[curr_index].signature,sig_buf,SIG_SIZE);	
	
	return temp_signed;

}

int main(int argc, char**argv)
{

			//Iterator
		  int i;

		  //Port number
		  int port;

		  //Host file
		  string hostfile;

		  //faulty 
		  int faulty;

		  //verify flag
		  int vflag=1;

		  //verify signatures
		  int cflag=0;

		  //order
		  int order=0;

		  //malicious order
		  int order_copy = 0;

		  //global_bytes_read
		  int global_bytes_n = 0;

		  for(i=0;i<argc;i++)
		  {
					 string arg = argv[i];
					 if(arg == "-p")
					 {					 			
					 			if(argv[i+1])
								{
									string temp = argv[i+1];
									port = atoi(temp.c_str());
									if(port == 0 || port<1024 || port>65535)
									{
										printf("Invalid port. Please specify a port greater than 1024\n");
										exit(EXIT_FAILURE);
									}
									i = i + 1;
								}
								else
								{
									printf("Please specify port number\n");
									exit(EXIT_FAILURE);
								}
					 }

					 if(arg == "-h")
					 {	
					 			if(argv[i+1])
								{
									hostfile = argv[i+1];
									i = i + 1;
								}
								else
								{
									printf("Please specify host file\n");
									exit(EXIT_FAILURE);
								}
					 }

					 if(arg == "-f")
					 {
					 			if(argv[i+1])
								{
									string temp = argv[i+1];
									faulty = atoi(temp.c_str());
									i = i + 1;
									if(faulty < 0)
									{
										printf("Number of faulty process should be non-negative \n");
										exit(EXIT_FAILURE);
									}
								}
								else
								{
									printf("Please specify number of faulty processes\n");
									exit(EXIT_FAILURE);
								}
					 }

					 if(arg == "-c")
					 {
								vflag=0;
					 }

					 if(arg == "-o")
					 {
								cflag = 1;
								if(argv[i+1])
								{
									if(strcmp(argv[i+1],ATTACK) == 0)
										order = 1;
									else if(strcmp(argv[i+1],RETREAT) == 0)
										order = 0;
									else
									{
										printf("Order should be either \'attack' or \'retreat'\n");
										exit(EXIT_FAILURE);
									}
								}
								else
								{
									printf("Please specify an order\n");
									exit(EXIT_FAILURE);
								}

					 }
		  }

			if(DEBUG)
			{
			  cout<<"Port: "<<port<<endl;
			  cout<<"Hostfile: "<<hostfile<<endl;
			  cout<<"Faulty: "<<faulty<<endl;
			  cout<<"VFlag: "<<vflag<<endl;
			  cout<<"CFlag: "<<cflag<<endl;
			  cout<<"Order: "<<order<<endl;
			  order_copy = order;
			}
			char myhostname[HOSTNAME_LEN];
			char dup_myhostname[HOSTNAME_LEN];	

			gethostname(myhostname,HOSTNAME_LEN);
			strcpy(dup_myhostname,myhostname);

			struct hostent *hplocal;
			hplocal = gethostbyname(myhostname);

			if(!hplocal)
			{
				fprintf(stderr,"%s %s\n",myhostname,"local not found");
				exit(EXIT_FAILURE);
			}

			memset(myhostname,0,HOSTNAME_LEN);
		
			if((inet_ntoa(*(struct in_addr *)hplocal->h_addr_list[0])))
			{
				char* s_local = inet_ntoa(*(struct in_addr *)hplocal->h_addr_list[0]); 
				fprintf(stderr,"Host length Dup host name %s %ld %s\n", s_local, strlen(s_local),dup_myhostname);

				if(compare(s_local,S_LOCAL))
					strcpy(myhostname,inet_ntoa(*(struct in_addr *)hplocal->h_addr_list[0]));
				else
					strcpy(myhostname,inet_ntoa(*(struct in_addr *)hplocal->h_addr_list[1]));
			}
			else
				strcpy(myhostname,inet_ntoa(*(struct in_addr *)hplocal->h_addr_list[1]));


			fprintf(stderr,"My name is: %s \n",myhostname);

			int maxhosts = MAX_HOSTS;
			char *hostnames[maxhosts];
			int flag_hostnames[maxhosts];

			int len = 0;

			char hostname[HOSTNAME_LEN];

			FILE *host_file;
			host_file = fopen(hostfile.c_str(),"rt");

			if(host_file == NULL)
			{
				printf("INVALID HOST FILE\n");
				exit(EXIT_FAILURE);
			}


			while(fgets(hostname,HOSTNAME_LEN,host_file) != NULL)
			{
							hostnames[len]=(char *)malloc(sizeof(char)*HOSTNAME_LEN);
							flag_hostnames[len]  = 0;

							int i=0;
							while(i<HOSTNAME_LEN)
							{
											if(hostname[i]=='\n')
											{
															hostname[i]='\0';
															break;
											}
											i++;
							}
							
							string temp(hostname);
							hostname_aux_to_sigid.insert(pair <string,int> (temp,len)) ;

							//Get IP address
							struct hostent *hp;
							hp = gethostbyname(hostname);

							if(!hp)
							{
											fprintf(stderr,"%s %s\n",hostname,"not found");
											exit(EXIT_FAILURE);
							}
							if((inet_ntoa(*(struct in_addr *)hplocal->h_addr_list[0])))
							{
								char* s_local = inet_ntoa(*(struct in_addr *)hplocal->h_addr_list[0]); 
								fprintf(stderr,"Host length %s %zd\n", s_local, strlen(s_local));

								if(compare(s_local,S_LOCAL))
									strcpy(hostnames[len],inet_ntoa(*(struct in_addr *)hplocal->h_addr_list[0]));
								else
									strcpy(hostnames[len],inet_ntoa(*(struct in_addr *)hplocal->h_addr_list[1]));
							}
							else
									strcpy(hostnames[len],inet_ntoa(*(struct in_addr *)hplocal->h_addr_list[1]));

//							strcpy(hostnames[len],inet_ntoa(*(struct in_addr *)hp->h_addr_list[0]));	
							fprintf(stderr,"%s ",hostnames[len]);
							hostname_to_sigid[hostnames[len]]= len;			
							len++;
			}
			flag_hostnames[len]=0;

			fprintf(stderr,"Len %d\n",len);		
			global_proc_counter = len;

				
			if(vflag==1)
				load_signatures();


			//Assuming the maximum size of any message to be (256+4)*number of hosts + 3 from the structure definition  
			int max_signed_msg_size = (MSG_SIZE+sizeof(int))*len + 3*sizeof(int); 

			if(len < faulty+2)
			{
				fprintf(stderr,"Total number of processes must be greater than faulty+2\n");
				exit(EXIT_FAILURE);
			}

			fprintf(stderr,"Size of map %ld\n",hostname_to_sigid.size());

			fclose(host_file);	

			int sockfd,n;
			struct sockaddr_in servaddr,cliaddr;
			socklen_t lensock;
			char *mesg = (char *)malloc(max_signed_msg_size);

			sockfd=socket(AF_INET,SOCK_DGRAM,0);

			if(sockfd == -1)
			{
				printf("Could not create socket\n");
				exit(EXIT_FAILURE);
			}

			bzero(&servaddr,sizeof(servaddr));
			servaddr.sin_family = AF_INET;
			servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
			servaddr.sin_port=htons(port);

			int bval = bind(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));

			if(bval==-1)
			{
				printf("Bind failed\n");
				exit(EXIT_FAILURE);
			}


			if(strcmp(myhostname,hostnames[0]) == 0)
			{
							//Commander
							//

							//Round Timer
							//
							fprintf(stderr,"Starting Round Timer Commander\n");
							SignedMessage* order_msg = sign_order(order,myhostname,dup_myhostname);

							//host to networ//host to networkk			
							host_network(order_msg);
												

							struct timeval time_start,time_curr;
							get_now(&time_start);							

							while(time_to_seconds(&time_start,get_now(&time_curr)) <= ROUND_TIME)
							{
											//printf(" Round Timer: %lf\n",time_to_seconds(&time_start,get_now(&time_curr)));
											int iter;
															

											for (iter=1;iter<len;iter++)
											{
															if(flag_hostnames[iter] == 0)
															{
																cliaddr.sin_family = AF_INET;
																cliaddr.sin_addr.s_addr = inet_addr(hostnames[iter]);
																cliaddr.sin_port = htons(port);
																n = sendto(sockfd,(char *)order_msg,sizeof(SignedMessage)+sizeof(struct sig),0,(struct sockaddr *)&cliaddr,sizeof(cliaddr));											
																fprintf(stderr,"%s  %s %d %d\n","======SENT=====",hostnames[iter],order_msg->order,n);																		
															}
											}

											//ACK Timer
											struct timeval time_ack,time_curr_ack;
											get_now(&time_ack);

											while(time_to_seconds(&time_ack,get_now(&time_curr_ack)) <= UDP_RETRANSMIT_TIMER)
											{
												//printf(" Ack Timer  %lf\n",time_to_seconds(&time_ack,get_now(&time_curr_ack)));
												socklen_t lensock = sizeof(cliaddr);
							
												if((n = recvfrom(sockfd,mesg,sizeof(Ack),MSG_DONTWAIT,(struct sockaddr *)&cliaddr,&lensock))< 0)
												{
													global_bytes_n += n;	
													//printf("Error code: %d\n",n);
													//sleep(SLEEP_TIME);
												}
																						
												Ack* ack_mesg = (Ack *)mesg;
												network_host_ack(ack_mesg);

												if(n>0)
													fprintf(stderr,"Received commander%d\n",n);
													
												if(n<=SIZE_ACK && n>0 && ack_mesg->type == TYPE_ACK) 
												{
													fprintf(stderr,"Recieved ACK from %s %d \n",inet_ntoa(cliaddr.sin_addr),n);
													int iter=1;
													for(iter=1;iter<len;iter++)
													{
															fprintf(stderr,"Ack from %s %d",inet_ntoa(cliaddr.sin_addr),strcmp(hostnames[iter],inet_ntoa(cliaddr.sin_addr)));
															if(strcmp(hostnames[iter],inet_ntoa(cliaddr.sin_addr)) == 0)
																flag_hostnames[iter] = 1;
													}
													//printf("Exiting ACK LOOP\n");
												}
											}
											//printf("Exiting Round Loop\n");
							}

			}
			else
			{
							//Lieutenant sleeps for a rounds time to maintain synchrony
//							sleep(SLEEP_TIME);
							//Global value set lieutenant
							vector<int> liet_val;

							//Map to store the messages and list of processes to which each message is to be sent
							map< char *,SignedMessage *> sender_msg_to_list_processes;						
							map< char *,SignedMessage *> reciever_msg_to_list_processes;


							//Lieutenant
							//
							//
							//
							//RoundTimer
							//
							//
							fprintf(stderr,"Starting Round Timer Lietenant\n");

							struct timeval time_start,time_curr;
							get_now(&time_start);

							int round = 0;							
							for(round=0;round<faulty+2;round++)
							{
								int flag = 0;
								if(round == 0)
								{
									while(time_to_seconds(&time_start,get_now(&time_curr)) <= ROUND_TIME && flag == 0)
									{							
											socklen_t lensock = sizeof(cliaddr);
											n = recvfrom(sockfd,mesg,max_signed_msg_size,MSG_WAITALL,(struct sockaddr *)&cliaddr,&lensock);		
											global_bytes_n += n;

											if(n>0)
												fprintf(stderr,"Received Lieutenant %d\n",n);

											//printf("-------------------------------------------------------\n");
											if(n<=SIZE_ACK && n>0)
											{
															Ack* msg= (Ack *)mesg;
															network_host_ack(msg);
															fprintf(stderr,"Received Ack:\n");
															fprintf(stderr,"Bytes recv %d %s %d\n",msg->type,inet_ntoa(cliaddr.sin_addr),msg->round);
															fprintf(stderr,"-------------------------------------------------------\n");						 
											}
											else if(n>0 && n>SIZE_ACK)
											{
															SignedMessage* msg = (SignedMessage *)mesg;
															network_host(msg,round);

															if(msg != NULL)
															{
																fprintf(stderr,"Received Signed Message:\n");
																fprintf(stderr,"Bytes recv %d %s %d %d %d\n",msg->type,inet_ntoa(cliaddr.sin_addr),n,msg->total_sigs,msg->order);
																fprintf(stderr,"-------------------------------------------------------\n");	

																int verify_out=0;
	
																if(vflag==1)
																	verify_out = verify_new_msg(msg);
																else
																	verify_out = 1;

																//Populate values of lietenant															
																//
																if(liet_val.empty() && (verify_out) && (msg->total_sigs == round+1) && (msg->type == TYPE_SIGN) && (msg->order==0 || msg->order ==1))
																{
																	verify(msg,reciever_msg_to_list_processes,myhostname);
																	fprintf(stderr,"%s %d\n","Populating liet_val",msg->order);
																	liet_val.push_back(msg->order);
																}
																else if(liet_val.empty() && msg->type == TYPE_SIGN && (verify_out) && (msg->order==0 || msg->order==1) && msg->total_sigs>1 && msg->total_sigs  < global_proc_counter)
																{
																	verify(msg,reciever_msg_to_list_processes,myhostname);
																	fprintf(stderr,"%s %d\n","Populating liet_val",msg->order);
																	liet_val.push_back(msg->order);
																	round = msg->total_sigs;

																}
																else if(liet_val.empty() == false)
																{
																	flag = 1;
																}

															}
															//Send Ack
															Ack* ack_msg = (Ack*)malloc(sizeof(Ack));
															ack_msg->type = TYPE_ACK;
															ack_msg->round = round;
															
															host_network_ack(ack_msg);

															n = sendto(sockfd,(char *)ack_msg,sizeof(Ack),0,(struct sockaddr *)&cliaddr,sizeof(cliaddr));
																//sleep(SLEEP_TIME);
															fprintf(stderr,"%s\n","Sent ACK");
											}
	
																				//printf("Exiting Round Loop Lietenant - Round: \n");
									}
									fprintf(stderr,"In Round %d\n",round);
									fprintf(stderr,"Size of sender list %ld\n",sender_msg_to_list_processes.size());
									fprintf(stderr,"Size of Receiver list %ld\n",reciever_msg_to_list_processes.size());						

								}
								else
								{
												fprintf(stderr,"In Round %d\n",round);
												fprintf(stderr,"Size of sender list %ld\n",sender_msg_to_list_processes.size());
												fprintf(stderr,"Size of Receiver list %ld\n",reciever_msg_to_list_processes.size());

												struct timeval time_start,time_curr;
												get_now(&time_start);

												while(time_to_seconds(&time_start,get_now(&time_curr)) <= ROUND_TIME)
												{			
																				
													//Sign and send All messages for this round
													//
													//
													map < char* , SignedMessage *> :: iterator msg_to_list_processes_iter;

													for(msg_to_list_processes_iter = sender_msg_to_list_processes.begin() ; msg_to_list_processes_iter != sender_msg_to_list_processes.end(); ++msg_to_list_processes_iter)
													{
																//printf("In send loop\n");
																char *hostname = 	msg_to_list_processes_iter->first;

																SignedMessage *curr_msg = msg_to_list_processes_iter->second;
																
																if(!curr_msg)
																	fprintf(stderr,"Current Message not null\n");


																if(curr_msg->total_sigs == round)
																{
																	SignedMessage *curr_msg_updated = sign_message(curr_msg,myhostname,dup_myhostname);
																	fprintf(stderr,"Current msg total sigs %d %d \n", curr_msg->total_sigs,curr_msg_updated->total_sigs);

																	if(curr_msg_updated != NULL)
																	{
																	cliaddr.sin_family = AF_INET;
																	cliaddr.sin_addr.s_addr = inet_addr(hostname);
																	cliaddr.sin_port = htons(port);
																	
																	size_t s = sizeof(SignedMessage) + sizeof(struct sig)*(curr_msg_updated->total_sigs);
																	host_network(curr_msg_updated);						

																	n = sendto(sockfd,(char *)curr_msg_updated,s,MSG_DONTWAIT,(struct sockaddr *)&cliaddr,sizeof(cliaddr));
																
																	fprintf(stderr,"Sent****** %d\n",n);
																	fprintf(stderr,"Error: %s (%d)\n", strerror(errno), errno);

																	}
																}
													}
												
													//Start Ack Timer
													//ACK Timer
													struct timeval time_ack,time_curr_ack;
													get_now(&time_ack);

													while(time_to_seconds(&time_ack,get_now(&time_curr_ack)) <= UDP_RETRANSMIT_TIMER)
													{
															//Receive Messages	
															socklen_t lensock = sizeof(cliaddr);
															if((n = recvfrom(sockfd,mesg,max_signed_msg_size,MSG_DONTWAIT,(struct sockaddr *)&cliaddr,&lensock))< 0)
															{
																//	printf("Recv from %d\n",n);
																	global_bytes_n += n;
																	//sleep(SLEEP_TIME);
															}
															//fprintf(stderr,"Error: %s (%d)\n", strerror(errno), errno);

															if(n<=SIZE_ACK && n>0)
															{
																Ack *msg = (Ack *)mesg;
																network_host_ack(msg);
																fprintf(stderr,"Ack received %d %d %s\n",msg->type,msg->round,inet_ntoa(cliaddr.sin_addr));
																if(msg->round == round && msg->type == TYPE_ACK)
																	sender_msg_to_list_processes.erase(inet_ntoa(cliaddr.sin_addr));
															}
															else if(n>0 && n>SIZE_ACK)
															{
																//process message
																SignedMessage* recv_msg = (SignedMessage *)mesg;
																network_host(recv_msg,round);	
																if(recv_msg != NULL)
																{

																	int verify_out = 0;
																	if(recv_msg)
																	{
																		if(vflag==1 && recv_msg->total_sigs == round+1 && recv_msg->type == TYPE_SIGN)
																			verify_out = verify_new_msg(recv_msg);
																		else
																			verify_out = 1;
																	}
																
																	if(verify_out == 1 && recv_msg->total_sigs == round+1 && recv_msg->type == TYPE_SIGN)
																	{
																			if(find(liet_val.begin(),liet_val.end(),recv_msg->order) == liet_val.end())
																			{
																				fprintf(stderr,"Recieved order %d\n",recv_msg->order);
																				liet_val.push_back(recv_msg->order);
																		
																			//To do verification
																			//if(recv_msg->total_sigs<global_proc_counter)
																				if(recv_msg->total_sigs<faulty+1)
																				{
																					fprintf(stderr,"Appending to recieve list\n");	
																					reciever_msg_to_list_processes.insert(pair<char *,SignedMessage *>(inet_ntoa(cliaddr.sin_addr),recv_msg));
																				}
																			}
																	}
																}
																//Send Ack
																Ack *msg = (Ack *)malloc(sizeof(Ack));
																msg->type = TYPE_ACK;
																msg->round = round;
																host_network_ack(msg);

																n = sendto(sockfd,(char *)msg,sizeof(Ack),0,(struct sockaddr *)&cliaddr,sizeof(cliaddr));
															}

													}
													
												}
									}
									sender_msg_to_list_processes.clear();	
									sender_msg_to_list_processes = reciever_msg_to_list_processes;
									reciever_msg_to_list_processes.clear();


							}

					if(choice(liet_val)==0)	
						printf("%s : Agreed on %s\n",myhostname,RETREAT);
					else
						printf("%s : Agreed on %s\n",myhostname,ATTACK);
			}
			fprintf(stderr,"Exit Route\n");
			if(strcmp(myhostname,hostnames[0]) == 0 && order == 0)
				printf("%s : Agreed on %s\n",myhostname,RETREAT);
			else if(strcmp(myhostname,hostnames[0]) == 0 && order == 1)
				printf("%s : Agreed on %s\n",myhostname,ATTACK);
			fprintf(stderr,"Global Bytes %d\n",global_bytes_n);
		//close(sockfd);		
			
}
