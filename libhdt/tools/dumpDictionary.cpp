/*
 * dumpDictionary.cpp
 *
 *  Created on: 08/03/2018
 *      Author: Axel Polleres
 */

#include <HDT.hpp>
#include "../src/hdt/BasicHDT.hpp"
#include "../src/dictionary/LiteralDictionary.hpp"
#include <HDTManager.hpp>
#include <signal.h>

#include <getopt.h>
#include <string>
#include <string.h>
#include <strings.h> // strncasecmp is usually in strongs.h, but sometimes in string.h. better include both?
#include <iostream>
#include <fstream>

#include "../src/util/StopWatch.hpp"
#include "../src/util/terms.hpp"

using namespace hdt;
using namespace std;

int interruptSignal = 0;

void signalHandler(int sig) {
	interruptSignal = 1;
}

void help() {
	cout << "$ dumpDictionary <HDT file>" << endl;
	cout << "\t-h\t\t\t\tThis help" << endl;
	cout << "\t-d\t\t\t\tPrint all distinct Pay-level-domains (PLDs occurring in HTTP and HTTPS IRIs" << endl;
	cout << "\t-p\t\t\t\tPrint all distinct non-qname prefixes" << endl;
	cout << "\t-q\t\t\t\tPrint all qnames per prefixes (that is, same qname per different prefix will appear duplicated!)" << endl;
	cout << "\t-r\t<rol>\t\t\tRol (s|p|o|h|a), where a=all, h=shared." << endl;
	cout << "\t-c\t\t\t\talso print counts" << endl;
	// TODO: add a verbose option:
	//cout << "\t-v\tVerbose output" << endl;
}





int main(int argc, char **argv) {
	int c;
	string query, inputFile, rolUser="a";
	bool count=false;
	bool pld=false;
	bool pref=false;
	bool qnames=false;


	while ((c = getopt(argc, argv, "hcdpqr:")) != -1) {
		switch (c) {
		case 'h':
		  help();
		  break;
		case 'c':
		  count = true;
		  break;
		case 'd':
		  pld = true;
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
		char otherIRI[] = "OTHER-IRI"; // non http or https IRI (for option PLD)
		char bnode[] = "BNODE";
		unsigned long int cnt = 0;


		if ( (pld + qnames + pref) > 1 ) {
		  cout << "ERROR: you cannot choose options -p -q or -d concurrently " << endl;
		}
		
		while (itSol->hasNext()) {
		  current = reinterpret_cast<char*>(itSol->next());		 
		  if (pref || qnames) {
				current = process_term_preffix_qname(literal, bnode, pref, current);
		  } else if(pld) {
				current = process_term_pld(literal, bnode, otherIRI, current);
		  }
		  
		  if( !previous ){
		    previous = current;
		    cnt++;
		  }
	  
		  if ( strcmp(previous,current) ) {
		    cout << previous;
		    if (count) {
		      cout << " : " << cnt;
		    }
		    cout << endl;
		    cnt = 1;
		  } else {
		    cnt++;
		  }
		  previous = current;
		}

		if( previous ) {
		    cout << previous;
		    if (count) {
		      cout << " : " << cnt;
		    }
		    cout << endl;
		}
		
		delete itSol;
		delete hdt;
		
	} catch (std::exception& e) {
		cerr << "ERROR: " << e.what() << endl;
	}
}
