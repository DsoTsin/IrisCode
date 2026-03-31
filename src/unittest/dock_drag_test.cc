#include "coreui/app.hpp"
#include "coreui/dock_window.hpp"
#include "coreui/view/code_editor.hpp"
#include "coreui/view/dock_view.hpp"
#include "coreui/view/label.hpp"
#include "coreui/view.hpp"
#include "coreui/window.hpp"

#include <Windows.h>

#include <cstdlib>
#include <fstream>

#pragma comment(linker, "/subsystem:windows")

namespace {
void log_line(const char* text) {
  std::ofstream log("dock_drag_test.log", std::ios::app);
  log << text << '\n';
}

iris::ui::dock_view* ancestor_dock_view(iris::ui::view* value) {
  for (auto* node = value; node != nullptr; node = node->parent()) {
    if (auto* dock = dynamic_cast<iris::ui::dock_view*>(node)) {
      return dock;
    }
  }
  return nullptr;
}
} // namespace

#ifdef _WIN32
int WINAPI WinMain(_In_ HINSTANCE,
                   _In_opt_ HINSTANCE,
                   _In_ LPSTR,
                   _In_ int) {
#else
int main() {
#endif
  {
    std::ofstream log("dock_drag_test.log", std::ios::trunc);
    log << "dock_drag_test start" << '\n';
  }

  _putenv_s("IRIS_DOCK_TEST_DISABLE_NATIVE_SHOW", "1");

  auto app = iris::app::shared();
  (void)app;

  iris::ref_ptr<iris::ui::dock_view> root(new iris::ui::dock_view());
  root->set_dock_padding(8.0f);
  root->set_default_dock_size(220.0f, 96.0f);

  iris::ref_ptr<iris::ui::view> sidebar(new iris::ui::view());
  sidebar->flex_dir(iris::ui::FlexDirection::Column)
      .justify_content(iris::ui::Justify::FlexStart)
      .align_items(iris::ui::Align::FlexStart)
      .padding(10.0f);

  iris::ref_ptr<iris::ui::label> sidebar_title(new iris::ui::label());
  sidebar_title->text("Automation")
      .font_size(18.0f)
      .alignment(iris::ui::text_alignment::left)
      .auto_width()
      .auto_height();
  sidebar->add_child(sidebar_title.get());

  iris::ref_ptr<iris::ui::view> splitter(new iris::ui::view());
  splitter->width(4.0f);

  iris::ref_ptr<iris::ui::code_editor> editor(new iris::ui::code_editor());
  editor->set_language(iris::ui::code_language::cpp);
  editor->set_font_size(16.0f);
  editor->set_text(
      "#include <iostream>\n"
      "\n"
      "int main() {\n"
      "  std::cout << \"dock automation\" << std::endl;\n"
      "  return 0;\n"
      "}\n");

  root->dock(sidebar.get(), iris::ui::dock_position::left);
  root->dock(splitter.get(), iris::ui::dock_position::fill);
  root->dock(editor.get(), iris::ui::dock_position::fill);

  log_line("initial layout ok");

  auto floating = root->undock_to_window(editor.get());
  const bool undocked = floating && floating->hosted_view() == editor.get()
                        && ancestor_dock_view(editor.get()) == floating->root_dock_view()
                        && ancestor_dock_view(editor.get()) != root.get();
  log_line(undocked ? "undock ok" : "undock failed");
  if (!undocked) {
    return 1;
  }

  const bool redock_called = floating->redock_to(root.get(), iris::ui::dock_position::fill);
  const bool final_docked = redock_called && floating->hosted_view() == nullptr
                            && ancestor_dock_view(editor.get()) == root.get();
  log_line(final_docked ? "redock ok" : "redock failed");
  return final_docked ? 0 : 1;
}
