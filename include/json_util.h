#ifndef ATG_ENGINE_SIM_JSON_UTIL_H
#define ATG_ENGINE_SIM_JSON_UTIL_H

#include <string>
#include <map>

// Dependency-free reader/writer for a single flat JSON object of string keys
// to string values, e.g. {"ignition": "A", "starter": "S"}. This is not a
// general-purpose JSON library -- no nesting, no numbers/booleans/arrays --
// it covers exactly the shape keybinds.json needs.
namespace json_util {
    bool writeFlatStringMap(
        const std::string &path,
        const std::map<std::string, std::string> &values);

    bool readFlatStringMap(
        const std::string &path,
        std::map<std::string, std::string> *out);
}

#endif /* ATG_ENGINE_SIM_JSON_UTIL_H */
