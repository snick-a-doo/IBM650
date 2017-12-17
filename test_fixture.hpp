#include "computer.hpp"

struct Computer_Ready_Fixture
{
    Computer_Ready_Fixture() {
        computer.power_on();
        computer.step(180);
    }
    IBM650::Computer computer;
};

struct Drum_Storage_Fixture : public Computer_Ready_Fixture
{
    // See operator's manual p.55 "Examples of Use of the Control Console"
    void store(const IBM650::Address& address, const IBM650::Word& word) {
        auto control = computer.get_control_mode();
        auto display = computer.get_display_mode();

        computer.set_storage_entry(word);
        computer.set_address(address);
        computer.set_control_mode(IBM650::Computer::Control_Mode::manual);
        computer.set_display_mode(IBM650::Computer::Display_Mode::read_in_storage);
        computer.program_reset();
        computer.transfer();
        computer.program_start();

        computer.set_control_mode(control);
        computer.set_display_mode(display);
    }
};

struct Run_Fixture : public Drum_Storage_Fixture
{
    Run_Fixture() {
        computer.set_programmed_mode(IBM650::Computer::Programmed_Mode::stop);
        computer.set_control_mode(IBM650::Computer::Control_Mode::run);
    }
};
