#pragma once

#include <iostream>
#include <string>
#include <sstream>

using namespace std;

namespace Perilla {

class Token
{
public:
    enum Type {
        Def,
        Extern,
        Ident,
        Number,
        Unknown
    };

    Token(Type type, string cont): type(type), content(move(cont))
    {
        if (type == Number) {
            numericValue = stod(content);
        } else if (type == Unknown) {
            character = content[0];
        }
    }
    
    Token(char ch): type(Unknown), content{1, ch}, character(ch) {}
    
    inline bool IsDef() const
    {
        return type == Def;
    }
    
    inline bool IsExtern() const
    {
        return type == Extern;
    }
    
    inline bool IsIdent() const
    {
        return type == Ident;
    }
    
    inline bool IsNumber() const
    {
        return type == Number;
    }
    
    inline bool IsUnknown() const
    {
        return type == Unknown;
    }
    
    Type GetType() const
    {
        return type;
    }
    
    string GetContent() const
    {
        return content;
    }
    
    double GetNumeric() const
    {
        return numericValue;
    }
    
    char GetChar() const
    {
        return character;
    }
    
    bool operator==(const Token& rhs)
    {
        if (type != rhs.type) {
            return false;
        }
        
        if (type == Unknown) {
            return character == rhs.character;
        }
        return content == rhs.content;
    }
    
    bool operator!=(const Token& rhs)
    {
        return !operator==(rhs);
    }
    
    string GetString() const
    {
        stringstream buffer;
        switch (type) {
            case Token::Def:
                buffer << "def";
                break;
            case Token::Extern:
                buffer << "extern";
                break;
            case Token::Ident:
                buffer << "id " << content;
                break;
            case Token::Number:
                buffer << "number " << content;
                break;
            case Token::Unknown:
                buffer << "unknown " << character;
                break;
        }
        return buffer.str();
    }
    
    friend ostream& operator<<(ostream& out, Token& token);

private:
    Type type;
    string content;
    double numericValue;
    char character;
};
    
ostream& operator<<(ostream& out, Token& token)
{
    switch (token.GetType()) {
        case Token::Def:
            out << "def";
            break;
        case Token::Extern:
            out << "extern";
            break;
        case Token::Ident:
            out << "id " << token.content;
            break;
        case Token::Number:
            out << "number " << token.content;
            break;
        case Token::Unknown:
            out << "unknown " << token.character;
            break;
    }
    return out;
}

};
