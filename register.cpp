#include "register.hpp"

#include <algorithm>

using namespace IBM650;

char IBM650::bin(char number)
{
    return number == '_' ? 0
        : bi_quinary_code[number == '-' ? 8
                          : number == '+' ? 9
                          : number];
}

char IBM650::dec(char code)
{
    char number = std::find(bi_quinary_code.begin(), bi_quinary_code.end(), code)
        - bi_quinary_code.begin();
    return number;
}
