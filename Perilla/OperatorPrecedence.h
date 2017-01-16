#pragma once

#include <unordered_map>

using namespace std;

namespace Perilla {
    class BinaryOperatorPrecedence {
    public:
        static int Get(char op) 
        {
            if (lut.find(op) != lut.end()) {
                return lut.at(op);
            } else {
                return -1;
            }
        }
        
        static bool Support(char op)
        {
            return lut.find(op) != lut.end();
        }

    private:
        static const unordered_map<char, int> lut;
    };
    
    const unordered_map<char, int> BinaryOperatorPrecedence::lut = {
        {'<', 100},
        {'+', 200},
        {'-', 200},
        {'*', 400}
    };
}
