#ifndef DEBUG_H
#define DEBUG_H

#include "globals.h"
//#include <string>
#include <string>
#include <cstdio>


void debugSetup();
void debugClose(); 

void debugPrint(double str);
void debugPrint(int i);
void debugPrint(std::string str);
void debugPrint(std::string * str);
void debugPrint(const char* str);
void debugPrint(char * const str);

void debugPrintln(double str);
void debugPrintln(int i);
void debugPrintln(std::string str);
void debugPrintln(std::string * str);
void debugPrintln(const char* str);
void debugPrintln(char * const str);

#endif