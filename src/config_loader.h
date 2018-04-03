/*
List of configuration parameters for program

Format for parameters is

type (int, double or std::string),
name (will be under namespace config::),
default value

*/

#ifndef CONFIG_LOADER_H
#define CONFIG_LOADER_H

namespace config {

#define PARAMS(P)               \
P(int, width, 800)              \
P(int, height, 600)    

// Load configuration macro-file
#include "config_loader.inl"

}

#endif // CONFIG_LOADER_H