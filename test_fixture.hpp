#include "computer.hpp"

struct Computer_Ready_Fixture
{
    Computer_Ready_Fixture() {
        computer.power_on();
        computer.step(180);
    }
    IBM650::Computer computer;
};

struct Run_Fixture : public Computer_Ready_Fixture
{
    Run_Fixture() {
        computer.set_programmed_mode(IBM650::Computer::Programmed_Mode::stop);
        computer.set_control_mode(IBM650::Computer::Control_Mode::run);
    }
};
