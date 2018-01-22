#include "input_output_unit.hpp"

#include <algorithm>
#include <iostream>

using namespace IBM533;
using namespace IBM650;

Word card_word(const Card& card, std::size_t n)
{
    auto decimal = [](int hex) {
        int dec = 0;
        for (; dec < 10 && (hex & 1) == 0; ++dec, hex = hex >> 1)
            ;
        assert(dec < 10);
        return dec;
    };
    auto sign = [](int hex) { return hex & 0x400 ? '-' : '+'; };

    using namespace IBM650;
    auto card_it1 = card.begin() + n*word_size;
    auto card_it2 = card_it1 + word_size;

    std::array<TDigit, word_size+1> digits;
    std::transform(card_it1, card_it2, digits.begin(), decimal);
    digits[word_size] = sign(*(card_it2 - 1));
    return Word(digits);
}

Input_Output_Unit::Input_Output_Unit()
    : m_pending_advance(false),
      m_end_of_file(false)
{
    while (m_fed_cards.size() < 3)
        m_fed_cards.push_back(nullptr);
}

void Input_Output_Unit::load_read_hopper(const Card_Deck& deck)
{
    m_read_hopper_deck = deck;
}

void Input_Output_Unit::read_start()
{
    std::size_t n_cards = m_pending_advance ? 1
        : !m_read_hopper_deck.empty()
        && std::all_of(m_fed_cards.begin(), m_fed_cards.end(), [](auto p) { return p; }) ? 0
        : std::min(std::max(m_read_hopper_deck.size(), static_cast<std::size_t>(1)),
                   static_cast<std::size_t>(3));

    // Run in 0 to 3 cards.
    for (std::size_t i = 0; i < n_cards; ++i)
        advance_cards();

    assert(m_fed_cards.size() == 3);

    if (auto client = m_source_client.lock())
        client->resume();
}

void Input_Output_Unit::end_of_file()
{
    m_end_of_file = true;
    advance();
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

void Input_Output_Unit::advance_cards()
{
    // Feed a card from the stack into the reader.
    m_fed_cards.push_back(m_read_hopper_deck.empty()
                          ? nullptr : std::make_shared<Card>(m_read_hopper_deck.front()));
        
    // If a card was pushed past the 3rd station, move it to the stack.
    if (m_fed_cards.front())
        m_read_stacker_deck.push_back(*(m_fed_cards.front()));
    m_fed_cards.pop_front();
        
    // If a card was pushed into the 3rd station, read it into the buffer.
    if (m_fed_cards.front())
    {
        m_source_buffer.clear();
        for (std::size_t i = 0; i < 8; ++i)
            m_source_buffer.push_back(card_word(*m_fed_cards.front(), i));
        assert(m_source_buffer.size() == 8);
    }

    // Remove the card from the hopper.
    if (!m_read_hopper_deck.empty())
        m_read_hopper_deck.pop_front();

    m_pending_advance = false;
}

void Input_Output_Unit::advance()
{
    // No more cards inside.
    if (std::all_of(m_fed_cards.begin(), m_fed_cards.end(), [](auto p) { return !p; }))
        m_end_of_file = false;
    
    m_pending_advance = true;
    if (m_read_hopper_deck.empty() && !m_end_of_file)
        return;

    advance_cards();
    if (auto client = m_source_client.lock())
        client->resume();
}

Buffer& Input_Output_Unit::get_source()
{
    return m_source_buffer;
}
