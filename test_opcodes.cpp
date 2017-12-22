#include <boost/test/unit_test.hpp>

#include "computer.hpp"
#include "test_fixture.hpp"

using namespace IBM650;

struct Opcode_Fixture : Computer_Ready_Fixture
{
    Opcode_Fixture(int opcode,
                   const Word& data,
                   const Address& addr,
                   const Word& upper,
                   const Word& lower,
                   const Word& distr)
        {
            assert(addr.value() > 999);
            Address next({0,0,2,0});
            Word instr;
            instr.digits()[0] = bin(opcode / 10);
            instr.digits()[1] = bin(opcode % 10);
            instr.load(addr, 0, 2);
            instr.load(next, 0, 6);

            computer.set_drum(Address({0,0,1,0}), instr);
            computer.set_drum(next, Word({0,1, 0,0,0,0, 0,0,0,0}));
            if (addr.value() < 2000)
                computer.set_drum(addr, data);
            computer.set_distributor(distr);
            computer.set_upper(upper);
            computer.set_lower(lower);
            computer.set_storage_entry(Word({0,0, 0,0,0,0, 0,0,1,0, '+'}));

            computer.set_programmed_mode(Computer::Programmed_Mode::stop);
            computer.set_display_mode(Computer::Display_Mode::distributor);
            computer.set_control_mode(Computer::Control_Mode::run);
            computer.set_error_mode(Computer::Error_Mode::stop);
            computer.program_reset();
            computer.program_start();
        }
    Opcode_Fixture(int opcode, const Address& addr, const Word& distr)
        : Opcode_Fixture(opcode, Word(), addr, Word(), Word(), distr)
        {}

    Word distributor() {
        computer.set_display_mode(Computer::Display_Mode::distributor);
        return computer.display();
    }
    Word upper() {
        computer.set_display_mode(Computer::Display_Mode::upper_accumulator);
        return computer.display();
    }
    Word lower() {
        computer.set_display_mode(Computer::Display_Mode::lower_accumulator);
        return computer.display();
    }
    Word drum(const Address& addr) {
        return computer.get_drum(addr);
    }
};

// 69  LD  Load Distributor
BOOST_AUTO_TEST_CASE(load_distributor_drum)
{
    Word data({0,0, 0,0,1,2, 3,4,5,6, '+'});
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word lower({0,0, 0,0,6,4, 3,2,1,7, '+'});
    Word distr(lower);

    Opcode_Fixture f(69, data, addr, upper, lower, distr);
    BOOST_CHECK_EQUAL(f.distributor(), data);
    BOOST_CHECK_EQUAL(f.upper(), upper);
    BOOST_CHECK_EQUAL(f.lower(), lower);
}

BOOST_AUTO_TEST_CASE(load_distributor_upper)
{
    Word data({0,0, 0,0,1,2, 3,4,5,6, '+'});
    Address addr({8,0,0,3});
    Word upper({1,2, 3,4,5,6, 7,8,9,0, '+'});
    Word lower({3,8, 3,8,5,6, 7,8,9,0, '+'});
    Word distr(lower);
    Word new_distr(upper);
    new_distr.digits().back() = bin('+');

    Opcode_Fixture f(69, data, addr, upper, lower, distr);
    BOOST_CHECK_EQUAL(f.distributor(), new_distr);
    BOOST_CHECK_EQUAL(f.upper(), upper);
    BOOST_CHECK_EQUAL(f.lower(), lower);
}

BOOST_AUTO_TEST_CASE(load_distributor_lower)
{
    Word data({0,0, 0,0,1,2, 3,4,5,6, '+'});
    Address addr({8,0,0,2});
    Word upper({1,2, 3,4,5,6, 7,8,9,0, '-'});
    Word lower({3,8, 3,8,5,6, 7,8,9,0, '-'});
    Word distr(lower);

    Opcode_Fixture f(69, data, addr, upper, lower, distr);
    BOOST_CHECK_EQUAL(f.distributor(), lower);
    BOOST_CHECK_EQUAL(f.upper(), upper);
    BOOST_CHECK_EQUAL(f.lower(), lower);
}

// 24  STD  Store Distributor
BOOST_AUTO_TEST_CASE(store_distributor)
{
    Address addr({1,0,1,0});
    Word distr({0,0, 0,1,0,4, 2,6,6,6, '-'});

    Opcode_Fixture f(24, addr, distr);
    BOOST_CHECK_EQUAL(f.distributor(), distr);
    BOOST_CHECK_EQUAL(f.drum(addr), distr);
}

BOOST_AUTO_TEST_CASE(store_distributor_800x)
{
    Word distr({0,0, 0,1,0,4, 2,6,6,6, '-'});

    { Opcode_Fixture f(24, Address({2,0,0,0}), distr);
        BOOST_CHECK(f.computer.storage_selection_error()); }
    { Opcode_Fixture f(24, Address({8,0,0,3}), distr);
        BOOST_CHECK(f.computer.storage_selection_error()); }
    { Opcode_Fixture f(24, Address({8,0,0,1}), distr);
        BOOST_CHECK(f.computer.storage_selection_error()); }
    { Opcode_Fixture f(24, Address({8,0,0,2}), distr);
        BOOST_CHECK(f.computer.storage_selection_error()); }
    { Opcode_Fixture f(24, Address({8,0,0,3}), distr);
        BOOST_CHECK(f.computer.storage_selection_error()); }
}

// 10  AU  Add to Upper
BOOST_AUTO_TEST_CASE(add_to_upper_1)
{
    Word data({0,0, 1,2,3,4, 5,6,7,8, '+'});
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,4,5, 8,6,3,2, '+'});
    Word lower({8,9, 4,2,7,1, 1,3,6,5, '+'});
    Word distr(lower);
    Word sum({0,0, 1,2,8,0, 4,3,1,0, '+'});

    Opcode_Fixture f(10, data, addr, upper, lower, distr);
    BOOST_CHECK_EQUAL(f.distributor(), data);
    BOOST_CHECK_EQUAL(f.upper(), sum);
    BOOST_CHECK_EQUAL(f.lower(), lower);
    BOOST_CHECK(!f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(add_to_upper_2)
{
    Word data({0,0, 1,2,3,4, 5,6,7,8, '+'});
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,4,5, 8,6,3,2, '-'});
    Word lower({8,9, 4,2,7,1, 1,3,6,5, '-'});
    Word distr(lower);
    Word upper_sum({0,0, 1,1,8,8, 7,0,4,5, '+'});
    Word lower_sum({1,0, 5,7,2,8, 8,6,3,5, '+'});

    Opcode_Fixture f(10, data, addr, upper, lower, distr);
    BOOST_CHECK_EQUAL(f.distributor(), data);
    BOOST_CHECK_EQUAL(f.upper(), upper_sum);
    BOOST_CHECK_EQUAL(f.lower(), lower_sum);
    BOOST_CHECK(!f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(add_to_upper_3)
{
    Word data({0,0, 1,2,3,4, 5,6,7,8, '+'});
    Address addr({1,0,0,0});
    Word upper({9,9, 8,9,3,7, 4,6,2,7, '+'});
    Word lower({8,9, 4,2,7,1, 1,3,6,5, '+'});
    Word distr(lower);
    Word sum({0,0, 0,1,7,2, 0,3,0,5, '+'});

    Opcode_Fixture f(10, data, addr, upper, lower, distr);
    BOOST_CHECK_EQUAL(f.distributor(), data);
    BOOST_CHECK_EQUAL(f.upper(), sum);
    BOOST_CHECK_EQUAL(f.lower(), lower);
    BOOST_CHECK(f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(add_to_upper_8003)
{
    Address addr({8,0,0,3});
    Word upper({1,2, 3,4,5,6, 7,8,9,0, '+'});
    Word lower({3,8, 3,8,5,6, 7,8,9,0, '+'});
    Word distr(lower);
    Word sum({2,4, 6,9,1,3, 5,7,8,0, '+'});

    Opcode_Fixture f(10, distr, addr, upper, lower, distr);
    BOOST_CHECK_EQUAL(f.distributor(), upper);
    BOOST_CHECK_EQUAL(f.upper(), sum);
    BOOST_CHECK_EQUAL(f.lower(), lower);
    BOOST_CHECK(!f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(add_to_upper_8002)
{
    Address addr({8,0,0,2});
    Word upper({1,2, 3,4,5,6, 7,8,9,0, '+'});
    Word lower({3,8, 3,8,5,6, 7,8,9,0, '+'});
    Word distr(upper);
    Word sum({5,0, 7,3,1,3, 5,7,8,0, '+'});

    Opcode_Fixture f(10, distr, addr, upper, lower, distr);
    BOOST_CHECK_EQUAL(f.distributor(), lower);
    BOOST_CHECK_EQUAL(f.upper(), sum);
    BOOST_CHECK_EQUAL(f.lower(), lower);
    BOOST_CHECK(!f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(add_to_upper_8001)
{
    Address addr({8,0,0,1});
    Word upper({1,2, 3,4,5,6, 7,8,9,0, '+'});
    Word lower({3,8, 3,8,5,6, 7,8,9,0, '+'});
    Word distr(change_sign(upper));
    Word sum({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Opcode_Fixture f(10, distr, addr, upper, lower, distr);
    BOOST_CHECK_EQUAL(f.distributor(), distr);
    BOOST_CHECK_EQUAL(f.upper(), sum);
    BOOST_CHECK_EQUAL(f.lower(), lower);
    BOOST_CHECK(!f.computer.overflow());
}

// 15  AL  Add to Lower
BOOST_AUTO_TEST_CASE(add_to_lower_1)
{
    Word data({0,0, 1,2,3,4, 5,6,7,8, '+'});
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word lower({9,9, 8,9,3,7, 4,6,2,7, '+'});
    Word distr(lower);
    Word upper_sum({0,0, 0,0,0,0, 0,0,0,1, '+'});
    Word lower_sum({0,0, 0,1,7,2, 0,3,0,5, '+'});

    Opcode_Fixture f(15, data, addr, upper, lower, distr);
    BOOST_CHECK_EQUAL(f.distributor(), data);
    BOOST_CHECK_EQUAL(f.upper(), upper_sum);
    BOOST_CHECK_EQUAL(f.lower(), lower_sum);
    BOOST_CHECK(!f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(add_to_lower_2)
{
    Word data({0,0, 1,2,3,4, 5,6,7,8, '+'});
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word lower({9,9, 8,9,3,7, 4,6,2,7, '-'});
    Word distr(lower);
    Word upper_sum({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower_sum({9,9, 7,7,0,2, 8,9,4,9, '-'});

    Opcode_Fixture f(15, data, addr, upper, lower, distr);
    BOOST_CHECK_EQUAL(f.distributor(), data);
    BOOST_CHECK_EQUAL(f.upper(), upper_sum);
    BOOST_CHECK_EQUAL(f.lower(), lower_sum);
    BOOST_CHECK(!f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(add_to_lower_3)
{
    Word data({0,0, 1,2,3,4, 5,6,7,8, '-'});
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower({9,9, 8,9,3,7, 4,6,2,7, '-'});
    Word distr(lower);
    Word upper_sum({0,0, 0,0,0,0, 0,0,0,1, '-'});
    Word lower_sum({0,0, 0,1,7,2, 0,3,0,5, '-'});

    Opcode_Fixture f(15, data, addr, upper, lower, distr);
    BOOST_CHECK_EQUAL(f.distributor(), data);
    BOOST_CHECK_EQUAL(f.upper(), upper_sum);
    BOOST_CHECK_EQUAL(f.lower(), lower_sum);
    BOOST_CHECK(!f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(add_to_lower_8003)
{
    Address addr({8,0,0,3});
    Word upper({9,8, 7,6,5,4, 3,2,1,0, '+'});
    Word lower({3,8, 3,8,5,6, 7,8,9,0, '+'});
    Word distr(lower);
    Word upper_sum({9,8, 7,6,5,4, 3,2,1,1, '+'});
    Word lower_sum({3,7, 1,5,1,1, 1,1,0,0, '+'});

    Opcode_Fixture f(15, distr, addr, upper, lower, distr);
    BOOST_CHECK_EQUAL(f.distributor(), upper);
    BOOST_CHECK_EQUAL(f.upper(), upper_sum);
    BOOST_CHECK_EQUAL(f.lower(), lower_sum);
    BOOST_CHECK(!f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(add_to_lower_8002)
{
    Address addr({8,0,0,2});
    Word upper({9,8, 7,6,5,4, 3,2,1,0, '+'});
    Word lower({3,8, 3,8,5,6, 7,8,9,0, '+'});
    Word distr(lower);
    Word lower_sum({7,6, 7,7,1,3, 5,7,8,0, '+'});

    Opcode_Fixture f(15, distr, addr, upper, lower, distr);
    BOOST_CHECK_EQUAL(f.distributor(), lower);
    BOOST_CHECK_EQUAL(f.upper(), upper);
    BOOST_CHECK_EQUAL(f.lower(), lower_sum);
    BOOST_CHECK(!f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(add_to_lower_8001)
{
    Address addr({8,0,0,1});
    Word upper({1,2, 3,4,5,6, 7,8,9,0, '+'});
    Word lower({3,8, 3,8,5,6, 7,8,9,0, '+'});
    Word distr(change_sign(lower));
    Word lower_sum({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Opcode_Fixture f(15, distr, addr, upper, lower, distr);
    BOOST_CHECK_EQUAL(f.distributor(), distr);
    BOOST_CHECK_EQUAL(f.upper(), upper);
    BOOST_CHECK_EQUAL(f.lower(), lower_sum);
    BOOST_CHECK(!f.computer.overflow());
}

// 11  SU  Subtract from Upper
BOOST_AUTO_TEST_CASE(subtract_from_upper_1)
{
    Word data({0,0, 1,2,3,4, 5,6,7,8, '-'});
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,4,5, 8,6,3,2, '-'});
    Word lower({8,9, 4,2,7,1, 1,3,6,5, '-'});
    Word distr(lower);
    Word upper_sum({0,0, 1,1,8,8, 7,0,4,5, '+'});
    Word lower_sum({1,0, 5,7,2,8, 8,6,3,5, '+'});

    Opcode_Fixture f(11, data, addr, upper, lower, distr);
    BOOST_CHECK_EQUAL(f.distributor(), data);
    BOOST_CHECK_EQUAL(f.upper(), upper_sum);
    BOOST_CHECK_EQUAL(f.lower(), lower_sum);
    BOOST_CHECK(!f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(subtract_from_upper_2)
{
    Word data({0,0, 1,2,3,4, 5,6,7,8, '+'});
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,4,5, 8,6,3,2, '-'});
    Word lower({8,9, 4,2,7,1, 1,3,6,5, '-'});
    Word distr(upper);
    Word upper_sum({0,0, 1,2,8,0, 4,3,1,0, '-'});
    Word lower_sum({8,9, 4,2,7,1, 1,3,6,5, '-'});

    Opcode_Fixture f(11, data, addr, upper, lower, distr);
    BOOST_CHECK_EQUAL(f.distributor(), data);
    BOOST_CHECK_EQUAL(f.upper(), upper_sum);
    BOOST_CHECK_EQUAL(f.lower(), lower_sum);
    BOOST_CHECK(!f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(subtract_from_upper_3)
{
    Word data({0,0, 1,2,3,4, 5,6,7,8, '+'});
    Address addr({1,0,0,0});
    Word upper({9,9, 8,9,3,7, 4,6,2,7, '-'});
    Word lower({8,9, 4,2,7,1, 1,3,6,5, '-'});
    Word distr(upper);
    Word sum({0,0, 0,1,7,2, 0,3,0,5, '-'});

    Opcode_Fixture f(11, data, addr, upper, lower, distr);
    BOOST_CHECK_EQUAL(f.distributor(), data);
    BOOST_CHECK_EQUAL(f.upper(), sum);
    BOOST_CHECK_EQUAL(f.lower(), lower);
    BOOST_CHECK(f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(subtract_from_upper_8003)
{
    Address addr({8,0,0,3});
    Word upper({1,2, 3,4,5,6, 7,8,9,0, '+'});
    Word lower({3,8, 3,8,5,6, 7,8,9,0, '+'});
    Word distr(lower);
    Word sum({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Opcode_Fixture f(11, distr, addr, upper, lower, distr);
    BOOST_CHECK_EQUAL(f.distributor(), upper);
    BOOST_CHECK_EQUAL(f.upper(), sum);
    BOOST_CHECK_EQUAL(f.lower(), lower);
    BOOST_CHECK(!f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(subtract_from_upper_8002)
{
    Address addr({8,0,0,2});
    Word upper({1,2, 3,4,5,6, 7,8,9,0, '+'});
    Word lower({3,8, 3,8,5,6, 7,8,9,0, '+'});
    Word distr(upper);
    Word upper_sum({2,6, 0,3,9,9, 9,9,9,9, '-'});
    Word lower_sum({6,1, 6,1,4,3, 2,1,1,0, '-'});

    Opcode_Fixture f(11, distr, addr, upper, lower, distr);
    BOOST_CHECK_EQUAL(f.distributor(), lower);
    BOOST_CHECK_EQUAL(f.upper(), upper_sum);
    BOOST_CHECK_EQUAL(f.lower(), lower_sum);
    BOOST_CHECK(!f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(subtract_from_upper_8001)
{
    Address addr({8,0,0,1});
    Word upper({1,2, 3,4,5,6, 7,8,9,0, '+'});
    Word lower({3,8, 3,8,5,6, 7,8,9,0, '+'});
    Word distr(upper);
    Word sum({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Opcode_Fixture f(11, distr, addr, upper, lower, distr);
    BOOST_CHECK_EQUAL(f.distributor(), distr);
    BOOST_CHECK_EQUAL(f.upper(), sum);
    BOOST_CHECK_EQUAL(f.lower(), lower);
    BOOST_CHECK(!f.computer.overflow());
}

// 16  SL  Subtract from Lower
BOOST_AUTO_TEST_CASE(subtract_from_lower_1)
{
    Word data({0,0, 1,2,3,4, 5,6,7,8, '+'});
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word lower({9,9, 8,9,3,7, 4,6,2,7, '+'});
    Word distr(lower);
    Word upper_sum({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word lower_sum({9,9, 7,7,0,2, 8,9,4,9, '+'});

    Opcode_Fixture f(16, data, addr, upper, lower, distr);
    BOOST_CHECK_EQUAL(f.distributor(), data);
    BOOST_CHECK_EQUAL(f.upper(), upper_sum);
    BOOST_CHECK_EQUAL(f.lower(), lower_sum);
    BOOST_CHECK(!f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(subtract_from_lower_2)
{
    Word data({0,0, 1,2,3,4, 5,6,7,8, '-'});
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word lower({9,9, 8,9,3,7, 4,6,2,7, '+'});
    Word distr(lower);
    Word upper_sum({0,0, 0,0,0,0, 0,0,0,1, '+'});
    Word lower_sum({0,0, 0,1,7,2, 0,3,0,5, '+'});

    Opcode_Fixture f(16, data, addr, upper, lower, distr);
    BOOST_CHECK_EQUAL(f.distributor(), data);
    BOOST_CHECK_EQUAL(f.upper(), upper_sum);
    BOOST_CHECK_EQUAL(f.lower(), lower_sum);
    BOOST_CHECK(!f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(subtract_from_lower_3)
{
    Word data({0,0, 1,2,3,4, 5,6,7,8, '-'});
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower({9,9, 8,9,3,7, 4,6,2,7, '-'});
    Word distr(lower);
    Word upper_sum({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower_sum({9,9, 7,7,0,2, 8,9,4,9, '-'});

    Opcode_Fixture f(16, data, addr, upper, lower, distr);
    BOOST_CHECK_EQUAL(f.distributor(), data);
    BOOST_CHECK_EQUAL(f.upper(), upper_sum);
    BOOST_CHECK_EQUAL(f.lower(), lower_sum);
    BOOST_CHECK(!f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(subtract_from_lower_8003)
{
    Address addr({8,0,0,3});
    Word upper({9,8, 7,6,5,4, 3,2,1,0, '+'});
    Word lower({3,8, 3,8,5,6, 7,8,9,0, '+'});
    Word distr(lower);
    Word upper_sum({9,8, 7,6,5,4, 3,2,0,9, '+'});
    Word lower_sum({3,9, 6,2,0,2, 4,6,8,0, '+'});

    Opcode_Fixture f(16, distr, addr, upper, lower, distr);
    BOOST_CHECK_EQUAL(f.distributor(), upper);
    BOOST_CHECK_EQUAL(f.upper(), upper_sum);
    BOOST_CHECK_EQUAL(f.lower(), lower_sum);
    BOOST_CHECK(!f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(subtract_from_lower_8002)
{
    Address addr({8,0,0,2});
    Word upper({1,2, 3,4,5,6, 7,8,9,0, '+'});
    Word lower({3,8, 3,8,5,6, 7,8,9,0, '+'});
    Word distr(lower);
    Word lower_sum({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Opcode_Fixture f(16, distr, addr, upper, lower, distr);
    BOOST_CHECK_EQUAL(f.distributor(), lower);
    BOOST_CHECK_EQUAL(f.upper(), upper);
    BOOST_CHECK_EQUAL(f.lower(), lower_sum);
    BOOST_CHECK(!f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(subtract_from_lower_8001)
{
    Address addr({8,0,0,1});
    Word upper({1,2, 3,4,5,6, 7,8,9,0, '+'});
    Word lower({3,8, 3,8,5,6, 7,8,9,0, '+'});
    Word distr(lower);
    Word lower_sum({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Opcode_Fixture f(16, distr, addr, upper, lower, distr);
    BOOST_CHECK_EQUAL(f.distributor(), distr);
    BOOST_CHECK_EQUAL(f.upper(), upper);
    BOOST_CHECK_EQUAL(f.lower(), lower_sum);
    BOOST_CHECK(!f.computer.overflow());
}

// 60  RAU  Reset and Add into Upper
BOOST_AUTO_TEST_CASE(reset_and_add_into_upper_1)
{
    Word data({0,0, 1,2,3,4, 5,6,7,8, '+'});
    Address addr({1,0,0,0});
    Word upper({8,9, 4,2,7,1, 1,3,6,5, '+'});
    Word lower({9,9, 8,9,3,7, 4,6,2,7, '+'});
    Word distr(upper);
    Word upper_sum(data);
    Word lower_sum({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Opcode_Fixture f(60, data, addr, upper, lower, distr);
    BOOST_CHECK_EQUAL(f.distributor(), data);
    BOOST_CHECK_EQUAL(f.upper(), upper_sum);
    BOOST_CHECK_EQUAL(f.lower(), lower_sum);
    BOOST_CHECK(!f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(reset_and_add_into_upper_2)
{
    Word data({0,0, 1,2,3,4, 5,6,7,8, '-'});
    Address addr({1,0,0,0});
    Word upper({8,9, 4,2,7,1, 1,3,6,5, '+'});
    Word lower({9,9, 8,9,3,7, 4,6,2,7, '+'});
    Word distr(upper);
    Word upper_sum(data);
    Word lower_sum({0,0, 0,0,0,0, 0,0,0,0, '-'});

    Opcode_Fixture f(60, data, addr, upper, lower, distr);
    BOOST_CHECK_EQUAL(f.distributor(), data);
    BOOST_CHECK_EQUAL(f.upper(), upper_sum);
    BOOST_CHECK_EQUAL(f.lower(), lower_sum);
    BOOST_CHECK(!f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(reset_and_add_into_upper_8003)
{
    Address addr({8,0,0,3});
    Word upper({1,2, 3,4,5,6, 7,8,9,0, '+'});
    Word lower({3,8, 3,8,5,6, 7,8,9,0, '+'});
    Word distr(lower);
    Word upper_sum(upper);
    Word lower_sum({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Opcode_Fixture f(60, distr, addr, upper, lower, distr);
    BOOST_CHECK_EQUAL(f.distributor(), upper);
    BOOST_CHECK_EQUAL(f.upper(), upper_sum);
    BOOST_CHECK_EQUAL(f.lower(), lower_sum);
    BOOST_CHECK(!f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(reset_and_add_into_upper_8002)
{
    Address addr({8,0,0,2});
    Word upper({1,2, 3,4,5,6, 7,8,9,0, '+'});
    Word lower({3,8, 3,8,5,6, 7,8,9,0, '+'});
    Word distr(upper);
    Word upper_sum(lower);
    Word lower_sum({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Opcode_Fixture f(60, distr, addr, upper, lower, distr);
    BOOST_CHECK_EQUAL(f.distributor(), lower);
    BOOST_CHECK_EQUAL(f.upper(), upper_sum);
    BOOST_CHECK_EQUAL(f.lower(), lower_sum);
    BOOST_CHECK(!f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(reset_and_add_into_upper_8001)
{
    Address addr({8,0,0,1});
    Word upper({1,2, 3,4,5,6, 7,8,9,0, '+'});
    Word lower({3,8, 3,8,5,6, 7,8,9,0, '+'});
    Word distr(change_sign(upper));
    Word upper_sum(distr);
    Word lower_sum({0,0, 0,0,0,0, 0,0,0,0, '-'});

    Opcode_Fixture f(60, distr, addr, upper, lower, distr);
    BOOST_CHECK_EQUAL(f.distributor(), distr);
    BOOST_CHECK_EQUAL(f.upper(), upper_sum);
    BOOST_CHECK_EQUAL(f.lower(), lower_sum);
    BOOST_CHECK(!f.computer.overflow());
}

// 65  RAL  Reset and Add into Lower
BOOST_AUTO_TEST_CASE(reset_and_add_into_lower_1)
{
    Word data({0,0, 1,2,3,4, 5,6,7,8, '+'});
    Address addr({1,0,0,0});
    Word upper({8,9, 4,2,7,1, 1,3,6,5, '+'});
    Word lower({9,9, 8,9,3,7, 4,6,2,7, '+'});
    Word distr(upper);
    Word upper_sum({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word lower_sum(data);

    Opcode_Fixture f(65, data, addr, upper, lower, distr);
    BOOST_CHECK_EQUAL(f.distributor(), data);
    BOOST_CHECK_EQUAL(f.upper(), upper_sum);
    BOOST_CHECK_EQUAL(f.lower(), lower_sum);
    BOOST_CHECK(!f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(reset_and_add_into_lower_2)
{
    Word data({0,0, 1,2,3,4, 5,6,7,8, '-'});
    Address addr({1,0,0,0});
    Word upper({8,9, 4,2,7,1, 1,3,6,5, '+'});
    Word lower({9,9, 8,9,3,7, 4,6,2,7, '+'});
    Word distr(upper);
    Word upper_sum({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower_sum(data);

    Opcode_Fixture f(65, data, addr, upper, lower, distr);
    BOOST_CHECK_EQUAL(f.distributor(), data);
    BOOST_CHECK_EQUAL(f.upper(), upper_sum);
    BOOST_CHECK_EQUAL(f.lower(), lower_sum);
    BOOST_CHECK(!f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(reset_and_add_into_lower_8003)
{
    Address addr({8,0,0,3});
    Word upper({1,2, 3,4,5,6, 7,8,9,0, '+'});
    Word lower({3,8, 3,8,5,6, 7,8,9,0, '+'});
    Word distr(lower);
    Word upper_sum({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word lower_sum(upper);

    Opcode_Fixture f(65, distr, addr, upper, lower, distr);
    BOOST_CHECK_EQUAL(f.distributor(), upper);
    BOOST_CHECK_EQUAL(f.upper(), upper_sum);
    BOOST_CHECK_EQUAL(f.lower(), lower_sum);
    BOOST_CHECK(!f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(reset_and_add_into_lower_8002)
{
    Address addr({8,0,0,2});
    Word upper({1,2, 3,4,5,6, 7,8,9,0, '+'});
    Word lower({3,8, 3,8,5,6, 7,8,9,0, '+'});
    Word distr(lower);
    Word upper_sum({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word lower_sum(lower);

    Opcode_Fixture f(65, distr, addr, upper, lower, distr);
    BOOST_CHECK_EQUAL(f.distributor(), lower);
    BOOST_CHECK_EQUAL(f.upper(), upper_sum);
    BOOST_CHECK_EQUAL(f.lower(), lower_sum);
    BOOST_CHECK(!f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(reset_and_add_into_lower_8001)
{
    Address addr({8,0,0,1});
    Word upper({1,2, 3,4,5,6, 7,8,9,0, '+'});
    Word lower({3,8, 3,8,5,6, 7,8,9,0, '+'});
    Word distr(change_sign(upper));
    Word upper_sum({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower_sum(distr);

    Opcode_Fixture f(65, distr, addr, upper, lower, distr);
    BOOST_CHECK_EQUAL(f.distributor(), distr);
    BOOST_CHECK_EQUAL(f.upper(), upper_sum);
    BOOST_CHECK_EQUAL(f.lower(), lower_sum);
    BOOST_CHECK(!f.computer.overflow());
}

// 61  RSU  Reset and Subtract into Upper
BOOST_AUTO_TEST_CASE(reset_and_subtract_into_upper_1)
{
    Word data({0,0, 1,2,3,4, 5,6,7,8, '+'});
    Address addr({1,0,0,0});
    Word upper({8,9, 4,2,7,1, 1,3,6,5, '+'});
    Word lower({9,9, 8,9,3,7, 4,6,2,7, '+'});
    Word distr(upper);
    Word upper_sum(change_sign(data));
    Word lower_sum({0,0, 0,0,0,0, 0,0,0,0, '-'});

    Opcode_Fixture f(61, data, addr, upper, lower, distr);
    BOOST_CHECK_EQUAL(f.distributor(), data);
    BOOST_CHECK_EQUAL(f.upper(), upper_sum);
    BOOST_CHECK_EQUAL(f.lower(), lower_sum);
    BOOST_CHECK(!f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(reset_and_subtract_into_upper_2)
{
    Word data({0,0, 1,2,3,4, 5,6,7,8, '-'});
    Address addr({1,0,0,0});
    Word upper({8,9, 4,2,7,1, 1,3,6,5, '+'});
    Word lower({9,9, 8,9,3,7, 4,6,2,7, '+'});
    Word distr(upper);
    Word upper_sum(change_sign(data));
    Word lower_sum({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Opcode_Fixture f(61, data, addr, upper, lower, distr);
    BOOST_CHECK_EQUAL(f.distributor(), data);
    BOOST_CHECK_EQUAL(f.upper(), upper_sum);
    BOOST_CHECK_EQUAL(f.lower(), lower_sum);
    BOOST_CHECK(!f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(reset_and_subtract_into_upper_8003)
{
    Address addr({8,0,0,3});
    Word upper({1,2, 3,4,5,6, 7,8,9,0, '+'});
    Word lower({3,8, 3,8,5,6, 7,8,9,0, '+'});
    Word distr(lower);
    Word upper_sum(change_sign(upper));
    Word lower_sum({0,0, 0,0,0,0, 0,0,0,0, '-'});

    Opcode_Fixture f(61, distr, addr, upper, lower, distr);
    BOOST_CHECK_EQUAL(f.distributor(), upper);
    BOOST_CHECK_EQUAL(f.upper(), upper_sum);
    BOOST_CHECK_EQUAL(f.lower(), lower_sum);
    BOOST_CHECK(!f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(reset_and_subtract_into_upper_8002)
{
    Address addr({8,0,0,2});
    Word upper({1,2, 3,4,5,6, 7,8,9,0, '+'});
    Word lower({3,8, 3,8,5,6, 7,8,9,0, '+'});
    Word distr(upper);
    Word upper_sum(change_sign(lower));
    Word lower_sum({0,0, 0,0,0,0, 0,0,0,0, '-'});

    Opcode_Fixture f(61, distr, addr, upper, lower, distr);
    BOOST_CHECK_EQUAL(f.distributor(), lower);
    BOOST_CHECK_EQUAL(f.upper(), upper_sum);
    BOOST_CHECK_EQUAL(f.lower(), lower_sum);
    BOOST_CHECK(!f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(reset_and_subtract_into_upper_8001)
{
    Address addr({8,0,0,1});
    Word upper({1,2, 3,4,5,6, 7,8,9,0, '+'});
    Word lower({3,8, 3,8,5,6, 7,8,9,0, '+'});
    Word distr(upper);
    Word upper_sum(change_sign(distr));
    Word lower_sum({0,0, 0,0,0,0, 0,0,0,0, '-'});

    Opcode_Fixture f(61, distr, addr, upper, lower, distr);
    BOOST_CHECK_EQUAL(f.distributor(), distr);
    BOOST_CHECK_EQUAL(f.upper(), upper_sum);
    BOOST_CHECK_EQUAL(f.lower(), lower_sum);
    BOOST_CHECK(!f.computer.overflow());
}

// 66  RSL  Reset and Add into Lower
BOOST_AUTO_TEST_CASE(reset_and_subtract_into_lower_1)
{
    Word data({0,0, 1,2,3,4, 5,6,7,8, '+'});
    Address addr({1,0,0,0});
    Word upper({8,9, 4,2,7,1, 1,3,6,5, '+'});
    Word lower({9,9, 8,9,3,7, 4,6,2,7, '+'});
    Word distr(upper);
    Word upper_sum({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower_sum(change_sign(data));

    Opcode_Fixture f(66, data, addr, upper, lower, distr);
    BOOST_CHECK_EQUAL(f.distributor(), data);
    BOOST_CHECK_EQUAL(f.upper(), upper_sum);
    BOOST_CHECK_EQUAL(f.lower(), lower_sum);
    BOOST_CHECK(!f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(reset_and_subtract_into_lower_2)
{
    Word data({0,0, 1,2,3,4, 5,6,7,8, '-'});
    Address addr({1,0,0,0});
    Word upper({8,9, 4,2,7,1, 1,3,6,5, '+'});
    Word lower({9,9, 8,9,3,7, 4,6,2,7, '+'});
    Word distr(lower);
    Word upper_sum({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word lower_sum(change_sign(data));

    Opcode_Fixture f(66, data, addr, upper, lower, distr);
    BOOST_CHECK_EQUAL(f.distributor(), data);
    BOOST_CHECK_EQUAL(f.upper(), upper_sum);
    BOOST_CHECK_EQUAL(f.lower(), lower_sum);
    BOOST_CHECK(!f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(reset_and_subtract_into_lower_8003)
{
    Address addr({8,0,0,3});
    Word upper({1,2, 3,4,5,6, 7,8,9,0, '+'});
    Word lower({3,8, 3,8,5,6, 7,8,9,0, '+'});
    Word distr(lower);
    Word upper_sum({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower_sum(change_sign(upper));

    Opcode_Fixture f(66, distr, addr, upper, lower, distr);
    BOOST_CHECK_EQUAL(f.distributor(), upper);
    BOOST_CHECK_EQUAL(f.upper(), upper_sum);
    BOOST_CHECK_EQUAL(f.lower(), lower_sum);
    BOOST_CHECK(!f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(reset_and_subtract_into_lower_8002)
{
    Address addr({8,0,0,2});
    Word upper({1,2, 3,4,5,6, 7,8,9,0, '-'});
    Word lower({3,8, 3,8,5,6, 7,8,9,0, '-'});
    Word distr(lower);
    Word upper_sum({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word lower_sum(change_sign(lower));

    Opcode_Fixture f(66, distr, addr, upper, lower, distr);
    BOOST_CHECK_EQUAL(f.distributor(), lower);
    BOOST_CHECK_EQUAL(f.upper(), upper_sum);
    BOOST_CHECK_EQUAL(f.lower(), lower_sum);
    BOOST_CHECK(!f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(reset_and_subtract_into_lower_8001)
{
    Address addr({8,0,0,1});
    Word upper({1,2, 3,4,5,6, 7,8,9,0, '+'});
    Word lower({3,8, 3,8,5,6, 7,8,9,0, '+'});
    Word distr(upper);
    Word upper_sum({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower_sum(change_sign(distr));

    Opcode_Fixture f(66, distr, addr, upper, lower, distr);
    BOOST_CHECK_EQUAL(f.distributor(), distr);
    BOOST_CHECK_EQUAL(f.upper(), upper_sum);
    BOOST_CHECK_EQUAL(f.lower(), lower_sum);
    BOOST_CHECK(!f.computer.overflow());
}
