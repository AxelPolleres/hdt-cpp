#include <math.h>
#include <limits>
#include <iostream>
#include <string>
#include <fstream>
#include <climits>
#include "../../include/HDTEnums.hpp"

using namespace std;

/** A class to store the range of IDs of domains/PLDs/prefixes in the dictionary.
 */
class Domains {
private:
	vector<string> predicate_range; //each position corresponds to a predicate ID, one domain/PLD/prefix per position, potentially repeated but improves time search and the number of predicates is often limited

	// We assume a continuous range, i.e. the beginning of a range marks the end of the previous one
	// Otherwise a vector of pairs would be needed
	vector<unsigned int> shared_range; //subject Object range
	vector<unsigned int> subject_range; // subjects not in shared. The IDs here should start at numshared + 1
	vector<unsigned int> object_range; // objects not in shared. The IDs should start at numshared + 1

	vector<string> shared_terms; //different domains/PLDs/prefixes for each range in shared_range
	vector<string> subject_terms; //different domains/PLDs/prefixes for each range in subject_range
	vector<string> object_terms; //different domains/PLDs/prefixes for each range in object_range

	map<string,unsigned int> subjectsOccurrences; //additional index to count the number of different terms for each domains/PLDs/prefixes (aggregating subjects and shared)


	unsigned int numShared; // number of shared subject-object elements

	char delim = ' ';


	vector<string> split(const string &s, char delim) {
		stringstream ss(s);
		string item;
		vector<string> tokens;
		while (getline(ss, item, delim)) {
			tokens.push_back(item);
		}
		return tokens;
	}

	unsigned int temp_next_range; // A temporal variable to point to store the next range from the last query

	void iterate_file_section(const string& name, string rol) {
		string line;
		ifstream inputFile(name);
		unsigned int start, end, prevEnd = 0;
		int numLine = 0;
		if (inputFile.is_open()) {
			while (getline(inputFile, line)) {
				// skip first header line
				if (numLine != 0) {
					vector<string> parts = split(line, delim);
					istringstream(parts[1]) >> start;
					istringstream(parts[2]) >> end;

					if (numLine != 1) {
						if ((prevEnd + 1) != start) {
							throw std::runtime_error(
									string(
											"Non correlative ranges in " + rol
													+ " ranges"));
						}
					}

					if (rol == "p") { //insert as many times as the range (potentially duplicated but good for querying)
						for (int i = start; i <= end; i++) {
							predicate_range.push_back(parts[0]);
						}
					} else if (rol == "h") {
						shared_terms.push_back(parts[0]);
						shared_range.push_back(start); //just mark the beginning, we assumme correlative ranges

						// build additional index with the occurrences
					    subjectsOccurrences[parts[0]]=subjectsOccurrences[parts[0]]+(end-start+1);

					} else if (rol == "s") {
						subject_terms.push_back(parts[0]);
						subject_range.push_back(start); //just mark the beginning, we assumme correlative ranges

						// build additional index with the occurrences
						subjectsOccurrences[parts[0]]=subjectsOccurrences[parts[0]]+(end-start+1);

					} else if (rol == "o") {
						object_terms.push_back(parts[0]);
						object_range.push_back(start); //just mark the beginning, we assumme correlative ranges

					}
					prevEnd = end;

				}
				numLine++;
			}
			inputFile.close();
		}
	}

	pair<string,unsigned int> getDomainInRol(unsigned int id,vector<unsigned int> &vect_range,vector<string> &vect_terms) {
		string ret="";
		//search for ID in the vector of ranges
		vector<unsigned int>::iterator low = std::lower_bound(
				vect_range.begin(), vect_range.end(), id);
		// lower_bound return the position of the first equal or bigger element that the id
		unsigned int position = (low - vect_range.begin());
		//cout << endl << "lower_bound at position " << position << endl;
		if (position >= vect_range.size())
			//if not found, then the position should be the last
			position = (vect_range.size() - 1);

		//if the element there is bigger, then one should go to the previous position
		if (vect_range[position] > id)
			position--;

		ret = vect_terms[position];
		//cout << "domain is:" << ret << endl;

		// store the next range as a temporal variable (to save more checks in the future)
		if ((position+1)<vect_range.size()){
			temp_next_range=vect_range[(position+1)];
		}
		else{
			//store the maximum as it won't be more than this
			temp_next_range=UINT_MAX;
		}


		return pair<string,unsigned int>(ret,position);
	}



public:
	/** Constructor
	 * A histogram that can count within a range of values. All bins of the histogram are set to zero.
	 * @param Start Description of the param.
	 * @param End Description of the param.
	 * @param nBins Description of the param.
	 */

	Domains() :
			numShared(0),temp_next_range(0) {
		/*predicate_range = new vector<string>();
		 shared_range = new vector<int>();
		 subject_range = new vector<int>();
		 object_range = new vector<int>();

		 shared_terms = new vector<string>();
		 subject_terms = new vector<string>();
		 object_terms = new vector<string>();*/
	}

	Domains(string fileName, unsigned int num_Shared) {
		numShared = num_Shared;
		temp_next_range=0;
		/*	predicate_range = new vector<string>();
		 shared_range = new vector<int>();
		 subject_range = new vector<int>();
		 object_range = new vector<int>();

		 shared_terms = new vector<string>();
		 subject_terms = new vector<string>();
		 object_terms = new vector<string>();*/

		string name = fileName;
		name.append("-p.csv");
		iterate_file_section(name, "p");
		name = fileName;
		name.append("-h.csv");
		iterate_file_section(name, "h");
		name = fileName;
		name.append("-s.csv");
		iterate_file_section(name, "s");
		name = fileName;
		name.append("-o.csv");
		iterate_file_section(name, "o");

	}

	pair<string,unsigned int> getDomain(unsigned int id, hdt::TripleComponentRole rol) {
		pair<string,unsigned int> ret;

		if (rol==hdt::PREDICATE){
			return pair<string,unsigned int>(predicate_range[(id-1)],id-1); // in predicates we store one id per position
		}

		//first, check the ID to see if we have to look in shared
		if (id <= numShared) {
			ret = getDomainInRol(id,shared_range,shared_terms);
		} else {

			if (rol == hdt::SUBJECT) {
				//search for ID in the vector of ranges
				ret = getDomainInRol(id,subject_range,subject_terms);
			} else if (rol == hdt::OBJECT) {
				ret = getDomainInRol(id,object_range,object_terms);
			}
		}
		return ret;
	}


	void verbose_print() {
		cout << "== PREDICATES ==" << endl;
		for (int i = 0; i < predicate_range.size(); i++) {
			cout << (i + 1) << ": " << predicate_range[i] << endl;
		}
		cout << "== SHARED ==" << endl;
		for (int i = 0; i < shared_range.size(); i++) {
			cout << shared_range[i] << ": " << shared_terms[i] << endl;
		}
		cout << "== SUBJECTS ==" << endl;
		for (int i = 0; i < subject_range.size(); i++) {
			cout << subject_range[i] << ": " << subject_terms[i] << endl;
		}
		cout << "== OBJECTS ==" << endl;
		for (int i = 0; i < object_range.size(); i++) {
			cout << object_range[i] << ": " << object_terms[i] << endl;
		}

		cout << "Num shared:" << numShared << endl;

	}

	unsigned int getNext_range(){
		return temp_next_range;
	}

	vector<string> getTerms(string rol){
		if (rol=="subject" || rol =="s"){
			return subject_terms;
		}
		else if (rol=="shared" || rol=="h"){
			return shared_terms;
		}
		else if (rol=="object" || rol =="o"){
			return object_terms;
		}
		else
			return predicate_range;
	}

	unsigned int getSubjectOccurrences(string domain){
		return subjectsOccurrences[domain];
	}
	vector<string> getDomains(hdt::DictionarySection rol) {
		if (rol == hdt::NOT_SHARED_SUBJECT) {
			return subject_terms;
		}
		else if (rol == hdt::SHARED_SUBJECT || rol == hdt::SHARED_OBJECT) {
			return shared_terms;
		}
		else if (rol == hdt::NOT_SHARED_OBJECT) {
			return object_terms;
		}
		else
			return predicate_range;
	}

};
