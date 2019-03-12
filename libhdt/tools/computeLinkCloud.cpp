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

#include <algorithm>
#include <string>

#include "../src/util/StopWatch.hpp"
#include "../src/triples/TripleIterators.hpp"
#include "../src/triples/BitmapTriples.hpp"
#include "../src/util/Domains.hpp"
#include "../src/util/terms.hpp"

#include <algorithm>    // std::find
#include <vector>

#include<dirent.h>

using namespace hdt;
using namespace std;

#include <unordered_set>

int thresholdNamespaces = 100; //disregard namespaces with less than 100 entities
bool import = false;
hdt::HDT *hdt_file1;
char delim = ' ';
bool verbose = false;
string literalDomain = "LITERAL";
string otherIRI = "OTHER-IRI"; // non http or https IRI (for option PLD)
string bnode = "BNODE";

unordered_set<std::string> plds;

struct DomainConnection {
	string source;
	string target;
	unsigned int numLinks;
};

struct DomainConnectionID {
	unsigned int sourceID;
	unsigned int targetID;
	unsigned int numLinks;
};

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
	cout << "$ computeLinkCloud [options] <hdtfile> " << endl;
	cout << "\t-h\t\t\tThis help" << endl;
	cout
			<< "\t-d\t\"dir1 dir2 dir3...\"\t\tImport dictionary ranges from the given directory names, from files <prefix_datasetName_...{rol}.csv.Format is: Term, Start ID, End ID"
			<< endl;
	cout
			<< "\t-v\t\t\tVerbose, show results (warning: will print all triples, use for test only)"
			<< endl;
	cout
			<< "\t-e\t<exportFilePrefix>\t\tExport cloud in <outputFilePrefix.json>"
			<< endl;
	cout
				<< "\t-A\t<exportFileAuthoritative>\t\tExport CSV of authoritative domains in <exportFileAuthoritative> file"
				<< endl;
	/*cout << "\t-l\t\t\tkeep LITERAL count in the export (false by default)"
	 << endl;*/
	cout
			<< "\t-m\t<minLinks>\t\t\tExport only those domains with at least minLinks (default: 50)"
			<< endl;

	/*cout << "\t-f\t\t\tPrint full raw numbers instead of percentage of links";*/

	//cout << "\t-v\tVerbose output" << endl;
}

// Driver function to sort the vector elements
// by second element of pairs
bool sortbysec(const pair<string, double> &a, const pair<string, double> &b) {
	return (a.second > b.second);
}

bool lower_test(char l, char r) {
	return (std::tolower(l) == std::tolower(r));
}

string trim(string str) {
	// trim trailing spaces
	size_t endpos = str.find_last_not_of(" \t");
	size_t startpos = str.find_first_not_of(" \t");
	if (std::string::npos != endpos) {
		str = str.substr(0, endpos + 1);
		str = str.substr(startpos);
	} else {
		str.erase(std::remove(str.begin(), str.end(), ' '), str.end());
	}

	// trim leading spaces
	startpos = str.find_first_not_of(" \t");
	if (string::npos != startpos) {
		str = str.substr(startpos);
	}
	return str;
}
void readPLDs(string filename) {
	ifstream inputFile(filename);
	string line = "";

	while (getline(inputFile, line)) {
		if (line[0] != '/') {
			line = trim(line);
			if (line.length() > 0) {
				//cout<<"line:"<<line<<";"<<endl;
				plds.insert(line);
			}
		}

	}
}

pair<string, string> splitPLD(string URI) {
	string currenthostname = string(
			process_term_hostname((char*) literalDomain.c_str(),
					(char*) bnode.c_str(), (char*) otherIRI.c_str(),
					(char*) URI.c_str()));

	string host = currenthostname;
	size_t pos = host.length();
	string extension, potentialExtension;
	bool processNextSubdomain = true;
	while (processNextSubdomain) {
		pos = currenthostname.find_last_of('.', pos - 1);
		if (pos != string::npos) {
			potentialExtension = currenthostname.substr(pos + 1);
			if (plds.find(potentialExtension) != plds.end()) {
				// found
				host = currenthostname.substr(0, pos);
				extension = potentialExtension;
			} else {
				// end as we won't find a longer domain if the short is not found
				processNextSubdomain = false;
			}
		} else {
			// not more '.' delimiters
			processNextSubdomain = false;
		}
	}
	// now get the domain before the pld
	pos = host.find_last_of('.');
	if (pos != string::npos) {
		host = host.substr(pos + 1);
	}
	//cout << "final host is:" << host << endl;
	//cout << "final extension is:" << extension << endl;

	return pair<string, string>(host, extension);
}

int main(int argc, char **argv) {
	int c;
	string inputFile;
	string importDirectoriesString, exportFileString,exportCSVAuthoritativeFile;
	string datasetURL = "";
	//so far we only allow one datasetURL for the full folder.
	//todo store potnetial different dataset URLs per file
	string PLDFile = "";
	bool use_PLDs = false;
	string testString, rolString;
	bool exports = false;
	//bool removeLiteral = false; //remove the LITERAL output in the export

	bool minLinks = true;
	int numMinLinks = 50;
	bool printPercentage = true;
	bool exportCSVAuthoritative=false;

	while ((c = getopt(argc, argv, "hd:ve:m:p:u:A:")) != -1) {
		switch (c) {
		case 'h':
			help();
			break;
			/*	case 'l':
			 removeLiteral = true;
			 break;*/
		case 'd':
			importDirectoriesString = optarg;
			import = true;
			break;
		case 'e':
			exportFileString = optarg;
			exports = true;
			break;
		case 'A':
			exportCSVAuthoritativeFile = optarg;
			exportCSVAuthoritative=true;
			break;
		case 'm':
			istringstream(optarg) >> numMinLinks;
			break;
			/*	case 'f':
			 printPercentage = false;
			 break;*/
		case 'p':
			PLDFile = optarg;
			use_PLDs = true;
			break;
		case 'u':
			datasetURL = optarg;
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

	if (use_PLDs) {
		readPLDs(PLDFile);
	}

	pair<string, string> dataset_host_pld;
	if (datasetURL != "") {
		dataset_host_pld = splitPLD(datasetURL);
	}
	vector<string> groups; //storing the different groups
	map<string, Domains> datasets; // a map with the name of the dataset and its range of domains
	map<string, pair<unsigned int, unsigned int>> datasetsGroups; // a map with the name of the dataset and a pair with its ID and the group
	map<string, vector<DomainConnection>> datasetConnections; // a map with the name of the dataset and the links
	// read directories
	vector<string> parts = split(importDirectoriesString, delim);

	unsigned int numDatasets = 1;
	for (int i = 0; i < parts.size(); i++) {
		string directoryDomain = parts[i];

		// get the group name
		vector<string> subparts = split(string(directoryDomain), '/');
		string groupName = subparts[subparts.size() - 1];
		cout << endl << "Group:" << groupName << endl;
		groups.push_back(groupName);

		// we assume the directory name is the name of the "group", i.e. bio2rdf
		// we assume the statistics are stores in a subfolder called result_analysis
		string result_analysis_Folder = directoryDomain + "/result_analysis";

		DIR *pDIR;
		struct dirent *entry;
		if (pDIR = opendir(result_analysis_Folder.c_str())) {
			while (entry = readdir(pDIR)) {
				if (strcmp(entry->d_name, ".") != 0
						&& strcmp(entry->d_name, "..") != 0) {
					if (strncmp(entry->d_name, "preffix", 7) == 0) { //check if it starts with preffix
						//cout << entry->d_name << "\n";

						// get the dataset name
						string nameFile(entry->d_name);
					//	subparts = split(nameFile, '_');
						//string datasetName = subparts[1]; // We used this in our prior analysis to split the main part but in LOD it is not needed
						string datasetName = nameFile;

						// check that we haven't processed this dataset
						if (datasets.find(datasetName) == datasets.end()) {
							cout << "Dataset name: " << datasetName << "\n";
							// parse the name, delete the final rol and extension, e.g. "-h.csv"
							nameFile = result_analysis_Folder + "/"
									+ nameFile.substr(0, nameFile.length() - 6);

							// Import the range of domains from the input file. Second parameter is the number of shared items, but we don't really need it for this use case
							Domains datasetDomain(nameFile, 0,
									thresholdNamespaces);

							datasets[datasetName] = datasetDomain;
							datasetsGroups[datasetName] = pair<unsigned int,
									unsigned int>(numDatasets, groups.size());
							numDatasets++;
							cout << "  -- inserted domain" << endl;

						}
					} else if ((strncmp(entry->d_name, "stats", 5) == 0)
							&& (strcmp(
									entry->d_name + strlen(entry->d_name) - 9,
									"cloud.csv") == 0)) { //check if it starts with stats and ends in cloud.csv
						// WE STORE the links of the dataset
						// get the dataset name

						string nameFile(entry->d_name);
						subparts = split(nameFile, '_');
						string datasetName = subparts[1];
						cout << "Computing stats for " << datasetName << endl;

						ifstream inputFile(
								result_analysis_Folder + "/" + entry->d_name);
						string line = "";
						getline(inputFile, line); //skip the first header line
						while (getline(inputFile, line)) {
							vector<string> nodes = split(line, ',');
							DomainConnection con;
							con.source = nodes[0];
							con.target = nodes[1];
							unsigned int numLinks;
							istringstream(nodes[2]) >> numLinks;
							con.numLinks = numLinks;
							datasetConnections[datasetName].push_back(con); // store the data connections

							//cout<<"Conn: "<<con.source<<" --> "<<con.target<< "("<<con.numLinks<<")"<<endl;

						}
					}

					// read the links (the authoritativeness will be checked later)

				}
			}
			closedir(pDIR);
		}
	}
	// Iterate trough all namespaces and assign as the authoritative dataset the one with more terms (as a subject) using such namespace
	map<string, vector<pair<string, double>>> authoritativeDataset; // map from a namespace to the dataset(s) that are authoritative for it

	map<string, Domains>::iterator it;

	cout << endl << endl << "Creating a set with all different domains..."
			<< endl;
	//first, create a set with all different domains
	set<string> domains;
	for (it = datasets.begin(); it != datasets.end(); it++) {
		Domains dom = it->second;

		vector<string> subjDomain = dom.getDomains(hdt::NOT_SHARED_SUBJECT);
		vector<string> sharedDomain = dom.getDomains(hdt::SHARED_SUBJECT);

		//cout << "subjDomain size:" << subjDomain.size() << endl;
		//cout << "sharedDomain size:" << sharedDomain.size() << endl;
		//insert in the set
		std::copy(subjDomain.begin(), subjDomain.end(),
				std::inserter(domains, domains.end()));
		std::copy(sharedDomain.begin(), sharedDomain.end(),
				std::inserter(domains, domains.end()));
	}
	cout << "  - Domains size:" << domains.size() << endl;
	cout << endl << endl
			<< "Iterating the set of different domains and compute the authoritative dataset(s)..."
			<< endl;
	// iterate the set of different domains and compute the authoritative dataset(s)
	std::set<string>::iterator doms;
	long numDoms = 1;
	cout << endl << "All counts of domains:" << endl;
	// for each domain, iterate the dataset and compute the max

	// For histograms
	int countUniqueCases = 0;
	int countUniqueCases_samePLD = 0;
	int countMultipleCases = 0;

	map<int, int> histogramUnique;
	histogramUnique[0] = histogramUnique[1] = histogramUnique[2] =
			histogramUnique[3] = 0;

	map<int, int> histogramUnique_samePLD;
	histogramUnique_samePLD[0] = histogramUnique_samePLD[1] =
			histogramUnique_samePLD[2] = histogramUnique_samePLD[3] = 0;

	map<int, int> histogramUnique_containDatasetName;
	histogramUnique_containDatasetName[0] =
			histogramUnique_containDatasetName[1] =
					histogramUnique_containDatasetName[2] =
							histogramUnique_containDatasetName[3] = 0;

	map<int, int> histogramMultiple_Top;
	histogramMultiple_Top[0] = histogramMultiple_Top[1] =
			histogramMultiple_Top[2] = histogramMultiple_Top[3] = 0;

	map<int, int> histogramMultiple_Top_containDatasetName;
	histogramMultiple_Top_containDatasetName[0] =
			histogramMultiple_Top_containDatasetName[1] =
					histogramMultiple_Top_containDatasetName[2] =
							histogramMultiple_Top_containDatasetName[3] = 0;


	map<int, int> histogramMultiple_DifferenceSecond;
	histogramMultiple_DifferenceSecond[0] =
			histogramMultiple_DifferenceSecond[1] =
					histogramMultiple_DifferenceSecond[2] =
							histogramMultiple_DifferenceSecond[3] = 0;

	int countContainDatasetName_Unique = 0;
	int countContainDatasetName_TopPosition = 0;
	int countContainDatasetName_notTopPosition = 0;
	for (doms = domains.begin(); doms != domains.end(); ++doms) {

		if (numDoms % 1000 == 0) {
			cout << "   " << numDoms << " domains" << endl;
		}
		numDoms++;
		string currentDomain = *doms;
		cout << endl << " - Domain: " << currentDomain << endl;

		pair<string, string> currentDomain_host_PLD;
		bool samehost = false; // for PLDs
		if (use_PLDs) {
			// get PLD and host name
			currentDomain_host_PLD = splitPLD(currentDomain);

			// try to search similarities between currentDomain and dataset URL
			if ((currentDomain_host_PLD.first == dataset_host_pld.first)
					&& (currentDomain_host_PLD.second == dataset_host_pld.second)) {
				// if the extensions are the same and the host is the same (we then obviate subdomain)
				samehost = true;
			}

		}

		double maxOccurrence = 0;
		vector<pair<string, double>> maxDatasets;
		// iterate all datasets
		map<string, Domains>::iterator subit;
		// Declaring vector of pairs
		vector<pair<string, double> > allPercentages; // store all percentages in case we want to print them
		for (subit = datasets.begin(); subit != datasets.end(); subit++) {
			string dataset = subit->first;
			Domains dom = subit->second;
			//unsigned int occs = dom.getSubjectOccurrences(currentDomain);
			//double occs = dom.getTotalOccurrencesPercentage(currentDomain);
			//consider the percentage of subjects+shared as the key for the authoritative
			//compute weights of the average between subjects and shared

			double occs = ((1-dom.getWeightShared())*(dom.getSubjectsOccurrencesPercentage(currentDomain)))+(dom.getWeightShared()*dom.getSubjectObjectsOccurrencesPercentage(currentDomain));
			if (occs > 0) {
				allPercentages.push_back(make_pair(dataset, occs)); // store all percentages
				if (occs > maxOccurrence) {
					maxDatasets.clear();
					maxDatasets.push_back(make_pair(dataset,occs));

				} else if (occs == maxOccurrence) {
					maxDatasets.push_back(make_pair(dataset,occs));
				}
			}
		}
		// sort by occurrence
		sort(allPercentages.begin(), allPercentages.end(), sortbysec);

		if (allPercentages.size() > 1) {
			countMultipleCases++;
		} else if (allPercentages.size() == 1) {
			countUniqueCases++;
			if (samehost) {
				countUniqueCases_samePLD++;
			}
			bool contains = false;
			//search if the dataset name is contained in the domain
			std::string::iterator fpos = std::search(currentDomain.begin(),
					currentDomain.end(), allPercentages[0].first.begin(),
					allPercentages[0].first.end(), lower_test);
			if (fpos != currentDomain.end()) {
				contains = true;
				countContainDatasetName_Unique++;
			}

			if (allPercentages[0].second < 25) {
				histogramUnique[0] = histogramUnique[0] + 1;
				if (samehost) {
					histogramUnique_samePLD[0] = histogramUnique_samePLD[0] + 1;
				}
				if (contains) {
					histogramUnique_containDatasetName[0] =
							histogramUnique_containDatasetName[0] + 1;
				}
			} else if (allPercentages[0].second < 50) {
				histogramUnique[1] = histogramUnique[1] + 1;
				if (samehost) {
					histogramUnique_samePLD[1] = histogramUnique_samePLD[1] + 1;
				}
				if (contains) {
					histogramUnique_containDatasetName[1] =
							histogramUnique_containDatasetName[1] + 1;
				}
			} else if (allPercentages[0].second < 75) {
				histogramUnique[2] = histogramUnique[2] + 1;
				if (samehost) {
					histogramUnique_samePLD[2] = histogramUnique_samePLD[2] + 1;
				}
				if (contains) {
					histogramUnique_containDatasetName[2] =
							histogramUnique_containDatasetName[2] + 1;
				}
			} else {
				histogramUnique[3] = histogramUnique[3] + 1;
				if (samehost) {
					histogramUnique_samePLD[3] = histogramUnique_samePLD[3] + 1;
				}
				if (contains) {
					histogramUnique_containDatasetName[3] =
							histogramUnique_containDatasetName[3] + 1;
				}
			}
		}

		// print all occurrences
		for (int i = 0; i < allPercentages.size(); i++) {
			cout << "    + dataset: " << allPercentages[i].first << " -> "
					<< allPercentages[i].second << " % of subjects (sh:"
					<< datasets[allPercentages[i].first].getSubjectObjectsOccurrencesPercentage(
							currentDomain) << " %; s:"
					<< datasets[allPercentages[i].first].getSubjectsOccurrencesPercentage(
							currentDomain) << " %; o:"
					<< datasets[allPercentages[i].first].getObjectsOccurrencesPercentage(
							currentDomain) << ")" << endl;

			if (allPercentages.size() > 1) {
				if (i == 0) {

					bool contains = false;
					//search if the dataset name is contained in the domain
					std::string::iterator fpos = std::search(
							currentDomain.begin(), currentDomain.end(),
							allPercentages[i].first.begin(),
							allPercentages[i].first.end(), lower_test);
					if (fpos != currentDomain.end()) {
						contains = true;
						countContainDatasetName_TopPosition++;
					}

					if (allPercentages[0].second < 25) {
						histogramMultiple_Top[0] = histogramMultiple_Top[0] + 1;
						if (contains) {
							histogramMultiple_Top_containDatasetName[0] =
									histogramMultiple_Top_containDatasetName[0]
											+ 1;
						}
					} else if (allPercentages[0].second < 50) {
						histogramMultiple_Top[1] = histogramMultiple_Top[1] + 1;
						if (contains) {
							histogramMultiple_Top_containDatasetName[1] =
									histogramMultiple_Top_containDatasetName[1]
											+ 1;
						}
					} else if (allPercentages[0].second < 75) {
						histogramMultiple_Top[2] = histogramMultiple_Top[2] + 1;
						if (contains) {
							histogramMultiple_Top_containDatasetName[2] =
									histogramMultiple_Top_containDatasetName[2]
											+ 1;
						}
					} else {
						histogramMultiple_Top[3] = histogramMultiple_Top[3] + 1;
						if (contains) {
							histogramMultiple_Top_containDatasetName[3] =
									histogramMultiple_Top_containDatasetName[3]
											+ 1;
						}
					}
				} else if (i == 1) {
					if ((allPercentages[0].second - allPercentages[1].second)
							< 25) {
						histogramMultiple_DifferenceSecond[0] =
								histogramMultiple_DifferenceSecond[0] + 1;
					} else if ((allPercentages[0].second
							- allPercentages[1].second) < 50) {
						histogramMultiple_DifferenceSecond[1] =
								histogramMultiple_DifferenceSecond[1] + 1;
					} else if ((allPercentages[0].second
							- allPercentages[1].second) < 75) {
						histogramMultiple_DifferenceSecond[2] =
								histogramMultiple_DifferenceSecond[2] + 1;
					} else {
						histogramMultiple_DifferenceSecond[3] =
								histogramMultiple_DifferenceSecond[3] + 1;
					}

					std::string::iterator fpos = std::search(
							currentDomain.begin(), currentDomain.end(),
							allPercentages[i].first.begin(),
							allPercentages[i].first.end(), lower_test);
					if (fpos != currentDomain.end()) {
						countContainDatasetName_notTopPosition++;
					}

				}
			}
		}

		// store the authoritative Dataset for the current domain
		authoritativeDataset[currentDomain] = maxDatasets;

	}

	//Print the histograms
	cout << endl << endl << endl << "Histograms:" << endl;
	cout << endl << "Unique: " << countUniqueCases << endl;

	cout << "-  x > 75%:" << histogramUnique[3] << endl;
	cout << "- 50% <= x < 75%:" << histogramUnique[2] << endl;
	cout << "- 25% <= x < 50%:" << histogramUnique[1] << endl;
	cout << "-  x < 25%:" << histogramUnique[0] << endl;

	cout << endl << "Multiple: " << countMultipleCases << endl;
	cout << " * Top: " << endl;
	cout << "-  x > 75%:" << histogramMultiple_Top[3] << endl;
	cout << "- 50% <= x < 75%:" << histogramMultiple_Top[2] << endl;
	cout << "- 25% <= x < 50%:" << histogramMultiple_Top[1] << endl;
	cout << "-  x < 25%:" << histogramMultiple_Top[0] << endl;

	cout << endl << " * Diff first-second:" << endl;
	cout << "-  x > 75%:" << histogramMultiple_DifferenceSecond[3] << endl;
	cout << "- 50% <= x < 75%:" << histogramMultiple_DifferenceSecond[2]
			<< endl;
	cout << "- 25% <= x < 50%:" << histogramMultiple_DifferenceSecond[1]
			<< endl;
	cout << "-  x < 25%:" << histogramMultiple_DifferenceSecond[0] << endl;

	if (use_PLDs) {
		cout << endl << "Unique with PLDs: " << countUniqueCases_samePLD
				<< endl;

		cout << "-  x > 75%:" << histogramUnique_samePLD[3] << endl;
		cout << "- 50% <= x < 75%:" << histogramUnique_samePLD[2] << endl;
		cout << "- 25% <= x < 50%:" << histogramUnique_samePLD[1] << endl;
		cout << "-  x < 25%:" << histogramUnique_samePLD[0] << endl;
	}

	cout << endl << "Unique where dataset contains domain name: "
			<< countContainDatasetName_Unique << endl;

	cout << "-  x > 75%:" << histogramUnique_containDatasetName[3] << endl;
	cout << "- 50% <= x < 75%:" << histogramUnique_containDatasetName[2]
			<< endl;
	cout << "- 25% <= x < 50%:" << histogramUnique_containDatasetName[1]
			<< endl;
	cout << "-  x < 25%:" << histogramUnique_containDatasetName[0] << endl;


	cout << endl << "Top dataset in Multiple options, where dataset contains domain name: "
				<< countContainDatasetName_TopPosition << endl;

		cout << "-  x > 75%:" << histogramMultiple_Top_containDatasetName[3] << endl;
		cout << "- 50% <= x < 75%:" << histogramMultiple_Top_containDatasetName[2]
				<< endl;
		cout << "- 25% <= x < 50%:" << histogramMultiple_Top_containDatasetName[1]
				<< endl;
		cout << "-  x < 25%:" << histogramMultiple_Top_containDatasetName[0] << endl;


	cout<<endl<<"There are '"<<countContainDatasetName_notTopPosition<< "' datasets not in the top position that contain the domain name"<<endl;
	// print authoritative domains
	cout << endl << endl << endl << "Authoritative domains:" << endl;
	map<string, vector<pair<string,double>>>::iterator auth;

	ofstream exportFileCSVAuth;
	if (exportCSVAuthoritative)
		exportFileCSVAuth.open(exportCSVAuthoritativeFile.c_str());
	for (auth = authoritativeDataset.begin();
			auth != authoritativeDataset.end(); auth++) {
		string domain = auth->first;
		if (auth->second.size()>0){
			if (exportCSVAuthoritative)
				exportFileCSVAuth<<domain<<";";
			cout << endl << " - Domain: " << domain << endl;
			vector<pair<string,double>> authDatasets = auth->second;
			for (int i = 0; i < authDatasets.size(); i++) {
				cout << "    + Auth. dataset: " << authDatasets[i].first << "("<<authDatasets[i].second<<"%)"<<endl;
			}
			if (exportCSVAuthoritative)
						exportFileCSVAuth<<authDatasets[0].first<<";"<<authDatasets[0].second<<endl;
		}
	}
	if (exportCSVAuthoritative)
		exportFileCSVAuth.close();

	// now iterate the domains in datasetConnections with the domain links per dataset
	// 1.- replace the source/target with the dataset that is authoritative
	// 2.- only consider those links where the subject or object is Authoritative of the current dataset

	cout << endl << endl
			<< "Iterating the domains in datasetConnections with the domain links per dataset..."
			<< endl;
	vector<DomainConnectionID> finalLinks;
	map<unsigned int, unsigned int> sizeDomains; //size of each domain
	map<string, vector<DomainConnection>>::iterator linksIt;
	for (linksIt = datasetConnections.begin();
			linksIt != datasetConnections.end(); linksIt++) {
		string dataset = linksIt->first;
		vector<DomainConnection> con = linksIt->second;
		for (int n = 0; n < con.size(); n++) {
			// get the authoritative dataset of the source and target
			vector<string> autSource;
			for (int l=0;l<authoritativeDataset[con[n].source].size();l++){
				autSource.push_back(authoritativeDataset[con[n].source][l].first);
			}

			vector<string> autTarget;
			for (int l=0;l<authoritativeDataset[con[n].target].size();l++){
							autSource.push_back(authoritativeDataset[con[n].target][l].first);
						}


			// check if the current dataset is authoritative in either source of target
			if ((std::find(autSource.begin(), autSource.end(), dataset)
					!= autSource.end())
					|| (std::find(autTarget.begin(), autTarget.end(), dataset)
							!= autTarget.end())) {
				// then, prepare the output and replace the string with IDs.
				for (int i = 0; i < autSource.size(); i++) {
					// look for the dataset authorative ID
					if (datasetsGroups.find(autSource[i])
							!= datasetsGroups.end()) {
						pair<unsigned int, unsigned int> pairIdsSource =
								datasetsGroups[(autSource[i])];

						for (int j = 0; j < autTarget.size(); j++) {
							if (datasetsGroups.find(autTarget[j])
									!= datasetsGroups.end()) {
								pair<unsigned int, unsigned int> pairIdsTarget =
										datasetsGroups[(autTarget[j])];
								DomainConnectionID conID;
								conID.sourceID = pairIdsSource.first;
								conID.targetID = pairIdsTarget.first;
								conID.numLinks = con[n].numLinks;
								sizeDomains[pairIdsSource.first] =
										sizeDomains[pairIdsSource.first]
												+ conID.numLinks;

								//add to final results
								finalLinks.push_back(conID);
							}
						}
					}
				}
			}
		}
	}

	ofstream exportFile;
	exportFile.open(exportFileString.c_str());
	// show the final links
	if (finalLinks.size() > 0) {
		exportFile << "{" << endl;
		exportFile << "  \"links\" : [" << endl;

		for (int i = 0; i < finalLinks.size(); i++) { // -1 because nodes should start in 0
			exportFile << "   {" << endl;
			exportFile << "    \"source\" : " << (finalLinks[i].sourceID - 1)
					<< "," << endl;
			exportFile << "    \"target\" : " << (finalLinks[i].targetID - 1)
					<< "," << endl;
			exportFile << "    \"size\" : " << finalLinks[i].numLinks << endl;
			if ((i + 1) < finalLinks.size()) {
				exportFile << "   }," << endl;
			} else
				exportFile << "   }" << endl;

		}
		exportFile << "  ]," << endl;
		exportFile << "  \"nodes\" : [" << endl;

		map<string, pair<unsigned int, unsigned int>>::iterator datsIt;
		int i = 0;
		for (datsIt = datasetsGroups.begin(); datsIt != datasetsGroups.end();
				datsIt++) {

			exportFile << "   {" << endl;
			exportFile << "    \"name\" : \"" << datsIt->first << "\"," << endl;
			exportFile << "    \"group\" : " << datsIt->second.second << ","
					<< endl;
			unsigned int sizedom = sizeDomains[datsIt->second.first];
			exportFile << "    \"size\" : " << sizedom << endl;
			if ((i + 1) < datasetsGroups.size()) {
				exportFile << "   }," << endl;
			} else
				exportFile << "   }" << endl;
			i++;
		}

		exportFile << "  ]" << endl;
		exportFile << "}" << endl;

	}
	exportFile.close();

}

