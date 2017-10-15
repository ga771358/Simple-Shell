#include<iostream>
#include<fstream>
using namespace std;

int main(){

	ifstream ifs ( "welcome.txt" , ifstream::in );
	string hello_m,command;

	while(getline(ifs, hello_m)) {
		cout << hello_m << endl;
	}

	while(true) {
		cout << "% ";
		getline(cin, command);
	}
}