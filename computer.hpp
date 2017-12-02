#ifndef COMPUTER_HPP
#define COMPUTER_HPP

#include "register.hpp"

namespace IBM650
{
    using TTime = int;

class Computer
{
public:
    Computer();

    /// Advance time by the passed-in number of seconds.
    void step(TTime seconds);

    /// Apply main power with the "power on" key.
    void power_on();
    /// Turn off main power with the "power off" key.
    void power_off();
    /// Turn on DC power with the "DC on" key.  Does nothing unless DC power was automatically
    /// turned on after startup, and then manually turned off.
    void dc_on();
    /// Turn off DC power with the "DC off" key.  Does nothing until DC power is automatically
    /// turned on after startup.
    void dc_off();
    /// The master power switch cuts all power immediately and forever.  The 650 could be
    /// turned back on by a technician.  Here, you can recover by creating a new instance.
    void master_power_off();

    /// @Return true when main power is on.
    bool is_on() const;
    /// @Return true when the cooling blower is on.  The blower is turned on when main power
    /// applied.  It's turned off 5 minutes after main power is turned off.
    bool is_blower_on() const;
    /// @Return true when DC power is on and the computer is ready for operation.
    bool is_ready() const;

    /// The control switch states.
    enum class Control
    {
        address_stop,
        run,
        manual,
    };
    /// Set the control mode.
    void set_control(Control mode);
    /// Set the address switches.
    void set_address(const Register<4>& address);

    //! remove when not needed for testing
    void set_distributor(const Signed_Register<10>& reg);
    void set_accumulator(const Signed_Register<20>& reg);
    void set_program_register(const Register<10>& reg);
    void set_error();

    /// Press the transfer key.  Sets the address register but only in manual control.
    void transfer();
    void program_reset();
    void accumulator_reset();
    void error_reset();
    void error_sense_reset();

    /// @Return the contents of the address register.
    const Signed_Register<10>& distributor() const;
    const Signed_Register<20>& accumulator() const;
    const Register<10>& program_register() const;
    const Register<2>& operation_register() const;
    const Register<4>& address_register() const;

    bool overflow() const;
    bool distributor_validity_error() const;
    bool accumulator_validity_error() const;
    bool program_register_validity_error() const;
    bool storage_selection_error() const;
    bool clocking_error() const;
    bool error_sense() const;

private:
    /// The number of seconds that have passed since main power was turned on or off.
    TTime m_elapsed_seconds;
    /// True until master power is turned off.
    bool m_can_turn_on;
    /// True when main power is on.
    bool m_power_on;
    /// True when DC power is on.
    bool m_dc_on;

    /// The state of the control switch.
    Control m_control_mode;
    /// The state of the address switches.
    Register<4> m_address_entry;
    /// The contents of the address register.
    Signed_Register<10> m_distributor;
    Signed_Register<20> m_accumulator;
    Register<10> m_program_register;
    Register<2> m_operation_register;
    Register<4> m_address_register;

    bool m_overflow;
    bool m_storage_selection_error;
    bool m_clocking_error;
    bool m_error_sense;
};
}

#endif
