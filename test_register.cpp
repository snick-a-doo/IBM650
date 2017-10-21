#include <boost/test/unit_test.hpp>

#include "register.hpp"

using namespace IBM650;

struct Register_Fixture
{
    Register<2*digits_per_word> two_words;
    Word one_word = Word({1,0, 2,0,3,0, 4,0,5,0}, false);
    Register<5> half_word = Register<5>({2,1,0,9,8});
    Program_Register prog;
};

BOOST_AUTO_TEST_CASE(init)
{
    Register_Fixture f;
    BOOST_CHECK_EQUAL(f.one_word.value(), -1020304050);
    BOOST_CHECK_EQUAL(f.half_word.value(), 21098);
    BOOST_CHECK_EQUAL(f.prog.value(), 0);
}

BOOST_AUTO_TEST_CASE(set)
{
    Register_Fixture f;
    f.one_word = Word({2,4, 2,4,2,4, 2,4,2,4}, true);
    BOOST_CHECK_EQUAL(f.one_word.value(), 2424242424);
    f.one_word.reset();
    BOOST_CHECK_EQUAL(f.one_word.value(), 0);
    f.one_word = Word({9,9, 9,9,9,9, 9,9,9,9}, true);
    BOOST_CHECK_EQUAL(f.one_word.value(), 9999999999);
    f.one_word = Word({9,9, 9,9,9,9, 9,9,9,9}, false);
    BOOST_CHECK_EQUAL(f.one_word.value(), -9999999999);

    f.half_word = Register<5>({0,0,2,4,8}, false);
    BOOST_CHECK_EQUAL(f.half_word.value(), -248);
    f.half_word.reset();
    BOOST_CHECK_EQUAL(f.half_word.value(), 0);

    Register<5> other_half({1,0,1,0,1}, true);
    f.half_word = other_half;
    BOOST_CHECK_EQUAL(f.half_word.value(), other_half.value());
}

BOOST_AUTO_TEST_CASE(validate)
{
    Register_Fixture f;
    BOOST_CHECK(!f.one_word.is_blank());
    BOOST_CHECK(f.one_word.has_value());
    f.one_word.blank();
    BOOST_CHECK(f.one_word.is_blank());
    BOOST_CHECK(!f.one_word.has_value());
    BOOST_CHECK(Address().has_value());
}

struct Register_Output_Fixture : public Register_Fixture
{
    std::ostringstream os;
};

BOOST_AUTO_TEST_CASE(output)
{
    Register_Output_Fixture f1;
    f1.one_word.output(f1.os);
    BOOST_CHECK_EQUAL(f1.os.str(), "1020304050-");

    Register_Output_Fixture f2;
    f2.os << f2.half_word;
    BOOST_CHECK_EQUAL(f2.os.str(), "21098");

    Register_Output_Fixture f3;
    f3.os << f3.prog;
    BOOST_CHECK_EQUAL(f3.os.str(), "0000000000");
}

BOOST_AUTO_TEST_CASE(program_register)
{
    Program_Register prog({1,2, 3,4,5,6, 7,8,9,0});
    BOOST_CHECK_EQUAL(prog.opcode(), Opcode({1,2}));
    BOOST_CHECK_EQUAL(prog.data_address(), Address({3,4,5,6}));
    BOOST_CHECK_EQUAL(prog.instruction_address(), Address({7,8,9,0}));
}
