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
    f.computer.set_control(Computer::Control_Mode::manual);
    f.computer.set_address(Address({1,2,3,4}));
    BOOST_CHECK(f.computer.address_register() != Address({1,2,3,4}));
    f.computer.transfer();
    BOOST_CHECK_EQUAL(f.computer.address_register(), Address({1,2,3,4}));
    f.computer.set_control(Computer::Control_Mode::address_stop);
    f.computer.set_address(Address({0,0,9,9}));
    f.computer.transfer();
    BOOST_CHECK_EQUAL(f.computer.address_register(), Address({1,2,3,4}));
    f.computer.set_control(Computer::Control_Mode::run);
    f.computer.set_address(Address({0,0,9,9}));
    f.computer.transfer();
    BOOST_CHECK_EQUAL(f.computer.address_register(), Address({1,2,3,4}));
}

struct Drum_Storage_Fixture : public Computer_Ready_Fixture
{
    // See operator's manual p.55 "Examples of Use of the Control Console"
    void store(const Address& address, const Word& word) {
        computer.set_storage_entry(word);
        computer.set_address(address);
        computer.set_control(Computer::Control_Mode::manual);
        computer.set_display(Computer::Display_Mode::read_in_storage);
        computer.program_reset();
        computer.transfer();
        computer.program_start();
    }
};

struct Reset_Fixture : public Drum_Storage_Fixture
{
    Reset_Fixture() {
        // Set the address switches and the distributor by side effect.
        store(Address({1,2,3,4}), Word({3,10, 3,2,3,3, 3,4,3,5, '-'}));
        computer.set_accumulator(
            Signed_Register<20>({10,1, 1,2,1,3, 1,4,1,5, 2,1, 2,2,2,3, 2,4,2,5, '+'}));
        computer.set_program_register(Word({1,2, 3,4,5,6, 7,8,9,10, '+'}));
        computer.set_error();
    }
};

BOOST_AUTO_TEST_CASE(program_reset_manual)
{
    Reset_Fixture f;
    BOOST_CHECK(f.computer.program_register_validity_error());
    BOOST_CHECK(f.computer.storage_selection_error());
    BOOST_CHECK(f.computer.clocking_error());
    f.computer.set_display(Computer::Display_Mode::program_register);
    BOOST_CHECK_EQUAL(f.computer.display(), Word({1,2, 3,4,5,6, 7,8,9,10, '_'}));
    BOOST_CHECK_EQUAL(f.computer.operation_register(), Register<2>({1,2}));
    BOOST_CHECK_EQUAL(f.computer.address_register(), Address({3,4,5,6}));

    f.computer.set_control(Computer::Control_Mode::manual);
    f.computer.program_reset();
    BOOST_CHECK(!f.computer.program_register_validity_error());
    BOOST_CHECK(!f.computer.storage_selection_error());
    BOOST_CHECK(!f.computer.clocking_error());
    BOOST_CHECK_EQUAL(f.computer.display(), Word({0,0, 0,0,0,0, 0,0,0,0, '_'}));
    BOOST_CHECK_EQUAL(f.computer.operation_register(), Register<2>());
    BOOST_CHECK_EQUAL(f.computer.address_register(), Address());
}

BOOST_AUTO_TEST_CASE(program_reset_run)
{
    Reset_Fixture f;
    f.computer.set_control(Computer::Control_Mode::run);
    f.computer.program_reset();
    BOOST_CHECK(!f.computer.program_register_validity_error());
    BOOST_CHECK(!f.computer.storage_selection_error());
    BOOST_CHECK(!f.computer.clocking_error());
    f.computer.set_display(Computer::Display_Mode::program_register);
    BOOST_CHECK_EQUAL(f.computer.display(), Word({0,0, 0,0,0,0, 0,0,0,0, '_'}));
    BOOST_CHECK_EQUAL(f.computer.operation_register(), Register<2>());
    BOOST_CHECK_EQUAL(f.computer.address_register(), Address({1,2,3,4}));
}

BOOST_AUTO_TEST_CASE(program_reset_address_stop)
{
    Reset_Fixture f;
    f.computer.set_control(Computer::Control_Mode::address_stop);
    f.computer.program_reset();
    BOOST_CHECK(!f.computer.program_register_validity_error());
    BOOST_CHECK(!f.computer.storage_selection_error());
    BOOST_CHECK(!f.computer.clocking_error());
    f.computer.set_display(Computer::Display_Mode::program_register);
    BOOST_CHECK_EQUAL(f.computer.display(), Word({0,0, 0,0,0,0, 0,0,0,0, '_'}));
    BOOST_CHECK_EQUAL(f.computer.operation_register(), Register<2>());
    BOOST_CHECK_EQUAL(f.computer.address_register(), Address({1,2,3,4}));
}

BOOST_AUTO_TEST_CASE(accumulator_reset_manual)
{
    Reset_Fixture f;
    BOOST_CHECK(f.computer.distributor_validity_error());
    BOOST_CHECK(f.computer.accumulator_validity_error());
    BOOST_CHECK(f.computer.storage_selection_error());
    BOOST_CHECK(f.computer.clocking_error());
    f.computer.set_display(Computer::Display_Mode::lower_accumulator);
    BOOST_CHECK_EQUAL(f.computer.display(), Word({2,1, 2,2,2,3, 2,4,2,5, '+'}));
    f.computer.set_display(Computer::Display_Mode::upper_accumulator);
    BOOST_CHECK_EQUAL(f.computer.display(), Word({10,1, 1,2,1,3, 1,4,1,5, '_'}));
    f.computer.set_display(Computer::Display_Mode::distributor);
    BOOST_CHECK_EQUAL(f.computer.display(), Word({3,10, 3,2,3,3, 3,4,3,5, '-'}));
    f.computer.set_display(Computer::Display_Mode::program_register);
    BOOST_CHECK_EQUAL(f.computer.display(), Word({1,2, 3,4,5,6, 7,8,9,10, '_'}));
    BOOST_CHECK_EQUAL(f.computer.operation_register(), Register<2>({1,2}));
    BOOST_CHECK_EQUAL(f.computer.address_register(), Address({3,4,5,6}));

    f.computer.set_control(Computer::Control_Mode::manual);
    f.computer.accumulator_reset();
    BOOST_CHECK(!f.computer.overflow());
    BOOST_CHECK(!f.computer.distributor_validity_error());
    BOOST_CHECK(!f.computer.accumulator_validity_error());
    // Program register still has a bad digit.
    BOOST_CHECK(f.computer.program_register_validity_error());
    // Address register is still out of range: 3456.
    BOOST_CHECK(f.computer.storage_selection_error());
    BOOST_CHECK(!f.computer.clocking_error());
    f.computer.set_display(Computer::Display_Mode::lower_accumulator);
    BOOST_CHECK_EQUAL(f.computer.display(), Word({0,0, 0,0,0,0, 0,0,0,0, '+'}));
    f.computer.set_display(Computer::Display_Mode::upper_accumulator);
    BOOST_CHECK_EQUAL(f.computer.display(), Word({0,0, 0,0,0,0, 0,0,0,0, '_'}));
    f.computer.set_display(Computer::Display_Mode::distributor);
    BOOST_CHECK_EQUAL(f.computer.display(), Word({0,0, 0,0,0,0, 0,0,0,0, '+'}));

    // Unchanged.
    f.computer.set_display(Computer::Display_Mode::program_register);
    BOOST_CHECK_EQUAL(f.computer.display(), Word({1,2, 3,4,5,6, 7,8,9,10, '_'}));
    BOOST_CHECK_EQUAL(f.computer.operation_register(), Register<2>({1,2}));
    BOOST_CHECK_EQUAL(f.computer.address_register(), Address({3,4,5,6}));
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
    f.computer.set_program_register(Word({1,2, 1,2,3,4, 2,4,6,8, '-'}));
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

BOOST_AUTO_TEST_CASE(computer_reset_manual)
{
    Reset_Fixture f;
    f.computer.set_control(Computer::Control_Mode::manual);
    f.computer.computer_reset();
    BOOST_CHECK(!f.computer.program_register_validity_error());
    BOOST_CHECK(!f.computer.storage_selection_error());
    BOOST_CHECK(!f.computer.clocking_error());
    BOOST_CHECK(!f.computer.error_sense());
    f.computer.set_display(Computer::Display_Mode::program_register);
    BOOST_CHECK_EQUAL(f.computer.display(), Word({0,0, 0,0,0,0, 0,0,0,0, '_'}));
    f.computer.set_display(Computer::Display_Mode::lower_accumulator);
    BOOST_CHECK_EQUAL(f.computer.display(), Word({0,0, 0,0,0,0, 0,0,0,0, '+'}));
    f.computer.set_display(Computer::Display_Mode::upper_accumulator);
    BOOST_CHECK_EQUAL(f.computer.display(), Word({0,0, 0,0,0,0, 0,0,0,0, '_'}));
    f.computer.set_display(Computer::Display_Mode::distributor);
    BOOST_CHECK_EQUAL(f.computer.display(), Word({0,0, 0,0,0,0, 0,0,0,0, '+'}));
    BOOST_CHECK_EQUAL(f.computer.operation_register(), Register<2>());
    BOOST_CHECK_EQUAL(f.computer.address_register(), Address());
}

BOOST_AUTO_TEST_CASE(computer_reset_run)
{
    Reset_Fixture f;
    f.computer.set_control(Computer::Control_Mode::run);
    f.computer.computer_reset();
    f.computer.set_display(Computer::Display_Mode::program_register);
    BOOST_CHECK_EQUAL(f.computer.display(), Word({0,0, 0,0,0,0, 0,0,0,0, '_'}));
    f.computer.set_display(Computer::Display_Mode::lower_accumulator);
    BOOST_CHECK_EQUAL(f.computer.display(), Word({0,0, 0,0,0,0, 0,0,0,0, '+'}));
    f.computer.set_display(Computer::Display_Mode::upper_accumulator);
    BOOST_CHECK_EQUAL(f.computer.display(), Word({0,0, 0,0,0,0, 0,0,0,0, '_'}));
    f.computer.set_display(Computer::Display_Mode::distributor);
    BOOST_CHECK_EQUAL(f.computer.display(), Word({0,0, 0,0,0,0, 0,0,0,0, '+'}));
    BOOST_CHECK_EQUAL(f.computer.operation_register(), Register<2>());
    BOOST_CHECK_EQUAL(f.computer.address_register(), Address({8,0,0,0}));
}

BOOST_AUTO_TEST_CASE(storage_entry)
{
    Word word({1,2, 1,2,3,4, 2,4,6,8, '-'});
    Address word_addr({0,5,1,2});
    Word zero({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Address zero_addr({0,5,1,3});

    Drum_Storage_Fixture f;
    f.store(word_addr, word);
    f.store(zero_addr, zero);

    // See operator's manual p.55 "Examples of Use of the Control Console"
    f.computer.set_address(word_addr);
    f.computer.set_display(Computer::Display_Mode::read_out_storage);
    f.computer.program_reset();
    f.computer.transfer();
    f.computer.program_start();
    BOOST_CHECK(f.computer.display() == word);

    f.computer.set_address(zero_addr);
    f.computer.set_display(Computer::Display_Mode::read_out_storage);
    f.computer.program_reset();
    f.computer.transfer();
    f.computer.program_start();
    BOOST_CHECK(f.computer.display() == zero);
}

BOOST_AUTO_TEST_CASE(accumulator_entry)
{
    Word word({1,2, 1,2,3,4, 2,4,6,8, '-'});
    Address word_addr({0,5,1,2});

    Drum_Storage_Fixture f;
    f.store(word_addr, word);
    f.computer.set_display(Computer::Display_Mode::distributor);
    BOOST_CHECK_EQUAL(f.computer.display(), word);

    // op 65 is "reset add lower".
    f.computer.set_storage_entry(Word({6,5, 0,5,1,2, 0,0,0,0, '+'}));
    f.computer.set_half_cycle(Computer::Half_Cycle_Mode::half);
    f.computer.set_control(Computer::Control_Mode::run);
    BOOST_CHECK_EQUAL(f.computer.operation_register(), Register<2>());
    BOOST_CHECK_EQUAL(f.computer.address_register(), Address());
    // The I-half-cycle is ready to be executed.
    BOOST_CHECK(!f.computer.data_address());
    BOOST_CHECK(f.computer.instruction_address());
    f.computer.program_reset();
    f.computer.program_start();

    // The 1st half-cycle loads the program register from the storage-entry switches.
    BOOST_CHECK_EQUAL(f.computer.operation_register(), Register<2>({6,5}));
    BOOST_CHECK_EQUAL(f.computer.address_register(), word_addr);
    // The D-half-cycle is ready to be executed.
    BOOST_CHECK(f.computer.data_address());
    BOOST_CHECK(!f.computer.instruction_address());
    f.computer.program_start();
    BOOST_CHECK_EQUAL(f.computer.operation_register(), Register<2>());
    BOOST_CHECK_EQUAL(f.computer.address_register(), Address({0,0,0,0}));
    f.computer.set_display(Computer::Display_Mode::lower_accumulator);
    BOOST_CHECK_EQUAL(f.computer.display(), word);
}

BOOST_AUTO_TEST_CASE(accumulator_entry_2)
{
    Word word({1,2, 2,4,6,8, 1,2,3,4, '-'});

    Computer_Ready_Fixture f;
    f.computer.set_display(Computer::Display_Mode::distributor);

    // "Reset add lower" from storage entry switches.
    f.computer.set_storage_entry(Word({6,5, 8,0,0,0, 0,0,0,0, '+'}));
    f.computer.set_half_cycle(Computer::Half_Cycle_Mode::half);
    f.computer.set_control(Computer::Control_Mode::run);
    f.computer.program_reset();
    f.computer.program_start();

    f.computer.set_storage_entry(word);
    f.computer.program_start();

    f.computer.set_display(Computer::Display_Mode::lower_accumulator);
    BOOST_CHECK_EQUAL(f.computer.display(), word);
}

BOOST_AUTO_TEST_CASE(start_program)
{
    // Add to upper
    Word instr1({1,0, 1,1,0,0, 0,1,0,1, '+'});
    Address addr1({0,1,0,0});
    // Add to upper
    Word instr2({1,0, 1,1,0,0, 0,1,0,8, '+'});
    Address addr2({0,1,0,1});
    // Program stop
    Word instr3({0,1, 0,0,0,0, 0,0,0,0, '+'});
    Address addr3({0,1,0,8});
    // Data
    Word data({0,0, 0,0,0,1, 7,1,7,1, '+'});
    Address data_addr({1,1,0,0});

    // Assuming that the sign is not displayed with the upper accumulator.
    Word data_display({0,0, 0,0,0,1, 7,1,7,1, '_'});

    Drum_Storage_Fixture f;
    f.store(addr1, instr1);
    f.store(addr2, instr2);
    f.store(addr3, instr3);
    f.store(data_addr, data);
    // Make the program stop on "program stop".
    f.computer.set_programmed(Computer::Programmed_Mode::stop);
    f.computer.set_control(Computer::Control_Mode::run);
    f.computer.set_display(Computer::Display_Mode::upper_accumulator);
    f.computer.set_storage_entry(Word({0,0, 0,0,0,0, 0,1,0,0, '+'}));

    f.computer.computer_reset();
    f.computer.program_start();
    BOOST_CHECK_EQUAL(f.computer.display(), Word({0,0, 0,0,0,3, 4,3,4,2, '_'}));
}
