#include <boost/test/unit_test.hpp>

#include "drum.hpp"

using namespace IBM650;

struct Drum_Fixture
{
    Drum_Fixture()
        : drum(true),
          address({0,0,0,1}),
          address_other_band({1,0,5,1}),
          data({1,2, 3,4,5,6, 7,8,9,0})
        {}
    Drum drum;
    Address address;
    Address address_other_band;
    Word data;
};

BOOST_AUTO_TEST_CASE(read_drum)
{
    Drum_Fixture f;
    BOOST_CHECK(!f.drum.is_at_read_head(f.address));
    BOOST_CHECK_THROW(f.drum.read(f.address), Address_Not_at_Read_Head);
    BOOST_CHECK_THROW(f.drum.write(f.address, f.data), Address_Not_at_Read_Head);
    f.drum.step();
    BOOST_CHECK(f.drum.is_at_read_head(f.address));
    BOOST_CHECK(f.drum.is_at_read_head(f.address_other_band));
    f.drum.write(f.address, f.data);
    for(int i = 0; i < 50; ++i)
        f.drum.step();
    BOOST_CHECK_EQUAL(f.drum.read(f.address), f.data);
}

// BOOST_AUTO_TEST_CASE(read_buffer)
// {
//     Drum_Fixture f;
//     Card card {10, 202, 3030, 40404, 505050, 60606, 7070, 808, 90, 10, 11, 12}; 
//     f.drum.set_read_buffer(card);
//     f.drum.transfer_read_buffer(Address({0,6,6,3}));
//     f.drum.step();
//     Address address({0,6,5,1});
//     for (auto number : card)
//     {
//         BOOST_CHECK_EQUAL(f.drum.read(address).value(), number);
//         ++address;
//         f.drum.step();
//     }
// }
