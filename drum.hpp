#ifndef DRUM_HPP
#define DRUM_HPP

#include <array>
#include <vector>

#include "register.hpp"

namespace IBM650
{
    class Address_Error : public std::exception
    {
    public:
        Address_Error(const std::string& message, const Address& address) noexcept;
        virtual const char* what() const noexcept override;

    private:
        std::string m_message;
        word_t m_address;
    };


    class Address_out_of_Bounds : public Address_Error
    {
    public:
        Address_out_of_Bounds(const Address& address) noexcept
            : Address_Error("Address out of bounds: ", address)
            {}
    };


    class Address_Not_at_Read_Head : public Address_Error
    {
    public:
        Address_Not_at_Read_Head(const Address& address) noexcept
            : Address_Error("Address not at read head: ", address)
            {}
    };

    class Drum
    {
    public:
        Drum(bool large) noexcept;
        void step() noexcept;
        bool is_at_read_head(const Address& address) const noexcept;
        Word read(const Address& address) const;
        void write(const Address& address, const Word& data);

    private:
        enum class N_Bands
        {
            small = 20,
            large = 40,
        };
        static constexpr int words_per_band = 50;

        int get_index(const Address& address) const noexcept;
        int get_band(const Address& address) const;

        const int m_bands;
        int m_index;
        std::vector<std::array<Word, words_per_band>> m_memory;
    };
}

#endif
