#include "Server.h"

void host_network(DataMessage* data_msg)
{	
	data_msg->type = htonl(data_msg->type);
	data_msg->sender = htonl(data_msg->sender);
	data_msg->msg_id = htonl(data_msg->msg_id);
	data_msg->data = htonl(data_msg->data);
}

void network_host(DataMessage* data_msg)
{
	data_msg->type = ntohl(data_msg->type);
	data_msg->sender = ntohl(data_msg->sender);
	data_msg->msg_id = ntohl(data_msg->msg_id);
	data_msg->data = ntohl(data_msg->data);
}

void host_network(AckMessage* ack_msg)
{
	ack_msg->type = htonl(ack_msg->type);
	ack_msg->sender = htonl(ack_msg->sender);
	ack_msg->msg_id = htonl(ack_msg->msg_id);
	ack_msg->proposed_seq = htonl(ack_msg->proposed_seq);
	ack_msg->receiver = htonl(ack_msg->receiver);
}

void network_host(AckMessage* ack_msg)
{
	ack_msg->type = ntohl(ack_msg->type);
	ack_msg->sender = ntohl(ack_msg->sender);
	ack_msg->msg_id = ntohl(ack_msg->msg_id);
	ack_msg->proposed_seq = ntohl(ack_msg->proposed_seq);
	ack_msg->receiver = ntohl(ack_msg->receiver);
}

void host_network(SeqMessage* seq_msg)
{
	seq_msg->type = htonl(seq_msg->type);
	seq_msg->sender = htonl(seq_msg->sender);
	seq_msg->msg_id = htonl(seq_msg->msg_id);
	seq_msg->final_seq = htonl(seq_msg->final_seq);
}

void network_host(SeqMessage* seq_msg)
{
	seq_msg->type = ntohl(seq_msg->type);
	seq_msg->sender = ntohl(seq_msg->sender);
	seq_msg->msg_id = ntohl(seq_msg->msg_id);
	seq_msg->final_seq = ntohl(seq_msg->final_seq);
}

map<string,bool> reinit(map<string,bool> retransmitinfo,string myhostip)
{
	for(map<string,bool>:: iterator it=retransmitinfo.begin(); it!= retransmitinfo.end();it++)
	{
		if(it->first.compare(myhostip) != 0)
			it->second = false;
	}

	return retransmitinfo;
}

multiset<int> reinit_set(multiset<int> ackset)
{
	ackset.clear();
	return ackset;
}

int check(map<string,bool> retransmitinfo)
{
	//O for ready to go to next msg, -1 for retransmit message
	int ret = 1;
	for(map<string,bool>:: iterator it=retransmitinfo.begin(); it!= retransmitinfo.end();it++)
	{
		if(it->second ==false)
			ret = 0;
	}
	return ret;
}

void deliver_messages(map<Key,DM*> *delivery_queue,vector<Key> *delivered_queue,int mypid)
{
	for(map<Key,DM*>:: iterator it= (*delivery_queue).begin(); it!= (*delivery_queue).end();it++)
	{
		if(it->second->deliverable == true)
		{
			Key sentid ((it->second)->data_msg->msg_id,(it->second)->data_msg->sender);
			(*delivered_queue).push_back(sentid);
			
			//Print delivered message output
			printf("%d : Processed message %d from sender %d with seq %d",mypid,(it->second)->data_msg->msg_id,(it->second)->data_msg->sender,(it->second)->seq_num);

			//erase from map
			(*delivery_queue).erase(it);
		}
	}
}


int main(int argc, char**argv)
{

			//Iterator
		  int i;

		  //Port number
		  int port;

		  //Host file
		  string hostfile;

		  //count of messages to be sent
			int count;

			//Delivery queue
			map<Key,DM*> delivery_queue;

			//Delivered Messages
			vector<Key> delivered_queue;

			//Map to store Hostname to IP address
			map<string,string> host_to_id;

			//Map to store Hostname to id
			map<string,int> hostname_to_id;

			//Map to maintain retransmission info
			map<string,bool> host_retransmit_info;

			//buffer to hold a recieved message
			char *mesg = (char *)malloc(sizeof(AckMessage));

			if(argc < 7)
			{
				kprintf("Usage: ");
				kprintf("proj2 -p port -h hostfile -c count");
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
									port = atoi(temp.c_str());
									if(port == 0 || port <=1024)
									{
										kprintf("Invalid port. Please specify a port greater than 1024");
										exit(EXIT_FAILURE);
									}
									i = i + 1;
								}
								else
								{
									kprintf("Please specify port number");
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

					 if(arg == "-c")
					 {
					 			if(argv[i+1])
								{
									string temp = argv[i+1];
									count = atoi(temp.c_str());
									i = i + 1;
									if(count < 0)
									{
										kprintf("Number of messages to be multicast must be greater than zero");
										exit(EXIT_FAILURE);
									}
								}
								else
								{
									kprintf("Please specify number of messages to be multicast");
									exit(EXIT_FAILURE);
								}
					 }

		  }

			kprintf("Port: ",port);
			kprintf("Hostfile: ",hostfile);
			kprintf("Count: ",count);
	
			ifstream hostfile_stream(hostfile.c_str());
		
			string hostname;

			int host_id = 0;

			if(hostfile_stream.is_open())
			{
				while(hostfile_stream.good())
				{		
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

			kprintf("Size is: ",host_to_id.at("xinu01.cs.purdue.edu"));

			//get my hostname
			char myhostname[HOSTNAME_LEN];
			gethostname(myhostname,HOSTNAME_LEN);

			string myhostname_str(myhostname);

			host_retransmit_info[host_to_id[myhostname_str]] = true;

			//Iterator for sending messages		
			int iter=0;

			//Seq numbers to be maintained throughout
			int max_last_proposed_seq_num=0;
			int last_proposed_seq_num = 0;

			//Socket variables
			int sockfd,n;
			struct sockaddr_in servaddr,cliaddr;
			socklen_t lensock;
		
			sockfd = socket(AF_INET,SOCK_DGRAM,0);

			if(sockfd == -1)
			{
				kprintf("Could not create socket");
				exit(EXIT_FAILURE);
			}

			bzero(&servaddr,sizeof(servaddr));
			servaddr.sin_family = AF_INET;
			servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
			servaddr.sin_port=htons(port);
			
			int bval = bind(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));
			
			if(bval==-1)
			{
				kprintf("Bind failed\n");
				exit(EXIT_FAILURE);
			}
			
			//Multiset to maintain all the sequence numbers recieved in Acks
			multiset<int> ackmultiset;


			//infinite loop for the protocol
			while(1)
			{
					//Send message to all other hosts/processes only if iter<count
					if(iter<count)
					{
						DataMessage *init_dm = (DataMessage *)malloc(sizeof(DataMessage));
						init_dm->type = TYPE_DM;
						init_dm->sender = hostname_to_id[myhostname_str];
					
						//Get sender ID
						kprintf("Sender ID: ",init_dm->sender);

						init_dm->msg_id = iter;
						init_dm->data = iter;
					
						host_network(init_dm);

						for(map<string,string>::iterator it	=	host_to_id.begin();it	!= host_to_id.end();++it)
						{
							if(host_retransmit_info[it->second.c_str()] == false)
							{
								cliaddr.sin_family = AF_INET;
								cliaddr.sin_addr.s_addr = inet_addr(it->second.c_str());
								cliaddr.sin_port = htons(port);
								int rval = sendto(sockfd,init_dm,sizeof(DataMessage),0,(struct sockaddr *)&cliaddr,sizeof(cliaddr));								
								kprintf("Sent message ",it->second.c_str());
							}
						}					
						
						network_host(init_dm);

						Key current_srch_key (last_proposed_seq_num,hostname_to_id[myhostname_str]);								
				
						//Add the sent message to your queue if it does not already exist
						if(delivery_queue.find(current_srch_key) == delivery_queue.end())
						{
							DM* sent_msg = (DM*)malloc(sizeof(DM));
							sent_msg->deliverable = false;
							sent_msg->seq_num = ++max_last_proposed_seq_num;
							sent_msg->data_msg = init_dm;
							Key current_key (sent_msg->seq_num,hostname_to_id[myhostname_str]);								
							last_proposed_seq_num = max_last_proposed_seq_num;	
							//Add the sent message to your queue
							delivery_queue[current_key] = sent_msg;
							kprintf("Inserted in delivery queue",current_key.seq);
						}
					}

					//Start Recieving messages here
					//start ack timer
					 struct timeval time_ack,time_curr_ack;                            
					 get_now(&time_ack);

					 while(time_to_seconds(&time_ack,get_now(&time_curr_ack)) <= UDP_RETRANSMIT_TIMER)
					 {
					 			
								socklen_t lensock = sizeof(cliaddr);                            
					      if((n = recvfrom(sockfd,mesg,sizeof(AckMessage),MSG_DONTWAIT,(struct sockaddr *)&cliaddr,&lensock)) > 0)
								{
									
									//Get the type by converting from network to host
									uint32_t* recv_type = (uint32_t*)malloc(sizeof(uint32_t));
									memcpy(recv_type,mesg,sizeof(uint32_t));
									int type = *recv_type;
									type = ntohl(type);
			
									kprintf("Recieved Type",type);

									if(type == TYPE_ACK)
									{
												AckMessage* ack_mesg = (AckMessage*)mesg;
												network_host(ack_mesg);
												
												int sender = hostname_to_id[myhostname_str];
												if(ack_mesg->receiver == sender && iter == ack_mesg->msg_id)
												{
													//Mark Ack recieved
													string ip = string(inet_ntoa(cliaddr.sin_addr));
													
													if(host_retransmit_info.find(ip) != host_retransmit_info.end())
													{
														host_retransmit_info[ip] = true;
														kprintf("Marked true for ip ",ip.c_str());
													}
												}
												ackmultiset.insert(ack_mesg->proposed_seq);
									}
									else if(type == TYPE_DM)
									{
										//Check if it is delivered
										DataMessage* dm_mesg = (DataMessage*) mesg;

										network_host(dm_mesg);

										int msg_id = dm_mesg->msg_id;
										int sender = dm_mesg->sender;
										bool delivered = false;

										for(vector<Key>:: iterator it=delivered_queue.begin();it!=delivered_queue.end();++it)
										{
											if((*it).seq == msg_id && (*it).pid == sender)
											{
												delivered = true;
											}
										}
										
										//Check if in ready queue
										bool inreadyqueue = false;

										for(map<Key,DM*>:: iterator it=delivery_queue.begin();it!=delivery_queue.end();it++)
										{
											if((it->second)->data_msg->msg_id == msg_id && (it->second)->data_msg->sender == sender)
												inreadyqueue = true;
										}

										//if not in ready queue or delivered
										if(delivered == false && inreadyqueue == false)
										{
												//Send Ack
												AckMessage* ack_mesg = (AckMessage*)malloc(sizeof(AckMessage));
												ack_mesg->type = TYPE_ACK;
												ack_mesg->sender = hostname_to_id[myhostname_str];
												ack_mesg->msg_id = msg_id;
												ack_mesg->proposed_seq = ++max_last_proposed_seq_num;
												ack_mesg->receiver = sender;
														
												host_network(ack_mesg);		

												//Send ack to the sender
												string ip = string(inet_ntoa(cliaddr.sin_addr));
												cliaddr.sin_family = AF_INET;
												cliaddr.sin_addr.s_addr = inet_addr(ip.c_str());
												cliaddr.sin_port = htons(port);
												int rval = sendto(sockfd,ack_mesg,sizeof(AckMessage),0,(struct sockaddr *)&cliaddr,sizeof(cliaddr));								
												kprintf("Sent Ack to ",ip.c_str());
			
												//Store in Queue ********************
												string current_hostname;

												for(map<string,string>:: iterator it=host_to_id.begin(); it!=host_to_id.end(); it++)
												{
													if((it->second).compare(ip) == 0)
														current_hostname = it->first;
												}
												
												DataMessage *init_dm = (DataMessage *)malloc(sizeof(DataMessage));
												init_dm->type = TYPE_DM;
												init_dm->sender = hostname_to_id[current_hostname];
												init_dm->msg_id = msg_id;
												init_dm->data = dm_mesg->data;											

											
												DM* sent_msg = (DM*)malloc(sizeof(DM));
												sent_msg->deliverable = false;
												sent_msg->seq_num = max_last_proposed_seq_num;
												sent_msg->data_msg = init_dm;
												Key current_key (sent_msg->seq_num,hostname_to_id[current_hostname]);								
												last_proposed_seq_num = max_last_proposed_seq_num;

												//Add the sent message to your queue
												delivery_queue[current_key] = sent_msg;
												kprintf("Inserted in delivery queue",current_key.seq);
										}
									}
									else if(type == TYPE_SEQ_MSG)
									{
												SeqMessage* seq_mesg = (SeqMessage*)mesg;	
												
												network_host(seq_mesg);

												int msg_id = seq_mesg->msg_id;
												int sender = seq_mesg->sender;

												//Search in delivery queue for the corresponding message
												for(map<Key,DM*>:: iterator it=delivery_queue.begin();it!=delivery_queue.end();it++)
												{
													if((it->second)->data_msg->msg_id == msg_id && (it->second)->data_msg->sender == sender)
														{
															(it->second)->deliverable = true;
															(it->second)->seq_num = seq_mesg->final_seq;
														}
												}
												max_last_proposed_seq_num = seq_mesg->final_seq;
												deliver_messages(&delivery_queue,&delivered_queue,hostname_to_id[myhostname_str]);
												kprintf("Got final seq message", seq_mesg->msg_id);

									}						
								}

					 }
												 

					//go for next message
					if(check(host_retransmit_info))
					{
													kprintf("After check");

													//Get the maximum sequence number recieved
													int max = *(ackmultiset.rbegin());

													kprintf("Max is",max);

													//Update the max proposed seq num for next message
													if(max_last_proposed_seq_num < max)
														max_last_proposed_seq_num = max;

													//Build the final seq message
													SeqMessage* final_seq_msg = (SeqMessage *)malloc(sizeof(SeqMessage));
													final_seq_msg->type = TYPE_SEQ_MSG;
													final_seq_msg->sender = hostname_to_id[myhostname_str];
													final_seq_msg->msg_id = iter;
													final_seq_msg->final_seq = max;

													host_network(final_seq_msg);

													//send it to everyone
													for(map<string,string>::iterator it	=	host_to_id.begin();it	!= host_to_id.end();++it)
													{
															if(myhostname_str.compare(it->first) !=0)
															{
																cliaddr.sin_family = AF_INET;
																cliaddr.sin_addr.s_addr = inet_addr(it->second.c_str());
																cliaddr.sin_port = htons(port);
																int rval = sendto(sockfd,final_seq_msg,sizeof(SeqMessage),0,(struct sockaddr *)&cliaddr,sizeof(cliaddr));								
																kprintf("Sent final sequence message ",it->second.c_str());
															}
													}
													
												int msg_id_srch = iter;
												int sender = hostname_to_id[myhostname_str];

												//update the message in delivery queue with the final sequence number
												for(map<Key,DM*>:: iterator it = delivery_queue.begin();it != delivery_queue.end();++it)
												{
														if(it->second->data_msg->sender == sender && it->second->data_msg->msg_id == msg_id_srch)
														{

															//Prepare new message
															DM* updatedDM = (DM*)malloc(sizeof(DM));	
															updatedDM->data_msg = it->second->data_msg;
															updatedDM->deliverable = true;
															updatedDM->seq_num = max;
														
															//erase the old msg
															delivery_queue.erase(it);
															
															//Prepare new key
															Key updated_key(max,sender);

															//Update queue
															delivery_queue[updated_key] = updatedDM;

															kprintf("Found the message and marked deliverable and updated sequence number too",updatedDM->seq_num);

														}
												}
												
					//deliver messages
					deliver_messages(&delivery_queue,&delivered_queue,hostname_to_id[myhostname_str]);
		
					iter++;	
					//Reinitialize for new message
					host_retransmit_info = reinit(host_retransmit_info,host_to_id[myhostname_str]);

					//Reinitialize ack multiset
					ackmultiset = reinit_set(ackmultiset);
				}
	

				
			}

}


