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
#include "../src/util/Domains.hpp"

using namespace hdt;
using namespace std;

bool import = false;
hdt::HDT *hdt_file;
char delim = ' ';
bool verbose = false;

void help() {
	cout << "$ hdtSearchLinksPerDomain [options] <hdtfile> " << endl;
	cout << "\t-h\t\t\tThis help" << endl;
	cout
			<< "\t-i\t<inputFilePrefix>\t\tImport ranges from <outputFilePrefix-{rol}.csv.Format is: Term, Start ID, End ID"
			<< endl;
	cout
			<< "\t-t\t<testID>\t\tOnly fetch the links for the given testID. Otherwise all links are inspected"
			<< endl;
	cout
			<< "\t-r\t<testRol>\t\tRol for the test = (s)ubject | (o)bject. By default, subject"
			<< endl;

	cout
			<< "\t-v\t\t\tVerbose, show results (warning: will print all triples, use for test only)"
			<< endl;

	//cout << "\t-v\tVerbose output" << endl;
}

int main(int argc, char **argv) {
	int c;
	string inputFile;
	string importFileString;
	string testString, rolString;
	int testId = 0;
	bool test = false;

	while ((c = getopt(argc, argv, "ht:r:i:v")) != -1) {
		switch (c) {
		case 'h':
			help();
			break;
		case 't':
			testString = optarg;
			test = true;
			break;
		case 'i':
			importFileString = optarg;
			import = true;
			break;
		case 'r':
			rolString = optarg;
			break;
		case 'v':
			verbose = true;
			break;
		default:
			cout << "ERROR: Unknown option" << endl;
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
		hdt_file = HDTManager::mapIndexedHDT(inputFile.c_str());

		istringstream(testString) >> testId;

		Domains domains(importFileString,
				hdt_file->getDictionary()->getNshared());

		if (verbose) {
			cout << endl << "(verbose)- Let's check the content of our ranges:" << endl;
			domains.verbose_print();
			cout << endl << endl;

			cout << "(verbose)- Let's print current triples!" << endl;
			IteratorTripleString* itS = hdt_file->search("", "", "");
			while (itS->hasNext()) {
				cout << *(itS->next()) << endl;
			}
			cout << endl << endl;
		}

		if (test) {

			cout<< "(test)- See all links with ID="<<testId<<" and rol "<<rolString<<endl;
			TripleComponentRole origin_rol = SUBJECT;
			TripleComponentRole target_rol = OBJECT;

			if (rolString == "object" || rolString == "o") {
				origin_rol = OBJECT;
				target_rol = SUBJECT;
			}
			string origin_domain = domains.getDomain(testId, origin_rol);

			IteratorTripleID* it =
					((BitmapTriples*) (hdt_file->getTriples()))->searchRange(
							testId, testId, origin_rol);

			unsigned int count=1;
			while (it->hasNext()) {
				TripleID triple = *it->next();
				unsigned int checkID = triple.getObject();
				if (target_rol == SUBJECT) {
					checkID = triple.getSubject();
				}
				//cout<<"Let's check ID "<<checkID<<" as of "<<target_rol<<endl;
				string target_domain = domains.getDomain(checkID, target_rol);
				if (origin_rol==SUBJECT)
					cout << "LINK " << count<<":"<<origin_domain << " --> " << target_domain<< endl;
					else
						cout << "LINK " << count<<":"<<origin_domain << " <-- " << target_domain<< endl;
				count++;
			}

			delete it;

		} else {
			unsigned int count=1;

			IteratorTripleID* it =
					hdt_file->getTriples()->searchAll();

			string origin_domain="";
			unsigned int prevSubject=0;
			while (it->hasNext()) {
				TripleID triple = *it->next();
				if ((triple.getSubject()!=prevSubject) && (triple.getSubject()>=domains.getNext_range())) {
					// some optimization to avoid more checks
					// - we look that the triple is different than the previous one
					// - we look that we have to enter into the next range
					// otherwise we don't get a new domain and we keep the previous
					origin_domain = domains.getDomain(triple.getSubject(), SUBJECT);
					prevSubject = triple.getSubject();
				}
				string target_domain = domains.getDomain(triple.getObject(), OBJECT);
				cout << "LINK " << count<<":"<<origin_domain << " --> " << target_domain<< endl;
				count++;
			}

			delete it;
		}
		delete hdt_file;
	} catch (std::exception& e) {
		cerr << "ERROR!: " << e.what() << endl;
	}
}
