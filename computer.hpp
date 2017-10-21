#ifndef COMPUTER_HPP
#define COMPUTER_HPP

#include <array>

#include "drum.hpp"
#include "register.hpp"

namespace IBM650
{
    class Computer
    {
        friend class Computer_Fixture;

    public:
        Computer(bool large);

        enum class Programmed
        {
            run,
            stop,
        };

        enum class Half_Cycle
        {
            half,
            run,
        };

        enum class Control
        {
            address_stop,
            run,
            manual,
        };

        enum class Display
        {
            lower_accumulator,
            upper_accumulator,
            distributor,
            program_register,
            read_out_storage,
            read_in_storage,
        };

        enum class Overflow
        {
            stop,
            sense,
        };

        enum class Error
        {
            stop,
            sense,
        };

        enum class Operating_Lights
        {
            data_address = 0,
            instruction_address,
            program,
            accumulator,
            punch,
            read,
            n_lights
        };
        using Op_Lights = std::array<int, static_cast<int>(Operating_Lights::n_lights)>;

        enum class Checking_Lights
        {
            program_register = 0,
            storage_selection,
            distributor,
            overflow,
            clocking,
            accumulator,
            error_sense,
            n_lights
        };
        using Check_Lights = std::array<int, static_cast<int>(Checking_Lights::n_lights)>;

        // Switches
        void set_storage_entry(const Word& word);
        void set_programmed(Programmed prog);
        void set_half_cycle(Half_Cycle cycle);
        void set_address_selection(const Address& address);
        void set_control(Control control);
        void set_display(Display display);
        void set_overflom(Overflow overflow);
        void set_error(Error error);

        // Keys
        void transfer();
        void program_start();
        void program_stop();
        void program_reset();
        void computer_reset();
        void accumulator_reset();
        void error_reset();
        void error_sense_reset();

        // Lights
        /// Return the word indicated by m_display_mode when the computer is stopped.
        /// (?) Blank when running.
        const Word& display_lights() const;
        const Opcode& opcode_lights() const;
        const Address& address_lights() const;
        const Op_Lights& operating_lights() const;
        const Check_Lights& checking_lights() const;

    private:
        void update_display();

        Drum m_drum;

        Word m_distributor; // address 8001
        Program_Register m_program_register;
        /// The operation register.  Holds the opcode of the current operation.  Copied from
        /// the program register.
        Opcode m_operation;
        /// The address register.  Holds the data address during the data half-cycle,
        /// instruction address during the instruction half-cycle.  Copied from the program
        /// register. 
        Address m_address;
        Accumulator m_accumulator; // address low: 8002, high: 8003

        // Console controls
        Word m_storage_entry; // address 8000
        Programmed m_programmed_mode;
        Half_Cycle m_half_cycle_mode;
        Address m_address_selection;
        Control m_control_mode;
        Display m_display_mode;
        Overflow m_overflow_mode;
        Error m_error_mode;

        // Errors
        bool m_program_register_error;
        bool m_accumulator_error;
        bool m_distributor_error;
        bool m_storage_selection_error;
        bool m_clocking_error;
        bool m_overflow;
        bool m_error_sense;

        Word m_display_lights;
        /// Set to m_operation during the data half-cycle, blank during the instruction
        /// half-cycle.  Labeled "Operation".  Named "opcode" here to avoid confusion with the
        /// "operating" lights.
        Opcode m_opcode_lights;
        /// Set to m_address.
        Address m_address_lights;
        Op_Lights m_operating_lights;
        Check_Lights m_checking_lights;
    };
}

#endif
