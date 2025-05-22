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

#include <print>

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

    // Better to do it this way to keep the indexing. 
    comboBox->addItem("LGN");
    comboBox->addItem("PAG");

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
    auto findBestMatchButton = new QPushButton{"Find Best Match"};

    lineEdit->setReadOnly(true);
    
    layout->addWidget(label);
    layout->addWidget(lineEdit);
    layout->addWidget(browseButton);
    layout->addWidget(loadImageButton);
    layout->addWidget(findBestMatchButton);

    loadImageButton->setEnabled(false);
    findBestMatchButton->setEnabled(false);
    
    QObject::connect(browseButton, &QPushButton::clicked, [lineEdit, loadImageButton]()
    {
      auto path = QFileDialog::getOpenFileName();
      lineEdit->setText(path);
      if(!path.isEmpty())
        loadImageButton->setEnabled(true);
      else
        loadImageButton->setEnabled(false); // Disable it again.
    });

    auto messenger = &AtlasMessenger::Messenger::Instance();

    QObject::connect(loadImageButton, &QPushButton::clicked, [lineEdit, findBestMatchButton, messenger](){
      auto path = lineEdit->text().toStdString();
      // bail, no path
      if(path.empty())
        return;

      findBestMatchButton->setEnabled(true); 
      auto args = std::string{"LoadImage," + path};
      messenger->SendMessage(args.c_str(), AtlasCommon::AtlasClasses::AtlasImageViewer);
    });

    QObject::connect(findBestMatchButton, &QPushButton::clicked, [lineEdit, messenger](){
      auto imgToProcess = lineEdit->text().toStdString();
      auto args = std::string{"GetBestFits," + imgToProcess};
      std::println("Sending Model to get Best Fits!");
      messenger->SendMessage(args.c_str(), AtlasCommon::AtlasClasses::AtlasModel);
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
   
  // TODO: Create a signal that will connect to a slot in the AtlasImageViewer to change images.
  auto AtlasControlWidgetLayout::BuildImageNavigationWidget() -> void
  {
    m_imageNavigationWidget = new QWidget{};
    auto layout = new QVBoxLayout{m_imageNavigationWidget};
    auto label = new QLabel{"Image Navigation"};
    auto nextButton = new QPushButton{"Next Image"};
    auto prevButton = new QPushButton{"Previous Image"};
    layout->addWidget(label);
    layout->addWidget(nextButton);
    layout->addWidget(prevButton);
    this->addWidget(m_imageNavigationWidget);

    auto lineEdit = new QLineEdit{};
    auto messenger = &AtlasMessenger::Messenger::Instance();

    // Next Button and Prev Button Handling
    QObject::connect(nextButton, &QPushButton::clicked, [lineEdit, messenger](){
      std::println("Next button clicked! Sending to back end");
      auto argsNext = std::string("NextButton,Test");
      messenger->SendMessage(argsNext.c_str(), AtlasCommon::AtlasClasses::AtlasImageViewer);
    });
    QObject::connect(prevButton, &QPushButton::clicked, [lineEdit, messenger]() {
       std::println("Prev button clicked! Sending to back end");
       auto argsPrev = std::string("PrevButton,Test");
       messenger->SendMessage(argsPrev.c_str(), AtlasCommon::AtlasClasses::AtlasImageViewer);
    });

  }

  auto AtlasControlWidgetLayout::LoadModel() -> void
  {
    auto comboBox = m_ratModelWidget->findChild<QComboBox*>();
    auto dataSet = comboBox->currentIndex();
    auto args = std::string{"LoadDataSet," + std::to_string(dataSet)};
    AtlasMessenger::Messenger::Instance().SendMessage(args.c_str(), AtlasCommon::AtlasClasses::AtlasModel);
    m_isModelLoaded = true;
  }


}