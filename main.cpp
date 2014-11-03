//
//  main.c
//  Test1021
//
//  Created by Sidney on 2014/10/21.
//  Copyright (c) 2014年 Sidney. All rights reserved.
//

#include <stdio.h>

#include <arpa/inet.h>
int main(int argc, char *argv[])
{
	int sockfd,newsockfd,clilen,childpid;
	struct sockaddr_in cli_addr, serv_addr;
	int pname = argv[0];
    
    
	/*
     * Open a TCP socket (an Internet stream socket).
	 */
	if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		err_dump("server: can't open stream socket");
        
	/*
     * Bind our local address so that the client can send to us.
     */
        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        serv_addr.sin_port = htons(SERV_TCP_PORT);
        if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
            err_dump("server: can't bind local address");
            listen(sockfd, 5);
            
            for ( ; ; )
            {
                clilen = sizeof(cli_addr);
                newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
                if (newsockfd < 0) err_dump("server: accept error");
                if ( (childpid = fork()) < 0) err_dump("server: fork error");
                else if (childpid == 0)
                { /* child process */
                    /* close original socket */
                    close(sockfd);
                    /* process the request */
                    str_echo(newsockfd);
                    exit(0);
                }
                close(newsockfd); /* parent process */
            }
}

#define MAXLINE 512
int str_echo(int sockfd)
{
	int n;
	char line[MAXLINE];
    
	for ( ; ; )
	{
		n = readline(sockfd, line, MAXLINE);
        
		if (n == 0) return; /* connection terminated */
		
		else if (n < 0) err_dump("str_echo: readline error");
		
		if (writen(sockfd, line, n) != n)
			err_dump("str_echo: writen error");
	}
}


int readline(int fd, char * ptr, int maxlen)
{
	int n, rc;
	char c;
	for (n = 1; n < maxlen; n++) 
	{
		if ( (rc = read(fd, &c, 1)) == 1) 
		{ 
            *ptr++ = c;
            if (c == '\n')  break;
		}
		else if (rc == 0)
		{
			if (n == 1)	return (0);/* EOF, no data read */
			else break;/* EOF, some data was read */
            
		}
		else
			return(-1); /*error */
		
	}
    
    
	*ptr = 0;
 	return(n);
}










