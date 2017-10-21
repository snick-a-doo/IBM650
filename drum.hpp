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
        Address_Error(const std::string& message, int  address) noexcept;
        virtual const char* what() const noexcept override;

    private:
        std::string m_message;
        int m_address;
    };


    class Address_Out_Of_Bounds : public Address_Error
    {
    public:
        Address_Out_Of_Bounds(const Address& address) noexcept
            : Address_Error("Address out of bounds: ", address.value())
            {}
    };


    class Address_Not_At_Read_Head : public Address_Error
    {
    public:
        Address_Not_At_Read_Head(const Address& address) noexcept
            : Address_Error("Address not at read head: ", address.value())
            {}
    };

    class Buffer_Not_At_Read_Head : public Address_Error
    {
    public:
        Buffer_Not_At_Read_Head(int index) noexcept
            : Address_Error("Buffer not at read head: ", index)
            {}
    };

    class Buffer_Index_Out_Of_Bounds : public Address_Error
    {
    public:
        Buffer_Index_Out_Of_Bounds(int index) noexcept
            : Address_Error("Buffer index out of bounds: ", index)
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

        void set_read_buffer(int index, const Word& data);
        void store_read_buffer(int index, const Address& address);

        void load_punch_buffer(int index, const Address& address);
        Word get_punch_buffer(int index) const;

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
