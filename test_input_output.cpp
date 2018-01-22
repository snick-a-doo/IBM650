#include <boost/test/unit_test.hpp>

#include "buffer.hpp"
#include "input_output_unit.hpp"
#include <iostream>
using namespace IBM533;

namespace test_input_output
{
// A card has 11 rows and 80 columns.  The rows are numbered 12, 11, 0, 1, 2, ..., 9.  Cards
// are read 12-row first.  The 80 columns represent 8 10-digit words.  Most significant digit
// to least.  Digits are single bits: Bit 0 = 0, bit 1 = 1, ...  Bit 11 (0x800) in the units
// digit indicates a positive word; bit 10 (0x400) indicates negative.  Positive is assumed if
// neither bit is set.
const Card card1 {0x002, 0x004, 0x002, 0x008, 0x002, 0x010, 0x002, 0x020, 0x002, 0x840,
        0x004, 0x004, 0x004, 0x008, 0x004, 0x010, 0x004, 0x020, 0x004, 0x440,
        0x008, 0x004, 0x008, 0x008, 0x008, 0x010, 0x008, 0x020, 0x008, 0x840,
        0x010, 0x004, 0x010, 0x008, 0x010, 0x010, 0x010, 0x020, 0x010, 0x440,
        0x020, 0x004, 0x020, 0x008, 0x020, 0x010, 0x020, 0x020, 0x020, 0x840,
        0x040, 0x004, 0x040, 0x008, 0x040, 0x010, 0x040, 0x020, 0x040, 0x440,
        0x080, 0x004, 0x080, 0x008, 0x080, 0x010, 0x080, 0x020, 0x080, 0x840,
        0x100, 0x004, 0x100, 0x008, 0x100, 0x010, 0x100, 0x020, 0x100, 0x440};

const Card card2{0x002, 0x004, 0x002, 0x010, 0x002, 0x040, 0x002, 0x100, 0x002, 0x801,
        0x004, 0x004, 0x004, 0x010, 0x004, 0x040, 0x004, 0x100, 0x004, 0x401,
        0x008, 0x004, 0x008, 0x010, 0x008, 0x040, 0x008, 0x100, 0x008, 0x801,
        0x010, 0x004, 0x010, 0x010, 0x010, 0x040, 0x010, 0x100, 0x010, 0x401,
        0x020, 0x004, 0x020, 0x010, 0x020, 0x040, 0x020, 0x100, 0x020, 0x801,
        0x040, 0x004, 0x040, 0x010, 0x040, 0x040, 0x040, 0x100, 0x040, 0x401,
        0x080, 0x004, 0x080, 0x010, 0x080, 0x040, 0x080, 0x100, 0x080, 0x801,
        0x100, 0x004, 0x100, 0x010, 0x100, 0x040, 0x100, 0x100, 0x100, 0x401};

const Card card3 {0x001, 0x008, 0x001, 0x008, 0x001, 0x008, 0x001, 0x008, 0x001, 0x802,
        0x001, 0x008, 0x001, 0x008, 0x001, 0x008, 0x001, 0x008, 0x001, 0x802,
        0x001, 0x008, 0x001, 0x008, 0x001, 0x008, 0x001, 0x008, 0x001, 0x802,
        0x001, 0x008, 0x001, 0x008, 0x001, 0x008, 0x001, 0x008, 0x001, 0x802,
        0x001, 0x008, 0x001, 0x008, 0x001, 0x008, 0x001, 0x008, 0x001, 0x802,
        0x001, 0x008, 0x001, 0x008, 0x001, 0x008, 0x001, 0x008, 0x001, 0x802,
        0x001, 0x008, 0x001, 0x008, 0x001, 0x008, 0x001, 0x008, 0x001, 0x802,
        0x001, 0x008, 0x001, 0x008, 0x001, 0x008, 0x001, 0x008, 0x001, 0x802};

const Card card4{0x010, 0x001, 0x010, 0x001, 0x010, 0x001, 0x010, 0x001, 0x010, 0x401,
        0x010, 0x001, 0x010, 0x001, 0x010, 0x001, 0x010, 0x001, 0x010, 0x401,
        0x010, 0x001, 0x010, 0x001, 0x010, 0x001, 0x010, 0x001, 0x010, 0x401,
        0x010, 0x001, 0x010, 0x001, 0x010, 0x001, 0x010, 0x001, 0x010, 0x401,
        0x010, 0x001, 0x010, 0x001, 0x010, 0x001, 0x010, 0x001, 0x010, 0x401,
        0x010, 0x001, 0x010, 0x001, 0x010, 0x001, 0x010, 0x001, 0x010, 0x401,
        0x010, 0x001, 0x010, 0x001, 0x010, 0x001, 0x010, 0x001, 0x010, 0x401,
        0x010, 0x001, 0x010, 0x001, 0x010, 0x001, 0x010, 0x001, 0x010, 0x401};

bool check_buffer(Buffer& buffer, const Card& card)
{
    if (buffer.size() != 8)
        return false;

    auto decimal = [](int hex) {
        int dec = 0;
        for (; dec < 10 && (hex & 1) == 0; ++dec, hex = hex >> 1)
            ;
         assert(dec < 10);
        return dec;
    };
    auto sign = [](int hex) { return hex & 0x400 ? '-' : '+'; };

    using namespace IBM650;
    auto card_it1 = card.begin();
    auto card_it2 = card.begin() + word_size;
    std::array<TDigit, word_size+1> digits;
    for (std::size_t i = 0; i < 8; ++i, buffer.pop_front())
    {
        std::transform(card_it1, card_it2, digits.begin(), decimal);
        digits[word_size] = sign(*(card_it2 - 1));
        if (Word(digits) != buffer.front())
        {
            buffer.clear();
            return false;
        }
        card_it1 = card_it2;
        card_it2 += word_size;
    }
    return true;
}

BOOST_AUTO_TEST_CASE(initial_state)
{
    Input_Output_Unit unit;
    BOOST_CHECK(unit.is_on());
    BOOST_CHECK(unit.read_is_idle());
    BOOST_CHECK(unit.punch_is_idle());
    BOOST_CHECK(!unit.double_punch_and_blank());
    BOOST_CHECK(!unit.double_punch_and_blank());

    BOOST_CHECK(unit.read_hopper_deck().empty());
    BOOST_CHECK(unit.read_stacker_deck().empty());
    BOOST_CHECK(unit.punch_hopper_deck().empty());
    BOOST_CHECK(unit.punch_stacker_deck().empty());
}

BOOST_AUTO_TEST_CASE(read_start_0_cards)
{
    Input_Output_Unit unit;
    unit.read_start();
    BOOST_CHECK(unit.read_hopper_deck().empty());
    BOOST_CHECK(unit.read_stacker_deck().empty());
    BOOST_CHECK_EQUAL(unit.get_source().size(), 0);
}

BOOST_AUTO_TEST_CASE(read_start_1_card)
{
    std::shared_ptr<Buffer> buffer;
    Input_Output_Unit unit;
    Card_Deck deck {card1};
    unit.load_read_hopper(deck);
    BOOST_CHECK_EQUAL(unit.read_hopper_deck().size(), 1);
    BOOST_CHECK(unit.read_stacker_deck().empty());
    BOOST_CHECK_EQUAL(unit.get_source().size(), 0);

    unit.read_start();
    // Card at 1st station.
    BOOST_CHECK(unit.read_hopper_deck().empty());
    BOOST_CHECK(unit.read_stacker_deck().empty());
    BOOST_CHECK_EQUAL(unit.get_source().size(), 0);

    unit.read_start();
    // Card passes 1st read brushes.
    BOOST_CHECK(unit.read_hopper_deck().empty());
    BOOST_CHECK(unit.read_stacker_deck().empty());
    BOOST_CHECK_EQUAL(unit.get_source().size(), 0);

    unit.read_start();
    // Card passes 2nd read brushes.
    BOOST_CHECK(unit.read_hopper_deck().empty());
    BOOST_CHECK(unit.read_stacker_deck().empty());
    BOOST_CHECK_EQUAL(unit.get_source().size(), 8);
    BOOST_CHECK(check_buffer(unit.get_source(), card1));
    BOOST_CHECK_EQUAL(unit.get_source().size(), 0);

    unit.read_start();
    // Card is stacked.
    BOOST_CHECK(unit.read_hopper_deck().empty());
    BOOST_CHECK_EQUAL(unit.read_stacker_deck().size(), 1);
    BOOST_CHECK_EQUAL(unit.get_source().size(), 0);

    BOOST_CHECK(unit.read_stacker_deck() == deck);
}

BOOST_AUTO_TEST_CASE(read_start_2_cards)
{
    Input_Output_Unit unit;
    Card_Deck deck {card1, card2};
    unit.load_read_hopper(deck);
    BOOST_CHECK_EQUAL(unit.read_hopper_deck().size(), 2);
    BOOST_CHECK(unit.read_stacker_deck().empty());
    BOOST_CHECK_EQUAL(unit.get_source().size(), 0);

    // Cards at 1 and 2
    unit.read_start();
    BOOST_CHECK(unit.read_hopper_deck().empty());
    BOOST_CHECK(unit.read_stacker_deck().empty());
    BOOST_CHECK_EQUAL(unit.get_source().size(), 0);

    // Cards at 2 and 3
    unit.read_start();
    BOOST_CHECK(unit.read_hopper_deck().empty());
    BOOST_CHECK(unit.read_stacker_deck().empty());
    BOOST_CHECK_EQUAL(unit.get_source().size(), 8);
    BOOST_CHECK(check_buffer(unit.get_source(), card1));
    BOOST_CHECK_EQUAL(unit.get_source().size(), 0);

    // Card at 3
    unit.read_start();
    BOOST_CHECK(unit.read_hopper_deck().empty());
    BOOST_CHECK_EQUAL(unit.read_stacker_deck().size(), 1);
    BOOST_CHECK_EQUAL(unit.get_source().size(), 8);
    BOOST_CHECK(check_buffer(unit.get_source(), card2));
    BOOST_CHECK_EQUAL(unit.get_source().size(), 0);

    unit.read_start();
    BOOST_CHECK(unit.read_hopper_deck().empty());
    BOOST_CHECK_EQUAL(unit.read_stacker_deck().size(), 2);
    BOOST_CHECK_EQUAL(unit.get_source().size(), 0);

    BOOST_CHECK(unit.read_stacker_deck() == deck);
}

BOOST_AUTO_TEST_CASE(read_start_3_cards)
{
    Input_Output_Unit unit;
    Card_Deck deck {card1, card2, card3};
    unit.load_read_hopper(deck);
    BOOST_CHECK_EQUAL(unit.read_hopper_deck().size(), 3);
    BOOST_CHECK(unit.read_stacker_deck().empty());
    BOOST_CHECK_EQUAL(unit.get_source().size(), 0);

    // Cards at 1, 2 and 3
    unit.read_start();
    BOOST_CHECK(unit.read_hopper_deck().empty());
    BOOST_CHECK(unit.read_stacker_deck().empty());
    BOOST_CHECK_EQUAL(unit.get_source().size(), 8);
    // Don't empty the buffer this time.

    // Cards at 2 and 3
    unit.read_start();
    BOOST_CHECK(unit.read_hopper_deck().empty());
    BOOST_CHECK_EQUAL(unit.read_stacker_deck().size(), 1);
    BOOST_CHECK_EQUAL(unit.get_source().size(), 8);
    BOOST_CHECK(check_buffer(unit.get_source(), card2));
    BOOST_CHECK_EQUAL(unit.get_source().size(), 0);

    // Card at 3
    unit.read_start();
    BOOST_CHECK(unit.read_hopper_deck().empty());
    BOOST_CHECK_EQUAL(unit.read_stacker_deck().size(), 2);
    BOOST_CHECK_EQUAL(unit.get_source().size(), 8);
    BOOST_CHECK(check_buffer(unit.get_source(), card3));
    BOOST_CHECK_EQUAL(unit.get_source().size(), 0);

    unit.read_start();
    BOOST_CHECK(unit.read_hopper_deck().empty());
    BOOST_CHECK_EQUAL(unit.read_stacker_deck().size(), 3);
    BOOST_CHECK_EQUAL(unit.get_source().size(), 0);

    BOOST_CHECK(unit.read_stacker_deck() == deck);
}

BOOST_AUTO_TEST_CASE(read_start_200_cards)
{
    Input_Output_Unit unit;
    Card_Deck deck(200, card1);
    unit.load_read_hopper(deck);
    BOOST_CHECK_EQUAL(unit.read_hopper_deck().size(), 200);
    BOOST_CHECK(unit.read_stacker_deck().empty());
    BOOST_CHECK_EQUAL(unit.get_source().size(), 0);

    // Cards at 1, 2 and 3
    unit.read_start();
    BOOST_CHECK_EQUAL(unit.read_hopper_deck().size(), 197);
    BOOST_CHECK(unit.read_stacker_deck().empty());
    BOOST_CHECK_EQUAL(unit.get_source().size(), 8);
    BOOST_CHECK(check_buffer(unit.get_source(), card1));
    BOOST_CHECK_EQUAL(unit.get_source().size(), 0);

    // Start does nothing with cards in the hopper.
    unit.read_start();
    BOOST_CHECK_EQUAL(unit.read_hopper_deck().size(), 197);
    BOOST_CHECK(unit.read_stacker_deck().empty());
    BOOST_CHECK_EQUAL(unit.get_source().size(), 0);
}

struct Mock_Source_Client : Source_Client
{
    // Source_Client interface
    virtual void connect_source(std::weak_ptr<Source> src) override { source = src; }
    virtual void resume() override { running = true; }

    // Test methods
    void read() {
        // Execute read instruction -- no op
        // Transfer buffer -- no op
        running = false;
        if (auto src = source.lock())
            src->advance();
    }
    void fill_buffer(Buffer& source_buffer) {
        buffer.clear();
        while (!source_buffer.empty()) {
            buffer.push_back(source_buffer.front());
            source_buffer.pop_front();
        }
    }
    std::weak_ptr<Source> source;
    Buffer buffer;
    bool running = false;
};

struct Card_Read_Fixture
{
    Card_Read_Fixture()
        : unit(std::make_shared<Input_Output_Unit>()),
          client(std::make_shared<Mock_Source_Client>())
        {
            unit->connect_source_client(client);
            client->connect_source(unit);
            unit->load_read_hopper(deck);
        }
    std::shared_ptr<Input_Output_Unit> unit;
    std::shared_ptr<Mock_Source_Client> client;
    Card_Deck deck {card1, card2, card3, card4};
};

BOOST_AUTO_TEST_CASE(run_in)
{
    // See use case 1.
    Card_Read_Fixture f;

    BOOST_CHECK_EQUAL(f.unit->read_hopper_deck().size(), 4);
    BOOST_CHECK(f.unit->read_stacker_deck().empty());
    f.unit->read_start();
    // 3 cards in unit
    BOOST_CHECK_EQUAL(f.unit->read_hopper_deck().size(), 1);
    BOOST_CHECK(f.unit->read_stacker_deck().empty());
    // 1st card read into buffer
    f.client->fill_buffer(f.unit->get_source());
    BOOST_CHECK(check_buffer(f.client->buffer, card1));
}

BOOST_AUTO_TEST_CASE(read_instruction)
{
    Card_Read_Fixture f;
    f.unit->read_start();
    f.client->fill_buffer(f.unit->get_source());
    
    // Computer executes a read instruction, transfers the buffer, signals advance.
    f.client->read();
    // Cards advance.
    BOOST_CHECK(f.unit->read_hopper_deck().empty());
    BOOST_CHECK_EQUAL(f.unit->read_stacker_deck().size(), 1);
    // Client gets resume signal.
    BOOST_CHECK(f.client->running);
    // 2nd card read into buffer
    f.client->fill_buffer(f.unit->get_source());
    BOOST_CHECK(check_buffer(f.client->buffer, card2));
}

BOOST_AUTO_TEST_CASE(reload_read_hopper)
{ 
    Card_Read_Fixture f;
    f.unit->read_start();
    f.client->read();
    BOOST_CHECK(f.unit->read_hopper_deck().empty());
    BOOST_CHECK_EQUAL(f.unit->read_stacker_deck().size(), 1);
    f.client->read();
    // Hopper empty, cards don't advance, resume signal not given.
    f.client->fill_buffer(f.unit->get_source());
    BOOST_CHECK(check_buffer(f.client->buffer, card2));
    BOOST_CHECK(!f.client->running);

    f.unit->load_read_hopper(f.deck);
    f.unit->read_start();
    // Cards advance.
    f.client->fill_buffer(f.unit->get_source());
    BOOST_CHECK(check_buffer(f.client->buffer, card3));
    f.client->read();
    BOOST_CHECK(f.client->running);
    f.client->fill_buffer(f.unit->get_source());
    BOOST_CHECK(check_buffer(f.client->buffer, card4));
}

BOOST_AUTO_TEST_CASE(end_of_file)
{
    Card_Read_Fixture f;
    f.unit->read_start();
    f.client->read();
    f.client->read();
    // Hopper empty, cards don't advance, resume signal not given.
    f.client->fill_buffer(f.unit->get_source());
    BOOST_CHECK(check_buffer(f.client->buffer, card2));
    BOOST_CHECK(!f.client->running);

    f.unit->end_of_file();
    f.client->fill_buffer(f.unit->get_source());
    BOOST_CHECK(check_buffer(f.client->buffer, card3));
    f.client->read();
    f.client->fill_buffer(f.unit->get_source());
    BOOST_CHECK(check_buffer(f.client->buffer, card4));
}
}
