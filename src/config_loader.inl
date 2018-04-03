/*
List of configuration parameters for program

Format for parameters is

type (int, double or std::string),
name (will be under namespace config::),
default value

*/

#define PARAMS(P)               \
P(int, width, 800)              \
P(int, height, 600)    
