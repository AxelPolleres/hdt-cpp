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

char literal[] = "LITERAL";
char otherIRI[] = "OTHER-IRI"; // non http or https IRI (for option PLD)
char bnode[] = "BNODE";
char delim=' ';
bool counts = false;
bool pld = false;
bool pref = false;
bool qnames = false;
bool exportOutput = false;
char* filter = 0;

void help() {
	cout << "$ dumpDictionary <HDT file>" << endl;
	cout << "\t-h\t\t\t\tThis help" << endl;
	cout
			<< "\t-d\t\t\t\tPrint all distinct Pay-level-domains (PLDs occurring in HTTP and HTTPS IRIs"
			<< endl;
	cout << "\t-p\t\t\t\tPrint all distinct non-qname prefixes" << endl;
	cout
			<< "\t-q\t\t\t\tPrint all qnames per prefixes (that is, same qname per different prefix will appear duplicated!)"
			<< endl;
	cout << "\t-r <rol>\t\t\tRol (s|p|o|h|a), where a=all, h=shared." << endl;
	cout << "\t-c\t\t\t\talso print counts" << endl;
	cout << "\t-f <PREFIX>\t\t\tfilter the dictionary by prefix PREFIX" << endl;
	cout
			<< "\t-e <outputFilePrefix>\t\texport ranges to <outputFilePrefix-{rol}.csv> "
			<< endl;
	// TODO: add a verbose option:
	//cout << "\t-v\tVerbose output" << endl;
}

void print_CSV_header(ofstream& exportFile){
	exportFile<<"<Term>"<<delim<<"<Initial ID>"<<delim<<"<End ID> \n";
}

void removeSpace(char* s)
{
    for (char* s2 = s; *s2; ++s2) {
        if (*s2 != ' ')
            *s++ = *s2;
    }
    *s = 0;
}

void parse_terms_iterator(ofstream& exportFile, int startID,
		IteratorUCharString* itSol) {
	char* previous = 0;
	char* current = 0;
	int endID = startID - 1;
	unsigned long int cnt = 0;
	while (itSol->hasNext()) {
		current = reinterpret_cast<char*>(itSol->next());
		removeSpace(current); // make sure there are no spaces in preffixes in order to write the output file
		if (pref || qnames) {
			current = process_term_preffix_qname(literal, bnode, pref, current);
		} else if (pld) {
			current = process_term_hostname(literal, bnode, otherIRI, current);
		}

		if (!previous) {
			previous = current;
			//cnt++;
		}
		if (strcmp(previous, current)) {
			cout << previous;
			if (counts) {
				cout << " : " << cnt;
			}
			cout << endl;
			cnt = 1;
			if (exportOutput) {
				exportFile << previous<<delim<< startID << delim<<endID << "\n";
			}
			startID = endID + 1;
		} else {
			cnt++;
		}
		previous = current;
		endID++;
	}
	if (previous) {
		cout << previous;
		if (counts) {
			cout << " : " << cnt;
		}
		cout << endl;
		if (exportOutput) {
			exportFile << previous<<delim<< startID <<delim<<endID << "\n";
		}
	}
}

int main(int argc, char **argv) {
	int c;
	string query, inputFile, rolUser = "a";

	string outFile = "output.csv";

	while ((c = getopt(argc, argv, "hcdpqf:r:e:")) != -1) {
		switch (c) {
		case 'h':
			help();
			break;
		case 'c':
			counts = true;
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
		case 'e':
			outFile = optarg;
			exportOutput = true;
			break;
		case 'f':
			filter = optarg;
			break;
		case 'r':
			rolUser = optarg;
			if( rolUser != "s"
			    &&  rolUser != "p"
			    && rolUser != "o"
			    && rolUser != "h"
			    && rolUser != "a" ) {
			  cerr << "ERROR: Unknown role" << endl;
			  help();
			  return 1;
			}
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

		IteratorUCharString * itSol=NULL;
		IteratorUCharString * itSol2=NULL;
		IteratorUCharString * itSol3=NULL;
		IteratorUCharString * itSol4=NULL;


		if(filter)
		  { TripleComponentRole role;
		    if ((rolUser =="s") || (rolUser =="p") || (rolUser =="o")) {
		      if(rolUser == "s")
			role = SUBJECT;
		      else if (rolUser == "p")
			role = PREDICATE;
		      else
			role = OBJECT;
		      itSol = dict->getSuggestions(filter,role);
		    }
		    else if( rolUser == "h") {
		      // TODO: I would need something like an IntersectIteratorUCharString to make that work, so, for the
		      // moment, I just forbid role "h" 
		      // itSol = new IntersectIteratorUCharString(dict->getSuggestions(filter,SUBJECT),
		      //				   dict->getSuggestions(filter,OBJECT));
		      cout << "ERROR: role 'h' not allowed in combination with -f" << endl;
		      help();
		      return 1;		      
		    } else if( rolUser == "a") {
		      itSol = new MergeIteratorUCharString(
							   new MergeIteratorUCharString(dict->getSuggestions(filter,SUBJECT),
											dict->getSuggestions(filter,PREDICATE)),
							   dict->getSuggestions(filter,OBJECT));
		    } else {
		      assert(0);
		    }
		  }
		else
		  {
		    switch (rolUser[0]) {
		    case 'o':
		      if (!exportOutput) {
			itSol = new MergeIteratorUCharString(dict->getShared(),
							     dict->getObjects());
		      } else {
			itSol = dict->getShared();
			itSol2 = dict->getObjects();
		      }
		      break;
		    case 's':
		      if (!exportOutput) {
			itSol = new MergeIteratorUCharString(dict->getShared(),
							     dict->getSubjects());
		      } else {
			itSol = dict->getShared();
			itSol2 = dict->getSubjects();
		      }
		      break;
		    case 'p':
		      itSol = dict->getPredicates();
		      break;
		    case 'h':
		      itSol = dict->getShared();
		      break;
		    case 'a':
		      if (!exportOutput) {
			itSol = new MergeIteratorUCharString(
							     new MergeIteratorUCharString(
											  new MergeIteratorUCharString(dict->getShared(),
														       dict->getObjects()),
											  dict->getSubjects()), dict->getPredicates());
		      } else {
			itSol = dict->getShared();
			itSol2 = dict->getSubjects();
			itSol3 = dict->getObjects();
			itSol4 = dict->getPredicates();
		      }
		      break;
		    default:
		      cout << "ERROR: Unknown option" << endl;
		      help();
		      return 1;
		    }
		  }

		if ((pld + qnames + pref) > 1) {
			cout << "ERROR: you cannot choose options -p -q or -d concurrently "
					<< endl;
		}

		ofstream exportFile;
		int startID = 1;
		string name = "";

		if (!exportOutput) //just a simple parse on the iterator
			parse_terms_iterator(exportFile, startID, itSol);
		else {
			// check the rol
			if (rolUser == "p") {

				name.append(outFile).append("-").append("p").append(".csv");
				exportFile.open(name.c_str());
				print_CSV_header(exportFile);
				parse_terms_iterator(exportFile, startID, itSol);
				exportFile.close();
			} else { // s, o, all or shared

				// first check the shared
				name.append(outFile).append("-").append("h").append(".csv");
				exportFile.open(name.c_str());
				print_CSV_header(exportFile);
				parse_terms_iterator(exportFile, startID, itSol);
				exportFile.close();
				name = "";
				startID=hdt->getDictionary()->getNshared()+1;
				if (rolUser == "s" || rolUser == "a") {
					// first check the subjects
					name.append(outFile).append("-").append("s").append(".csv");
					exportFile.open(name.c_str());
					print_CSV_header(exportFile);

					parse_terms_iterator(exportFile, startID, itSol2);
					exportFile.close();
					name = "";

					if (rolUser == "a") {
						// check the objects, then the predicates
						name.append(outFile).append("-").append("o").append(
								".csv");
						exportFile.open(name.c_str());
						print_CSV_header(exportFile);
						parse_terms_iterator(exportFile, startID, itSol3);
						exportFile.close();
						name = "";

						//finally, the predicates
						name.append(outFile).append("-").append("p").append(
								".csv");
						exportFile.open(name.c_str());
						print_CSV_header(exportFile);
						parse_terms_iterator(exportFile, 1, itSol4);
						exportFile.close();
					}
				}
				else if (rolUser=="o"){
					// check the objects
					name.append(outFile).append("-").append("o").append(
							".csv");
					exportFile.open(name.c_str());
					print_CSV_header(exportFile);
					parse_terms_iterator(exportFile, startID, itSol2);
					exportFile.close();
				} else {
				  assert(0);
				}

			}

		}

		delete itSol;
		delete hdt;

	} catch (std::exception& e) {
		cerr << "ERROR: " << e.what() << endl;
	}
}
