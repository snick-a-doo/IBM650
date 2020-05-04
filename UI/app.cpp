// g++ app.cpp -o app `pkg-config gtkmm-3.0 --cflags --libs` -L.. -lIBM650
#include "../computer.hpp"
#include <gtkmm.h>
#include <iostream>

static constexpr int timestep_ms = 100;
static constexpr std::size_t bits_per_word = 7;

class Console : public Gtk::Window
{
public:
    Console(Glib::RefPtr<Gtk::Builder> builder);
    virtual ~Console() = default;

private:
    IBM650::Computer c;

    void on_power_on();
    void on_power_off();
    void on_dc_off();
    void on_dc_on();

    void on_storage(int digit, Gtk::Scale* s);
    void on_storage_sign(IBM650::TDigit sign);

    void on_programmed(int index);
    void on_half_cycle(int index);

    void on_address(int digit, Gtk::Scale* s);

    void on_control(int index);
    void on_display(int index);
    void on_overflow(int index);
    void on_error(int index);

    void on_transfer();
    void on_program_start();
    void on_program_stop();
    void on_program_reset();
    void on_computer_reset();
    void on_accum_reset();
    void on_error_reset();
    void on_error_sense_reset();

    bool update(int timer);
    void display(IBM650::Word);

    Gtk::ToggleButton* m_power_on_light;
    Gtk::ToggleButton* m_ready_light;

    using TDigit_Display = std::array<Gtk::CheckButton*, bits_per_word>;
    using TWord_Display = std::array<TDigit_Display, IBM650::word_size>;
    TWord_Display m_display_light;
    Gtk::CheckButton* m_display_plus;
    Gtk::CheckButton* m_display_minus;

    IBM650::Word m_storage = IBM650::zero;
    IBM650::Address m_address = IBM650::Address({0,0,0,0});

    double m_time_s = 0.0;
    double m_last_step_s = 0.0;
};

Console::Console(Glib::RefPtr<Gtk::Builder> builder)
{
    auto add_button = [builder, this](std::string id, void (Console::*callback)()) {
        Gtk::Button* b;
        builder->get_widget(id, b);
        b->signal_clicked().connect(sigc::mem_fun(*this, callback));
    };
    auto add_radio_button = [builder, this](std::string id,
                                            int index,
                                            void (Console::*callback)(int)) {
        Gtk::RadioButton* rb;
        builder->get_widget(id, rb);
        rb->signal_clicked().connect(sigc::bind<int>(sigc::mem_fun(*this, callback), index));
    };
    auto add_radio = [add_radio_button](std::string group,
                                        std::vector<std::string> choices,
                                        void (Console::*callback)(int)) {
        int index = 0;
        for (const auto& choice : choices)
            add_radio_button(group + ' ' + choice, index++, callback);
    };
    auto add_scale = [builder, this](std::string id,
                                     int digit,
                                     void (Console::*callback)(int, Gtk::Scale* s)) {
        Gtk::Scale* s;
        builder->get_widget(id + ' ' + std::to_string(digit), s);
        s->set_value(0);
        s->signal_value_changed().connect(
            sigc::bind<int, Gtk::Scale*>(sigc::mem_fun(*this, callback), digit+1, s));
    };
    auto add_sign = [builder, this](std::string id,
                                    IBM650::TDigit sign,
                                    void (Console::*callback)(IBM650::TDigit)) {
        Gtk::RadioButton* rb;
        builder->get_widget(id + ' ' + sign, rb);
        rb->signal_clicked().connect(
            sigc::bind<IBM650::TDigit>(sigc::mem_fun(*this, callback), sign));
    };

    add_button("power on", &Console::on_power_on);
    builder->get_widget("power on light", m_power_on_light);
    builder->get_widget("ready light", m_ready_light);
    add_button("power off", &Console::on_power_off);
    add_button("dc off", &Console::on_dc_off);
    add_button("dc on", &Console::on_dc_on);

    for (std::size_t digit = 0; digit < m_display_light.size(); ++digit)
        for (std::size_t bit = 0; bit < m_display_light.front().size(); ++bit)
        {
            std::string id = "display " + std::to_string(digit) + ' ' + std::to_string(bit);
            builder->get_widget(id, m_display_light[digit][bit]);
        }

    builder->get_widget("display +", m_display_plus);
    builder->get_widget("display -", m_display_minus);

    for (std::size_t i = 0; i < IBM650::word_size; ++i)
        add_scale("storage", i, &Console::on_storage);
    for (auto sign : {'+', '-'})
        add_sign("storage", sign, &Console::on_storage_sign);

    add_radio("programmed", {"stop", "run"}, &Console::on_programmed);
    add_radio("half cycle", {"half", "run"}, &Console::on_half_cycle);

    for (std::size_t i = 0; i < IBM650::address_size; ++i)
        add_scale("address", i, &Console::on_address);

    add_radio("control", {"address stop", "run", "manual operation"}, &Console::on_control);
    add_radio("display", {"lower accum",
                          "upper accum",
                          "distributor",
                          "program register",
                          "read-out storage",
                          "read-in storage"}, &Console::on_display);
    add_radio("overflow", {"stop", "sense"}, &Console::on_overflow);
    add_radio("error", {"stop", "sense"}, &Console::on_error);

    add_button("transfer", &Console::on_transfer);
    add_button("program start", &Console::on_program_start);
    add_button("program stop", &Console::on_program_stop);
    add_button("program reset", &Console::on_program_reset);
    add_button("computer reset", &Console::on_computer_reset);
    add_button("accum reset", &Console::on_accum_reset);
    add_button("error reset", &Console::on_error_reset);
    add_button("error sense reset", &Console::on_error_sense_reset);

    // Set the computer state to match the controls.
    c.set_storage_entry(m_storage);
    c.set_programmed_mode(IBM650::Computer::Programmed_Mode(0));
    c.set_half_cycle_mode(IBM650::Computer::Half_Cycle_Mode(0));
    c.set_address(m_address);
    c.set_control_mode(IBM650::Computer::Control_Mode(0));
    c.set_display_mode(IBM650::Computer::Display_Mode(0));
    c.set_overflow_mode(IBM650::Computer::Overflow_Mode(0));
    c.set_error_mode(IBM650::Computer::Error_Mode(0));

    Glib::signal_timeout().connect(
        sigc::bind(sigc::mem_fun(*this, &Console::update), 0), timestep_ms);
}

void Console::on_power_on()
{
    c.power_on();
    // Jump forward in time so we only have to Wait for 5 seconds for DC power.
    c.step(175);
}
void Console::on_power_off()
{
    c.power_off();
}
void Console::on_dc_off()
{
    c.dc_off();
}
void Console::on_dc_on()
{
    c.dc_on();
}
void Console::on_storage(int digit, Gtk::Scale* s)
{
    m_storage[digit] = IBM650::bin(s->get_value());
    c.set_storage_entry(m_storage);
}
void Console::on_storage_sign(IBM650::TDigit sign)
{
    m_storage[0] = IBM650::bin(sign);
    c.set_storage_entry(m_storage);
}
void Console::on_programmed(int index)
{
    c.set_programmed_mode(IBM650::Computer::Programmed_Mode(index));
}
void Console::on_half_cycle(int index)
{
    c.set_half_cycle_mode(IBM650::Computer::Half_Cycle_Mode(index));
}
void Console::on_address(int digit, Gtk::Scale* s)
{
    m_address[digit] = IBM650::bin(s->get_value());
    c.set_address(m_address);
}
void Console::on_control(int index)
{
    c.set_control_mode(IBM650::Computer::Control_Mode(index));
}
void Console::on_display(int index)
{
    c.set_display_mode(IBM650::Computer::Display_Mode(index));
    display(c.display());
}
void Console::on_overflow(int index)
{
    c.set_overflow_mode(IBM650::Computer::Overflow_Mode(index));
}
void Console::on_error(int index)
{
    c.set_error_mode(IBM650::Computer::Error_Mode(index));
}
void Console::on_transfer()
{
    c.transfer();
}
void Console::on_program_start()
{
    c.program_start();
}
void Console::on_program_stop()
{
    //!!TODO c.program_stop();
}
void Console::on_program_reset()
{
    c.program_reset();
}
void Console::on_computer_reset()
{
    c.computer_reset();
}
void Console::on_accum_reset()
{
    c.accumulator_reset();
}
void Console::on_error_reset()
{
    c.error_reset();
}
void Console::on_error_sense_reset()
{
    c.error_sense_reset();
}

void Console::display(IBM650::Word word)
{
    static const std::size_t places = m_display_light.size();
    static const std::size_t bits = m_display_light.front().size();
    // The sign could be any bit pattern, not necessarily + or -.
    m_display_plus->set_active(word.sign() == '+');
    m_display_minus->set_active(word.sign() == '-');
    for (std::size_t place = 0; place < places; ++place)
        for (std::size_t bit = 0; bit < bits; ++bit)
            m_display_light[place][bit]->set_active(word[place+1] & (1 << bit));
}

bool Console::update(int timer)
{
    m_time_s += timestep_ms/1000.0;
    if (m_time_s - m_last_step_s >= 1.0)
    {
        int seconds = static_cast<int>(m_time_s - m_last_step_s);
        c.step(seconds);
        m_last_step_s += seconds;
    }
    m_power_on_light->set_active(c.is_on());
    m_ready_light->set_active(c.is_ready());
    display(c.display());
    return true;
}

int main(int argc, char* argv[])
{
  auto app = Gtk::Application::create("org.gtkmm.examples.base");
  auto builder = Gtk::Builder::create_from_file("IBM650.glade");
  Gtk::Window* window;
  builder->get_widget("console", window);

  Console console(builder);

  return app->run(*window, argc, argv);
}
