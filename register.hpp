#ifndef REGISTER_HPP
#define REGISTER_HPP

#include <array>
#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <limits>

namespace IBM650
{
using TDigit = char;

constexpr TDigit base = 10;
/// An array of ASCII characters that have the bi-quinary bit patterns for 0, 1, ..., 9.
/// Bits 0-4 indicate the numbers 0-4 if bit 5 is set, 5-9 if bit 6 is set.  Exactly two
/// bits should be set: one of bits 0-4 and either 5 or 6.  Any other bit pattern does not
/// represent a number.  E.g. '(' is ASCII 0x28 = 0010 1000 or 01 01000 which represents
/// 3.  'B' is ASCII 0x42 = 0100 0010 or 10 00010 which represents 6.
constexpr std::array<TDigit, base> bi_quinary_code {'!','\"','$','(','0','A','B','D','H','P'};

/// @Return the bi-quinary code for a given integer.  E.g. bin(3) returns 'B'.  If number
/// is '_' return 0 (no bits).  Since signs are encoded as digits, return 8 for '-', 9
/// for '+'.
TDigit bin(TDigit number);
/// @Return the integer for a given bi-quinary code.  dec() is the inverse of bin() for
/// integer arguments in [0, base), and '_'.  I.e. dec(0) returns '_'.  If the argument of
/// dec() is not a code or 0, '?' is returned.
TDigit dec(TDigit code);

/// The type for the numeric value of a register.  Must be large enough to avoid overflow
/// in all cases of interest.
//! make value() free in computer.cpp.  Make digits() protected? friend<<
using TValue = std::size_t;

/// An array of bi-quinary codes in display order: MSB first.
template <std::size_t N> class Register
{
public:
    /// Make a register initialized with all bits unset.
    Register();
    virtual ~Register() = default;
    /// Make a register initialized with the codes for the digits in passed-in integer
    /// array.  The character '_' may be passed to indicate a blank (all bits unset).
    Register(const std::array<TDigit, N>& digits);

    /// Set the digits of this register from another.  Digits are copied from in starting at
    /// position in_offset into this register starting at reg_offset.  Copying stops when
    /// either register runs out of digits.
    template<std::size_t M>
    Register<N>& load(const Register<M>& in, size_t in_offset, size_t reg_offset);
    /// Set all digits to the passed-in integer.
    void fill(TDigit digit);
    /// Unset all bits.
    void clear();

    /// @Return true if all bits are unset in all digits.
    bool is_blank() const;
    /// @Return true if the register represents an integer.
    bool is_number() const;

    /// @Return the integer value of the decimal number represented by the digits in the
    /// register.
    TValue value() const;

    /// Give access to the binary register contents.
    const std::array<TDigit, N>& digits() const;
    std::array<TDigit, N>& digits();

    /// @Return true if reg has all the same digits.
    bool operator==(const Register<N>& reg) const;
    /// @Return true if any digits of reg differ.
    bool operator!=(const Register<N>& reg) const;

    /// @Return the code for the nth most significant digit.
    virtual TDigit& operator[](std::size_t n);
    virtual const TDigit& operator[](std::size_t n) const;

    Register<N>& operator++();

private:
    /// The contents of the register as bi-quinary codes.
    std::array<TDigit, N> m_digits;
};

template <std::size_t N>
Register<N>::Register()
    : m_digits({0})
{}

template <std::size_t N>
Register<N>::Register(const std::array<TDigit, N>& digits)
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
    auto length = std::min(M - in_offset, N - reg_offset);
    std::copy(in.digits().begin() + in_offset,
              in.digits().begin() + in_offset + length,
              m_digits.begin() + reg_offset);
    return *this;
}

template <std::size_t N>
void Register<N>::fill(TDigit digit)
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
bool Register<N>::is_number() const
{
    // Bi-quinary codes have exactly one of bits 0-4 set, and one of 5-6.  No other bit
    // patterns represent a decimal digit.
    auto one_bit_set = [](TDigit digit, int start, int bits) {
        int sum = 0;
        for (auto i = start; i < start + bits; ++i)
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
    TValue total = 0;
    for (auto digit : m_digits)
        total = base*total + dec(digit);
    return total;
}

template <std::size_t N>
const std::array<TDigit, N>& Register<N>::digits() const
{
    return m_digits;
}

template <std::size_t N>
std::array<TDigit, N>& Register<N>::digits()
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

template <std::size_t N>
TDigit& Register<N>::operator[](std::size_t n)
{
    assert(0 <= n && n < N);
    return m_digits[N-n-1];
}

template <std::size_t N>
const TDigit& Register<N>::operator[](std::size_t n) const
{
    assert(0 <= n && n < N);
    return m_digits[N-n-1];
}

template <std::size_t N>
Register<N>& Register<N>::operator++()
{
    TDigit carry = 1;
    for (auto it = m_digits.rbegin(); it != m_digits.rend() && carry == 1; ++it)
    {
        TDigit n = dec(*it) + 1;
        *it = bin(n % base);
        carry = n / base;
    }
    return *this;
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
    Signed_Register(const std::array<TDigit, N+1>& digits);
    /// Make a register from the digits of an unsigned register and the passed-in sign.
    Signed_Register(const Register<N>& reg, TDigit sign);
    Signed_Register(TDigit fill_digit, TDigit sign);

    /// Set the digits to the passed-in integer.  Set the sign to the passed-in sign.
    void fill(TDigit digit, TDigit sign);

    /// @Return the sign as a character: +, -, _, or ?.
    TDigit sign() const;
    virtual TDigit& operator[](std::size_t n) override;
    virtual const TDigit& operator[](std::size_t n) const override;
};

template <std::size_t N>
Signed_Register<N>::Signed_Register()
{
}

template <std::size_t N>
Signed_Register<N>::Signed_Register(const std::array<TDigit, N+1>& digits)
    : Register<N+1>(digits)
{
}

template <std::size_t N>
Signed_Register<N>::Signed_Register(const Register<N>& reg, TDigit sign)
{
    Register<N+1>::load(reg, 0, 0);
    Register<N+1>::digits().back() = bin(sign);
}

template <std::size_t N>
Signed_Register<N>::Signed_Register(TDigit fill_digit, TDigit sign)
{
    fill(fill_digit, sign);
}

template <std::size_t N>
void Signed_Register<N>::fill(TDigit digit, TDigit sign)
{
    Register<N+1>::fill(digit);
    Register<N+1>::digits()[N] = bin(sign);
}

template <std::size_t N>
TDigit Signed_Register<N>::sign() const
{
    auto n = dec(Register<N+1>::digits().back());
    return n == 9 ? '+'
        : n == 8 ? '-'
        : n == 0 ? '_'
        : '?';
}

template <std::size_t N>
TDigit& Signed_Register<N>::operator[](std::size_t n)
{
    return Register<N+1>::digits()[N-n];
}

template <std::size_t N>
const TDigit& Signed_Register<N>::operator[](std::size_t n) const
{
    assert(0 <= n && n <= N);
    return Register<N+1>::digits()[N-n];
}

template <std::size_t N>
Signed_Register<N> shift(const Signed_Register<N>& reg, std::size_t left)
{
    Signed_Register<N> out(reg);
    for (std::size_t i = 0; i < N; ++i)
    {
        std::size_t j = i + left;
        out.digits()[i] = 0 <= j && j < N ? reg.digits()[j] : bin(0);
    }
    return out;
}

template <std::size_t N>
Signed_Register<N> abs(const Signed_Register<N>& reg)
{
    Signed_Register<N> out(reg);
    out[0] = bin('+');
    return out;
}

template <std::size_t N>
Signed_Register<N> change_sign(const Signed_Register<N>& reg)
{
    Signed_Register<N> out(reg);
    out[0] = out.sign() == '+' ? bin('-') : bin('+');
    return out;
}

template <std::size_t N>
Signed_Register<N> add(const Signed_Register<N>& lhs,
                       const Signed_Register<N>& rhs,
                       TDigit& carry)
{
    std::array<TDigit, N+1> sum;
    carry = 0;

    auto sub = [&carry, &sum](const auto& subl, const auto& subr)
    {
        carry = 0;
        for (std::size_t i = 1; i <= N; ++i)
        {
            auto l = dec(subl[i]);
            auto r = dec(subr[i]) + carry;
            carry = 0;
            sum[N-i] = l - r;
            if (l < r)
            {
                sum[N-i] += base;
                carry = 1;
            }
        }
    };

    if (lhs.sign() == rhs.sign())
    {
        for (std::size_t i = 1; i <= N; ++i)
        {
            auto digit = carry + dec(lhs[i]) + dec(rhs[i]);
            sum[N-i] = digit % base;
            carry = digit/base;
        }
        sum[N] = lhs.sign();
    }
    else
    {
        const Signed_Register<N>& left = rhs.sign() == '-' ? lhs : rhs;
        const Signed_Register<N>& right = rhs.sign() == '-' ? rhs : lhs;
        sub(left, abs(right));
        sum[N] = left.sign();
        if (carry)
        {
            sub(abs(right), left);
            sum[N] = right.sign();
        }
    }
    return Signed_Register<N>(sum);
}

template <std::size_t N>
bool less(const Signed_Register<N>& lhs, const Signed_Register<N>& rhs)
{
    auto itl = lhs.digits().begin() + 1;
    auto itr = rhs.digits().begin() + 1;
    for ( ; itl != lhs.digits().end() && itr != rhs.digits().end(); ++itl, ++itr)
        if (*itl != *itr)
            return *itl < *itr;
    return false;
}

constexpr std::size_t word_size = 10;
using UWord = Register<word_size>;
using Word = Signed_Register<word_size>;
const Word zero({0,0, 0,0,0,0, 0,0,0,0, '+'});
}

namespace std
{
template<std::size_t N>
std::ostream& operator<<(std::ostream& os, const IBM650::Register<N>& reg)
{
    for (std::size_t i = 0; i < N; ++i)
    {
        auto d = IBM650::dec(reg.digits()[i]);
        os << static_cast<char>(d < IBM650::base ? d + '0' : d);
    }
    return os;
}
}

#endif
