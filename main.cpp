#include "viewer.h"
#include <gtkmm.h>
#include <iostream>

int main(int argc, char *argv[]) {
  auto app = Gtk::Application::create("org.gtkmm.examples.base",
                                      Gio::Application::Flags::HANDLES_OPEN);

  app->signal_open().connect(
      [&app](const std::vector<Glib::RefPtr<Gio::File>> &files,
             const Glib::ustring &hint) {
        // Create window
        auto win = Gtk::make_managed<Viewer>(200, 200);
        app->add_window(*win);

        // Load the first file
        if (!files.empty()) {
          std::string filename = files[0]->get_basename();
          win->load_file(filename);
        }

        win->show();
      });
  // Handle activation when no files are passed
  app->signal_activate().connect([&app]() {
    std::cerr << "Error: No file provided.\n";
    app->quit();
  });

  return app->run(argc, argv);
}
