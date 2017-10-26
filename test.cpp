#include <iostream>
#include <map>
#include <unistd.h>
using namespace std;

class MyMap: public map<int,pair<int,int > >{
public:
	~MyMap(){
		map<int, pair<int,int> >::iterator it = begin();
	    if(it != end()){
	    	cout << "close " << it->second.first << endl;
	        cout << "close " << it->second.second << endl;
	        erase(it);
	        ++it;
	    }
	};
	
};

int main(){

	MyMap table;
	table[0] = pair<int,int>(1,2);

}