#ifndef SOUND_WAVE_HPP
#define SOUND_WAVE_HPP

#include <giomm/settings.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/builder.h>
#include <gtkmm/button.h>
#include <gtkmm/grid.h>
#include <gtkmm/stack.h>
#include <memory>
#include "microphone.hpp"

class SoundWave : public Gtk::Grid {
 public:
  SoundWave(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder);
  SoundWave(const SoundWave&) = delete;
  auto operator=(const SoundWave&) -> SoundWave& = delete;
  SoundWave(const SoundWave&&) = delete;
  auto operator=(const SoundWave &&) -> SoundWave& = delete;
  ~SoundWave() override;

  static void add_to_stack(Gtk::Stack* stack);

 private:
  std::string log_tag = "sound wave: ";

  bool left_mouse_button_pressed = false;
  bool right_mouse_button_pressed = false;

  Glib::RefPtr<Gio::Settings> settings;

  std::vector<sigc::connection> connections;

  std::unique_ptr<Microphone> microphone;

  static void get_object(const Glib::RefPtr<Gtk::Builder>& builder,
                         const std::string& name,
                         Glib::RefPtr<Gtk::Adjustment>& object) {
    object = Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder->get_object(name));
  }
};

#endif