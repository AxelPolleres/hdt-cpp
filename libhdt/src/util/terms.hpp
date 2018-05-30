/*
 * terms.hpp
 *
 *  Created on: 17/04/2018
 *      Author: Javier D. Fernández and Axel Polleres
 */
#include <stdexcept>
#ifndef TERMS_HPP_
#define TERMS_HPP_

#include <iostream>

using namespace std;

/**
 * Extract the preffix or qname from a dictionary term
 */
char* process_term_preffix_qname(char literal[], char bnode[], bool pref,char* term) {
	char* pscheme = 0;
	char* ph = 0; // position of last hash
	char* ps = 0; // position of last slash
	char* pc = 0; // position of last colon
	char* pu = 0; // position of last underline '_'
	char* p = 0; // position of namespace separator
	// find Prefix-Qnames separator:
	// ignore literals or weird identifiers for qname or prefix search.!
	if (term[0] == '"') {
		term = literal;
	} else if (term[0] == '_') {
		term = bnode;
	} else {
		// look for fartherst right '#','/', or ':'
		pscheme = strchr(term, ':');
		//cout << "x1" << endl;
		while (pscheme != NULL && *(++pscheme) == '/') {
			// find the last position of the scheme part of the URI.
		}
		//cout << "x2" << endl;
		if (pscheme == NULL) {
			pscheme = term;
		}
		//cout << "x3" << endl;
		ps = strrchr(pscheme, '/');
		ph = strrchr(pscheme, '#');
		pc = strrchr(pscheme, ':');
		pu = strrchr(pscheme, '_'); //added as a test
		if (ph || pc || ps || pu) {
			p = (pc > ph) ? pc : ph; // set p to the max of ph,pc,ps
			p = (ps > p) ? ps : p;
			p = (pu > p) ? pu : p;
			if (pref) { // print prefix only
				//TODO: Is that evil? i.e. does it cause memory troubles to simply overwrite a character in the middle with 0?
				*(p + 1) = 0;
			} else { // print qname only
				term = (p + 1);
			}
		}
	}
	return term;
}

/**
 * Extract the Pay Level Domain from a dictionary term
 */

char* process_term_hostname(char literal[], char bnode[], char otherIRI[],
		char* term) {
	char *p; // position of namespace separator
	if (term[0] == '"') {
		term = literal;
	} else if (term[0] == '_') {
		term = bnode;
	} else {
		int start = 0;
		// do search HTTP and HTTPS URIS for for, server-addresses-or-(sub-)domains
		if (strncasecmp(term, "http://", 7) == 0) {
			start = 7;
		} else if (strncasecmp(term, "https://", 8) == 0) {
			start = 8;
		}

		if (start) {
			// Note: we do not really collect pay-level-domains here yet,
			// but rather "server-addresses-or-(sub-)domains":
			term += start;

			// strip off anythin after the domain/host name:
			p = strchr(term, '/');
			if (p == NULL) {
				p = strchr(term, '#');
			}

			if (p != NULL) {
				*p = 0;
			}

			// strip off a leading user-name:
			p = NULL;
			p = strchr(term, '@');
			if (p != NULL) {
				term = p + 1;
			}

			// strip off a trailing port number:
			p = term + strlen(term) - 1;
			//cout << current << " ! " << p << endl ;
			while (isdigit(p[0])) {
				//cout << "!! " << p << endl ;
				p--;
			}
			if (*p == ':') {
				*p = 0;
			}

			// cout << "found a HTTPS or HTTP IRI with PLD: '" << current << "'" << endl;
		} else {
			term = otherIRI;
		}
	}
	return term;
}

#endif /* TERMS_HPP_ */
