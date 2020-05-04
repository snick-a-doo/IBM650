#include "computer.hpp"
#include "test_fixture.hpp"
#include "doctest.h"

using namespace IBM650;

TEST_CASE("turn comptuter on")
{
    Computer computer;
    CHECK(!computer.is_on());
    CHECK(!computer.is_blower_on());
    CHECK(!computer.is_ready());

    computer.power_on();
    CHECK(computer.is_on());
    CHECK(computer.is_blower_on());
    CHECK(!computer.is_ready());

    // Computer is ready after 3 minutes.
    computer.step(179);
    CHECK(!computer.is_ready());
    computer.step(1);
    CHECK(computer.is_ready());
}

TEST_CASE("turn computer off")
{
    Computer_Ready_Fixture f;
    CHECK(f.computer.is_on());
    CHECK(f.computer.is_blower_on());
    CHECK(f.computer.is_ready());
    f.computer.power_off();
    CHECK(!f.computer.is_on());
    CHECK(f.computer.is_blower_on());
    CHECK(!f.computer.is_ready());
    f.computer.step(299);
    CHECK(f.computer.is_blower_on());
    f.computer.step(1);
    CHECK(!f.computer.is_blower_on());
}

TEST_CASE("turn dc on and off")
{
    Computer computer;
    computer.power_on();
    CHECK(computer.is_on());
    computer.step(10);
    computer.dc_on(); // Too early; no effect.
    CHECK(!computer.is_ready());
    computer.step(10);
    computer.dc_off(); // Doesn't prevent automatic DC turn-on.
    computer.step(180);
    CHECK(computer.is_on());
    CHECK(computer.is_blower_on());
    CHECK(computer.is_ready());
    computer.dc_off();
    CHECK(computer.is_on());
    CHECK(computer.is_blower_on());
    CHECK(!computer.is_ready());
    computer.step(10);
    CHECK(!computer.is_ready());
    computer.step(1800);
    computer.dc_on();
    CHECK(computer.is_on());
    CHECK(computer.is_blower_on());
    CHECK(computer.is_ready());
    computer.power_off();
    computer.step(500);
    CHECK(!computer.is_ready());
    computer.dc_on();
    CHECK(!computer.is_ready());
}

TEST_CASE("turn master power off")
{
    Computer_Ready_Fixture f;
    f.computer.master_power_off();
    CHECK(!f.computer.is_on());
    CHECK(!f.computer.is_blower_on());
    CHECK(!f.computer.is_ready());
    f.computer.power_on();
    // Can't be turned back on.
    CHECK(!f.computer.is_on());
    CHECK(!f.computer.is_blower_on());
    CHECK(!f.computer.is_ready());
    f.computer.dc_on();
    CHECK(!f.computer.is_on());
    CHECK(!f.computer.is_blower_on());
    CHECK(!f.computer.is_ready());
}

TEST_CASE("transfer button")
{
    Computer_Ready_Fixture f;
    f.computer.set_control_mode(Computer::Control_Mode::manual);
    f.computer.set_address(Address({1,2,3,4}));
    CHECK(f.computer.address_register() != Address({1,2,3,4}));
    f.computer.transfer();
    CHECK(f.computer.address_register() == Address({1,2,3,4}));
    f.computer.set_control_mode(Computer::Control_Mode::address_stop);
    f.computer.set_address(Address({0,0,9,9}));
    f.computer.transfer();
    CHECK(f.computer.address_register() == Address({1,2,3,4}));
    f.computer.set_control_mode(Computer::Control_Mode::run);
    f.computer.set_address(Address({0,0,9,9}));
    f.computer.transfer();
    CHECK(f.computer.address_register() == Address({1,2,3,4}));
}

struct Reset_Fixture : public Computer_Ready_Fixture
{
    Reset_Fixture() {
        computer.set_address(Address({1,2,3,4}));
        computer.set_distributor(Word({3,10, 3,2,3,3, 3,4,3,5, '-'}));
        computer.set_upper(Word({10,1, 1,2,1,3, 1,4,1,5, '+'}));
        computer.set_lower(Word({2,1, 2,2,2,3, 2,4,2,5, '+'}));
        computer.set_program_register(Word({1,2, 3,4,5,6, 7,8,9,10, '+'}));
        computer.set_error();
    }
};

TEST_CASE("reset state")
{
    Reset_Fixture f;
    CHECK(f.computer.distributor_validity_error());
    CHECK(f.computer.accumulator_validity_error());
    CHECK(f.computer.program_register_validity_error());
    CHECK(f.computer.storage_selection_error());
    CHECK(f.computer.clocking_error());
}

TEST_CASE("program reset")
{
    Reset_Fixture f;
    f.computer.set_display_mode(Computer::Display_Mode::program_register);
    SUBCASE("initial register states")
    {
        CHECK(f.computer.display() == Word({1,2, 3,4,5,6, 7,8,9,10, '_'}));
        CHECK(f.computer.operation_register() == Register<2>({1,2}));
        CHECK(f.computer.address_register() == Address({3,4,5,6}));
    }
    SUBCASE("manual mode")
    {
        f.computer.set_control_mode(Computer::Control_Mode::manual);
        f.computer.program_reset();
        CHECK(!f.computer.program_register_validity_error());
        CHECK(!f.computer.storage_selection_error());
        CHECK(!f.computer.clocking_error());
        CHECK(f.computer.display() == Word({0,0, 0,0,0,0, 0,0,0,0, '_'}));
        CHECK(f.computer.operation_register() == Register<2>());
        CHECK(f.computer.address_register().is_blank()); // cleared in manual mode
    }
    SUBCASE("run mode")
    {
        f.computer.set_control_mode(Computer::Control_Mode::run);
        f.computer.program_reset();
        CHECK(!f.computer.program_register_validity_error());
        CHECK(!f.computer.storage_selection_error());
        CHECK(!f.computer.clocking_error());
        f.computer.set_display_mode(Computer::Display_Mode::program_register);
        CHECK(f.computer.display() == Word({0,0, 0,0,0,0, 0,0,0,0, '_'}));
        CHECK(f.computer.operation_register() == Register<2>());
        CHECK(f.computer.address_register() == Address({8,0,0,0})); // zeroed
    }
    SUBCASE("address stop mode")
    {
        f.computer.set_control_mode(Computer::Control_Mode::address_stop);
        f.computer.program_reset();
        CHECK(!f.computer.program_register_validity_error());
        CHECK(!f.computer.storage_selection_error());
        CHECK(!f.computer.clocking_error());
        f.computer.set_display_mode(Computer::Display_Mode::program_register);
        CHECK(f.computer.display() == Word({0,0, 0,0,0,0, 0,0,0,0, '_'}));
        CHECK(f.computer.operation_register() == Register<2>());
        CHECK(f.computer.address_register() == Address({8,0,0,0})); // zeroed
    }
}

TEST_CASE("accumulator reset")
{
    Reset_Fixture f;

    f.computer.set_display_mode(Computer::Display_Mode::lower_accumulator);
    CHECK(f.computer.display() == Word({2,1, 2,2,2,3, 2,4,2,5, '+'}));
    f.computer.set_display_mode(Computer::Display_Mode::upper_accumulator);
    CHECK(f.computer.display() == Word({10,1, 1,2,1,3, 1,4,1,5, '+'}));
    f.computer.set_display_mode(Computer::Display_Mode::distributor);
    CHECK(f.computer.display() == Word({3,10, 3,2,3,3, 3,4,3,5, '-'}));
    f.computer.set_display_mode(Computer::Display_Mode::program_register);
    CHECK(f.computer.display() == Word({1,2, 3,4,5,6, 7,8,9,10, '_'}));
    CHECK(f.computer.operation_register() == Register<2>({1,2}));
    CHECK(f.computer.address_register() == Address({3,4,5,6}));

    f.computer.set_control_mode(Computer::Control_Mode::manual);
    f.computer.accumulator_reset();

    CHECK(!f.computer.overflow());
    CHECK(!f.computer.distributor_validity_error());
    CHECK(!f.computer.accumulator_validity_error());
    // Program register still has a bad digit.
    CHECK(f.computer.program_register_validity_error());
    // Address register is still out of range: 3456.
    CHECK(f.computer.storage_selection_error());
    CHECK(!f.computer.clocking_error());
    f.computer.set_display_mode(Computer::Display_Mode::lower_accumulator);
    CHECK(f.computer.display() == Word({0,0, 0,0,0,0, 0,0,0,0, '+'}));
    f.computer.set_display_mode(Computer::Display_Mode::upper_accumulator);
    CHECK(f.computer.display() == Word({0,0, 0,0,0,0, 0,0,0,0, '+'}));
    f.computer.set_display_mode(Computer::Display_Mode::distributor);
    CHECK(f.computer.display() == Word({0,0, 0,0,0,0, 0,0,0,0, '+'}));
    // Unchanged.
    f.computer.set_display_mode(Computer::Display_Mode::program_register);
    CHECK(f.computer.display() == Word({1,2, 3,4,5,6, 7,8,9,10, '_'}));
    CHECK(f.computer.operation_register() == Register<2>({1,2}));
    CHECK(f.computer.address_register() == Address({3,4,5,6}));
}

TEST_CASE("error reset")
{
    Reset_Fixture f;
    f.computer.error_reset();
    CHECK(f.computer.overflow());
    CHECK(f.computer.distributor_validity_error());
    CHECK(f.computer.accumulator_validity_error());
    CHECK(f.computer.program_register_validity_error());
    CHECK(f.computer.error_sense());
    // Still has bad address.
    CHECK(f.computer.storage_selection_error());
    f.computer.set_program_register(Word({1,2, 1,2,3,4, 2,4,6,8, '-'}));
    CHECK(!f.computer.storage_selection_error());
    CHECK(!f.computer.clocking_error());
}

TEST_CASE("error sense reset")
{
    Reset_Fixture f;
    CHECK(f.computer.error_sense());
    f.computer.error_sense_reset();
    CHECK(!f.computer.error_sense());
}

TEST_CASE("computer reset")
{
    Reset_Fixture f;
    SUBCASE("manual mode")
    {
        f.computer.set_control_mode(Computer::Control_Mode::manual);
        f.computer.computer_reset();
        CHECK(!f.computer.program_register_validity_error());
        CHECK(!f.computer.storage_selection_error());
        CHECK(!f.computer.clocking_error());
        CHECK(!f.computer.error_sense());
        f.computer.set_display_mode(Computer::Display_Mode::program_register);
        CHECK(f.computer.display() == Word({0,0, 0,0,0,0, 0,0,0,0, '_'}));
        f.computer.set_display_mode(Computer::Display_Mode::lower_accumulator);
        CHECK(f.computer.display() == Word({0,0, 0,0,0,0, 0,0,0,0, '+'}));
        f.computer.set_display_mode(Computer::Display_Mode::upper_accumulator);
        CHECK(f.computer.display() == Word({0,0, 0,0,0,0, 0,0,0,0, '+'}));
        f.computer.set_display_mode(Computer::Display_Mode::distributor);
        CHECK(f.computer.display() == Word({0,0, 0,0,0,0, 0,0,0,0, '+'}));
        CHECK(f.computer.operation_register() == Register<2>());
        CHECK(f.computer.address_register() == Address());
    }
    SUBCASE("run mode")
    {
        f.computer.set_control_mode(Computer::Control_Mode::run);
        f.computer.computer_reset();
        f.computer.set_display_mode(Computer::Display_Mode::program_register);
        CHECK(f.computer.display() == Word({0,0, 0,0,0,0, 0,0,0,0, '_'}));
        f.computer.set_display_mode(Computer::Display_Mode::lower_accumulator);
        CHECK(f.computer.display() == Word({0,0, 0,0,0,0, 0,0,0,0, '+'}));
        f.computer.set_display_mode(Computer::Display_Mode::upper_accumulator);
        CHECK(f.computer.display() == Word({0,0, 0,0,0,0, 0,0,0,0, '+'}));
        f.computer.set_display_mode(Computer::Display_Mode::distributor);
        CHECK(f.computer.display() == Word({0,0, 0,0,0,0, 0,0,0,0, '+'}));
        CHECK(f.computer.operation_register() == Register<2>());
        CHECK(f.computer.address_register() == Address({8,0,0,0}));
    }
}

TEST_CASE("storage entry")
{
    Word word({1,2, 1,2,3,4, 2,4,6,8, '-'});
    Address word_addr({0,5,1,2});
    Word zero({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Address zero_addr({0,5,1,3});

    Computer_Ready_Fixture f;
    // See operator's manual p.55 "Examples of Use of the Control Console/Enter
    // Information"
    // 1. Write word to word_addr.
    f.computer.set_storage_entry(word);
    f.computer.set_address(word_addr);
    f.computer.set_control_mode(IBM650::Computer::Control_Mode::manual);
    f.computer.set_display_mode(IBM650::Computer::Display_Mode::read_in_storage);
    f.computer.program_reset();
    f.computer.transfer();
    f.computer.program_start();
    f.computer.set_display_mode(Computer::Display_Mode::distributor);
    CHECK(f.computer.display() == word);
    // 2. Write zero to zero_addr.
    f.computer.set_storage_entry(zero);
    f.computer.set_address(zero_addr);
    f.computer.set_display_mode(IBM650::Computer::Display_Mode::read_in_storage);
    f.computer.program_reset();
    f.computer.transfer();
    f.computer.program_start();
    f.computer.set_display_mode(Computer::Display_Mode::distributor);
    CHECK(f.computer.display() == zero);
    // 3. Read word from word_addr.
    f.computer.set_address(word_addr);
    f.computer.set_display_mode(Computer::Display_Mode::read_out_storage);
    f.computer.program_reset();
    f.computer.transfer();
    f.computer.program_start();
    CHECK(f.computer.display() == word);
    // 4. Read zero from zero_addr.
    f.computer.set_address(zero_addr);
    f.computer.set_display_mode(Computer::Display_Mode::read_out_storage);
    f.computer.program_reset();
    f.computer.transfer();
    f.computer.program_start();
    CHECK(f.computer.display() == zero);
}

TEST_CASE("accumulator entry")
{
    Word word({1,2, 1,2,3,4, 2,4,6,8, '-'});
    Address word_addr({0,5,1,2});

    Computer_Ready_Fixture f;
    // Write word to the drum at word_addr.
    f.computer.set_storage_entry(word);
    f.computer.set_address(word_addr);
    f.computer.set_control_mode(IBM650::Computer::Control_Mode::manual);
    f.computer.set_display_mode(IBM650::Computer::Display_Mode::read_in_storage);
    f.computer.program_reset();
    f.computer.transfer();
    f.computer.program_start();

    f.computer.set_display_mode(Computer::Display_Mode::distributor);
    CHECK(f.computer.display() == word);

    SUBCASE("copy word from word_addr to the lower accumulator")
    {
        // op 65 is "reset add lower".
        f.computer.set_storage_entry(Word({6,5, 0,5,1,2, 0,0,0,0, '+'}));
        f.computer.set_half_cycle_mode(Computer::Half_Cycle_Mode::half);
        f.computer.set_control_mode(Computer::Control_Mode::run);
        CHECK(f.computer.operation_register() == Register<2>());
        CHECK(f.computer.address_register() == word_addr);
        // The I-half-cycle is ready to be executed.
        f.computer.program_reset();
        CHECK(!f.computer.data_address());
        CHECK(f.computer.instruction_address());
        f.computer.program_start();

        // The 1st half-cycle loads the program register from the storage-entry switches.
        CHECK(f.computer.operation_register() == Register<2>({6,5}));
        CHECK(f.computer.address_register() == word_addr);
        // The D-half-cycle is ready to be executed.
        CHECK(f.computer.data_address());
        CHECK(!f.computer.instruction_address());
        f.computer.program_start();
        CHECK(f.computer.operation_register() == Register<2>());
        CHECK(f.computer.address_register() == Address({0,0,0,0}));
        f.computer.set_display_mode(Computer::Display_Mode::lower_accumulator);
        CHECK(f.computer.display() == word);
    }
    SUBCASE("copy from storage entry to the lower accumulator")
    {
        f.computer.set_display_mode(Computer::Display_Mode::distributor);

        // "Reset add lower" from address 8000, the storage entry switches.
        f.computer.set_storage_entry(Word({6,5, 8,0,0,0, 0,0,0,0, '+'}));
        f.computer.set_half_cycle_mode(Computer::Half_Cycle_Mode::half);
        f.computer.set_control_mode(Computer::Control_Mode::run);
        f.computer.program_reset();
        f.computer.program_start();

        f.computer.set_storage_entry(word);
        f.computer.program_start();

        f.computer.set_display_mode(Computer::Display_Mode::lower_accumulator);
        CHECK(f.computer.display() == word);
    }
}

TEST_CASE("start program")
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

    Computer_Ready_Fixture f;
    f.computer.set_drum(addr1, instr1);
    f.computer.set_drum(addr2, instr2);
    f.computer.set_drum(addr3, instr3);
    f.computer.set_drum(data_addr, data);
    // Make the program stop on "program stop".
    f.computer.set_programmed_mode(Computer::Programmed_Mode::stop);
    f.computer.set_control_mode(Computer::Control_Mode::run);
    f.computer.set_display_mode(Computer::Display_Mode::upper_accumulator);
    f.computer.set_storage_entry(Word({0,0, 0,0,0,0, 0,1,0,0, '+'}));

    f.computer.computer_reset();
    f.computer.program_start();
    CHECK(f.computer.display() == Word({0,0, 0,0,0,3, 4,3,4,2, '+'}));
}

struct LD_Fixture : public Run_Fixture
{
    LD_Fixture()
        : LD({6,9, 0,1,0,0, 0,0,0,1, '+'}),
          STOP({0,1, 0,0,0,0, 0,0,0,0, '+'}),
          data({0,0, 0,1,1,2, 2,3,3,4, '-'})
        {
            computer.set_drum(Address({0,0,0,0}), LD);
            computer.set_drum(Address({0,0,0,1}), STOP);
            computer.set_drum(Address({0,1,0,0}), data);
            computer.set_storage_entry(Word({0,0, 0,0,0,0, 0,0,0,0, '+'}));
            computer.set_display_mode(Computer::Display_Mode::distributor);
        }

    Word LD;
    Word STOP;
    Word data;
};

TEST_CASE("run LD")
{
    LD_Fixture f;
    f.computer.computer_reset();
    f.computer.program_start();
    CHECK(f.computer.display() == f.data);
}

TEST_CASE("run LD twice")
{
    // Check that the program can be run again after reset.
    LD_Fixture f;
    f.computer.computer_reset();
    f.computer.program_start();
    CHECK(f.computer.display() == f.data);

    f.computer.computer_reset();
    f.computer.program_start();
    CHECK(f.computer.display() == f.data);
}

TEST_CASE("load distributor' timing")
{
    LD_Fixture f;
    f.computer.computer_reset();
    f.computer.program_start();
    // Drum index = 0
    // 1 to enable PR
    // 0 for address search (8000)
    // 2 to fill PR, OP, DA to ADDR
    // 0 for no-op
    // 2 to for IA to ADDR, enable PR
    // Drum index = 5
    // 45 to find inst addr 0000 on drum
    // t = 50
    // 2 to fill PR, OP, DA to ADDR
    // 1 to enable distributor
    // Drum index = 3
    // 47 to find data addr 0100 on drum.
    // t = 100
    // 1 to load distributor.
    // 2 to for IA to ADDR, enable PR
    // Drum index = 3
    // 48 to find inst addr 0001 on drum.
    // t = 151
    // 2 to fill PR, OP, DA to ADDR
    // 0 for stop
    // 2 to for IA to ADDR, enable PR
    CHECK(f.computer.run_time() == 155);
    CHECK(f.computer.display() == f.data);
}

struct Optimum_LD_Fixture : public Run_Fixture
{
    Optimum_LD_Fixture()
        : LD({6,9, 0,1,0,8, 0,0,6,1, '+'}),
          STOP({0,1, 0,0,0,0, 0,0,0,0, '+'}),
          data({0,0, 0,1,1,2, 2,3,3,4, '-'})
        {
            computer.set_drum(Address({0,0,0,5}), LD);
            computer.set_drum(Address({0,0,6,1}), STOP);
            computer.set_drum(Address({0,1,0,8}), data);
            computer.set_storage_entry(Word({0,0, 0,0,0,0, 0,0,0,5, '+'}));
            computer.set_display_mode(Computer::Display_Mode::distributor);
        }

    Word LD;
    Word STOP;
    Word data;
};

TEST_CASE("optimum 'load distributor' timing")
{
    Optimum_LD_Fixture f;
    f.computer.computer_reset();
    f.computer.program_start();
    // Drum index = 0
    // 1 to enable PR
    // 0 for address search (8000)
    // 2 to fill PR, OP, DA to ADDR
    // 0 for no-op
    // 2 to for IA to ADDR, enable PR
    // Drum index = 5
    // 0 to find inst addr 0005 on drum
    // t = 5
    // 2 to fill PR, OP, DA to ADDR
    // 1 to enable distributor
    // Drum index = 8
    // 0 to find data addr 0108 on drum.
    // t = 8
    // 1 to load distributor.
    // 2 to for IA to ADDR, enable PR
    // Drum index = 11
    // 0 to find inst addr 0061 on drum.
    // t = 11
    // 2 to fill PR, OP, DA to ADDR
    // 0 for stop
    // 2 to for IA to ADDR, enable PR
    CHECK(f.computer.run_time() == 15);
    CHECK(f.computer.display() == f.data);
}

struct RAL_Fixture : public Run_Fixture
{
    RAL_Fixture()
        : RAL({6,5, 0,1,0,0, 0,0,0,1, '+'}),
          STOP({0,1, 0,0,0,0, 0,0,0,0, '+'}),
          data({0,0, 0,1,1,2, 2,3,3,4, '-'})
        {
            computer.set_drum(Address({0,0,0,0}), RAL);
            computer.set_drum(Address({0,0,0,1}), STOP);
            computer.set_drum(Address({0,1,0,0}), data);
            computer.set_storage_entry(Word({0,0, 0,0,0,0, 0,0,0,0, '+'}));
            computer.set_display_mode(Computer::Display_Mode::lower_accumulator);
        }

    Word RAL;
    Word STOP;
    Word data;
};

TEST_CASE("'reset and add lower' timing")
{
    RAL_Fixture f;
    f.computer.computer_reset();
    f.computer.program_start();
    // Drum index = 0
    // 1 to enable PR
    // 0 for address search (8000)
    // 2 to fill PR, OP, DA to ADDR
    // 0 for no-op
    // 2 to for IA to ADDR, enable PR
    // Drum index = 5
    // 45 to find inst addr 0000 on drum
    // t = 50
    // 2 to fill PR, OP, DA to ADDR
    // 1 to enable distributor
    // Drum index = 3
    // 47 to find data addr 0100 on drum.
    // t = 100
    // 1 to load distributor.
    // 1 wait for even
    // 2 fill accumulator (restart and IA to ADDR)
    // 1 remove interlock A (enable PR)
    // Drum index = 4
    // 47 to find inst addr 0001 on drum.
    // t = 151
    // 2 to fill PR, OP, DA to ADDR
    // 0 for stop
    // 2 to for IA to ADDR, enable PR
    CHECK(f.computer.run_time() == 155);
    CHECK(f.computer.display() == f.data);
}

struct Optimum_RAL_Fixture : public Run_Fixture
{
    Optimum_RAL_Fixture()
        : RAL({6,5, 1,1,5,8, 0,0,1,3, '+'}),
          STOP({0,1, 0,0,0,0, 0,0,0,0, '+'}),
          data({0,0, 0,1,1,2, 2,3,3,4, '-'})
        {
            computer.set_drum(Address({0,0,0,5}), RAL);
            computer.set_drum(Address({0,0,1,3}), STOP);
            computer.set_drum(Address({1,1,5,8}), data);
            computer.set_storage_entry(Word({0,0, 0,0,0,0, 0,0,0,5, '+'}));
            computer.set_display_mode(Computer::Display_Mode::lower_accumulator);
        }

    Word RAL;
    Word STOP;
    Word data;
};

TEST_CASE("optimum 'reset and add lower' timing")
{
    Optimum_RAL_Fixture f;
    f.computer.computer_reset();
    std::cerr << "start\n";
    f.computer.program_start();
    // Drum index = 0
    // 1 to enable PR
    // 0 for address search (8000)
    // 2 to fill PR, OP, DA to ADDR
    // 0 for no-op
    // 2 to for IA to ADDR, enable PR
    // Drum index = 5
    // 0 to find inst addr 0000 on drum
    // t = 5
    // 2 to fill PR, OP, DA to ADDR
    // 1 to enable distributor
    // Drum index = 8
    // 0 to find data addr 1158 on drum.
    // t = 8
    // 1 to load distributor.
    // 1 wait for even
    // 2 fill accumulator (restart and IA to ADDR)
    // 1 remove interlock A (enable PR)
    // Drum index = 13
    // 0 to find inst addr 0013 on drum.
    // t = 13
    // 2 to fill PR, OP, DA to ADDR
    // 0 for stop
    // 2 to for IA to ADDR, enable PR
    CHECK(f.computer.run_time() == 17);
    CHECK(f.computer.display() == f.data);
}
