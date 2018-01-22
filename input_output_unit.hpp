#include "buffer.hpp"

#include <array>
#include <deque>
#include <memory>

namespace IBM533
{
using Card = std::array<int, 80>;
using Card_Deck = std::deque<Card>;

class Input_Output_Unit : public Source
{
public:
    Input_Output_Unit();

    /// @Return true if the power light is on.  The unit has a physical power switch, so as far
    /// as we're concerned, it's always on.
    bool is_on() const { return true; }
    /// @Return true if not reading.  Unlabeled light.
    bool read_is_idle() const { return true; }
    /// @Return true if not punching.  Unlabeled light.
    bool punch_is_idle() const { return true; }
    /// @Return true if a double punch or blank column was detected.
    bool double_punch_and_blank() const { return false; }

    const Card_Deck& read_hopper_deck() const;
    const Card_Deck& read_stacker_deck() const;
    const Card_Deck& punch_hopper_deck() const;
    const Card_Deck& punch_stacker_deck() const;

    void load_read_hopper(const Card_Deck& deck);
    void read_start();
    /// Processing stops when the hopper is emptied.  The last cards read are still in the
    /// reader.  The program will continue if more cards are added to the hopper.  Pressing
    /// "end of file" lets the program continue and process the cards that are in the reader.
    void end_of_file();

    // Source overrides

    virtual void connect_source_client(std::weak_ptr<Source_Client> client) override;
    virtual void advance() override;
    virtual Buffer& get_source() override;

private:
    void advance_cards();
    Card_Deck m_read_hopper_deck;
    Card_Deck m_read_stacker_deck;
    Card_Deck m_punch_hopper_deck;
    Card_Deck m_punch_stacker_deck;
    std::deque<std::shared_ptr<Card>> m_fed_cards;
    bool m_pending_advance;
    bool m_end_of_file;

    std::weak_ptr<Source_Client> m_source_client;
    Buffer m_source_buffer;
};
}
