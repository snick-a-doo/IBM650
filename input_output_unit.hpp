#include "buffer.hpp"

#include <array>
#include <deque>
#include <memory>

namespace IBM533
{
constexpr std::size_t buffer_size = 10;
constexpr std::size_t card_words = 8;
constexpr std::size_t card_columns = IBM650::word_size*card_words;
using Card = std::array<int, card_columns>;
using Card_Deck = std::deque<Card>;

Buffer card_to_buffer(const Card& card);

class Input_Output_Unit : public Source, public Sink
{
    using Card_Ptr_Deck = std::deque<std::shared_ptr<Card>>;

public:
    Input_Output_Unit();

    /// @Return true if the power light is on.  The unit has a physical power switch, so as far
    /// as we're concerned, it's always on.
    bool is_on() const  { return true; }
    /// @Return true if not reading.  The light goes off when read-start is pressed, and turns
    /// on when the hopper is emptied, or the stop key is pressed.
    bool is_read_idle() const;
    /// @Return true if not punching.  The light goes off when punch-start is pressed, and
    /// turns on when the hopper is emptied, or the stop key is pressed.
    bool is_punch_idle() const;
    /// @Return true if the reader is processing the remaining cards after the end-of-file key
    /// was pressed.
    bool is_end_of_file() const;
    /// @Return true if the read feed is jammed.  Always false.
    bool is_read_feed_stopped() const { return false; }
    /// @Return true if a double punch or blank column was detected.  Always false.
    bool is_double_punch_or_blank() const { return false; }

    const Card_Deck& read_hopper_deck() const;
    const Card_Deck& read_stacker_deck() const;
    const Card_Deck& punch_hopper_deck() const;
    const Card_Deck& punch_stacker_deck() const;

    void load_read_hopper(const Card_Deck& deck);
    void load_punch_hopper(const Card_Deck& deck);
    void read_start();
    void punch_start();
    void read_stop();
    void punch_stop();
    /// Processing stops when the hopper is emptied.  The last cards read are still in the
    /// reader.  The program will continue if more cards are added to the hopper.  Pressing
    /// "end of file" lets the program continue and process the cards that are in the reader.
    void end_of_file();

    // Source overrides

    virtual void connect_source_client(std::weak_ptr<Source_Client> client) override;
    virtual void advance_source() override;
    virtual Buffer& get_source() override;

    // Sink overrides

    virtual void connect_sink_client(std::weak_ptr<Sink_Client> client) override;
    virtual void advance_sink() override;
    virtual Buffer& get_sink() override;

private:
    void advance_read_cards();
    void punch();
    Card_Deck m_read_hopper_deck;
    Card_Deck m_read_stacker_deck;
    Card_Deck m_punch_hopper_deck;
    Card_Deck m_punch_stacker_deck;
    Card_Ptr_Deck m_fed_read_cards;
    Card_Ptr_Deck m_fed_punch_cards;
    bool m_read_running = false;
    bool m_punch_running = false;
    bool m_pending_read_advance = false;
    bool m_pending_punch_advance = false;
    bool m_end_of_file = false;

    std::weak_ptr<Source_Client> m_source_client;
    std::weak_ptr<Sink_Client> m_sink_client;
    Buffer m_source_buffer;
    Buffer m_sink_buffer;
};
}
