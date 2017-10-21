#include <cassert>

#include "computer.hpp"

using namespace IBM650;

Computer::Computer(bool large)
    : m_drum(large),
      m_programmed_mode(Programmed::run),
      m_half_cycle_mode(Half_Cycle::run),
      m_control_mode(Control::run),
      m_display_mode(Display::distributor),
      m_overflow_mode(Overflow::stop),
      m_error_mode(Error::stop),
      m_program_register_error(false),
      m_accumulator_error(false),
      m_distributor_error(false),
      m_storage_selection_error(false),
      m_clocking_error(false),
      m_overflow(false),
      m_error_sense(false),
      m_operating_lights({false}),
      m_checking_lights({false})
{}

void Computer::set_storage_entry(const Word& word)
{
    m_storage_entry = word;
}

void Computer::set_programmed(Programmed prog)
{
    m_programmed_mode = prog;
}

void Computer::set_half_cycle(Half_Cycle half)
{
    m_half_cycle_mode = half;
}

void Computer::set_address_selection(const Address& address)
{
    m_address_selection = address;
}

void Computer::set_control(Control control)
{
    m_control_mode = control;
}

void Computer::set_display(Display display)
{
    // Display Switch - p 53
    //! error if running
    m_display_mode = display;
}

void Computer::transfer()
{
    // Does nothing if not in manual control.  Key Controls/Transfer Key - p 53
    if (m_control_mode == Control::manual)
        m_address = m_address_selection;
}

void Computer::program_start()
{
    if (m_control_mode == Control::manual)
    {
        while (!m_drum.is_at_read_head(m_address))
            m_drum.step();

        // It seems the display switch determines whether this is a read or a write.
        if (m_display_mode == Display::read_out_storage)
            m_distributor = m_drum.read(m_address);
        else if (m_display_mode == Display::read_in_storage)
        {
            m_distributor = m_storage_entry;
            m_drum.write(m_address, m_distributor);
        }
        //! What happens for other display modes?
    }
    //! else error if display mode is READ_IN_STORAGE or READ_OUT_STORAGE

    update_display();
}

void Computer::update_display()
{
    switch (m_display_mode)
    {
    case Display::read_out_storage:
    case Display::read_in_storage:
        assert(m_control_mode == Control::manual);
    case Display::distributor:
        m_display_lights = m_distributor;
        break;
    default:
        assert(false);
    }
}

void Computer::program_stop()
{
}

void Computer::program_reset()
{
    m_program_register.reset();
    m_operation.reset();

    m_program_register_error = false;
    error_reset();

    if (m_control_mode == Control::manual)
        m_address.reset();
    else
        m_address = m_address_selection;
}

void Computer::computer_reset()
{
    program_reset();
    accumulator_reset();
    //! clear all errors
}

void Computer::accumulator_reset()
{
    m_distributor.reset();
    m_accumulator.reset();

    error_reset();
    m_overflow = false;
    m_accumulator_error = false;
    m_distributor_error = false;
}

void Computer::error_reset()
{
    m_storage_selection_error = false;
    m_clocking_error = false;
}

void Computer::error_sense_reset()
{
    m_error_sense = false;
}

const Word& Computer::display_lights() const
{
    return m_display_lights;
}

const Opcode& Computer::opcode_lights() const
{
    return m_opcode_lights;
}

const Address& Computer::address_lights() const
{
    return m_address_lights;
}

const Computer::Op_Lights& Computer::operating_lights() const
{
    return m_operating_lights;
}

const Computer::Check_Lights& Computer::checking_lights() const
{
    return m_checking_lights;
}
