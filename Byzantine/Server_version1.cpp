/* Sample UDP server */
#include "Server.h"

using namespace std;

int global_proc_counter;

int verify(SignedMessage* msg, map < SignedMessage, vector<int> > msg_to_list_processes,char* myhostname)
{	
	

	return 1;
}

SignedMessage* sign_order(int order)
{
	SignedMessage* temp_signed = (SignedMessage *)malloc(sizeof(SignedMessage));
	temp_signed->order = order;
	temp_signed->type = TYPE_SIGN;
	temp_signed->total_sigs = 1;
	
	//To do: Add a signature in the signature array

}

SignedMessage* sign_message(SignedMessage* msg)
{
	SignedMessage* temp_signed = (SignedMessage *)malloc(sizeof(SignedMessage));
	temp_signed->order = msg->order;
	temp_signed->type = TYPE_SIGN;
	temp_signed->total_sigs = msg->total_sigs+1;

	//To do copy all the signature from msg into temp_signed

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

		  //commander
		  int cflag=0;

		  //order
		  int order;

		  for(i=0;i<argc;i++)
		  {
					 string arg = argv[i];
					 if(arg == "-p")
					 {
								string temp = argv[i+1];
								port = atoi(temp.c_str());
								i = i + 1;
					 }

					 if(arg == "-h")
					 {
								hostfile = argv[i+1];
								i = i + 1;
					 }

					 if(arg == "-f")
					 {
								string temp = argv[i+1];
								faulty = atoi(temp.c_str());
								i = i + 1;
								if(faulty < 0)
								{
									printf("Number of faulty process should be nonzero \n");
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
								order = atoi(argv[i+1]);
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
			}

		  char myhostname[HOSTNAME_LEN];
		  myhostname[HOSTNAME_LEN-1]='\0';
		  gethostname(myhostname,HOSTNAME_LEN-1);
			struct hostent *hplocal;
			hplocal = gethostbyname(myhostname);

			if(!hplocal)
			{
				printf("%s %s\n",myhostname,"local not found");
				exit(EXIT_FAILURE);
			}

			strcpy(myhostname,inet_ntoa(*(struct in_addr *)hplocal->h_addr_list[0]));
			printf("My name is: %s \n",myhostname);

		  int maxhosts = MAX_HOSTS;
			char *hostnames[maxhosts];
			int flag_hostnames[maxhosts];



			int len = 0;

			char hostname[HOSTNAME_LEN];

			FILE *host_file;
			host_file = fopen(hostfile.c_str(),"rt");

			while(fgets(hostname,HOSTNAME_LEN,host_file) != NULL)
			{
							hostnames[len]=(char *)malloc(sizeof(char)*80);
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

							//Get IP address
							struct hostent *hp;
							hp = gethostbyname(hostname);

							if(!hp)
							{
											printf("%s %s\n",hostname,"not found");
											exit(EXIT_FAILURE);
							}


							strcpy(hostnames[len],inet_ntoa(*(struct in_addr *)hp->h_addr_list[0]));	
							printf("%s\n",hostnames[len]);
							len++;
			}

			printf("Len %d\n",len);
			global_proc_counter = len;
	
			if(len < faulty+2)
			{
				printf("Total number of processes must be greater than faulty+2\n");
				exit(EXIT_FAILURE);
			}



			fclose(host_file);	

			int sockfd,n;
			struct sockaddr_in servaddr,cliaddr;
			socklen_t lensock;
			char mesg[1000];

			sockfd=socket(AF_INET,SOCK_DGRAM,0);

			bzero(&servaddr,sizeof(servaddr));
			servaddr.sin_family = AF_INET;
			servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
			servaddr.sin_port=htons(port);

			bind(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));


			if(strcmp(myhostname,hostnames[0]) == 0)
			{
							//Commander
							//

							//Round Timer
							//
							printf("Starting Round Timer Commander\n");


							struct timeval time_start,time_curr;
							get_now(&time_start);

							

							while(time_to_seconds(&time_start,get_now(&time_curr)) <= ROUND_TIME)
							{
											printf(" Round Timer: %lf\n",time_to_seconds(&time_start,get_now(&time_curr)));
											int iter;
											for (iter=1;iter<len;iter++)
											{
															if(flag_hostnames[iter] == 0)
															{
																cliaddr.sin_family = AF_INET;
																cliaddr.sin_addr.s_addr = inet_addr(hostnames[iter]);
																cliaddr.sin_port = htons(port);
																SignedMessage* order_msg = (SignedMessage *)malloc(sizeof(SignedMessage));
																order_msg->type=1;
																order_msg->total_sigs=4;
																order_msg->order=1;
																sendto(sockfd,(char *)order_msg,sizeof(SignedMessage),0,(struct sockaddr *)&cliaddr,sizeof(cliaddr));
																printf("%s %d\n","======SENT=====",order_msg->order);			
															}
											}

											//ACK Timer
											struct timeval time_ack,time_curr_ack;
											get_now(&time_ack);

											while(time_to_seconds(&time_ack,get_now(&time_curr_ack)) <= UDP_RETRANSMIT_TIMER)
											{
												printf(" Ack Timer  %lf\n",time_to_seconds(&time_ack,get_now(&time_curr_ack)));
												socklen_t lensock = sizeof(cliaddr);
							
												if((n = recvfrom(sockfd,mesg,sizeof(Ack),MSG_DONTWAIT,(struct sockaddr *)&cliaddr,&lensock))< 0)
												{
													printf("Error code: %d\n",n);
												}

												printf("Recieved ACK from %s\n",inet_ntoa(cliaddr.sin_addr));

												int iter=1;
												for(iter=1;iter<len;iter++)
												{
//													printf("Ack from %s %d",inet_ntoa(cliaddr.sin_addr),strcmp(hostnames[iter],inet_ntoa(cliaddr.sin_addr)));
														if(strcmp(hostnames[iter],inet_ntoa(cliaddr.sin_addr)) == 0)
															flag_hostnames[iter] = 1;
												}
												printf("Exiting ACK LOOP\n");
											}
											printf("Exiting Round Loop\n");
							}
			}
			else
			{		
							//Global value set lieutenant
							vector<int> liet_val;

							//Map to store the messages and list of processes to which each message is to be sent
							map< SignedMessage,vector<int> > msg_to_list_processes;						


							//Lieutenant
							//
							//
							//
							//RoundTimer
							//
							//
							printf("Starting Round Timer Lietenant\n");

							struct timeval time_start,time_curr;
							get_now(&time_start);

							int curr_round = 0;							

							if(curr_round == 0)
							{
								while(time_to_seconds(&time_start,get_now(&time_curr)) <= ROUND_TIME)
								{							
											socklen_t lensock = sizeof(cliaddr);
											n = recvfrom(sockfd,mesg,1000,MSG_DONTWAIT,(struct sockaddr *)&cliaddr,&lensock);		
											printf("-------------------------------------------------------\n");
											if(n<=sizeof(Ack))
											{
															Ack* msg= (Ack *)mesg;
															printf("Received Ack:\n");
															printf("%d %s Bytes recv %d",msg->type,inet_ntoa(cliaddr.sin_addr),n);
															printf("-------------------------------------------------------\n");						 
											}
											else
											{
															SignedMessage* msg = (SignedMessage *)mesg;
															printf("Received Signed Message:\n");
															printf("%d %s Bytes recv %d %d %d",msg->type,inet_ntoa(cliaddr.sin_addr),n,msg->total_sigs,msg->order);
															printf("-------------------------------------------------------\n");	

															//Populate values of lietenant
													if(liet_val.empty() && verify(msg,msg_to_list_processes,myhostname))
																liet_val.push_back(msg->order);

															//Send Ack
															Ack* ack_msg = (Ack*)malloc(sizeof(Ack));
															ack_msg->type = 2;
															ack_msg->round = 1;
															sendto(sockfd,(char *)ack_msg,sizeof(Ack),0,(struct sockaddr *)&cliaddr,sizeof(cliaddr));
															printf("%s\n","Sent ACK");
											}
							

								printf("Exiting Round Loop Lietenant - Round: %d\n",curr_round);
								curr_round++;
								}
							}
							else if(curr_round==1)
							{
											while(time_to_seconds(&time_start,get_now(&time_curr)) <= ROUND_TIME)
											{							

															int iter=1;	
															//Send message to all processes except commander and itself
															for (iter=1;iter<len;iter++)
															{
																			if(strcmp(myhostname,hostnames[iter]) != 0 && flag_hostnames[iter]==0 && !liet_val.empty())
																			{
																							cliaddr.sin_family = AF_INET;
																							cliaddr.sin_addr.s_addr = inet_addr(hostnames[iter]);
																							cliaddr.sin_port = htons(port);
																							SignedMessage* order_msg = (SignedMessage *)malloc(sizeof(SignedMessage));
																							order_msg->type=1;
																							order_msg->total_sigs=4;

																							//Send the order from commander
																							if( !liet_val.empty())		
																											order_msg->order=liet_val.at(0);

																							sendto(sockfd,(char *)order_msg,sizeof(SignedMessage),0,(struct sockaddr *)&cliaddr,sizeof(cliaddr));
																							printf("%s %d\n","======SENT=====",order_msg->order);			
																			}
															}
											
												//ACK Timer
												struct timeval time_ack,time_curr_ack;
												get_now(&time_ack);
	
												while(time_to_seconds(&time_ack,get_now(&time_curr_ack)) <= UDP_RETRANSMIT_TIMER)
												{
														printf(" Ack Timer  %lf\n",time_to_seconds(&time_ack,get_now(&time_curr_ack)));
														socklen_t lensock = sizeof(cliaddr);
							
														if((n = recvfrom(sockfd,mesg,1000,MSG_DONTWAIT,(struct sockaddr *)&cliaddr,&lensock))< 0)
														{
															printf("Error code: %d\n",n);
														}

														printf("Recieved ACK from %s\n",inet_ntoa(cliaddr.sin_addr));

														int iter=1;
														for(iter=1;iter<len;iter++)
														{
//															printf("Ack from %s %d",inet_ntoa(cliaddr.sin_addr),strcmp(hostnames[iter],inet_ntoa(cliaddr.sin_addr)));
																if(strcmp(hostnames[iter],inet_ntoa(cliaddr.sin_addr)) == 0)
																		flag_hostnames[iter] = 1;
														}
														printf("Exiting ACK LOOP\n");
												}

												printf("Exiting Round Loop Lietenant - Round: %d\n",curr_round);
												curr_round++;
					
											}
							}
							else
							{
																

							}


							}
}
