/*
 * Copyright (C) 2012, Sjors Gielen <dazeus@sjorsgielen.nl>
 * See LICENSE for license.
 */

#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <stdint.h>
#include "utils.h"

uint8_t read_utf8_byte(const std::string &json, size_t &pos, bool is_first) {
	if(json[pos] != '\\' || json[pos+1] != 'u') {
		throw std::runtime_error("Assumption failed: Valid Unicode out of JSON write function");
	}
	std::stringstream ss;
	char unicode[4] = {json[pos+2], json[pos+3], json[pos+4], json[pos+5]};
	pos += 6;
	ss << std::hex << unicode;
	uint32_t res;
	ss >> res;

	/// Sanity checking of read byte
	// If it's 0xxxxxxx, it's a single ASCII byte, only allowed if it's the "first" one
	if(is_first && res < 128) {
		return (uint8_t)res;
	}
	// We're fixing issues where \u00XX appears incorrectly, assuming this is needed
	if(res > 255) {
		throw std::runtime_error("Assumption failed: Only byte representations out of JSON write function");
	}
	// If it's a first byte, may not be 10xxxxxx
	if(is_first && res < 192) {
		throw std::runtime_error("Assumption failed: First bits of Unicode first byte must be 11");
	}
	// If it's a continuation byte, must be 10xxxxxx
	if(!is_first && (res < 128 || res >= 192)) {
		throw std::runtime_error("Assumption failed: First bits of Unicode continuation byte must be 10");
	}

	return (uint8_t)res;
}

uint32_t read_utf8_codepoint(const std::string &json, size_t &pos) {
	uint8_t first = read_utf8_byte(json, pos, true);
	if(first < 128) {
		// Single byte, leave as-is
		return first;
	}

	// Source: https://en.wikipedia.org/wiki/Utf-8#Description
	// The number of bytes for the character is the number of set bits to
	// the left of this byte; i.e. 0b1111_0XXX means this character
	// consists of 4 bytes -- except for a character consisting of 1 byte,
	// but that's already handled above.
	uint8_t num_bytes;
	for(num_bytes = 0;; ++num_bytes) {
		// explictly store this in a variable, as the compiler might
		// optimize (first << 2 >> 7) to be (first >> 5).
		uint8_t s = first << num_bytes;
		if((s >> 7) == 0) break;
	}

	// Since RFC 3629, UTF-8 won't have > 4 characters anymore, and we
	// won't handle 1-byte characters here.
	if(num_bytes == 1 || num_bytes > 4) {
		std::cerr << "In read_utf8_codepoint(), JSON string has invalid UTF-8 "
		             "character (num_bytes = " << num_bytes << "), returning as-is."
		          << std::endl;
		return first;
	}

	// Assemble the resulting codepoint using the remaining bits
	uint32_t res;
	{
		uint32_t s = first;
		s = s << (num_bytes + 25);
		res = s >> (num_bytes + 25);
	}
	while(num_bytes-- > 1) {
		uint8_t next_byte = read_utf8_byte(json, pos, false);
		uint8_t s = next_byte << 2;
		// Six new bytes!
		res = (res << 6) + (s >> 2);
	}

	return res;
}

std::string fix_unicode_in_json(const std::string &json) {
	// This function is intended to fix UTF8 encoding in the given JSON
	// string. In a std::string, all characters are assumed to be 1 byte,
	// but in UTF8 characters can span any number of bytes between 1 and 6.
	// When libjson converts a std::string to JSON, it takes every byte
	// and transforms it into its own \uXXXX character, i.e. the character
	// \u0192 becomes \u00c6\u0092. This function fixes that.
	std::string fixed;
	bool escaped = false;
	for(size_t i = 0; i < json.length(); ++i) {
		unsigned char t = json[i];
		if(escaped) {
			escaped = false;
			if(t == 'u') { // beginning of Unicode character
				i--; // read_utf8_codepoint starts at \, we're already at u
				uint32_t cp_value = read_utf8_codepoint(json, i);
				i--; // read_utf8_codepoint ends at next char
				std::string hex_representation;
				std::stringstream ss;
				ss << std::hex << std::setw(4) << std::setfill('0') << cp_value;
				ss >> hex_representation;
				fixed += 'u';
				fixed += hex_representation;
			} else {
				fixed += t;
			}
		} else {
			fixed += t;
			if(t == '\\') {
				escaped = true;
			}
		}
	}
	return fixed;
}
