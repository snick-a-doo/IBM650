#include "register.hpp"

using namespace IBM650;

Opcode Program_Register::opcode() const
{
    return get_range(0, Opcode());
}

Address Program_Register::data_address() const
{
    return get_range(2, Address());
}

Address Program_Register::instruction_address() const
{
    return get_range(6, Address());
}
