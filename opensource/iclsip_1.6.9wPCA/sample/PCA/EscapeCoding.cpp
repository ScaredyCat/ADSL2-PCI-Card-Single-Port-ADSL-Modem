#include <sstream>
#include <iostream>
#include <string>
#include <vector>

#pragma warning(disable:4786)	// disable VC warnings about STL

char HexValue(char cHex)
{
	if (cHex >= '0' && cHex <= '9')
		return cHex - '0';
	else if (cHex >= 'a' && cHex <= 'f')
		return cHex - 'a' + 10;
	else if (cHex >= 'A' && cHex <= 'F')
		return cHex - 'A' + 10;
	else
		{
		}
}


char HexCode(unsigned char iOct)
{
	if (iOct >= 0 && iOct <= 9)
		return '0' + iOct;
	else if (iOct >= 10 && iOct <= 15)
		return 'A' + iOct - 10;
	else
		{
		}
}

const std::string Unescape(const std::string& strEscaped) 
{
	std::stringstream ssUnescaped;

	for (int i = 0, len = strEscaped.length(); i < len; i++) {
		const char& cEscaped = strEscaped[i];
		if (cEscaped != '%')
			ssUnescaped << cEscaped;
		else {
			ssUnescaped << char(HexValue(strEscaped[i + 1]) << 4
			            | HexValue(strEscaped[i + 2]));
			i += 2;
		}
	}

	return ssUnescaped.str();
}


const std::string Escape(const std::string& strUnescaped) 
{
	std::stringstream ssEscaped;

	for (int i = 0, len = strUnescaped.length(); i < len; i++) {
		const char& cUnescaped = strUnescaped[i];

		// check if this is a two bytes word, ignore it.
		int value = (int)cUnescaped;
		if (value < 0) {
			ssEscaped << cUnescaped << strUnescaped[i+1];
			i++;
			continue;
		}

		switch (cUnescaped)
                {
		case '[': // avoid conflicting with SIP URI
		case ']':
		case '\"':
		case ' ':
		case '<':
		case '>':
			ssEscaped << '%' << HexCode(cUnescaped >> 4) << HexCode(cUnescaped & 0xF);
			break;

		default:
			ssEscaped << cUnescaped;
			break;
                }
        }

        return ssEscaped.str();
}
