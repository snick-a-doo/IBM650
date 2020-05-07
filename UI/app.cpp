#include "../computer.hpp"
#include <gtkmm.h>
#include <iostream>

static constexpr int timestep_ms = 100;
static constexpr std::size_t bits_per_word = 7;

using TDigit_Display = std::array<Gtk::CheckButton*, bits_per_word>;

class Console : public Gtk::Window
{
public:
    Console(Glib::RefPtr<Gtk::Builder> builder);

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
    void on_master_power(bool on);

    bool update(int timer);
    void display();

    Gtk::ToggleButton* m_power_on_light;
    Gtk::ToggleButton* m_ready_light;

    std::array<TDigit_Display, IBM650::word_size + 1> m_display_lights;
    std::array<TDigit_Display, 2> m_operation_lights;
    std::array<TDigit_Display, IBM650::address_size> m_address_lights;

    IBM650::Word m_storage = IBM650::zero;

    Gtk::ToggleButton* m_data_adddress_light;
    Gtk::ToggleButton* m_instruction_adddress_light;
    Gtk::ToggleButton* m_program_register_error_light;
    Gtk::ToggleButton* m_storage_selection_error_light;
    Gtk::ToggleButton* m_overflow_light;
    Gtk::ToggleButton* m_clocking_error_light;
    Gtk::ToggleButton* m_accumulator_error_light;
    Gtk::ToggleButton* m_error_sense_light;

    IBM650::Address m_address = IBM650::Address({0,0,0,0});

    double m_time_s = 0.0;
    double m_last_step_s = 0.0;
};

Console::Console(Glib::RefPtr<Gtk::Builder> builder)
{
    auto add_button = [builder, this](std::string id, void(Console::*callback)()) {
        Gtk::Button* b;
        builder->get_widget(id, b);
        b->signal_clicked().connect(sigc::mem_fun(*this, callback));
    };
    auto add_radio_button = [builder, this](std::string id,
                                            int index,
                                            void(Console::*callback)(int)) {
        Gtk::RadioButton* rb;
        builder->get_widget(id, rb);
        rb->signal_clicked().connect(sigc::bind<int>(sigc::mem_fun(*this, callback), index));
    };
    auto add_radio = [add_radio_button](std::string group,
                                        std::vector<std::string> choices,
                                        void(Console::*callback)(int)) {
        int index = 0;
        for (const auto& choice : choices)
            add_radio_button(group + ' ' + choice, index++, callback);
    };
    auto add_scale = [builder, this](std::string id,
                                     int digit,
                                     void(Console::*callback)(int, Gtk::Scale* s)) {
        Gtk::Scale* s;
        builder->get_widget(id + ' ' + std::to_string(digit), s);
        s->set_value(0);
        s->signal_value_changed().connect(
            sigc::bind<int, Gtk::Scale*>(sigc::mem_fun(*this, callback), digit, s));
    };
    auto add_register = [builder](std::string id, auto& lights) {
        for (std::size_t digit = 0; digit < lights.size(); ++digit)
            for (std::size_t bit = 0; bit < lights.front().size(); ++bit)
            {
                auto name = id + ' ' + std::to_string(digit) + ' ' + std::to_string(bit);
                auto& light = lights[digit][bit];
                builder->get_widget(name, light);
                light->set_can_focus(false);
                light->set_sensitive(false);
            }
    };
    auto add_sign = [builder, this](std::string id,
                                    IBM650::TDigit sign,
                                    void(Console::*callback)(IBM650::TDigit)) {
        Gtk::RadioButton* rb;
        builder->get_widget(id + ' ' + sign, rb);
        rb->signal_clicked().connect(
            sigc::bind<IBM650::TDigit>(sigc::mem_fun(*this, callback), sign));
    };
    auto add_light = [builder](std::string id) {
        Gtk::CheckButton* check;
        builder->get_widget(id, check);
        check->set_can_focus(false);
        check->set_sensitive(false);
        return check;
    };

    // Power
    add_button("power on", &Console::on_power_on);
    builder->get_widget("power on light", m_power_on_light);
    builder->get_widget("ready light", m_ready_light);
    add_button("power off", &Console::on_power_off);
    add_button("dc off", &Console::on_dc_off);
    add_button("dc on", &Console::on_dc_on);

    // Storage Selection
    add_register("display", m_display_lights);
    for (std::size_t i = 0; i < IBM650::word_size; ++i)
        add_scale("storage", i, &Console::on_storage);
    for (auto sign : {'+', '-'})
        add_sign("storage", sign, &Console::on_storage_sign);

    // Operation and Address
    add_register("operation", m_operation_lights);
    add_register("address", m_address_lights);

    // Operating Lights
    m_data_adddress_light = add_light("data address light");
    m_instruction_adddress_light = add_light("instruction address light");
    // Not implemented yet.
    add_light("program light");
    add_light("accumulator light");
    add_light("punch light");
    add_light("read light");

    // Checking Lights
    m_program_register_error_light = add_light("program register error light");
    m_storage_selection_error_light = add_light("storage selection error light");
    m_overflow_light = add_light("overflow light");
    m_clocking_error_light = add_light("clocking error light");
    m_accumulator_error_light = add_light("accumulator error light");
    m_error_sense_light = add_light("error sense light");
    add_light("blank light");  // For symmetry.  Does nothing.

    // Address Selection
    for (std::size_t i = 0; i < IBM650::address_size; ++i)
        add_scale("address", i, &Console::on_address);

    // Modes
    add_radio("programmed", {"stop", "run"}, &Console::on_programmed);
    add_radio("half cycle", {"half", "run"}, &Console::on_half_cycle);
    add_radio("control", {"address stop", "run", "manual operation"}, &Console::on_control);
    add_radio("display", {"lower accum",
                          "upper accum",
                          "distributor",
                          "program register",
                          "read-out storage",
                          "read-in storage"}, &Console::on_display);
    add_radio("overflow", {"stop", "sense"}, &Console::on_overflow);
    add_radio("error", {"stop", "sense"}, &Console::on_error);

    // Actions
    add_button("transfer", &Console::on_transfer);
    add_button("program start", &Console::on_program_start);
    add_button("program stop", &Console::on_program_stop);
    add_button("program reset", &Console::on_program_reset);
    add_button("computer reset", &Console::on_computer_reset);
    add_button("accum reset", &Console::on_accum_reset);
    add_button("error reset", &Console::on_error_reset);
    add_button("error sense reset", &Console::on_error_sense_reset);

    Gtk::Switch* master_power;
    builder->get_widget("master power", master_power);
    master_power->set_active(true);
    // Doesn't matter here because we can only turn it off, but it would be nice to know
    // how to pass the switch state.
    master_power->property_active().signal_changed().connect(
        sigc::bind<bool>(sigc::mem_fun(*this, &Console::on_master_power), false));

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
    m_storage[digit+1] = IBM650::bin(s->get_value());
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
    display();
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
void Console::on_master_power(bool on)
{
    std::cerr << "power " << on << std::endl;
    // Can't be turned back on.
    if (!on)
        c.master_power_off();
}

template <std::size_t N>
void display_register(const IBM650::Register<N>& reg,
                      const std::array<TDigit_Display, N>& lights)
{
    for (std::size_t place = 0; place < N; ++place)
        for (std::size_t bit = 0; bit < bits_per_word; ++bit)
            lights[place][bit]->set_active(reg[place] & (1 << bit));
}

void Console::display()
{
    m_power_on_light->set_active(c.is_on());
    m_ready_light->set_active(c.is_ready());

    display_register(c.display(), m_display_lights);
    display_register(c.operation_register(), m_operation_lights);
    display_register(c.address_register(), m_address_lights);

    m_data_adddress_light->set_active(c.data_address());
    m_instruction_adddress_light->set_active(c.instruction_address());
    m_program_register_error_light->set_active(c.program_register_validity_error());
    m_storage_selection_error_light->set_active(c.storage_selection_error());
    m_overflow_light->set_active(c.overflow());
    m_clocking_error_light->set_active(c.clocking_error());
    m_accumulator_error_light->set_active(c.accumulator_validity_error());
    m_error_sense_light->set_active(c.error_sense());
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
    display();
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
