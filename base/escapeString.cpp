#include "escapeString.h"
#include <iostream>
#include <fstream>
#include <set>

std::string escape_string(const std::string& str)
{
    if(str.length() == 0)
        return ""; 

    char* new_str = (char *) malloc(2*str.length() + 1); 
	if(!new_str)
		return "";
    char* target = new_str;
	int len = str.length();
    for(auto it = str.begin(); it != str.end(); it++){
        switch (*it) {
            case '\0':
                *target++ = '\\';
                *target++ = '0';
				len ++;
                break;
            case '\'':
            case '\"':
            case '\\':
                *target++ = '\\';
				len++;
                /* break is missing intentionally */
            default:
                *target++ = *it;
                break;
        }   
    }   
    *target = 0;

    std::string res(new_str, len);
    free(new_str);
    return res;
}

void unescape_string(std::string& str)
{
    int l, len;
    l = len = str.length();
    int s = 0;
    int t = 0;
    while(l > 0) {
        if(str[t] == '\\') {
            t++;                /* skip the slash */
            len--;
            l--;
            if(l > 0) {
                if(str[t] == '0') {
                    str[s++] = '\0';
                    t++;
                } else {
                    str[s++] = str[t++];    /* preserve the next character */
                }
                l--;
            }
        } else{
            str[s++] = str[t++];
            l--;
        }
    }
    if (s != t) {
        str[s] = '\0';
    }
    str.resize(len);
}

