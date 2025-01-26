#include "atlascontrolwidget_layout.hpp"

#include "AtlasCommon/atlasenums.hpp"
#include "AtlasMessenger/atlasmessenger.hpp"

#include <ranges>
#include <algorithm>

#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QFileDialog>
#include <QPushButton>

namespace AtlasGUI
{
  AtlasControlWidgetLayout::AtlasControlWidgetLayout(QWidget* parent) : QVBoxLayout{parent}
  {
    Initialize();
  }
    
  auto AtlasControlWidgetLayout::Initialize() -> void
  {
    BuildRatModelWidget();
    BuildImagePathWidget();
    BuildRenderingOptionsWidget();
    BuildImageNavigationWidget();
  }
    
  auto AtlasControlWidgetLayout::BuildRatModelWidget() -> void
  {
    m_ratModelWidget = new QWidget{};
    auto layout = new QVBoxLayout{m_ratModelWidget};
    auto label = new QLabel{"Select Rat Brain Model"};
    auto comboBox = new QComboBox{};
    auto loadButton = new QPushButton{"Load Model"};

    std::ranges::for_each(AtlasCommon::AtlasDataSetNames, [&comboBox](const auto& pair)
    {
      comboBox->addItem(pair.second);
    });

    layout->addWidget(label);
    layout->addWidget(comboBox);
    layout->addWidget(loadButton);
    QObject::connect(loadButton, &QPushButton::clicked, this, &AtlasControlWidgetLayout::LoadModel);
    this->addWidget(m_ratModelWidget);
  }
    
  auto AtlasControlWidgetLayout::BuildImagePathWidget() -> void
  {
    m_imagePathWidget = new QWidget{};
    auto layout = new QVBoxLayout{m_imagePathWidget};
    auto label = new QLabel{"Image Path"};
    auto lineEdit = new QLineEdit{};
    auto browseButton = new QPushButton{"Browse"};
    auto loadImageButton = new QPushButton{"Load Image"};
    layout->addWidget(label);
    layout->addWidget(lineEdit);
    layout->addWidget(browseButton);
    layout->addWidget(loadImageButton);
    
    QObject::connect(browseButton, &QPushButton::clicked, [lineEdit]()
    {
      auto path = QFileDialog::getOpenFileName();
      lineEdit->setText(path);
    });

    auto messenger = &AtlasMessenger::Messenger::Instance();

    QObject::connect(loadImageButton, &QPushButton::clicked, [lineEdit, messenger](){
      auto path = lineEdit->text().toStdString();
      // bail, no path
      if(path.empty())
        return;

      auto args = std::string{"LoadImage," + path};
      messenger->SendMessage(args.c_str(), AtlasCommon::AtlasClasses::AtlasImageViewer);
    });
    
    this->addWidget(m_imagePathWidget);
  }
    
  auto AtlasControlWidgetLayout::BuildRenderingOptionsWidget() -> void
  {
    // TODO: Flesh out the rendering options widget
    m_renderingOptionsWidget = new QWidget{};
    auto layout = new QVBoxLayout{m_renderingOptionsWidget};
    auto label = new QLabel{"Rendering Options"};
    layout->addWidget(label);
    this->addWidget(m_renderingOptionsWidget);
  }
    
  auto AtlasControlWidgetLayout::BuildImageNavigationWidget() -> void
  {
    m_imageNavigationWidget = new QWidget{};
    auto layout = new QVBoxLayout{m_imageNavigationWidget};
    auto label = new QLabel{"Image Navigation"};
    auto button = new QPushButton{"Next Image"};
    layout->addWidget(label);
    layout->addWidget(button);
    this->addWidget(m_imageNavigationWidget);
  }

  auto AtlasControlWidgetLayout::LoadModel() -> void
  {
    auto comboBox = m_ratModelWidget->findChild<QComboBox*>();
    auto dataSet = comboBox->currentIndex();
    auto args = std::string{"LoadDataSet," + std::to_string(dataSet)};
    AtlasMessenger::Messenger::Instance().SendMessage(args.c_str(), AtlasCommon::AtlasClasses::AtlasModel);
  }
}