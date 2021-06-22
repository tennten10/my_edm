

#include "globals.h"
#include "debug.h"
std::string unitsToString(Units u)
{
    switch (u)
    {
    case g:
        return std::string("g");
        break;
    case kg:
        return std::string("kg");
        break;
    case oz:
        return std::string("oz");
        break;
    case lb:
        return std::string("lb");
        break;
    default:
        return std::string("err");
        break;
    }
}

Units stringToUnits(std::string v)
{
    Units retVal = err;
    if (v.compare("g") == 0)
    {
        debugPrintln("stringtoUnits g");
        retVal = g;
    }
    else if (v.compare("kg") == 0)
    {
        retVal = kg;
        debugPrintln("stringtoUnits kg");
    }
    else if (v.compare("oz") == 0)
    {
        retVal = oz;
    }
    else if (v.compare("lb") == 0)
    {
        retVal = lb;
    }
    return retVal;
    // g, kg, oz, lb
}