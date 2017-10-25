#include <iostream>
#include <sstream>
#include <vector>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdlib>
#include <errno.h>
#include <string.h>
#include <map>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>

using namespace std;
#define MAXERR 100
#define MAXLINE 15000
#define MAXCONN 1000

void remove_pipe(map<int, pair<int,int> >& pipe_table,int pos){
    
    map<int, pair<int,int> >::iterator it;
    if((it = pipe_table.find(pos)) != pipe_table.end()){
        close(pipe_table[pos].first);
        close(pipe_table[pos].second);
        pipe_table.erase(it);
    }
    return;
}
inline void endofpipe(int& back_step, int& step, int& number_pipe){ back_step = 0,step = 0,number_pipe = 1; }

int TcpListen(struct sockaddr_in* servaddr,socklen_t servlen,int port){
    int listenfd,reuse = 1;
    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) cout << "socket error" << endl;
    
    bzero(servaddr, servlen);
    servaddr->sin_family = AF_INET;
    servaddr->sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr->sin_port = htons(port);
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    
    if(bind(listenfd, (struct sockaddr*) servaddr, servlen) < 0) cout << "bind error" << endl;
    if(listen(listenfd, MAXCONN) < 0) cout << "listen error" << endl;/*server listen port*/
    return listenfd;
}

void readline(int sockfd,char* buf){
    int n,len = 0;
    while((n = read(sockfd, &buf[len], 1)) > 0) {
        if(buf[len] !='\n') ++len;
        else {
            buf[len++] = 0;
            break;
        }
        if(len == MAXLINE) len = 0;
    }
}

int redirect(int oldfd,int newfd) {
    int backfd = dup(oldfd);
    dup2(newfd,oldfd);
    return backfd;
}

void restore(int oldfd,int backfd) {
    dup2(backfd,oldfd);
    close(backfd);
}

int main(int argc, char* argv[], char* envp[]){
    
    struct sockaddr_in cli_addr, serv_addr;
    int listenfd = TcpListen(&serv_addr, sizeof(serv_addr), atoi(argv[1]));
    socklen_t clilen = sizeof(cli_addr);
    
    while(true) {
        
serv_next:
        int step = 0, next, first;
        map<int,pair<int,int> > pipe_table;
        map<int,bool> notremove;
        enum state{ PIPE , END , FILE };
	  
        int connfd = accept(listenfd, (struct sockaddr *) &cli_addr, &clilen);
        write(connfd,"****************************************\n** Welcome to the information server. **\n****************************************\n",123);
        setenv("PATH","bin:.", 1);
        chdir("ras");
       
        while(true) {
                      
            write(connfd,"% ",2);
            char buf[MAXLINE]={0},response[MAXLINE] = {0};
            readline(connfd, buf);
            string input(buf),tok;
            first = 1;
            istringstream line(input);
            
            while(line >> tok){
                step++;
                vector<string> Arglist;
                state s = END;
                
                do {
                    if(tok[0] == '|') {
                        if(tok.size() != 1) {
                            next = atoi(tok.substr(1,tok.size()-1).c_str());
                            notremove[step+next] = 1;
                        }
                        else next = 1;
                        
                        s = PIPE;
                        break; ///
                    }
                    if(tok[0] != '>') {
                        if(s == FILE) {
                            break;
                        }
                        Arglist.push_back(tok);
                    }
                    else s = FILE;
                }
                while(line >> tok); ///
                
                if(Arglist.empty()) continue;
                if(Arglist[0] == "exit") {
                    close(connfd);
        	    goto serv_next;            
                }
                
                const char** arglist = new const char* [Arglist.size()+1];
                int i;
                for(i = 0 ; i < Arglist.size(); i++) {
                    arglist[i] = Arglist[i].c_str();
                }
                arglist[i] = NULL; //
                //read arg//
                int file_fd,data_fd[2];
                char data_buf[MAXLINE] = {0};
               	  
                if(Arglist[0] == "setenv") {
                    if(arglist[2] != NULL) setenv(arglist[1],arglist[2], 1);
                    break;
                }
                 
                if(Arglist[0] != "printenv") {
                    string syspath(getenv("PATH"));
                    int found_pos = 0,found = 0,test_fd = -1;
                    
                    while((found_pos = syspath.find(":")) != string::npos) {
                        if((test_fd = open((syspath.substr(0,found_pos)+"/"+Arglist[0]).c_str(),O_RDONLY)) > 0) {
                            found = 1;
                            break;
                        }
                        syspath = syspath.substr(found_pos+1);
                    }
                    if(!found && found_pos == string::npos) {
                        if((test_fd = open((syspath+"/"+Arglist[0]).c_str(),O_RDONLY)) > 0) {
                            found = 1;
                        }
                    }
                    if(!found) {
                        string str("Unknown command: [" + Arglist[0] + "].\n");
                        memcpy(response, str.c_str(), str.size());
                        write(connfd, response, str.size());
                        if(notremove.find(step) == notremove.end()) {
                            remove_pipe(pipe_table,step);
                        }
                        if(!first) step--;
                        if(s == FILE) close(file_fd);
                    }
                    first = 0;
                    if(!found) break;
                    
                    close(test_fd);
                }
                
                if(s == PIPE){
                    
                    if(pipe_table.find(step+next) == pipe_table.end()) {
                        pipe(data_fd);
                        pipe_table[step+next] = pair<int,int>(data_fd[0],data_fd[1]);
                    }
                    else {
                        data_fd[0] = pipe_table[step+next].first;
                        data_fd[1] = pipe_table[step+next].second;
                    }
                    
                }
                else if(s == FILE) {
                    file_fd = open(tok.c_str(), O_RDWR | O_CREAT, 0666);
                    if(file_fd < 0) break;
                }
                else pipe(data_fd);
               
                pid_t pid = fork();
                if(pid == 0) {
                    
                    //cout << step << endl;
                    if(pipe_table.find(step) != pipe_table.end()) {
                        cout << step << endl;
                        dup2(pipe_table[step].first, 0);
                        close(pipe_table[step].second);
                    }
                    
                    if(s == PIPE || s == END) dup2(data_fd[1],1);
                    else if(s == FILE) dup2(file_fd,1);
                    
                    dup2(connfd, 2);
                    
                    if(Arglist[0] == "printenv") {
                        
                        char* val = getenv(arglist[1]);
                        if(arglist[2] == NULL) {
                            string str(Arglist[1] + "=" + val + "\n");
                            memcpy(response, str.c_str(), str.size());
                            if(val != NULL) write(connfd, response, str.size());
                            else ;
                        }
                        else ;
                        exit(0);
                    }
                    
                    if(execvp(arglist[0], (char*const*)arglist) < 0) exit(errno);
                    exit(0); //close all fd
                }
                else {
                    remove_pipe(pipe_table,step);
                    
                    if(s == END) close(data_fd[1]);
                    if(s == FILE) close(file_fd);
                    int n;
                    if(s == END) { //end of pipe
                        while((n = read(data_fd[0],data_buf,MAXLINE)) > 0) {
                            write(connfd, data_buf, n);
                            memset(data_buf,0,sizeof(data_buf));
                        }
                        //endofpipe(back_step,step,number_pipe);
                    }
                    
                    if(s == END) close(data_fd[0]);
                    
                    int status = -1;
                    wait(&status);
                    if(WEXITSTATUS(status) > 0) break;
                    cout << "The exit code of " << Arglist[0] << " is " << WEXITSTATUS(status) << endl;
                }
            }
        }
    }
}
