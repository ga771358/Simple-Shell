#include<iostream>
#include<fstream>
#include <sstream>
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
#include<utility>
using namespace std;
#define MAXERR 100
#define MAXLINE 150


int main(int argc, char* argv[], char* envp[]){

	string message,command,line,tok;

	cout << "****************************************\n** Welcome to the information server. **\n****************************************" << endl;
	
	setenv("PATH","bin:.", 1);

	int step = 0,next;
	map<int,pair<int,int> > pipe_table;

	char data_buf[MAXLINE] = {0};
	while(true) {
		cout << "% ";

		getline(cin, command);
			
		istringstream line(command);
		while(line >> tok){
			step++;
			vector<string > Arglist;
			enum state{ FILE,PIPE,END};
			state s = END;

			char filename[20] = {0};

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

			const char** arglist = new const char* [Arglist.size()+1];
			int i;
			for(i = 0 ; i < Arglist.size(); i++) {
				arglist[i] = Arglist[i].c_str();
			}
			arglist[i] = NULL; //
			//read arg//

			if(Arglist[0] == "printenv") {
				cout << Arglist[1] << "=" << getenv(arglist[1]) << endl;
				break;
			}
			if(Arglist[0] == "setenv") {
				setenv(arglist[1],arglist[2],1);
				break;
			}

			int file_fd,err_fd[2],data_fd[2];

			char err_buf[MAXERR] = {0}; ///?
			if(s == PIPE){
				
				if(pipe_table.find(step+next) == pipe_table.end()) {
					pipe(data_fd);
					pipe_table[step+next] = make_pair(data_fd[0],data_fd[1]);
				}
				else {
					//cout << Arglist[0] << step+next << endl;
					data_fd[0] = pipe_table[step+next].first;
					data_fd[1] = pipe_table[step+next].second;
				}
					
			}
			else if(s == FILE) {
				file_fd = open(tok.c_str(), O_RDWR | O_CREAT, "0666");
				if(file_fd < 0) {
					cout << "open error" << endl;
					break;
				}
				
			}
			else {
				//pipe(data_fd);
			}


			pid_t pid = fork();
			if(pid < 0) {
				cout << "fork error" << endl;
				break;
			}
			else if(pid == 0) {
				if(pipe_table.find(step)!=pipe_table.end()) {
					dup2(pipe_table[step].first,0);
					close(pipe_table[step].second);
					//pipe_table.erase(pipe_table.find(step));
				}
				if(s == PIPE) {
					close(data_fd[0]);
					dup2(data_fd[1],1);
					close(data_fd[1]);
				}
				else if(s == FILE){
					dup2(file_fd,1);
					close(file_fd);
				}
				
				//exec
				string path("bin/");
				int file;
				if((file = open((path+Arglist[0]).c_str(),O_RDONLY)) < 0) {
					cout << "Unknown Command [" << Arglist[0] << "]" << endl;
					exit(errno);
				}
				else {
					close(file);
					if(execvp(arglist[0], (char*const*)arglist) < 0) { //
						exit(errno);
					}
				}

				exit(0); //close all fd
			}
			else {

				if(s == FILE) close(file_fd);
				int status = -1;
	    		wait(&status);
	    		
	        	
        		if(pipe_table.find(step) != pipe_table.end()){
					close(pipe_table[step].first);
					close(pipe_table[step].second);
					pipe_table.erase(pipe_table.find(step));
				}
        	
	    		
	    		//cout << "The exit code of " << Arglist[0] << " is " << WEXITSTATUS(status) << endl;
			}

		}
		
	}
}