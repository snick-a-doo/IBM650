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
    BOOST_CHECK_THROW(f.drum.read(f.address), Address_Not_At_Read_Head);
    BOOST_CHECK_THROW(f.drum.write(f.address, f.data), Address_Not_At_Read_Head);
    f.drum.step();
    BOOST_CHECK(f.drum.is_at_read_head(f.address));
    BOOST_CHECK(f.drum.is_at_read_head(f.address_other_band));
    f.drum.write(f.address, f.data);
    for (int i = 0; i < 50; ++i)
        f.drum.step();
    BOOST_CHECK_EQUAL(f.drum.read(f.address), f.data);
}

BOOST_AUTO_TEST_CASE(read_buffer)
{
    Drum_Fixture f;
    BOOST_CHECK_THROW(f.drum.set_read_buffer(1, f.data), Buffer_Not_At_Read_Head);
    f.drum.step();
    f.drum.set_read_buffer(1, f.data);
    f.drum.store_read_buffer(1, Address({0,6,6,3})); // anywhere in the band
    for (int i = 0; i < 25; ++i)
        f.drum.step();
    BOOST_CHECK_THROW(f.drum.set_read_buffer(26, f.data), Buffer_Index_Out_Of_Bounds);
    for (int i = 0; i < 25; ++i)
        f.drum.step();
    BOOST_CHECK_EQUAL(f.drum.read(Address({0,6,5,1})), f.data);
}

BOOST_AUTO_TEST_CASE(punch_buffer)
{
    Drum_Fixture f;
    BOOST_CHECK_THROW(f.drum.load_punch_buffer(4, Address({1,1,1,1})), Buffer_Not_At_Read_Head);
    for (int i = 0; i < 30; ++i)
        f.drum.step();
    f.drum.write(Address({1,1,3,0}), f.data);
    f.drum.load_punch_buffer(4, Address({1,1,1,1})); // anywhere in the band
    for (int i = 0; i < 10; ++i)
        f.drum.step();
    BOOST_CHECK_THROW(f.drum.get_punch_buffer(4), Buffer_Not_At_Read_Head);
    BOOST_CHECK_THROW(f.drum.get_punch_buffer(14), Buffer_Index_Out_Of_Bounds);
    for (int i = 0; i < 40; ++i)
        f.drum.step();
    BOOST_CHECK_EQUAL(f.drum.get_punch_buffer(4), f.data);
}
