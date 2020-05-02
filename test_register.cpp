#include <boost/test/unit_test.hpp>

#include "register.hpp"

using namespace IBM650;

struct Register_Fixture
{
    Register<2*word_size> two_words;
    Word one_word = Word({1,0, 2,0,3,0, 4,0,5,0, '-'});
    Register<5> half_word = Register<5>({2,1,0,9,8});
    UWord prog;
};

BOOST_AUTO_TEST_CASE(init)
{
    Register_Fixture f;
    BOOST_CHECK_EQUAL(f.one_word.value(), 10203040508);
    BOOST_CHECK_EQUAL(f.half_word.value(), 21098);
    BOOST_CHECK_EQUAL(zero.value(), 9); // Just a plus sign.
}

BOOST_AUTO_TEST_CASE(set)
{
    Register_Fixture f;
    f.one_word = Word({2,4, 2,4,2,4, 2,4,2,4, '+'});
    // Signed values include the sign digit in the LSD.
    BOOST_CHECK_EQUAL(f.one_word.value(), 24242424249);
    f.one_word = Word({9,9, 9,9,9,9, 9,9,9,9, '+'});
    BOOST_CHECK_EQUAL(f.one_word.value(), 99999999999);
    f.one_word = Word({9,9, 9,9,9,9, 9,9,9,9, '-'});
    BOOST_CHECK_EQUAL(f.one_word.value(), 99999999998);
    f.half_word = Register<5>({0,0,2,4,8});
    BOOST_CHECK_EQUAL(f.half_word.value(), 248);

    Register<5> other_half({1,0,1,0,1});
    f.half_word = other_half;
    BOOST_CHECK_EQUAL(f.half_word.value(), other_half.value());
}

BOOST_AUTO_TEST_CASE(validate)
{
    Register_Fixture f;
    BOOST_CHECK(!f.one_word.is_blank());
    BOOST_CHECK(f.one_word.is_number());
    f.one_word.clear();
    BOOST_CHECK(f.one_word.is_blank());
    BOOST_CHECK(!f.one_word.is_number());
}

struct Register_Output_Fixture : public Register_Fixture
{
    std::ostringstream os;
};

BOOST_AUTO_TEST_CASE(output)
{
    Register_Output_Fixture f1;
    f1.os << f1.one_word;
    BOOST_CHECK_EQUAL(f1.os.str(), "10203040508");

    Register_Output_Fixture f2;
    f2.os << f2.half_word;
    BOOST_CHECK_EQUAL(f2.os.str(), "21098");

    Register_Output_Fixture f3;
    f3.os << f3.prog;
    BOOST_CHECK_EQUAL(f3.os.str(), "__________");
}

BOOST_AUTO_TEST_CASE(test_index)
{
    Register_Fixture f;
    // Register indices go from LSD (or sign if signed) to MSB
    BOOST_CHECK_EQUAL(f.one_word[0], bin('-'));
    BOOST_CHECK_EQUAL(f.one_word[2], bin(5));
    BOOST_CHECK_EQUAL(f.one_word[10], bin(1));
    // The digits array goes from MSD to LSD: the order needed for display.
    auto digits = f.one_word.digits();
    BOOST_CHECK_EQUAL(digits[0], bin(1));
    BOOST_CHECK_EQUAL(digits[2], bin(2));
    BOOST_CHECK_EQUAL(digits[10], bin('-'));
}

BOOST_AUTO_TEST_CASE(convert)
{
    BOOST_CHECK_EQUAL(bin(3), '(');
    BOOST_CHECK_EQUAL(bin(6), 'B');
    BOOST_CHECK_EQUAL(bin(9), 'P');
    BOOST_CHECK_EQUAL(bin(8), 'H');
    BOOST_CHECK_EQUAL(bin('+'), 'P');  // 'P' is the b-q code for 9
    BOOST_CHECK_EQUAL(bin('-'), 'H');  // 'H' is the b-q code for 8
    BOOST_CHECK_EQUAL(bin('_'), '\0'); // '_' means no bits set
    BOOST_CHECK_EQUAL(dec('('), 3);
    BOOST_CHECK_EQUAL(dec('B'), 6);
    BOOST_CHECK_EQUAL(dec('P'), 9);
    BOOST_CHECK_EQUAL(dec('H'), 8);
    BOOST_CHECK_EQUAL(dec('\0'), '_');
    BOOST_CHECK_EQUAL(dec('Q'), '?');
}
