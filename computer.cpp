#include "computer.hpp"

#include <cassert>

using namespace IBM650;

namespace
{
    /// DC power comes on 3 minutes after main power.
    const TTime dc_on_delay_seconds = 180;
    /// The blower stays on 5 minutes after main power is turned off.
    const TTime blower_off_delay_seconds = 300;
}

/// The initial state is: powered off for long enough that the blower is off.
Computer::Computer()
    : m_elapsed_seconds(blower_off_delay_seconds),
      m_can_turn_on(true),
      m_power_on(false),
      m_dc_on(false),
      m_control_mode(Control::run), //!TODO make persistent
      m_overflow(false),
      m_storage_selection_error(false),
      m_clocking_error(false),
      m_error_sense(false)
{}

void Computer::power_on()
{
    if (m_power_on || !m_can_turn_on)
        return;

    m_elapsed_seconds = 0;
    m_power_on = true;
    assert(!m_dc_on);
}

void Computer::power_off()
{
    m_elapsed_seconds = 0;
    m_power_on = false;
    m_dc_on = false;
}

void Computer::dc_on()
{
    // DC power can be turned on manually only after it's been turned of automatically and then
    // turned off manually.
    bool can_turn_on = m_power_on && m_elapsed_seconds >= dc_on_delay_seconds;
    // We're either in a state where DC can be turned on, or it's currently off.
    assert(can_turn_on || !m_dc_on);
    if (can_turn_on)
        m_dc_on = true;
}

void Computer::dc_off()
{
    // DC power is either already off or it can be turned off.
    m_dc_on = false;
}

void Computer::master_power_off()
{
    power_off();
    m_can_turn_on = false;
}

void Computer::step(int seconds)
{
    // Turn DC on if power has been on long enough.
    if (m_power_on
        && m_elapsed_seconds < dc_on_delay_seconds
        && m_elapsed_seconds + seconds >= dc_on_delay_seconds)
    {
        assert(!m_dc_on);
        m_dc_on = true;
    }
    m_elapsed_seconds += seconds;
}

bool Computer::is_on() const
{
    return m_power_on;
}

bool Computer::is_blower_on() const
{
    // Turning off master power turns off the blower immediately.  After normal power off, the
    // blower stays on for a while.
    return m_can_turn_on && (m_power_on || m_elapsed_seconds < blower_off_delay_seconds);
}

bool Computer::is_ready() const
{
    return m_dc_on;
}

void Computer::set_control(Control mode)
{
    m_control_mode = mode;
}

void Computer::set_address(const Register<4>& address)
{
    m_address_entry = address;
}

void Computer::set_distributor(const Signed_Register<10>& reg)
{
    m_distributor = reg;
}

void Computer::set_accumulator(const Signed_Register<20>& reg)
{
    m_accumulator = reg;
}

void Computer::set_program_register(const Register<10>& reg)
{
    m_program_register = reg;
    std::copy(reg.digits().begin(), reg.digits().begin()+2, m_operation_register.digits().begin());
    std::copy(reg.digits().begin()+2, reg.digits().begin()+6, m_address_register.digits().begin());
}

void Computer::set_error()
{
    m_overflow = true;
    m_storage_selection_error = true;
    m_clocking_error = true;
    m_error_sense = true;
}

void Computer::transfer()
{
    // Only works in manual control.
    if (m_control_mode == Control::manual)
        m_address_register = m_address_entry;
}

void Computer::program_reset()
{
    m_program_register.fill(0);
    m_operation_register.clear();
    if (m_control_mode == Control::manual)
        m_address_register.clear();
    else
        m_address_register = m_address_entry;

    m_storage_selection_error = false;
    m_clocking_error = false;
}

void Computer::accumulator_reset()
{
    m_distributor.fill(0);
    m_accumulator.fill(0);
    m_overflow = false;
    m_storage_selection_error = false;
    m_clocking_error = false;
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

const Signed_Register<10>& Computer::distributor() const
{
    return m_distributor;
}

const Signed_Register<20>& Computer::accumulator() const
{
    return m_accumulator;
}

const Register<10>& Computer::program_register() const
{
    return m_program_register;
}

const Register<2>& Computer::operation_register() const
{
    return m_operation_register;
}

const Register<4>& Computer::address_register() const
{
    return m_address_register;
}

bool Computer::overflow() const
{
    return m_overflow;
}

bool Computer::distributor_validity_error() const
{
    return !m_distributor.is_valid();
}

bool Computer::accumulator_validity_error() const
{
    return !m_accumulator.is_valid();
}

bool Computer::program_register_validity_error() const
{
    return !m_program_register.is_valid();
}

bool Computer::storage_selection_error() const
{
    if (m_address_register.is_blank())
        return false;
    int address = m_address_register.value();
    return (address > 1999 && (address < 8000 || address > 8003)) || m_storage_selection_error;
}

bool Computer::clocking_error() const
{
    return m_clocking_error;
}

bool Computer::error_sense() const
{
    return m_error_sense;
}
