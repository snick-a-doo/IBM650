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

BOOST_AUTO_TEST_CASE(load_distributor_drum)
{
    Word data({0,0, 0,0,1,2, 3,4,5,6, '+'});
    Address addr({1,0,0,0});
    Word upper({0,0, 0,0,0,0, 0,0,0,0, '_'});
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
    Word upper({1,2, 3,4,5,6, 7,8,9,0, '_'});
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
    Word upper({1,2, 3,4,5,6, 7,8,9,0, '_'});
    Word lower({3,8, 3,8,5,6, 7,8,9,0, '-'});
    Word distr(lower);

    Opcode_Fixture f(69, data, addr, upper, lower, distr);
    BOOST_CHECK_EQUAL(f.distributor(), lower);
    BOOST_CHECK_EQUAL(f.upper(), upper);
    BOOST_CHECK_EQUAL(f.lower(), lower); 
}

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
