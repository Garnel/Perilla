#pragma once

#include <unordered_map>

using namespace std;

namespace Perilla {
    class BinaryOperatorPrecedence {
    public:
        static int Get(char op) 
        {
            if (stdLut.find(op) != stdLut.end()) {
                return stdLut.at(op);
            } else if (userLut.find(op) != userLut.end()) {
                return userLut.at(op);
            } else {
                return 0;
            }
        }
        
        static void Set(char op, int precedence) {
            userLut[op] = precedence;
        }
        
        static bool Support(char op)
        {
            return stdLut.find(op) != stdLut.end() || userLut.find(op) != userLut.end();
        }

    private:
        static const unordered_map<char, int> stdLut;
        static unordered_map<char, int> userLut;
    };
    
    const unordered_map<char, int> BinaryOperatorPrecedence::stdLut = {
        {'<', 100},
        {'+', 200},
        {'-', 200},
        {'*', 400}
    };
    
    unordered_map<char, int> BinaryOperatorPrecedence::userLut = {};
}
