/*
 * Copyright (C) 2012, Sjors Gielen <dazeus@sjorsgielen.nl>
 * See LICENSE for license.
 */

#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>

std::string strToLower(const std::string &f);

template <typename Container, typename Key>
bool contains(Container x, Key k) {
	return x.count(k) != 0;
}

template <typename Value>
bool contains(std::vector<Value> x, Value v) {
	return find(x.begin(), x.end(), v) != x.end();
}

template <typename Container, typename Value>
void erase(Container &x, Value v) {
	x.erase(remove(x.begin(), x.end(), v));
}

#endif
