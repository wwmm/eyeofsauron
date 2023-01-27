#include "application.hpp"
#include "application_ui.hpp"

namespace app {

using namespace std::string_literals;

G_DEFINE_TYPE(Application, application, ADW_TYPE_APPLICATION)

void hide_all_windows(GApplication* app) {
  auto* list = gtk_application_get_windows(GTK_APPLICATION(app));

  while (list != nullptr) {
    auto* window = list->data;
    auto* next = list->next;

    gtk_window_destroy(GTK_WINDOW(window));

    list = next;
  }
}

void on_startup(GApplication* gapp) {
  G_APPLICATION_CLASS(application_parent_class)->startup(gapp);

  auto* self = WW_APP(gapp);

  self->data = new Data();

  if (self->settings == nullptr) {
    self->settings = g_settings_new(tags::app::id);
  }
}

void application_class_init(ApplicationClass* klass) {
  auto* application_class = G_APPLICATION_CLASS(klass);

  application_class->handle_local_options = [](GApplication* gapp, GVariantDict* options) {
    if (options == nullptr) {
      return -1;
    }

    auto* self = WW_APP(gapp);

    if (self->settings == nullptr) {
      self->settings = g_settings_new(tags::app::id);
    }

    return -1;
  };

  application_class->startup = on_startup;

  application_class->command_line = [](GApplication* gapp, GApplicationCommandLine* cmdline) {
    auto* self = WW_APP(gapp);
    auto* options = g_application_command_line_get_options_dict(cmdline);

    if (g_variant_dict_contains(options, "quit") != 0) {
      hide_all_windows(gapp);

      g_application_quit(G_APPLICATION(gapp));
    } else if (g_variant_dict_contains(options, "reset") != 0) {
      util::reset_all_keys_except(self->settings);

      util::info("All settings were reset");
    } else if (g_variant_dict_contains(options, "hide-window") != 0) {
      hide_all_windows(gapp);

      util::info("Hiding the window...");
    } else {
      g_application_activate(gapp);
    }

    return G_APPLICATION_CLASS(application_parent_class)->command_line(gapp, cmdline);
  };

  application_class->activate = [](GApplication* gapp) {
    if (gtk_application_get_active_window(GTK_APPLICATION(gapp)) == nullptr) {
      G_APPLICATION_CLASS(application_parent_class)->activate(gapp);

      auto* window = ui::application_window::create(gapp);

      gtk_window_present(GTK_WINDOW(window));
    }
  };

  application_class->shutdown = [](GApplication* gapp) {
    G_APPLICATION_CLASS(application_parent_class)->shutdown(gapp);

    auto* self = WW_APP(gapp);

    for (auto& c : self->data->connections) {
      c.disconnect();
    }

    for (auto& handler_id : self->data->gconnections) {
      g_signal_handler_disconnect(self->settings, handler_id);
    }

    self->data->connections.clear();

    g_object_unref(self->settings);

    delete self->data;

    self->data = nullptr;

    util::debug("shutting down...");
  };
}

void application_init(Application* self) {
  std::array<GActionEntry, 8> entries{};

  entries[0] = {"quit",
                [](GSimpleAction* action, GVariant* parameter, gpointer app) {
                  util::debug("The user pressed <Ctrl>Q or executed a similar action. Our process will exit.");

                  g_application_quit(G_APPLICATION(app));
                },
                nullptr, nullptr, nullptr};

  entries[1] = {"help",
                [](GSimpleAction* action, GVariant* parameter, gpointer gapp) {
                  gtk_show_uri(gtk_application_get_active_window(GTK_APPLICATION(gapp)), "help:eyeofsauron",
                               GDK_CURRENT_TIME);
                },
                nullptr, nullptr, nullptr};

  entries[2] = {
      "about",
      [](GSimpleAction* action, GVariant* parameter, gpointer gapp) {
        std::array<const char*, 2> developers = {"Wellington Wallace <wellingtonwallace@gmail.com>", nullptr};

        std::array<const char*, 2> documenters = {"Wellington Wallace <wellingtonwallace@gmail.com>", nullptr};

        adw_show_about_window(
            gtk_application_get_active_window(GTK_APPLICATION(gapp)), "application-name", APP_NAME, "version", VERSION,
            "developer-name", "Wellington Wallace", "developers", developers.data(), "application-icon",
            IS_DEVEL_BUILD ? std::string(tags::app::id).append(".Devel").c_str() : tags::app::id, "copyright",
            "Copyright © 2020–2022 Easy Effects Contributors", "license-type", GTK_LICENSE_GPL_3_0, "website",
            "https://github.com/wwmm/easyeffects", "debug-info", std::string("Commit: ").append(COMMIT_DESC).c_str(),
            "documenters", documenters.data(), "issue-url", "https://github.com/wwmm/easyeffects/issues", nullptr);
      },
      nullptr, nullptr, nullptr};

  entries[3] = {"fullscreen",
                [](GSimpleAction* action, GVariant* parameter, gpointer gapp) {
                  auto* self = WW_APP(gapp);

                  auto state = g_settings_get_boolean(self->settings, "window-fullscreen") != 0;

                  if (state) {
                    gtk_window_unfullscreen(GTK_WINDOW(gtk_application_get_active_window(GTK_APPLICATION(gapp))));

                    g_settings_set_boolean(self->settings, "window-fullscreen", 0);
                  } else {
                    gtk_window_fullscreen(GTK_WINDOW(gtk_application_get_active_window(GTK_APPLICATION(gapp))));

                    g_settings_set_boolean(self->settings, "window-fullscreen", 1);
                  }
                },
                nullptr, nullptr, nullptr};

  entries[4] = {"preferences",
                [](GSimpleAction* action, GVariant* parameter, gpointer gapp) {
                  auto* preferences = ui::preferences::window::create();

                  gtk_window_set_transient_for(GTK_WINDOW(preferences),
                                               GTK_WINDOW(gtk_application_get_active_window(GTK_APPLICATION(gapp))));

                  gtk_window_present(GTK_WINDOW(preferences));
                },
                nullptr, nullptr, nullptr};

  entries[5] = {"reset",
                [](GSimpleAction* action, GVariant* parameter, gpointer gapp) {
                  auto* self = WW_APP(gapp);

                  util::reset_all_keys_except(self->settings);
                },
                nullptr, nullptr, nullptr};

  entries[6] = {"shortcuts",
                [](GSimpleAction* action, GVariant* parameter, gpointer gapp) {
                  auto* builder = gtk_builder_new_from_resource(tags::resources::shortcuts_ui);

                  auto* window = GTK_SHORTCUTS_WINDOW(gtk_builder_get_object(builder, "window"));

                  gtk_window_set_transient_for(GTK_WINDOW(window),
                                               GTK_WINDOW(gtk_application_get_active_window(GTK_APPLICATION(gapp))));

                  gtk_window_present(GTK_WINDOW(window));

                  g_object_unref(builder);
                },
                nullptr, nullptr, nullptr};

  entries[7] = {"hide_windows",
                [](GSimpleAction* action, GVariant* parameter, gpointer app) {
                  util::debug("The user pressed <Ctrl>W or executed a similar action. Hiding our window.");

                  hide_all_windows(G_APPLICATION(app));
                },
                nullptr, nullptr, nullptr};

  g_action_map_add_action_entries(G_ACTION_MAP(self), entries.data(), entries.size(), self);

  std::array<const char*, 2> quit_accels = {"<Ctrl>Q", nullptr};
  std::array<const char*, 2> hide_windows_accels = {"<Ctrl>W", nullptr};

  std::array<const char*, 2> help_accels = {"F1", nullptr};
  std::array<const char*, 2> fullscreen_accels = {"F11", nullptr};

  gtk_application_set_accels_for_action(GTK_APPLICATION(self), "app.quit", quit_accels.data());
  gtk_application_set_accels_for_action(GTK_APPLICATION(self), "app.hide_windows", hide_windows_accels.data());
  gtk_application_set_accels_for_action(GTK_APPLICATION(self), "app.help", help_accels.data());
  gtk_application_set_accels_for_action(GTK_APPLICATION(self), "app.fullscreen", fullscreen_accels.data());
}

auto application_new() -> GApplication* {
  g_set_application_name(APP_NAME);

  auto* app = g_object_new(EE_TYPE_APPLICATION, "application-id",
                           IS_DEVEL_BUILD ? std::string(tags::app::id).append(".Devel").c_str() : tags::app::id,
                           "flags", G_APPLICATION_HANDLES_COMMAND_LINE, nullptr);

  g_application_add_main_option(G_APPLICATION(app), "quit", 'q', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE,
                                _("Quit Eye Of Sauron."), nullptr);

  g_application_add_main_option(G_APPLICATION(app), "reset", 'r', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE,
                                _("Reset Eye Of Sauron."), nullptr);

  g_application_add_main_option(G_APPLICATION(app), "hide-window", 'w', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE,
                                _("Hide the Window."), nullptr);

  return G_APPLICATION(app);
}

}  // namespace app
