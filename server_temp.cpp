//
//  server.cpp
//  Test1021
//
//  Created by Sidney on 2014/10/21.
//  Copyright (c) 2014年 Sidney. All rights reserved.
//
// #include <wait.h>
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
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>
#include <dirent.h>

#define SERV_TCP_PORT 5566

#define MAXLINE 10100
#define MAXPIPE 10100  //pipe nubers
#define MAXCMD  20
#define MAXINTPIPE 10100 //pipe |N   1=<N<=1000
using std::cout;
using namespace std;

int STDIN_OLD,STDOUT_OLD,STDERR_OLD;
int pipe_array[MAXPIPE][3];//0:ouput to whom // 1: get input ? //finish ??
int pipe_fd[MAXPIPE][2];
int now_pipe_count= 0;

int readline(int fd, char * ptr, int maxlen) ;
int parse(int fd,int line_sep_count,char*line[MAXLINE][MAXCMD]);
int cut_line (int fd,char *line,int debug);
void err_dump (char *message);
void str_echo (int sockfd);
void welcome   (int sockfd);
void cut_line_pipe (int fd,char *line,int debug);
void clear_pipe();
bool isKnownCommand(char *path, char *cmd);
void err_dump(const char *msg);

sig_atomic_t signaled = 0;

int  main(int argc, char *argv[])
{
	int sockfd,newsockfd,clilen,childpid;
	struct sockaddr_in cli_addr, serv_addr;

    clear_pipe();

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
        std::cout<<"________client connect successfull______"<<std::endl;
        std::cout<<"________________________________________"<<std::endl;

        // signal(SIGCHLD, reaper);////////

        if (newsockfd < 0)
            err_dump("server: accept error");

        if ( (childpid = fork()) < 0)
            err_dump("server: fork error");

        else if (childpid == 0)
        {
            /* child process */
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
        // wait(NULL);
        close(newsockfd); /* parent process */
    }
}

void clear_pipe()
{
    for(int i=0; i < MAXPIPE; i++)
    {
        pipe_array[i][0] = -1;
        pipe_array[i][1] = -1;
        pipe_array[i][2] = -1;

    }
}

void str_echo(int sockfd)
{
	int n;
    int debug=0;
	char line[MAXLINE];

   // char eee []="exit";
    for ( ; ; )
    {
        write(sockfd, "% ", 2);
        // line[MAXLINE]={0};
        memset(line, 0, sizeof line);
		// send(sockfd,a,strlen(a),0);
        n = readline(sockfd, line, MAXLINE);

        debug++;

        if (n == 0) return ; /* connection terminated */
        else if (n < 0)
        {
            err_dump("str_echo: readline error");
        }

        cut_line(sockfd,line,debug);
    }
}

int cut_line (int fd,char *line ,int debug)
{
    // fprintf(stderr, "begin cut line \n");
    char delim[10] = " \n\r\t";
    char * pch [MAXLINE];
    char *threeDArray[MAXLINE][MAXCMD];

    // fprintf(stderr, "---Splitting string %s \n ",line);
    int line_sep_count=0;
    pch[line_sep_count] = strtok(line,delim);
    while (pch[line_sep_count] != NULL)
    {
        line_sep_count++;
        pch[line_sep_count] = strtok (NULL, delim);
    }

    int j=0;

    for(int i=0,k=0;i<line_sep_count;i++)
    {
        threeDArray[j][k] = pch[i] ;
              // fprintf(stderr, "@@%s",threeDArray[j][k]);
        if(pch[i][0]=='|')
        {
            j++;
            k=0;
        }
        else
        {
            k++;
        }
    }

    line_sep_count=j+1;



    parse(fd,line_sep_count,threeDArray);
    dup2(STDIN_OLD,0);
    dup2(STDOUT_OLD,2);

    return 0;
}




int parse(int fd,int line_sep_count,char*line[MAXLINE][MAXCMD])
{
    // fprintf (stderr,"%slengh=%d,%s-%s-%s-%s\n","[[PARSE START]]",line_sep_count,line[0][0],line[1][0],line[2][0],line[3][0]);

    int write_file=0;
    int WriteSame_index=-1;
    int WriteSame_count=-1;
    char *filename;
    char *patt ;


    for(int j=0;j<line_sep_count;j++)
    {
        patt = getenv("PATH");
        if(line[j][0]==NULL)
        {
            return 0;
        }



        bool samepipe=0;
        bool known=1;
        // legal_cmd="legal";

        WriteSame_index= -1;
        WriteSame_count= -1;

        // char *a=line[j][0];

        bool writeToPipe = false, readFromPipe = false;

        if( line_sep_count > 1 )//must have pipe
        {
            //
            // // threeDArray
            for(int i=0; line[j][i]!=NULL ;i++)
            {
                writeToPipe = false;
                if (line[j][i][0]=='|')///
                {

                    writeToPipe = true;
                    // cerr << now_pipe_count << ", Need to Write to Pipe: " << line[j][0] << endl;
                    if(line[j][i][1]!='\0')/// have pipe_number
                    {
                        int temp = atoi(++line[j][i]);
                        /*pipe_array[now_pipe_count][0]:record to input which pipe*/
                        pipe_array [ now_pipe_count ][0] = temp;

                        int temp_write = pipe_array [ now_pipe_count + temp ][1];//

                        if( temp_write<0 )
                        {
                            pipe_array[ now_pipe_count + temp][1]= now_pipe_count;
                            // cerr << "save " << now_pipe_count << "to " << (now_pipe_count + temp) << endl;

                            if (pipe(pipe_fd[now_pipe_count]) < 0)
                            {
                                err_dump("pipe error");
                            }
                        }
                        else
                        {
                            WriteSame_index = now_pipe_count + temp;//要寫道哪個
                            WriteSame_count = temp_write;//現在的 pipe  id
                            samepipe = 1;
                        }
                        /*pipe_array[now_pipe_count+temp][1]:record  whom will get pipe*/
                    }
                    else
                    {
                        // int temp = atoi(++line[j][i]);
                        pipe_array[now_pipe_count][0] = 1;
                        int temp_write = pipe_array [now_pipe_count + 1][1];
                        if(temp_write<0)
                        {
                            pipe_array[ now_pipe_count + 1 ][ 1 ]= now_pipe_count;

                            if(pipe(pipe_fd[now_pipe_count])<0)
                            {
                                err_dump("pipe error");
                            }
                        }
                        else//原本就在的不要動他
                        {

                            WriteSame_index = now_pipe_count + 1;
                            WriteSame_count = temp_write;
                            samepipe = 1;

                        }
                    }

                    line[j][i] = NULL;

                    // now_pipe_count++;
                }

            }

        }//must have pipe

        // fprintf(stderr, "cmd= %s ,cmd_count= %d\n",line[j][0],now_pipe_count);
        char *env;
        if(strcmp(line[j][0],"printenv")==0)
        {

            ///裡面要有cut_cmd 去切切切

            env = getenv(line[j][1]);
            // char *pathh;
            if(env != NULL)
            {
                send(fd,line[j][1],strlen(line[j][1]),0);
                send(fd,"=",1,0);
                send(fd,env,strlen(env),0);
                send(fd,"\n",1,0);/**  must display bin */
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
        else if(strcmp(line[j][0],"remove")==0)
        {
                // printf ("*********************%s********\n","sentev");
            setenv(line[j][1],"", 1);
            return 0;
        }
        if(strcmp(line[j][0],"exit")==0)///////////////// ???
        {
            close(fd);
            exit(0);
        }


        char *aaa=getenv("PATH");
        // cerr << aaa  << "aaaaa ~~~ "<< endl;

        if ( !isKnownCommand( aaa , line[j][0] ) )
        {
            dup2 (fd, STDERR_FILENO);
            dup2 (fd, STDOUT_FILENO);

            std::cout << "Unknown command: [" << line[j][0] << "]" <<std::endl;

            // dup2(pipe_fd [ pipe_array[now_pipe_count][1] ][1], STDOUT_FILENO);
            return 0;
        }
        // cerr << "-after isKnownCommand"<< endl;
        for (int i = 0; line[j][i]!=NULL; i++) /* do this write to file "<"  */
        {

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
        }
        // if(access( ,F_OK)!=-1)

        for(int i=0;i < now_pipe_count ;i++)
        {
            if(pipe_array[i][0]==0)//this now is pipe_array[now_pipe][1] = pipe fd
            {
                readFromPipe=true;
                // cerr << now_pipe_count  << "＝now_pipe_count, Need to Read from Pipe: " << line[j][0] << endl;
            }
        }


        if (readFromPipe)
        {
            // cerr << now_pipe_count << ", write close parent "<< pipe_array[now_pipe_count ][1] << endl;
            close(pipe_fd[ pipe_array[now_pipe_count ][1]][1] );
        }
        int childpid ;

        // cerr<< "before fork !!!!"<<endl;

        if ( (childpid = fork()) < 0)
            err_dump("server: fork error");
        else if (childpid > 0) //parent
        {

            if (writeToPipe)//先
            {

            }
            if (readFromPipe)//後
            {

                // close(pipe_fd[ pipe_array[now_pipe_count ][1]][1] );
                close(pipe_fd[ pipe_array[now_pipe_count ][1]] [0]);//減一是因為single pipe
                // cerr << now_pipe_count << ", readFromPipe" << endl;
            }
        }
        else if (childpid == 0)
        {
            STDERR_OLD = dup(2);
            /* this is child */


            if (write_file == 0) dup2(fd, STDOUT_FILENO);
          // dup2 ( fd , 1);

                if (readFromPipe) //後
                {
                    /**not the fisrt command*/

                    // cerr << now_pipe_count << ",child read from pipe: " << pipe_array[now_pipe_count][1] << endl;

                    dup2(pipe_fd[pipe_array[now_pipe_count][1]][0], STDIN_FILENO);//減一是因為single pipe
                    // dup2(pipe_fd[pipe_array[now_pipe_count][1]][1], STDIN_FILENO);//減一是因為single pipe
                    close(pipe_fd[pipe_array[now_pipe_count][1]][0]);
                    // cerr << now_pipe_count << ", read close child "<< pipe_array[now_pipe_count ][1] << endl;
                }
                if (writeToPipe) //先
                {
                    if(samepipe)
                    {
                        // cerr << now_pipe_count << ", in the samepipe" << endl;
                        // cerr << now_pipe_count << ", write close child "<< pipe_array[now_pipe_count ][1] << endl;
                        dup2 (pipe_fd[ WriteSame_count ][1], STDOUT_FILENO);
                    }
                    else
                    {
                        dup2(pipe_fd[now_pipe_count][1], STDOUT_FILENO);
                        close(pipe_fd[now_pipe_count][1]);
                    }
                }
                if(execvp(line[j][0],line[j])== -1)
                {

                }
        }//childpid


        // cerr << "before wait" << endl;
        int status;
        wait(&status);

        // cerr << "after wait" << endl;


        // fprintf (stderr,"IIIIIIIID = %d \n" , now_pipe_count);

        if (status == 0)
        {

           for (int i=0;i<=now_pipe_count;i++)pipe_array[i][0]--;
            now_pipe_count++;
        }
        else
        {
           close(pipe_fd[pipe_array[now_pipe_count][1]][0]);
           close(pipe_fd[pipe_array[now_pipe_count][1]][1]);
        }



    }


    return 0;
}

int readline(int fd, char * ptr, int maxlen)
{
   // fprintf (stderr,"--rrrrrrrrrrrrr\n");
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
    char string[200];
    strcpy(string,"****************************************\n** Welcome to the information server. **\n****************************************\n");
    write(sockfd, string,strlen(string) ) ;
}

bool isKnownCommand(char *path, char *cmd)
{
    int i=0;
    char* pch[20];
    char*pathh[10];
    // cerr<<"****isKnownCommand *****"<<endl;
     // cerr<<"path  "<<path<<endl;

    pch[0] = strtok(path,":");

    while (path[i] != '\0')
    {
        i++;
        pch[i] = strtok (NULL, ":"); // threeDArray[line_sep_count]=strtok(NULL,delim_space);
        if( pch[i] == NULL ){   break;  }

        // fprintf(stderr, "pch [%d] =%s\n" ,i, pch[i]);
    }
// fprintf(stderr, "pch [i] ==%s\n" , path);
    for(int j=0;j<i;j++)
    {
        struct dirent *ent;
        DIR *dir;
        if ((dir = opendir(pch[j])) != NULL)
        {

            while ( (ent = readdir(dir)) != NULL)
            {
                // cerr << "****read  dir ent = "<< dir << endl;
                if (strcmp(ent->d_name, cmd) == 0)
                {
                    closedir(dir);
                    return true;
                }
            }
        }
        closedir(dir);
     }

   return false;
}

void err_dump(const char *msg)
{
        fprintf(stderr, "%s, errno = %d\n", msg,errno);
    // //cout << msg << ", errno = " << errno << endl;
        exit(1);
}
