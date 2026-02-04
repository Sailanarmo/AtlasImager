#pragma once

#include <QVBoxLayout>
#include <QProgressBar>
#include <QLabel>

namespace AtlasGUI
{
  class AtlasProgressPopupLayout : public QVBoxLayout
  {
    Q_OBJECT

  public:
    AtlasProgressPopupLayout(const QString& mainLoadingText, const QString& format, QWidget* parent = nullptr);
    ~AtlasProgressPopupLayout() = default;

    auto SetMaxProgressBarValue(const int max) -> void;

  public slots:
    auto SetProgressValue(int value) -> void;
  
  signals:
    auto DisplayWidgetSignal() -> void;
    auto DestroyWidgetSignal() -> void;

  private:
    QProgressBar* m_progressBar{nullptr};
    QLabel* m_mainLoadingLabel{nullptr};

    auto Initialize() -> void;
    auto SetMainLoadingText(const QString& text) -> void;
    auto SetProgressBarText(const QString& text) -> void;
  };
}