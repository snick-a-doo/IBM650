#include <boost/test/unit_test.hpp>

#include "buffer.hpp"
#include "input_output_unit.hpp"
#include "register.hpp"

#include <iostream>
using namespace IBM533;
using namespace IBM650;

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

const std::array<Card, 4> test_cards {card1, card2, card3, card4};

BOOST_AUTO_TEST_CASE(initial_state)
{
    Input_Output_Unit unit;
    BOOST_CHECK(unit.is_on());
    BOOST_CHECK(unit.is_read_idle());
    BOOST_CHECK(unit.is_punch_idle());
    BOOST_CHECK(!unit.is_read_feed_stopped());
    BOOST_CHECK(!unit.is_end_of_file());
    BOOST_CHECK(!unit.is_double_punch_or_blank());

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
    BOOST_CHECK(unit.is_read_idle());
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
    BOOST_CHECK(unit.is_read_idle());

    unit.read_start();
    // Card passes 1st read brushes.
    BOOST_CHECK(unit.read_hopper_deck().empty());
    BOOST_CHECK(unit.read_stacker_deck().empty());
    BOOST_CHECK_EQUAL(unit.get_source().size(), 0);

    unit.read_start();
    // Card passes 2nd read brushes.
    BOOST_CHECK(unit.read_hopper_deck().empty());
    BOOST_CHECK(unit.read_stacker_deck().empty());
    BOOST_CHECK_EQUAL(unit.get_source().size(), buffer_size);
    BOOST_CHECK(card_to_buffer(card1) == unit.get_source());

    unit.read_start();
    // Card is stacked.
    BOOST_CHECK(unit.read_hopper_deck().empty());
    BOOST_CHECK_EQUAL(unit.read_stacker_deck().size(), 1);

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
    BOOST_CHECK_EQUAL(unit.get_source().size(), buffer_size);
    BOOST_CHECK(card_to_buffer(card1) == unit.get_source());

    // Card at 3
    unit.read_start();
    BOOST_CHECK(unit.read_hopper_deck().empty());
    BOOST_CHECK_EQUAL(unit.read_stacker_deck().size(), 1);
    BOOST_CHECK_EQUAL(unit.get_source().size(), buffer_size);
    BOOST_CHECK(card_to_buffer(card2) == unit.get_source());

    unit.read_start();
    BOOST_CHECK(unit.read_hopper_deck().empty());
    BOOST_CHECK_EQUAL(unit.read_stacker_deck().size(), 2);

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
    BOOST_CHECK_EQUAL(unit.get_source().size(), buffer_size);
    // Don't empty the buffer this time.

    // Cards at 2 and 3
    unit.read_start();
    BOOST_CHECK(unit.read_hopper_deck().empty());
    BOOST_CHECK_EQUAL(unit.read_stacker_deck().size(), 1);
    BOOST_CHECK_EQUAL(unit.get_source().size(), buffer_size);
    BOOST_CHECK(card_to_buffer(card2) == unit.get_source());

    // Card at 3
    unit.read_start();
    BOOST_CHECK(unit.read_hopper_deck().empty());
    BOOST_CHECK_EQUAL(unit.read_stacker_deck().size(), 2);
    BOOST_CHECK_EQUAL(unit.get_source().size(), buffer_size);
    BOOST_CHECK(card_to_buffer(card3) == unit.get_source());

    unit.read_start();
    BOOST_CHECK(unit.read_hopper_deck().empty());
    BOOST_CHECK_EQUAL(unit.read_stacker_deck().size(), 3);

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
    BOOST_CHECK_EQUAL(unit.get_source().size(), buffer_size);
    BOOST_CHECK(card_to_buffer(card1) == unit.get_source());

    // Start does nothing with cards in the hopper.
    unit.read_start();
    BOOST_CHECK_EQUAL(unit.read_hopper_deck().size(), 197);
    BOOST_CHECK(unit.read_stacker_deck().empty());
}

struct Mock_Source_Client : Source_Client
{
    // Source_Client interface
    virtual void connect_source(std::weak_ptr<Source> src) override { source = src; }
    virtual void resume_source_client() override { running = true; }

    // Test methods
    void read() {
        // Execute read instruction -- no op
        // Transfer buffer -- no op
        running = false;
        if (auto src = source.lock())
            src->advance_source();
    }
    void fill_buffer() {
        if (auto src = source.lock())
        {
            buffer.clear();
            Buffer& source_buffer = src->get_source();
            for ( ; !source_buffer.empty(); source_buffer.pop_front())
                buffer.push_back(source_buffer.front());
        }
    }
    std::weak_ptr<Source> source;
    Buffer buffer;
    bool running = false;
};

struct Card_Read_Fixture
{
    Card_Read_Fixture(std::size_t cards = 4)
        : unit(std::make_shared<Input_Output_Unit>()),
          client(std::make_shared<Mock_Source_Client>())
        {
            for (std::size_t i = 0; i < cards; ++i)
                deck.push_back(test_cards[i % test_cards.size()]);
            unit->connect_source_client(client);
            client->connect_source(unit);
            unit->load_read_hopper(deck);
        }
    std::shared_ptr<Input_Output_Unit> unit;
    std::shared_ptr<Mock_Source_Client> client;
    Card_Deck deck;
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
    f.client->fill_buffer();
    BOOST_CHECK(card_to_buffer(card1) == f.client->buffer);
    BOOST_CHECK(!f.unit->is_read_idle());
}

BOOST_AUTO_TEST_CASE(read_instruction)
{
    Card_Read_Fixture f;
    f.unit->read_start();
    f.client->fill_buffer();

    // Computer executes a read instruction, transfers the buffer, signals advance.
    f.client->read();
    // Cards advance.
    BOOST_CHECK(f.unit->read_hopper_deck().empty());
    BOOST_CHECK_EQUAL(f.unit->read_stacker_deck().size(), 1);
    // Client gets resume signal.
    BOOST_CHECK(f.client->running);
    // 2nd card read into buffer
    f.client->fill_buffer();
    BOOST_CHECK(card_to_buffer(card2) == f.client->buffer);
    // Stacker deck is empty.
    BOOST_CHECK(f.unit->is_read_idle());
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
    f.client->fill_buffer();
    BOOST_CHECK(card_to_buffer(card2) == f.client->buffer);
    BOOST_CHECK(!f.client->running);
    BOOST_CHECK(f.unit->is_read_idle());
    BOOST_CHECK(!f.unit->is_end_of_file());

    f.unit->load_read_hopper(f.deck);
    f.unit->read_start();
    BOOST_CHECK(!f.unit->is_read_idle());
    // Cards advance.
    f.client->fill_buffer();
    BOOST_CHECK(card_to_buffer(card3) == f.client->buffer);
    f.client->read();
    BOOST_CHECK(f.client->running);
    f.client->fill_buffer();
    BOOST_CHECK(card_to_buffer(card4) == f.client->buffer);
}

BOOST_AUTO_TEST_CASE(end_of_file)
{
    Card_Read_Fixture f;
    f.unit->read_start();
    f.client->read();
    f.client->read();
    // Hopper empty, cards don't advance, resume signal not given.
    f.client->fill_buffer();
    BOOST_CHECK(card_to_buffer(card2) == f.client->buffer);
    BOOST_CHECK(!f.client->running);
    BOOST_CHECK(f.unit->is_read_idle());

    f.unit->end_of_file();
    f.client->fill_buffer();
    BOOST_CHECK(card_to_buffer(card3) == f.client->buffer);
    BOOST_CHECK(f.unit->is_end_of_file());
    f.client->read();
    f.client->fill_buffer();
    BOOST_CHECK(card_to_buffer(card4) == f.client->buffer);
}

BOOST_AUTO_TEST_CASE(read_stop)
{
    Card_Read_Fixture f(8);
    f.unit->read_start();
    f.client->read();
    BOOST_CHECK_EQUAL(f.unit->read_hopper_deck().size(), 4);
    BOOST_CHECK_EQUAL(f.unit->read_stacker_deck().size(), 1);
    // Either key stops reading and punching.
    f.unit->read_stop();
    BOOST_CHECK(f.unit->is_read_idle());
    BOOST_CHECK(!f.unit->is_end_of_file());
    f.client->read();
    // Stopped, cards don't advance, resume signal not given.
    f.client->fill_buffer();
    BOOST_CHECK(card_to_buffer(card2) == f.client->buffer);
    BOOST_CHECK(!f.client->running);

    f.unit->read_start();
    BOOST_CHECK(!f.unit->is_read_idle());
    BOOST_CHECK(!f.unit->is_end_of_file());
    // Cards advance.
    BOOST_CHECK(f.client->running);
    f.client->fill_buffer();
    BOOST_CHECK(card_to_buffer(card3) == f.client->buffer);
    f.client->read();
    f.client->fill_buffer();
    BOOST_CHECK(card_to_buffer(card4) == f.client->buffer);
}

BOOST_AUTO_TEST_CASE(punch_stop)
{
    Card_Read_Fixture f(8);
    f.unit->read_start();
    f.client->read();
    BOOST_CHECK_EQUAL(f.unit->read_hopper_deck().size(), 4);
    BOOST_CHECK_EQUAL(f.unit->read_stacker_deck().size(), 1);
    // Either key stops reading and punching.
    f.unit->punch_stop();
    f.client->read();
    BOOST_CHECK(!f.client->running);

    f.unit->read_start();
    BOOST_CHECK(!f.unit->is_read_idle());
    BOOST_CHECK(!f.unit->is_end_of_file());
    BOOST_CHECK(f.client->running);
    f.client->fill_buffer();
    BOOST_CHECK(card_to_buffer(card3) == f.client->buffer);
    f.client->read();
    f.client->fill_buffer();
    BOOST_CHECK(card_to_buffer(card4) == f.client->buffer);
}

struct Mock_Sink_Client : Sink_Client
{
    // Sink_Client interface
    virtual void connect_sink(std::weak_ptr<Sink> snk) override { sink = snk; }
    virtual void resume_sink_client() override { running = true; }

    // Test methods
    void write(Buffer buffer) {
        if (auto snk = sink.lock())
        {
            // Transfer to buffer -- no op
            // Write buffer
            for ( ; !buffer.empty(); buffer.pop_front())
                snk->get_sink().push_back(buffer.front());
            running = false;
            snk->advance_sink();
        }
    }
    std::weak_ptr<Sink> sink;
    bool running = false;
};

struct Card_Punch_Fixture
{
    Card_Punch_Fixture(std::size_t cards = 4)
        : unit(std::make_shared<Input_Output_Unit>()),
          client(std::make_shared<Mock_Sink_Client>())
        {
            unit->connect_sink_client(client);
            client->connect_sink(unit);
            unit->load_punch_hopper(Card_Deck(cards));
        }
    std::shared_ptr<Input_Output_Unit> unit;
    std::shared_ptr<Mock_Sink_Client> client;
};

BOOST_AUTO_TEST_CASE(punch_run_in_0_cards)
{
    Card_Punch_Fixture f(0);
    BOOST_CHECK(f.unit->is_punch_idle());
    BOOST_CHECK_EQUAL(f.unit->punch_hopper_deck().size(), 0);
    BOOST_CHECK_EQUAL(f.unit->punch_stacker_deck().size(), 0);
    f.unit->punch_start();
    BOOST_CHECK(f.unit->is_punch_idle());
    BOOST_CHECK_EQUAL(f.unit->punch_hopper_deck().size(), 0);
    BOOST_CHECK_EQUAL(f.unit->punch_stacker_deck().size(), 0);
}

BOOST_AUTO_TEST_CASE(punch_run_in_1_card)
{
    Card_Punch_Fixture f(1);
    BOOST_CHECK(f.unit->is_punch_idle());
    BOOST_CHECK_EQUAL(f.unit->punch_hopper_deck().size(), 1);
    BOOST_CHECK_EQUAL(f.unit->punch_stacker_deck().size(), 0);
    f.unit->punch_start();
    BOOST_CHECK(f.unit->is_punch_idle());
    BOOST_CHECK_EQUAL(f.unit->punch_hopper_deck().size(), 0);
    BOOST_CHECK_EQUAL(f.unit->punch_stacker_deck().size(), 0);
}

BOOST_AUTO_TEST_CASE(punch_run_in)
{
    Card_Punch_Fixture f;
    BOOST_CHECK(f.unit->is_punch_idle());
    BOOST_CHECK_EQUAL(f.unit->punch_hopper_deck().size(), 4);
    BOOST_CHECK_EQUAL(f.unit->punch_stacker_deck().size(), 0);
    f.unit->punch_start();
    BOOST_CHECK(!f.unit->is_punch_idle());
    BOOST_CHECK_EQUAL(f.unit->punch_hopper_deck().size(), 2);
    BOOST_CHECK_EQUAL(f.unit->punch_stacker_deck().size(), 0);
}

BOOST_AUTO_TEST_CASE(punch_instruction)
{
    Card_Punch_Fixture f;
    f.unit->punch_start();
    f.client->write(card_to_buffer(card1));
    BOOST_CHECK(!f.unit->is_punch_idle());
    BOOST_CHECK_EQUAL(f.unit->punch_hopper_deck().size(), 1);
    BOOST_CHECK_EQUAL(f.unit->punch_stacker_deck().size(), 1);
    BOOST_CHECK(f.unit->punch_stacker_deck().front() == card1);
}

}
