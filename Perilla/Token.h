#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <cassert>

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
        Unknown,
        Eof
    };
    
    Token(Type type) noexcept
    : type(type)
    {
        assert(type == Eof);
    }

    Token(Type type, string cont) noexcept
    : type(type), content(move(cont))
    {
        if (type == Number) {
            numericValue = stod(content);
        } else if (type == Unknown) {
            character = content[0];
        }
    }

    Token(char ch) noexcept
    : type(Unknown), content{1, ch}, character(ch) {}
    
    Token(const Token& token) noexcept
    {
        type = token.type;
        content = token.content;
        numericValue = token.numericValue;
        character = token.character;
    }
    
    Token(Token&& token) noexcept
    {
        type = token.type;
        swap(content, token.content);
        numericValue = token.numericValue;
        character = token.character;
    }
    
    ~Token() = default;
    
    Token& operator=(const Token& token) noexcept
    {
        type = token.type;
        content = token.content;
        numericValue = token.numericValue;
        character = token.character;
        return *this;
    }
    
    Token& operator=(Token&& token) noexcept
    {
        type = token.type;
        swap(content, token.content);
        numericValue = token.numericValue;
        character = token.character;
        return *this;
    }
    
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
    
    inline bool IsEof() const
    {
        return type == Eof;
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
    
    bool operator==(const Token& rhs) const
    {
        if (type != rhs.type) {
            return false;
        }
        
        if (type == Def || type == Extern || type == Eof) {
            return true;
        }
        
        if (type == Unknown) {
            return character == rhs.character;
        }
        return content == rhs.content;
    }
    
    bool operator!=(const Token& rhs) const
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
            case Token::Eof:
                buffer << "eof";
                break;
        }
        return buffer.str();
    }
    
    friend ostream& operator<<(ostream& out, Token& token);
    
    static const Token DefToken;
    static const Token ExternToken;
    static const Token EofToken;

private:
    Type type;
    string content;
    double numericValue;
    char character;
};
    
const Token Token::DefToken = {Token::Type::Def, "def"};
const Token Token::ExternToken = {Token::Type::Extern, "extern"};
const Token Token::EofToken = {Token::Type::Eof};

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
        case Token::Eof:
            out << "eof";
            break;
    }
    return out;
}

};
