#include <fcntl.h>
#include <iostream>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <algorithm>
#include <vector> 
#include <functional>

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)
#define PORTNUM 5566
#define pipe_limit 1000
#define welcomemsg "****************************************\n** Welcome to the information server. **\n****************************************\n\n% "
using namespace std; 

void str_echo(int sockfd);
int readline(int fd, char * ptr, int maxlen);
int bigpipe[pipe_limit][3]={0};
string wd="./bin/";
int igc=0;
bool ig=false;
int main(int argc, char *argv[]) { 
	 
	int sockfd,newsockfd,clilen,childpid;
	struct sockaddr_in	cli_addr,serv_addr;
	socklen_t socksize = sizeof(struct sockaddr_in);
	chdir("./ras");
	setenv("PATH","bin:.",1);
	if((sockfd=socket(AF_INET,SOCK_STREAM,0))<0)
		cout << "error creating server socket!!";
	bzero((char *) &serv_addr,sizeof(serv_addr));
	serv_addr.sin_family 		= AF_INET;
	serv_addr.sin_addr.s_addr	=htonl(INADDR_ANY);
	serv_addr.sin_port 		=htons(PORTNUM);

	int sock_opt = 1;
  	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (void*)&sock_opt, sizeof(sock_opt) ) == -1){
		perror("setsockopt error");
	    	exit(1);
	}
	 
 	if(bind(sockfd,(struct sockaddr*) &serv_addr, sizeof(struct sockaddr)) < 0 )
		handle_error("bind");
	cout << "-------------Server Start-------------" << endl;
	listen(sockfd,1);
	cout << "Waiting clients..." << endl;
	for(;;){
		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &socksize);
		cout << "New client connected. " << endl;
		if(newsockfd <0)
			cout << "error accept!!";
		if((childpid = fork())<0)
			cout << "error fork child!!";
		else if (childpid == 0){
			close(sockfd);
			str_echo(newsockfd);
			exit(0);
		}
		close(newsockfd);
	}
return 0; 
}
void  parse(char *line, char **argv)
{
     while (*line != '\0') {       /* if not the end of line ....... */ 
          while (*line == ' ' || *line == '\t' || *line == '\n')
               *line++ = '\0';     /* replace white spaces with 0    */
          *argv++ = line;          /* save the argument position     */
          while (*line != '\0' && *line != ' ' && 
                 *line != '\t' && *line != '\n') 
               line++;             /* skip the argument until ...    */
     }
     *argv = '\0';                 /* mark the end of argument list  */
}

 
//send string to client
void display(int sockfd,string s){
	char *a=new char[s.size()+1];
	a[s.size()]=0;
	memcpy(a,s.c_str(),s.size());	
	send(sockfd,a,strlen(a),0);
}

bool isint(string s){
 for(int i=0;i<s.size();i++){
                if(!isdigit(s[i])) return false;
        }

	return true;
}

int gpipein(){
	int i=0;
	for(i=0; i < pipe_limit ; i++){
        	if((bigpipe[i][0]!=0) && (bigpipe[i][2]==0)){
                	break;
		}
	}
        return i;
}

int gpipeget(){
	int i=0;
        for(i=0; i < pipe_limit ; i++){
                if(bigpipe[i][0] == 0)
                        break;
        }
        return i;
}


int gpipefind(int n){
	int i=0;
	for(i=0; i < pipe_limit ; i++){
		if(bigpipe[i][2] == n)
			break;	
	}
	return i;
}
void gpipedec(){
	for(int i=0 ; i < pipe_limit ; i++){
                if(bigpipe[i][0]!=0)
                        bigpipe[i][2]--;
        }
}

bool gpipepro(int localfd){
	int i=0;
	for(i=0 ; i < pipe_limit ; i++){
                if(bigpipe[i][0]==localfd)
                        return false;
        }
	return true;	
}


void exe(int sockfd, string p){
	int cpid,i=0;
	char *a=new char[p.size()+1];
        a[p.size()]=0;
        memcpy(a,p.c_str(),p.size());
	char *arg[20];
	parse(a,arg);
	gpipedec();
	
	if ((cpid=fork()) <0){
		handle_error("fork");
	}	
	else if (cpid == 0){ /*CHILD*/
		i=gpipein();
	        if( i != pipe_limit){
        		cout << p << " Get input!!!!!!" << bigpipe[i][0] << endl;
			if(dup2(bigpipe[i][0], 0) < 0){
        	        	handle_error("Gpipe read dup error");
                	}
			close(bigpipe[i][0]);
	        close(bigpipe[i][1]);
	        }
		
		dup2(sockfd,1);
		dup2(sockfd,2);
		if(execvp(*arg,arg)<0)
			perror("exec");
		//cout << wd+p.substr(0,p.find(" ")) << ":" << p << " executed\n";
	}else{
		i=gpipein();
		if( i != pipe_limit ){
                	cout << "close global pipe no." << i << endl;
	                close(bigpipe[i][0]);
			cout << "close:" << bigpipe[i][0] << endl;
			close(bigpipe[i][1]);
			cout << "close:" << bigpipe[i][1] << endl;
			bigpipe[i][0]=0;
	                bigpipe[i][1]=0;
	                bigpipe[i][2]=0;
        		cout << "close done" << endl;
	     	}
	}
	cout << "ya" << endl;
	waitpid(cpid,NULL,0);
	cout << "done"<< endl;
}

string strLTrim(const string& str)
 {
         return str.substr(str.find_first_not_of(" \n\r\t"));
 }
 
 string strRTrim(const string& str)
 {
         return str.substr(0,str.find_last_not_of(" \n\r\t")+1);
 }

string trim(const string& str)
{
	return strLTrim(strRTrim(str));
}
bool cmdexst(string s){
	string w = getenv("PATH");
	string token="";
	int pos=0;
	cout << "get: "+s << endl;
	while ((pos = w.find(":")) != std::string::npos) {
		token = trim(w.substr(0,pos))+"/"+s ;
        	cout << "token= " << token << endl;
		if( access( token.c_str(), F_OK ) != -1 ) {
        	   	return true;
	        } 
	w.erase(0, pos + 1);
	}
	token = trim(w)+"/"+s;
	cout << "token= " << token << endl;
	if( access( token.c_str(), F_OK ) != -1 ) {
        	    return true;
        }
	return false;
}
string penv(string s){
	cout << s.substr(s.find(" ")+1) << endl;
	string k=getenv(s.substr(s.find(" ")+1).c_str());
	return k+"\n";
}

void senv(string s){
	char *value[512];
	char *a=new char[s.size()+1];
        a[s.size()]=0;
        memcpy(a,s.c_str(),s.size());
	parse(a,value);
	setenv(value[1],value[2],1);
	
}

void errstilcounts(){
	int i=0;
	gpipedec();
	i=gpipein();
	if( i != pipe_limit ){
		close(bigpipe[i][0]);
                close(bigpipe[i][1]);
                bigpipe[i][0]=0;
                bigpipe[i][1]=0;
                bigpipe[i][2]=0;	
	}
}
void runpipe(string cmds, int pipecount, int sockfd,bool err,int jp){
	int i = 0, j = 0, gpi=0;
	int pipefds[pipecount][2];
	string delimiter="|";
 	string cmd;	
	pid_t pid;
	size_t pos=0;
	
	cout << "get in pipes :" << cmds <<",jp="<<jp<< endl;
	gpipedec();
	cout << "Gpipe has: ";
	
	for(i=0;i<1000;i++){
		if (bigpipe[i][0] != 0)
			cout << i << ", ";	
	}
	cout << endl;
	
	for(i = 0; i <pipecount; i++){
       	if( pipe(pipefds[i]) < 0 ) {
	       	handle_error("pipe");
		}
    }
	
	for(j=0;j<=pipecount;j++){
		pos = cmds.find(delimiter);
		if (pos != std::string::npos)
			cmd = trim(cmds.substr(0, pos));
		else
			cmd = trim(cmds);
		
		char *a=new char[cmd.size()+1];
	    a[cmd.size()]=0;
		memcpy(a,cmd.c_str(),cmd.size());
		char *arg[20];
		parse(a,arg);
	
		if(isint(cmd)){
			gpi = gpipefind(atoi(cmd.c_str()));
			if( gpi != pipe_limit){
                               	cout << "ifwrite to global pipe index = "<< gpi << endl;
			}else if(gpi == pipe_limit){
				gpi=gpipeget();
                if( gpi != pipe_limit){
					cout << "new jump write to global " << gpi << " pipe "<<endl;
					bigpipe[gpi][0]=pipefds[j-1][0];
	                bigpipe[gpi][1]=pipefds[j-1][1];
	                bigpipe[gpi][2]=atoi(cmd.c_str());
				}else{
					display(sockfd,"pipe exceed limit");	
				}
			}
		}else{
			if((pid=fork())<0)
				handle_error("fork");
			if(pid==0){
				cout << "now processing:\"" << cmd << "\"" << endl;
				i=gpipein();
				if((i != pipe_limit) && (j==0)){
					cout << cmd << " Get input from global readfd:" << bigpipe[i][0] << endl;
					if(dup2(bigpipe[i][0], 0) < 0){
						handle_error("Gpipe read dup error");
					}
					cout << "close" << i <<endl;
					close(bigpipe[i][0]);
					close(bigpipe[i][1]);
				}
				
				if( j!=0 ){
					cout << cmd << "dupin" << endl;
					if( dup2(pipefds[j-1][0], 0) < 0){
						handle_error("dupin");///j-2 0 j+1 1
					}
				
				}	
				 
				if( j != pipecount){ //not the last cmd
					if((jp!=0) && (j==pipecount-1)){
						gpi = gpipefind(jp);
						if( gpi != pipe_limit){
							cout << "find exist global pipe" << gpi << endl;
								if(dup2(bigpipe[gpi][1],1) < 0){
									handle_error("2found same N ,Gpipe write dup error!");
								}
							if(err){
								if(dup2(bigpipe[gpi][1],2) < 0){
									handle_error("2found same N ,Gpipe write dup error!");
								}
							}
						}else {
							cout << cmd << "dup out to pipe"+j << endl;
							if( dup2(pipefds[j][1], 1) < 0){
								handle_error("dup out");
							}
							if(err){
								if( dup2(pipefds[j][1], 2) < 0){
									handle_error("dup err");
								}
							}
						}
					}else {
							cout << cmd << "dup out to pipe"+j << endl;
							if( dup2(pipefds[j][1], 1) < 0){
								handle_error("dup out");
							}
							if(err){
								if( dup2(pipefds[j][1], 2) < 0){
									handle_error("dup err");
								}
							}
						}
				}else{ //if the last command ,to socket output
					cout << cmd << "is last, to sockfd"<< endl;
					close(1);
					if(dup(sockfd)<0)	handle_error("dup last cmd to sockfd");
					close(2);
					if(dup(sockfd)<0)	handle_error("dup last cmd err to sockfd");
				}
					
				for(i = 0; i < pipecount; i++){
					close(pipefds[i][0]);
					close(pipefds[i][1]);
				}
				if(execvp(*arg,arg)<0)
						handle_error("exec");
			}else{//parent	
				i=gpipein();
				if( i != pipe_limit ){
					cout << "close" << i <<endl;
					close(bigpipe[i][0]);
					close(bigpipe[i][1]);	
					bigpipe[i][0]=0;
					bigpipe[i][1]=0;
					bigpipe[i][2]=0;
				}
				if( j == pipecount ){
					cmd.clear();
				}else
					cmds.erase(0, pos + delimiter.length());
			}
		}
	}// end for
			for(i = 0; i<pipecount; i++){
				if(gpipepro(pipefds[i][0])){
					close(pipefds[i][0]);
					close(pipefds[i][1]);
				}else
					cout << "didnt close " << i << "fd read=" << pipefds[i][0] << ", write=" << pipefds[i][1] << endl;
			}
			waitpid(pid,0,0);
}

int writen(int sockfd,char *line,int n){
	int pipecount=0,rdcount=0,excount=0,jp=0;
	int file,bck=sockfd;
	bool err=false;
	string s(line);
	string w(wd);
	string t=trim(s);
	string fname = w+t;
        string delimiter = "|";

	size_t pos = 0;
	string token;
	
	if(t=="exit"){
		display(sockfd,"Goodbye!\n");
		close(sockfd);
		cout << "Client Disconnected." << endl;
		exit(0);
        }
	for(int i=0;i<s.size();i++){
		if(s[i] == '|') pipecount++;
		if(s[i] == '>') rdcount++;
		if(s[i] == '%') {
			ig=true;
			string iglines=trim(t.substr(s.find('%')+1));
			igc=atoi(iglines.c_str());
			cout << igc << endl;
		}
		if(s[i] == '!') {
			pipecount++;
			err=true;
			s[i]='|';
			t=trim(s);
		}
	}
	if(ig==true){
		sockfd = 0;
	}
	if(igc!=0){
		igc--;
	}else
		ig=false;

	cout << "Get :"+t << endl;
	if(rdcount!=0){
	 	string filepath=trim(t.substr(s.find('>')+1));
		file = open(filepath.c_str(),O_WRONLY | O_CREAT,0666);
		t = t.substr(0,t.find('>')-1);
		sockfd = file;
	}
	if (pipecount>1000){
		display(sockfd,"The amount of pipe each line must <=1000!");
	}	
	if(pipecount>0){
		while ((pos = t.find(delimiter)) != std::string::npos) {
		    token = trim(t.substr(0, pos));
			if(!cmdexst(token.substr(0,token.find(" ")))){
				display(sockfd,"Unknown command: [" + token.substr(0,token.find(" ")) + "].\n% ");
				errstilcounts();
				return n;
			}
		    std::cout << token << std::endl;
		    t.erase(0, pos + delimiter.length());
		}
		t=trim(t);
                if(isint(t))//pipe to future lines
		{
		  cout << "call " << t << " pipes!\n";
		  jp=atoi(t.c_str());
		}else{
			if(!cmdexst(t)){
                	        display(sockfd,"Unknown command: [" + t + "].\n% ");
				errstilcounts();
				return n;
			}
				//*****TODO exec command
		}
		if(rdcount!=0){
			t=s.substr(0,s.size()-2);
			t=t.substr(0,t.find('>')-1);
			runpipe(t,pipecount,sockfd,err,jp);
		}else
			runpipe(trim(s),pipecount,sockfd,err,jp);
	}else{
		
		if(t.substr(0,t.find(" "))=="printenv")
			display(sockfd,penv(trim(t)));
		else if (t.substr(0,t.find(" "))=="setenv")
			senv(trim(t));
		else if(!cmdexst(t.substr(0,t.find(" ")))){
			display(sockfd,"Unknown command: [" + t.substr(0,t.find(" ")) + "].\n");
			errstilcounts();
		}else
			exe(sockfd,t);		
                }
	
	//cout << t << "size=" << t.size() << endl << "has: " << pipecount << endl;
	sockfd=bck;
	display(sockfd,"% ");
	return (n);
}

#define MAXLINE 10000 
void str_echo(int sockfd){
	int n;
	char line[MAXLINE];
	display(sockfd,"\33c");
	display(sockfd,welcomemsg);	
	for(;;){
		n=readline(sockfd,line,MAXLINE);
		if(n==0) return;
		else if(n<0) cout << "readline ERROR!!";
		if(writen(sockfd,line,n) !=n)
			cout << "error writen!";
	}

} 

int readline(int fd, char * ptr, int maxlen) 
{ 
	int n, rc; char c; 
 	for (n = 1; n < maxlen; n++) { 
 		if ( (rc = read(fd, &c, 1)) == 1) { 
			*ptr++ = c; 
			if (c == '\n') break; 
		}else if (rc == 0) { 
			if (n == 1) return(0); /* EOF, no data read */ 
 			else break; /* EOF, some data was read */ 
 		} else 
			return(-1); /* error */ 
 	} 
 	*ptr = 0; 
 	return(n); 
}

 
