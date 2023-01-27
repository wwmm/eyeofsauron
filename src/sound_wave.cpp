#include "sound_wave.hpp"
#include <glibmm/i18n.h>
#include "util.hpp"

SoundWave::SoundWave(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder)
    : Gtk::Grid(cobject),
      settings(Gio::Settings::create("com.github.wwmm.eyeofsauron.soundwave")),
      microphone(std::make_unique<Microphone>()) {}

SoundWave::~SoundWave() {
  for (auto& c : connections) {
    c.disconnect();
  }

  util::debug(log_tag + "destroyed");
}

void SoundWave::add_to_stack(Gtk::Stack* stack) {
  auto builder = Gtk::Builder::create_from_resource("/com/github/wwmm/eyeofsauron/ui/sound_wave.glade");

  SoundWave* ui = nullptr;

  builder->get_widget_derived("widgets_grid", ui);

  stack->add(*ui, "sound_wave", _("SoundWave"));
  stack->child_property_icon_name(*ui) = "microphone-symbolic";
}