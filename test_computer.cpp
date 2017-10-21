#include <boost/test/unit_test.hpp>

#include "computer.hpp"

using namespace IBM650;

struct Computer_Fixture
{
    Computer_Fixture()
        : computer(true),
          address({0,0,1,0}),
          data({1,2, 3,4,5,6, 7,8,9,0})
        {
            load(address, data);
        }

    void load(const Address& address, const Word& data)
        {
            // Enter Information - Page 55
            computer.program_stop();
            computer.set_storage_entry(data);
            computer.set_address_selection(address);
            computer.set_control(Computer::Control::manual);
            computer.set_display(Computer::Display::read_in_storage);
            computer.program_reset();
            computer.transfer();
            computer.program_start();
        }

    Computer computer;
    Address address;
    Word data;
};

BOOST_AUTO_TEST_CASE(set_drum_storage)
{
    Computer_Fixture f;

    BOOST_CHECK_EQUAL(f.computer.display_lights(), f.data);

    f.computer.set_display(Computer::Display::read_out_storage);
    f.computer.program_reset();
    f.computer.transfer();
    f.computer.program_start();

    BOOST_CHECK_EQUAL(f.computer.display_lights(), f.data);
}

BOOST_AUTO_TEST_CASE(set_accumulator)
{
    Computer_Fixture f;
    Word add();
    f.computer.set_control(Computer::Control::run);
    f.computer.set_half_cycle(Computer::Half_Cycle::half);
    f.computer.set_display(Computer::Display::distributor);
    f.computer.program_reset();
    f.computer.program_start();
}
