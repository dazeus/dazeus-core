/*
 * Copyright (C) 2012, Sjors Gielen <dazeus@sjorsgielen.nl>
 * See LICENSE for license.
 */

#include "utils.h"

std::string strToLower(const std::string &f) {
	std::string res;
	res.reserve(f.length());
	for(unsigned i = 0; i < f.length(); ++i) {
		res.push_back(tolower(f[i]));
	}
	return res;
}
