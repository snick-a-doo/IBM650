#include "drum.hpp"

using namespace IBM650;

Address_Error::Address_Error(const std::string& message, int address) noexcept
    : m_message(message),
      m_address(address)
{}

const char* Address_Error::what() const noexcept
{
    std::ostringstream os;
    os << m_message << m_address << std::endl;
    return os.str().c_str();
}


Drum::Drum(bool large) noexcept
    : m_bands(static_cast<int>(large ? N_Bands::large : N_Bands::small)),
      m_index(0),
      m_memory(m_bands + 1) // Allocate an extra band for read and punch buffers.
{}

void Drum::step() noexcept
{
    m_index = ++m_index % words_per_band;
}

int Drum::get_index(const Address& address) const noexcept
{
    return (address.value()) % words_per_band;
}

int Drum::get_band(const Address& address) const
{
    int band = address.value()/words_per_band;
    if (band >= m_bands)
        throw Address_Out_Of_Bounds(address);
    return band;
}

bool Drum::is_at_read_head(const Address& address) const noexcept
{
    return m_index == get_index(address);
}

Word Drum::read(const Address& address) const
{
    if (!is_at_read_head(address))
        throw Address_Not_At_Read_Head(address);
    return m_memory[m_index][get_band(address)];
}

void Drum::write(const Address& address, const Word& data)
{
    if (!is_at_read_head(address))
        throw Address_Not_At_Read_Head(address);
    m_memory[m_index][get_band(address)] = data;
}

namespace
{
    const int read_buffer_offset = 0;
    const int punch_buffer_offset = 26;

    void check_buffer_index(int m_index, int index, int offset)
    {
        if (index + offset != m_index)
            throw Buffer_Not_At_Read_Head(index);
        if (index < 1 || index > 10)
            throw Buffer_Index_Out_Of_Bounds(index);
    }
}

void Drum::set_read_buffer(int index, const Word& data)
{
    check_buffer_index(m_index, index, read_buffer_offset);
    m_memory[m_index][m_bands] = data;
}

void Drum::store_read_buffer(int index, const Address& address)
{
    check_buffer_index(m_index, index, read_buffer_offset);
    m_memory[m_index][get_band(address)] = m_memory[m_index][m_bands];
}

void Drum::load_punch_buffer(int index, const Address& address)
{
    check_buffer_index(m_index, index, punch_buffer_offset);
    m_memory[m_index][m_bands] = m_memory[m_index][get_band(address)];
}

Word Drum::get_punch_buffer(int index) const
{
    check_buffer_index(m_index, index, punch_buffer_offset);
    return m_memory[m_index][m_bands];
}

