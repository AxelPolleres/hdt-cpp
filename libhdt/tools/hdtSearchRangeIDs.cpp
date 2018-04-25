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

bool import = false;
hdt::HDT *hdt_file;
char delim = ' ';
bool verbose = false;

vector<string> split(const string &s, char delim) {
	stringstream ss(s);
	string item;
	vector<string> tokens;
	while (getline(ss, item, delim)) {
		tokens.push_back(item);
	}
	return tokens;
}

void help() {
	cout << "$ hdtSearchRangeIDs [options] <hdtfile> " << endl;
	cout << "\t-h\t\t\tThis help" << endl;
	cout << "\t-s\t<start>\t\tStart ID." << endl;
	cout << "\t-e\t<start>\t\tEnd ID." << endl;
	cout
			<< "\t-r\t<rol>\t\tRol = (s)ubject | (p)redicate | (o)bject | s(hared) | (a)ll  (shared and all only with -i import)"
			<< endl;
	cout
			<< "\t-i\t<inputFilePrefix>\t\tImport ranges from <outputFilePrefix-{rol}.csv.Format is: Term, Start ID, End ID"
			<< endl;
	cout << "\t-v\t\t\tVerbose, show results" << endl;

	//cout << "\t-v\tVerbose output" << endl;
}

unsigned int search_range(unsigned int start, unsigned int end,
		TripleComponentRole rol) {
	if (verbose)
		cout << "Let's search from " << start << " to " << end << " with rol "
				<< rol << endl;
	IteratorTripleID* it =
			((BitmapTriples*) (hdt_file->getTriples()))->searchRange(start, end,
					rol);
	if (verbose) {
		cout << "Done, let's see the results" << endl;
		while (it->hasNext()) {
			cout << *it->next() << endl;
		}
	}

	return it->estimatedNumResults();
	delete it;
}

void iterate_file_section(const string& name, TripleComponentRole rol) {
	string line;
	ifstream inputFile(name);
	unsigned int start, end, num_results;
	int numLine = 0;
	if (inputFile.is_open()) {
		while (getline(inputFile, line)) {
			// skip first header line
			if (numLine != 0) {
				vector<string> parts = split(line, delim);
				istringstream(parts[1]) >> start;
				istringstream(parts[2]) >> end;
				num_results = search_range(start, end, rol);
				cout << parts[0] << " " << num_results << endl;
			}
			numLine++;
		}
		inputFile.close();
	}
}

int main(int argc, char **argv) {
	int c;
	string inputFile;
	string importFileString;
	string startString, endString, rolString;
	unsigned int start = 0, end = 0;

	while ((c = getopt(argc, argv, "hs:e:r:i:v")) != -1) {
		switch (c) {
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
		case 'i':
			importFileString = optarg;
			import = true;
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

		istringstream(startString) >> start;
		istringstream(endString) >> end;

		TripleComponentRole rol = SUBJECT;
		if (!import) {

			if (rolString == "predicate" || rolString == "p") {
				rol = PREDICATE;
			} else if (rolString == "object" || rolString == "o") {
				rol = OBJECT;
			}

			unsigned int num_results = search_range(start, end, rol);
			cout << "Num triples:" << num_results << endl;
		} else {

			// import from CSVs

			string name = importFileString;

			TripleComponentRole rol;
			if (rolString == "predicate" || rolString == "p"
					|| rolString == "all" || rolString == "a") {
				cout << "== PREDICATES ==" << endl;
				cout << "* TERM: NUM TRIPLES *" << endl;
				rol = PREDICATE;
				name = importFileString;
				name.append("-p.csv");

				iterate_file_section(name, rol);
			}
			if (rolString == "subject" || rolString == "s" || rolString == "all"
					|| rolString == "a") {
				cout << endl<<endl<<"== SUBJECTS ==" << endl;
				cout << "* TERM: NUM TRIPLES *" << endl;
				rol = SUBJECT;
				name = importFileString;
				name.append("-s.csv");

				iterate_file_section(name, rol);
			}
			if (rolString == "object" || rolString == "o" || rolString == "all"
					|| rolString == "a") {
				cout << endl<<endl<< "== OBJECTS ==" << endl;
				cout << "* TERM: NUM TRIPLES *" << endl;
				rol = OBJECT;
				name = importFileString;
				name.append("-o.csv");

				iterate_file_section(name, rol);
			}
			if (rolString == "shared" || rolString == "h" || rolString == "all"
					|| rolString == "a") {
				cout << endl<<endl<< "== SHARED AS SUBJECT==" << endl;
				cout << "* TERM: NUM TRIPLES *" << endl;
				rol = SUBJECT;
				name = importFileString;
				name.append("-h.csv");
				iterate_file_section(name, rol);

				cout << endl<<endl<< "== SHARED AS OBJECT==" << endl;
				cout << "* TERM: NUM TRIPLES *" << endl;
				rol = OBJECT;
				iterate_file_section(name, rol);
			}

		}
		delete hdt_file;
	} catch (std::exception& e) {
		cerr << "ERROR!: " << e.what() << endl;
	}
}
