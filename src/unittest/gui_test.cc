#include "coreui/app.hpp"
#include "coreui/window.hpp"
#include "coreui/view.hpp"
#include "coreui/view/button.hpp"
#include "coreui/view/label.hpp"

#include <Windows.h>

#pragma comment(linker, "/subsystem:windows")

class MyView : public iris::ui::view {
public:
  MyView();

  void draw(const iris::ui::rect& dirty_rect) override;
};

#ifdef _WIN32
int WINAPI WinMain(_In_ HINSTANCE hInInstance,
                   _In_opt_ HINSTANCE hPrevInstance,
                   _In_ LPSTR,
                   _In_ int nCmdShow) {
#else
int main() {
#endif
  auto app = iris::app::shared();
  iris::ref_ptr<iris::ui::window> w(new iris::ui::window());
  iris::ref_ptr<iris::ui::view> view_ptr(new MyView);
  iris::ref_ptr<iris::ui::button> button_ptr(new iris::ui::button);
  iris::ref_ptr<iris::ui::label> label_ptr(new iris::ui::label);
  (*button_ptr).width(32).height(32)/*.padding(20)*/;

  (*label_ptr).text("test IrisUI OP")
      .font_size(24)
      .alignment(iris::ui::text_alignment::center)
      .auto_width()
      .auto_height();

  (*view_ptr).flex_dir(iris::ui::FlexDirection::Row)
      .justify_content(iris::ui::Justify::FlexStart)
      .align_items(iris::ui::Align::FlexStart)
      .padding(10);

  view_ptr->add_child(button_ptr.get());
  view_ptr->add_child(label_ptr.get());

  w->set_content_view(view_ptr.get());
  w->show();
  return app->run();
}

MyView::MyView() {}

void MyView::draw(const iris::ui::rect& dirty_rect) {
  iris::ui::view::draw(dirty_rect);

  auto rect = get_layout_rect();
}
