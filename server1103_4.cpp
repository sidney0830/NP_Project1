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
#define MAXPIPE 10000  //pipe nubers
#define MAXCMD  10
#define MAXINTPIPE 1001 //pipe |N   1=<N<=1000
using std::cout;
using namespace std;

int readline(int fd, char * ptr, int maxlen) ;
void err_dump (char *message);
void str_echo (int sockfd);
void welcome (int sockfd);
char * parse(int fd,int line_sep_count,char*line[MAXLINE][MAXCMD]);
int cut_line (int fd,char *line,int debug);
int cut_line_pipe (int fd,char *line,int debug);
int STDIN_OLD,STDOUT_OLD,STDERR_OLD;
int pipe_array[MAXPIPE][3];//0:ouput to whom // 1: get input ? //finish ?? 
int pipe_fd[MAXPIPE][2];
int now_pipe_count= 0;
int now_count=0;
// int now_pipe_count = 0;
void clear_pipe();

int  main(int argc, char *argv[])
{
	int sockfd,newsockfd,clilen,childpid;
	struct sockaddr_in cli_addr, serv_addr;
	//pname = argv[0];
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
        fprintf(stderr, "---_after readline -----\n");

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
        fprintf(stderr, "---after print input -----\n");

        
    }
}



int cut_line_pipe(int fd,char *line,int debug)////////// 切切切 pipe！
{
    char *delim = "|";
    char *delim_space= " \n\r\t";
    char * pch [MAXLINE];
    char *threeDArray[MAXLINE][MAXCMD];
    // memset(pch, 0, sizeof(pch[0][0]) * m * n);
    // printf ("Splitting string \"%s\" \n ",line);
    fprintf(stderr, "--((cut_line_pipe))-Splitting string:\"%s\"  ",line);
    int line_sep_count=0;
    pch[line_sep_count] = strtok(line,delim);
    // threeDArray[line_sep_count][0]=strtok(pch[line_sep_count],delim_space);
    while (pch[line_sep_count] != NULL)
    {
        line_sep_count++;
        now_pipe_count++;
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


    parse(fd,line_sep_count,threeDArray);
    dup2(STDIN_OLD,0);
    dup2(STDERR_OLD,2);
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

int cut_line (int fd,char *line ,int debug)
{
    fprintf(stderr, "begin cut line \n");
    char *delim = " \n\r\t";
    char * pch [MAXLINE];
    char *threeDArray[MAXLINE][MAXCMD];
    memset(threeDArray, 0, sizeof(threeDArray[0][0]) * MAXLINE * MAXCMD);
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

    // for(int j=0;j<line_sep_count;j++)
    // {
    //     for(int i=0;i<4;i++)
    //     {////////////////////
    //         fprintf(stderr, "-%s",threeDArray[j][i]);
    //     }
    //     fprintf(stderr, "\n");
    // }

   parse(fd,line_sep_count,threeDArray);
   dup2(STDIN_OLD,0);
   dup2(STDOUT_OLD,2);
    // printf ("=cut line=%d===debg = %d/",line_sep_count,debug);

    // printf ("===2===%s====\n",pch[1]);

    // printf ("===3===%s====\n",pch[2]);


   return 0;
}


bool has_any_digits(const std::string& s)
{
    return std::any_of(s.begin(), s.end(), ::isdigit);
}


char * parse(int fd,int line_sep_count,char*line[MAXLINE][MAXCMD])
{
    fprintf (stderr,"%slengh=%d  %s-%s-%s-%s\n","[[PARSE START]]",line_sep_count,line[0][0],line[1][0],line[2][0],line[3][0]);
    // fprintf (stderr,"sizeof line[0] = %d",sizeof(line[0])/sizeof( line[0][0] ));
    // for(int i=0;i<line_sep_count;i++)
    // {
    //可能要有for去跑去掉空白的一個指令   //一行多個指令

    int write_file=0;
    int WriteSame_index=-1;
    int WriteSame_count=-1;
    char *filename;

    // bool writeToPipe = false, readFromPipe = false;
    char *legal_cmd="legal";
    // remove_space(line[MAXLINE])
    for(int j=0;j<line_sep_count;j++)
    {
        bool ifneedtopiep=0;
        bool known=1;
        legal_cmd="legal";

        WriteSame_index=-1;
        WriteSame_count=-1;

        // char *a=line[j][0];
        
        bool writeToPipe = false, readFromPipe = false;
        // string aa;
        // aa.assign(a);


        // fprintf(stderr, "\n cmd =%s\n",line[j][0]);
        // std::regex e ("\d+");
        // static const std::boost::regex e("(\d)+");
         // regex_match(line[j][0], e);
        if( line_sep_count > 1 )//must have pipe
        {
            //
            // // threeDArray     
            for(int i=0; line[j][i]!=NULL ;i++)
            {
                if (line[j][i][0]=='|')///
                {
                    if(line[j][i][1]!=NULL)/// have pipe_number 
                    { 
                        int temp = atoi(++line[j][i]);
                        /*pipe_array[now_pipe_count][0]:record to input which pipe*/
                        pipe_array[now_pipe_count][0] = temp;

                        int temp_write = pipe_array[now_pipe_count+temp][1];//

                        if(temp_write<0)
                        { 
                            pipe_array[now_pipe_count+temp][1]= now_pipe_count;
                        }
                        else
                        {
                            WriteSame_index = now_pipe_count + temp;//要寫道哪個
                            WriteSame_count = now_pipe_count;//現在的 pipe  id 
                        } 
                        /*pipe_array[now_pipe_count+temp][1]:record  whom will get pipe*/
                    }
                    else
                    {
                        // int temp = atoi(++line[j][i]);
                        pipe_array[now_pipe_count][0]= 1;
                        int temp_write=pipe_array[now_pipe_count+1][1];
                        if(temp_write<0) 
                        {
                            pipe_array[now_pipe_count+1][1]= now_pipe_count;
                        }
                        else//原本就在的不要動他
                        {
                            WriteSame_index = now_pipe_count+1;
                            WriteSame_count = now_pipe_count;
                        } 

                        
                    }
                    
                    if(pipe(pipe_fd[now_pipe_count])<0)
                    {
                        err_dump("pipe error");
                    }
                    
                    line[j][i]=NULL;
                    ifneedtopiep=1;

                } 

            }

        }

       
        //   for(int jj=0;jj<line_sep_count;jj++)
        //   {
        //      for(int i=0;i<4;i++)
        //      {
        //     fprintf(stderr, "+%s",line[jj][i]);
        //      }
        // fprintf(stderr, "\n");
        //   }
        //     int pipe_nume= atoi( aa.c_str() ) ; 

        //   //   if(pipe(pipe_fd[now_cmd])<0){
        //   //   err_dump("pipe error");
        //   // }

        // }

         fprintf(stderr, "cmd= %s cmd_count= %d\n",line[j][0],now_pipe_count);
         if(strcmp(line[j][0],"printenv")==0)
         {

                ///裡面要有cut_cmd 去切切切
            char *env;
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

        else if(strcmp(line[j][0],"exit")==0)///////////////// ???
        {
            close(fd);
            exit(0);
        }
        for (int i = 0; line[j][i]!=NULL; i++) /* do this write to file "<"  */
        {
            fprintf (stderr,"i==%d,count=%d \n" , i ,line_sep_count);
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

        // fprintf (stderr,"--begin forkkk\n");
            // printf ("begin forkkk");
        // for(int i=0 ;i < now_pipe_count ;i++)
        // {
        //     if(pipe_array[i][0]==0)readFromPipe=true;
        // }
        /*是第一個且不是最後一個 , 中間項 *//*梅考慮：最後是 ｜ */
        if ( (j == 0 &&((j+1) != line_sep_count)) || ((j+1) != line_sep_count))
        {
            cerr << now_pipe_count << ", Need to Write to Pipe: " << line[j][0] << endl;
            writeToPipe = true;
            
        }
        int once=0;
        for(int i=0;i < now_pipe_count ;i++)
        {
            if(pipe_array[i][0]==0)//this now is pipe_array[now_pipe][1] = pipe fd
            {
                readFromPipe=true;
                cerr << now_pipe_count  << ", Need to Read from Pipe: " << line[j][0] << endl;
                once++;
            }
            if(once>1)
            {

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
        



        int childpid ;

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
                close(pipe_fd[now_pipe_count][1]);
            }
            if (readFromPipe)//後
            {
                close(pipe_fd[pipe_array[now_pipe_count][1]][0]);//減一是因為single pipe
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

              if (write_file == 0) dup2(fd, STDOUT_FILENO); 
          // dup2 ( fd , 1); 

                if (readFromPipe) //後
                {
                    /**not the fisrt command*/

                    dup2(pipe_fd[pipe_array[now_pipe_count][1]][0], STDIN_FILENO);//減一是因為single pipe
                    // dup2(pipe_fd[pipe_array[now_pipe_count][1]][1], STDIN_FILENO);//減一是因為single pipe
                    close(pipe_fd[pipe_array[now_pipe_count][1]][0]);
                }
                if (writeToPipe) //先
                {
                    // write(pipe_fd[pipe_array[WriteSame_index][1]][1],&pipe_fd[ pipe_array[now_pipe_count][0] ][0], sizeof(int)); 
                    dup2(pipe_fd[now_pipe_count][1], STDOUT_FILENO);
                    close(pipe_fd[now_pipe_count][1]);
                }
             // fprintf (stderr,"--k\n"); // std::cout << "----in the fork----"  <<std::endl;
                cerr << "exec command!!!" << endl;
                dup2 ( fd , STDERR_FILENO);
                if(execvp(line[j][0],line[j])== -1)
                {
                    dup2(fd,STDOUT_FILENO);
                    legal_cmd = "non";
                    std::cout << "Unknown command: [" << line[j][0] << "]" <<std::endl;
                    // fprintf (stderr,"legaglll=%d $$$$$$\n",legal_cmd); 
                    dup2(pipe_fd[pipe_array[now_pipe_count][1]][1], STDOUT_FILENO);
                    // j=line_sep_count;
                    // std::cout << "Unknown command: [" << line[j][0] << "]" <<std::endl;
                    return 0;
                }

        }//childpid
        // if(childpid>0){}
            wait(NULL);           

            fprintf (stderr,"!!!finish forkkk\n");
            if( strcmp(legal_cmd,"legal")==0)/*應該要legal, 且前面有pipe ,才要 now_cont ＋1 */
            {
                for (int i=0;i<=now_pipe_count;i++)pipe_array[i][0]--;//所有都要 --
                 // fprintf (stderr,"legaglll=%d $$$$$$\n",legal_cmd); 
                    now_count++;
                fprintf (stderr,"count=%d\n",now_count);              
                
            }   
            if( strcmp(legal_cmd,"legal")==0 && line_sep_count>1)/*應該要legal, 且前面有pipe ,才要 now_cont ＋1 */
            {
               
                now_pipe_count++;
                
            } 
            if(strcmp(legal_cmd,"non")==0)
            {
                fprintf (stderr,"not legaglll $$$$$$\n");
            }
// fprintf (stderr,"not legagllll  $$$$$$$$ \n");
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
    // write (sockfd,,);
// "****************************************\n** Welcome to the information server. **\n****************************************\n";
    char *string;
    string="****************************************\n** Welcome to the information server. **\n****************************************\n";
    write(sockfd, string,strlen(string) ) ;
                // err_dump("str_echo: writen error");

}

// void accept()
// {




// }
