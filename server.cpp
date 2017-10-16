#include<iostream>
#include<fstream>
#include <sstream>
#include<vector>
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

	

	while(true) {
		cout << "% ";
		getline(cin, command);
		istringstream CMD(command);
		CMD >> tok;
		if(tok == "printenv") {
			CMD >> tok;
			int pos = findenv(tok,env);
			if(pos != -1) cout << env[pos] << endl;		
		}
		else if(tok == "setenv"){
			CMD >> tok;
			int pos = findenv(tok,env);
			if(pos != -1) {
				string var(tok);
				CMD >> tok;
				env[pos] = var + "=" + tok;
			}
		}
	}
}