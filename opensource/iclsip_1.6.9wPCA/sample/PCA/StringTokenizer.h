#include <sstream>
#include <vector>

using namespace std;

class StringTokenizer
{
private:
	vector<string> _tokens;
	vector<string::size_type> _tokensPos;
	string _body;
	string _delim;
	
	string::size_type _gotIdx; // how much token got;

	void _tokenize(int);
public:
	StringTokenizer(string str);
	StringTokenizer(string str, string delim);

	int countTokens();
	bool hasMoreTokens();
	string nextToken();
	string nextToken(string delim);
};
