#include "Client.h"

void host_network(Client_Update* c)
{	
	c->type = htonl(c->type);
	c->client_id = htonl(c->client_id);
	c->server_id = htonl(c->server_id);
	c->timestamp = htonl(c->timestamp);
	c->update = htonl(c->update);
}

void network_host(Client_Update* c)
{		
		c->type = ntohl(c->type);
		c->client_id = ntohl(c->client_id);
		c->server_id = ntohl(c->server_id);
		c->timestamp = ntohl(c->timestamp);
		c->update = ntohl(c->update);
}

int main(int argc, char**argv)
{

			fprintf(stderr,"%s","Started Client");

			//Iterator
		  int i;

			//host file
			string hostfile;

			//Server Port number
			int server_port;

			//string server port
			string server_port_str;

		  //Command file
		  string commandfile;

			//client id
			int client_id;

			//Map to store Hostname to IP address
			map<string,string> host_to_id;

			//Map to store Hostname to id
			map<string,int> hostname_to_id;


			//current update
			int current_update;
	
			if(argc < 9)
			{
				kprintf("Usage: ");
				kprintf("client -s server_port -f command_file -i client_id -h host_file");
				exit(EXIT_FAILURE);
			}

		  for(i=0;i<argc;i++)
		  {
					 string arg = argv[i];
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
									kprintf("Please specify server port number");
									exit(EXIT_FAILURE);
								}
					 }

					 if(arg == "-f")
					 {	
					 			if(argv[i+1])
								{
									commandfile = argv[i+1];
									i = i + 1;
								}
								else
								{
									kprintf("Please specify command file");
									exit(EXIT_FAILURE);
								}
					 }

					 if(arg == "-i")
					 {
					 			if(argv[i+1])
								{
									string temp = argv[i+1];
									client_id = atoi(temp.c_str());
								i = i + 1;
								}
								else
								{
									kprintf("Please specify client id");
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

		  }

			kprintf("Client ID: ",client_id);
			kprintf("Command file: ",commandfile);
			kprintf("Server Port: ",server_port);
			
			
			ifstream hostfile_stream(hostfile.c_str());

			string hostname,hostname_split;
			int host_id = 0;

			if(hostfile_stream.is_open())
			{
				while(hostfile_stream.good())
				{		
						getline(hostfile_stream,hostname);		
						if(!hostname.empty())
						{							
								hostname_to_id[hostname] = host_id;
								kprintf(hostname.c_str(),hostname_to_id[hostname]);
								host_id = host_id + 1;
						}
				}
			}

			//Socket variables for client
			int client_sockfd,n;
			struct sockaddr_in serv_addr;
	
			memset((char *) &serv_addr, 0, sizeof(serv_addr));
	    serv_addr.sin_family = AF_INET;
	    serv_addr.sin_port = htons(server_port);
			
			
			ifstream commandfile_stream(commandfile.c_str());
		
			if(commandfile_stream.is_open())
			{
				while(commandfile_stream.good())
				{		
						getline(commandfile_stream,hostname);		
						unsigned pos =  hostname.find(" ");
						
						if(pos != -1)
						{
							string update =  hostname.substr(pos+1);
							int update_length = update.length();
							int hostname_length = hostname.length();
							hostname_split = hostname.substr(0,hostname_length-update_length-1);
						
							kprintf("hostname",hostname_split.length());
							kprintf("update",update);

							if(!hostname_split.empty())
							{							
						
								//Get IP address
								struct hostent *hp;
								hp = gethostbyname(hostname_split.c_str());
		
								if(!hp)
								{
									kprintf(" not found ",hostname_split);
									exit(EXIT_FAILURE);			
								}
					
								if((inet_ntoa(*(struct in_addr *)hp->h_addr_list[0])))
								{
								  string s_local(inet_ntoa(*(struct in_addr *)hp->h_addr_list[0]));
									kprintf(s_local.c_str());
										
									if(s_local.find("127") != 0)
									{
										host_to_id[hostname_split] = s_local;
									}
									else
									{
										host_to_id[hostname_split] = string(inet_ntoa(*(struct in_addr *)hp->h_addr_list[1]));
									}
								}
								else
								{
									host_to_id[hostname_split] = string(inet_ntoa(*(struct in_addr *)hp->h_addr_list[1]));
								}
								kprintf(hostname_split.c_str());
	
							
							}

							kprintf("Sending update to: ",host_to_id[hostname_split]);

							//send update
							client_sockfd = socket(AF_INET,SOCK_STREAM,0);
			
							if(client_sockfd == -1)
							{
									kprintf("Could not create socket");
								exit(EXIT_FAILURE);
							}


							 if(inet_aton(host_to_id[hostname_split].c_str(), &serv_addr.sin_addr) == 0){
											 kprintf("INET_ATON failed\n");
						           kprintf("Address storing in sockaddr_in failed for %s\n", host_to_id[hostname_split]);
										   kprintf("Skipping sending his update %d\n", update.c_str());										    
										   continue;																																																															        			 }	


							int rval = connect(client_sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr));

							if(rval==-1)
							{
								kprintf("Connect failed skipping update");
							}

							fprintf(stderr,"%d: Sending update %s to server %s timestamp %d\n",client_id,update.c_str(),hostname_split.c_str(),host_id);

							Client_Update* cli_update = (Client_Update*)malloc(sizeof(Client_Update));
							cli_update->type = TYPE_CLI_UPDATE;
							cli_update->client_id = client_id;
							cli_update->server_id = hostname_to_id[hostname_split];
							kprintf("Server id: ",cli_update->server_id);
							cli_update->timestamp = host_id++;
							cli_update->update = atoi(update.c_str());
							host_network(cli_update);	

							rval = send(client_sockfd,cli_update,sizeof(Client_Update),0);

							if(rval == -1)
							{
								kprintf("Send failed");
								fprintf(stderr,"%d: Unable to send update %s to server %s\n",client_id,update.c_str(),hostname_split.c_str());
							}							

							network_host(cli_update);

							char recv_buff[CLIENT_UPDATE_LEN];
		
							rval = recv(client_sockfd,recv_buff,CLIENT_UPDATE_LEN,0);
							
							kprintf("Error: ",errno);
							kprintf("Recieved",rval);

							if(rval == 0)
								fprintf(stderr,"%d: Connection for update %s is closed by server %s\n",client_id,update.c_str(),hostname_split.c_str());
							else if(rval == -1)
							{
								kprintf("Recv failed");
							}
							else
							{
								fprintf(stderr,"%d: Update %s sent to server %d is executed\n",client_id,update.c_str(),cli_update->server_id);								
							}			

					close(client_sockfd);
				}
			}

			commandfile_stream.close();
		}
		else
		{
			kprintf("Unable to read host file");
			exit(EXIT_FAILURE);
		}
			
}


