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
    const Address distributor_address({8,0,0,1});
    const Address lower_accumulator_address({8,0,0,2});
    const Address upper_accumulator_address({8,0,0,3});
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
      m_run_time(0),
      m_restart(false),
      m_overflow(false),
      m_storage_selection_error(false),
      m_clocking_error(false),
      m_error_sense(false),
      m_drum_index(0)
{
    m_next_instruction_step = {
        std::bind(&Computer::instruction_to_program_register, this),
        std::bind(&Computer::op_and_address_to_registers, this),
        std::bind(&Computer::instruction_address_to_address_register, this),
        std::bind(&Computer::enable_program_register, this)
    };

    m_operation_steps.resize(4);
    m_operation_steps[1] = {
        std::bind(&Computer::enable_distributor, this),
        std::bind(&Computer::data_to_distributor, this)
    };
    m_operation_steps[2] = {
        std::bind(&Computer::enable_distributor, this),
        std::bind(&Computer::data_to_distributor, this),
        std::bind(&Computer::distributor_to_accumulator, this),
        std::bind(&Computer::remove_interlock_a, this)
    };
    m_operation_steps[3] = {
        std::bind(&Computer::enable_position_set, this),
        std::bind(&Computer::store_distributor, this)
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

void Computer::set_programmed_mode(Programmed_Mode mode)
{
    m_programmed_mode = mode;
}

void Computer::set_half_cycle_mode(Half_Cycle_Mode mode)
{
    m_cycle_mode = mode;
}

void Computer::set_control_mode(Control_Mode mode)
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

void Computer::set_display_mode(Display_Mode mode)
{
    m_display_mode = mode;
}

void Computer::set_error_mode(Error_Mode mode)
{
    m_error_mode = mode;
}

void Computer::set_address(const Address& address)
{
    m_address_entry = address;
}

Computer::Control_Mode Computer::get_control_mode() const
{
    return m_control_mode;
}

Computer::Display_Mode Computer::get_display_mode() const
{
    return m_display_mode;
}

void Computer::transfer()
{
    // Only works in manual control.
    if (m_control_mode == Control_Mode::manual)
        m_address_register = m_address_entry;
}

std::size_t Computer::operation_index(Operation op) const
{
    switch (op)
    {
    case Operation::no_operation:
    case Operation::stop:
        return 0;
    case Operation::load_distributor:
        return 1;
    case Operation::add_to_upper:
    case Operation::subtract_from_upper:
    case Operation::add_to_lower:
    case Operation::subtract_from_lower:
    case Operation::reset_and_add_into_upper:
    case Operation::reset_and_subtract_into_upper:
    case Operation::reset_and_add_into_lower:
    case Operation::reset_and_subtract_into_lower:
        return 2;
    case Operation::store_distributor:
        return 3;
    }
}

void Computer::program_start()
{
    std::cerr << "program start\n";
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
            for (m_next_op_it = m_next_instruction_step.begin();
                 m_half_cycle == Half_Cycle::instruction; )
            {
                if ((*m_next_op_it)())
                    ++m_next_op_it;
                ++m_run_time;
                m_drum_index = (m_drum_index + 1) % 50;
            }
            if (m_cycle_mode == Half_Cycle_Mode::half)
                return;
        }
        // m_half_cycle changes during execution.  The ifs are not exclusive.
        if (m_half_cycle == Half_Cycle::data)
        {
            m_operation = Operation(m_operation_register.value());
            std::cerr << "D: op=" << static_cast<int>(m_operation) << std::endl;
            m_operation_register.clear();

            bool restarted = false;
            auto op_seq = m_operation_steps[operation_index(m_operation)];
            auto op_end = op_seq.end();
            auto inst_end = m_next_instruction_step.end();
            for (auto op_it = op_seq.begin();
                 op_it != op_end || m_next_op_it != inst_end; )
            {
                if (op_it != op_end)
                    if ((*op_it)())
                        ++op_it;

                if (m_operation == Operation::stop && op_it == op_end)
                    return;

                if ((m_restart || op_it == op_end) && m_next_op_it != inst_end)
                {
                    // It takes a cycle to process the "restart" signal and begin parallel
                    // execution.  So the first time through, we just set the "restarted"
                    // flag.
                    if (restarted || op_it == op_end)
                        if ((*m_next_op_it)())
                            ++m_next_op_it;
                    restarted = true;
                }

                ++m_run_time;
                m_drum_index = (m_drum_index + 1) % 50;
            }
            if (m_cycle_mode == Half_Cycle_Mode::half)
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
    m_half_cycle = Half_Cycle::instruction;
    m_run_time = 0;
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
        upper.digits().back() = m_accumulator.digits().back();
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

int Computer::run_time() const
{
    return m_run_time;
}

void Computer::set_storage(const Address& address, const Word& word)
{
    m_drum[address.value()] = word;
}

const Word Computer::get_storage(const Address& address) const
{
    std::cerr << "get_storage " << address.value() << std::endl;
    assert(!address.is_blank());
    if (address == storage_entry_address)
        return m_storage_entry;
    else if (address == distributor_address)
        return m_distributor;
    else if (address == lower_accumulator_address)
    {
        Word lower;
        lower.load(m_accumulator, 10, 0);
        return lower;
    }
    else if (address == upper_accumulator_address)
    {
        Word upper;
        upper.load(m_accumulator, 0, 0);
        upper.digits().back() = bin('_');
        return upper;
    }

    assert(address.value() < m_drum.size());
    return m_drum[address.value()];
}

bool Computer::instruction_to_program_register()
{
    std::cerr << m_run_time << " I to PR: addr=" << m_address_register 
              << " drum=" << m_drum_index << std::endl;

    TValue addr = m_address_register.value();
    if (addr >= 8000 || m_drum_index == addr % 50)
    {
        m_program_register.load(get_storage(m_address_register), 0, 0);
        std::cerr << "I to PR: PR=" << m_program_register << std::endl;
        return true;
    }
    return false;
}

bool Computer::op_and_address_to_registers()
{
    m_operation_register.load(m_program_register, 0, 0);
    m_address_register.load(m_program_register, 2, 0);
    std::cerr << m_run_time << " Op and DA to reg: Op=" << m_operation_register
              << " DA=" << m_address_register << std::endl;

    m_half_cycle = Half_Cycle::data;
    return true;
}

bool Computer::instruction_address_to_address_register()
{
    m_address_register.load(m_program_register, 6, 0);
    std::cerr << m_run_time << " IA to R: IA=" << m_address_register << std::endl;

    m_half_cycle = Half_Cycle::instruction;
    return true;
}

bool Computer::enable_program_register()
{
    std::cerr << "enable PR\n";
    return true;
}


bool Computer::enable_distributor()
{
    std::cerr << m_run_time << " enable distributor\n";
    return true;
}

bool Computer::data_to_distributor()
{
    std::cerr << m_run_time << " data to dist: addr=" << m_address_register
              << " drum=" << m_drum_index << std::endl;

    TValue addr = m_address_register.value();
    if (addr >= storage_entry_address.value() || m_drum_index == addr % 50)
    {
        TDigit sign = m_distributor.digits().back();
        m_distributor = get_storage(m_address_register);
        // Loading the upper accumulator should not disturb the sign.
        if (m_address_register == upper_accumulator_address)
            m_distributor.digits().back() = sign;

        std::cerr << "data to dist: dist=" << m_distributor << std::endl;
        return true;
    }
    return false;
}

bool Computer::distributor_to_accumulator()
{
    std::cerr << m_run_time << " Dist to Acc: Dist=" << m_distributor << std::endl;

    // Wait for even time
    if (!m_restart && m_run_time % 2 != 0)
        return false;
    
    // Start looking for next instruction.
    m_restart = true;

    // It takes 2 cycles to fill the accumulator and we start on an even time.
    if (m_run_time % 2 == 0)
        return false;

    //! compliment takes more cycles

    Signed_Register<20> rhs;
    rhs.fill(0, '+');
    char carry;
    switch (m_operation)
    {
    case Operation::add_to_upper:
        rhs.load(m_distributor, 0, 10);
        m_accumulator = add(m_accumulator, shift(rhs, 10), carry);
        break;
    case Operation::subtract_from_upper:
        rhs.load(m_distributor, 0, 10);
        m_accumulator = add(m_accumulator, change_sign(shift(rhs, 10)), carry);
        break;
    case Operation::add_to_lower:
        rhs.load(m_distributor, 0, 10);
        m_accumulator = add(m_accumulator, rhs, carry);
        break;
    case Operation::subtract_from_lower:
        rhs.load(m_distributor, 0, 10);
        m_accumulator = add(m_accumulator, change_sign(rhs), carry);
        break;
    case Operation::reset_and_add_into_upper:
        rhs.load(m_distributor, 0, 10);
        m_accumulator = shift(rhs, 10);
        break;
    case Operation::reset_and_subtract_into_upper:
        rhs.load(m_distributor, 0, 10);
        m_accumulator = change_sign(shift(rhs, 10));
        break;
    case Operation::reset_and_add_into_lower:
        rhs.load(m_distributor, 0, 10);
        m_accumulator = rhs;
        break;
    case Operation::reset_and_subtract_into_lower:
        rhs.load(m_distributor, 0, 10);
        m_accumulator = change_sign(rhs);
        break;
    default:
        assert(false);
    }
    m_overflow = carry > 0;
    return true;
}

bool Computer::remove_interlock_a()
{
    std::cerr << m_run_time << " remove interlock A\n";
    m_restart = false;
    return true;
}

bool Computer::enable_position_set()
{
    return true;
}

bool Computer::store_distributor()
{
    std::cerr << m_run_time << " store dist: addr=" << m_address_register
              << " dist=" << m_distributor << std::endl;

    TValue addr = m_address_register.value();
    if (addr >= m_drum_capacity)
    {
        m_storage_selection_error = true;
        return true;
    }
    if (m_drum_index == addr % 50)
    {
        set_storage(m_address_register, m_distributor);
        return true;
    }
    return false;
}

#ifdef TEST
void Computer::set_distributor(const Word& reg)
{
    m_distributor = reg;
}

void Computer::set_upper(const Word& reg)
{
    Register<10> upper;
    upper.load(reg, 0, 0);
    m_accumulator.load(upper, 0, 0);
}

void Computer::set_lower(const Word& reg)
{
    m_accumulator.load(reg, 0, 10);
}

void Computer::set_program_register(const Word& reg)
{
    m_program_register.load(reg, 0, 0);
    // Copy the operation and address to those registers.
    m_operation_register.load(reg, 0, 0);
    m_address_register.load(reg, 2, 0);
}

void Computer::set_drum(const Address& address, const Word& word)
{
    m_drum[address.value()] = word;
}

void Computer::set_error()
{
    m_overflow = true;
    m_storage_selection_error = true;
    m_clocking_error = true;
    m_error_sense = true;
}

Word Computer::get_drum(const Address& address) const
{
    return m_drum[address.value()];
}
#endif // TEST
