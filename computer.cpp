#include "computer.hpp"

#include <cassert>

using namespace IBM650;

namespace
{
    /// DC power comes on 3 minutes after main power.
    const TTime dc_on_delay_seconds = 180;
    /// The blower stays on 5 minutes after main power is turned off.
    const TTime blower_off_delay_seconds = 300;

    const Address storage_entry_address({8,0,0,0});
    //! other 800x addresses.
}

/// The initial state is: powered off for long enough that the blower is off.
Computer::Computer()
    : m_elapsed_seconds(blower_off_delay_seconds),
      m_can_turn_on(true),
      m_power_on(false),
      m_dc_on(false),
      m_control_mode(Control_Mode::run), //!TODO make persistent
      m_cycle_mode(Half_Cycle_Mode::run), //!TODO make persistent
      m_display_mode(Display_Mode::distributor),
      m_half_cycle(Half_Cycle::instruction),
      m_overflow(false),
      m_storage_selection_error(false),
      m_clocking_error(false),
      m_error_sense(false)
{
    m_operation_map[Operation::next_instruction] = {
        std::bind(&Computer::enable_program_register, this),
        std::bind(&Computer::search_for_instruction, this),
        std::bind(&Computer::instruction_to_program_register, this),
        std::bind(&Computer::op_and_address_to_registers, this),
        std::bind(&Computer::instruction_address_to_address_register, this)
    };
    m_next_op_it = m_operation_map[Operation::next_instruction].begin();

    m_operation_map[Operation::no_operation] = {};
    m_operation_map[Operation::stop] = {};
    m_operation_map[Operation::add_to_upper] = {
        std::bind(&Computer::enable_distributor, this),
        std::bind(&Computer::search_for_data_location, this),
        std::bind(&Computer::data_to_distributor, this),
        std::bind(&Computer::wait_for_even, this),
        std::bind(&Computer::distributor_to_accumulator, this),
        std::bind(&Computer::compliment, this),
        std::bind(&Computer::remove_interlock_a, this),
    };
    m_operation_map[Operation::reset_and_add_to_lower] = {
        std::bind(&Computer::enable_distributor, this),
        std::bind(&Computer::search_for_data_location, this),
        std::bind(&Computer::data_to_distributor, this),
        std::bind(&Computer::wait_for_even, this),
        std::bind(&Computer::distributor_to_accumulator, this),
        std::bind(&Computer::compliment, this),
        std::bind(&Computer::remove_interlock_a, this),
    };
    m_operation_map[Operation::load_distributor] = {
        std::bind(&Computer::enable_distributor, this),
        std::bind(&Computer::search_for_data_location, this),
        std::bind(&Computer::data_to_distributor, this),
    };
}

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

void Computer::set_storage_entry(const Word& word)
{
    m_storage_entry = word;
}

void Computer::set_programmed(Programmed_Mode mode)
{
    m_programmed_mode = mode;
}

void Computer::set_half_cycle(Half_Cycle_Mode mode)
{
    m_cycle_mode = mode;
}

void Computer::set_control(Control_Mode mode)
{
    if (mode == m_control_mode)
        return;

    m_control_mode = mode;
    if (m_control_mode == Control_Mode::run && m_cycle_mode == Half_Cycle_Mode::half)
    {
        m_half_cycle = Half_Cycle::instruction;
        m_operation_register.clear();
        m_address_register.clear();
    }
}

void Computer::set_display(Display_Mode mode)
{
    m_display_mode = mode;
}

void Computer::set_address(const Address& address)
{
    m_address_entry = address;
}

void Computer::set_accumulator(const Signed_Register<20>& reg)
{
    m_accumulator = reg;
}

void Computer::set_program_register(const Word& reg)
{
    m_program_register.load(reg, 0, 0);
    // Copy the operation and address to those registers.
    m_operation_register.load(reg, 0, 0);
    m_address_register.load(reg, 2, 0);
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
    if (m_control_mode == Control_Mode::manual)
        m_address_register = m_address_entry;
}

void Computer::program_start()
{
    if (m_control_mode == Control_Mode::manual)
    {
        m_distributor = m_storage_entry;

        // It's odd that what happens on program start depends on the display mode, but that
        // appears to be the case.
        switch (m_display_mode)
        {
        case Display_Mode::read_in_storage:
            set_storage(m_address_entry, m_distributor);
            break;
        case Display_Mode::read_out_storage:
            m_distributor = get_storage(m_address_entry);
            break;
        default:
            // I don't know what happens if you start in manual mode with other display
            // settings.  Let's assume it just sets the distributor.
            break;
        }
        return;
    }

    while (true)
    {
        if (m_half_cycle == Half_Cycle::instruction)
        {
            std::cerr << "I\n";
            // Load the data address.
            for (m_next_op_it = m_operation_map[Operation::next_instruction].begin();
                 m_half_cycle == Half_Cycle::instruction;
                 ++m_next_op_it)
                while (!(*m_next_op_it)());

            if (m_cycle_mode == Half_Cycle_Mode::half)
                return;
        }

        if (m_half_cycle == Half_Cycle::data)
        {
            m_operation = Operation(m_operation_register.value());
            std::cerr << "D: op=" << static_cast<int>(m_operation) << std::endl;
            m_operation_register.clear();
            // Do the operation steps.
            for (auto step : m_operation_map[m_operation])
                while (!step());
            // Load the address of the next instruction.
            for ( ; m_next_op_it != m_operation_map[Operation::next_instruction].end(); ++m_next_op_it)
                while (!(*m_next_op_it)());

            if (m_cycle_mode == Half_Cycle_Mode::half)
                return;
            if (m_operation == Operation::stop)
                return;
        }
    }
}

void Computer::program_reset()
{
    m_program_register.fill(0);
    m_operation_register.clear();
    if (m_control_mode == Control_Mode::manual)
        m_address_register.clear();
    else
        m_address_register = Address({8,0,0,0});

    m_storage_selection_error = false;
    m_clocking_error = false;
}

void Computer::computer_reset()
{
    program_reset();
    accumulator_reset();
    error_sense_reset();
    if (m_control_mode != Control_Mode::manual)
        m_address_register = storage_entry_address;
}

void Computer::accumulator_reset()
{
    m_distributor.fill(0, '+');
    m_accumulator.fill(0, '+');
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

Word Computer::display() const
{
    switch (m_display_mode)
    {
    case Display_Mode::lower_accumulator:
    {
        Word lower;
        lower.load(m_accumulator, 10, 0);
        return lower;
    }
    case Display_Mode::upper_accumulator:
    {
        Word upper;
        upper.load(m_accumulator, 0, 0);
        upper.digits().back() = bin('_');
        return upper;
    }
    case Display_Mode::program_register:
        return Word(m_program_register, '_');
    default:
        return m_distributor;
    }
}

const Register<2>& Computer::operation_register() const
{
    return m_operation_register;
}

const Address& Computer::address_register() const
{
    return m_address_register;
}
 
bool Computer::data_address() const
{
    return m_half_cycle == Half_Cycle::data;
}
 
bool Computer::instruction_address() const
{
    return m_half_cycle == Half_Cycle::instruction;
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
    return (address > 1999 && (address < storage_entry_address.value() || address > 8003))
        || m_storage_selection_error;
}

bool Computer::clocking_error() const
{
    return m_clocking_error;
}

bool Computer::error_sense() const
{
    return m_error_sense;
}


void Computer::set_storage(const Address& address, const Word& word)
{
    //!need to wait for location to pass read head.
    m_drum[address.value()] = word;
}

const Word& Computer::get_storage(const Address& address) const
{
    std::cerr << "get_storage " << address.value() << std::endl;
    assert(!address.is_blank());
    if (address == storage_entry_address)
        return m_storage_entry;

    //! other 800x addresses

    //!need to wait for location to pass read head.
    assert(address.value() < m_drum.size());
    return m_drum[address.value()];
}

bool Computer::instruction_to_program_register()
{
    m_program_register.load(get_storage(m_address_register), 0, 0);
    std::cerr << "I to PR: addr=" << m_address_register 
              << " word=" << m_program_register << std::endl;
    return true;
}

bool Computer::op_and_address_to_registers()
{
    m_operation_register.load(m_program_register, 0, 0);
    m_address_register.load(m_program_register, 2, 0);
    std::cerr << "Op and DA to reg: Op=" << m_operation_register
              << " DA=" << m_address_register << std::endl;

    m_half_cycle = Half_Cycle::data;
    return true;
}

bool Computer::instruction_address_to_address_register()
{
    m_address_register.load(m_program_register, 6, 0);
    std::cerr << "IA to R: IA=" << m_address_register << std::endl;

    m_half_cycle = Half_Cycle::instruction;
    return true;
}

bool Computer::enable_distributor()
{
    std::cerr << "enable distributor\n";
    return true;
}

bool Computer::search_for_data_location()
{
    std::cerr << "search for data loc: addr=" << m_address_register << std::endl;
    //! wait for drum location to get to read head
    return true;
}

bool Computer::data_to_distributor()
{
    std::cerr << "data to dist: addr=" << m_address_register << std::endl;
    m_distributor = get_storage(m_address_register);
    return true;
}

bool Computer::wait_for_even()
{
    std::cerr << "wait for even\n";
    return true;
}

bool Computer::distributor_to_accumulator()
{
    std::cerr << "Dist to Acc: Dist=" << m_distributor << std::endl;

    switch (m_operation)
    {
    case Operation::reset_and_add_to_lower:
        m_accumulator.load(m_distributor, 0, 10);
        break;
    case Operation::add_to_upper:
    {
        Signed_Register<20> rhs;
        rhs.load(m_distributor, 0, 10);
        char carry;
        m_accumulator = add(m_accumulator, shift(rhs, 10), carry);
        break;
    }
    default:
        assert(false);
        break;
    }
    return true;
}

bool Computer::compliment()
{
    std::cerr << "compliment\n";
    return true;
}

bool Computer::remove_interlock_a()
{
    std::cerr << "remove interlock A\n";
    return true;
}

bool Computer::enable_program_register()
{
    std::cerr << "enable PR\n";
    return true;
}

bool Computer::search_for_instruction()
{
    std::cerr << "search for inst: addr=" << m_address_register << std::endl;
    //! wait for drum location to get to read head
    return true;
}
