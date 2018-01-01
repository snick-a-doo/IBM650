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
            assert(addr.value() > 99);
            Word instr;
            instr.digits()[0] = bin(opcode / 10);
            instr.digits()[1] = bin(opcode % 10);
            instr.load(addr, 0, 2);
            instr.load(stop_address, 0, 6);

            computer.set_drum(start_address, instr);
            computer.set_drum(stop_address, Word({0,1, 0,0,0,0, 0,0,0,0}));
            if (addr.value() < 2000)
                computer.set_drum(addr, data);
            computer.set_distributor(distr);
            computer.set_upper(upper);
            computer.set_lower(lower);
            Word entry;
            entry.fill(0, '+');
            entry.load(start_address, 0, 6);
            computer.set_storage_entry(entry);

            computer.set_programmed_mode(Computer::Programmed_Mode::stop);
            computer.set_display_mode(Computer::Display_Mode::distributor);
            computer.set_control_mode(Computer::Control_Mode::run);
            computer.set_error_mode(Computer::Error_Mode::stop);
            computer.program_reset();
        }
    Opcode_Fixture(int opcode, const Address& addr, const Word& distr)
        : Opcode_Fixture(opcode, Word(), addr, Word(), Word(), distr)
        {}
    Opcode_Fixture(int opcode, const Address& addr)
        : Opcode_Fixture(opcode, Word(), addr, Word(), Word(), Word())
        {}

    void run() {
        computer.program_start();
    }

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

    static const Address start_address;
    static const Address stop_address;
};

const Address Opcode_Fixture::start_address = Address({0,0,1,0});
const Address Opcode_Fixture::stop_address = Address({0,0,2,0});


// 69  LD  Load Distributor
BOOST_AUTO_TEST_CASE(load_distributor_drum)
{
    Word data({0,0, 0,0,1,2, 3,4,5,6, '+'});
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word lower({0,0, 0,0,6,4, 3,2,1,7, '+'});
    Word distr(lower);

    Opcode_Fixture f(69, data, addr, upper, lower, distr);
    f.run();
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
    f.run();
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
    f.run();
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
    f.run();
    BOOST_CHECK_EQUAL(f.distributor(), distr);
    BOOST_CHECK_EQUAL(f.drum(addr), distr);
}

BOOST_AUTO_TEST_CASE(store_distributor_800x)
{
    { Opcode_Fixture f(24, Address({2,0,0,0}));
        f.run();
        BOOST_CHECK(f.computer.storage_selection_error()); }
    { Opcode_Fixture f(24, Address({8,0,0,3}));
        f.run();
        BOOST_CHECK(f.computer.storage_selection_error()); }
    { Opcode_Fixture f(24, Address({8,0,0,1}));
        f.run();
        BOOST_CHECK(f.computer.storage_selection_error()); }
    { Opcode_Fixture f(24, Address({8,0,0,2}));
        f.run();
        BOOST_CHECK(f.computer.storage_selection_error()); }
    { Opcode_Fixture f(24, Address({8,0,0,3}));
        f.run();
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
    f.run();
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
    Word upper_sum({0,0, 1,1,8,8, 7,0,4,5, '-'});
    Word lower_sum({1,0, 5,7,2,8, 8,6,3,5, '+'});

    Opcode_Fixture f(10, data, addr, upper, lower, distr);
    f.run();
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
    f.run();
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
    f.run();
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
    f.run();
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
    f.run();
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
    f.run();
    BOOST_CHECK_EQUAL(f.distributor(), data);
    BOOST_CHECK_EQUAL(f.upper(), upper_sum);
    BOOST_CHECK_EQUAL(f.lower(), lower_sum);
    BOOST_CHECK(!f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(add_to_lower_2)
{
    Word data({0,0, 1,2,3,4, 5,6,7,8, '+'});
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower({9,9, 8,9,3,7, 4,6,2,7, '-'});
    Word distr(lower);
    Word upper_sum({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower_sum({9,9, 7,7,0,2, 8,9,4,9, '-'});

    Opcode_Fixture f(15, data, addr, upper, lower, distr);
    f.run();
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
    f.run();
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
    f.run();
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
    f.run();
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
    f.run();
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
    Word upper_sum({0,0, 1,1,8,8, 7,0,4,5, '-'});
    Word lower_sum({1,0, 5,7,2,8, 8,6,3,5, '+'});

    Opcode_Fixture f(11, data, addr, upper, lower, distr);
    f.run();
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
    f.run();
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
    f.run();
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
    f.run();
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
    Word upper_sum({2,6, 0,3,9,9, 9,9,9,9, '+'});
    Word lower_sum({6,1, 6,1,4,3, 2,1,1,0, '-'});

    Opcode_Fixture f(11, distr, addr, upper, lower, distr);
    f.run();
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
    f.run();
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
    f.run();
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
    f.run();
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
    f.run();
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
    f.run();
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
    f.run();
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
    f.run();
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
    f.run();
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
    f.run();
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
    f.run();
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
    f.run();
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
    f.run();
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
    f.run();
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
    f.run();
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
    f.run();
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
    f.run();
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
    f.run();
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
    f.run();
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
    f.run();
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
    f.run();
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
    f.run();
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
    f.run();
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
    f.run();
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
    f.run();
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
    f.run();
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
    f.run();
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
    f.run();
    BOOST_CHECK_EQUAL(f.distributor(), distr);
    BOOST_CHECK_EQUAL(f.upper(), upper_sum);
    BOOST_CHECK_EQUAL(f.lower(), lower_sum);
    BOOST_CHECK(!f.computer.overflow());
}

// 20  STL  Store Lower in Memory
BOOST_AUTO_TEST_CASE(store_lower_in_memory)
{
    Word data({0,0, 1,2,3,4, 5,6,7,8, '+'});
    Address addr({1,0,0,0});
    Word upper({0,4, 0,4,5,3, 2,8,7,7, '-'});
    Word lower({0,0, 0,0,5,5, 3,8,2,2, '-'});
    Word distr({0,0, 0,0,0,1, 2,9,9,8, '+'});

    Opcode_Fixture f(20, distr, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK_EQUAL(f.distributor(), lower);
    BOOST_CHECK_EQUAL(f.upper(), upper);
    BOOST_CHECK_EQUAL(f.lower(), lower);
    BOOST_CHECK_EQUAL(f.drum(addr), lower);
}

BOOST_AUTO_TEST_CASE(store_lower_in_memory_800x)
{
    { Opcode_Fixture f(20, Address({2,0,0,0}));
        f.run();
        BOOST_CHECK(f.computer.storage_selection_error()); }
    { Opcode_Fixture f(20, Address({8,0,0,3}));
        f.run();
        BOOST_CHECK(f.computer.storage_selection_error()); }
    { Opcode_Fixture f(20, Address({8,0,0,1}));
        f.run();
        BOOST_CHECK(f.computer.storage_selection_error()); }
    { Opcode_Fixture f(20, Address({8,0,0,2}));
        f.run();
        BOOST_CHECK(f.computer.storage_selection_error()); }
    { Opcode_Fixture f(20, Address({8,0,0,3}));
        f.run();
        BOOST_CHECK(f.computer.storage_selection_error()); }
}

// 21  STU  Store Upper in Memory
BOOST_AUTO_TEST_CASE(store_upper_in_memory_1)
{
    Word data({0,0, 1,2,3,4, 5,6,7,8, '+'});
    Address addr({1,0,0,0});
    Word upper({0,4, 0,4,5,3, 2,8,7,7, '+'});
    Word lower({0,0, 0,0,5,5, 3,8,2,2, '+'});
    Word distr({0,0, 0,0,0,1, 2,9,9,8, '+'});

    Opcode_Fixture f(21, distr, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK_EQUAL(f.distributor(), upper);
    BOOST_CHECK_EQUAL(f.upper(), upper);
    BOOST_CHECK_EQUAL(f.lower(), lower);
    BOOST_CHECK_EQUAL(f.drum(addr), upper);
}

BOOST_AUTO_TEST_CASE(store_upper_in_memory_2)
{
    Word data({0,0, 1,2,3,4, 5,6,7,8, '+'});
    Address addr({1,0,0,0});
    Word upper({0,4, 0,4,5,3, 2,8,7,7, '-'});
    Word lower({0,0, 0,0,5,5, 3,8,2,2, '-'});
    Word distr({0,0, 0,0,0,1, 2,9,9,8, '+'});

    Opcode_Fixture f(21, distr, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK_EQUAL(f.distributor(), upper);
    BOOST_CHECK_EQUAL(f.upper(), upper);
    BOOST_CHECK_EQUAL(f.lower(), lower);
    BOOST_CHECK_EQUAL(f.drum(addr), upper);
}

BOOST_AUTO_TEST_CASE(store_upper_in_memory_3)
{
    Word data({0,1, 3,6,5,4, 3,2,1,0, '+'});
    Address addr({1,0,0,0});
    Word remainder({0,0, 0,6,3,4, 9,2,1,1, '-'});
    Word quotient({0,0, 1,4,9,8, 7,6,2,1, '+'});
    Word distr({0,0, 0,6,4,3, 2,1,9,8, '+'});

    Opcode_Fixture f(21, distr, addr, remainder, quotient, distr);
    f.run();
    BOOST_CHECK_EQUAL(f.distributor(), remainder);
    BOOST_CHECK_EQUAL(f.upper(), remainder);
    BOOST_CHECK_EQUAL(f.lower(), quotient);
    BOOST_CHECK_EQUAL(f.drum(addr), remainder);
}

BOOST_AUTO_TEST_CASE(store_upper_in_memory_800x)
{
    { Opcode_Fixture f(21, Address({2,0,0,0}));
        f.run();
        BOOST_CHECK(f.computer.storage_selection_error()); }
    { Opcode_Fixture f(21, Address({8,0,0,3}));
        f.run();
        BOOST_CHECK(f.computer.storage_selection_error()); }
    { Opcode_Fixture f(21, Address({8,0,0,1}));
        f.run();
        BOOST_CHECK(f.computer.storage_selection_error()); }
    { Opcode_Fixture f(21, Address({8,0,0,2}));
        f.run();
        BOOST_CHECK(f.computer.storage_selection_error()); }
    { Opcode_Fixture f(21, Address({8,0,0,3}));
        f.run();
        BOOST_CHECK(f.computer.storage_selection_error()); }
}

// 22  STDA  Store Lower Data Address
BOOST_AUTO_TEST_CASE(store_lower_data_address)
{
    Word data({6,9, 1,9,4,6, 0,6,0,0, '+'});
    Address addr({0,1,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower({0,0, 1,9,8,8, 0,5,6,0, '-'});
    Word distr({6,9, 1,9,5,0, 0,6,0,0, '+'});
    Word store({6,9, 1,9,8,8, 0,6,0,0, '+'});

    Opcode_Fixture f(22, distr, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK_EQUAL(f.distributor(), store);
    BOOST_CHECK_EQUAL(f.upper(), upper);
    BOOST_CHECK_EQUAL(f.lower(), lower);
    BOOST_CHECK_EQUAL(f.drum(addr), store);
}

BOOST_AUTO_TEST_CASE(store_lower_data_address_800x)
{
    { Opcode_Fixture f(22, Address({2,0,0,0}));
        f.run();
        BOOST_CHECK(f.computer.storage_selection_error()); }
    { Opcode_Fixture f(22, Address({8,0,0,3}));
        f.run();
        BOOST_CHECK(f.computer.storage_selection_error()); }
    { Opcode_Fixture f(22, Address({8,0,0,1}));
        f.run();
        BOOST_CHECK(f.computer.storage_selection_error()); }
    { Opcode_Fixture f(22, Address({8,0,0,2}));
        f.run();
        BOOST_CHECK(f.computer.storage_selection_error()); }
    { Opcode_Fixture f(22, Address({8,0,0,3}));
        f.run();
        BOOST_CHECK(f.computer.storage_selection_error()); }
}

// 23  STDA  Store Lower Instruction Address
BOOST_AUTO_TEST_CASE(store_lower_instruction_address)
{
    Word data({6,9, 1,8,0,0, 0,7,1,5, '+'});
    Address addr({0,1,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower({0,0, 1,9,8,8, 0,8,1,6, '-'});
    Word distr({6,9, 1,8,0,0, 0,7,5,0, '+'});
    Word store({6,9, 1,8,0,0, 0,8,1,6, '+'});

    Opcode_Fixture f(23, distr, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK_EQUAL(f.distributor(), store);
    BOOST_CHECK_EQUAL(f.upper(), upper);
    BOOST_CHECK_EQUAL(f.lower(), lower);
    BOOST_CHECK_EQUAL(f.drum(addr), store);
}

BOOST_AUTO_TEST_CASE(store_lower_instruction_address_800x)
{
    { Opcode_Fixture f(23, Address({2,0,0,0}));
        f.run();
        BOOST_CHECK(f.computer.storage_selection_error()); }
    { Opcode_Fixture f(23, Address({8,0,0,3}));
        f.run();
        BOOST_CHECK(f.computer.storage_selection_error()); }
    { Opcode_Fixture f(23, Address({8,0,0,1}));
        f.run();
        BOOST_CHECK(f.computer.storage_selection_error()); }
    { Opcode_Fixture f(23, Address({8,0,0,2}));
        f.run();
        BOOST_CHECK(f.computer.storage_selection_error()); }
    { Opcode_Fixture f(23, Address({8,0,0,3}));
        f.run();
        BOOST_CHECK(f.computer.storage_selection_error()); }
}

// 17  AABL  Add Absolute to Lower
BOOST_AUTO_TEST_CASE(add_absolute_to_lower_1)
{
    Word data({0,0, 1,2,3,4, 5,6,7,8, '+'});
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word lower({9,9, 8,9,3,7, 4,6,2,7, '+'});
    Word distr(lower);
    Word upper_sum({0,0, 0,0,0,0, 0,0,0,1, '+'});
    Word lower_sum({0,0, 0,1,7,2, 0,3,0,5, '+'});

    Opcode_Fixture f(17, data, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK_EQUAL(f.distributor(), data);
    BOOST_CHECK_EQUAL(f.upper(), upper_sum);
    BOOST_CHECK_EQUAL(f.lower(), lower_sum);
    BOOST_CHECK(!f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(add_absolute_to_lower_2)
{
    Word data({0,0, 1,2,3,4, 5,6,7,8, '-'});
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower({8,9, 4,2,7,1, 1,3,6,5, '-'});
    Word distr(lower);
    Word upper_sum({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower_sum({8,9, 3,0,3,6, 5,6,8,7, '-'});

    Opcode_Fixture f(17, data, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK_EQUAL(f.distributor(), data);
    BOOST_CHECK_EQUAL(f.upper(), upper_sum);
    BOOST_CHECK_EQUAL(f.lower(), lower_sum);
    BOOST_CHECK(!f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(add_absolute_to_lower_3)
{
    Word data({0,0, 1,2,3,4, 5,6,7,8, '+'});
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower({9,9, 8,9,3,7, 4,6,2,7, '-'});
    Word distr(lower);
    Word upper_sum({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower_sum({9,9, 7,7,0,2, 8,9,4,9, '-'});

    Opcode_Fixture f(15, data, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK_EQUAL(f.distributor(), data);
    BOOST_CHECK_EQUAL(f.upper(), upper_sum);
    BOOST_CHECK_EQUAL(f.lower(), lower_sum);
    BOOST_CHECK(!f.computer.overflow());
}

// 67  RAABL  Reset and Add Absolute into Lower
BOOST_AUTO_TEST_CASE(reset_and_add_absolute_into_lower_1)
{
    Word data({0,0, 1,2,3,4, 5,6,7,8, '-'});
    Address addr({1,0,0,0});
    Word upper({8,9, 4,2,7,1, 1,3,6,5, '-'});
    Word lower({9,9, 8,9,3,7, 4,6,2,7, '-'});
    Word distr(lower);
    Word upper_sum({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word lower_sum({0,0, 1,2,3,4, 5,6,7,8, '+'});

    Opcode_Fixture f(67, data, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK_EQUAL(f.distributor(), data);
    BOOST_CHECK_EQUAL(f.upper(), upper_sum);
    BOOST_CHECK_EQUAL(f.lower(), lower_sum);
    BOOST_CHECK(!f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(reset_and_add_absolute_into_lower_2)
{
    Word data({0,0, 1,2,3,4, 5,6,7,8, '+'});
    Address addr({1,0,0,0});
    Word upper({8,9, 4,2,7,1, 1,3,6,5, '+'});
    Word lower({9,9, 8,9,3,7, 4,6,2,7, '+'});
    Word distr(lower);
    Word upper_sum({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word lower_sum({0,0, 1,2,3,4, 5,6,7,8, '+'});

    Opcode_Fixture f(67, data, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK_EQUAL(f.distributor(), data);
    BOOST_CHECK_EQUAL(f.upper(), upper_sum);
    BOOST_CHECK_EQUAL(f.lower(), lower_sum);
    BOOST_CHECK(!f.computer.overflow());
}

// 18  SABL  Subtract Absolute from Lower
BOOST_AUTO_TEST_CASE(subtract_absolute_from_lower_1)
{
    Word data({0,0, 1,2,3,4, 5,6,7,8, '-'});
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower({8,9, 4,2,7,1, 1,3,6,5, '-'});
    Word distr(lower);
    Word upper_sum({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower_sum({8,9, 5,5,0,5, 7,0,4,3, '-'});

    Opcode_Fixture f(18, data, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK_EQUAL(f.distributor(), data);
    BOOST_CHECK_EQUAL(f.upper(), upper_sum);
    BOOST_CHECK_EQUAL(f.lower(), lower_sum);
    BOOST_CHECK(!f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(subtract_absolute_from_lower_2)
{
    Word data({0,0, 1,2,3,4, 5,6,7,8, '+'});
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word lower({8,9, 4,2,7,1, 1,3,6,5, '+'});
    Word distr(lower);
    Word upper_sum({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word lower_sum({8,9, 3,0,3,6, 5,6,8,7, '+'});

    Opcode_Fixture f(18, data, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK_EQUAL(f.distributor(), data);
    BOOST_CHECK_EQUAL(f.upper(), upper_sum);
    BOOST_CHECK_EQUAL(f.lower(), lower_sum);
    BOOST_CHECK(!f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(subtract_absolute_from_lower_3)
{
    Word data({0,0, 1,2,3,4, 5,6,7,8, '+'});
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower({9,9, 8,9,3,7, 4,6,2,7, '-'});
    Word distr(lower);
    Word upper_sum({0,0, 0,0,0,0, 0,0,0,1, '-'});
    Word lower_sum({0,0, 0,1,7,2, 0,3,0,5, '-'});

    Opcode_Fixture f(18, data, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK_EQUAL(f.distributor(), data);
    BOOST_CHECK_EQUAL(f.upper(), upper_sum);
    BOOST_CHECK_EQUAL(f.lower(), lower_sum);
    BOOST_CHECK(!f.computer.overflow());
}

// 68  RSABL  Reset and Subtract Absolute into Lower
BOOST_AUTO_TEST_CASE(reset_and_subtract_absolute_into_lower_1)
{
    Word data({0,0, 1,2,3,4, 5,6,7,8, '+'});
    Address addr({1,0,0,0});
    Word upper({8,9, 4,2,7,1, 1,3,6,5, '+'});
    Word lower({9,9, 8,9,3,7, 4,6,2,7, '+'});
    Word distr(lower);
    Word upper_sum({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower_sum({0,0, 1,2,3,4, 5,6,7,8, '-'});

    Opcode_Fixture f(68, data, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK_EQUAL(f.distributor(), data);
    BOOST_CHECK_EQUAL(f.upper(), upper_sum);
    BOOST_CHECK_EQUAL(f.lower(), lower_sum);
    BOOST_CHECK(!f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(reset_and_subtract_absolute_into_lower_2)
{
    Word data({0,0, 1,2,3,4, 5,6,7,8, '-'});
    Address addr({1,0,0,0});
    Word upper({8,9, 4,2,7,1, 1,3,6,5, '+'});
    Word lower({9,9, 8,9,3,7, 4,6,2,7, '+'});
    Word distr(lower);
    Word upper_sum({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower_sum({0,0, 1,2,3,4, 5,6,7,8, '-'});

    Opcode_Fixture f(68, data, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK_EQUAL(f.distributor(), data);
    BOOST_CHECK_EQUAL(f.upper(), upper_sum);
    BOOST_CHECK_EQUAL(f.lower(), lower_sum);
    BOOST_CHECK(!f.computer.overflow());
}

// 19  MULT  Multiply
BOOST_AUTO_TEST_CASE(multiply_1)
{
    Word data({0,0, 1,2,3,4, 5,6,7,8, '+'});
    Address addr({1,0,0,0});
    Word upper({8,9, 4,2,7,1, 1,3,6,5, '+'});
    Word lower({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word distr(lower);
    Word upper_product({0,0, 1,1,0,4, 0,3,8,3, '+'});
    Word lower_product({4,9, 5,9,2,3, 0,4,7,0, '+'});

    Opcode_Fixture f(19, data, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK_EQUAL(f.distributor(), data);
    BOOST_CHECK_EQUAL(f.upper(), upper_product);
    BOOST_CHECK_EQUAL(f.lower(), lower_product);
    BOOST_CHECK(!f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(multiply_2)
{
    Word data({0,0, 1,2,3,4, 5,6,7,8, '-'});
    Address addr({1,0,0,0});
    Word upper({8,9, 4,2,7,1, 1,3,6,5, '+'});
    Word lower({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word distr(lower);
    Word upper_product({0,0, 1,1,0,4, 0,3,8,3, '-'});
    Word lower_product({4,9, 5,9,2,3, 0,4,7,0, '-'});

    Opcode_Fixture f(19, data, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK_EQUAL(f.distributor(), data);
    BOOST_CHECK_EQUAL(f.upper(), upper_product);
    BOOST_CHECK_EQUAL(f.lower(), lower_product);
    BOOST_CHECK(!f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(multiply_3)
{
    Word data({0,0, 1,2,3,4, 5,6,7,8, '-'});
    Address addr({1,0,0,0});
    Word upper({8,9, 4,2,7,1, 1,3,6,5, '+'});
    Word lower({0,0, 0,0,0,0, 0,1,1,1, '+'});
    Word distr(lower);
    Word upper_product({0,0, 1,1,0,4, 0,4,9,4, '-'});
    Word lower_product({4,9, 5,9,2,3, 0,4,7,0, '-'});

    Opcode_Fixture f(19, data, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK_EQUAL(f.distributor(), data);
    BOOST_CHECK_EQUAL(f.upper(), upper_product);
    BOOST_CHECK_EQUAL(f.lower(), lower_product);
    BOOST_CHECK(!f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(multiply_4)
{
    Word data({0,0, 1,2,3,4, 5,6,7,8, '-'});
    Address addr({1,0,0,0});
    Word upper({8,9, 4,2,7,1, 1,3,6,5, '+'});
    Word lower({9,9, 9,9,9,9, 9,9,9,9, '+'});
    Word distr(lower);
    // Product overflows into multiplier; one extra addition is done and the most significant
    // digit of the product (1) is shifted out.
    Word upper_product({0,0, 1,1,0,4, 0,3,8,2, '-'});
    Word lower_product({4,9, 7,1,5,7, 6,1,4,8, '-'});

    Opcode_Fixture f(19, data, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK_EQUAL(f.distributor(), data);
    BOOST_CHECK_EQUAL(f.upper(), upper_product);
    BOOST_CHECK_EQUAL(f.lower(), lower_product);
    BOOST_CHECK( f.computer.overflow());
}

// 14  DIV  Divide
BOOST_AUTO_TEST_CASE(divide_1)
{
    Word data({0,0, 0,0,0,0, 0,1,2,5, '+'});
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word lower({0,0, 0,0,0,0, 4,6,8,2, '+'});
    Word distr(lower);
    Word remainder({0,0, 0,0,0,0, 0,0,5,7, '+'});
    Word quotient({0,0, 0,0,0,0, 0,0,3,7, '+'});

    Opcode_Fixture f(14, data, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK_EQUAL(f.distributor(), data);
    BOOST_CHECK_EQUAL(f.upper(), remainder);
    BOOST_CHECK_EQUAL(f.lower(), quotient);
    BOOST_CHECK(!f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(divide_2)
{
    Word data({0,0, 0,0,0,0, 0,1,2,5, '-'});
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word lower({0,0, 0,0,0,0, 4,6,8,2, '+'});
    Word distr(lower);
    Word remainder({0,0, 0,0,0,0, 0,0,5,7, '+'});
    Word quotient({0,0, 0,0,0,0, 0,0,3,7, '-'});

    Opcode_Fixture f(14, data, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK_EQUAL(f.distributor(), data);
    BOOST_CHECK_EQUAL(f.upper(), remainder);
    BOOST_CHECK_EQUAL(f.lower(), quotient);
    BOOST_CHECK(!f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(divide_3)
{
    Word data({0,0, 0,0,0,0, 0,1,2,5, '+'});
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower({0,0, 0,0,0,0, 4,6,8,2, '-'});
    Word distr(lower);
    Word remainder({0,0, 0,0,0,0, 0,0,5,7, '-'});
    Word quotient({0,0, 0,0,0,0, 0,0,3,7, '-'});

    Opcode_Fixture f(14, data, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK_EQUAL(f.distributor(), data);
    BOOST_CHECK_EQUAL(f.upper(), remainder);
    BOOST_CHECK_EQUAL(f.lower(), quotient);
    BOOST_CHECK(!f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(divide_4)
{
    Word data({0,0, 0,0,0,0, 0,1,2,5, '+'});
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,8,5,2, '+'});
    Word lower({1,0, 3,4,0,1, 0,2,0,0, '+'});
    Word distr(lower);
    // The manual doesn't specify the remainder and quotient when overflow  occurs.  Here's
    // what we currently get.
    Word remainder({0,0, 0,0,0,0, 7,2,7,1, '+'});
    Word quotient({0,3, 4,0,1,0, 2,0,0,9, '+'});

    Opcode_Fixture f(14, data, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK_EQUAL(f.distributor(), data);
    BOOST_CHECK_EQUAL(f.upper(), remainder);
    BOOST_CHECK_EQUAL(f.lower(), quotient);
    BOOST_CHECK(f.computer.overflow());
}

// 64  DIV RU  Divide and Reset Upper
BOOST_AUTO_TEST_CASE(divide_and_reset_upper_1)
{
    Word data({0,0, 0,0,0,0, 0,1,2,5, '+'});
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word lower({0,0, 0,0,0,0, 4,6,8,2, '+'});
    Word distr(lower);
    Word remainder({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word quotient({0,0, 0,0,0,0, 0,0,3,7, '+'});

    Opcode_Fixture f(64, data, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK_EQUAL(f.distributor(), data);
    BOOST_CHECK_EQUAL(f.upper(), remainder);
    BOOST_CHECK_EQUAL(f.lower(), quotient);
    BOOST_CHECK(!f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(divide_and_reset_upper_2)
{
    Word data({0,0, 0,0,0,0, 0,1,2,5, '-'});
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word lower({0,0, 0,0,0,0, 4,6,8,2, '+'});
    Word distr(lower);
    Word remainder({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word quotient({0,0, 0,0,0,0, 0,0,3,7, '-'});

    Opcode_Fixture f(64, data, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK_EQUAL(f.distributor(), data);
    BOOST_CHECK_EQUAL(f.upper(), remainder);
    BOOST_CHECK_EQUAL(f.lower(), quotient);
    BOOST_CHECK(!f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(divide_and_reset_upper_3)
{
    Word data({0,0, 0,0,0,0, 0,1,2,5, '+'});
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower({0,0, 0,0,0,0, 4,6,8,2, '-'});
    Word distr(lower);
    Word remainder({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word quotient({0,0, 0,0,0,0, 0,0,3,7, '-'});

    Opcode_Fixture f(64, data, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK_EQUAL(f.distributor(), data);
    BOOST_CHECK_EQUAL(f.upper(), remainder);
    BOOST_CHECK_EQUAL(f.lower(), quotient);
    BOOST_CHECK(!f.computer.overflow());
}

BOOST_AUTO_TEST_CASE(divide_and_reset_upper_4)
{
    Word data({0,0, 0,0,0,0, 0,1,2,5, '+'});
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,8,5,2, '+'});
    Word lower({1,0, 3,4,0,1, 0,2,0,0, '+'});
    Word distr(lower);
    // The manual doesn't specify the remainder and quotient when overflow  occurs.  Here's
    // what we currently get.
    Word remainder({0,0, 0,0,0,0, 7,2,7,1, '+'});
    Word quotient({0,3, 4,0,1,0, 2,0,0,9, '+'});

    Opcode_Fixture f(64, data, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK_EQUAL(f.distributor(), data);
    BOOST_CHECK_EQUAL(f.upper(), remainder);
    BOOST_CHECK_EQUAL(f.lower(), quotient);
    BOOST_CHECK(f.computer.overflow());
}

struct Branch_Fixture : public Opcode_Fixture
{
    Branch_Fixture(int opcode,
                   const Address& addr,
                   const Word& upper,
                   const Word& lower,
                   const Word& distr)
        : Opcode_Fixture(opcode, Word(), addr, upper, lower, distr),
          LD({6,9, 0,0,0,0, 0,0,0,0, '+'})
        {
            // Put the "load yourself" instruction at the data address.
            LD.load(addr, 0, 2);
            LD.load(Opcode_Fixture::stop_address, 0, 6);
            computer.set_drum(addr, LD);

            // Modify the instruction to bypass the stop instruction
            Word inst = drum(Opcode_Fixture::start_address);
            Address addr3({0,0,3,0});
            inst.load(addr3, 0, 6);
            computer.set_drum(Address(Opcode_Fixture::start_address), inst);

            // If we don't branch, the stop instruction is loaded into the distributor.
            Word load_stop({6,9, 0,0,0,0, 0,0,0,0, '+'});
            load_stop.load(Opcode_Fixture::stop_address, 0, 2);
            load_stop.load(Opcode_Fixture::stop_address, 0, 6);
            computer.set_drum(addr3, load_stop);
        }
    /// @return true if the branch was taken 
    bool did_branch() {
        return distributor() == LD;
    }

    /// @return true if an error prevented getting to the stop instruction.
    bool error_stop() {
        return !did_branch() && distributor() != drum(Opcode_Fixture::stop_address);
    }
    Word LD;
};

// 44  BRNZU  Branch on Non-Zero in Upper
BOOST_AUTO_TEST_CASE(branch_on_non_zero_in_upper_1)
{
    Address addr({1,0,0,0});
    Word upper({0,0, 1,2,3,4, 5,6,7,8, '+'});
    Word lower({0,0, 0,8,7,6, 4,3,2,9, '+'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Branch_Fixture f(44, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK(f.did_branch());
    BOOST_CHECK_EQUAL(f.upper(), upper);
    BOOST_CHECK_EQUAL(f.lower(), lower);
}

BOOST_AUTO_TEST_CASE(branch_on_non_zero_in_upper_2)
{
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,1,2,3, '+'});
    Word lower({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Branch_Fixture f(44, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK(f.did_branch());
    BOOST_CHECK_EQUAL(f.upper(), upper);
    BOOST_CHECK_EQUAL(f.lower(), lower);
}

BOOST_AUTO_TEST_CASE(branch_on_non_zero_in_upper_3)
{
    Address addr({1,0,0,0});
    Word upper({0,0, 1,2,3,4, 5,6,7,8, '-'});
    Word lower({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Branch_Fixture f(44, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK(f.did_branch());
    BOOST_CHECK_EQUAL(f.upper(), upper);
    BOOST_CHECK_EQUAL(f.lower(), lower);
}

BOOST_AUTO_TEST_CASE(branch_on_non_zero_in_upper_4)
{
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word lower({0,0, 1,2,3,4, 5,6,7,8, '+'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Branch_Fixture f(44, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK(!f.did_branch());
    BOOST_CHECK_EQUAL(f.upper(), upper);
    BOOST_CHECK_EQUAL(f.lower(), lower);
}

BOOST_AUTO_TEST_CASE(branch_on_non_zero_in_upper_5)
{
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word lower({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Branch_Fixture f(44, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK(!f.did_branch());
    BOOST_CHECK_EQUAL(f.upper(), upper);
    BOOST_CHECK_EQUAL(f.lower(), lower);
}

BOOST_AUTO_TEST_CASE(branch_on_non_zero_in_upper_6)
{
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Branch_Fixture f(44, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK(!f.did_branch());
    BOOST_CHECK_EQUAL(f.upper(), upper);
    BOOST_CHECK_EQUAL(f.lower(), lower);
}

// 45  BRNZ  Branch on Non-Zero
BOOST_AUTO_TEST_CASE(branch_on_non_zero_1)
{
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,1,2,3, '+'});
    Word lower({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Branch_Fixture f(45, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK(f.did_branch());
    BOOST_CHECK_EQUAL(f.upper(), upper);
    BOOST_CHECK_EQUAL(f.lower(), lower);
}

BOOST_AUTO_TEST_CASE(branch_on_non_zero_2)
{
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word lower({0,0, 0,0,0,0, 0,1,2,3, '+'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Branch_Fixture f(45, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK(f.did_branch());
    BOOST_CHECK_EQUAL(f.upper(), upper);
    BOOST_CHECK_EQUAL(f.lower(), lower);
}

BOOST_AUTO_TEST_CASE(branch_on_non_zero_3)
{
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower({0,0, 0,0,0,0, 0,1,2,3, '-'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Branch_Fixture f(45, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK(f.did_branch());
    BOOST_CHECK_EQUAL(f.upper(), upper);
    BOOST_CHECK_EQUAL(f.lower(), lower);
}

BOOST_AUTO_TEST_CASE(branch_on_non_zero_4)
{
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word lower({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Branch_Fixture f(45, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK(!f.did_branch());
    BOOST_CHECK_EQUAL(f.upper(), upper);
    BOOST_CHECK_EQUAL(f.lower(), lower);
}

BOOST_AUTO_TEST_CASE(branch_on_non_zero_5)
{
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Branch_Fixture f(45, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK(!f.did_branch());
    BOOST_CHECK_EQUAL(f.upper(), upper);
    BOOST_CHECK_EQUAL(f.lower(), lower);
}

// 46  BRMIN  Branch on Minus
BOOST_AUTO_TEST_CASE(branch_on_minus_1)
{
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower({0,0, 0,0,0,0, 0,1,2,3, '-'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Branch_Fixture f(46, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK(f.did_branch());
    BOOST_CHECK_EQUAL(f.upper(), upper);
    BOOST_CHECK_EQUAL(f.lower(), lower);
}

BOOST_AUTO_TEST_CASE(branch_on_minus_2)
{
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,1,2,3, '-'});
    Word lower({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Branch_Fixture f(46, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK(f.did_branch());
    BOOST_CHECK_EQUAL(f.upper(), upper);
    BOOST_CHECK_EQUAL(f.lower(), lower);
}

BOOST_AUTO_TEST_CASE(branch_on_minus_3)
{
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Branch_Fixture f(46, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK(f.did_branch());
    BOOST_CHECK_EQUAL(f.upper(), upper);
    BOOST_CHECK_EQUAL(f.lower(), lower);
}

BOOST_AUTO_TEST_CASE(branch_on_minus_4)
{
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word lower({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Branch_Fixture f(46, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK(!f.did_branch());
    BOOST_CHECK_EQUAL(f.upper(), upper);
    BOOST_CHECK_EQUAL(f.lower(), lower);
}

BOOST_AUTO_TEST_CASE(branch_on_minus_5)
{
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word lower({0,0, 0,0,0,0, 0,1,2,3, '+'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Branch_Fixture f(46, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK(!f.did_branch());
    BOOST_CHECK_EQUAL(f.upper(), upper);
    BOOST_CHECK_EQUAL(f.lower(), lower);
}

BOOST_AUTO_TEST_CASE(branch_on_minus_6)
{
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word lower({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word distr({0,0, 0,0,0,0, 0,1,2,3, '+'});

    Branch_Fixture f(46, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK(!f.did_branch());
    BOOST_CHECK_EQUAL(f.upper(), upper);
    BOOST_CHECK_EQUAL(f.lower(), lower);
}

struct Branch_on_Overflow_Fixture : public Opcode_Fixture
{
    Branch_on_Overflow_Fixture(int opcode,
                               const Word& upper,
                               const Word& lower,
                               const Word& distr)
        : Opcode_Fixture(opcode, Word(), Address({1,0,0,0}), upper, lower, distr)
        {
            computer.set_overflow_mode(Computer::Overflow_Mode::sense);
            // On overflow, load the branch instruction into the distributor.
            computer.set_drum(Address({0,0,1,0}), Word({1,0, 0,0,4,0, 0,0,2,0, '+'}));
            computer.set_drum(Address({0,0,2,0}), Word({4,7, 0,0,4,0, 0,0,3,0, '+'}));
            computer.set_drum(Address({0,0,3,0}), Word({0,1, 0,0,0,0, 0,0,0,0, '+'}));
            computer.set_drum(Address({0,0,4,0}), Word({6,9, 0,0,1,0, 0,0,3,0, '+'}));
        }
    /// @return true if the branch was taken, i.e. the distributor has the word at address 0010
    /// from the load instruction at 0040.
    bool did_branch() {
        return distributor() == drum(Address({0,0,1,0}));
    }
};

// 47  BROV  Branch on Overflow
BOOST_AUTO_TEST_CASE(branch_on_overflow_1)
{
    Word upper({5,0, 0,0,0,0, 0,0,0,0, '+'});
    Word lower({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Branch_on_Overflow_Fixture f(10, upper, lower, distr);
    f.run();
    BOOST_CHECK(f.computer.overflow());
    BOOST_CHECK(f.did_branch());
    // Upper has the low 10 digits of 50... + the word at address 0040.
    BOOST_CHECK_EQUAL(f.upper(), Word({1,9, 0,0,1,0, 0,0,3,0, '+'}));
    BOOST_CHECK_EQUAL(f.lower(), lower);
}

BOOST_AUTO_TEST_CASE(branch_on_overflow_2)
{
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word lower({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Branch_on_Overflow_Fixture f(10, upper, lower, distr);
    f.run();
    BOOST_CHECK(!f.computer.overflow());
    BOOST_CHECK(!f.did_branch());
    // Upper has 0 + the word at address 0040.
    BOOST_CHECK_EQUAL(f.upper(), Word({6,9, 0,0,1,0, 0,0,3,0, '+'}));
    BOOST_CHECK_EQUAL(f.lower(), lower);
}

BOOST_AUTO_TEST_CASE(branch_on_overflow_3)
{
    Word upper({5,0, 0,0,0,0, 0,0,0,0, '+'});
    Word lower({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Branch_on_Overflow_Fixture f(10, upper, lower, distr);
    // Stop the program on overflow.
    f.computer.set_overflow_mode(Computer::Overflow_Mode::stop);
    f.run();
    BOOST_CHECK(f.computer.overflow());
    // No branch. Program stopped on overflow.
    BOOST_CHECK(!f.did_branch());
    // Upper has the low 10 digits of 50... + the word at address 0040.
    BOOST_CHECK_EQUAL(f.upper(), Word({1,9, 0,0,1,0, 0,0,3,0, '+'}));
    BOOST_CHECK_EQUAL(f.lower(), lower);
}

// 90-99  BRD 1-10  Branch on 8 in Distributor Position 1-10
BOOST_AUTO_TEST_CASE(branch_on_8_in_position_10_1)
{
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower({0,0, 0,0,0,0, 0,1,2,3, '-'});
    Word distr({8,0, 0,0,0,0, 0,0,0,0, '+'});

    Branch_Fixture f(90, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK(f.did_branch());
    BOOST_CHECK(!f.error_stop());
    BOOST_CHECK_EQUAL(f.upper(), upper);
    BOOST_CHECK_EQUAL(f.lower(), lower);
}

BOOST_AUTO_TEST_CASE(branch_on_8_in_position_10_2)
{
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower({0,0, 0,0,0,0, 0,1,2,3, '-'});
    Word distr({9,0, 0,0,0,0, 0,0,0,0, '+'});

    Branch_Fixture f(90, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK(!f.did_branch());
    BOOST_CHECK(!f.error_stop());
    BOOST_CHECK_EQUAL(f.upper(), upper);
    BOOST_CHECK_EQUAL(f.lower(), lower);
}

BOOST_AUTO_TEST_CASE(branch_on_8_in_position_10_3)
{
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower({0,0, 0,0,0,0, 0,1,2,3, '-'});
    Word distr({0,9, 8,0,0,0, 0,0,0,0, '+'});

    Branch_Fixture f(90, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK(!f.did_branch());
    BOOST_CHECK(f.error_stop());
    BOOST_CHECK_EQUAL(f.upper(), upper);
    BOOST_CHECK_EQUAL(f.lower(), lower);
}

BOOST_AUTO_TEST_CASE(branch_on_8_in_position_1_1)
{
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower({0,0, 0,0,0,0, 0,1,2,3, '-'});
    Word distr({0,1, 2,3,4,5, 6,7,8,8, '+'});

    Branch_Fixture f(91, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK(f.did_branch());
    BOOST_CHECK(!f.error_stop());
    BOOST_CHECK_EQUAL(f.upper(), upper);
    BOOST_CHECK_EQUAL(f.lower(), lower);
}

BOOST_AUTO_TEST_CASE(branch_on_8_in_position_1_2)
{
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower({0,0, 0,0,0,0, 0,1,2,3, '-'});
    Word distr({0,1, 2,3,4,5, 6,7,8,9, '+'});

    Branch_Fixture f(91, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK(!f.did_branch());
    BOOST_CHECK(!f.error_stop());
    BOOST_CHECK_EQUAL(f.upper(), upper);
    BOOST_CHECK_EQUAL(f.lower(), lower);
}

BOOST_AUTO_TEST_CASE(branch_on_8_in_position_2_1)
{
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower({0,0, 0,0,0,0, 0,1,2,3, '-'});
    Word distr({0,1, 2,3,4,5, 6,7,8,8, '+'});

    Branch_Fixture f(92, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK(f.did_branch());
    BOOST_CHECK(!f.error_stop());
    BOOST_CHECK_EQUAL(f.upper(), upper);
    BOOST_CHECK_EQUAL(f.lower(), lower);
}

BOOST_AUTO_TEST_CASE(branch_on_8_in_position_2_2)
{
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower({0,0, 0,0,0,0, 0,1,2,3, '-'});
    Word distr({0,1, 2,3,4,5, 6,7,9,8, '+'});

    Branch_Fixture f(92, addr, upper, lower, distr);
    f.run();
    BOOST_CHECK(!f.did_branch());
    BOOST_CHECK(!f.error_stop());
    BOOST_CHECK_EQUAL(f.upper(), upper);
    BOOST_CHECK_EQUAL(f.lower(), lower);
}
