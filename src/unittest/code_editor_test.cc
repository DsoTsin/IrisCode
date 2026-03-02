#include "coreui/app.hpp"
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
  title_button->width(120.f).height(40.f);

  
  iris::ref_ptr<iris::ui::vsplitter> vsplit(new iris::ui::vsplitter());
  vsplit->width(4);

  iris::ref_ptr<iris::ui::label> sidebar_title(new iris::ui::label());
  sidebar_title->text("Project")
      .font_size(18.0f)
      .alignment(iris::ui::text_alignment::left)
      .auto_width()
      .auto_height();

  iris::ref_ptr<iris::ui::button> build_button(new iris::ui::button());
  build_button->text("构建").width(120.0f).height(34.0f);

  sidebar->add_child(title_button.get());
  sidebar->add_child(sidebar_title.get());
  sidebar->add_child(build_button.get());

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

  window_ref->set_content_view(root.get());
  window_ref->show();
  return app->run();
}
