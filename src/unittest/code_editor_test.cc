#include "coreui/app.hpp"
#include "coreui/dock_window.hpp"
#include "coreui/view/dock_view.hpp"
#include "coreui/view/code_editor.hpp"
#include "coreui/view/button.hpp"
#include "coreui/view/label.hpp"
#include "coreui/view.hpp"
#include "coreui/window.hpp"

#include <Windows.h>

#pragma comment(linker, "/subsystem:windows")

#ifdef _WIN32
int WINAPI WinMain(_In_ HINSTANCE hInInstance,
                   _In_opt_ HINSTANCE hPrevInstance,
                   _In_ LPSTR,
                   _In_ int nCmdShow) {
#else
int main() {
#endif
  auto app = iris::app::shared();
  iris::ref_ptr<iris::ui::window> window_ref(new iris::ui::window());

  iris::ref_ptr<iris::ui::dock_view> root(new iris::ui::dock_view());
  root->set_dock_padding(8.0f);
  root->set_default_dock_size(220.0f, 96.0f);

  iris::ref_ptr<iris::ui::view> sidebar(new iris::ui::view());
  sidebar->flex_dir(iris::ui::FlexDirection::Column)
      .justify_content(iris::ui::Justify::FlexStart)
      .align_items(iris::ui::Align::FlexStart)
      .padding(10.0f);

  iris::ref_ptr<iris::ui::title_button> title_button(new iris::ui::title_button());
  title_button->width(120.0f).height(40.0f);

  iris::ref_ptr<iris::ui::vsplitter> vsplit(new iris::ui::vsplitter());
  vsplit->width(4.0f);

  iris::ref_ptr<iris::ui::label> sidebar_title(new iris::ui::label());
  sidebar_title->text("Project")
      .font_size(18.0f)
      .alignment(iris::ui::text_alignment::left)
      .auto_width()
      .auto_height();

  iris::ref_ptr<iris::ui::button> build_button(new iris::ui::button());
  build_button->text("Build").width(120.0f).height(34.0f);

  iris::ref_ptr<iris::ui::button> undock_button(new iris::ui::button());
  undock_button->text("Undock Editor").width(120.0f).height(34.0f);

  iris::ref_ptr<iris::ui::button> dock_button(new iris::ui::button());
  dock_button->text("Dock Editor").width(120.0f).height(34.0f);

  sidebar->add_child(title_button.get());
  sidebar->add_child(sidebar_title.get());
  sidebar->add_child(build_button.get());
  sidebar->add_child(undock_button.get());
  sidebar->add_child(dock_button.get());

  iris::ref_ptr<iris::ui::code_editor> editor(new iris::ui::code_editor());
  editor->set_language(iris::ui::code_language::cpp);
  editor->set_font_size(16.0f);
  editor->set_text(
      "#include <iostream>\n"
      "\n"
      "int main() {\n"
      "  // Iris code editor sample\n"
      "  std::cout << \"hello from code_editor\" << std::endl;\n"
      "  return 0;\n"
      "}\n");

  root->dock(sidebar.get(), iris::ui::dock_position::left);
  root->dock(vsplit.get(), iris::ui::dock_position::fill);
  root->dock(editor.get(), iris::ui::dock_position::fill);

  iris::ref_ptr<iris::ui::dock_window> editor_window;
  undock_button->on_click([&]() {
    if (editor_window || editor->parent() != root.get()) {
      return;
    }
    editor_window = root->undock_to_window(editor.get());
  });
  dock_button->on_click([&]() {
    if (!editor_window) {
      return;
    }
    if (editor_window->redock_to(root.get(), iris::ui::dock_position::fill)) {
      editor_window = iris::ref_ptr<iris::ui::dock_window>();
    }
  });

  window_ref->set_content_view(root.get());
  window_ref->show();
  return app->run();
}
