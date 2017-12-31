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

namespace IBM650
{
#define OPERATION_STEP(name, body) \
class name : public Operation_Step { \
public: \
    name(Computer& computer) : Operation_Step(computer) {}; \
    virtual bool execute(Computer::Operation op) override body \
};

class Operation_Step
{
public:
    Operation_Step(Computer& computer) : c(computer) {};
    virtual bool execute(Computer::Operation op) { return true; }
protected:
    Computer& c;
};

OPERATION_STEP(Instruction_to_Program_Register,
{
    std::cerr << "I to P addr:=" << c.m_address_register << std::endl;
    TValue addr = c.m_address_register.value();
    if (addr >= 8000 || c.m_drum_index == addr % 50)
    {
        c.m_program_register.load(c.get_storage(c.m_address_register), 0, 0);
        std::cerr << "I to PR: PR=" << c.m_program_register << std::endl;
        return true;
    }
    return false;
})

OPERATION_STEP(Op_and_Address_to_Registers,
{
    //! Absorb instruction_address_to_address_register().  Condition setting the address
    //! register for branch instructions.
    c.m_operation_register.load(c.m_program_register, 0, 0);
    c.m_address_register.load(c.m_program_register, 2, 0);
    std::cerr << c.m_run_time << " Op and DA to reg: Op=" << c.m_operation_register
              << " DA=" << c.m_address_register << std::endl;

    c.m_half_cycle = c.Half_Cycle::data;
    return true;
})

OPERATION_STEP(Instruction_Address_to_Address_Register,
{
    //! Do within op_and_address_to_registers().
    c.m_address_register.load(c.m_program_register, 6, 0);
    std::cerr << c.m_run_time << " IA to R: IA=" << c.m_address_register << std::endl;

    c.m_half_cycle = c.Half_Cycle::instruction;
    return true;
})

OPERATION_STEP(Enable_Program_Register,
{
    std::cerr << "enable PR\n";
    return true;
})

OPERATION_STEP(Enable_Distributor, { return true; });

OPERATION_STEP(Data_to_Distributor,
{
    Address addr;
    switch (op)
    {
    case Computer::Operation::store_lower_in_memory:
        c.m_distributor = c.m_lower_accumulator;
        return true;
    case Computer::Operation::store_lower_data_address:
        addr.load(c.m_lower_accumulator, 2, 0);
        c.m_distributor.load(addr, 0, 2);
        return true;
    case Computer::Operation::store_lower_instruction_address:
        addr.load(c.m_lower_accumulator, 6, 0);
        c.m_distributor.load(addr, 0, 6);
        return true;
    case Computer::Operation::store_upper_in_memory:
        c.m_distributor = c.m_upper_accumulator;
        return true;
    }

    if (c.m_drum_index == c.m_address_register.value() % 50)
    {
        c.m_distributor = c.get_storage(c.m_address_register);
        return true;
    }
    return false;
})

OPERATION_STEP(Distributor_to_Accumulator,
{
    std::cerr << c.m_run_time << " Dist to Acc: Dist=" << c.m_distributor << std::endl;

    // Wait for even time
    if (!c.m_restart && c.m_run_time % 2 != 0)
        return false;

    // Start looking for next instruction.
    c.m_restart = true;

    // It takes 2 cycles to fill the accumulator and we start on an even time.
    if (c.m_run_time % 2 == 0)
        return false;

    TDigit carry = 0;

    switch (c.m_operation)
    {
    case Computer::Operation::add_to_upper:
        c.add_to_accumulator(c.m_distributor, true, carry);
        break;
    case Computer::Operation::subtract_from_upper:
        c.add_to_accumulator(change_sign(c.m_distributor), true, carry);
        break;
    case Computer::Operation::add_to_lower:
        c.add_to_accumulator(c.m_distributor, false, carry);
        break;
    case Computer::Operation::subtract_from_lower:
        c.add_to_accumulator(change_sign(c.m_distributor), false, carry);
        break;
    case Computer::Operation::add_absolute_to_lower:
        c.add_to_accumulator(abs(c.m_distributor), false, carry);
        break;
    case Computer::Operation::subtract_absolute_from_lower:
        c.add_to_accumulator(change_sign(abs(c.m_distributor)), false, carry);
        break;
    case Computer::Operation::reset_and_add_into_upper:
        c.m_upper_accumulator = c.m_distributor;
        c.m_lower_accumulator.fill(0, c.m_upper_accumulator.sign());
        break;
    case Computer::Operation::reset_and_subtract_into_upper:
        c.m_upper_accumulator = change_sign(c.m_distributor);
        c.m_lower_accumulator.fill(0, c.m_upper_accumulator.sign());
        break;
    case Computer::Operation::reset_and_add_into_lower:
        c.m_lower_accumulator = c.m_distributor;
        c.m_upper_accumulator.fill(0, c.m_lower_accumulator.sign());
        break;
    case Computer::Operation::reset_and_subtract_into_lower:
        c.m_lower_accumulator = change_sign(c.m_distributor);
        c.m_upper_accumulator.fill(0, c.m_lower_accumulator.sign());
        break;
    case Computer::Operation::reset_and_add_absolute_into_lower:
        c.m_lower_accumulator = abs(c.m_distributor);
        c.m_upper_accumulator.fill(0, c.m_lower_accumulator.sign());
        break;
    case Computer::Operation::reset_and_subtract_absolute_into_lower:
        c.m_lower_accumulator = change_sign(abs(c.m_distributor));
        c.m_upper_accumulator.fill(0, c.m_lower_accumulator.sign());
        break;
    default:
        assert(false);
    }
    c.m_overflow = carry > 0;
    return true;
})

OPERATION_STEP(Remove_Interlock_A,
{
    std::cerr << c.m_run_time << " remove interlock A\n";
    c.m_restart = false;
    return true;
})

OPERATION_STEP(Enable_Position_Set, { return true; })

OPERATION_STEP(Store_Distributor,
{
    std::cerr << c.m_run_time << " store dist: addr=" << c.m_address_register
              << " dist=" << c.m_distributor << std::endl;

    auto addr = c.m_address_register.value();
    if (addr >= c.m_drum_capacity)
    {
        c.m_storage_selection_error = true;
        return true;
    }
    if (c.m_drum_index == addr % 50)
    {
        c.set_storage(c.m_address_register, c.m_distributor);
        return true;
    }
    return false;
})

class Multiply : public Operation_Step
{
public:
    Multiply(Computer& computer) : Operation_Step(computer) {}

    virtual bool execute(Computer::Operation op) override {
        // Match the accumulator sign to the distributor so that the absolute value of the lower
        // adds to the absolute value of the product, i.e the value in lower makes the product more
        // positive if the product is positive, and more negative if it's negative.
        if (m_shift_count == 0 && m_upper_overflow == 0)
        {
            c.m_upper_accumulator[0] = c.m_distributor[0];
            c.m_lower_accumulator[0] = c.m_distributor[0];
        }

        if (m_upper_overflow == 0)
        {
            // Record the high digit and shift left.
            m_upper_overflow = dec(c.m_upper_accumulator[word_size]);
            c.shift_accumulator();
            ++m_shift_count;
            return false;
        }

        // Add the distributor until the loop count gets to 0.
        TDigit carry = 0;
        // Signal overflow if the product overflows its 10 digits and changes the units digit of
        // the multiplier.
        TDigit multiplier_units = c.m_upper_accumulator[m_shift_count+1];
        c.add_to_accumulator(c.m_distributor, false, carry);
        c.m_overflow = c.m_overflow || c.m_upper_accumulator[m_shift_count+1] != multiplier_units;
        --m_upper_overflow;
        if (m_upper_overflow > 0 || m_shift_count < word_size)
            return false;

        // Reset the shift count for the next multiplication.
        m_shift_count = 0;
        return true;
    }
private:
    TDigit m_upper_overflow = 0;
    std::size_t m_shift_count = 0;
};

class Divide : public Operation_Step
{
public:
    Divide(Computer& computer) : Operation_Step(computer) {}

    virtual bool execute(Computer::Operation op) override {
        if (m_shift_count == 0 && m_upper_overflow == 0)
            c.m_lower_accumulator[0]
                = bin(c.m_distributor.sign() == c.m_lower_accumulator.sign() ? '+' : '-');

        if (m_shift)
        {
            // Record the high digit and shift left.
            m_upper_overflow = dec(c.m_upper_accumulator[word_size]);
            c.shift_accumulator();
            ++m_shift_count;
            m_shift = false;
            return false;
        }

        // Subtract the distributor until the sign changes.
        TDigit carry = 0;
        Signed_Register<word_size+1> a;
        a[word_size+1] = m_upper_overflow;
        a.load(abs(c.m_upper_accumulator), 0, 1);
        Signed_Register<word_size+1> b;
        b[word_size+1] = 0;
        b.load(abs(c.m_distributor), 0, 1);
        a = add(a, change_sign(b), carry);
        if (a.sign() == '+')
        {
            TDigit sign = c.m_upper_accumulator[0];
            c.m_upper_accumulator.load(a, 1, 0);
            c.m_upper_accumulator[0] = sign;
            TDigit ones = dec(c.m_lower_accumulator[1]);
            if (ones == 9)
            {
                //! The machine should stop unconditionally on quotient overflow.
                c.m_overflow = true;
                m_shift_count = 0;
                return true;
            }
            c.m_lower_accumulator[1] = bin(ones + 1);
            return false;
        }

        m_shift = true;
        a = add(a, b, carry);

        if (m_shift_count < word_size)
            return false;

        if (c.m_operation == Computer::Operation::divide_and_reset_upper)
            c.m_upper_accumulator.fill(0, c.m_lower_accumulator.sign());

        // Reset the shift count for the next division.
        m_shift_count = 0;
        return true;
    }
private:
    TDigit m_upper_overflow = 0;
    std::size_t m_shift_count = 0;
    bool m_shift = true;
};
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
    m_next_instruction_steps = {
        std::make_shared<Instruction_to_Program_Register>(*this),
        std::make_shared<Op_and_Address_to_Registers>(*this),
        std::make_shared<Instruction_Address_to_Address_Register>(*this),
        std::make_shared<Enable_Program_Register>(*this)
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

Computer::Op_Sequence Computer::operation_steps(Operation op)
{
    Op_Sequence steps;
    switch (op)
    {
    case Operation::no_operation:
    case Operation::stop:
        break;
    case Operation::load_distributor:
        steps.push_back(std::make_shared<Enable_Distributor>(*this));
        steps.push_back(std::make_shared<Data_to_Distributor>(*this));
        break;
    case Operation::add_to_upper:
    case Operation::subtract_from_upper:
    case Operation::add_to_lower:
    case Operation::subtract_from_lower:
    case Operation::add_absolute_to_lower:
    case Operation::subtract_absolute_from_lower:
    case Operation::reset_and_add_into_upper:
    case Operation::reset_and_subtract_into_upper:
    case Operation::reset_and_add_into_lower:
    case Operation::reset_and_subtract_into_lower:
    case Operation::reset_and_add_absolute_into_lower:
    case Operation::reset_and_subtract_absolute_into_lower:
        steps.push_back(std::make_shared<Enable_Distributor>(*this));
        steps.push_back(std::make_shared<Data_to_Distributor>(*this));
        steps.push_back(std::make_shared<Distributor_to_Accumulator>(*this));
        steps.push_back(std::make_shared<Remove_Interlock_A>(*this));
        break;
    case Operation::store_distributor:
        steps.push_back(std::make_shared<Enable_Position_Set>(*this));
        steps.push_back(std::make_shared<Store_Distributor>(*this));
        break;
    case Operation::store_lower_in_memory:
    case Operation::store_upper_in_memory:
        steps.push_back(std::make_shared<Enable_Distributor>(*this));
        steps.push_back(std::make_shared<Data_to_Distributor>(*this));
        steps.push_back(std::make_shared<Store_Distributor>(*this));
        break;
    case Operation::store_lower_data_address:
    case Operation::store_lower_instruction_address:
        steps.push_back(std::make_shared<Data_to_Distributor>(*this));
        steps.push_back(std::make_shared<Store_Distributor>(*this));
        break;
    case Operation::multiply:
        steps.push_back(std::make_shared<Enable_Distributor>(*this));
        steps.push_back(std::make_shared<Data_to_Distributor>(*this));
        steps.push_back(std::make_shared<Multiply>(*this));
        steps.push_back(std::make_shared<Remove_Interlock_A>(*this));
        break;
    case Operation::divide:
    case Operation::divide_and_reset_upper:
        steps.push_back(std::make_shared<Enable_Distributor>(*this));
        steps.push_back(std::make_shared<Data_to_Distributor>(*this));
        steps.push_back(std::make_shared<Divide>(*this));
        steps.push_back(std::make_shared<Remove_Interlock_A>(*this));
        break;
    }
    return steps;
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
            for (m_next_op_it = m_next_instruction_steps.begin();
                 m_half_cycle == Half_Cycle::instruction; )
            {
                // Execute the operation.  Go on to the next operation if this one is done.
                if ((*m_next_op_it)->execute(m_operation))
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
            //!! make non-member
            m_operation = Operation(m_operation_register.value());
            std::cerr << "D: op=" << static_cast<int>(m_operation) << std::endl;
            m_operation_register.clear();

            bool restarted = false;
            auto op_seq = operation_steps(m_operation);
            auto op_end = op_seq.end();
            auto inst_end = m_next_instruction_steps.end();
            // The operation sequence and the next address sequence may happen in parallel.
            // Loop until both are done.
            for (auto op_it = op_seq.begin(); op_it != op_end || m_next_op_it != inst_end; )
            {
                if (op_it != op_end)
                    if ((*op_it)->execute(m_operation))
                        ++op_it;
                // Don't bother looking for the next address if the program is done.
                //! m_programmed_mode must be "stop"
                if (m_operation == Operation::stop && op_it == op_end)
                    return;

                if ((m_restart || op_it == op_end) && m_next_op_it != inst_end)
                {
                    // It takes a cycle to process the "restart" signal and begin parallel
                    // execution.  So the first time through, we just set the "restarted"
                    // flag.
                    if (restarted || op_it == op_end)
                        if ((*m_next_op_it)->execute(m_operation))
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
    m_upper_accumulator.fill(0, '+');
    m_lower_accumulator.fill(0, '+');
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
        return get_storage(lower_accumulator_address);
    case Display_Mode::upper_accumulator:
        return get_storage(upper_accumulator_address);
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
    return !m_upper_accumulator.is_valid() || !m_lower_accumulator.is_valid();
}

bool Computer::program_register_validity_error() const
{
    return !m_program_register.is_valid();
}

bool Computer::storage_selection_error() const
{
    if (m_address_register.is_blank())
        return false;
    auto address = m_address_register.value();
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
        return m_lower_accumulator;
    else if (address == upper_accumulator_address)
        return m_upper_accumulator;

    assert(address.value() < m_drum.size());
    return m_drum[address.value()];
}

// The manual says the upper sign is affected by reset, multiplying and, dividing.  Addition
// and subtraction are not in that list, but it's not clear if the upper sign should be
// considered when adding to upper.  It's easiest to ignore (but preserve) the upper sign and
// treat the accumulator as one big register with the sign of the lower.  This strategy avoids
// the need for special cases for carrying and complementing, and gives the same answers as the
// examples in the manual.
void Computer::add_to_accumulator(const Word& reg, bool to_upper, TDigit& carry)
{
    // Make 20-digit registers for the accumulator and the argument.  Take the sign of the
    // lower accumulator.
    Signed_Register<2*word_size> accum;
    accum.load(m_upper_accumulator, 0, 0);
    accum.load(m_lower_accumulator, 0, word_size);
    Signed_Register<2*word_size> rhs;
    rhs.fill(0, '+');
    rhs.load(reg, 0, word_size);
    accum = add(accum, shift(rhs, to_upper ? word_size : 0), carry);
    // Copy the upper and lower parts of the sums to the registers, preserving the upper sign.
    TDigit upper_sign = m_upper_accumulator[0];
    m_upper_accumulator.load(accum, 0, 0);
    m_upper_accumulator[0] = upper_sign;
    m_lower_accumulator.load(accum, word_size, 0);
}

void Computer::shift_accumulator()
{
    Signed_Register<2*word_size> accum;
    accum.load(m_upper_accumulator, 0, 0);
    accum.load(m_lower_accumulator, 0, word_size);
    accum = shift(accum, 1);
    TDigit sign = m_upper_accumulator[0];
    m_upper_accumulator.load(accum, 0, 0);
    m_upper_accumulator[0] = sign;
    m_lower_accumulator.load(accum, word_size, 0);
}

#ifdef TEST
void Computer::set_distributor(const Word& reg)
{
    m_distributor = reg;
}

void Computer::set_upper(const Word& reg)
{
    m_upper_accumulator = reg;
}

void Computer::set_lower(const Word& reg)
{
    m_lower_accumulator = reg;
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
