#include "Server.h"

int main(int argc, char**argv)
{

			//Iterator
		  int i;

		  //String server port
			string server_port_str;

		  //Host file
		  string hostfile;

		  //count of messages to be sent
			int count;

			//Map to maintain retransmission info
			map<string,bool> host_retransmit_info;

			//buffer to hold a recieved message
			char *mesg = (char *)malloc(MAX_SIZE);

			//timer
			time_t timer;

			if(argc < 7)
			{
				kprintf("Usage: ");
				kprintf("server -h hostfile -p paxos_port -s server_port");
				exit(EXIT_FAILURE);
			}

		  for(i=0;i<argc;i++)
		  {
					 string arg = argv[i];
					 if(arg == "-p")
					 {					 			
					 			if(argv[i+1])
								{
									string temp = argv[i+1];
									paxos_port = atoi(temp.c_str());
									if(paxos_port == 0 || (paxos_port <=1024 || paxos_port >= 65535))
									{
										kprintf("Invalid port. Please specify a paxos port greater than 1024");
										exit(EXIT_FAILURE);
									}
									i = i + 1;
								}
								else
								{
									kprintf("Please specify paxos port number");
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
									kprintf("Please specify host file");
									exit(EXIT_FAILURE);
								}
					 }

					 if(arg == "-s")
					 {
					 			if(argv[i+1])
								{
									string temp = argv[i+1];
									server_port_str = temp;
									server_port = atoi(temp.c_str());
									if(server_port == 0 || (server_port <=1024 || server_port >= 65535))
									{
										kprintf("Invalid port. Please specify a server port greater than 1024");
										exit(EXIT_FAILURE);
									}
									i = i + 1;
								}
								else
								{
									kprintf("Please specify paxos port number");
									exit(EXIT_FAILURE);
								}								
					 }

		  }

			kprintf("Port: ",paxos_port);
			kprintf("Hostfile: ",hostfile);
			kprintf("Server Port: ",server_port);

			if(paxos_port == server_port)
			{
				kprintf("Server Port cannot be equal to paxos port");
				exit(EXIT_FAILURE);
			}
	
			ifstream hostfile_stream(hostfile.c_str());
		
			string hostname;

			int host_id = 0;

			if(hostfile_stream.is_open())
			{
				while(hostfile_stream.good())
				{		
						N++;
						getline(hostfile_stream,hostname);		
						if(!hostname.empty())
						{							
							hostname_to_id[hostname] = host_id++;
						
							//Get IP address
							struct hostent *hp;
							hp = gethostbyname(hostname.c_str());
		
							if(!hp)
							{
								kprintf(" not found ",hostname);
								exit(EXIT_FAILURE);			
							}
					
							if((inet_ntoa(*(struct in_addr *)hp->h_addr_list[0])))
							{
									  string s_local(inet_ntoa(*(struct in_addr *)hp->h_addr_list[0]));
	
										kprintf(s_local.c_str());
										
										if(s_local.find("127") != 0)
										{
											host_to_id[hostname] = s_local;
											host_retransmit_info[s_local] = false;
										}
										else
										{
											host_to_id[hostname] = string(inet_ntoa(*(struct in_addr *)hp->h_addr_list[1]));
											host_retransmit_info[string(inet_ntoa(*(struct in_addr *)hp->h_addr_list[1]))] = false;
										}
							}
							else
							{
									host_to_id[hostname] = string(inet_ntoa(*(struct in_addr *)hp->h_addr_list[1]));
									host_retransmit_info[string(inet_ntoa(*(struct in_addr *)hp->h_addr_list[1]))] = false;
							}
							kprintf(hostname.c_str());
	
						}
				}			
				hostfile_stream.close();
			}
			else
			{
				kprintf("Unable to read host file");
				exit(EXIT_FAILURE);
			}

			kprintf("Size is: ",hostname_to_id.size());
			N = N-1;

			kprintf("N is: ",N);

			//get my hostname
			char myhostname[HOSTNAME_LEN];
			gethostname(myhostname,HOSTNAME_LEN);

			string myhostname_str(myhostname);

			host_retransmit_info[host_to_id[myhostname_str]] = true;

			if(hostname_to_id.find(myhostname_str) != hostname_to_id.end())
				My_server_id = hostname_to_id[myhostname_str];
			else
				kprintf("My server id not found");
			
			kprintf("My_server_id is",My_server_id);
			kprintf("My name is",myhostname_str);
			kprintf("My state is",my_state);


			//Iterator for sending messages		
			int iter=0;

			//Socket variables for client
			int n;
			
			struct sockaddr_storage remoteaddr;
			socklen_t addrlen;
			int rv;

			struct addrinfo hints, *servinfo, *p;

			memset(&hints, 0, sizeof hints);
	    hints.ai_family = AF_UNSPEC;
	    hints.ai_socktype = SOCK_STREAM;
	    hints.ai_flags = AI_PASSIVE; // use my IP

			if ((rv = getaddrinfo(NULL,server_port_str.c_str(),&hints, &servinfo)) != 0) 
			{
			        kprintf("getaddrinfo: ", gai_strerror(rv));
			        exit(EXIT_FAILURE);
	    }
									

			socklen_t lensock;
			int yes = 1;	
			// loop through all the results and bind to the first we can
			for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((client_sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(client_sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(client_sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(client_sockfd);
            perror("server: bind");
            continue;
        }

        break;
    	}

    	if (p == NULL)  {
      	  kprintf("server: failed to bind");
        	exit(EXIT_FAILURE);
    		}

			int lval = listen(client_sockfd,BACKLOG);

			if(lval == -1)
			{
				kprintf("Listen failed");
				exit(EXIT_FAILURE);
			}

			kprintf("Waiting for connections on client_sockfd",client_sockfd);

			paxos_sockfd = socket(AF_INET,SOCK_DGRAM,0);

			if(paxos_sockfd == -1)
			{
				kprintf("Could not create socket");
				exit(EXIT_FAILURE);
			}

			bzero(&servaddr,sizeof(servaddr));
			servaddr.sin_family = AF_INET;
			servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
			servaddr.sin_port=htons(paxos_port);
			
			int	rval = bind(paxos_sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));
			
			if(rval==-1)
			{
				kprintf("Bind failed");
				exit(EXIT_FAILURE);
			}

			fd_set master;
			fd_set read_fds;
			int newfd;
			int fdmax;

			FD_ZERO(&master);
			FD_ZERO(&read_fds);
			FD_SET(client_sockfd,&master);
			FD_SET(paxos_sockfd,&master);

			if(client_sockfd>paxos_sockfd)
				fdmax = client_sockfd;
			else
				fdmax = paxos_sockfd;
			
			struct timeval tv;
			struct timeval end;
			struct timeval diff;
			get_now(&curr);


			for(;;)
			{
						get_now(&end);
						
						
						long int diffMS = ((end.tv_sec - curr.tv_sec)*pow(10,3) + ((end.tv_usec - curr.tv_usec)/pow(10,6))*pow(10,3));
								
						if(diffMS > Progress_Timer)
						{
							tv.tv_sec = 0;
							tv.tv_usec = 0;
						}
						else
						{
							tv.tv_sec = diffMS/pow(10,3) ;
							tv.tv_usec = (diffMS - tv.tv_sec*pow(10,3))*pow(10,3);  
						}
						

						kprintf("sec",tv.tv_sec);
						kprintf("usec",tv.tv_usec);

						progressTimer = 1;
						read_fds = master;
						int ret = select(fdmax+1,&read_fds,NULL,NULL,&tv);
						if(ret == -1)
						{
							kprintf("Error in select");
						}
						else if(ret)
						{
							for(iter=0;iter<=fdmax;iter++)
							{
									if(FD_ISSET(iter,&read_fds))
									{
										//tcp
										if(iter == client_sockfd)
										{
												addrlen = sizeof(remoteaddr);
												newfd = accept(client_sockfd,(struct sockaddr *)&remoteaddr,&addrlen);
												if(newfd == -1)
												{
													kprintf("Error in accept");
												}
												else
												{
													FD_SET(newfd, &master);
													if(newfd > fdmax)
														fdmax = newfd;
													if((n = recvfrom(newfd,mesg,MAX_SIZE,MSG_DONTWAIT,(struct sockaddr *)&cliaddr,&lensock)) > 0)
													{
														uint32_t* recv_type = (uint32_t*)malloc(sizeof(uint32_t));
					                  memcpy(recv_type,mesg,sizeof(uint32_t));
					                  int type = *recv_type;
					                  type = ntohl(type);
														
														if(type == TYPE_CLIENT_UPDATE)
														{
															kprintf("Client Update",type);
															Client_Update* c_up = (Client_Update*)malloc(sizeof(Client_Update));
															memcpy(c_up,mesg,sizeof(Client_Update));
															network_host(c_up);			
															time(&timer);
															update_time_for_timer[c_up->client_id] = timer;
															client_id_to_ip[c_up->client_id] = string(inet_ntoa(cliaddr.sin_addr)) ;
															client_id_to_update[c_up->update] = c_up;
															Client_Update_Handler(c_up);
														}
									
														

													}
												}
										}
										//udp
										else if(iter == paxos_sockfd)
										{
												int n;
												if((n = recvfrom(paxos_sockfd,mesg,MAX_SIZE,MSG_DONTWAIT,(struct sockaddr *)&cliaddr,&lensock)) > 0)									
												{
														uint32_t* recv_type = (uint32_t*)malloc(sizeof(uint32_t));
					                  memcpy(recv_type,mesg,sizeof(uint32_t));
					                  int type = *recv_type;
					                  type = ntohl(type);

														kprintf("Received Type",type);
													
														//I'm the leader and I get a client update
														if(type == TYPE_CLIENT_UPDATE && My_server_id == Last_Installed % N)
														{
															kprintf("In type",TYPE_CLIENT_UPDATE);
															Client_Update* c_up = (Client_Update*)malloc(sizeof(Client_Update));
															memcpy(c_up,mesg,sizeof(Client_Update));
															network_host(c_up);			
															Client_Update_Handler(c_up);
														}
									

														if(type == TYPE_VIEW_CHANGE)
														{
															kprintf("In type",TYPE_VIEW_CHANGE);		
															View_Change* vc = (View_Change*)malloc(sizeof(View_Change));
															memcpy(vc,mesg,sizeof(View_Change));
															network_host(vc);

															if(Conflict((char *)vc,TYPE_VIEW_CHANGE) == false)
															{
																//Network byte order
																if(vc->attempted > Last_Attempted && progressTimer == 0)
																{
																	Shift_to_Leader_Election(vc->attempted);
																	update_data_structures(vc,TYPE_VIEW_CHANGE);
																}
																if(vc->attempted == Last_Attempted)
																{
																	update_data_structures(vc,TYPE_VIEW_CHANGE);
																	if(Preinstall_Ready(vc->attempted))
																	{
																		Progress_Timer = Progress_Timer * 2;
																		progressTimer = 1;
																		get_now(&curr);

																		//Leader of last attempted
																		int leader_last_attempted = Last_Attempted % N;
																		if(leader_last_attempted == My_server_id)
																			Shift_to_Prepare_Phase();
																	}
																}
															}
														}
														if(type == TYPE_VC_PROOF)
														{
															kprintf("In type",TYPE_VC_PROOF);	

															VC_Proof* vcproof = (VC_Proof*)malloc(sizeof(VC_Proof));
															memcpy(vcproof,mesg,sizeof(VC_Proof));
															network_host(vcproof);
															//Network byte order
															if(Conflict((char *)vcproof,TYPE_VC_PROOF) == false)
															{
																if(vcproof->installed > Last_Installed)
																{
																	Last_Attempted = vcproof->installed;
																	//Leader last attempted
																	int leader_last_attempted = Last_Attempted % N;
																	if(leader_last_attempted == My_server_id)
																		Shift_to_Prepare_Phase();
																	else
																		Shift_to_Reg_Non_Leader();
																}
															}
														}
														
														if(type ==	TYPE_PROPOSAL)
														{
															kprintf("In type",TYPE_PROPOSAL);

															Proposal* c_p = (Proposal*)malloc(sizeof(Proposal));
															memcpy(c_p,mesg,sizeof(Proposal));
															network_host(c_p);

															if(Conflict((char *)c_p,TYPE_PROPOSAL) == false)
															{
																update_data_structures(c_p,TYPE_PROPOSAL);
																Accept* c_a = (Accept*)malloc(sizeof(Accept));
																c_a->type = TYPE_ACCEPT;
																c_a->server_id = My_server_id;
																c_a->view = c_p->view;
																c_a->seq = c_p->seq;

																host_network(c_a);

																//Send to all servers accept
																for(map<string,string>::iterator it=host_to_id.begin(); it != host_to_id.end(); it++)
																{
																	cliaddr.sin_family = AF_INET;
																	cliaddr.sin_addr.s_addr = inet_addr(it->second.c_str());
																	cliaddr.sin_port = htons(paxos_port);
																	int rval = sendto(paxos_sockfd,c_a,sizeof(Accept),0,(struct sockaddr *)&cliaddr,sizeof(cliaddr));								
																	kprintf("Sent message ",it->second.c_str());
																}
																network_host(c_a);
															}
														}

														if(type == TYPE_ACCEPT)
														{
															kprintf("In type",TYPE_ACCEPT);

															Accept* c_a = (Accept*)malloc(sizeof(Accept));
															memcpy(c_a,mesg,sizeof(Accept));
															network_host(c_a);

															if(Conflict((char *)c_a,TYPE_ACCEPT) == false)
															{
																update_data_structures(c_a,TYPE_ACCEPT);
																if(Globally_Ordered_Ready(c_a->seq))
																{
																	Client_Update update;
																	for(map<int,Global_Slot*>::iterator it = global_history.begin(); it!=global_history.end(); it++)
																	{
																			if(it->first == c_a->seq)
																			{
																				if(it->second->latest_proposal_accepted)
																				{
																					update = it->second->latest_proposal_accepted->update;
																					kprintf("Update Found in global history",update.update);
																				}
																				else
																				{
																					kprintf("Update not found");
																				}
																				break;
																			}

																	}

																	//Check update
																	Globally_Ordered_Update* gp = Construct_Globally_Ordered_Update(c_a->seq,c_a->server_id,update);
																	update_data_structures(gp,TYPE_GLOBALLY_ORDERED_UPDATE);
																	Advance_Aru();
																}
															}
														}

														if(type == TYPE_PREPARE)
														{
															kprintf("In type",TYPE_PREPARE);

															Prepare* c_pr = (Prepare*)malloc(sizeof(Prepare));
															memcpy(c_pr,mesg,sizeof(Prepare));
															network_host(c_pr);

															if(Conflict((char *)c_pr,TYPE_PREPARE) == false)
															{
																if(my_state == LEADER_ELECTION)
																{
																	update_data_structures(c_pr,TYPE_PREPARE);
																	vector<Proposal*> proposal_list;
																	vector<Globally_Ordered_Update*> global_list;
																	Construct_DataList(Local_Aru,&proposal_list,&global_list);

																	Prepare_OK* c_pok = Construct_Prepare_OK(c_pr->view,&proposal_list,&global_list);
																	Prepare_oks[My_server_id]	= c_pok;
																	Shift_to_Reg_Non_Leader();

																	//Send to Leader
																	int leader = Last_Installed % N;
																	int size = sizeof(Prepare_OK) + proposal_list.size()*sizeof(Proposal) + global_list.size()*sizeof(Globally_Ordered_Update);
																	string leader_ip;
																	for(map<string,int>::iterator it = hostname_to_id.begin(); it != hostname_to_id.end(); it++)
																	{
																		if(it->second == leader)
																		{
																			string leader_host_name = it->first;
																			leader_ip = host_to_id[leader_host_name];
																			kprintf("In prepare Leader IP",leader_ip);
																		}
																	}
																	cliaddr.sin_family = AF_INET;
																	cliaddr.sin_addr.s_addr = inet_addr(leader_ip.c_str());
																	cliaddr.sin_port = htons(paxos_port);
																	host_network(c_pok);
																		
																	char *to_be_sent = (char *)malloc(sizeof(size));
																	memcpy(to_be_sent,c_pok,sizeof(uint32_t)*5);
																	memcpy(to_be_sent+sizeof(uint32_t)*5,c_pok->proposals,sizeof(Proposal)*proposal_list.size());
																	memcpy(to_be_sent+sizeof(uint32_t)*5+sizeof(Proposal)*proposal_list.size(),c_pok->globally_ordered_updates,sizeof(Globally_Ordered_Update)*global_list.size());

																	int rval = sendto(paxos_sockfd,to_be_sent,size,0,(struct sockaddr *)&cliaddr,sizeof(cliaddr));								
																	kprintf("Sent message ",leader_ip.c_str());																
																	network_host(c_pok);
																	free(to_be_sent);
																	to_be_sent = NULL;
																
																}
																else
																{
																	//Send to Leader
																	int leader = Last_Installed % N;
																	Prepare_OK* c_pok = Prepare_oks[My_server_id];
																	int size = sizeof(Prepare_OK) + c_pok->total_globally_ordered_updates*sizeof(Globally_Ordered_Update) + c_pok->total_proposals*sizeof(Proposal);
																	
																	int proposal_list_size = c_pok->total_proposals;
																	int global_list_size = c_pok->total_globally_ordered_updates;

																	string leader_ip;
																	for(map<string,int>::iterator it = hostname_to_id.begin(); it != hostname_to_id.end(); it++)
																	{
																		if(it->second == leader)
																		{
																			string leader_host_name = it->first;
																			leader_ip = host_to_id[leader_host_name];
																			kprintf("In prepare Leader IP",leader_ip);
																		}
																	}
																	cliaddr.sin_family = AF_INET;
																	cliaddr.sin_addr.s_addr = inet_addr(leader_ip.c_str());
																	cliaddr.sin_port = htons(paxos_port);
																	host_network(c_pok);

																	char *to_be_sent = (char *)malloc(sizeof(size));
																	memcpy(to_be_sent,c_pok,sizeof(uint32_t)*5);
																	memcpy(to_be_sent+sizeof(uint32_t)*5,c_pok->proposals,sizeof(Proposal)*proposal_list_size);
																	memcpy(to_be_sent+sizeof(uint32_t)*5+sizeof(Proposal)*proposal_list_size,c_pok->globally_ordered_updates,sizeof(Globally_Ordered_Update)*global_list_size);


																	int rval = sendto(paxos_sockfd,to_be_sent,size,0,(struct sockaddr *)&cliaddr,sizeof(cliaddr));								
																	kprintf("Sent message ",leader_ip.c_str());																
																	network_host(c_pok);
																	free(to_be_sent);
																	to_be_sent = NULL;			

																}
															}
														}

														if(type == TYPE_PREPARE_OK)
														{
															kprintf("In type", TYPE_PREPARE_OK);
															Prepare_OK* c_pok = (Prepare_OK*)malloc(sizeof(Prepare_OK));						
															memcpy(c_pok,mesg,sizeof(uint32_t)*5);

															uint32_t total_proposals = ntohl(c_pok->total_proposals);
															uint32_t total_globally_ordered_updates = ntohl(c_pok->total_globally_ordered_updates);
					
															Proposal* proposals = (Proposal*)malloc(sizeof(Proposal)*total_proposals);
															memcpy(proposals,mesg+sizeof(uint32_t)*5,total_proposals*sizeof(Proposal));

															Globally_Ordered_Update* globally_ordered_updates = (Globally_Ordered_Update*)malloc(sizeof(Globally_Ordered_Update)*total_globally_ordered_updates);
															memcpy(globally_ordered_updates,mesg+sizeof(uint32_t)*5+total_proposals*sizeof(Proposal),total_globally_ordered_updates*sizeof(Globally_Ordered_Update));

															c_pok->proposals = proposals;
															c_pok->globally_ordered_updates = globally_ordered_updates;

															network_host(c_pok);

															if(Conflict((char *)c_pok,TYPE_PREPARE_OK)	== false)
															{
																update_data_structures(c_pok,TYPE_PREPARE_OK);
																if(View_Prepared_Ready(c_pok->view))
																	Shift_to_Reg_Leader();
															}
														}

														//Expiration of Update Timer	
														for(map<int,time_t>::iterator it = update_time_for_timer.begin(); it!=update_time_for_timer.end(); it++)
														{
																	int client_id = it->first;
																	time_t start = update_time_for_timer[client_id];
																	time_t current_time;
																	time(&current_time);
																	if(difftime(current_time,start)*pow(10,3) > Update_Timer)
																	{
																		update_time_for_timer[client_id] = current_time;	
																		if(my_state == REG_NONLEADER)
																		{
																			//Send to Leader
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
																			Client_Update* to_be_sent = Pending_Updates[client_id];
																			cliaddr.sin_family = AF_INET;
																			cliaddr.sin_addr.s_addr = inet_addr(leader_ip.c_str());
																			cliaddr.sin_port = htons(paxos_port);
																			host_network(to_be_sent);
																			int rval = sendto(paxos_sockfd,to_be_sent,size,0,(struct sockaddr *)&cliaddr,sizeof(cliaddr));								
																			kprintf("Sent message ",leader_ip.c_str());																			
																			network_host(to_be_sent);
																		}
																	}
														}








												}
										}
									}
							}
						}
						//timeout
						else
						{
								progressTimer = 0;
								Leader_Election();
						}
						Send_VC_Proof();
			}

}


