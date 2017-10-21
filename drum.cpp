#include "drum.hpp"

using namespace IBM650;

Address_Error::Address_Error(const std::string& message, const Address& address) noexcept
    : m_message(message),
      m_address(address.value())
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
      m_memory(m_bands)
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
        throw Address_out_of_Bounds(address);
    return band;
}

bool Drum::is_at_read_head(const Address& address) const noexcept
{
    return m_index == get_index(address);
}

Word Drum::read(const Address& address) const
{
    if (!is_at_read_head(address))
        throw Address_Not_at_Read_Head(address);
    return m_memory[get_index(address)][get_band(address)];
}

void Drum::write(const Address& address, const Word& data)
{
    if (!is_at_read_head(address))
        throw Address_Not_at_Read_Head(address);
    m_memory[get_index(address)][get_band(address)] = data;
}
