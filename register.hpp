#ifndef REGISTER_HPP
#define REGISTER_HPP

#include <array>
#include <algorithm>
#include <cassert>
#include <iostream>
#include <limits>

namespace IBM650
{
    /// An array of ASCII characters that have the bi-quinary bit patterns for 0, 1, ..., 9.
    constexpr std::array<char, 10> bi_quinary_code {'!','\"','$','(','0','A','B','D','H','P'};

    /// @Return the bi-quinary code for a given integer.  If number is '_' return 0 (no bits).
    /// Since, signs are encoded as digits, return 8 for '-', 9 for '+'.
    char bin(char number);
    /// @Return the integer for a given bi-quinary code.
    char dec(char code);

    /// The type for the numeric value of a register.  Must be large enough to avoid overflow
    /// in all cases of interest.
    using TValue = int;

    template <std::size_t N> class Register
    {
    public:
        /// Make a register initialized with all bits unset.
        Register();
        /// Make a register initialized with the codes for the digits in passed-in integer
        /// array.  The character '_' may be passed to indicate a blank (all bits unset).
        Register(const std::array<char, N>& digits);

        /// Set the digits of this register from another.  Digits are copied from in starting at
        /// position in_offset into this register starting at reg_offset.  Copying stops when
        /// either register runs out of digits.
        template<std::size_t M>
        Register<N>& load(const Register<M>& in, size_t in_offset, size_t reg_offset);
        /// Set all digits to the passed-in integer.
        void fill(char digit);
        /// Unset all bits.
        void clear();

        /// @Return true if all bits are unset in all digits.
        bool is_blank() const;
        /// @Return true if the register represents an integer.
        bool is_valid() const;

        /// @Return the integer value of the decimal number represented by the digits in the
        /// register.
        TValue value() const;

        /// Give access to the binary register contents.
        const std::array<char, N>& digits() const;
        std::array<char, N>& digits();

        /// @Return true if reg has all the same digits.
        bool operator==(const Register<N>& reg) const;
        /// @Return true if any digits of reg differ.
        bool operator!=(const Register<N>& reg) const;

    private:
        /// The contents of the register as bi-quinary codes.
        std::array<char, N> m_digits;
    };

    template <std::size_t N>
    Register<N>::Register()
        : m_digits({0})
    {}

    template <std::size_t N>
    Register<N>::Register(const std::array<char, N>& digits)
    {
        for (std::size_t i = 0; i < N; ++i)
            m_digits[i] = bin(digits[i]);
    }

    template<std::size_t N>
    template<std::size_t M>
    Register<N>& Register<N>::load(const Register<M>& in, size_t in_offset, size_t reg_offset)
    {
        assert(in_offset < M);
        assert(reg_offset < N);
        size_t length = std::min(M - in_offset, N - reg_offset);
        std::copy(in.digits().begin() + in_offset,
                  in.digits().begin() + in_offset + length,
                  m_digits.begin() + reg_offset);
        return *this;
    }

    template <std::size_t N>
    void Register<N>::fill(char digit)
    {
        for (std::size_t i = 0; i < N; ++i)
            m_digits[i] = bin(digit);
    }

    template <std::size_t N>
    void Register<N>::clear()
    {
        for (std::size_t i = 0; i < N; ++i)
            m_digits[i] = '\0';
    }

    template <std::size_t N>
    bool Register<N>::is_blank() const
    {
        return std::all_of(m_digits.cbegin(), m_digits.cend(),
                           [](auto digit) { return digit == '\0'; });
    }

    template <std::size_t N>
    bool Register<N>::is_valid() const
    {
        // Bi-quinary codes have exactly one of bits 0-4 set, and one of 5-6.  No other bit
        // patterns represent a decimal digit.
        auto one_bit_set = [](char digit, int start, int bits) {
            int sum = 0;
            for (int i = start; i < start + bits; ++i)
                sum += digit >> i & 1;
            return sum == 1;
        };
        return std::all_of(m_digits.cbegin(), m_digits.cend(),
                           [&](auto digit) {
                               return one_bit_set(digit, 0, 5) && one_bit_set(digit, 5, 2); });
    }

    template <std::size_t N>
    TValue Register<N>::value() const
    {
        // Fail if the return type does not have enough bits hold the register's maximum
        // value.
        static_assert(N < std::numeric_limits<TValue>::digits10);
        int total = 0;
        for (auto digit : m_digits)
            total = 10*total + dec(digit);
        return total;
    }

    template <std::size_t N>
    const std::array<char, N>& Register<N>::digits() const
    {
        return m_digits;
    }

    template <std::size_t N>
    std::array<char, N>& Register<N>::digits()
    {
        return m_digits;
    }

    template <std::size_t N>
    bool Register<N>::operator==(const Register<N>& reg) const
    {
        return std::equal(m_digits.begin(), m_digits.end(), reg.m_digits.begin());
    }

    template <std::size_t N>
    bool Register<N>::operator!=(const Register<N>& reg) const
    {
        return !(m_digits == reg.m_digits);
    }

    /// A register with an extra digit to represent the sign.
    template <std::size_t N>
    class Signed_Register : public Register<N+1>
    {
    public:
        /// Make a signed register initialized with all bits unset, including the sign.
        Signed_Register();
        /// Make a register initialized with the codes for the digits in passed-in integer
        /// array.  The character '_' may be passed to indicate a blank (all bits unset).  The
        /// last digit is the sign: 8 for -, 9 for +.  The characters '-' and '+' may be used.
        Signed_Register(const std::array<char, N+1>& digits);
        /// Make a register from the digits of an unsigned register and the passed-in sign.
        Signed_Register(const Register<N>& reg, char sign);

        /// Set the digits to the passed-in integer.  Set the sign to the passed-in sign.
        void fill(char digit, char sign);
    };

    template <std::size_t N>
    Signed_Register<N>::Signed_Register()
    {
    }

    template <std::size_t N>
    Signed_Register<N>::Signed_Register(const std::array<char, N+1>& digits)
        : Register<N+1>(digits)
    {
    }

    template <std::size_t N>
    Signed_Register<N>::Signed_Register(const Register<N>& reg, char sign)
    {
        Register<N+1>::load(reg, 0, 0);
        Register<N+1>::digits().back() = bin(sign);
    }

    template <std::size_t N>
    void Signed_Register<N>::fill(char digit, char sign)
    {
        Register<N+1>::fill(digit);
        Register<N+1>::digits()[N] = bin(sign);
    }
}

namespace std
{
    template<std::size_t N>
    std::ostream& operator<<(std::ostream& os, const IBM650::Register<N>& reg)
    {
        for (auto digit : reg.digits())
            os << digit;
        os << std::endl;
    }
}

#endif
