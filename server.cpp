#include<iostream>
#include<sstream>
#include<vector>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdlib>
#include <errno.h>
#include<string.h>
#include <map>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;
#define MAXERR 100
#define MAXLINE 1000

int main(int argc, char* argv[], char* envp[]){

	string input,tok;
	cout << "****************************************\n** Welcome to the information server. **\n****************************************" << endl;	
	setenv("PATH","bin:.", 1);

	int step = 0, next, back_step = 0,first;
	map<int,pair<int,int> > pipe_table;
	chdir("ras");
	while(true) {
		cout << "% ";
		getline(cin, input);
		first = 1;
		istringstream line(input);
		while(line >> tok){
			step++;
			vector<string > Arglist;
			enum state{ FILE , PIPE , END };
			char err_buf[MAXERR] = {0},data_buf[MAXLINE] = {0};
			state s = END;

			do {
				if(tok[0] == '|') {
					if(tok.size() != 1) next = atoi(tok.substr(1,tok.size()-1).c_str());
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
				else {
					s = FILE;
				}	
			}
			while(line >> tok); ///

			if(Arglist[0] == "exit") return 0;

			const char** arglist = new const char* [Arglist.size()+1];
			int i;
			for(i = 0 ; i < Arglist.size(); i++) {
				arglist[i] = Arglist[i].c_str();
			}
			arglist[i] = NULL; //
			//read arg//

			int file_fd,err_fd[2],data_fd[2],tag_fd[2];
			pipe(err_fd);
			pipe(tag_fd);

			if(s == PIPE){
				
				if(pipe_table.find(step+next) == pipe_table.end()) {
					pipe(data_fd);
					pipe_table[step+next] = pair<int,int>(data_fd[0],data_fd[1]);
				}
				else {
					//cout << Arglist[0] << step+next << endl;
					data_fd[0] = pipe_table[step+next].first;
					data_fd[1] = pipe_table[step+next].second;
				}
					
			}
			else if(s == FILE) {
				file_fd = open(tok.c_str(), O_RDWR | O_CREAT, 0666);
				if(file_fd < 0) {
					cout << "open error" << endl;
					break;
				}
				
			}
			else {
				pipe(data_fd);
			}

			if(Arglist[0] == "setenv") {
				if(arglist[2] != NULL) setenv(arglist[1],arglist[2],1);
				break;
			}
			
			if(Arglist[0] != "printenv") {
				string syspath(getenv("PATH"));
				int found_pos = 0,found = 0,test_fd;
				
				while((found_pos = syspath.find(":")) != string::npos) {
					if((test_fd = open((syspath.substr(0,found_pos)+"/"+Arglist[0]).c_str(),O_RDONLY)) > 0) {
						found = 1;
						break;
					}
					syspath = syspath.substr(found_pos+1);
				}
				if(found_pos == string::npos) {
					if((test_fd = open((syspath+"/"+Arglist[0]).c_str(),O_RDONLY)) > 0) {
						found = 1;
					}
				}
				if(!found) {
					cout << "Unknown Command: [" << Arglist[0] << "]." << endl;
					if(!first) back_step++;
				}
				first = 0;
				if(!found) break;
				close(test_fd);
			}
			pid_t pid = fork();
			if(pid < 0) {
				cout << "fork error" << endl;
				break;
			}
			else if(pid == 0) {

				if(pipe_table.find(step - back_step) != pipe_table.end()) {
					dup2(pipe_table[step - back_step].first, 0);
					close(pipe_table[step - back_step].second);
					//cout << Arglist[0] << endl;
					//pipe_table.erase(pipe_table.find(step));
				}
				
				if(s == PIPE) {
					//close(data_fd[0]);
					dup2(data_fd[1],1);
					//close(data_fd[1]);
				}
				else if(s == FILE){
					dup2(file_fd,1);
					close(file_fd);
				}
				else {
					dup2(data_fd[1],1);
				}
				dup2(err_fd[1],2);
				//close(2);
				if(Arglist[0] == "printenv") {

						char* val = getenv(arglist[1]);
						if(arglist[2] == NULL) {
							if(val != NULL) cout << Arglist[1] << "=" << val << endl;
							else cout << "No such env" << endl;
						}
						else cout << "Wrong argument number for printenv" << endl;
						exit(0);
				}
				//exec

				if(execvp(arglist[0], (char*const*)arglist) < 0) { //
					exit(errno);
				}

				exit(0); //close all fd
			}
			else {
				map<int, pair<int,int> >::iterator it;
				if((it = pipe_table.find(step - back_step)) != pipe_table.end()){
					close(pipe_table[step - back_step].first);
					close(pipe_table[step - back_step].second);
					pipe_table.erase(it);
				}
				int backfd;
				close(err_fd[1]);
				close(tag_fd[1]);
				if(s == END) 
					close(data_fd[1]);
				else {
					//backfd = dup(1);
					//dup2(data_fd[1],1);
				}

				if(read(err_fd[0],err_buf,MAXERR) > 0) {
					cout << err_buf;
				}
				
					
					if(s == END) {
						while(read(data_fd[0],data_buf,MAXLINE) > 0) {
							cout << data_buf;
							memset(data_buf,sizeof(data_buf),0);
						}
						back_step = 0;
						step = 0;
					}
				
				close(err_fd[0]);
				close(tag_fd[0]);
				if(s == FILE) close(file_fd);
				else if(s == END) {
					close(data_fd[0]);
				}
				else {
					//dup2(backfd,1);
					//close(backfd);
				}	
				int status = -1;
	    		wait(&status);
	    		//cout << Arglist[0] << " " << step << endl;
	    		if(WEXITSTATUS(status) > 0) break;
	    		//cout << "The exit code of " << Arglist[0] << " is " << WEXITSTATUS(status) << endl;
			}
		}
	}
}