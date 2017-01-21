#include "jsonwrap.h"

JSON::JSON(std::string j, size_t f) {
	json_error_t error;
	json = json_loads(j.c_str(), f, &error);
	if(!json) {
		throw std::runtime_error("Failed to load JSON: " + std::string(error.text));
	}
}

JSON::JSON(json_t *t) : json(t) {}

JSON::JSON(JSON const &o) : json(nullptr) {
	*this = o;
}

JSON::JSON() : json(nullptr) {}

JSON::JSON(JSON &&o) {
	*this = std::move(o);
}

JSON::~JSON() {
	if(json) {
		json_decref(json);
	}
}

void JSON::operator=(JSON const &o) {
	if(json) {
		json_decref(json);
	}
	if(o.json) {
		json = json_incref(o.json);
	}
}

void JSON::operator=(JSON &&o) {
	json_t *j = o.json;
	o.json = json;
	json = j;
}

json_t *JSON::object_get(std::string name) {
	return json_object_get(json, name.c_str());
}

std::string JSON::toString() {
	char *raw_json = json_dumps(json, 0);
	std::string jsonMsg = raw_json;
	free(raw_json);
	return jsonMsg;
}

void JSON::object_set_new(std::string name, json_t *value) {
	json_object_set_new(json, name.c_str(), value);
}

json_t *JSON::get_json() {
	return json;
}
