#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<signal.h>
#include<fcntl.h>
#include<ctype.h>
#define MAX_DATA	1024
int gzip_support(char *msg);
void response(int);
int urlDecode(const char *s, char *dec);
char* run_command(char *command);
void stop_port(int);
//https://www.gnu.org/software/libc/manual/html_node/Inet-Example.html#Inet-Example
int make_socket (uint16_t port)
{
  int sock,len,reuse=1;
  struct sockaddr_in server;

  if ((sock = socket (PF_INET, SOCK_STREAM, 0))==-1)
    {
      perror ("socket");
      exit (EXIT_FAILURE);
    }

  server.sin_family = AF_INET;
  server.sin_port = htons (port);
  server.sin_addr.s_addr = htonl (INADDR_ANY);
  bzero(&server.sin_zero, 8);
  len = sizeof(server);
  if (setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(int))==-1)
  {
	perror("setsock");
	exit(EXIT_FAILURE);
  }
  if (bind (sock, (struct sockaddr *) &server, len) == -1)
    {
      perror ("bind");
      exit (EXIT_FAILURE);
    }
  //printf("\nsocket built!!");
  return sock;
}

int main(int argc, char **argv)
{
	extern int make_socket (uint16_t port);
	int sock,cli,sent,data_len;
	fd_set active_fd_set, read_fd_set;
	int i,len;
	char data[MAX_DATA];
        struct sockaddr_in clientname;
	signal(SIGINT, stop_port);
	sock = make_socket (atoi(argv[1]));
	if (listen (sock, 1000) < 0)
	 {
	      perror ("listen");
 	      exit (EXIT_FAILURE);
	  }
	//printf("\nServer started at port no. %s",argv[1]);
	while(1)
	{
		len=sizeof(clientname);
		 if((cli = accept(sock,(struct sockaddr *)&clientname,&len))==-1)
		{
			perror ("listen");
	       	        exit (EXIT_FAILURE);
		}
		else
		{
			//printf("\nNew Client connected from port %d and from IP %s", ntohs(clientname.sin_port), inet_ntoa		(clientname.sin_addr));
			response(cli);
		}

	}
}
void stop_port(int i)
{
	fflush(stdout);
	exit(0);
	close(i);
}
void response(int client)
{
	char msg[MAX_DATA], *req[3],*output,*res,*res2,num[100];

	char *res1="HTTP/1.1 200 OK\n"
		   "Content-Type: text/html\n";
	int rcv;
	memset( (void*)msg, (int)'\0', MAX_DATA );
	rcv = recv(client,msg,MAX_DATA,0);
	if(rcv < 0)
	{
		fprintf(stderr,("recv() error < 0"));
	}
	else if(rcv==0)
	{
		fprintf(stderr,"Client disconnected upexpectedly.\n");
	}
	else
	{
		//printf("\nconnected %s", msg);
		int conf = gzip_support(msg);
		req[0] = strtok(msg," \t\n");
		if(strncmp(req[0],"GET\0",4)==0)
		{
			req[1]=strtok(NULL," \t");
			req[2]=strtok(NULL," \t\n");
			if(strncmp(req[2],"HTTP/1.1",8)!=0)
			{
				printf("http req error: %s",req[1]);
				send(client, "HTTP/1.1 400 Bad Request\n", 25,0);
			}
			else
			{
				char *t = strchr(req[1]+1,'/');
				if((t!=NULL) && (strcmp(t,"favicon.ico")!=0))
				{
					*t++='\0';
					output = malloc(strlen(t)+1);
					output = (urlDecode(t,output) < 0 ? "Not valid" : output);
					res2=run_command(output);
					if(res2 == NULL)
					{
						res2="No content in stdout/Invalid command/Invalid option on a command";
						res1="HTTP/1.1 404 Not Found\n"
						     "Content-Type: text/html\n";
					}
					else
					{
						res1="HTTP/1.1 200 OK\n"
		   				     "Content-Type: text/html\n";
					}

				}
				else
				{
					res2="Null Command/Favicon.ico request";
					res1="HTTP/1.1 404 Not Found\n"
					     "Content-Type: text/html\n";

				}

			}
		}
		else
		{
			res2="HTTP request error!!";
			res1="HTTP/1.1 404 Not Found\n"
			     "Content-Type: text/html\n";

		}

		/*if(conf==1)
		{
			//char src[1024];
			//printf("ff");
			//sprintf(src,"Content-Encoding: gzip\n","%s");
			strcat(res1,"Content-Encoding: gzip\n");
			printf("res1:%s",res1);
		}*/
		sprintf(num,"Content-Length: %lu\n\n",strlen(res2));
		res = (char *) malloc(1 + strlen(res1)+ strlen(res2) +strlen(num) );
		strcpy(res,res1);
		strcat(res,num);
		strcat(res,res2);
		send(client,res, strlen(res)+1, 0);
	}
	shutdown (client, SHUT_RDWR);
	close(client);
}
int gzip_support(char *msg)
{
	char *token, *string, *tofree;
	tofree = string = strdup(msg);
        while ((token = strsep(&string, "\n")) != NULL){
		if(strncmp(token,"Accept-Encoding",15)==0)
		{
			if(strstr(token,"gzip"))
			{
				free(tofree);
				return 1;
			}
		}
		//printf("\ntoken:%s",token);
	}
	free(tofree);
	return -1;
}
//https://stackoverflow.com/questions/26932616/read-input-from-popen-into-char-in-c
char* run_command(char *command)
{
	char *str=NULL,*temp=NULL;
	char buffer[100];
        unsigned int  cntr;
	unsigned int  size=1;
        FILE *pipe_fp;
	//process(argv[1]);
	//printf("%s",argv[1]);
        if (( pipe_fp = popen(command, "r")) == NULL)
        {
                perror("popen");
                exit(0);
        }
	while(fgets(buffer,sizeof(buffer),pipe_fp)!=NULL)
	{
		cntr=strlen(buffer);
		temp=realloc(str,size+cntr);
		if(temp==NULL)
		{
			printf("allocation error");
		}
		else
		{
			str=temp;
		}
		strcpy(str+size-1,buffer);
		size += cntr;
	}
        pclose(pipe_fp);
	return str;
}
//https://rosettacode.org/wiki/URL_decoding
int ishex(int z)
{
	return	(z >= '0' && z <= '9')	||
		(z >= 'a' && z <= 'f')	||
		(z >= 'A' && z <= 'F');
}

int urlDecode(const char *s, char *dec)
{
	char *o;
	const char *end = s + strlen(s);
	int c;

	for (o = dec; s <= end; o++) {
		c = *s++;
		if (c == '+') c = ' ';
		else if (c == '%' && (	!ishex(*s++)	||
					!ishex(*s++)	||
					!sscanf(s - 2, "%2x", &c)))
			return -1;

		if (dec) *o = c;
	}

	return o - dec;
}
