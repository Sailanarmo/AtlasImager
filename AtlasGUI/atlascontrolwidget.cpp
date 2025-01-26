#include "atlascontrolwidget.hpp"
#include "atlascontrolwidget_layout.hpp"

namespace AtlasGUI
{
  AtlasControlWidget::AtlasControlWidget(QWidget* parent) : QWidget{parent}
  {
    Initialize();
  }

  auto AtlasControlWidget::Initialize() -> void
  {
    auto layout = new AtlasControlWidgetLayout{this};
    this->setLayout(layout);
  }
}