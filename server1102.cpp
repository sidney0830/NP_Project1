//
//  server.cpp
//  Test1021
//
//  Created by Sidney on 2014/10/21.
//  Copyright (c) 2014年 Sidney. All rights reserved.
//

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <regex>  
#include <sys/types.h>
#define SERV_TCP_PORT 3000

#define MAXLINE 10000
#define MAXPIPE 10000

using std::cout;
using namespace std;

int readline(int fd, char * ptr, int maxlen) ;
void err_dump (char *message);
void str_echo (int sockfd);
void welcome (int sockfd);
char * parse(int fd,int line_sep_count,char*wholeline[MAXLINE],char*line[MAXLINE][10]);
int cut_line (int fd,char *line,int debug);
int cut_line_pipe (int fd,char *line,int debug);
int STDIN_OLD,STDOUT_OLD;
int pipe_array[MAXPIPE][3];//0:ouput to whom // 1: get input ? //finish ?? 
int pipe_fd[MAXPIPE][2];
int now_cmd_count= 0;


int  main(int argc, char *argv[])
{
	int sockfd,newsockfd,clilen,childpid;
	struct sockaddr_in cli_addr, serv_addr;
	//pname = argv[0];

    setenv("PATH","bin:.",1);
    int directory = chdir("./ras");
	/*
     * Open a TCP socket (an Internet stream socket).
	*/
     if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
     {
        err_dump("server: can't open stream socket");
    }


	/*
     * Bind our local address so that the client can send to us.
     */
    bzero( (char *) &serv_addr, sizeof(serv_addr) );//clear
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(SERV_TCP_PORT);

    int yes=1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
    {
        perror("setsockopt");
        exit(1);
    }


    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        err_dump("server: can't bind local address");
    
    listen(sockfd, 5);
    // signal(SIGCHLD, reaper);
    for ( ; ; )
    {

        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, (socklen_t *)&clilen);
        
        std::cout<<"________________________________________"<<std::endl; 
        fprintf(stderr, "---------client connect successfull----\n");
        std::cout<<"________________________________________"<<std::endl; 
        // signal(SIGCHLD, reaper);////////
        if (newsockfd < 0) 
            err_dump("server: accept error");
        if ( (childpid = fork()) < 0) 
            err_dump("server: fork error");
        else if (childpid == 0)
        { /* child process */

            fprintf(stderr, "---------childpid ---------\n");
                /* close original socket */
            close(sockfd);

                    /* process the request */
            welcome(newsockfd);
            str_echo(newsockfd); 

            close(newsockfd);
            fprintf(stderr, "---------client close connention\n" );
            exit(0);
        }
        close(newsockfd); /* parent process */

    }
}

void str_echo(int sockfd)
{
	int n;
	char line[MAXLINE]={0};
    int debug=0;
    // char eee []="exit";
	for ( ; ; )
	{
        write(sockfd, "% ", 2);
        // line[MAXLINE]={0};
        memset(line, 0, sizeof line);
		// send(sockfd,a,strlen(a),0);
        n = readline(sockfd, line, MAXLINE);
        
        debug++;
        // std::cout<<"______________after readline____________"<<std::endl;     
       // fprintf(stderr, "---_after readline -----\n");

        if (n == 0) return; /* connection terminated */		
        else if (n < 0) 
        {
            err_dump("str_echo: readline error");
        } 
        // if (write(sockfd, line, strlen(line)) != n)
        // {
        //     err_dump("str_echo: writen error");
        // }


        // else 
        // {
        //     err_dump(line);
            // write(sockfd, line, strlen(line));
        
        // }
        // cutt
        cut_line_pipe(sockfd,line,debug);


    // if(line[4]=='\r')
//  {
  //     std::cout<<"55"<<line[4]<<"55"<<std::endl;
         // }
    fprintf(stderr, "---after print input -----\n");
        // std::cout<<"---after print input -----"<<std::endl;
        // std::cout<<"88"<<line[3]<<"88"<<std::endl;
        // std::cout<<"++"<<line[4]<<"++"<<std::endl;
        // std::cout<<"--"<<line[5]<<"--"<<std::endl;
        // std::cout<<"_____"<<strlen(line)<<"_______"<<std::endl;
        // if(strncmp(line,"exit",strlen(line)-1)==0)//include "?" and  "\n" '\r'
        // {            
        //     close(sockfd);
        //     exit(0);
        //     return;
        // }
        // if(strncmp(line,"printenv",strlen(line)-2)==0)
        // {

        //     write(sockfd,"")
        // }
        
    }
}



int cut_line_pipe(int fd,char *line,int debug)////////// 切切切 pipe！
{
    char *delim = "|";
    char *delim_space= " \n\r\t";
    char * pch [MAXLINE];
    char *threeDArray[MAXLINE][10];
    // memset(pch, 0, sizeof(pch[0][0]) * m * n);
    // printf ("Splitting string \"%s\" \n ",line);
    fprintf(stderr, "--((cut_line_pipe))-Splitting string:\"%s\"  ",line);
    int line_sep_count=0;
    pch[line_sep_count] = strtok(line,delim);
    // threeDArray[line_sep_count][0]=strtok(pch[line_sep_count],delim_space);
    while (pch[line_sep_count] != NULL)
    {
        line_sep_count++;
        now_cmd_count++;
        pch[line_sep_count] = strtok (NULL, delim);         
        // threeDArray[line_sep_count]=strtok(NULL,delim_space);
    } 
    int a;

    for (int i=0;i<line_sep_count;i++)
    {
        int line_sep_count_space=0;
        threeDArray[i][line_sep_count_space]=strtok(pch[i],delim_space);
        while (threeDArray[i][line_sep_count_space] != NULL)
        {

            line_sep_count_space++;
            threeDArray[i][line_sep_count_space] = strtok (NULL, delim_space);         
         // threeDArray[line_sep_count]=strtok(NULL,delim_space);
        } 
        a=line_sep_count_space;
        
    }
    fprintf(stderr, "-(list array)-\n");
    for(int j=0;j<line_sep_count;j++)
    {
        for(int i=0;i<4;i++)
        {////////////////////
            fprintf(stderr, "-%s",threeDArray[j][i]);
        }
        fprintf(stderr, "\n");
    }
    
   
    // {

    //     pipe_array[pipe_th][0]= find line[]; //output to whom ? //find " | "
    //     pipe_array[pipe_th][1]=  //intput get whose ? 
    //     pipe_array[pipe_th][2]=  //done or not



    // }

    // threeDArray


    parse(fd,line_sep_count,pch,threeDArray);
    dup2(STDIN_OLD,0);
    dup2(STDOUT_OLD,2);
memset(line, 0, sizeof line);



}

int pipe (int fd,char*line[MAXLINE] ,int pipe_th)
{

// pipe_fd[now_cmd])<0
    if ( pipe(pipe_fd[pipe_th]) < 0 )
    {
        //error
    }
    // for(int i=0;;i++)
    // {

    //     pipe_array[pipe_th][0]= find line[]; //output to whom ? //find " | "
    //     pipe_array[pipe_th][1]=  //intput get whose ? 
    //     pipe_array[pipe_th][2]=  //done or not

    // }  
    // // int **pipe_array ＝new int [];
    // pipe_array
    // MAXPIPE


}
bool has_any_digits(const std::string& s)
{
    return std::any_of(s.begin(), s.end(), ::isdigit);
}

char * parse(int fd,int line_sep_count,char*wholeline[MAXLINE],char*line[MAXLINE][10])
{
    fprintf (stderr,"%slengh=%d#%s-%s-%s-%s\n","[[PARSE START]]",line_sep_count,line[0][0],line[1][0],line[2][0],line[3][0]);
    // for(int i=0;i<line_sep_count;i++)
    // {
    //可能要有for去跑去掉空白的一個指令   //一行多個指令
    int write_file=0;
    char *filename;
    
    // remove_space(line[MAXLINE])
    for(int j=0;j<line_sep_count;j++)
    {
        char *a=line[j][0];
        string aa;
        aa.assign(a);
        fprintf(stderr, "\ncmd =%s\n",line[j][0]);
        // std::regex e ("\d+");
        // static const std::boost::regex e("(\d)+");
         // regex_match(line[j][0], e);

        // if(std::regex_match(aa ,std::regex("(\d)+"))){
        // fprintf(stderr, "+++++++\n");

        // }
        if( line_sep_count>1 )//must have pipe
        {
        //
            // // threeDArray     
            // for(int j=0;j<line_sep_count;j++)
            // {
            //     if (threeDArray[j][0])////
            //     pipe_array[]
            //     fprintf(stderr, "\n");
            // }
        }
        if(has_any_digits(aa))
        {
            
            fprintf(stderr, "have pipe :%s+++\n",aa.c_str());
            int index=0;
            for (int index = 0; line[j][index+1]!=NULL; index++)//remove pipe number
            {
                // strcpy(line[j][index],NULL);
                line[j][index]=0;
// fprintf(stderr, "--j=%d,index=%d**%s\n",j,index,line[j][index]);
                strcpy(line[j][index],line[j][index+1]);
                 fprintf(stderr, "j=%d,index=%d**%s\n",j,index,line[j][index]);
                // array[length-1] = 0;
            }
            // strcpy(line[j][index],NULL);
         line[j][index]=0;

          for(int jj=0;jj<line_sep_count;jj++)
          {
             for(int i=0;i<4;i++)
             {
            fprintf(stderr, "+%s",line[jj][i]);
             }
        fprintf(stderr, "\n");
          }
            int pipe_nume= atoi( aa.c_str() ) ; 

          //   if(pipe(pipe_fd[now_cmd])<0){
          //   err_dump("pipe error");
          // }
        }

     fprintf(stderr, "cmmm= %s\n",line[j][0]);
        if(strcmp(line[j][0],"printenv")==0)
        {
            
                ///裡面要有cut_cmd 去切切切
            char *env;
            env = getenv(line[j][1]);
            if(env != NULL)
            {
                send(fd,env,strlen(env),0);
                send(fd,"\n",1,0);
                // output();
                // printf ("***********%s********\n","PATH");
            }
            else
                err_dump("get env error");
            return 0;
        }
        else if(strcmp(line[j][0],"setenv")==0)
        {
            // printf ("*********************%s********\n","sentev");
            setenv(line[j][1], line[j][2], 1);
            return 0;           
        }
        else if(strcmp(line[j][0],"exit")==0)///////////////// ???
        {
            close(fd);
            exit(0);
        }
        for (int i = 0; line[j][i]!=NULL; i++) 
        {
            fprintf (stderr,"i==%d,count=%d \n",i,line_sep_count);
            // fprintf (stderr,"maydaycha: %d", line_sep_count);
            if (strcmp(line[j][i], ">") == 0) 
            {
                
                filename = line[j][i + 1];
                // fprintf (stderr,"maydaycha: %s\n", filename);
                int filefd = open (filename,O_WRONLY|O_CREAT|O_TRUNC,0666);
                dup2(filefd,STDOUT_FILENO);
                close(filefd);
                write_file = 1;
                for(int k=i;line[j][k+1]!=NULL;k++)
                {
                    line[j][k] = NULL;
                    line[j][k + 1] = NULL;
                }
                
                break;
            }
        }//for

 fprintf (stderr,"--begin forkkk\n");
            // printf ("begin forkkk");
            
        int childpid ;
        // childpid=-1;
        if ( (childpid = fork()) < 0) 
            err_dump("server: fork error");
        else if (childpid == 0)
        {

            ///裡面要有cut_cmd 去切切切
            STDIN_OLD  = dup(0);
            STDOUT_OLD = dup(2);
            if (write_file == 0) dup2(fd, 1); 
            // dup2 ( fd , 2); 
             // std::cout << "----in the fork----"  <<std::endl;
            if(execvp(line[j][0],line[j])== -1)
            {
                std::cout << "Unknown command: [" << line[j][0] << "]" <<std::endl;
                return 0;
            }
            
            // close(fd);  
            // dup2(fd, 2);// dup2 pipe[1] ,2          
            close(1);
            close(2);
            close(fd);
              // exit(0);
        }
        wait(NULL);           
        // return 0;
        // }
        
        // printf ("***********%s********\n",line[0]);
        // if(execvp(cmlined[0],line)==-1)
        // {
        //     cout << "Unknown command: [" << line[0] << "]" <<endl;
        //     exit(0);
        // 
        // }
     fprintf (stderr,"--finish forkkk\n");   
    }
    // }
// memset(array, 0, sizeof(array[0][0]) * m * n);

}
// int output(int fd,char* output_line)
// {


//     oid display(int sockfd,string s){
//     char *a=new char[s.size()+1];
//     a[s.size()]=0;
//     memcpy(a,s.c_str(),s.size());   
//     send(sockfd,a,strlen(a),0);
// }
int readline(int fd, char * ptr, int maxlen) 
{ 
    int n, rc; char c; 
    for (n = 1; n < maxlen; n++)
    { 
        if ( (rc = read(fd, &c, 1)) == 1) 
        { 
            *ptr++ = c; 
            if (c == '\n') break; 
        }
        else if (rc == 0) 
        { 
            if (n == 1) return(0); /* EOF, no data read */ 
            else break; /* EOF, some data was read */ 
        } 
        else 
            return(-1); /* error */ 
    } 
*ptr = 0; 
return(n); 
}

void err_dump (char *message)
{
    fprintf(stderr, "%s, errno = %d\n", message,errno);
    // //cout << msg << ", errno = " << errno << endl;
    // exit(1);
}

void welcome (int sockfd)
{
    // write (sockfd,,);
// "****************************************\n** Welcome to the information server. **\n****************************************\n";
    char *string;
    string="***************\n** Welcome to the information server. **\n****************************************\n";
    write(sockfd, string,strlen(string) ) ;
                // err_dump("str_echo: writen error");

}

// void accept()
// {




// }
int cut_line (int fd,char *line,int debug)
{

    char *delim = " \n\r\t";
    char * pch [MAXLINE];
    
    // printf ("Splitting string \"%s\" \n ",line);
    fprintf(stderr, "---Splitting string \"%s\" \n ",line);
    int line_sep_count=0;
    pch[line_sep_count] = strtok(line,delim);
    while (pch[line_sep_count] != NULL)
    {

        line_sep_count++;
        pch[line_sep_count] = strtok (NULL, delim);         

    } 
    // parse(fd,line_sep_count,pch);
    dup2(STDIN_OLD,0);
    dup2(STDOUT_OLD,2);
    // printf ("=cut line=%d===debg = %d/",line_sep_count,debug);

    // printf ("===2===%s====\n",pch[1]);

    // printf ("===3===%s====\n",pch[2]);


    return 0;
}




