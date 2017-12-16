#include "register.hpp"

#include <algorithm>

using namespace IBM650;

TDigit IBM650::bin(TDigit number)
{
    return number == '_' ? 0
        : bi_quinary_code[number == '-' ? 8
                          : number == '+' ? 9
                          : number];
}

TDigit IBM650::dec(TDigit code)
{
    auto it = std::find(bi_quinary_code.begin(), bi_quinary_code.end(), code);
    return code == 0 ? '_'
        : it == bi_quinary_code.end() ? '?'
        : std::distance(bi_quinary_code.begin(), it);
}
