#pragma once

#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <cctype>
#include <exception>
#include "Token.h"

using namespace std;

namespace Perilla {
    
class Lexer
{
public:
    void Reset()
    {
        tokens.clear();
    }

    virtual char Next() = 0;
    virtual bool Eof() = 0;
    
    inline bool GetCurrent()
    {
        if (!Eof()) {
            current = Next();
            return true;
        }
        return false;
    }
    
    void Run()
    {
        GetCurrent();
        Parse();
    }

    void Parse()
    {
        if (Eof()) return;
        if (isspace(current)) {
            GetCurrent();
            Parse();
        } else if (isalpha(current)) {
            ParseIdent();
        } else if (isnumber(current)) {
            ParseNumber();
        } else if (current == '#') {
            ParseComment();
        } else {
            ParseUnknown();
        }
    }

    void ParseIdent()
    {
        if (Eof()) return;
        string buffer(1, current);
        while (!Eof() && GetCurrent() && isalnum(current)) {
            buffer += current;
        }

        if (buffer == "def") {
            tokens.push_back(make_shared<Token>(Token::Type::Def, "def"));
        } else if (buffer == "extern") {
            tokens.push_back(make_shared<Token>(Token::Type::Extern, "extern"));
        } else {
            tokens.push_back(make_shared<Token>(Token::Type::Ident, buffer));
        }
        
        Parse();
    }
    
    void ParseNumber()
    {
        // TODO validate number
        if (Eof()) return;
        string buffer(1, current);
        while (!Eof() && GetCurrent() && ValidInNumber(current)) {
            buffer += current;
        }
        
        tokens.push_back(make_shared<Token>(Token::Type::Number, buffer));
        Parse();
    }
    
    void ParseComment()
    {
        while (!Eof() && current != '\n') {
            GetCurrent();
        }
        
        GetCurrent();  // skip \n
        Parse();
    }
    
    void ParseUnknown()
    {
        if (Eof()) return;
        tokens.push_back(make_shared<Token>(current));
        GetCurrent();
        Parse();
    }
    
    void PrintTokens()
    {
        for (auto& token: tokens) {
            cout << *token << endl;
        }
    }
    
    vector<shared_ptr<Token>> getTokens() const
    {
        return tokens;
    }
    
private:
    inline bool ValidInNumber(char ch) const
    {
        return isnumber(ch) || ch == 'E' || ch == 'e' || ch == '+' || ch == '-' || ch == '.';
    }
    
    char current;
    vector<shared_ptr<Token>> tokens;
};

class StringLexer: public Lexer
{
public:
    StringLexer(string src): source(move(src)), pos(0) {}

    char Next() override
    {
        return source[pos++];
    }
    
    bool Eof() override
    {
        return pos >= source.size();
    }
    
private:
    string source;
    size_t pos;
};

};

