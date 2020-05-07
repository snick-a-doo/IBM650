#include "computer.hpp"
#include "test_fixture.hpp"
#include "doctest.h"

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
    Opcode_Fixture(int opcode, const Address& addr,
                   const Word& upper, const Word& lower, const Word& distr)
        : Opcode_Fixture(opcode, Word(), addr, upper, lower, distr)
        {}
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
TEST_CASE("load distributor from drum")
{
    Word data({0,0, 0,0,1,2, 3,4,5,6, '+'});
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word lower({0,0, 0,0,6,4, 3,2,1,7, '+'});
    Word distr(lower);

    Opcode_Fixture f(69, data, addr, upper, lower, distr);
    f.run();
    CHECK(f.distributor() == data);
    CHECK(f.upper() == upper);
    CHECK(f.lower() == lower);
}

TEST_CASE("load distributor from upper accumulator")
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
    CHECK(f.distributor() == new_distr);
    CHECK(f.upper() == upper);
    CHECK(f.lower() == lower);
}

TEST_CASE("load distributor from lower accumulator")
{
    Word data({0,0, 0,0,1,2, 3,4,5,6, '+'});
    Address addr({8,0,0,2});
    Word upper({1,2, 3,4,5,6, 7,8,9,0, '-'});
    Word lower({3,8, 3,8,5,6, 7,8,9,0, '-'});
    Word distr(lower);

    Opcode_Fixture f(69, data, addr, upper, lower, distr);
    f.run();
    CHECK(f.distributor() == lower);
    CHECK(f.upper() == upper);
    CHECK(f.lower() == lower);
}

// 24  STD  Store Distributor
TEST_CASE("store distributor")
{
    Address addr({1,0,1,0});
    Word distr({0,0, 0,1,0,4, 2,6,6,6, '-'});

    Opcode_Fixture f(24, addr, distr);
    f.run();
    CHECK(f.distributor() == distr);
    CHECK(f.drum(addr) == distr);
}

TEST_CASE("store distributor from 800x addresses")
{
    { Opcode_Fixture f(24, Address({2,0,0,0}));
        f.run();
        CHECK(f.computer.storage_selection_error()); }
    { Opcode_Fixture f(24, Address({8,0,0,3}));
        f.run();
        CHECK(f.computer.storage_selection_error()); }
    { Opcode_Fixture f(24, Address({8,0,0,1}));
        f.run();
        CHECK(f.computer.storage_selection_error()); }
    { Opcode_Fixture f(24, Address({8,0,0,2}));
        f.run();
        CHECK(f.computer.storage_selection_error()); }
    { Opcode_Fixture f(24, Address({8,0,0,3}));
        f.run();
        CHECK(f.computer.storage_selection_error()); }
}

// 10  AU  Add to Upper
TEST_CASE("add to upper accumulator")
{
    Word data({0,0, 1,2,3,4, 5,6,7,8, '+'});
    SUBCASE("add positive")
    {
        Address addr({1,0,0,0});
        Word upper({0,0, 0,0,4,5, 8,6,3,2, '+'});
        Word lower({8,9, 4,2,7,1, 1,3,6,5, '+'});
        Word distr(lower);
        Word sum({0,0, 1,2,8,0, 4,3,1,0, '+'});

        Opcode_Fixture f(10, data, addr, upper, lower, distr);
        f.run();
        CHECK(f.distributor() == data);
        CHECK(f.upper() == sum);
        CHECK(f.lower() == lower);
        CHECK(!f.computer.overflow());
    }
    SUBCASE("add negative")
    {
        Address addr({1,0,0,0});
        Word upper({0,0, 0,0,4,5, 8,6,3,2, '-'});
        Word lower({8,9, 4,2,7,1, 1,3,6,5, '-'});
        Word distr(lower);
        Word upper_sum({0,0, 1,1,8,8, 7,0,4,5, '-'});
        Word lower_sum({1,0, 5,7,2,8, 8,6,3,5, '+'});

        Opcode_Fixture f(10, data, addr, upper, lower, distr);
        f.run();
        CHECK(f.distributor() == data);
        CHECK(f.upper() == upper_sum);
        CHECK(f.lower() == lower_sum);
        CHECK(!f.computer.overflow());
    }
    SUBCASE("overflow")
    {
        Address addr({1,0,0,0});
        Word upper({9,9, 8,9,3,7, 4,6,2,7, '+'});
        Word lower({8,9, 4,2,7,1, 1,3,6,5, '+'});
        Word distr(lower);
        Word sum({0,0, 0,1,7,2, 0,3,0,5, '+'});

        Opcode_Fixture f(10, data, addr, upper, lower, distr);
        f.run();
        CHECK(f.distributor() == data);
        CHECK(f.upper() == sum);
        CHECK(f.lower() == lower);
        CHECK(f.computer.overflow());
    }
    SUBCASE("add upper accumulator to upper accumulator")
    {
        Address addr({8,0,0,3});
        Word upper({1,2, 3,4,5,6, 7,8,9,0, '+'});
        Word lower({3,8, 3,8,5,6, 7,8,9,0, '+'});
        Word distr(lower);
        Word sum({2,4, 6,9,1,3, 5,7,8,0, '+'});

        Opcode_Fixture f(10, distr, addr, upper, lower, distr);
        f.run();
        CHECK(f.distributor() == upper);
        CHECK(f.upper() == sum);
        CHECK(f.lower() == lower);
        CHECK(!f.computer.overflow());
    }
    SUBCASE("add lower accumulator to upper accumulator")
    {
        Address addr({8,0,0,2});
        Word upper({1,2, 3,4,5,6, 7,8,9,0, '+'});
        Word lower({3,8, 3,8,5,6, 7,8,9,0, '+'});
        Word distr(upper);
        Word sum({5,0, 7,3,1,3, 5,7,8,0, '+'});

        Opcode_Fixture f(10, distr, addr, upper, lower, distr);
        f.run();
        CHECK(f.distributor() == lower);
        CHECK(f.upper() == sum);
        CHECK(f.lower() == lower);
        CHECK(!f.computer.overflow());
    }
    SUBCASE("add distributor to upper accumulator")
    {
        Address addr({8,0,0,1});
        Word upper({1,2, 3,4,5,6, 7,8,9,0, '+'});
        Word lower({3,8, 3,8,5,6, 7,8,9,0, '+'});
        Word distr(change_sign(upper));
        Word sum({0,0, 0,0,0,0, 0,0,0,0, '+'});

        Opcode_Fixture f(10, distr, addr, upper, lower, distr);
        f.run();
        CHECK(f.distributor() == distr);
        CHECK(f.upper() == sum);
        CHECK(f.lower() == lower);
        CHECK(!f.computer.overflow());
    }
}
// 15  AL  Add to Lower
TEST_CASE("add to lower")
{
    Word data({0,0, 1,2,3,4, 5,6,7,8, '+'});
    SUBCASE("add positive")
    {
        Address addr({1,0,0,0});
        Word upper({0,0, 0,0,0,0, 0,0,0,0, '+'});
        Word lower({9,9, 8,9,3,7, 4,6,2,7, '+'});
        Word distr(lower);
        Word upper_sum({0,0, 0,0,0,0, 0,0,0,1, '+'});
        Word lower_sum({0,0, 0,1,7,2, 0,3,0,5, '+'});

        Opcode_Fixture f(15, data, addr, upper, lower, distr);
        f.run();
        CHECK(f.distributor() == data);
        CHECK(f.upper() == upper_sum);
        CHECK(f.lower() == lower_sum);
        CHECK(!f.computer.overflow());
    }
    SUBCASE("add negative")
    {
        Address addr({1,0,0,0});
        Word upper({0,0, 0,0,0,0, 0,0,0,0, '-'});
        Word lower({9,9, 8,9,3,7, 4,6,2,7, '-'});
        Word distr(lower);
        Word upper_sum({0,0, 0,0,0,0, 0,0,0,0, '-'});
        Word lower_sum({9,9, 7,7,0,2, 8,9,4,9, '-'});

        Opcode_Fixture f(15, data, addr, upper, lower, distr);
        f.run();
        CHECK(f.distributor() == data);
        CHECK(f.upper() == upper_sum);
        CHECK(f.lower() == lower_sum);
        CHECK(!f.computer.overflow());
    }
    SUBCASE("carry to upper accumulator")
    {
        Address addr({1,0,0,0});
        Word upper({0,0, 0,0,0,0, 0,0,0,0, '-'});
        Word lower({9,9, 8,9,3,7, 4,6,2,7, '-'});
        Word distr(lower);
        Word upper_sum({0,0, 0,0,0,0, 0,0,0,1, '-'});
        Word lower_sum({0,0, 0,1,7,2, 0,3,0,5, '-'});

        Opcode_Fixture f(15, change_sign(data), addr, upper, lower, distr);
        f.run();
        CHECK(f.distributor() == change_sign(data));
        CHECK(f.upper() == upper_sum);
        CHECK(f.lower() == lower_sum);
        CHECK(!f.computer.overflow());
    }
    SUBCASE("add upper accumulator to lower accumulator")
    {
        Address addr({8,0,0,3});
        Word upper({9,8, 7,6,5,4, 3,2,1,0, '+'});
        Word lower({3,8, 3,8,5,6, 7,8,9,0, '+'});
        Word distr(lower);
        Word upper_sum({9,8, 7,6,5,4, 3,2,1,1, '+'});
        Word lower_sum({3,7, 1,5,1,1, 1,1,0,0, '+'});

        Opcode_Fixture f(15, distr, addr, upper, lower, distr);
        f.run();
        CHECK(f.distributor() == upper);
        CHECK(f.upper() == upper_sum);
        CHECK(f.lower() == lower_sum);
        CHECK(!f.computer.overflow());
    }
    SUBCASE("add lower accumulator to lower accumulator")
    {
        Address addr({8,0,0,2});
        Word upper({9,8, 7,6,5,4, 3,2,1,0, '+'});
        Word lower({3,8, 3,8,5,6, 7,8,9,0, '+'});
        Word distr(lower);
        Word lower_sum({7,6, 7,7,1,3, 5,7,8,0, '+'});

        Opcode_Fixture f(15, distr, addr, upper, lower, distr);
        f.run();
        CHECK(f.distributor() == lower);
        CHECK(f.upper() == upper);
        CHECK(f.lower() == lower_sum);
        CHECK(!f.computer.overflow());
    }
    SUBCASE("add distributor to lower accumulator")
    {
        Address addr({8,0,0,1});
        Word upper({1,2, 3,4,5,6, 7,8,9,0, '+'});
        Word lower({3,8, 3,8,5,6, 7,8,9,0, '+'});
        Word distr(change_sign(lower));
        Word lower_sum({0,0, 0,0,0,0, 0,0,0,0, '+'});

        Opcode_Fixture f(15, distr, addr, upper, lower, distr);
        f.run();
        CHECK(f.distributor() == distr);
        CHECK(f.upper() == upper);
        CHECK(f.lower() == lower_sum);
        CHECK(!f.computer.overflow());
    }
}
// 11  SU  Subtract from Upper
TEST_CASE("subtract from upper")
{
    SUBCASE("subtract negative from negative accumulator")
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
        CHECK(f.distributor() == data);
        CHECK(f.upper() == upper_sum);
        CHECK(f.lower() == lower_sum);
        CHECK(!f.computer.overflow());
    }
    SUBCASE("subtract positive from negative accumulator")
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
        CHECK(f.distributor() == data);
        CHECK(f.upper() == upper_sum);
        CHECK(f.lower() == lower_sum);
        CHECK(!f.computer.overflow());
    }
    SUBCASE("overflow")
    {
        Word data({0,0, 1,2,3,4, 5,6,7,8, '+'});
        Address addr({1,0,0,0});
        Word upper({9,9, 8,9,3,7, 4,6,2,7, '-'});
        Word lower({8,9, 4,2,7,1, 1,3,6,5, '-'});
        Word distr(upper);
        Word sum({0,0, 0,1,7,2, 0,3,0,5, '-'});

        Opcode_Fixture f(11, data, addr, upper, lower, distr);
        f.run();
        CHECK(f.distributor() == data);
        CHECK(f.upper() == sum);
        CHECK(f.lower() == lower);
        CHECK(f.computer.overflow());
    }
    SUBCASE("subtract upper accumulator from upper accumulator")
    {
        Address addr({8,0,0,3});
        Word upper({1,2, 3,4,5,6, 7,8,9,0, '+'});
        Word lower({3,8, 3,8,5,6, 7,8,9,0, '+'});
        Word distr(lower);
        Word sum({0,0, 0,0,0,0, 0,0,0,0, '+'});

        Opcode_Fixture f(11, distr, addr, upper, lower, distr);
        f.run();
        CHECK(f.distributor() == upper);
        CHECK(f.upper() == sum);
        CHECK(f.lower() == lower);
        CHECK(!f.computer.overflow());
    }
    SUBCASE("subtract lower accumulator from upper accumulator")
    {
        Address addr({8,0,0,2});
        Word upper({1,2, 3,4,5,6, 7,8,9,0, '+'});
        Word lower({3,8, 3,8,5,6, 7,8,9,0, '+'});
        Word distr(upper);
        Word upper_sum({2,6, 0,3,9,9, 9,9,9,9, '+'});
        Word lower_sum({6,1, 6,1,4,3, 2,1,1,0, '-'});

        Opcode_Fixture f(11, distr, addr, upper, lower, distr);
        f.run();
        CHECK(f.distributor() == lower);
        CHECK(f.upper() == upper_sum);
        CHECK(f.lower() == lower_sum);
        CHECK(!f.computer.overflow());
    }
    SUBCASE("subtract distributor from upper accumulator")
    {
        Address addr({8,0,0,1});
        Word upper({1,2, 3,4,5,6, 7,8,9,0, '+'});
        Word lower({3,8, 3,8,5,6, 7,8,9,0, '+'});
        Word distr(upper);
        Word sum({0,0, 0,0,0,0, 0,0,0,0, '+'});

        Opcode_Fixture f(11, distr, addr, upper, lower, distr);
        f.run();
        CHECK(f.distributor() == distr);
        CHECK(f.upper() == sum);
        CHECK(f.lower() == lower);
        CHECK(!f.computer.overflow());
    }
}

// 16  SL  Subtract from Lower
TEST_CASE("subtract from lower")
{
    SUBCASE("subtract positive")
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
        CHECK(f.distributor() == data);
        CHECK(f.upper() == upper_sum);
        CHECK(f.lower() == lower_sum);
        CHECK(!f.computer.overflow());
    }
    SUBCASE("subtract negative carry to upper accumalator")
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
        CHECK(f.distributor() == data);
        CHECK(f.upper() == upper_sum);
        CHECK(f.lower() == lower_sum);
        CHECK(!f.computer.overflow());
    }
    SUBCASE("subtract negative")
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
        CHECK(f.distributor() == data);
        CHECK(f.upper() == upper_sum);
        CHECK(f.lower() == lower_sum);
        CHECK(!f.computer.overflow());
    }
    SUBCASE("subtract upper accumulator from lower accumulator")
    {
        Address addr({8,0,0,3});
        Word upper({9,8, 7,6,5,4, 3,2,1,0, '+'});
        Word lower({3,8, 3,8,5,6, 7,8,9,0, '+'});
        Word distr(lower);
        Word upper_sum({9,8, 7,6,5,4, 3,2,0,9, '+'});
        Word lower_sum({3,9, 6,2,0,2, 4,6,8,0, '+'});

        Opcode_Fixture f(16, distr, addr, upper, lower, distr);
        f.run();
        CHECK(f.distributor() == upper);
        CHECK(f.upper() == upper_sum);
        CHECK(f.lower() == lower_sum);
        CHECK(!f.computer.overflow());
    }
    SUBCASE("subtract lower accumulator from lower accumulator")
    {
        Address addr({8,0,0,2});
        Word upper({1,2, 3,4,5,6, 7,8,9,0, '+'});
        Word lower({3,8, 3,8,5,6, 7,8,9,0, '+'});
        Word distr(lower);
        Word lower_sum({0,0, 0,0,0,0, 0,0,0,0, '+'});

        Opcode_Fixture f(16, distr, addr, upper, lower, distr);
        f.run();
        CHECK(f.distributor() == lower);
        CHECK(f.upper() == upper);
        CHECK(f.lower() == lower_sum);
        CHECK(!f.computer.overflow());
    }
    SUBCASE("subtract distributor from lower accumulator")
    {
        Address addr({8,0,0,1});
        Word upper({1,2, 3,4,5,6, 7,8,9,0, '+'});
        Word lower({3,8, 3,8,5,6, 7,8,9,0, '+'});
        Word distr(lower);
        Word lower_sum({0,0, 0,0,0,0, 0,0,0,0, '+'});

        Opcode_Fixture f(16, distr, addr, upper, lower, distr);
        f.run();
        CHECK(f.distributor() == distr);
        CHECK(f.upper() == upper);
        CHECK(f.lower() == lower_sum);
        CHECK(!f.computer.overflow());
    }
}

// 60  RAU  Reset and Add into Upper
TEST_CASE("reset and add into upper 1")
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
    CHECK(f.distributor() == data);
    CHECK(f.upper() == upper_sum);
    CHECK(f.lower() == lower_sum);
    CHECK(!f.computer.overflow());
}

TEST_CASE("reset and add into upper 2")
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
    CHECK(f.distributor() == data);
    CHECK(f.upper() == upper_sum);
    CHECK(f.lower() == lower_sum);
    CHECK(!f.computer.overflow());
}

TEST_CASE("reset and add into upper 8003")
{
    Address addr({8,0,0,3});
    Word upper({1,2, 3,4,5,6, 7,8,9,0, '+'});
    Word lower({3,8, 3,8,5,6, 7,8,9,0, '+'});
    Word distr(lower);
    Word upper_sum(upper);
    Word lower_sum({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Opcode_Fixture f(60, distr, addr, upper, lower, distr);
    f.run();
    CHECK(f.distributor() == upper);
    CHECK(f.upper() == upper_sum);
    CHECK(f.lower() == lower_sum);
    CHECK(!f.computer.overflow());
}

TEST_CASE("reset and add into upper 8002")
{
    Address addr({8,0,0,2});
    Word upper({1,2, 3,4,5,6, 7,8,9,0, '+'});
    Word lower({3,8, 3,8,5,6, 7,8,9,0, '+'});
    Word distr(upper);
    Word upper_sum(lower);
    Word lower_sum({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Opcode_Fixture f(60, distr, addr, upper, lower, distr);
    f.run();
    CHECK(f.distributor() == lower);
    CHECK(f.upper() == upper_sum);
    CHECK(f.lower() == lower_sum);
    CHECK(!f.computer.overflow());
}

TEST_CASE("reset and add into upper 8001")
{
    Address addr({8,0,0,1});
    Word upper({1,2, 3,4,5,6, 7,8,9,0, '+'});
    Word lower({3,8, 3,8,5,6, 7,8,9,0, '+'});
    Word distr(change_sign(upper));
    Word upper_sum(distr);
    Word lower_sum({0,0, 0,0,0,0, 0,0,0,0, '-'});

    Opcode_Fixture f(60, distr, addr, upper, lower, distr);
    f.run();
    CHECK(f.distributor() == distr);
    CHECK(f.upper() == upper_sum);
    CHECK(f.lower() == lower_sum);
    CHECK(!f.computer.overflow());
}

// 65  RAL  Reset and Add into Lower
TEST_CASE("reset and add into lower 1")
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
    CHECK(f.distributor() == data);
    CHECK(f.upper() == upper_sum);
    CHECK(f.lower() == lower_sum);
    CHECK(!f.computer.overflow());
}

TEST_CASE("reset and add into lower 2")
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
    CHECK(f.distributor() == data);
    CHECK(f.upper() == upper_sum);
    CHECK(f.lower() == lower_sum);
    CHECK(!f.computer.overflow());
}

TEST_CASE("reset and add into lower 8003")
{
    Address addr({8,0,0,3});
    Word upper({1,2, 3,4,5,6, 7,8,9,0, '+'});
    Word lower({3,8, 3,8,5,6, 7,8,9,0, '+'});
    Word distr(lower);
    Word upper_sum({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word lower_sum(upper);

    Opcode_Fixture f(65, distr, addr, upper, lower, distr);
    f.run();
    CHECK(f.distributor() == upper);
    CHECK(f.upper() == upper_sum);
    CHECK(f.lower() == lower_sum);
    CHECK(!f.computer.overflow());
}

TEST_CASE("reset and add into lower 8002")
{
    Address addr({8,0,0,2});
    Word upper({1,2, 3,4,5,6, 7,8,9,0, '+'});
    Word lower({3,8, 3,8,5,6, 7,8,9,0, '+'});
    Word distr(lower);
    Word upper_sum({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word lower_sum(lower);

    Opcode_Fixture f(65, distr, addr, upper, lower, distr);
    f.run();
    CHECK(f.distributor() == lower);
    CHECK(f.upper() == upper_sum);
    CHECK(f.lower() == lower_sum);
    CHECK(!f.computer.overflow());
}

TEST_CASE("reset and add into lower 8001")
{
    Address addr({8,0,0,1});
    Word upper({1,2, 3,4,5,6, 7,8,9,0, '+'});
    Word lower({3,8, 3,8,5,6, 7,8,9,0, '+'});
    Word distr(change_sign(upper));
    Word upper_sum({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower_sum(distr);

    Opcode_Fixture f(65, distr, addr, upper, lower, distr);
    f.run();
    CHECK(f.distributor() == distr);
    CHECK(f.upper() == upper_sum);
    CHECK(f.lower() == lower_sum);
    CHECK(!f.computer.overflow());
}

// 61  RSU  Reset and Subtract into Upper
TEST_CASE("reset and subtract into upper 1")
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
    CHECK(f.distributor() == data);
    CHECK(f.upper() == upper_sum);
    CHECK(f.lower() == lower_sum);
    CHECK(!f.computer.overflow());
}

TEST_CASE("reset and subtract into upper 2")
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
    CHECK(f.distributor() == data);
    CHECK(f.upper() == upper_sum);
    CHECK(f.lower() == lower_sum);
    CHECK(!f.computer.overflow());
}

TEST_CASE("reset and subtract into upper 8003")
{
    Address addr({8,0,0,3});
    Word upper({1,2, 3,4,5,6, 7,8,9,0, '+'});
    Word lower({3,8, 3,8,5,6, 7,8,9,0, '+'});
    Word distr(lower);
    Word upper_sum(change_sign(upper));
    Word lower_sum({0,0, 0,0,0,0, 0,0,0,0, '-'});

    Opcode_Fixture f(61, distr, addr, upper, lower, distr);
    f.run();
    CHECK(f.distributor() == upper);
    CHECK(f.upper() == upper_sum);
    CHECK(f.lower() == lower_sum);
    CHECK(!f.computer.overflow());
}

TEST_CASE("reset and subtract into upper 8002")
{
    Address addr({8,0,0,2});
    Word upper({1,2, 3,4,5,6, 7,8,9,0, '+'});
    Word lower({3,8, 3,8,5,6, 7,8,9,0, '+'});
    Word distr(upper);
    Word upper_sum(change_sign(lower));
    Word lower_sum({0,0, 0,0,0,0, 0,0,0,0, '-'});

    Opcode_Fixture f(61, distr, addr, upper, lower, distr);
    f.run();
    CHECK(f.distributor() == lower);
    CHECK(f.upper() == upper_sum);
    CHECK(f.lower() == lower_sum);
    CHECK(!f.computer.overflow());
}

TEST_CASE("reset and subtract into upper 8001")
{
    Address addr({8,0,0,1});
    Word upper({1,2, 3,4,5,6, 7,8,9,0, '+'});
    Word lower({3,8, 3,8,5,6, 7,8,9,0, '+'});
    Word distr(upper);
    Word upper_sum(change_sign(distr));
    Word lower_sum({0,0, 0,0,0,0, 0,0,0,0, '-'});

    Opcode_Fixture f(61, distr, addr, upper, lower, distr);
    f.run();
    CHECK(f.distributor() == distr);
    CHECK(f.upper() == upper_sum);
    CHECK(f.lower() == lower_sum);
    CHECK(!f.computer.overflow());
}

// 66  RSL  Reset and Add into Lower
TEST_CASE("reset and subtract into lower 1")
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
    CHECK(f.distributor() == data);
    CHECK(f.upper() == upper_sum);
    CHECK(f.lower() == lower_sum);
    CHECK(!f.computer.overflow());
}

TEST_CASE("reset and subtract into lower 2")
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
    CHECK(f.distributor() == data);
    CHECK(f.upper() == upper_sum);
    CHECK(f.lower() == lower_sum);
    CHECK(!f.computer.overflow());
}

TEST_CASE("reset and subtract into lower 8003")
{
    Address addr({8,0,0,3});
    Word upper({1,2, 3,4,5,6, 7,8,9,0, '+'});
    Word lower({3,8, 3,8,5,6, 7,8,9,0, '+'});
    Word distr(lower);
    Word upper_sum({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower_sum(change_sign(upper));

    Opcode_Fixture f(66, distr, addr, upper, lower, distr);
    f.run();
    CHECK(f.distributor() == upper);
    CHECK(f.upper() == upper_sum);
    CHECK(f.lower() == lower_sum);
    CHECK(!f.computer.overflow());
}

TEST_CASE("reset and subtract into lower 8002")
{
    Address addr({8,0,0,2});
    Word upper({1,2, 3,4,5,6, 7,8,9,0, '-'});
    Word lower({3,8, 3,8,5,6, 7,8,9,0, '-'});
    Word distr(lower);
    Word upper_sum({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word lower_sum(change_sign(lower));

    Opcode_Fixture f(66, distr, addr, upper, lower, distr);
    f.run();
    CHECK(f.distributor() == lower);
    CHECK(f.upper() == upper_sum);
    CHECK(f.lower() == lower_sum);
    CHECK(!f.computer.overflow());
}

TEST_CASE("reset and subtract into lower 8001")
{
    Address addr({8,0,0,1});
    Word upper({1,2, 3,4,5,6, 7,8,9,0, '+'});
    Word lower({3,8, 3,8,5,6, 7,8,9,0, '+'});
    Word distr(upper);
    Word upper_sum({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower_sum(change_sign(distr));

    Opcode_Fixture f(66, distr, addr, upper, lower, distr);
    f.run();
    CHECK(f.distributor() == distr);
    CHECK(f.upper() == upper_sum);
    CHECK(f.lower() == lower_sum);
    CHECK(!f.computer.overflow());
}

// 20  STL  Store Lower in Memory
TEST_CASE("store lower in memory")
{
    Word data({0,0, 1,2,3,4, 5,6,7,8, '+'});
    Address addr({1,0,0,0});
    Word upper({0,4, 0,4,5,3, 2,8,7,7, '-'});
    Word lower({0,0, 0,0,5,5, 3,8,2,2, '-'});
    Word distr({0,0, 0,0,0,1, 2,9,9,8, '+'});

    Opcode_Fixture f(20, distr, addr, upper, lower, distr);
    f.run();
    CHECK(f.distributor() == lower);
    CHECK(f.upper() == upper);
    CHECK(f.lower() == lower);
    CHECK(f.drum(addr) == lower);
}

TEST_CASE("store lower in memory 800x")
{
    { Opcode_Fixture f(20, Address({2,0,0,0}));
        f.run();
        CHECK(f.computer.storage_selection_error()); }
    { Opcode_Fixture f(20, Address({8,0,0,3}));
        f.run();
        CHECK(f.computer.storage_selection_error()); }
    { Opcode_Fixture f(20, Address({8,0,0,1}));
        f.run();
        CHECK(f.computer.storage_selection_error()); }
    { Opcode_Fixture f(20, Address({8,0,0,2}));
        f.run();
        CHECK(f.computer.storage_selection_error()); }
    { Opcode_Fixture f(20, Address({8,0,0,3}));
        f.run();
        CHECK(f.computer.storage_selection_error()); }
}

// 21  STU  Store Upper in Memory
TEST_CASE("store upper in memory 1")
{
    Word data({0,0, 1,2,3,4, 5,6,7,8, '+'});
    Address addr({1,0,0,0});
    Word upper({0,4, 0,4,5,3, 2,8,7,7, '+'});
    Word lower({0,0, 0,0,5,5, 3,8,2,2, '+'});
    Word distr({0,0, 0,0,0,1, 2,9,9,8, '+'});

    Opcode_Fixture f(21, distr, addr, upper, lower, distr);
    f.run();
    CHECK(f.distributor() == upper);
    CHECK(f.upper() == upper);
    CHECK(f.lower() == lower);
    CHECK(f.drum(addr) == upper);
}

TEST_CASE("store upper in memory 2")
{
    Word data({0,0, 1,2,3,4, 5,6,7,8, '+'});
    Address addr({1,0,0,0});
    Word upper({0,4, 0,4,5,3, 2,8,7,7, '-'});
    Word lower({0,0, 0,0,5,5, 3,8,2,2, '-'});
    Word distr({0,0, 0,0,0,1, 2,9,9,8, '+'});

    Opcode_Fixture f(21, distr, addr, upper, lower, distr);
    f.run();
    CHECK(f.distributor() == upper);
    CHECK(f.upper() == upper);
    CHECK(f.lower() == lower);
    CHECK(f.drum(addr) == upper);
}

TEST_CASE("store upper in memory 3")
{
    Word data({0,1, 3,6,5,4, 3,2,1,0, '+'});
    Address addr({1,0,0,0});
    Word remainder({0,0, 0,6,3,4, 9,2,1,1, '-'});
    Word quotient({0,0, 1,4,9,8, 7,6,2,1, '+'});
    Word distr({0,0, 0,6,4,3, 2,1,9,8, '+'});

    Opcode_Fixture f(21, distr, addr, remainder, quotient, distr);
    f.run();
    CHECK(f.distributor() == remainder);
    CHECK(f.upper() == remainder);
    CHECK(f.lower() == quotient);
    CHECK(f.drum(addr) == remainder);
}

TEST_CASE("store upper in memory 800x")
{
    { Opcode_Fixture f(21, Address({2,0,0,0}));
        f.run();
        CHECK(f.computer.storage_selection_error()); }
    { Opcode_Fixture f(21, Address({8,0,0,3}));
        f.run();
        CHECK(f.computer.storage_selection_error()); }
    { Opcode_Fixture f(21, Address({8,0,0,1}));
        f.run();
        CHECK(f.computer.storage_selection_error()); }
    { Opcode_Fixture f(21, Address({8,0,0,2}));
        f.run();
        CHECK(f.computer.storage_selection_error()); }
    { Opcode_Fixture f(21, Address({8,0,0,3}));
        f.run();
        CHECK(f.computer.storage_selection_error()); }
}

// 22  STDA  Store Lower Data Address
TEST_CASE("store lower data address")
{
    Word data({6,9, 1,9,4,6, 0,6,0,0, '+'});
    Address addr({0,1,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower({0,0, 1,9,8,8, 0,5,6,0, '-'});
    Word distr({6,9, 1,9,5,0, 0,6,0,0, '+'});
    Word store({6,9, 1,9,8,8, 0,6,0,0, '+'});

    Opcode_Fixture f(22, distr, addr, upper, lower, distr);
    f.run();
    CHECK(f.distributor() == store);
    CHECK(f.upper() == upper);
    CHECK(f.lower() == lower);
    CHECK(f.drum(addr) == store);
}

TEST_CASE("store lower data address 800x")
{
    { Opcode_Fixture f(22, Address({2,0,0,0}));
        f.run();
        CHECK(f.computer.storage_selection_error()); }
    { Opcode_Fixture f(22, Address({8,0,0,3}));
        f.run();
        CHECK(f.computer.storage_selection_error()); }
    { Opcode_Fixture f(22, Address({8,0,0,1}));
        f.run();
        CHECK(f.computer.storage_selection_error()); }
    { Opcode_Fixture f(22, Address({8,0,0,2}));
        f.run();
        CHECK(f.computer.storage_selection_error()); }
    { Opcode_Fixture f(22, Address({8,0,0,3}));
        f.run();
        CHECK(f.computer.storage_selection_error()); }
}

// 23  STDA  Store Lower Instruction Address
TEST_CASE("store lower instruction address")
{
    Word data({6,9, 1,8,0,0, 0,7,1,5, '+'});
    Address addr({0,1,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower({0,0, 1,9,8,8, 0,8,1,6, '-'});
    Word distr({6,9, 1,8,0,0, 0,7,5,0, '+'});
    Word store({6,9, 1,8,0,0, 0,8,1,6, '+'});

    Opcode_Fixture f(23, distr, addr, upper, lower, distr);
    f.run();
    CHECK(f.distributor() == store);
    CHECK(f.upper() == upper);
    CHECK(f.lower() == lower);
    CHECK(f.drum(addr) == store);
}

TEST_CASE("store lower instruction address 800x")
{
    { Opcode_Fixture f(23, Address({2,0,0,0}));
        f.run();
        CHECK(f.computer.storage_selection_error()); }
    { Opcode_Fixture f(23, Address({8,0,0,3}));
        f.run();
        CHECK(f.computer.storage_selection_error()); }
    { Opcode_Fixture f(23, Address({8,0,0,1}));
        f.run();
        CHECK(f.computer.storage_selection_error()); }
    { Opcode_Fixture f(23, Address({8,0,0,2}));
        f.run();
        CHECK(f.computer.storage_selection_error()); }
    { Opcode_Fixture f(23, Address({8,0,0,3}));
        f.run();
        CHECK(f.computer.storage_selection_error()); }
}

// 17  AABL  Add Absolute to Lower
TEST_CASE("add absolute to lower 1")
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
    CHECK(f.distributor() == data);
    CHECK(f.upper() == upper_sum);
    CHECK(f.lower() == lower_sum);
    CHECK(!f.computer.overflow());
}

TEST_CASE("add absolute to lower 2")
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
    CHECK(f.distributor() == data);
    CHECK(f.upper() == upper_sum);
    CHECK(f.lower() == lower_sum);
    CHECK(!f.computer.overflow());
}

TEST_CASE("add absolute to lower 3")
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
    CHECK(f.distributor() == data);
    CHECK(f.upper() == upper_sum);
    CHECK(f.lower() == lower_sum);
    CHECK(!f.computer.overflow());
}

// 67  RAABL  Reset and Add Absolute into Lower
TEST_CASE("reset and add absolute into lower 1")
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
    CHECK(f.distributor() == data);
    CHECK(f.upper() == upper_sum);
    CHECK(f.lower() == lower_sum);
    CHECK(!f.computer.overflow());
}

TEST_CASE("reset and add absolute into lower 2")
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
    CHECK(f.distributor() == data);
    CHECK(f.upper() == upper_sum);
    CHECK(f.lower() == lower_sum);
    CHECK(!f.computer.overflow());
}

// 18  SABL  Subtract Absolute from Lower
TEST_CASE("subtract absolute from lower 1")
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
    CHECK(f.distributor() == data);
    CHECK(f.upper() == upper_sum);
    CHECK(f.lower() == lower_sum);
    CHECK(!f.computer.overflow());
}

TEST_CASE("subtract absolute from lower 2")
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
    CHECK(f.distributor() == data);
    CHECK(f.upper() == upper_sum);
    CHECK(f.lower() == lower_sum);
    CHECK(!f.computer.overflow());
}

TEST_CASE("subtract absolute from lower 3")
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
    CHECK(f.distributor() == data);
    CHECK(f.upper() == upper_sum);
    CHECK(f.lower() == lower_sum);
    CHECK(!f.computer.overflow());
}

// 68  RSABL  Reset and Subtract Absolute into Lower
TEST_CASE("reset and subtract absolute into lower 1")
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
    CHECK(f.distributor() == data);
    CHECK(f.upper() == upper_sum);
    CHECK(f.lower() == lower_sum);
    CHECK(!f.computer.overflow());
}

TEST_CASE("reset and subtract absolute into lower 2")
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
    CHECK(f.distributor() == data);
    CHECK(f.upper() == upper_sum);
    CHECK(f.lower() == lower_sum);
    CHECK(!f.computer.overflow());
}

// 19  MULT  Multiply
TEST_CASE("multiply 1")
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
    CHECK(f.distributor() == data);
    CHECK(f.upper() == upper_product);
    CHECK(f.lower() == lower_product);
    CHECK(!f.computer.overflow());
}

TEST_CASE("multiply 2")
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
    CHECK(f.distributor() == data);
    CHECK(f.upper() == upper_product);
    CHECK(f.lower() == lower_product);
    CHECK(!f.computer.overflow());
}

TEST_CASE("multiply 3")
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
    CHECK(f.distributor() == data);
    CHECK(f.upper() == upper_product);
    CHECK(f.lower() == lower_product);
    CHECK(!f.computer.overflow());
}

TEST_CASE("multiply 4")
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
    CHECK(f.distributor() == data);
    CHECK(f.upper() == upper_product);
    CHECK(f.lower() == lower_product);
    CHECK( f.computer.overflow());
}

// 14  DIV  Divide
TEST_CASE("divide 1")
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
    CHECK(f.distributor() == data);
    CHECK(f.upper() == remainder);
    CHECK(f.lower() == quotient);
    CHECK(!f.computer.overflow());
}

TEST_CASE("divide 2")
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
    CHECK(f.distributor() == data);
    CHECK(f.upper() == remainder);
    CHECK(f.lower() == quotient);
    CHECK(!f.computer.overflow());
}

TEST_CASE("divide 3")
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
    CHECK(f.distributor() == data);
    CHECK(f.upper() == remainder);
    CHECK(f.lower() == quotient);
    CHECK(!f.computer.overflow());
}

TEST_CASE("divide 4")
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
    CHECK(f.distributor() == data);
    CHECK(f.upper() == remainder);
    CHECK(f.lower() == quotient);
    CHECK(f.computer.overflow());
}

// 64  DIV RU  Divide and Reset Upper
TEST_CASE("divide and reset upper 1")
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
    CHECK(f.distributor() == data);
    CHECK(f.upper() == remainder);
    CHECK(f.lower() == quotient);
    CHECK(!f.computer.overflow());
}

TEST_CASE("divide and reset upper 2")
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
    CHECK(f.distributor() == data);
    CHECK(f.upper() == remainder);
    CHECK(f.lower() == quotient);
    CHECK(!f.computer.overflow());
}

TEST_CASE("divide and reset upper 3")
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
    CHECK(f.distributor() == data);
    CHECK(f.upper() == remainder);
    CHECK(f.lower() == quotient);
    CHECK(!f.computer.overflow());
}

TEST_CASE("divide and reset upper 4")
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
    CHECK(f.distributor() == data);
    CHECK(f.upper() == remainder);
    CHECK(f.lower() == quotient);
    CHECK(f.computer.overflow());
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
TEST_CASE("branch on non zero in upper 1")
{
    Address addr({1,0,0,0});
    Word upper({0,0, 1,2,3,4, 5,6,7,8, '+'});
    Word lower({0,0, 0,8,7,6, 4,3,2,9, '+'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Branch_Fixture f(44, addr, upper, lower, distr);
    f.run();
    CHECK(f.did_branch());
    CHECK(f.upper() == upper);
    CHECK(f.lower() == lower);
}

TEST_CASE("branch on non zero in upper 2")
{
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,1,2,3, '+'});
    Word lower({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Branch_Fixture f(44, addr, upper, lower, distr);
    f.run();
    CHECK(f.did_branch());
    CHECK(f.upper() == upper);
    CHECK(f.lower() == lower);
}

TEST_CASE("branch on non zero in upper 3")
{
    Address addr({1,0,0,0});
    Word upper({0,0, 1,2,3,4, 5,6,7,8, '-'});
    Word lower({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Branch_Fixture f(44, addr, upper, lower, distr);
    f.run();
    CHECK(f.did_branch());
    CHECK(f.upper() == upper);
    CHECK(f.lower() == lower);
}

TEST_CASE("branch on non zero in upper 4")
{
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word lower({0,0, 1,2,3,4, 5,6,7,8, '+'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Branch_Fixture f(44, addr, upper, lower, distr);
    f.run();
    CHECK(!f.did_branch());
    CHECK(f.upper() == upper);
    CHECK(f.lower() == lower);
}

TEST_CASE("branch on non zero in upper 5")
{
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word lower({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Branch_Fixture f(44, addr, upper, lower, distr);
    f.run();
    CHECK(!f.did_branch());
    CHECK(f.upper() == upper);
    CHECK(f.lower() == lower);
}

TEST_CASE("branch on non zero in upper 6")
{
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Branch_Fixture f(44, addr, upper, lower, distr);
    f.run();
    CHECK(!f.did_branch());
    CHECK(f.upper() == upper);
    CHECK(f.lower() == lower);
}

// 45  BRNZ  Branch on Non-Zero
TEST_CASE("branch on non zero 1")
{
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,1,2,3, '+'});
    Word lower({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Branch_Fixture f(45, addr, upper, lower, distr);
    f.run();
    CHECK(f.did_branch());
    CHECK(f.upper() == upper);
    CHECK(f.lower() == lower);
}

TEST_CASE("branch on non zero 2")
{
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word lower({0,0, 0,0,0,0, 0,1,2,3, '+'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Branch_Fixture f(45, addr, upper, lower, distr);
    f.run();
    CHECK(f.did_branch());
    CHECK(f.upper() == upper);
    CHECK(f.lower() == lower);
}

TEST_CASE("branch on non zero 3")
{
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower({0,0, 0,0,0,0, 0,1,2,3, '-'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Branch_Fixture f(45, addr, upper, lower, distr);
    f.run();
    CHECK(f.did_branch());
    CHECK(f.upper() == upper);
    CHECK(f.lower() == lower);
}

TEST_CASE("branch on non zero 4")
{
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word lower({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Branch_Fixture f(45, addr, upper, lower, distr);
    f.run();
    CHECK(!f.did_branch());
    CHECK(f.upper() == upper);
    CHECK(f.lower() == lower);
}

TEST_CASE("branch on non zero 5")
{
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Branch_Fixture f(45, addr, upper, lower, distr);
    f.run();
    CHECK(!f.did_branch());
    CHECK(f.upper() == upper);
    CHECK(f.lower() == lower);
}

// 46  BRMIN  Branch on Minus
TEST_CASE("branch on minus 1")
{
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower({0,0, 0,0,0,0, 0,1,2,3, '-'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Branch_Fixture f(46, addr, upper, lower, distr);
    f.run();
    CHECK(f.did_branch());
    CHECK(f.upper() == upper);
    CHECK(f.lower() == lower);
}

TEST_CASE("branch on minus 2")
{
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,1,2,3, '-'});
    Word lower({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Branch_Fixture f(46, addr, upper, lower, distr);
    f.run();
    CHECK(f.did_branch());
    CHECK(f.upper() == upper);
    CHECK(f.lower() == lower);
}

TEST_CASE("branch on minus 3")
{
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Branch_Fixture f(46, addr, upper, lower, distr);
    f.run();
    CHECK(f.did_branch());
    CHECK(f.upper() == upper);
    CHECK(f.lower() == lower);
}

TEST_CASE("branch on minus 4")
{
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word lower({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Branch_Fixture f(46, addr, upper, lower, distr);
    f.run();
    CHECK(!f.did_branch());
    CHECK(f.upper() == upper);
    CHECK(f.lower() == lower);
}

TEST_CASE("branch on minus 5")
{
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word lower({0,0, 0,0,0,0, 0,1,2,3, '+'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Branch_Fixture f(46, addr, upper, lower, distr);
    f.run();
    CHECK(!f.did_branch());
    CHECK(f.upper() == upper);
    CHECK(f.lower() == lower);
}

TEST_CASE("branch on minus 6")
{
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word lower({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word distr({0,0, 0,0,0,0, 0,1,2,3, '+'});

    Branch_Fixture f(46, addr, upper, lower, distr);
    f.run();
    CHECK(!f.did_branch());
    CHECK(f.upper() == upper);
    CHECK(f.lower() == lower);
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
TEST_CASE("branch on overflow 1")
{
    Word upper({5,0, 0,0,0,0, 0,0,0,0, '+'});
    Word lower({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Branch_on_Overflow_Fixture f(10, upper, lower, distr);
    f.run();
    CHECK(f.computer.overflow());
    CHECK(f.did_branch());
    // Upper has the low 10 digits of 50... + the word at address 0040.
    CHECK(f.upper() == Word({1,9, 0,0,1,0, 0,0,3,0, '+'}));
    CHECK(f.lower() == lower);
}

TEST_CASE("branch on overflow 2")
{
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word lower({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Branch_on_Overflow_Fixture f(10, upper, lower, distr);
    f.run();
    CHECK(!f.computer.overflow());
    CHECK(!f.did_branch());
    // Upper has 0 + the word at address 0040.
    CHECK(f.upper() == Word({6,9, 0,0,1,0, 0,0,3,0, '+'}));
    CHECK(f.lower() == lower);
}

TEST_CASE("branch on overflow 3")
{
    Word upper({5,0, 0,0,0,0, 0,0,0,0, '+'});
    Word lower({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Branch_on_Overflow_Fixture f(10, upper, lower, distr);
    // Stop the program on overflow.
    f.computer.set_overflow_mode(Computer::Overflow_Mode::stop);
    f.run();
    CHECK(f.computer.overflow());
    // No branch. Program stopped on overflow.
    CHECK(!f.did_branch());
    // Upper has the low 10 digits of 50... + the word at address 0040.
    CHECK(f.upper() == Word({1,9, 0,0,1,0, 0,0,3,0, '+'}));
    CHECK(f.lower() == lower);
}

// 90-99  BRD 1-10  Branch on 8 in Distributor Position 1-10
TEST_CASE("branch on 8 in position 10 1")
{
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower({0,0, 0,0,0,0, 0,1,2,3, '-'});
    Word distr({8,0, 0,0,0,0, 0,0,0,0, '+'});

    Branch_Fixture f(90, addr, upper, lower, distr);
    f.run();
    CHECK(f.did_branch());
    CHECK(!f.error_stop());
    CHECK(f.upper() == upper);
    CHECK(f.lower() == lower);
}

TEST_CASE("branch on 8 in position 10 2")
{
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower({0,0, 0,0,0,0, 0,1,2,3, '-'});
    Word distr({9,0, 0,0,0,0, 0,0,0,0, '+'});

    Branch_Fixture f(90, addr, upper, lower, distr);
    f.run();
    CHECK(!f.did_branch());
    CHECK(!f.error_stop());
    CHECK(f.upper() == upper);
    CHECK(f.lower() == lower);
}

TEST_CASE("branch on 8 in position 10 3")
{
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower({0,0, 0,0,0,0, 0,1,2,3, '-'});
    Word distr({0,9, 8,0,0,0, 0,0,0,0, '+'});

    Branch_Fixture f(90, addr, upper, lower, distr);
    f.run();
    CHECK(!f.did_branch());
    CHECK(f.error_stop());
    CHECK(f.upper() == upper);
    CHECK(f.lower() == lower);
}

TEST_CASE("branch on 8 in position 1 1")
{
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower({0,0, 0,0,0,0, 0,1,2,3, '-'});
    Word distr({0,1, 2,3,4,5, 6,7,8,8, '+'});

    Branch_Fixture f(91, addr, upper, lower, distr);
    f.run();
    CHECK(f.did_branch());
    CHECK(!f.error_stop());
    CHECK(f.upper() == upper);
    CHECK(f.lower() == lower);
}

TEST_CASE("branch on 8 in position 1 2")
{
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower({0,0, 0,0,0,0, 0,1,2,3, '-'});
    Word distr({0,1, 2,3,4,5, 6,7,8,9, '+'});

    Branch_Fixture f(91, addr, upper, lower, distr);
    f.run();
    CHECK(!f.did_branch());
    CHECK(!f.error_stop());
    CHECK(f.upper() == upper);
    CHECK(f.lower() == lower);
}

TEST_CASE("branch on 8 in position 2 1")
{
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower({0,0, 0,0,0,0, 0,1,2,3, '-'});
    Word distr({0,1, 2,3,4,5, 6,7,8,8, '+'});

    Branch_Fixture f(92, addr, upper, lower, distr);
    f.run();
    CHECK(f.did_branch());
    CHECK(!f.error_stop());
    CHECK(f.upper() == upper);
    CHECK(f.lower() == lower);
}

TEST_CASE("branch on 8 in position 2 2")
{
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '-'});
    Word lower({0,0, 0,0,0,0, 0,1,2,3, '-'});
    Word distr({0,1, 2,3,4,5, 6,7,9,8, '+'});

    Branch_Fixture f(92, addr, upper, lower, distr);
    f.run();
    CHECK(!f.did_branch());
    CHECK(!f.error_stop());
    CHECK(f.upper() == upper);
    CHECK(f.lower() == lower);
}

// 30  SRT  Shift Right
TEST_CASE("shift right")
{
    Address addr({0,0,0,2});
    Word upper({1,2, 3,4,5,6, 7,8,9,0, '+'});
    Word lower({1,2, 3,4,5,6, 7,8,9,0, '+'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Opcode_Fixture f(30, addr, upper, lower, distr);
    f.run();
    CHECK(f.distributor() == distr);
    CHECK(f.upper() == Word({0,0, 1,2,3,4, 5,6,7,8, '+'}));
    CHECK(f.lower() == Word({9,0, 1,2,3,4, 5,6,7,8, '+'}));
}

// 31  SRD  Shift and Round
TEST_CASE("shift and round 1")
{
    Address addr({0,0,0,2});
    Word upper({1,2, 3,4,5,6, 7,8,9,0, '+'});
    Word lower({1,2, 3,4,5,6, 7,8,9,0, '+'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Opcode_Fixture f(31, addr, upper, lower, distr);
    f.run();
    CHECK(f.distributor() == distr);
    CHECK(f.upper() == Word({0,0, 1,2,3,4, 5,6,7,8, '+'}));
    CHECK(f.lower() == Word({9,0, 1,2,3,4, 5,6,7,9, '+'}));
}

TEST_CASE("shift and round 2")
{
    Address addr({0,0,0,0});
    Word upper({1,2, 3,4,5,6, 7,8,9,0, '+'});
    Word lower({1,2, 3,4,5,6, 7,8,9,0, '+'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Opcode_Fixture f(31, addr, upper, lower, distr);
    f.run();
    CHECK(f.distributor() == distr);
    CHECK(f.upper() == Word({0,0, 0,0,0,0, 0,0,0,0, '+'}));
    CHECK(f.lower() == Word({1,2, 3,4,5,6, 7,8,9,0, '+'}));
}

TEST_CASE("shift and round 3")
{
    Address addr({0,0,5,5});
    Word upper({1,2, 3,4,5,6, 7,8,9,0, '-'});
    Word lower({1,2, 3,4,5,6, 7,8,9,0, '-'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Opcode_Fixture f(31, addr, upper, lower, distr);
    f.run();
    CHECK(f.distributor() == distr);
    CHECK(f.upper() == Word({0,0, 0,0,0,1, 2,3,4,5, '-'}));
    CHECK(f.lower() == Word({6,7, 8,9,0,1, 2,3,4,6, '-'}));
}

// 35  SLT  Shift Left
TEST_CASE("shift left")
{
    Address addr({0,0,0,6});
    Word upper({1,2, 3,4,5,6, 7,8,9,0, '+'});
    Word lower({1,2, 3,4,5,6, 7,8,9,0, '+'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Opcode_Fixture f(35, addr, upper, lower, distr);
    f.run();
    CHECK(f.distributor() == distr);
    CHECK(f.upper() == Word({7,8, 9,0,1,2, 3,4,5,6, '+'}));
    CHECK(f.lower() == Word({7,8, 9,0,0,0, 0,0,0,0, '+'}));
    CHECK(!f.computer.overflow());
}

// 36  SCT  Shift Left and Count
TEST_CASE("shift left and count 1")
{
    Address addr({00,0,0});
    Word upper({0,0, 0,0,0,1, 2,3,4,5, '+'});
    Word lower({2,2, 2,2,2,5, 5,5,5,5, '+'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Opcode_Fixture f(36, addr, upper, lower, distr);
    f.run();
    CHECK(f.distributor() == distr);
    CHECK(f.upper() == Word({1,2, 3,4,5,2, 2,2,2,2, '+'}));
    CHECK(f.lower() == Word({5,5, 5,5,5,0, 0,0,0,5, '+'}));
    CHECK(!f.computer.overflow());
}

TEST_CASE("shift left and count 2")
{
    Address addr({0,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word lower({1,2, 3,4,5,2, 2,2,2,2, '+'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Opcode_Fixture f(36, addr, upper, lower, distr);
    f.run();
    CHECK(f.distributor() == distr);
    CHECK(f.upper() == Word({1,2, 3,4,5,2, 2,2,2,2, '+'}));
    CHECK(f.lower() == Word({0,0, 0,0,0,0, 0,0,1,0, '+'}));
    CHECK(!f.computer.overflow());
}

TEST_CASE("shift left and count 3")
{
    Address addr({0,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word lower({0,0, 1,2,3,4, 5,2,2,2, '+'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Opcode_Fixture f(36, addr, upper, lower, distr);
    f.run();
    CHECK(f.distributor() == distr);
    CHECK(f.upper() == Word({0,0, 1,2,3,4, 5,2,2,2, '+'}));
    CHECK(f.lower() == Word({0,0, 0,0,0,0, 0,0,1,0, '+'}));
    CHECK(f.computer.overflow());
}

TEST_CASE("shift left and count 4")
{
    Address addr({0,0,0,0});
    Word upper({1,2, 3,4,5,6, 7,8,9,0, '+'});
    Word lower({2,2, 2,2,2,5, 5,5,5,5, '+'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Opcode_Fixture f(36, addr, upper, lower, distr);
    f.run();
    CHECK(f.distributor() == distr);
    CHECK(f.upper() == Word({1,2, 3,4,5,6, 7,8,9,0, '+'}));
    CHECK(f.lower() == Word({2,2, 2,2,2,5, 5,5,0,0, '+'}));
    CHECK(!f.computer.overflow());
}

TEST_CASE("shift left and count 5")
{
    Address addr({0,0,0,0});
    Word upper({0,1, 2,3,4,5, 6,7,8,9, '+'});
    Word lower({2,2, 2,2,2,5, 5,5,5,5, '+'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Opcode_Fixture f(36, addr, upper, lower, distr);
    f.run();
    CHECK(f.distributor() == distr);
    CHECK(f.upper() == Word({1,2, 3,4,5,6, 7,8,9,2, '+'}));
    CHECK(f.lower() == Word({2,2, 2,2,5,5, 5,5,0,1, '+'}));
    CHECK(!f.computer.overflow());
}

TEST_CASE("shift left and count 6")
{
    Address addr({0,0,0,6});
    Word upper({0,0, 0,0,1,2, 3,4,5,6, '+'});
    Word lower({2,2, 2,2,2,5, 5,5,5,5, '+'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Opcode_Fixture f(36, addr, upper, lower, distr);
    f.run();
    CHECK(f.distributor() == distr);
    CHECK(f.upper() == Word({1,2, 3,4,5,6, 2,2,2,2, '+'}));
    CHECK(f.lower() == Word({2,5, 5,5,5,5, 0,0,0,8, '+'}));
    CHECK(!f.computer.overflow());
}

TEST_CASE("shift left and count 7")
{
    Address addr({0,0,0,6});
    Word upper({0,0, 0,0,0,0, 1,2,3,4, '+'});
    Word lower({2,2, 2,2,2,5, 5,5,5,5, '+'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Opcode_Fixture f(36, addr, upper, lower, distr);
    f.run();
    CHECK(f.distributor() == distr);
    CHECK(f.upper() == Word({1,2, 3,4,2,2, 2,2,2,5, '+'}));
    CHECK(f.lower() == Word({5,5, 5,5,0,0, 0,0,1,0, '+'}));
    CHECK(!f.computer.overflow());
}

TEST_CASE("shift left and count 8")
{
    Address addr({0,0,0,6});
    Word upper({0,0, 0,0,0,0, 0,1,2,3, '+'});
    Word lower({2,2, 2,2,2,5, 5,5,5,5, '+'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Opcode_Fixture f(36, addr, upper, lower, distr);
    f.run();
    CHECK(f.distributor() == distr);
    CHECK(f.upper() == Word({0,1, 2,3,2,2, 2,2,2,5, '+'}));
    CHECK(f.lower() == Word({5,5, 5,5,0,0, 0,0,1,0, '+'}));
    CHECK(f.computer.overflow());
}

TEST_CASE("shift left and count 9")
{
    Address addr({0,0,0,6});
    Word upper({1,2, 3,4,5,6, 7,8,9,0, '+'});
    Word lower({2,2, 2,2,2,5, 5,5,5,5, '+'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Opcode_Fixture f(36, addr, upper, lower, distr);
    f.run();
    CHECK(f.distributor() == distr);
    CHECK(f.upper() == Word({1,2, 3,4,5,6, 7,8,9,0, '+'}));
    CHECK(f.lower() == Word({2,2, 2,2,2,5, 5,5,0,0, '+'}));
    CHECK(!f.computer.overflow());
}

TEST_CASE("shift left and count 10")
{
    Address addr({0,0,0,6});
    Word upper({0,1, 2,3,4,5, 6,7,8,9, '+'});
    Word lower({2,2, 2,2,2,5, 5,5,5,5, '+'});
    Word distr({0,0, 0,0,0,0, 0,0,0,0, '+'});

    Opcode_Fixture f(36, addr, upper, lower, distr);
    f.run();
    CHECK(f.distributor() == distr);
    CHECK(f.upper() == Word({1,2, 3,4,5,6, 7,8,9,2, '+'}));
    CHECK(f.lower() == Word({2,2, 2,2,5,5, 5,5,0,5, '+'}));
    CHECK(!f.computer.overflow());
}

struct Table_Fixture : public Opcode_Fixture
{
    Table_Fixture(const Address& addr,
                  Word value,
                  const Word& upper,
                  const Word& lower,
                  const Word& distr)
        : Opcode_Fixture(84, Word(), addr, upper, lower, distr)
        {
            Word one_hundred({0,0, 0,0,0,0, 0,1,0,0, '+'});
            // Fill addresses 0200 to 0247 with values starting at value and incrementing by
            // 100.
            for (Address addr = Address({0,2,0,0}); addr != Address({0,2,6,0}); ++addr)
            {
                TDigit carry;
                computer.set_drum(addr, value);
                value = add(value, one_hundred, carry);
            }
        }
};

// 84  TLU  Table Lookup
TEST_CASE("table lookup 1")
{
    Address addr({0,2,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Word lower({6,5, 0,0,0,0, 0,5,5,4, '+'});
    Word distr({8,3, 6,5,8,2, 8,3,0,0, '+'});
    Word start({8,3, 6,5,8,2, 4,3,0,0, '+'});

    Table_Fixture f(addr, start, upper, lower, distr);
    f.run();
    CHECK(f.distributor() == distr);
    CHECK(f.upper() == upper);
    CHECK(f.lower() == Word({6,5, 0,2,4,0, 0,5,5,4, '+'}));
}

TEST_CASE("table lookup 2")
{
    Address addr({0,2,1,5});
    Word upper({0,0, 0,0,0,1, 2,3,4,5, '+'});
    Word lower({6,5, 0,0,0,0, 0,5,5,4, '+'});
    Word distr({8,3, 6,5,8,2, 8,3,0,0, '+'});
    Word start({8,3, 6,5,8,2, 4,3,0,0, '+'});

    Table_Fixture f(addr, start, upper, lower, distr);
    f.run();
    CHECK(f.distributor() == distr);
    CHECK(f.upper() == upper);
    CHECK(f.lower() == Word({6,5, 0,2,5,5, 0,5,5,4, '+'}));
}

TEST_CASE("table lookup 3")
{
    Address addr({0,2,1,5});
    Word upper({0,0, 0,0,0,1, 2,3,4,5, '+'});
    Word lower({6,5, 0,0,0,0, 0,5,5,4, '+'});
    Word distr({8,3, 6,5,8,2, 8,3,2,1, '+'});
    Word start({8,3, 6,5,8,2, 4,3,0,0, '+'});

    Table_Fixture f(addr, start, upper, lower, distr);
    f.run();
    CHECK(f.distributor() == distr);
    CHECK(f.upper() == upper);
    // Distributor is not an exact match, get the next higher address.
    CHECK(f.lower() == Word({6,5, 0,2,5,6, 0,5,5,4, '+'}));
}

TEST_CASE("table lookup 4")
{
    Address addr({0,2,0,0});
    Word upper({0,0, 0,0,0,1, 2,3,4,5, '+'});
    Word lower({6,5, 0,0,0,0, 0,5,5,4, '+'});
    Word distr({8,3, 6,5,8,2, 9,1,0,0, '+'});
    Word start({8,3, 6,5,8,2, 4,3,0,0, '+'});

    Table_Fixture f(addr, start, upper, lower, distr);
    f.run();
    CHECK(f.distributor() == distr);
    CHECK(f.upper() == upper);
    // Can't match at address 0248 or 0249.
    CHECK(f.lower() == Word({6,5, 0,2,5,0, 0,5,5,4, '+'}));
}
