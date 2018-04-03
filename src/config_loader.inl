#ifdef PARAMS

#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>

namespace detail {

static void auto_parse(float &retval, std::string value) {
    retval = stof(value);
}

static void auto_parse(double &retval, std::string value) {
    retval = stod(value);
}

static void auto_parse(int &retval, std::string value) {
    retval = stoi(value);
}

static void auto_parse(bool &retval, std::string value) {
    retval = value[0] = 1 || value[0] == 'T' || value[0] == 't';
}

static void auto_parse(std::string &retval, std::string value) {
    value.erase(0, 1).pop_back();
    retval = value;
}

}

/**
 * If set to true will print additional information on load/save.
 */
static bool VERBOSE = false;

/**
 * If set to true will save all configuration values, otherwise will only save those that differ from the default values.
 */
static bool FULL_SAVE = false;

// Macro that loads field from line
#define LOAD_PARAM(type, name, value) {                                                 \
    std::string left = line.substr(0, line.find("="));                                  \
    left.erase(std::remove_if(left.begin(), left.end(), ::isspace), left.end());        \
    if (#name == left) {                                                                \
        std::string right = line.substr(line.find("="), line.length());                 \
        right.erase(std::remove_if(right.begin(), right.end(), ::isspace), right.end());\
        right.erase(0, 1);                                                              \
        detail::auto_parse( name , right);                                              \
        if (VERBOSE) std::cout << "\n" << "config::" << #name << " = " << name;         \
    }                                                                                   \
}

// Macro that writes field to line
#define SAVE_PARAM(type, name, value)                                                   \
if (name!=default_##name) {                                                             \
    file << #name << " = " << name << "\n";                                             \
    if (VERBOSE) std::cout << "\n" << "config::" << #name << " = " << name;             \
}

// Macro that adds the given value as a static field to the file
#define ADD_PARAM(type, name, value)        \
    static type name = value;               \
    static const type default_##name = value;

// Adds all configuration parameters to the file
PARAMS(ADD_PARAM)

/**
 * Function for loading predefined configuration values.
 * See config_loader.iml to add values
 */
static void load(std::string path) {
    std::string line;
    std::ifstream file(path);
    if (file.is_open()){
        if (VERBOSE) std::cout << "Loading configuration file " << path;
        while(getline(file,line)) {
            if (line[0] == '#') continue;
            PARAMS(LOAD_PARAM)
        }
        file.close();
        if (VERBOSE) std::cout << std::endl;
    }
    else std::cout << "Unable to open file" << std::endl;; 
    
}

/**
 * Function for saving predefined configuration values.
 * See config_loader.iml to add values
 */
static void save(std::string path) {
    std::ofstream file(path);
    if (file.is_open()) {
        if (VERBOSE) std::cout << "Saving configuration file as " << path;
        PARAMS(SAVE_PARAM)
        file.close();
        if (VERBOSE) std::cout << std::endl;
    }
    else std::cout << "Unable to open file " << path;
    
}

#undef PARAMS
#endif // PARAMS

#undef ADD_PARAM
#undef SAVE_PARAM
#undef LOAD_PARAM

