//Licence belong to C.S.Lee

#include "StringTokenizer.h"
#include <iostream>

StringTokenizer::StringTokenizer(string str)
: _body(str)
{
	_gotIdx = 0;
	_delim = string(" \t\f\n\r");

	_tokenize(0);
}

StringTokenizer::StringTokenizer(string str, string delim)
: _body(str), _delim(delim)
{
	_gotIdx = 0;

	_tokenize(0);
}

void StringTokenizer::_tokenize(int point)
{
	string::size_type startP = _body.find_first_not_of(_delim, point);
	string::size_type endP = _body.find_first_of(_delim, startP);

	while(startP != string::npos || endP != string::npos){
		_tokens.push_back(_body.substr(startP, endP - startP));
		_tokensPos.push_back(startP);

		startP = _body.find_first_not_of(_delim, endP);
		endP = _body.find_first_of(_delim, startP);
	}
}

int StringTokenizer::countTokens()
{
	return _tokens.size();
}

string StringTokenizer::nextToken()
{
	return _tokens[_gotIdx++];
}

string StringTokenizer::nextToken(string delim)
{
	_delim = delim;

	if(_gotIdx == 0){
		_tokens.clear();
		_tokensPos.clear();
		_tokenize(0);
	}else{
		while(_gotIdx != _tokens.size()){
			_tokens.pop_back();
			_tokensPos.pop_back();
		}
		_tokenize(_tokensPos[_gotIdx - 1] + _tokens[_gotIdx - 1].size());
	}

	return nextToken();
}


bool StringTokenizer::hasMoreTokens()
{
	if(_gotIdx < _tokens.size())
		return true;

	return false;
}

