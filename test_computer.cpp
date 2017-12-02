#include <boost/test/unit_test.hpp>

#include "computer.hpp"

using namespace IBM650;

struct Computer_Ready_Fixture
{
    Computer_Ready_Fixture() {
        computer.power_on();
        computer.step(180);
    }
    Computer computer;
};

BOOST_AUTO_TEST_CASE(off)
{
    Computer computer;
    BOOST_CHECK(!computer.is_on());
    BOOST_CHECK(!computer.is_blower_on());
    BOOST_CHECK(!computer.is_ready());
}

BOOST_AUTO_TEST_CASE(turn_on)
{
    Computer computer;
    computer.power_on();
    BOOST_CHECK(computer.is_on());
    BOOST_CHECK(computer.is_blower_on());
    BOOST_CHECK(!computer.is_ready());
    computer.step(179);
    BOOST_CHECK(!computer.is_ready());
    computer.step(1);
    BOOST_CHECK(computer.is_ready());
}

BOOST_AUTO_TEST_CASE(turn_off)
{
    Computer_Ready_Fixture f;
    BOOST_CHECK(f.computer.is_on());
    BOOST_CHECK(f.computer.is_blower_on());
    BOOST_CHECK(f.computer.is_ready());
    f.computer.power_off();
    BOOST_CHECK(!f.computer.is_on());
    BOOST_CHECK(f.computer.is_blower_on());
    BOOST_CHECK(!f.computer.is_ready());
    f.computer.step(299);
    BOOST_CHECK(f.computer.is_blower_on());
    f.computer.step(1);
    BOOST_CHECK(!f.computer.is_blower_on());
}

BOOST_AUTO_TEST_CASE(dc_on_off)
{
    Computer computer;
    computer.power_on();
    BOOST_CHECK(computer.is_on());
    computer.step(10);
    computer.dc_on(); // Too early; no effect.
    BOOST_CHECK(!computer.is_ready());
    computer.step(10);
    computer.dc_off(); // Doesn't prevent automatic DC turn-on.
    computer.step(180);
    BOOST_CHECK(computer.is_on());
    BOOST_CHECK(computer.is_blower_on());
    BOOST_CHECK(computer.is_ready());
    computer.dc_off();
    BOOST_CHECK(computer.is_on());
    BOOST_CHECK(computer.is_blower_on());
    BOOST_CHECK(!computer.is_ready());
    computer.step(10);
    BOOST_CHECK(!computer.is_ready());
    computer.step(1800);
    computer.dc_on();
    BOOST_CHECK(computer.is_on());
    BOOST_CHECK(computer.is_blower_on());
    BOOST_CHECK(computer.is_ready());
    computer.power_off();
    computer.step(500);
    BOOST_CHECK(!computer.is_ready());
    computer.dc_on();
    BOOST_CHECK(!computer.is_ready());
}

BOOST_AUTO_TEST_CASE(master_power)
{
    Computer_Ready_Fixture f;
    f.computer.master_power_off();
    BOOST_CHECK(!f.computer.is_on());
    BOOST_CHECK(!f.computer.is_blower_on());
    BOOST_CHECK(!f.computer.is_ready());
    f.computer.power_on();
    // Can't be turned back on.
    BOOST_CHECK(!f.computer.is_on());
    BOOST_CHECK(!f.computer.is_blower_on());
    BOOST_CHECK(!f.computer.is_ready());
    f.computer.dc_on();
    BOOST_CHECK(!f.computer.is_on());
    BOOST_CHECK(!f.computer.is_blower_on());
    BOOST_CHECK(!f.computer.is_ready());
}

BOOST_AUTO_TEST_CASE(transfer)
{
    Computer_Ready_Fixture f;
    f.computer.set_control(Computer::Control::manual);
    f.computer.set_address(Register<4>({1,2,3,4}));
    BOOST_CHECK(f.computer.address_register() != Register<4>({1,2,3,4}));
    f.computer.transfer();
    BOOST_CHECK_EQUAL(f.computer.address_register(), Register<4>({1,2,3,4}));
    f.computer.set_control(Computer::Control::address_stop);
    f.computer.set_address(Register<4>({0,0,9,9}));
    f.computer.transfer();
    BOOST_CHECK_EQUAL(f.computer.address_register(), Register<4>({1,2,3,4}));
    f.computer.set_control(Computer::Control::run);
    f.computer.set_address(Register<4>({0,0,9,9}));
    f.computer.transfer();
    BOOST_CHECK_EQUAL(f.computer.address_register(), Register<4>({1,2,3,4}));
}

struct Reset_Fixture : public Computer_Ready_Fixture
{
    Reset_Fixture() {
        computer.set_address(Register<4>({1,2,3,4}));
        computer.set_distributor(Signed_Register<10>({3,10, 3,2,3,3, 3,4,3,5, '-'}));
        computer.set_accumulator(
            Signed_Register<20>({10,1, 1,2,1,3, 1,4,1,5, 2,1, 2,2,2,3, 2,4,2,5, '+'}));
        computer.set_program_register(Register<10>({1,2, 3,4,5,6, 7,8,9,10}));
        computer.set_error();
    }
};

BOOST_AUTO_TEST_CASE(program_reset_manual)
{
    Reset_Fixture f;
    BOOST_CHECK(f.computer.program_register_validity_error());
    BOOST_CHECK(f.computer.storage_selection_error());
    BOOST_CHECK(f.computer.clocking_error());
    BOOST_CHECK_EQUAL(f.computer.program_register(), Register<10>({1,2, 3,4,5,6, 7,8,9,10}));
    BOOST_CHECK_EQUAL(f.computer.operation_register(), Register<2>({1,2}));
    BOOST_CHECK_EQUAL(f.computer.address_register(), Register<4>({3,4,5,6}));

    f.computer.set_control(Computer::Control::manual);
    f.computer.program_reset();
    BOOST_CHECK(!f.computer.program_register_validity_error());
    BOOST_CHECK(!f.computer.storage_selection_error());
    BOOST_CHECK(!f.computer.clocking_error());
    BOOST_CHECK_EQUAL(f.computer.program_register(), Register<10>({0,0, 0,0,0,0, 0,0,0,0}));
    BOOST_CHECK_EQUAL(f.computer.operation_register(), Register<2>());
    BOOST_CHECK_EQUAL(f.computer.address_register(), Register<4>());
}

BOOST_AUTO_TEST_CASE(program_reset_run)
{
    Reset_Fixture f;
    f.computer.set_control(Computer::Control::run);
    f.computer.program_reset();
    BOOST_CHECK(!f.computer.program_register_validity_error());
    BOOST_CHECK(!f.computer.storage_selection_error());
    BOOST_CHECK(!f.computer.clocking_error());
    BOOST_CHECK_EQUAL(f.computer.program_register(), Register<10>({0,0, 0,0,0,0, 0,0,0,0}));
    BOOST_CHECK_EQUAL(f.computer.operation_register(), Register<2>());
    BOOST_CHECK_EQUAL(f.computer.address_register(), Register<4>({1,2,3,4}));
}

BOOST_AUTO_TEST_CASE(program_reset_address_stop)
{
    Reset_Fixture f;
    f.computer.set_control(Computer::Control::address_stop);
    f.computer.program_reset();
    BOOST_CHECK(!f.computer.program_register_validity_error());
    BOOST_CHECK(!f.computer.storage_selection_error());
    BOOST_CHECK(!f.computer.clocking_error());
    BOOST_CHECK_EQUAL(f.computer.program_register(), Register<10>({0,0, 0,0,0,0, 0,0,0,0}));
    BOOST_CHECK_EQUAL(f.computer.operation_register(), Register<2>());
    BOOST_CHECK_EQUAL(f.computer.address_register(), Register<4>({1,2,3,4}));
}

BOOST_AUTO_TEST_CASE(accumulator_reset_manual)
{
    Reset_Fixture f;
    BOOST_CHECK(f.computer.distributor_validity_error());
    BOOST_CHECK(f.computer.accumulator_validity_error());
    BOOST_CHECK(f.computer.storage_selection_error());
    BOOST_CHECK(f.computer.clocking_error());
    BOOST_CHECK_EQUAL(f.computer.distributor(), Signed_Register<10>({3,10, 3,2,3,3, 3,4,3,5, '-'}));
    BOOST_CHECK_EQUAL(f.computer.accumulator(),
                      Signed_Register<20>({10,1, 1,2,1,3, 1,4,1,5, 2,1, 2,2,2,3, 2,4,2,5, '+'}));
    BOOST_CHECK_EQUAL(f.computer.program_register(), Register<10>({1,2, 3,4,5,6, 7,8,9,10}));
    BOOST_CHECK_EQUAL(f.computer.operation_register(), Register<2>({1,2}));
    BOOST_CHECK_EQUAL(f.computer.address_register(), Register<4>({3,4,5,6}));

    f.computer.set_control(Computer::Control::manual);
    f.computer.accumulator_reset();
    BOOST_CHECK(!f.computer.overflow());
    BOOST_CHECK(!f.computer.distributor_validity_error());
    BOOST_CHECK(!f.computer.accumulator_validity_error());
    // Program register still has a bad digit.
    BOOST_CHECK(f.computer.program_register_validity_error());
    // Address register is still out of range: 3456.
    BOOST_CHECK(f.computer.storage_selection_error());
    BOOST_CHECK(!f.computer.clocking_error());
    BOOST_CHECK_EQUAL(f.computer.distributor(), Signed_Register<10>({0,0, 0,0,0,0, 0,0,0,0, '+'}));
    BOOST_CHECK_EQUAL(f.computer.accumulator(),
                      Signed_Register<20>({0,0, 0,0,0,0, 0,0,0,0, 0,0, 0,0,0,0, 0,0,0,0, '+'}));
    // Unchanged.
    BOOST_CHECK_EQUAL(f.computer.program_register(), Register<10>({1,2, 3,4,5,6, 7,8,9,10}));
    BOOST_CHECK_EQUAL(f.computer.operation_register(), Register<2>({1,2}));
    BOOST_CHECK_EQUAL(f.computer.address_register(), Register<4>({3,4,5,6}));
}

BOOST_AUTO_TEST_CASE(error_reset)
{
    Reset_Fixture f;
    f.computer.error_reset();
    BOOST_CHECK(f.computer.overflow());
    BOOST_CHECK(f.computer.distributor_validity_error());
    BOOST_CHECK(f.computer.accumulator_validity_error());
    BOOST_CHECK(f.computer.program_register_validity_error());
    BOOST_CHECK(f.computer.error_sense());
    // Still has bad address.
    BOOST_CHECK(f.computer.storage_selection_error());
    f.computer.set_program_register(Register<10>({1,2, 1,2,3,4, 2,4,6,8}));
    BOOST_CHECK(!f.computer.storage_selection_error());
    BOOST_CHECK(!f.computer.clocking_error());
}

BOOST_AUTO_TEST_CASE(error_sense_reset)
{
    Reset_Fixture f;
    BOOST_CHECK(f.computer.error_sense());
    f.computer.error_sense_reset();
    BOOST_CHECK(!f.computer.error_sense());
}
