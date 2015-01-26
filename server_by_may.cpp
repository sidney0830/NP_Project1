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
void err_dump (const char *message);
void str_echo (int sockfd);
void welcome   (int sockfd);
void cut_line_pipe (int fd,char *line,int debug);
void clear_pipe();
bool isKnownCommand(char *path, char *cmd);
// void err_dump(const char *msg);

sig_atomic_t signaled = 0;

int main(int argc, char *argv[])
{
  int sockfd, newsockfd, clilen, childpid;
  struct sockaddr_in cli_addr, serv_addr;

  //pname = argv[0];
    clear_pipe();

    setenv("PATH","bin:.",1);

    int directory = chdir("./ras");
    cerr<<"port= "<<SERV_TCP_PORT<<endl;
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
void clear_pipe(){
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
  char line[MAXLINE];
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
        cut_line(sockfd,line,debug);
        // fprintf(stderr, "---after print input -----\n");


    }
}



int cut_line (int fd,char *line ,int debug)
{
    // fprintf(stderr, "begin cut line \n");
    char delim[10] = " \n\r\t";
    char * pch [MAXLINE];
    char *threeDArray[MAXLINE][MAXCMD];
    // memset(threeDArray, 0, sizeof(threeDArray[0][0]) * MAXLINE * MAXCMD);
    // printf ("Splitting string \"%s\" \n ",line);
    fprintf(stderr, "---Splitting string %s \n ",line);
    int line_sep_count=0;

    pch[line_sep_count] = strtok(line,delim);
    while (pch[line_sep_count] != NULL)
    {
        line_sep_count++;
        pch[line_sep_count] = strtok (NULL, delim);
    }

    int j=0;
    bool havePipe=0;
    int line_sep_count_now=0;
    for(int i=0,k=0;i<line_sep_count;i++)
    {
        threeDArray[j][k] = pch[i] ;
        threeDArray[j][k+1] = NULL ;
              fprintf(stderr, "@@%s ",threeDArray[j][k]);
        if(pch[i][0]=='|')
        {
            j++;
            k=0;
            havePipe=1;
            if(i==(line_sep_count-1)){line_sep_count_now=j;}
            else{ line_sep_count_now = j + 1;}
        }
        else
        {
            k++;
            // if(i==(line_sep_count-1)){line_sep_count_now=j;}
            // line_sep_count_now = line_sep_count+1;
        }
        // if(line_sep_count==1)){line_sep_count_now=1;}
    }
    if(!havePipe){line_sep_count_now=1;}


    // for(int j=0;j<line_sep_count;j++)
    // {
    //     for(int i=0;i<4;i++)
    //     {////////////////////
    //         fprintf(stderr, "-%s",threeDArray[j][i]);
    //     }
    //     fprintf(stderr, "\n");
    // }

    parse(fd,line_sep_count_now,threeDArray);

    // dup2(STDIN_OLD,0);//??
    dup2(STDOUT_OLD,1);//??
    // dup2(STDERR_OLD,2);//??
    // dup2(STDIN_OLD,0);
    // dup2(STDERR_OLD,2);
    // dup2
    return 0;
}



/*    line_sep_count : how many pipe*/
int parse(int fd,int line_sep_count,char*line[MAXLINE][MAXCMD])
{
    fprintf (stderr,"%slengh=%d  %s-%s-%s-%s\n","[[PARSE START]]",line_sep_count,line[0][0],line[0][1],line[0][2],line[0][3]);
    // fprintf (stderr,"sizeof line[0] = %d",sizeof(line[0])/sizeof( line[0][0] ));
    // for(int i=0;i<line_sep_count;i++)
    // {char *patt=getenv("PATH");
    //可能要有for去跑去掉空白的一個指令   //一行多個指令
    // prev_handler = signal (SIGINT, my_handler);

    int write_file=0;
    int WriteSame_index=-1;
    int WriteSame_count=-1;
    char *filename;
    char *patt ;

    // bool writeToPipe = false, readFromPipe = false;
    // char *legal_cmd="legal";
    // remove_space(line[MAXLINE])

    for(int j=0;j<line_sep_count;j++)
    {
        patt = getenv("PATH");
        if(line[j][0]==NULL)
        {
            return 0;
        }

        // if (isKnownCommand("bin", line[j][0])) {
        //     int unknownCmd = true;
        // }

        // cerr << "cmd: " << line[j][0] << endl;
        // bool tt = isKnownCommand("bin", line[j][0]);
        // cerr << "is know "<< tt <<endl;

        bool samepipe=0;
        bool known=1;
        // legal_cmd="legal";

        WriteSame_index= -1;
        WriteSame_count= -1;

        // char *a=line[j][0];

        bool writeToPipe = false, readFromPipe = false;
        if (line_sep_count==1)
        {
            // line[j][1]='\0';

        }
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
                    cerr << now_pipe_count << ", Need to Write to Pipe: " << line[j][0] << endl;
                    if(line[j][i][1]!='\0')/// have pipe_number
                    {
                        int temp = atoi(++line[j][i]);
                        /*pipe_array[now_pipe_count][0]:record to input which pipe*/
                        pipe_array [ now_pipe_count ][0] = temp;

                        int temp_write = pipe_array [ now_pipe_count + temp ][1];//

                        if( temp_write<0 )
                        {
                            pipe_array[ now_pipe_count + temp][1]= now_pipe_count;
                            cerr << "save " << now_pipe_count << "to " << (now_pipe_count + temp) << endl;

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

        fprintf(stderr, "cmd= %s ,cmd_count= %d\n",line[j][0],now_pipe_count);
        char *env;
        if(strcmp(line[j][0],"printenv")==0)
        {

            env = getenv(line[j][1]);
            // char *pathh;
            if(env != NULL)
            {
                send(fd, line[j][1], strlen(line[j][1]), 0);
                send(fd, "=", 1, 0);
                send(fd, env, strlen(env), 0);
                send(fd,"\n",1,0);/**  must display bin */
                cerr <<" printenv  =="<< env  << " env "<< endl;
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
        // bool tt = isKnownCommand("bin", line[j][0]);
        // cerr<< env <<" ,issssssssss" <<endl;
        // patt = strtok(patt,":");
        // patt = strtok(patt,":");
        //   while (patt != NULL)
        //   {
        //     // printf ("%s\n",patt);
        //     patt = strtok (NULL, ":");
        //   }
        char aaa[20];
        strcpy(aaa, getenv("PATH"));
        cerr <<" get PATH =="<< aaa  << " ~~~ "<< endl;
        cerr <<" line [j][0]=="<< line[j][0]  << " ~~~ "<< endl;
// dup2 ( fd , STDERR_FILENO);
        // dup(STDERR_FILENO)
        // dup2(STDIN_OLD,STDERR_FILENO);
        // STDERR_OLD  = dup(STDERR_FILENO) ;/*STORE ERR*/


        if ( !isKnownCommand( aaa , line[j][0] ) )
        {
            // dup2 (fd, STDERR_FILENO);
            STDOUT_OLD  = dup(1) ;
            dup2 (fd, STDOUT_FILENO);

            // fprintf(stderr, "errno = %d\n",errno);

            std::cout << "Unknown command: [" << line[j][0] << "]." <<std::endl;

            // dup2(pipe_fd [ pipe_array[now_pipe_count][1] ][1], STDOUT_FILENO);
            return 0;
        }

        cerr << "-after isKnownCommand"<< endl;
        // char *bbb=getenv("PATH");
        // cerr <<" get PATH =="<< bbb  << " ~~~ "<< endl;
        for (int i = 0; line[j][i]!=NULL; i++) /* do this write to file "<"  */
        {
            // fprintf (stderr,"i==%d,count=%d \n" , i ,line_sep_count);

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

        /*是第一個且不是最後一個 , 中間項 *//*梅考慮：最後是 ｜ */
        // if ( j == 0 && ((j+1) != line_sep_count) || j>0)
        // {
        //     cerr << now_pipe_count << ", Need to Write to Pipe: " << line[j][0] << endl;
        //     writeToPipe = true;

        // }
        // int once=0;
        // fprintf (stderr,"maydaycha: %s", line[j][i]);
        for(int i=0;i < now_pipe_count ;i++)
        {
            if(pipe_array[i][0]==0)//this now is pipe_array[now_pipe][1] = pipe fd
            {
                readFromPipe=true;
                cerr << now_pipe_count  << "＝now_pipe_count, Need to Read from Pipe: " << line[j][0] << endl;
            }
        }

        // if (j > 0) /** have to read from pipe *//*梅考慮：接前行要掃前面pipearray*/
        // {
        //     cerr << now_pipe_count  << ", Need to Read from Pipe: " << line[j][0] << endl;
        //     readFromPipe = true;

        //     // STDIN_OLD  = dup(STDIN_FILENO) ;
        //     // dup2 ( pipe_fd[now_pipe_count-1][0], STDIN_FILENO) ;//上衣個pipe
        //     // dup2 ( pipe_fd[0][0], STDIN_FILENO) ;//上衣個pipe
        //     // close( pipe_fd[0][0]) ;
        // }

        if (readFromPipe)
        {
            cerr << now_pipe_count << ", write close parent "<< pipe_array[now_pipe_count ][1] << endl;
            close(pipe_fd[ pipe_array[now_pipe_count ][1]][1] );
        }
        int childpid ;

        // cerr<< "before fork !!!!"<<endl;

        if ( (childpid = fork()) < 0)
            err_dump("server: fork error");
        else if (childpid > 0) //parent
        {
            // if()判斷是否要從stin讀或是pipe 讀
            // if( (j==0 && pipe_array[now_pipe_count][1]!=NULL) || j>0 )//have pipe
            // {
            //     STDIN_OLD  = dup(STDIN_FILENO);
            //     dup2 (pipe_fd[now_pipe_count-1][1],STDIN_FILENO);//上衣個pipe
            //     close(pipe_fd[now_pipe_count-1][1]);

            // }

            // close(pipe_fd[now_pipe_count][0]);
            // close(pipe_fd[now_pipe_count][1]);
            if (writeToPipe)//先
            {
                // cerr<<"do write to piep" << endl;
                /** 要處理多個指令的out給同個指令的情況
                    欸欸，根你說唷。
                    這個指令需要寫道pipe，所以關掉pipe的輸出端，
                    保留pipe的"讀取端"已提供之後的指令讀這次的output。
                    因為已經fork了，你的兒子已經有你關掉前的pipe的fd了
                */
                // int gotnum[10];
                // read(pipe_fd[pipe_array[now_pipe_count][1]][1], gotnum, sizeof(int));
                // printf("Received string: %d", gotnum);
                // write(pipe_fd[pipe_array[WriteSame_index][1]][1],&pipe_fd[pipe_array[WriteSame_index][1]][0], sizeof(int));
                // pipe_array[now_pipe_count+temp][1] //此index同,但裡面已揪有誰要write他的id
                // WriteSame_index = now_pipe_count+1;
                // WriteSame_count = now_pipe_count;
                    // fprintf (stderr,"maydaycha=");
                }
            if (readFromPipe)//後
            {

                // close(pipe_fd[ pipe_array[now_pipe_count ][1]][1] );
                close(pipe_fd[ pipe_array[now_pipe_count ][1]] [0]);//減一是因為single pipe
                cerr << now_pipe_count << ", readFromPipe" << endl;
            }
        }
        else if (childpid == 0)
        {
            STDERR_OLD = dup(2);
            /* this is child */
            // close(pipe_fd[now_pipe_count][0]);
            // fprintf (stderr,"--in dupp--\n");

            /**裡面要有cut_cmd 去切切切*/
          // STDIN_OLD  = dup(STDIN_FILENO);
          // STDOUT_OLD = dup(STDOUT_FILENO);

            if (write_file == 0)
                {
                    dup2(fd, STDOUT_FILENO);
                    // close(fd);
                }
          // dup2 ( fd , 1);

                if (readFromPipe) //後
                {
                    /**not the fisrt command*/

                    cerr << now_pipe_count << ",child read from pipe: " << pipe_array[now_pipe_count][1] << endl;

                    dup2(pipe_fd[pipe_array[now_pipe_count][1]][0], STDIN_FILENO);//減一是因為single pipe
                    // dup2(pipe_fd[pipe_array[now_pipe_count][1]][1], STDIN_FILENO);//減一是因為single pipe
                    close(pipe_fd[pipe_array[now_pipe_count][1]][0]);
                    cerr << now_pipe_count << ", read close child "<< pipe_array[now_pipe_count ][1] << endl;
                }
                if (writeToPipe) //先
                {
                    // write(pipe_fd[pipe_array[WriteSame_index][1]][1],&pipe_fd[ pipe_array[now_pipe_count][0] ][0], sizeof(int));
// fprintf (stderr,"maydaycha=");
                    if(samepipe)
                    {
                        cerr << now_pipe_count << ", in the samepipe" << endl;
                        cerr << now_pipe_count << ", write close child "<< pipe_array[now_pipe_count ][1] << endl;
                        dup2 (pipe_fd[ WriteSame_count ][1], STDOUT_FILENO);
                    }
                    else
                    {
                        dup2(pipe_fd[now_pipe_count][1], STDOUT_FILENO);
                        close(pipe_fd[now_pipe_count][1]);
                        // fprintf (stderr,"maydaycha==");

                    }
                }
             // fprintf (stderr,"--k\n"); // std::cout << "----in the fork----"  <<std::endl;
                // cerr << "execvp [j][0]==" <<line[j][0]<< endl;
                // cerr << "execvp [j][1]==" <<line[j][1]<< endl;

                // dup2 ( fd , STDOUT_FILENO);
                cerr <<  "!!!!!!!!!!!" << endl;
                if(execvp(line[j][0],line[j])== -1)
                {

                    // return 0;
                    // dup2(fd,STDOUT_FILENO);
                    // legal_cmd = "non";
                    // std::cout << "Unknown command: [" << line[j][0] << "]" <<std::endl;

                    // dup2(pipe_fd [ pipe_array[now_pipe_count][1] ][1], STDOUT_FILENO);
                    //  // signal(SIGCHLD, reaper);
                    // // j=line_sep_count;
                    // // std::cout << "Unknown command: [" << line[j][0] << "]" <<std::endl;
                    // exit(1);

                    // raise(SIGINT);
                    // signal (SIGINT, my_handler);
                    // return 0;
                    // cerr <<  "!!!!!!!!!!!" << endl;
                }
                // cerr <<  "!!!!!!!!!!!" << endl;
        }//childpid

        // cerr<< "after fork !!!!"<<endl;

        // wait(NULL);
        // cerr << "before wait" << endl;
        // cerr <<  "!!!!!!!!!!!" << endl;
        int status;
        wait(&status);
        // fprintf (stderr,"a--********************** %d---\n",status);
        // cerr << "after wait" << endl;
        // if((!readFromPipe) && (!writeToPipe)){
        //     close(pipe_fd[ pipe_array[now_pipe_count ][1]] [0]);
        //     close(pipe_fd[ pipe_array[now_pipe_count ][1]] [1]);
        // }
// cerr << "execvp [j][0]==" <<line[j][0]<< endl;
        // cerr <<  "!!!!!!!!!!!" << endl;
                cerr << "IIIID =" <<now_pipe_count<< endl;
//
        // fprintf (stderr,"IIIID = %d \n" , now_pipe_count);

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
    // }
    // memset(array, 0, sizeof(array[0][0]) * m * n);

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

void err_dump (const char *message)
{
    fprintf(stderr, "%s, errno = %d\n", message,errno);
    // //cout << msg << ", errno = " << errno << endl;
    // exit(1);
}

void welcome (int sockfd)
{
// "****************************************\n** Welcome to the information server. **\n****************************************\n";
    char string[200];
    strcpy(string,"****************************************\n** Welcome to the information server. **\n****************************************\n");
    write(sockfd, string,strlen(string) ) ;
                // err_dump("str_echo: writen error");

}
// isKnownCommand( aaa , line[j][0] )
bool isKnownCommand(char *path, char *cmd) {
    char *path_orignal=path;
    int i = 0;
    char *pch[20];
    char *pathh[10];
    struct dirent *ent;
    DIR *dir;
    // cerr<<"****isKnownCommand *****"<<endl;
     cerr << "path== " << path << endl;
     cerr << "cmd==" << cmd << endl;
     //  fprintf(stderr, "pch [i] =%s\n" , path);

    pch[0] = strtok(path,":");
    // pathh[0]=
    // fprintf(stderr, "pch [0] =%s\n",pch[0]);
// cerr << "****pch[i]= "<< pch[1] << endl;
    // threeDArray[line_sep_count][0]=strtok(pch[line_sep_count],delim_space);
    while (path[i] != '\0')
    {
        i++;
        pch[i] = strtok (NULL, ":"); // threeDArray[line_sep_count]=strtok(NULL,delim_space);
        if(pch[i]==NULL){break;}
        // pch[i] =
        fprintf(stderr, "pch [%d] =%s\n" ,i, pch[i]);

    }
    // cerr << "i : " << i << endl;
// fprintf(stderr, "pch [i] ==%s\n" , path);
    for(int j=0;j<i;j++)
    {
        // cerr << "pch[j]: " << pch[j] << endl;
        if ((dir = opendir(pch[j])) != NULL)
        {

            while ( (ent = readdir(dir)) != NULL)
            {
                cerr << "****read  dir ent = "<< dir << endl;
                cerr << "d_name: " << ent->d_name << endl;
                cerr << "cmd: " << cmd << endl;
                if (strcmp(ent->d_name, cmd) == 0)
                {
                    cerr << "**************************return true**************************" << endl;
                    // path=path_orignal;
                    closedir(dir);
                    return true;
                }
            }
            // path=path_orignal;
        closedir(dir);
        }
     }


    cerr << "-----------------return fasle**************************" << endl;
   return false;
}
