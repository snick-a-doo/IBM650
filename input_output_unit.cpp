#include "input_output_unit.hpp"

#include <algorithm>
#include <iostream>

using namespace IBM533;
using namespace IBM650;

using Card_Ptr_Deck = std::deque<std::shared_ptr<Card>>;

const std::size_t read_feed_size = 3;
const std::size_t punch_feed_size = 2;

Buffer IBM533::card_to_buffer(const Card& card)
{
    auto digit = [](std::size_t n) {
        TDigit i;
        for (i = 0; !(n & 1) && i < 10; ++i, n = n >> 1)
            ;
        return i;
    };

    Buffer buffer(buffer_size);
    for (std::size_t i = 0; i < card.size()/word_size; ++i)
    {
        std::array<TDigit, word_size+1> digits;
        std::size_t n = i*word_size;
        for (std::size_t j = 0; j < word_size; ++j, ++n)
            digits[j] = digit(card[n]);
        digits[word_size] = (card[n-1] & 0x400) ? '-' : '+';
        buffer[i] = Word(digits);
    }
    buffer[8] = zero;
    buffer[9] = zero;
    return buffer;
}

/// @Return a card punched with the first 8 words of the buffer.
Card buffer_to_card(const Buffer& buffer)
{
    Card card;
    for (std::size_t i = 0; i < card.size()/word_size; ++i)
    {
        assert(i < buffer.size());
        std::size_t n = i*word_size;
        for (std::size_t j = 0; j < word_size; ++j, ++n)
        {
            assert(n < card_columns);
            card[n] = 1 << dec(buffer[i].digits()[j]);
        }
        card[n - 1] |= 1 << (buffer[i].sign() == '+' ? 11 : 10);
    }
    return card;
}

void advance(Card_Deck& hopper, Card_Ptr_Deck& fed, Card_Deck& stacker)
{
    // Feed a card from the stack into the device.
    fed.push_back(hopper.empty() ? nullptr : std::make_shared<Card>(hopper.front()));

    // If a card was pushed past the last station, move it to the stack.
    if (fed.front())
        stacker.push_back(*(fed.front()));
    fed.pop_front();

    // Remove the card from the hopper.
    if (!hopper.empty())
        hopper.pop_front();
}

Input_Output_Unit::Input_Output_Unit()
    : m_fed_read_cards(read_feed_size),
      m_fed_punch_cards(punch_feed_size)
{
}

bool Input_Output_Unit::is_read_idle() const
{
    return !m_read_running || m_read_hopper_deck.empty();
}

bool Input_Output_Unit::is_punch_idle() const
{
    return !m_punch_running;
}

bool Input_Output_Unit::is_end_of_file() const
{
    return m_end_of_file;
}

void Input_Output_Unit::load_read_hopper(const Card_Deck& deck)
{
    m_read_hopper_deck = deck;
}

void Input_Output_Unit::load_punch_hopper(const Card_Deck& deck)
{
    m_punch_hopper_deck = deck;
}

void Input_Output_Unit::read_start()
{
    m_read_running = true;

    std::size_t n_cards = m_pending_read_advance ? 1
        : !m_read_hopper_deck.empty()
        && std::all_of(m_fed_read_cards.begin(), m_fed_read_cards.end(),
                       [](auto p) { return p; })
        ? 0
        : std::min(std::max(m_read_hopper_deck.size(), static_cast<std::size_t>(1)),
                   static_cast<std::size_t>(read_feed_size));

    // Run in 0 to 3 cards.
    for (std::size_t i = 0; i < n_cards; ++i)
        advance_read_cards();

    assert(m_fed_read_cards.size() == read_feed_size);

    if (auto client = m_source_client.lock())
        client->resume_source_client();
}

void Input_Output_Unit::punch_start()
{
    if (m_punch_hopper_deck.empty())
    {
        // Run out one card.
        advance(m_punch_hopper_deck, m_fed_punch_cards, m_punch_stacker_deck);
        return;
    }

    while ((!m_fed_punch_cards.front() && !m_punch_hopper_deck.empty())
           || m_pending_punch_advance)
    {
        if (m_pending_punch_advance)
            punch();
        advance(m_punch_hopper_deck, m_fed_punch_cards, m_punch_stacker_deck);
        m_pending_punch_advance = false;
    }
    m_punch_running = !m_punch_hopper_deck.empty();
    if (auto client = m_sink_client.lock())
        if (m_punch_running)
            client->resume_sink_client();
}

void Input_Output_Unit::read_stop()
{
    m_read_running = false;
    m_punch_running = false;
}

void Input_Output_Unit::punch_stop()
{
    m_read_running = false;
    m_punch_running = false;
}

void Input_Output_Unit::end_of_file()
{
    m_end_of_file = true;
    advance_source();
}

const Card_Deck& Input_Output_Unit::read_hopper_deck() const
{
    return m_read_hopper_deck;
}

const Card_Deck& Input_Output_Unit::read_stacker_deck() const
{
    return m_read_stacker_deck;
}

const Card_Deck& Input_Output_Unit::punch_hopper_deck() const
{
    return m_punch_hopper_deck;
}

const Card_Deck& Input_Output_Unit::punch_stacker_deck() const
{
    return m_punch_stacker_deck;
}

void Input_Output_Unit::connect_source_client(std::weak_ptr<Source_Client> client)
{
    m_source_client = client;
}

void Input_Output_Unit::advance_read_cards()
{
    advance(m_read_hopper_deck, m_fed_read_cards, m_read_stacker_deck);
        
    // If a card was pushed into the 3rd station, read it into the buffer.
    if (m_fed_read_cards.front())
        m_source_buffer = card_to_buffer(*m_fed_read_cards.front());

    m_pending_read_advance = false;
}

void Input_Output_Unit::advance_source()
{
    // No more cards inside.
    if (std::all_of(m_fed_read_cards.begin(), m_fed_read_cards.end(), [](auto p) { return !p; }))
        m_end_of_file = false;
    
    m_pending_read_advance = true;
    if (!m_read_running || (m_read_hopper_deck.empty() && !m_end_of_file))
        return;

    advance_read_cards();
    if (auto client = m_source_client.lock())
        client->resume_source_client();
}

Buffer& Input_Output_Unit::get_source()
{
    return m_source_buffer;
}

void Input_Output_Unit::connect_sink_client(std::weak_ptr<Sink_Client> client)
{
    m_sink_client = client;
}

void Input_Output_Unit::advance_sink()
{
    m_pending_punch_advance = !m_punch_running;
    if (!m_punch_running)
        return;

    punch();
    advance(m_punch_hopper_deck, m_fed_punch_cards, m_punch_stacker_deck);
    m_punch_running = !m_punch_hopper_deck.empty();
    if (auto client = m_sink_client.lock())
        if (m_punch_running)
            client->resume_sink_client();
}

void Input_Output_Unit::punch()
{
    *m_fed_punch_cards.front() = buffer_to_card(m_sink_buffer);
    m_sink_buffer.clear();
}

Buffer& Input_Output_Unit::get_sink()
{
    return m_sink_buffer;
}
