#include "register.hpp"
#include "doctest.h"
#include <sstream>

using namespace IBM650;

TEST_CASE("convert to and from bi-quinary codes")
{
    CHECK(bin(3) == '(');
    CHECK(bin(6) == 'B');
    CHECK(bin(9) == 'P');
    CHECK(bin(8) == 'H');
    CHECK(bin('+') == 'P');  // 'P' is the b-q code for 9
    CHECK(bin('-') == 'H');  // 'H' is the b-q code for 8
    CHECK(bin('_') == '\0'); // '_' means no bits set
    CHECK(dec('(') == 3);
    CHECK(dec('B') == 6);
    CHECK(dec('P') == 9);
    CHECK(dec('H') == 8);
    CHECK(dec('\0') == '_');
    CHECK(dec('Q') == '?');
}

TEST_CASE("register operations")
{
    Register<2*word_size> two_words;
    Word one_word({1,0, 2,0,3,0, 4,0,5,0, '-'});
    Register<5> half_word({2,1,0,9,8});
    UWord unsigned_word;

    SUBCASE("initial values")
    {
        CHECK(one_word.value() == 10203040508);
        CHECK(half_word.value() == 21098);
        CHECK(zero.value() == 9); // Just a plus sign.
    }

    SUBCASE("values after assignment")
    {
        one_word = Word({2,4, 2,4,2,4, 2,4,2,4, '+'});
        // Signed values include the sign digit in the LSD.
        CHECK(one_word.value() == 24242424249);
        one_word = Word({9,9, 9,9,9,9, 9,9,9,9, '+'});
        CHECK(one_word.value() == 99999999999);
        one_word = Word({9,9, 9,9,9,9, 9,9,9,9, '-'});
        CHECK(one_word.value() == 99999999998);
        half_word = Register<5>({0,0,2,4,8});
        CHECK(half_word.value() == 248);

        Register<5> other_half({1,0,1,0,1});
        half_word = other_half;
        CHECK(half_word.value() == other_half.value());
    }

    SUBCASE("is blank, is number")
    {
        CHECK(!one_word.is_blank());
        CHECK(one_word.is_number());
        one_word.clear();
        CHECK(one_word.is_blank());
        CHECK(!one_word.is_number());
    }

    SUBCASE("output to stream")
    {
        std::ostringstream os;
        SUBCASE("") {
            os << one_word;
            CHECK(os.str() == "10203040508");
        }
        SUBCASE("") {
            os << half_word;
            CHECK(os.str() == "21098");
        }
        SUBCASE("") {
            os << unsigned_word;
            CHECK(os.str() == "__________");
        }
    }

    SUBCASE("indexing of registers and digits()")
    {
        // Register indices go from LSD (or sign if signed) to MSB
        CHECK(one_word[0] == bin('-'));
        CHECK(one_word[2] == bin(5));
        CHECK(one_word[10] == bin(1));
        // The digits array goes from MSD to LSD: the order needed for display.
        auto digits = one_word.digits();
        CHECK(digits[0] == bin(1));
        CHECK(digits[2] == bin(2));
        CHECK(digits[10] == bin('-'));
    }
}
