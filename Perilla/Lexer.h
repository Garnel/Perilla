#pragma once

#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <deque>
#include <cctype>
#include <exception>
#include "Token.h"

using namespace std;

namespace Perilla {
    
class Lexer
{
public:
    Lexer(): current() {}
    virtual ~Lexer() = default;
    
    void Reset()
    {
        tokens.clear();
    }

    virtual char Next() = 0;
    virtual bool Eof() const = 0;

    Token NextToken()
    {
        if (!current.valid) {
            if (!GetCurrent()) {
                return Token::EofToken;
            }
        }
        
        while (current.valid && tokens.empty()) {
            Parse();
        }
        
        if (tokens.empty()) {
            return Token::EofToken;
        }

        Token front = tokens.front();
        tokens.pop_front();
        return front;
    }

private:
    struct CharRef
    {
        bool valid;
        size_t line;
        size_t column;
        char current;
        
        CharRef(): valid(false) {}

        CharRef& operator=(char ch)
        {
            current = ch;
            if (!valid) {
                line = 1;
                column = 1;
                valid = true;
            } else if (ch == '\n') {
                line++;
                column = 1;
            } else {
                column++;
            }
            
            return *this;
        }
        
        bool operator==(char ch)
        {
            return valid && current == ch;
        }
        
        bool operator==(const CharRef& rhs)
        {
            return valid && rhs.valid && current == rhs.current &&
                    line == rhs.line && column == rhs.column;
        }
        
        operator char() const
        {
            return current;
        }
        
        void Reset()
        {
            valid = false;
        }
    };

    inline bool GetCurrent()
    {
        if (!Eof()) {
            current = Next();
            return true;
        }
        current.Reset();
        return false;
    }
    
    void Parse()
    {
        while (isspace(current)) {
            GetCurrent();
        }

        if (isalpha(current)) {
            ParseIdent();
        } else if (current == '-' || isnumber(current)) {
            ParseNumber();
        } else if (current == '#') {
            ParseComment();
        } else {
            ParseUnknown();
        }
    }
    
    void ParseIdent()
    {
        string buffer(1, current);
        while (!Eof() && GetCurrent() && isalnum(current)) {
            buffer += current;
        }
        
        if (buffer == "def") {
            tokens.push_back(Token::DefToken);
        } else if (buffer == "extern") {
            tokens.push_back(Token::ExternToken);
        } else {
            tokens.push_back(Token{Token::Type::Ident, buffer});
        }
    }
    
    void ParseNumber()
    {
        char previous = current;
        if (!GetCurrent() || !isdigit(current)) {
            if (isdigit(previous)) {
                tokens.push_back(Token{Token::Type::Number, string(1, previous)});
            } else {
                // '-' not part of the number
                tokens.push_back(Token {previous});
            }
            return;
        }
        
        //    +   -   .  e/E  0  1-9
        static const short transformTable[][6] = {
            {-1,  1, -1, -1,  2,  3},  // 0: initial
            {-1, -1, -1, -1,  2,  3},  // 1: just after the leading '-' in negtive number
            {-1, -1,  4,  5, -1, -1},  // 2: terminal, just after the leading '0'
            {-1, -1,  4,  5,  3,  3},  // 3: terminal, in integer part
            {-1, -1, -1, -1,  6,  6},  // 4: just after the '.' in floating number
            { 7,  7, -1, -1,  8,  8},  // 5: just after the 'e/E'
            {-1, -1, -1,  5,  6,  6},  // 6: terminal, in fractional part
            {-1, -1, -1, -1,  8,  8},  // 7: just after '+/-' in exponential part
            {-1, -1, -1, -1,  8,  8}   // 8: terminal, in exponential part
        };

        string buffer(1, previous);
        short state = 0; // initial state
        while (CurrentValidInNumber()) {
            int inputType = -1;
            switch (current) {
                case '+': inputType = 0; break;
                case '-': inputType = 1; break;
                case '.': inputType = 2; break;
                case 'e':
                case 'E': inputType = 3; break;
                case '0': inputType = 4; break;
                default: inputType = 5; break;
            }
            state = transformTable[state][inputType];
            if (state == -1) { // valid number
                break;
            }
            
            buffer += current;
            GetCurrent();
        }

//        if (state == -1 || state == 0 || state == 1 || state == 4 || state == 5 || state == 7) {
//            // TODO error number
//            HandlerError("invalid number");
//        }

        tokens.push_back(Token{Token::Type::Number, buffer});
    }
    
    void ParseComment()
    {
        // only support single line comment
        while (!Eof() && current != '\n') {
            GetCurrent();
        }
        
        GetCurrent();  // skip \n
        Parse();
    }
    
    void ParseUnknown()
    {
        tokens.push_back(Token{current});
        GetCurrent(); // skip the character
    }
    
    void HandlerError(string errorMessage)
    {
        cout << "at line " << current.line << " col " << current.column << ": "
            << errorMessage << endl;
    }
    
    inline bool CurrentValidInNumber() const {
        return  (current == '+' || current == '-' || current == '.' ||
                 current == 'e' || current == 'E' || std::isdigit(current) != 0);
    }

    CharRef current;
    deque<Token> tokens;
};

class StringLexer: public Lexer
{
public:
    StringLexer(string src): source(move(src)), pos(0) {}
    virtual ~StringLexer() = default;

    inline char Next() override
    {
        return source[pos++];
    }
    
    inline bool Eof() const override
    {
        return pos >= source.size();
    }
    
private:
    string source;
    size_t pos;
};

};

