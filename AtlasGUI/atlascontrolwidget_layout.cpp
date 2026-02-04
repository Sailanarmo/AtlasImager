#include "atlascontrolwidget_layout.hpp"

#include "AtlasLogger/atlaslogger.hpp"
#include "AtlasMessenger/atlasmessenger.hpp"
#include "AtlasImageViewer/atlasimageviewer.hpp"
#include "atlasslider.hpp"

#include <ranges>
#include <algorithm>

#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QFileDialog>
#include <QPushButton>
#include <QCheckBox>

#include <print>

namespace AtlasGUI
{
  static AtlasLogger::Logger m_logger{std::filesystem::current_path().string() + "/Logs/AtlasControlWidgetLayout.log", "AtlasGUI::AtlasControlWidgetLayout"};

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
    BuildSaveImageWidget();
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
    auto renderImageButton = new QPushButton{"Render Image"};
    auto loadAllModelImagesButton = new QPushButton{"Load All Model Images"};

    // TODO: Implement matching functionality
    //auto findBestMatchButton = new QPushButton{"Find Best Match"};

    lineEdit->setReadOnly(true);
    
    layout->addWidget(label);
    layout->addWidget(lineEdit);
    layout->addWidget(browseButton);
    layout->addWidget(renderImageButton);
    layout->addWidget(loadAllModelImagesButton);
    //layout->addWidget(findBestMatchButton);

    renderImageButton->setEnabled(false);
    loadAllModelImagesButton->setEnabled(false);
    //findBestMatchButton->setEnabled(false);
    
    QObject::connect(browseButton, &QPushButton::clicked, [lineEdit, renderImageButton]()
    {
      auto path = QFileDialog::getOpenFileName();
      lineEdit->setText(path);
      if(!path.isEmpty())
        renderImageButton->setEnabled(true);
      else
        renderImageButton->setEnabled(false); // Disable it again.
    });

    auto messenger = &AtlasMessenger::Messenger::Instance();

    QObject::connect(renderImageButton, &QPushButton::clicked, [lineEdit, loadAllModelImagesButton, messenger, this](){
      auto path = lineEdit->text().toStdString();
      // bail, no path
      if(path.empty())
        return;

      messenger->UpdateState(AtlasCommon::AtlasImageViewerState::LoadImage, AtlasCommon::AtlasClasses::AtlasImageViewer, path);

      loadAllModelImagesButton->setEnabled(true); 
      m_renderingOptionsWidget->setEnabled(true); // Enable rendering options once an image is loaded.
    });

    QObject::connect(loadAllModelImagesButton, &QPushButton::clicked, [this, lineEdit, messenger](){
      auto imgToProcess = lineEdit->text().toStdString();
      m_logger.Log(AtlasLogger::LogLevel::Info, "Loading all model images for: {}", imgToProcess);
      messenger->UpdateState(AtlasCommon::AtlasModelState::LoadAllModelImages, AtlasCommon::AtlasClasses::AtlasModel);

      if (nextButton && prevButton) {
          nextButton->setEnabled(true);
          prevButton->setEnabled(true);
      }
    });

    /*
    QObject::connect(loadImageButton, &QPushButton::clicked, [lineEdit, findBestMatchButton, messenger](){
      auto path = lineEdit->text().toStdString();
      // bail, no path
      if(path.empty())
        return;

      findBestMatchButton->setEnabled(true); 
      auto args = std::string{"LoadImage," + path};
      messenger->UpdateState(AtlasCommon::AtlasImageViewerState::LoadImage, AtlasCommon::AtlasClasses::AtlasImageViewer, path);
    });

    QObject::connect(findBestMatchButton, &QPushButton::clicked, [this, lineEdit, messenger](){
      auto imgToProcess = lineEdit->text().toStdString();
      auto args = std::string{"GetBestFits," + imgToProcess};
      m_logger.Log(AtlasLogger::LogLevel::Info, "Requesting best fits for image: {}", imgToProcess);
      messenger->UpdateState(AtlasCommon::AtlasModelState::FindingBestFits, AtlasCommon::AtlasClasses::AtlasModel, imgToProcess);

      if (nextButton && prevButton) {
          nextButton->setEnabled(true);
          prevButton->setEnabled(true);
      }
    });
    */
    
    this->addWidget(m_imagePathWidget);
  }
    
  auto AtlasControlWidgetLayout::BuildRenderingOptionsWidget() -> void
  {
    m_renderingOptionsWidget = new QWidget{};
    auto layout = new QVBoxLayout{m_renderingOptionsWidget};
    auto label = new QLabel("Rendering Options\n");
    layout->addWidget(label);

    // Create slider
    auto label2 = new QLabel("Opacity: 1.0");
    layout->addWidget(label2);
    AtlasSlider* slider = new AtlasSlider(Qt::Horizontal, 0, 100);
    slider->setValue(100);
    layout->addWidget(slider);

    auto luminanceThresholdLabel = new QLabel("Luminance Threshold: 0 (0.000)");
    layout->addWidget(luminanceThresholdLabel);
    // Luminance in shader is 0..1, but texels are 0..255-ish in 8-bit images.
    // Using 0..255 here is more intuitive; viewer normalizes in shader.
    AtlasSlider* luminanceSlider = new AtlasSlider(Qt::Horizontal, 0, 255);
    luminanceSlider->setValue(13);
    layout->addWidget(luminanceSlider);

    auto luminanceFeatherLabel = new QLabel("Luminance Feather: 0 (0.000)");
    layout->addWidget(luminanceFeatherLabel);
    AtlasSlider* featherSlider = new AtlasSlider(Qt::Horizontal, 0, 255);
    featherSlider->setValue(5);
    layout->addWidget(featherSlider);

    auto brightnessLabel = new QLabel("Brightness:");
    layout->addWidget(brightnessLabel);
    AtlasSlider* brightnessSlider = new AtlasSlider(Qt::Horizontal, 0, 200);
    brightnessSlider->setValue(100);
    layout->addWidget(brightnessSlider);

    connect(slider, &QSlider::valueChanged, this, [label2](int value){
        m_logger.Log(AtlasLogger::LogLevel::Info, "Slider adjusted in GUI. New value: {}", value);
        double opacity_value = value / 100.0;
        label2->setText(QString("Opacity: %1").arg(opacity_value));
        auto messenger = &AtlasMessenger::Messenger::Instance();
        messenger->UpdateState(AtlasCommon::AtlasImageViewerState::SliderUpdated, AtlasCommon::AtlasClasses::AtlasImageViewer, value);
    });

    connect(luminanceSlider, &QSlider::valueChanged, this, [this, luminanceThresholdLabel](int value){
      m_logger.Log(AtlasLogger::LogLevel::Info, "Luminance Slider adjusted in GUI. New value: {}", value);
      const double normalized = static_cast<double>(value) / 255.0;
      luminanceThresholdLabel->setText(QString("Luminance Threshold: %1 (%2)")
        .arg(value)
        .arg(normalized, 0, 'f', 3));
      auto messenger = &AtlasMessenger::Messenger::Instance();
      messenger->UpdateState(AtlasCommon::AtlasImageViewerState::LuminanceThresholdUpdated, AtlasCommon::AtlasClasses::AtlasImageViewer, value);
    });

    connect(featherSlider, &QSlider::valueChanged, this, [this, luminanceFeatherLabel](int value){
      m_logger.Log(AtlasLogger::LogLevel::Info, "Feather Slider adjusted in GUI. New value: {}", value);
      const double normalized = static_cast<double>(value) / 255.0;
      luminanceFeatherLabel->setText(QString("Luminance Feather: %1 (%2)")
        .arg(value)
        .arg(normalized, 0, 'f', 3));
      auto messenger = &AtlasMessenger::Messenger::Instance();
      messenger->UpdateState(AtlasCommon::AtlasImageViewerState::LuminanceFeatherUpdated, AtlasCommon::AtlasClasses::AtlasImageViewer, value);
    });

    connect(brightnessSlider, &QSlider::valueChanged, this, [this](int value){
        m_logger.Log(AtlasLogger::LogLevel::Info, "Brightness Slider adjusted in GUI. New value: {}", value);
        auto messenger = &AtlasMessenger::Messenger::Instance();
        messenger->UpdateState(AtlasCommon::AtlasImageViewerState::BrightnessUpdated, AtlasCommon::AtlasClasses::AtlasImageViewer, value);
    });

    auto rotateButton = new QPushButton("Rotate Image");
    layout->addWidget(rotateButton);
    QObject::connect(rotateButton, &QPushButton::clicked, []() {
        m_logger.Log(AtlasLogger::LogLevel::Info, "Rotate button clicked in GUI.");
        auto messenger = &AtlasMessenger::Messenger::Instance();
        messenger->UpdateState(AtlasCommon::AtlasImageViewerState::RotateImage, AtlasCommon::AtlasClasses::AtlasImageViewer);
    });
    auto resetButton = new QPushButton("Reset Rotation");
    layout->addWidget(resetButton);
    QObject::connect(resetButton, &QPushButton::clicked, []() {
      m_logger.Log(AtlasLogger::LogLevel::Info, "Reset Rotation button clicked in GUI.");
      //auto messenger = &AtlasMessenger::Messenger::Instance();
      //messenger->SendMessage("ResetImage,", AtlasCommon::AtlasClasses::AtlasImageViewer);
    });
    m_renderingOptionsWidget->setEnabled(false); // Disabled until Images are loaded.
    this->addWidget(m_renderingOptionsWidget);
  }
   
  // TODO: Create a signal that will connect to a slot in the AtlasImageViewer to change images.
  auto AtlasControlWidgetLayout::BuildImageNavigationWidget() -> void
  {
    m_imageNavigationWidget = new QWidget{};
    auto layout = new QVBoxLayout{m_imageNavigationWidget};
    auto label = new QLabel{"Image Navigation"};
    nextButton = new QPushButton{"Next Image"};
    prevButton = new QPushButton{"Previous Image"};
    layout->addWidget(label);
    layout->addWidget(nextButton);
    layout->addWidget(prevButton);
    nextButton->setEnabled(false);
    prevButton->setEnabled(false);
    this->addWidget(m_imageNavigationWidget);

    auto lineEdit = new QLineEdit{};
    auto messenger = &AtlasMessenger::Messenger::Instance();

    // Next Button and Prev Button Handling
    QObject::connect(nextButton, &QPushButton::clicked, [lineEdit, messenger](){
      m_logger.Log(AtlasLogger::LogLevel::Info, "Next Image button clicked in GUI.");
      messenger->UpdateState(AtlasCommon::AtlasImageViewerState::NextImage, AtlasCommon::AtlasClasses::AtlasImageViewer);
    });

    QObject::connect(prevButton, &QPushButton::clicked, [lineEdit, messenger]() {
       m_logger.Log(AtlasLogger::LogLevel::Info, "Previous Image button clicked in GUI.");
       messenger->UpdateState(AtlasCommon::AtlasImageViewerState::PreviousImage, AtlasCommon::AtlasClasses::AtlasImageViewer);
    });

  }


  auto AtlasControlWidgetLayout::BuildSaveImageWidget() -> void {
      m_saveImageWidget = new QWidget{};
      auto layout = new QVBoxLayout{m_saveImageWidget};
      auto label = new QLabel{"Save Image"};
      auto saveButton = new QPushButton{"Save Image"};
      layout->addWidget(label);
      layout->addWidget(saveButton);
      QObject::connect(saveButton, &QPushButton::clicked, []() {
        m_logger.Log(AtlasLogger::LogLevel::Info, "Save Image button clicked in GUI.");
        //auto messenger = &AtlasMessenger::Messenger::Instance();
        //messenger->SendMessage("SaveImage,", AtlasCommon::AtlasClasses::AtlasImageViewer);
      });
      this->addWidget(m_saveImageWidget);
  }

  auto AtlasControlWidgetLayout::LoadModel() -> void
  {
    auto comboBox = m_ratModelWidget->findChild<QComboBox*>();
    const auto dataSet = comboBox->currentText().toStdString();
    const auto dataSetEnum = AtlasCommon::AtlasDataSetNames.at(dataSet);

    auto messenger = &AtlasMessenger::Messenger::Instance();

    if(dataSetEnum == AtlasCommon::AtlasDataSet::LGN)
    {
      m_logger.Log(AtlasLogger::LogLevel::Info, "Loading LGN Rat Brain Model");
      messenger->UpdateState(AtlasCommon::AtlasModelState::LoadLGNModel, AtlasCommon::AtlasClasses::AtlasModel);
      m_isModelLoaded = true;
    }
    else if(dataSetEnum == AtlasCommon::AtlasDataSet::PAG)
    {
      m_logger.Log(AtlasLogger::LogLevel::Info, "Loading PAG Rat Brain Model");
      messenger->UpdateState(AtlasCommon::AtlasModelState::LoadPAGModel, AtlasCommon::AtlasClasses::AtlasModel);
      m_isModelLoaded = true;
    }
    else
      m_logger.Log(AtlasLogger::LogLevel::Error, "Unknown Rat Brain Model selected: {}", dataSet);
    
  }

}