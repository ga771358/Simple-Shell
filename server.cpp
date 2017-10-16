#include<iostream>
#include<fstream>
#include <sstream>
#include<vector>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdlib>
using namespace std;


int findenv(const string& input,const vector<string >& env){
	for(vector<string>::const_iterator str = env.begin(); str != env.end(); str++){
		string::size_type found = str->find("=");
		if (found != string::npos) {
			string var(str->substr(0,found));
			if(input == var) return str - env.begin();
		}
	}
	return -1;
}
int main(int argc, char* argv[], char* envp[]){

	ifstream ifs ( "welcome.txt" , ifstream::in );
	string hello_m,command,tok;
	vector<string> env;
	for(int i = 0; envp[i] != 0; i++) {
		env.push_back(envp[i]);
	}

	while(getline(ifs, hello_m)) {
		cout << hello_m << endl;
	}

	
	int path = findenv("PATH",env);
	while(true) {
		cout << "% ";
		getline(cin, command);
		
		istringstream CMD(command);
		CMD >> tok;
		if(tok == "printenv") {
			if(CMD >> tok){
				int pos = findenv(tok,env);
				if(pos != -1) cout << env[pos] << endl;
			}		
		}
		else if(tok == "setenv"){
			if(CMD >> tok){
				int pos = findenv(tok,env);
				if(pos != -1) {
					string var(tok);
					if(CMD >> tok) env[pos] = var + "=" + tok;
				}
			}
		}
		else {

			/*FORK ONE PROCESS*/
			pid_t pid = fork();
			
			vector<string> Arglist;
			Arglist.push_back(tok);
			while(CMD >> tok) {
				Arglist.push_back(tok);
			}
			const char** arglist = new const char* [Arglist.size()+1]; //
			int i;
			for(i = 0 ; i < Arglist.size(); i++) {
				arglist[i] = Arglist[i].c_str();
			}
			arglist[i] = NULL; //
			if(pid < 0) {

			}
			else if(pid == 0) {
				
				//exec
				string::size_type arg = env[path].find("=");
				if(execv((env[path].substr(arg+1)+"/"+Arglist[0]).c_str(), (char*const*)arglist) < 0) { //
					exit(1);
				}
				exit(0);
			}
			else {
				int status = -1;
        		wait(&status);
        		cout << "The exit code of " << Arglist[0] << " is " << WEXITSTATUS(status) << endl;
        		
			}
		}
	}
}