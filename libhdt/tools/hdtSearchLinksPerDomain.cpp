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

string colorFewLinks = "#949494";
int thresholdFewLinks = 50;
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

vector<string> colors = { "#e6194b", "#3cb44b", "#ffe119", "#0082c8", "#f58231",
		"#911eb4", "#46f0f0", "#f032e6", "#d2f53c", "#fabebe", "#008080",
		"#e6beff", "#aa6e28", "#fffac8", "#800000", "#aaffc3", "#808000",
		"#ffd8b1", "#000080", "#808080", "#FFFFFF", "#000000" };
// different colors from https://sashat.me/2017/01/11/list-of-20-simple-distinct-colors/
int prevColor = 0;
string getColor(unsigned int links) {
	string ret = "";
	//print default color if links are less than the threshold, typically 50
	if (links < thresholdFewLinks) {
		ret = colorFewLinks;
	} else
		ret = colors[prevColor % (colors.size())];
	prevColor++;
	return ret;

}

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
	cout
			<< "\t-e\t<exportFilePrefix>\t\tExport links in <outputFilePrefix.json> and <outputFilePrefix.csv> including the links in json and the plain info in csv"
			<< endl;
	cout << "\t-l\t\t\tKeep LITERAL count in the export (false by default)"
			<< endl;
	cout
			<< "\t-m\t<minLinks>\t\t\tExport only those domains with at least minLinks (default: 50)"
			<< endl;

	//cout << "\t-v\tVerbose output" << endl;
}

int main(int argc, char **argv) {
	int c;
	string inputFile;
	string importFileString, exportFileString;
	string testString, rolString;
	int testId = 0;
	bool test = false;
	bool exports = false;
	bool removeLiteral = true; //remove the LITERAL output in the export
	string literalDomain = "LITERAL";
	bool minLinks = true;
	int numMinLinks = 50;

	while ((c = getopt(argc, argv, "ht:r:i:ve:lm:")) != -1) {
		switch (c) {
		case 'h':
			help();
			break;
		case 'l':
			removeLiteral = false;
			break;
		case 't':
			testString = optarg;
			test = true;
			break;
		case 'i':
			importFileString = optarg;
			import = true;
			break;
		case 'e':
			exportFileString = optarg;
			exports = true;
			break;
		case 'r':
			rolString = optarg;
			break;
		case 'm':
			istringstream(optarg) >> numMinLinks;
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

		// Import the range of domains from the input file
		Domains domains(importFileString,
				hdt_file->getDictionary()->getNshared());

		if (verbose) {
			cout << endl << "(verbose)- Let's check the content of our ranges:"
					<< endl;
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

			cout << "(test)- See all links with ID=" << testId << " and rol "
					<< rolString << endl;
			TripleComponentRole origin_rol = SUBJECT;
			TripleComponentRole target_rol = OBJECT;

			if (rolString == "object" || rolString == "o") {
				origin_rol = OBJECT;
				target_rol = SUBJECT;
			}
			pair<string, unsigned int> origin = domains.getDomain(testId,
					origin_rol);
			string origin_domain = origin.first;

			IteratorTripleID* it =
					((BitmapTriples*) (hdt_file->getTriples()))->searchRange(
							testId, testId, origin_rol);

			unsigned int count = 1;
			while (it->hasNext()) {
				TripleID triple = *it->next();
				unsigned int checkID = triple.getObject();
				if (target_rol == SUBJECT) {
					checkID = triple.getSubject();
				}
				//cout<<"Let's check ID "<<checkID<<" as of "<<target_rol<<endl;
				pair<string, unsigned int> target = domains.getDomain(checkID,
						target_rol);
				string target_domain = target.first;
				if (origin_rol == SUBJECT)
					cout << "LINK " << count << ":" << origin_domain << " --> "
							<< target_domain << endl;
				else
					cout << "LINK " << count << ":" << origin_domain << " <-- "
							<< target_domain << endl;
				count++;
			}

			delete it;

		} else {
			unsigned int count = 1;
			map<string, int> simple_links; //count the different simple links (domain-subject domain-object);
			map<string, int> labeled_links; //count the different labeled links (domain-subject predicate domain-object);
			map<string, int> domainLabeled_links; //count the different domain-labeled links (domain-subject domain-predicate domain-object);
			//todo maybe compute also the type of the subject/object?

			IteratorTripleID* it = hdt_file->getTriples()->searchAll();

			pair<string, unsigned int> origin;
			pair<string, unsigned int> target;
			string origin_domain = "";
			unsigned int prevSubject = 0;
			unsigned int countPerDomain = 0;
			unsigned int subjectDomain = 0;
			vector<unsigned int> totalCountsPerDomain;
			while (it->hasNext()) {
				TripleID triple = *it->next();
				if ((triple.getSubject() != prevSubject)
						&& (triple.getSubject() >= domains.getNext_range())) {
					// some optimization to avoid more checks
					// - we look that the subject is different than the previous one
					// - we look that we have to enter into the next range
					// otherwise we don't get a new domain and we keep the previous
					origin = domains.getDomain(triple.getSubject(), SUBJECT);
					origin_domain = origin.first;
					prevSubject = triple.getSubject();
					totalCountsPerDomain.push_back(countPerDomain);
					countPerDomain = 0;
					subjectDomain++;
				}
				target = domains.getDomain(triple.getObject(), OBJECT);
				string target_domain = target.first;
				if (verbose) {
					cout << "LINK " << count << ":" << origin_domain << " --> "
							<< target_domain << endl;
				}
				count++;
				countPerDomain++;

				//store in the string maps
				string simple_link = origin_domain + " " + target_domain; // we assume the space is not found in the domains
				string labeled_link = origin_domain + " "
						+ hdt_file->getDictionary()->idToString(
								triple.getPredicate(), PREDICATE) + " "
						+ target_domain; // we assume the space is not found in the domains
				string domainLabeled_link =
						origin_domain + " "
								+ domains.getDomain(triple.getPredicate(),
										PREDICATE).first + " " + target_domain; // we assume the space is not found in the domains

				simple_links[simple_link] = simple_links[simple_link] + 1;
				labeled_links[labeled_link] = labeled_links[labeled_link] + 1;
				domainLabeled_links[domainLabeled_link] =
						domainLabeled_links[domainLabeled_link] + 1;

				//store in the matrix (for export and visualization)

			}
			//store the last count of the subject
			totalCountsPerDomain.push_back(countPerDomain);

			map<string, unsigned int> exportDomains; //save the new unique id of domains (needed as domains are repeated in shared and subjects, and shared and objects
			vector<string> differentDomains; //save the different domains
			map<unsigned int, unsigned int> exportCount; //save the total number of links per domain, i.e. ID--> count
			map<unsigned int, map<unsigned int, unsigned int>> exportMatrix; //i.e. IDsubject-->[IDObject-->count]
			// Print stats
			cout << "-------------------------------------" << endl;
			cout << endl << endl << "SIMPLE LINKS" << endl;
			unsigned int currentId = 1;
			for (std::map<string, int>::iterator it = simple_links.begin();
					it != simple_links.end(); ++it) {
				std::cout << it->first << " => " << it->second << '\n';

				//prepare the stats for the export
				if (exports) {
					vector<string> parts = split(it->first, ' ');
					string subject = parts[0];
					string object = parts[1];
					if (!removeLiteral || object != literalDomain) { //avoid literals

					  if (!minLinks||it->second>=numMinLinks){

					//first save the ID if it's a new subject or object
						if (exportDomains[subject] == 0) { //subject
							exportDomains[subject] = currentId;
							currentId++;
							differentDomains.push_back(subject);
						}
						if (exportDomains[object] == 0) { //object
							exportDomains[object] = currentId;
							currentId++;
							differentDomains.push_back(object);
						}

						//save the count
						// exportMatrix[exportDomains[subject]] //gets the row of the subject
						// now update the number of links for this object
						exportMatrix[exportDomains[subject]][exportDomains[object]] =
								it->second; //it->second has the number of links

						exportCount[exportDomains[subject]] =
								exportCount[exportDomains[subject]]
										+ it->second; //save the total count
					  }
					}
				}
			}

			cout << endl << endl << "LINKS WITH PREDICATES" << endl;
			for (std::map<string, int>::iterator it = labeled_links.begin();
					it != labeled_links.end(); ++it) {
				std::cout << it->first << " => " << it->second << '\n';
			}

			cout << endl << endl << "LINKS WITH DOMAIN PREDICATES" << endl;
			for (std::map<string, int>::iterator it =
					domainLabeled_links.begin();
					it != domainLabeled_links.end(); ++it) {
				std::cout << it->first << " => " << it->second << '\n';
			}
			cout << endl << endl << "-------------------------------------"
					<< endl << endl;

			if (exports) {

				ofstream exportFileCSV, exportFileJSON;
				string name = "";
				name.append(exportFileString).append(".csv");
				exportFileCSV.open(name.c_str());
				name = "";
				name.append(exportFileString).append(".json");
				exportFileJSON.open(name.c_str());

				//iterate and export first shared, then subjects
				exportFileCSV << "domain,links,color" << endl;

				for (int i = 0; i < differentDomains.size(); i++) {

						exportFileCSV << differentDomains[i] << ","
								<< exportCount[(i + 1)] << ","
								<< getColor(exportCount[(i + 1)]) << endl;

				}
				exportFileCSV.close();

				//now we print the matrix iterating through all IDs, even if there are empty (for consistency with the format)
				exportFileJSON << "[";
				for (int i = 1; i <= differentDomains.size(); i++) {
					if (i != 1)
						exportFileJSON << ","; //next list
					//if ((exportMatrix[i]!=NULL) &&
					if (exportMatrix[i].size() > 0) {
						exportFileJSON << "[";
						for (int j = 1; j <= differentDomains.size(); j++) {
							if (j != 1)
								exportFileJSON << ","; //next list
							exportFileJSON << exportMatrix[i][j];

						}
						exportFileJSON << "]";
					} else {
						exportFileJSON << "[";
						for (int j = 1; j < differentDomains.size(); j++) {
							exportFileJSON << "0,";
						}
						exportFileJSON << "0]";
					}

				}
				exportFileJSON << "]";
				exportFileJSON.close();

				/*
				 for (std::map<unsigned int, map<unsigned int, unsigned int>>::iterator it =
				 exportMatrix.begin(); it != exportMatrix.end(); ++it) {
				 // std::cout << it->first << ": [ ";
				 if (!firstlist)
				 exportFileJSON<<",";
				 exportFileJSON<<"[";
				 bool firstsublist=true;
				 map<unsigned int, unsigned int> row = it->second;
				 for (std::map<unsigned int, unsigned int>::iterator it =
				 row.begin(); it != row.end();
				 ++it) {
				 //std::cout << "(id "<<it->first<<")"<< it->second << " , " ;
				 if (!firstsublist)
				 exportFileJSON<<",";
				 exportFileJSON<<it->second;
				 firstsublist=false;
				 }
				 exportFileJSON<<"]";
				 //cout<<"]"<<endl;
				 firstlist=false;
				 }
				 exportFileJSON<<"]";
				 exportFileJSON.close();
				 */

			}

			delete it;
		}
		delete hdt_file;
	} catch (std::exception& e) {
		cerr << "ERROR!: " << e.what() << endl;
	}
}
