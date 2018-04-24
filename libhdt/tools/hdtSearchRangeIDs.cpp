/*
 * Tutorial01.cpp
 *
 *  Created on: 02/03/2011
 *      Author: mck
 */

#include <HDT.hpp>
#include <HDTManager.hpp>

#include <getopt.h>
#include <string>
#include <iostream>
#include <fstream>

#include "../src/util/StopWatch.hpp"

#include "../src/triples/TripleIterators.hpp"
#include "../src/triples/BitmapTriples.hpp"

using namespace hdt;
using namespace std;


void help() {
	cout << "$ hdtSearchRangeIDs [options] <hdtfile> " << endl;
	cout << "\t-h\t\t\tThis help" << endl;
	cout << "\t-s\t<start>\t\tStart ID." << endl;
	cout << "\t-e\t<start>\t\tEnd ID." << endl;
	cout << "\t-r\t<rol>\t\tRol = subject | predicate | object." << endl;
	cout << "\t-c\t\t\tDo not show results, just count the range." << endl;

	//cout << "\t-v\tVerbose output" << endl;
}


int main(int argc, char **argv) {
	int c;
	string inputFile;
	string startString, endString,rolString;
	unsigned int start=0, end=0;
	bool count = false;

	while( (c = getopt(argc,argv,"hs:e:cr:"))!=-1) {
		switch(c) {
		case 'h':
			help();
			break;
		case 's':
			startString = optarg;
			break;
		case 'e':
			endString = optarg;
			break;
		case 'r':
			rolString = optarg;
			break;
		case 'c':
			count = true;
			break;
		default:
			cout << "ERROR: Unknown option" << endl;
			help();
			return 1;
		}
	}

	if(argc-optind<1) {
		cout << "ERROR: You must supply an HDT File" << endl << endl;
		help();
		return 1;
	}

	inputFile = argv[optind];


	try {
		HDT *hdt = HDTManager::mapIndexedHDT(inputFile.c_str());

		int numb;
		istringstream ( startString ) >> start;
		istringstream ( endString ) >> end;

		TripleComponentRole rol = SUBJECT;
		if (rolString=="predicate"||rolString=="p"){
			rol = PREDICATE;
		}
		else if (rolString=="object"||rolString=="o"){
			rol = OBJECT;
		}

		cout<<"Let's search from "<<start<<" to "<<end<<" with rol "<<rol<<endl;
		IteratorTripleID *it = ((BitmapTriples*)hdt->getTriples())->searchRange(start,end,rol);
		cout<<"Done, let's see the results"<<endl;
		if (!count){
			while(it->hasNext()) {
				cout << *it->next() << endl;

			}
		}

	   cout<< "Num triples:"<<it->estimatedNumResults()<<endl;



		delete it;

		delete hdt;
	} catch (std::exception& e) {
		cerr << "ERROR: " << e.what() << endl;
	}
}
