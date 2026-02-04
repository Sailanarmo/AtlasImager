#pragma once

#include <QFrame>

#include "atlasprogresspopupwidget.hpp"


namespace AtlasGUI
{
  class AtlasMainWindow : public QFrame
  {
    Q_OBJECT

  public:
    AtlasMainWindow(QWidget* parent = nullptr);
    ~AtlasMainWindow() = default;
  
  public slots:
    auto CreateLoadingPopup(const QString& mainLoadingText, const QString& format) -> void;
    auto SetMaximumProgressBarValue(const int max) -> void;
    auto DisplayLoadingPopup() -> void;
    auto DestroyLoadingPopup() -> void;

  private:
    auto Initialize() -> void;

    AtlasProgressPopupWidget* m_loadingPopup{nullptr};
  };
}