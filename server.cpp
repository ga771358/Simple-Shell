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
using namespace std;
#define MAXERR 100
#define MAXLIEN 15000


int main(int argc, char* argv[], char* envp[]){

	string message,command,line,tok;

	cout << "****************************************\n** Welcome to the information server. **\n****************************************" << endl;
	
	setenv("PATH","bin:.", 1);

	while(true) {
		cout << "% ";
		getline(cin, command);
		
		istringstream line(command);
		while(line >> tok){
			vector<string > Arglist;
			enum state{ FILE, NOT_FILE };
			state s = NOT_FILE;
			char filename[20]={0};

			do {
				if(tok[0] == '|') break; ///
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

			int file_fd,err_fd[2];

			char err_buf[MAXERR] = {0};
			if(s == NOT_FILE){
				pipe(err_fd);
			}
			else {
				pipe(err_fd);
				file_fd = open(tok.c_str(), O_RDWR | O_CREAT, "0666");
				if(file_fd < 0) {
					cout << "open error" << endl;
					break;
				}
				
			}

			pid_t pid = fork();
			if(pid < 0) {
				cout << "fork error" << endl;
			}
			else if(pid == 0) {
			
				close(err_fd[0]);
				dup2(err_fd[1],2);
				close(err_fd[1]);
				
				if(s == FILE) {
					dup2(file_fd,1);
					close(file_fd);
				}
				
				//exec
				if(execvp(arglist[0], (char*const*)arglist) < 0) { //
					exit(errno);
				}
			}
			else {
				close(file_fd);
				close(err_fd[1]);
				int status = -1;
	    		wait(&status);
	    		if(WEXITSTATUS(status) > 0) {
	        		if(read(err_fd[0],err_buf,MAXERR) > 0) {
	        			cout << err_buf;
	        			break;
	        		}
	        		else {
	        			cout << "Unknown Command: [" << Arglist[0] << "]" << endl;
	        			break;
	        		}
	    		}
	    		close(err_fd[0]);
	    		//cout << "The exit code of " << Arglist[0] << " is " << WEXITSTATUS(status) << endl;
			}
		}
		
	}
}