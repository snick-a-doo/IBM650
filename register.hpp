#ifndef REGISTER_HPP
#define REGISTER_HPP

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <limits>
#include <numeric>
#include <ostream>
#include <sstream>

namespace IBM650
{
    /// Type for the signed numeric value of a register.  Must be large enough to hold the
    /// widest register for which Register::value() is called.
    using word_t = long;
    /// Type for a register digit.  Must be able to hold digits 0-9 plus a value for "blank".
    using digit_t = char;

    /// A set of contiguous digits in memory.
    template<std::size_t N>
    using Digits = std::array<digit_t, N>;

    /// The number of base-10 digits in memory location.
    constexpr std::size_t digits_per_word = 10;

    /// A register with N digits and a sign.
    template<std::size_t N>
    class Register
    {
    public:
        /// Make a register initialized with zeros.
        Register() noexcept;
        /// Make a register initialized with an array of digits.
        Register(const Digits<N>& digits) noexcept;
        Register(const Digits<N>&& digits) noexcept;
        /// Make a register initialized with an array of digits and a sign.
        Register(const Digits<N>& digits, bool positive) noexcept;
        Register(const Digits<N>&& digits, bool positive) noexcept;
        /// Fill the register with zeros and set the sign to positive.
        void reset() noexcept;
        /// Fill the register with blanks - all bits zeroed.  The 650 stores digits with a
        /// 7-bit bi-quinary code.  Most bit patters, including all zeros, don't correspond to
        /// a digit.  Blanks or other non-digit patterns in a register interpreted as an
        /// opcode, address, or data will cause a validation failure.  Blanks may be useful for
        /// an initial state or to indicate no output on the console.
        void blank() noexcept;

        /// @return true if all positions hold blanks.
        bool is_blank() const noexcept;
        /// @return true if all positions hold a digit 0-9.  A call to value will throw if
        /// false.
        bool has_value() const noexcept;
        /// @return the numeric value of the register, from -(10^10 - 1) to (10^10 -1).  Throws
        /// Register_Value_Error if the register contains any non-digits, i.e. has_value()
        /// returns false.  Returns positive if there's no sign.
        word_t value() const;

        /// True if rhs has the same number of digits, the same value for each digit and the
        /// same sign (or lack of sign).
        bool operator==(const Register<N>& rhs) const noexcept;
        /// Write the digits and sign to a stream.
        std::ostream& output(std::ostream& os) const noexcept;

    protected:
        enum class Sign
        {
            /// No memory location for a sign.  Treated as positive for value() but == is
            /// always false when comparing to a signed register.  Nothing is output in the
            /// sign position.
            no_sign = 0,
            positive = 8, ///< The input/output value for positive sign.
            negative = 9, ///< The input/output value for negative sign.
        };

        /// @return a new, unsigned register made up of a range of digits from this register.
        template<std::size_t M>
        Register<M> get_range(std::size_t start, Register<M>&& out) const;

    private:
        // It seems a constexpr member can't be used in a template class.
        enum : digit_t
        {
            /// A number other than 0-9 to represent no bits set.  The 650's bi-quinary code
            /// has exactly 2 bits set for 0-9.
            blank_digit = -1
        };

        Digits<N> m_digits; 
        Sign m_sign;
    };

    /// Register for the opcode copied from the program register.
    using Opcode = Register<2>;
    /// Register for the addresses copied from the program register.
    using Address = Register<4>;
    /// A word of memory for input, output, and storage.
    using Word = Register<digits_per_word>;
    /// The accumulator.  Holds two words' worth of digits but one sign.
    using Accumulator = Register<2*digits_per_word>;

    /// Register for an instruction read from storage.  Holds a word's worth of digits but no
    /// sign.
    class Program_Register : public Word
    {
    public:
        Program_Register() noexcept
            : Word() {}
        Program_Register(const Digits<digits_per_word>& digits) noexcept
            : Word(digits) {}
        Program_Register(const Digits<digits_per_word>&& digits) noexcept
            : Word(std::move(digits)) {}
        /// @return the opcode portion of the instruction.
        Opcode opcode() const;
        /// @return the data address portion of the instruction.
        Address data_address() const;
        /// @return the portion of the instruction that gives the address of the next
        /// instruction.
        Address instruction_address() const;
    };

    
    /// Exception thrown when value() is called on a register that contains non-digits.
    template<std::size_t N>
    class Register_Value_Error : public std::exception
    {
    public:
        Register_Value_Error(const Register<N>& reg)
            : m_register(reg)
            {}
        virtual const char* what() const noexcept override
            {
                std::ostringstream os;
                os << "Register does not have a numeric value: " << m_register << std::endl;
                return os.str().c_str();
            }
    private:
        Register<N> m_register;
    };


    /// Exception thrown when range() is called for a range of digits that exceeds the bounds
    /// of the register.
    class Register_Range_Error : public std::exception
    {
    public:
        Register_Range_Error(std::size_t reg_size, std::size_t start, std::size_t range)
            : m_reg_size(reg_size),
              m_start(start),
              m_range(range)
            {}
        virtual const char* what() const noexcept override
            {
                std::ostringstream os;
                os << "Range [" << m_start << "-" << m_start + m_range << ") "
                   << "exceeds register size: " << m_reg_size << std::endl;
                return os.str().c_str();
            }
    private:
        std::size_t m_reg_size;
        std::size_t m_start;
        std::size_t m_range;
    };


    // Register implementation

    template<std::size_t N>
    Register<N>::Register() noexcept
        : Register<N>({0})
    {}

    template<std::size_t N>
    Register<N>::Register(const Digits<N>& digits) noexcept
        : m_digits(digits),
          m_sign(Sign::no_sign)
    {}

    template<std::size_t N>
    Register<N>::Register(const Digits<N>&& digits) noexcept
        : m_digits(std::move(digits)),
          m_sign(Sign::no_sign)
    {}

    template<std::size_t N>
    Register<N>::Register(const Digits<N>& digits, bool positive) noexcept
        : m_digits(digits),
          m_sign(positive ? Sign::positive : Sign::negative)
    {}

    template<std::size_t N>
    Register<N>::Register(const Digits<N>&& digits, bool positive) noexcept
        : m_digits(std::move(digits)),
          m_sign(positive ? Sign::positive : Sign::negative)
    {}

    template<std::size_t N>
    void Register<N>::reset() noexcept
    {
        std::fill(m_digits.begin(), m_digits.end(), 0);
        m_sign = Sign::positive;
    }

    template<std::size_t N>
    void Register<N>::blank() noexcept
    {
        std::fill(m_digits.begin(), m_digits.end(), blank_digit);
        m_sign = Sign::positive;
    }

    template<std::size_t N>
    bool Register<N>::is_blank() const noexcept
    {
        return std::all_of(m_digits.begin(), m_digits.end(),
                           [](auto digit){ return digit == blank_digit; });
    }

    template<std::size_t N>
    bool Register<N>::has_value() const noexcept
    {
        return std::all_of(m_digits.begin(), m_digits.end(),
                           [](auto digit){ return digit >= 0 && digit < 10; });
    }

    template<std::size_t N>
    word_t Register<N>::value() const
    {
        // Ensure that the return type is wide enough to hold an N-digit number.
        static_assert(std::numeric_limits<word_t>::max() >= std::pow(10, N) - 1);
        static_assert(std::numeric_limits<word_t>::min() <= -(std::pow(10, N) - 1));
        return std::accumulate(m_digits.begin(), m_digits.end(), 0L,
                               [this](word_t a, word_t b){
                                   if (b < 0 || b > 9)
                                       throw Register_Value_Error<N>(*this);
                                   return 10*a + b;
                               })
            * (m_sign == Sign::negative ? -1.0 : 1.0);
    }

    template<std::size_t N>
    bool Register<N>::operator==(const Register<N>& rhs) const noexcept
    {
        return m_digits == rhs.m_digits && m_sign == rhs.m_sign;
    }

    template<std::size_t N>
    std::ostream& Register<N>::output(std::ostream& os) const noexcept
    {
        for (auto digit : m_digits)
            os << static_cast<int>(digit);

        switch (m_sign)
        {
        case Sign::positive:
            return os << '+';
        case Sign::negative:
            return os << '-';
        default:
            assert(m_sign == Sign::no_sign);
            return os;
        }
    }

    template<std::size_t N>
    template<std::size_t M>
    Register<M> Register<N>::get_range(std::size_t start, Register<M>&& out) const
    {
        static_assert(M <= N);
        if (start + M > N)
            throw Register_Range_Error(N, start, M);
        Digits<M> range;
        std::copy(m_digits.begin() + start, m_digits.begin() + start + M, range.begin());
        // Ranges are unsigned.
        return Register<M>(range);
    }
}

namespace std
{
    /// Output stream operator.  Needs to be in std for unit test messages.
    template <size_t N>
    ostream& operator<<(ostream& os, const IBM650::Register<N>& reg)
    {
        return reg.output(os);
    }
}

#endif
