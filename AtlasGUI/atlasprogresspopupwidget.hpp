#pragma once

#include "atlasprogresspopup_layout.hpp"

namespace AtlasGUI
{
  class AtlasProgressPopupWidget : public QWidget
  {
    Q_OBJECT
    public:
      AtlasProgressPopupWidget(const QString& mainLoadingText, const QString& format, QWidget* parent = nullptr);
      ~AtlasProgressPopupWidget() = default;

      auto SetMaxProgressBarValue(const int max) -> void;

    public slots:
      auto UpdateProgressValue(int value) -> void;
    
    private:
      AtlasProgressPopupLayout* m_layout{nullptr};
  };
}