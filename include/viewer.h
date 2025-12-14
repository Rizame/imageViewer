#include <gtkmm/image.h>
#include <gtkmm/window.h>
#include <memory>

class Viewer : public Gtk::Window {
public:
  Viewer(int w, int h);
  void load_file(const std::string &filename);

private:
  std::unique_ptr<Gtk::Image> m_image;
};
