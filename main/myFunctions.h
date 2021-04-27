#ifndef MY_FUNCTIONS_H
#define MY_FUNCTIONS_H

#ifdef __cplusplus
extern "C"{
#endif



#include <string>
/** To more easily transfer form Arduino code to C/Cpp, Arduino String function replacements are below **/
std::string trim(const std::string& str,
                 const std::string& whitespace = " \t")
{
    const auto strBegin = str.find_first_not_of(whitespace);
    if (strBegin == std::string::npos)
        return ""; // no content

    const auto strEnd = str.find_last_not_of(whitespace);
    const auto strRange = strEnd - strBegin + 1;

    return str.substr(strBegin, strRange);
}
bool startsWith(std::string str, std::string condition){
    char temp_ch[32]={};
    strcpy(temp_ch, str.c_str()); 
    char t[32]={};
    strcpy(t, strtok(temp_ch," "));
    return condition.compare(t);
}
// Converting char arrays to uint8_t
uint8_t* ch_to_u8t(char* in){
  
    size_t length = strlen(in) + 1;

    const char* beg = in;
    const char* end = in + length;

    uint8_t* msg2 = new uint8_t[length];

    size_t i = 0;
    for (; beg != end; ++beg, ++i)
    {
        msg2[i] = (uint8_t)(*beg);
    }
    return msg2;
}

#ifdef __cplusplus
}
#endif

#endif