#pragma once

#include <jansson.h>
#include <string>

struct JSON {
	JSON(std::string j, size_t f);
	JSON(json_t *t);
	JSON();

	JSON(JSON const &o);
	JSON(JSON &&o);

	~JSON();

	void operator=(JSON const &o);
	void operator=(JSON &&o);

	json_t *get_json();
	json_t *object_get(std::string name);
	void object_set_new(std::string name, json_t *value);

	std::string toString();

private:
	json_t *json;
};
