#ifndef DEBUG_H
#define DEBUG_H

#include "globals.h"
#include <string.h>
#include <string>
#include "stdio.h"


void debugSetup();
void debugClose(); 

void debugPrint(double str);
void debugPrint(int i);
void debugPrint(std::string str);
void debugPrint(std::string * str);
void debugPrint(const char* str);

void debugPrintln(double str);
void debugPrintln(int i);
void debugPrintln(std::string str);
void debugPrintln(std::string * str);
void debugPrintln(const char* str);

#endif