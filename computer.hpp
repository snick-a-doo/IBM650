#ifndef COMPUTER_HPP
#define COMPUTER_HPP

#include "register.hpp"

#include <memory>
#include <vector>

namespace IBM650
{
    using TTime = int;

    using Address = Register<4>;
    constexpr std::size_t word_size = 10;
    using UWord = Register<word_size>;
    using Word = Signed_Register<word_size>;

    class Operation_Step;

class Computer
{
    friend class Instruction_to_Program_Register;
    friend class Op_and_Address_to_Registers;
    friend class Instruction_Address_to_Address_Register;
    friend class Data_to_Distributor;
    friend class Distributor_to_Accumulator;
    friend class Remove_Interlock_A;
    friend class Enable_Position_Set;
    friend class Store_Distributor;
    friend class Multiply;
    friend class Divide;
    friend class Enable_Shift_Control;
    friend class Shift;
    friend class Look_Up_Address;
    friend class Address_to_Program_Register;
    friend class Insert_Address_in_Lower;

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

    enum class Programmed_Mode
    {
        stop,
        run,
    };
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
    enum class Overflow_Mode
    {
        stop,
        sense,
    };
    enum class Error_Mode
    {
        stop,
        sense,
    };

    // Console Switches

    /// Set the storage-entry switches.  10 digit selectors and 1 sign selector for the word at
    /// address 8000.
    void set_storage_entry(const Word& word);
    //// Set the "programmed" switch.  When set to "stop", "stop" instructions halt the
    //// program.  When set to "run", stops are ignored.  This lets stops act like
    //// breakpoints.
    void set_programmed_mode(Programmed_Mode mode);
    /// Set the half-cycle switch.  2-position selector for continuous or stepped running.
    void set_half_cycle_mode(Half_Cycle_Mode mode);
    /// Set the control switch.  3-position selector for selecting how to proceed when the
    /// "program start" key is pressed.
    void set_control_mode(Control_Mode mode);
    /// Set the display switch.  6-position switch for choosing the word displayed on the
    /// console.
    void set_display_mode(Display_Mode mode);
    /// Set the error switch.  When set to "stop", errors halt the program.  When set to
    /// "sense", execution continues with the instruction in the storage-entry switches.
    void set_overflow_mode(Overflow_Mode mode);
    void set_error_mode(Error_Mode mode);
    /// Set the address switches.  4 digit selectors for the address used for manual operation.
    /// It's also the stop address in the "address stop" control mode.
    void set_address(const Address& address);

    /// @Return the state of the control switch.
    Control_Mode get_control_mode() const;
    /// @Return the state of the display switch.
    Display_Mode get_display_mode() const;

#ifdef TEST
    // Direct access to the machine's state for unit tests.
    void set_distributor(const Word& reg);
    void set_upper(const Word& reg);
    void set_lower(const Word& reg);
    void set_program_register(const Word& reg);
    void set_drum(const Address& addr, const Word& data);
    void set_error();

    Word get_drum(const Address& addr) const;
#endif

    // Console Keys

    /// Press the transfer key.  Sets the address register but only in manual control.
    void transfer();
    /// Start program execution.
    void program_start();
    /// Reset registers and errors to prepare to run a program.
    void program_reset();
    /// Full reset: roughly equivalent to doing the three other resets.
    void computer_reset();
    /// Set the accumulator and distributor to zero and reset errors.
    void accumulator_reset();
    /// Reset errors.
    void error_reset();
    void error_sense_reset();

    // Register Lights

    /// @Return the states of the display lights.  May be blank.
    Word display() const;
    /// @Return the states of the operation register lights.  May be blank.
    const Register<2>& operation_register() const;
    /// @Return the states of the address register lights.  May be blank.
    const Address& address_register() const;

    // "Operating" Lights

    /// True if the data address is displayed in the address lights.  Both data_address and
    /// instruction_address return false if no address is displayed.
    bool data_address() const;
    /// True if the instruction address is displayed in the address lights.
    bool instruction_address() const;

    // "Checking" Lights

    /// True if an operation overflowed the accumulator.
    bool overflow() const;
    /// True if a non-digit was detected in the distributor.
    bool distributor_validity_error() const;
    /// True if a non-digit was detected in the accumulator.
    bool accumulator_validity_error() const;
    /// True if a non-digit was detected in the program register.
    bool program_register_validity_error() const;
    /// True if there was a problem with an address.
    bool storage_selection_error() const;
    /// True if there was a timing problem.
    bool clocking_error() const;
    /// True if an error cause the program to stop.
    bool error_sense() const;

    /// The number of word times since computer or program reset.
    int run_time() const;

private:
    /// Write a word to a storage address.
    void set_storage(const Address& address, const Word& word);
    /// @Return the word in the passed-in address.
    const Word get_storage(const Address& address) const;

    /// The number of seconds that have passed since main power was turned on or off.
    TTime m_elapsed_seconds;
    /// True until master power is turned off.
    bool m_can_turn_on;
    /// True when main power is on.
    bool m_power_on;
    /// True when DC power is on.
    bool m_dc_on;

    Programmed_Mode m_programmed_mode;
    /// The state of the control switch.
    Control_Mode m_control_mode;
    /// The state of the half-cycle switch.
    Half_Cycle_Mode m_cycle_mode;
    /// The state of the display switch.
    Display_Mode m_display_mode;
    Overflow_Mode m_overflow_mode;
    Error_Mode m_error_mode;
    /// The state of the storage entry switches.
    Word m_storage_entry;
    /// The state of the address switches.
    Address m_address_entry;

    // Registers

    Word m_distributor;
    Word m_upper_accumulator;
    Word m_lower_accumulator;
    UWord m_program_register;
    Register<2> m_operation_register;
    Address m_address_register;

    enum class Half_Cycle
    {
        data,
        instruction,
    };
    Half_Cycle m_half_cycle;
    int m_run_time;
    bool m_restart;

    // Error flags

    bool m_overflow;
    bool m_storage_selection_error;
    bool m_clocking_error;
    bool m_error_sense;
    /// True if an error that unconditionally stops the program occurred.
    bool m_error_stop;

    class Drum
    {
    public:
        void step();
        bool is_address(const Address& address) const;
        bool is_at_head(const Address& address) const;
        Word read(const Address& address) const;
        void write(const Address& address, const Word& word);
        std::size_t index() const;
    private:
        /// The number of words that can be stored on the drum.
        constexpr static size_t m_capacity = 2000;
        /// The words stored on the drum.
        std::array<Word, m_capacity> m_storage;
        /// The drum position, 0-49.  Determines which addresses are at the read head.
        std::size_t m_index = 0;
    };

    Drum m_drum;

    // Support for multiply and divide loops.
    void add_to_accumulator(const Word& reg, bool to_upper, TDigit& carry);
    void shift_accumulator(int n_places_left);
};

}

#endif
