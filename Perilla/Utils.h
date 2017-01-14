#pragma once

namespace Perilla {
    string GenerateRandom(size_t len) {
        static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
        
        string res(len, ' ');
        for (size_t i = 0; i < len; ++i) {
            res[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
        }
        return res;
    }
}
