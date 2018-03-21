/*
 * dump_dict.cpp
 *
 *  Created on: 08/03/2018
 *      Author: axel
 */

#include <HDT.hpp>
#include "../src/hdt/BasicHDT.hpp"
#include "../src/dictionary/LiteralDictionary.hpp"
#include <HDTManager.hpp>
#include <signal.h>

#include <getopt.h>
#include <string>
#include <string.h>
#include <iostream>
#include <fstream>

#include "../src/util/StopWatch.hpp"

using namespace hdt;
using namespace std;

int interruptSignal = 0;

void signalHandler(int sig) {
	interruptSignal = 1;
}

void help() {
	cout << "$ dump_dict <HDT file>" << endl;
	cout << "\t-h\t\t\t\tThis help" << endl;
	cout << "\t-p\t\t\t\tPrint all distinct non-qname prefixes (TODO - not yet implemented)" << endl;
	cout << "\t-q\t\t\t\tPrint all qnames per prefixes (TODO - not yet implemented)." << endl;
	cout << "\t-r\t<rol>\t\t\tRol (s|p|o|h|a), where a=all, h=shared." << endl;

	//cout << "\t-v\tVerbose output" << endl;
}

int main(int argc, char **argv) {
	int c;
	string query, inputFile, rolUser="a";
	bool pref=false;
	bool qnames=false;


	while ((c = getopt(argc, argv, "hpqr:")) != -1) {
		switch (c) {
		case 'h':
		  help();
		  break;
		case 'p':
		  pref = true;
		  break;
		case 'q':
		  qnames = true;
		  break;
		case 'r':
		  rolUser = optarg;
		  break;		
		default:
		  cout << "ERROR: Unknown option:" << c << endl;
		  help();
		  return 1;
		}
	}

	if (argc - optind < 1) {
		cout << "ERROR: You must supply an HDT File" << endl << endl;
		help();
		return 1;
	}

	inputFile = argv[optind];

	try {
		HDT *hdt = HDTManager::mapIndexedHDT(inputFile.c_str());

		Dictionary *dict = hdt->getDictionary();

		IteratorUCharString * itSol;

		switch (rolUser[0]) {
		case 'o':
		  itSol =  new MergeIteratorUCharString(dict->getShared(),dict->getObjects());
		  break;
		case 's':
		  itSol =  new MergeIteratorUCharString(dict->getShared(),dict->getSubjects());
		  break;
		case 'p':
		  itSol = dict->getPredicates();
		  break;
		case 'h':
		  itSol = dict->getShared();
		  break;
		case 'a':
		  itSol = new MergeIteratorUCharString(new MergeIteratorUCharString(new MergeIteratorUCharString(dict->getShared(),dict->getObjects()),dict->getSubjects()),dict->getPredicates());
		  break;
		default:
		  cout << "ERROR: Unknown option" << endl;
		  help();
		  return 1;
		}
		    
		char* previous = 0;
		char* current = 0;
		char literal[] = "LITERAL";
		unsigned long int cnt = 0;
		char *ph; // position of last hash
		char *ps; // position of last slash
		char *pc; // position of last colon
		char *p; // position of namespace separator
		
		while (itSol->hasNext()) {
		 current = reinterpret_cast<char*>(itSol->next());		 
		  if (pref || qnames) {
		    ph=0; // position of last hash
		    ps=0; // position of last slash
		    pc=0; // position of last colon
		    p=0; // position of namespace separator
		    // find Prefix-Qnames separator:

		    // ignore literals or weird identifiers for qname or prefix search.!
		    if( current[0]=='"' ) {
		      current = literal;		      
		    } else {
		      // look for fartherst right '#','/', or ':'
		      ph = strrchr(current, '#');
		      pc = strrchr(current, ':');
		      ps = strrchr(current, '/');
		      if(ph || pc || ps) {
			p = (pc > ph) ? pc : ph; // set p to the max of ph,pc,ps
			p = (ps > p ) ? ps : p;
			if(pref) { // print prefix only
			  //TODO: Is that evil? i.e. does it cause memory troubles to simply overwrite a character in the middle with 0?
			  *(p+1) = 0;
			} else { // print qname only
			  current = (p+1);
			}
		      }
		    }
		  }
		  
		  if( !previous ){
		    previous = current;
		    cnt++;
		  }
	  
		  if ( strcmp(previous,current) ) {
		    cout << previous << " : " << cnt << endl;
		    cnt = 1;
		  } else {
		    cnt++;
		  }
		  previous = current;
		}

		if( previous ) { cout << previous << " : " << cnt << endl; }
		
		delete itSol;
		delete hdt;
		
	} catch (std::exception& e) {
		cerr << "ERROR: " << e.what() << endl;
	}
}
