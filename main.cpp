#include "ResourcesParser/ResourceTypes.h"
#include "ResourcesParser/ResourcesParser.h"
#include "ResourcesParser/ResourcesParserInterpreter.h"

#include <iostream>
#include <sstream>

using namespace std;

int findArgvIndex(const char* argv, char *argvs[], int count);
const char* getArgv(const char* argv, char *argvs[], int count);
void printHelp();

int main(int argc, char *argv[]) {
	const char* path = getArgv("-p", argv, argc);

	if(nullptr == path) {
		printHelp();
		return -1;
	} else if(getArgv("-h", argv, argc) != nullptr) {
		printHelp();
	}

	ResourcesParser parser(path);
    ResourcesParserInterpreter interpreter(&parser);
/*
	if(all >= 0) {
		interpreter.parserResource(ResourcesParserInterpreter::ALL_TYPE);
	}

	if(type) {
		interpreter.parserResource(type);
	}

	if(id) {
		interpreter.parserId(id);
	}
*/	

    //res/xml/network_security_config.xml
    
    uint32_t newResKeyId = parser.addResKeyStr("", "xml", "network_security_config");
    cout<<"[newResId]:0x" << hex << newResKeyId << dec << endl;

    parser.saveToFile("out.arsc");
    //rebuild_arscfile(parser);

	return 0;
}

int findArgvIndex(const char* argv, char *argvs[], int count) {
	for(int i = 0 ; i<count ; i++) {
		if(strcmp(argv, argvs[i])==0) {
			return i;
		}
	}
	return -1;
}

const char* getArgv(const char* argv, char *argvs[], int count) {
	int index = findArgvIndex(argv, argvs, count);
	if(index>=0 && index+1 < count) {
		return argvs[index+1];
	}
	return nullptr;
}

void printHelp() {
	cout <<"rp -p path [-a] [-t type] [-i id]" <<endl<<endl;
	cout <<"-p : set path of resources.arsc" <<endl;
	cout <<"-a : show all of resources.arsc" <<endl;
	cout <<"-t : select the type in resources.arsc to show" <<endl;
	cout <<"-i : select the id of resource to show" <<endl;
}
