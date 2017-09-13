#ifndef _ESCAPE_CODING
#define _ESCAPE_CODING

#include <string>
#include <sstream>

const std::string Unescape(const std::string& strEscaped);
const std::string Escape(const std::string& strUnescaped);

#endif