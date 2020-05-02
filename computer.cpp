#include "computer.hpp"

#include <cassert>

using namespace IBM650;


namespace IBM650
{
/// DC power comes on 3 minutes after main power.
const TTime dc_on_delay_seconds = 180;
/// The blower stays on 5 minutes after main power is turned off.
const TTime blower_off_delay_seconds = 300;

const Address storage_entry_address({8,0,0,0});
const Address distributor_address({8,0,0,1});
const Address lower_accumulator_address({8,0,0,2});
const Address upper_accumulator_address({8,0,0,3});

const Word five({0,0, 0,0,0,0, 0,0,0,5, '+'});
const Word negative_five({0,0, 0,0,0,0, 0,0,0,5, '-'});

enum class Operation
{
    no_operation = 00,
    stop = 01,

    add_to_upper = 10,
    subtract_from_upper = 11,
    divide = 14,
    add_to_lower = 15,
    subtract_from_lower = 16,
    add_absolute_to_lower = 17,
    subtract_absolute_from_lower = 18,
    multiply = 19,

    store_lower_in_memory = 20,
    store_upper_in_memory = 21,
    store_lower_data_address = 22,
    store_lower_instruction_address = 23,
    store_distributor = 24,

    shift_right = 30,
    shift_and_round = 31,
    shift_left = 35,
    shift_left_and_count = 36,

    branch_on_nonzero_in_upper = 44,
    branch_on_nonzero = 45,
    branch_on_minus = 46,
    branch_on_overflow = 47,

    reset_and_add_into_upper = 60,
    reset_and_subtract_into_upper = 61,
    divide_and_reset_upper = 64,
    reset_and_add_into_lower = 65,
    reset_and_subtract_into_lower = 66,
    reset_and_add_absolute_into_lower = 67,
    reset_and_subtract_absolute_into_lower = 68,

    load_distributor = 69,

    table_lookup = 84,

    branch_on_8_in_distributor_position_10 = 90
};

std::size_t band_of_address(const Address& addr)
{
    return addr.value() / band_size;
}

std::size_t index_of_address(const Address& addr)
{
    return addr.value() % band_size;
}

class Operation_Step
{
public:
    Operation_Step(Computer& computer, Operation op)
        : c(computer),
          op(op)
        {};
    virtual bool execute() { return true; }
protected:
    Computer& c;
    Operation op;
};

#define OPERATION_STEP(name, body)                                      \
    class name : public Operation_Step {                                \
    public:                                                             \
    name(Computer& computer, Operation op) : Operation_Step(computer, op) {}; \
    virtual bool execute() override body                                \
};

OPERATION_STEP(Instruction_to_Program_Register,
{
    std::cerr << "I to P: addr=" << c.m_address_register
              << " Drum: " << c.m_drum.index() << std::endl;
    if (c.m_address_register.value() >= 8000
        || index_of_address(c.m_address_register) == c.m_drum.index())
    {
        c.m_program_register.load(c.get_storage(c.m_address_register), 0, 0);
        std::cerr << "I to PR: PR=" << c.m_program_register << std::endl;
        return true;
    }
    return false;
})

OPERATION_STEP(Op_and_Address_to_Registers,
{
    c.m_operation_register.load(c.m_program_register, 0, 0);
    c.m_address_register.load(c.m_program_register, 2, 0);
    std::cerr << c.m_run_time << " Op and DA to reg: Op=" << c.m_operation_register
              << " DA=" << c.m_address_register << std::endl;

    c.m_half_cycle = c.Half_Cycle::data;
    return true;
})

OPERATION_STEP(Instruction_Address_to_Address_Register,
{
    bool branch = false;
    switch (op)
    {
    case Operation::branch_on_nonzero_in_upper:
        branch = abs(c.m_upper_accumulator) != zero;
        break;
    case Operation::branch_on_nonzero:
        branch = abs(c.m_upper_accumulator) != zero || abs(c.m_lower_accumulator) != zero;
        break;
    case Operation::branch_on_minus:
        branch = c.m_lower_accumulator.sign() == '-';
        break;
    case Operation::branch_on_overflow:
        branch = c.m_overflow;
        break;
    default:
    {
        // Positions are counted from least significant to most significant.  The opcode for
        // position 10 is 90; the others are 90 + position.
        std::size_t pos = static_cast<int>(op)
            - static_cast<int>(Operation::branch_on_8_in_distributor_position_10);
        pos = pos == 0 ? word_size : pos;
        if (0 < pos && pos <= word_size)
        {
            TDigit digit = dec(c.m_distributor[pos]);
            branch = digit == 8;
            c.m_error_stop = !branch && digit != 9;
        }
    }
    }

    if (!branch)
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
    std::cerr << c.m_run_time << " Data to Dist\n";
    Address addr;
    switch (op)
    {
    case Operation::store_lower_in_memory:
        c.m_distributor = c.m_lower_accumulator;
        return true;
    case Operation::store_lower_data_address:
        addr.load(c.m_lower_accumulator, 2, 0);
        c.m_distributor.load(addr, 0, 2);
        return true;
    case Operation::store_lower_instruction_address:
        addr.load(c.m_lower_accumulator, 6, 0);
        c.m_distributor.load(addr, 0, 6);
        return true;
    case Operation::store_upper_in_memory:
        c.m_distributor = c.m_upper_accumulator;
        return true;
    default:
        break;
    }

    std::cerr << "  addr=" << c.m_address_register << std::endl;
    if (index_of_address(c.m_address_register) == c.m_drum.index())
    {
        c.m_distributor = c.get_storage(c.m_address_register);
        std::cerr << "  dist=" << c.m_distributor << std::endl;
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

    switch (op)
    {
    case Operation::add_to_upper:
        c.add_to_accumulator(c.m_distributor, true, carry);
        break;
    case Operation::subtract_from_upper:
        c.add_to_accumulator(change_sign(c.m_distributor), true, carry);
        break;
    case Operation::add_to_lower:
        c.add_to_accumulator(c.m_distributor, false, carry);
        break;
    case Operation::subtract_from_lower:
        c.add_to_accumulator(change_sign(c.m_distributor), false, carry);
        break;
    case Operation::add_absolute_to_lower:
        c.add_to_accumulator(abs(c.m_distributor), false, carry);
        break;
    case Operation::subtract_absolute_from_lower:
        c.add_to_accumulator(change_sign(abs(c.m_distributor)), false, carry);
        break;
    case Operation::reset_and_add_into_upper:
        c.m_upper_accumulator = c.m_distributor;
        c.m_lower_accumulator.fill(0, c.m_upper_accumulator.sign());
        break;
    case Operation::reset_and_subtract_into_upper:
        c.m_upper_accumulator = change_sign(c.m_distributor);
        c.m_lower_accumulator.fill(0, c.m_upper_accumulator.sign());
        break;
    case Operation::reset_and_add_into_lower:
        c.m_lower_accumulator = c.m_distributor;
        c.m_upper_accumulator.fill(0, c.m_lower_accumulator.sign());
        break;
    case Operation::reset_and_subtract_into_lower:
        c.m_lower_accumulator = change_sign(c.m_distributor);
        c.m_upper_accumulator.fill(0, c.m_lower_accumulator.sign());
        break;
    case Operation::reset_and_add_absolute_into_lower:
        c.m_lower_accumulator = abs(c.m_distributor);
        c.m_upper_accumulator.fill(0, c.m_lower_accumulator.sign());
        break;
    case Operation::reset_and_subtract_absolute_into_lower:
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

    if (band_of_address(c.m_address_register) >= n_bands)
    {
        c.m_storage_selection_error = true;
        return true;
    }
    if (index_of_address(c.m_address_register) == c.m_drum.index())
    {
        c.set_storage(c.m_address_register, c.m_distributor);
        return true;
    }
    return false;
})

class Multiply : public Operation_Step
{
public:
    Multiply(Computer& computer, Operation op) : Operation_Step(computer, op) {}

    virtual bool execute() override {
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
            c.shift_accumulator(1);
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

        return true;
    }

private:
    TDigit m_upper_overflow = 0;
    std::size_t m_shift_count = 0;
};

class Divide : public Operation_Step
{
public:
    Divide(Computer& computer, Operation op) : Operation_Step(computer, op) {}

    virtual bool execute() override {
        if (m_shift_count == 0 && m_upper_overflow == 0)
            c.m_lower_accumulator[0]
                = bin(c.m_distributor.sign() == c.m_lower_accumulator.sign() ? '+' : '-');

        if (m_shift)
        {
            // Record the high digit and shift left.
            m_upper_overflow = dec(c.m_upper_accumulator[word_size]);
            c.shift_accumulator(1);
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

        if (op == Operation::divide_and_reset_upper)
            c.m_upper_accumulator.fill(0, c.m_lower_accumulator.sign());

        return true;
    }

private:
    TDigit m_upper_overflow = 0;
    std::size_t m_shift_count = 0;
    bool m_shift = true;
};

OPERATION_STEP(Enable_Shift_Control,
{
    std::cerr << "Enable shift control\n";
    // 1 word time + 1 if odd time
    return c.m_run_time % 2 == 0;
})

class Shift : public Operation_Step
{
public:
    Shift(Computer& computer, Operation op)
        : Operation_Step(computer, op),
          m_shift_count(base - dec(c.m_address_register[0]))
        {
            // The shift count starts at the complement of the address's units digit and counts
            // up to 10.  For shift and count, shifting may stop before we get to 10.  Whatever
            // we get to is what goes into the lower accumulator.
            if (m_shift_count == base)
                m_shift_count = 0;
            if (op == Operation::shift_left_and_count && dec(c.m_upper_accumulator[word_size]) > 0)
                m_shift_count = 0;
        }

    virtual bool execute() override {
        if (op == Operation::shift_left_and_count
            && (dec(c.m_upper_accumulator[word_size]) > 0
                || m_shift_count == base))
        {
            // Put the shift count in the zeroes shifted into the lower accumulator.  If no
            // shifting took place, insert zeroes.
            c.m_lower_accumulator[1] = bin(m_shift_count % base);
            c.m_lower_accumulator[2] = bin(m_shift_count / base);
            c.m_overflow = dec(c.m_upper_accumulator[word_size]) == 0;
            return true;
        }

        ++m_shift_count;

        switch (op)
        {
        case Operation::shift_right:
        case Operation::shift_and_round:
            c.shift_accumulator(-1);
            break;
        case Operation::shift_left:
        case Operation::shift_left_and_count:
            c.shift_accumulator(1);
            break;
        default:
            break;
        }

        if (op == Operation::shift_and_round && m_shift_count == 9)
        {
            // Round by adding 5 to the last digit to be shifted off.  Subtract five if the
            // accumulator sign is negative (round away from zero).
            TDigit carry;
            c.add_to_accumulator(c.m_lower_accumulator.sign() == '-' ? negative_five : five,
                                 false, carry);
        }

        return op != Operation::shift_left_and_count && m_shift_count == base;
    }
        
private:
    std::size_t m_shift_count;
};

class Look_Up_Address : public Operation_Step
{
public:
    Look_Up_Address(Computer& computer, Operation op)
        : Operation_Step(computer, op),
          m_band(-1)
        {}

    virtual bool execute() override {
        if (c.m_drum.index() == 0)
            m_band = band_of_address(c.m_address_register);
        if (m_band < 0)
            return false;

        // Can't look up in the last two words of a band.
        if (band_size - c.m_drum.index() <= 2 || less(c.m_drum.read(m_band), c.m_distributor))
        {
            ++c.m_address_register;
            return false;
        }
        return true;
    }

private:
    int m_band;
};

OPERATION_STEP(Address_to_Program_Register,
{
    // The timing chart in the manual includes "Add to PR", in TLU after the argument is found.
    // Addition doesn't make sense because the adder operates on the accumulator.  "Add" could
    // be mean "address", but the PR gets input from accumulator, distributor, or drum, not the
    // address register.  I don't know why the PR would need the address since the only thing
    // it can do with it is put it in the address register.
    return true;
})

OPERATION_STEP(Insert_Address_in_Lower,
{
    c.m_lower_accumulator.load(c.m_address_register, 0, 2);
    return true;
})
}

using Op_Sequence = std::vector<std::shared_ptr<Operation_Step>>;

Op_Sequence next_instruction_i_steps(Computer& computer, Operation op)
{
    return { std::make_shared<Instruction_to_Program_Register>(computer, op),
            std::make_shared<Op_and_Address_to_Registers>(computer, op) };
}

Op_Sequence next_instruction_d_steps(Computer& computer, Operation op)
{
    return { std::make_shared<Instruction_Address_to_Address_Register>(computer, op),
            std::make_shared<Enable_Program_Register>(computer, op) };
}

/// @return the steps for the passed-in operation.
Op_Sequence operation_steps(Computer& computer, Operation op)
{
    switch (op)
    {
    case Operation::no_operation:
    case Operation::stop:
    case Operation::branch_on_nonzero_in_upper:
    case Operation::branch_on_nonzero:
    case Operation::branch_on_minus:
    case Operation::branch_on_overflow:
        return {};
    case Operation::load_distributor:
        return { std::make_shared<Enable_Distributor>(computer, op),
                std::make_shared<Data_to_Distributor>(computer, op) };
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
        return { std::make_shared<Enable_Distributor>(computer, op),
                std::make_shared<Data_to_Distributor>(computer, op),
                std::make_shared<Distributor_to_Accumulator>(computer, op),
                std::make_shared<Remove_Interlock_A>(computer, op) };
    case Operation::store_distributor:
        return { std::make_shared<Enable_Position_Set>(computer, op),
                std::make_shared<Store_Distributor>(computer, op) };
    case Operation::store_lower_in_memory:
    case Operation::store_upper_in_memory:
        return { std::make_shared<Enable_Distributor>(computer, op),
                std::make_shared<Data_to_Distributor>(computer, op),
                std::make_shared<Store_Distributor>(computer, op) };
    case Operation::store_lower_data_address:
    case Operation::store_lower_instruction_address:
        return { std::make_shared<Data_to_Distributor>(computer, op),
                std::make_shared<Store_Distributor>(computer, op) };
    case Operation::multiply:
        return { std::make_shared<Enable_Distributor>(computer, op),
                std::make_shared<Data_to_Distributor>(computer, op),
                std::make_shared<Multiply>(computer, op),
                std::make_shared<Remove_Interlock_A>(computer, op) };
    case Operation::divide:
    case Operation::divide_and_reset_upper:
        return { std::make_shared<Enable_Distributor>(computer, op),
                std::make_shared<Data_to_Distributor>(computer, op),
                std::make_shared<Divide>(computer, op),
                std::make_shared<Remove_Interlock_A>(computer, op) };
    case Operation::shift_right:
    case Operation::shift_and_round:
    case Operation::shift_left:
    case Operation::shift_left_and_count:
        return { std::make_shared<Enable_Shift_Control>(computer, op),
                std::make_shared<Shift>(computer, op),
                std::make_shared<Remove_Interlock_A>(computer, op) };
    case Operation::table_lookup:
        return { std::make_shared<Enable_Position_Set>(computer, op),
                std::make_shared<Look_Up_Address>(computer, op),
                std::make_shared<Address_to_Program_Register>(computer, op),
                std::make_shared<Insert_Address_in_Lower>(computer, op) };
    default:
    {
        // Check for branch on 8 in distributor position.
        std::size_t pos = static_cast<int>(op)
            - static_cast<int>(Operation::branch_on_8_in_distributor_position_10);
        assert(0 <= pos && pos < word_size);
        return {};
    }
    }
}

/// The initial state is: powered off for long enough that the blower is off.
Computer::Computer()
    : m_elapsed_seconds(blower_off_delay_seconds),
      m_can_turn_on(true),
      m_power_on(false),
      m_dc_on(false),
      m_programmed_mode(Programmed_Mode::stop),
      m_control_mode(Control_Mode::run), //!TODO make persistent
      m_cycle_mode(Half_Cycle_Mode::run), //!TODO make persistent
      m_display_mode(Display_Mode::distributor),
      m_overflow_mode(Overflow_Mode::stop),
      m_error_mode(Error_Mode::stop),
      m_half_cycle(Half_Cycle::instruction),
      m_run_time(0),
      m_restart(false),
      m_overflow(false),
      m_storage_selection_error(false),
      m_clocking_error(false),
      m_error_sense(false),
      m_error_stop(false)
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
    // DC power can be turned on manually only after it's been turned on automatically and then
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

void Computer::step(TTime seconds)
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
    m_control_mode = mode;
}

void Computer::set_display_mode(Display_Mode mode)
{
    m_display_mode = mode;
}

void Computer::set_overflow_mode(Overflow_Mode mode)
{
    m_overflow_mode = mode;
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
            while (m_drum.index() != index_of_address(m_address_entry))
                m_drum.step();
            set_storage(m_address_entry, m_distributor);
            break;
        case Display_Mode::read_out_storage:
            while (m_drum.index() != index_of_address(m_address_entry))
                m_drum.step();
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
            Operation operation = Operation(m_operation_register.value());
            auto inst_seq = next_instruction_i_steps(*this, operation);
            for (auto next_op_it = inst_seq.begin();
                 next_op_it != inst_seq.end(); )
            {
                // Execute the operation.  Go on to the next operation if this one is done.
                if ((*next_op_it)->execute())
                    ++next_op_it;
                ++m_run_time;
                m_drum.step();
            }
            if (m_cycle_mode == Half_Cycle_Mode::half)
                return;
        }
        // m_half_cycle changes during execution.  The ifs are not exclusive.
        if (m_half_cycle == Half_Cycle::data)
        {
            Operation operation = Operation(m_operation_register.value());
            std::cerr << "D: op=" << static_cast<int>(operation) << std::endl;
            m_operation_register.clear();

            bool restarted = false;
            auto op_seq = operation_steps(*this, operation);
            auto op_end = op_seq.end();
            auto inst_seq = next_instruction_d_steps(*this, operation);
            auto next_op_it = inst_seq.begin();
            auto inst_end = inst_seq.end();
            // The operation sequence and the next address sequence may happen in parallel.
            // Loop until both are done.
            for (auto op_it = op_seq.begin(); op_it != op_end || next_op_it != inst_end; )
            {
                if (op_it != op_end)
                    if ((*op_it)->execute())
                        ++op_it;

                if ((m_restart || op_it == op_end) && next_op_it != inst_end)
                {
                    // It takes a cycle to process the "restart" signal and begin parallel
                    // execution.  So the first time through, we just set the "restarted"
                    // flag.
                    if (restarted || op_it == op_end)
                        if ((*next_op_it)->execute())
                            ++next_op_it;
                    restarted = true;
                }

                ++m_run_time;
                m_drum.step();
            }
            //! Don't stop on op=stop if m_programmed_mode is not "stop".
            if (m_cycle_mode == Half_Cycle_Mode::half
                || operation == Operation::stop
                || (m_overflow && m_overflow_mode == Overflow_Mode::stop)
                || m_error_stop)
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
    return !m_distributor.is_number();
}

bool Computer::accumulator_validity_error() const
{
    return !m_upper_accumulator.is_number() || !m_lower_accumulator.is_number();
}

bool Computer::program_register_validity_error() const
{
    return !m_program_register.is_number();
}

bool Computer::storage_selection_error() const
{
    if (m_address_register.is_blank())
        return false;
    auto address = m_address_register.value();
    return (address > 1999
            && (address < storage_entry_address.value()
                || address > upper_accumulator_address.value()))
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
    m_drum.write(band_of_address(address), word);
}

const Word Computer::get_storage(const Address& address) const
{
    assert(!address.is_blank());
    if (address == storage_entry_address)
        return m_storage_entry;
    else if (address == distributor_address)
        return m_distributor;
    else if (address == lower_accumulator_address)
        return m_lower_accumulator;
    else if (address == upper_accumulator_address)
        return m_upper_accumulator;
    return m_drum.read(band_of_address(address));
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
    Signed_Register<2*word_size> rhs(0, '+');
    rhs.load(reg, 0, word_size);
    accum = add(accum, shift(rhs, to_upper ? word_size : 0), carry);
    // Copy the upper and lower parts of the sums to the registers, preserving the upper sign.
    TDigit upper_sign = m_upper_accumulator[0];
    m_upper_accumulator.load(accum, 0, 0);
    m_upper_accumulator[0] = upper_sign;
    m_lower_accumulator.load(accum, word_size, 0);
}

void Computer::shift_accumulator(int n_places_left)
{
    Signed_Register<2*word_size> accum;
    accum.load(m_upper_accumulator, 0, 0);
    accum.load(m_lower_accumulator, 0, word_size);
    accum = shift(accum, n_places_left);
    TDigit sign = m_upper_accumulator[0];
    m_upper_accumulator.load(accum, 0, 0);
    m_upper_accumulator[0] = sign;
    m_lower_accumulator.load(accum, word_size, 0);
}

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

void Computer::set_error()
{
    m_overflow = true;
    m_storage_selection_error = true;
    m_clocking_error = true;
    m_error_sense = true;
}

void Computer::set_drum(const Address& address, const Word& word)
{
    m_drum.set_storage(band_of_address(address), index_of_address(address), word);
}

Word Computer::get_drum(const Address& address) const
{
    return m_drum.get_storage(band_of_address(address), index_of_address(address));
}

void Computer::Drum::step()
{
    m_index = (m_index + 1) % band_size;
}

Word Computer::Drum::read(std::size_t band) const
{
    assert(band < n_bands);
    return m_storage[band][m_index];
}

void Computer::Drum::write(std::size_t band, const Word& word)
{
    assert(band < n_bands);
    m_storage[band][m_index] = word;
}

std::size_t Computer::Drum::index() const
{
    return m_index;
}

void Computer::Drum::set_storage(std::size_t band, std::size_t index, const Word& word)
{
    m_storage[band][index] = word;
}

Word Computer::Drum::get_storage(std::size_t band, std::size_t index) const
{
    return m_storage[band][index];
}
