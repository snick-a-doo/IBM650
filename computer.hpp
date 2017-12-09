#ifndef COMPUTER_HPP
#define COMPUTER_HPP

#include "register.hpp"

namespace IBM650
{
    using TTime = int;

    using Address = Register<4>;
    using Word = Signed_Register<10>;

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

    enum class Half_Cycle_Mode
    {
        half,
        run,
    };
    enum class Control_Mode
    {
        address_stop,
        run,
        manual,
    };
    enum class Display_Mode
    {
        lower_accumulator,
        upper_accumulator,
        distributor,
        program_register,
        read_in_storage,
        read_out_storage,
    };

    /// Set the storage-entry switches.
    void set_storage_entry(const Word& word);
    /// Set the half-cycle switch.
    void set_half_cycle(Half_Cycle_Mode mode);
    /// Set the control switch.
    void set_control(Control_Mode mode);
    /// Set the display switch.
    void set_display(Display_Mode mode);
    /// Set the address switches.
    void set_address(const Address& address);

    //! remove when not needed for testing
    void set_accumulator(const Signed_Register<20>& reg);
    void set_program_register(const Word& reg); //! make private
    void set_error();

    /// Press the transfer key.  Sets the address register but only in manual control.
    void transfer();
    void program_start();
    void program_reset();
    void accumulator_reset();
    void error_reset();
    void error_sense_reset();

    /// @Return the states of the display lights.
    Word display() const;
    /// @Return the states of the operation register lights.
    const Register<2>& operation_register() const;
    /// @Return the states of the address register lights.
    const Address& address_register() const;

    // "Operating" lights
    bool data_address() const;
    bool instruction_address() const;

    // "Checking" lights

    bool overflow() const;
    bool distributor_validity_error() const;
    bool accumulator_validity_error() const;
    bool program_register_validity_error() const;
    bool storage_selection_error() const;
    bool clocking_error() const;
    bool error_sense() const;

private:
    void set_storage(const Address& address, const Word& word);
    const Word& get_storage(const Address& address) const;

    /// The number of seconds that have passed since main power was turned on or off.
    TTime m_elapsed_seconds;
    /// True until master power is turned off.
    bool m_can_turn_on;
    /// True when main power is on.
    bool m_power_on;
    /// True when DC power is on.
    bool m_dc_on;

    /// The state of the control switch.
    Control_Mode m_control_mode;
    Half_Cycle_Mode m_cycle_mode;
    /// The state of the display switch.
    Display_Mode m_display_mode;
    /// The state of the storage entry switches.
    Word m_storage_entry;
    /// The state of the address switches.
    Address m_address_entry;

    Word m_distributor;
    Signed_Register<20> m_accumulator;
    Register<10> m_program_register;
    Register<2> m_operation_register;
    /// The contents of the address register.
    Address m_address_register;

    enum class Half_Cycle
    {
        data,
        instruction,
    };
    Half_Cycle m_half_cycle;

    bool m_overflow;
    bool m_storage_selection_error;
    bool m_clocking_error;
    bool m_error_sense;

    //!drum class?
    constexpr static size_t m_drum_capacity = 2000;
    std::array<Word, m_drum_capacity> m_drum;
};
}

#endif
