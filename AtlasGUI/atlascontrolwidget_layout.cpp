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

namespace AtlasGUI
{
  static AtlasLogger::Logger m_logger{
    QStandardPaths::standardLocations(QStandardPaths::AppConfigLocation).at(1).toStdString() + 
    "/Atlas-Imager/Logs/" + AtlasLogger::GetCurrentDateString() + "/AtlasControlWidgetLayout.log", "AtlasGUI::AtlasControlWidgetLayout"
  };

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
    
    QObject::connect(browseButton, &QPushButton::clicked, [lineEdit, renderImageButton, layout]()
    {
      auto path = QFileDialog::getOpenFileName(
        layout->parentWidget(),
        tr("Select Image"),
        QDir::homePath() + "/Documents",
        tr("Image Files (*.png *.jpg *.bmp *.tif);;All Files (*)")
      );
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
    // TODO: Flesh out the rendering options widget
    m_renderingOptionsWidget = new QWidget{};
    auto layout = new QVBoxLayout{m_renderingOptionsWidget};
    auto label = new QLabel("Rendering Options\n");
    layout->addWidget(label);

    // Create slider
    auto opacityLabel = new QLabel("Opacity: ");
    layout->addWidget(opacityLabel);
    auto opacitySlider = new AtlasSlider(Qt::Horizontal, 0, 100);
    opacitySlider->setValue(75);
    layout->addWidget(opacitySlider);

    auto rotationLabel = new QLabel("Rotation: ");
    layout->addWidget(rotationLabel);
    auto rotationSlider = new AtlasSlider(Qt::Horizontal, -180000, 180000);
    rotationSlider->setValue(0);
    layout->addWidget(rotationSlider);

    auto scaleLabel = new QLabel("Scale: ");
    layout->addWidget(scaleLabel);
    auto scaleSlider = new AtlasSlider(Qt::Horizontal, 1, 300);
    scaleSlider->setValue(100);
    layout->addWidget(scaleSlider);

    connect(opacitySlider, &QSlider::valueChanged, this, [opacityLabel](int value){
      m_logger.Log(AtlasLogger::LogLevel::Info, "Slider adjusted in GUI. New value: {}", value);
      const double opacity_value = value / 100.0;
      opacityLabel->setText(QString("Opacity: %1").arg(opacity_value));
      auto messenger = &AtlasMessenger::Messenger::Instance();
      messenger->UpdateState(AtlasCommon::AtlasImageViewerState::SliderUpdated, AtlasCommon::AtlasClasses::AtlasImageViewer, value);
    });

    connect(rotationSlider, &QSlider::valueChanged, this, [rotationLabel](int value){
      m_logger.Log(AtlasLogger::LogLevel::Info, "Rotation slider adjusted in GUI. New value: {}", value);
      rotationLabel->setText(QString("Rotation: %1°").arg(value / 1000.0));
      auto messenger = &AtlasMessenger::Messenger::Instance();
      messenger->UpdateState(AtlasCommon::AtlasImageViewerState::RotateImage, AtlasCommon::AtlasClasses::AtlasImageViewer, value);
    });

    connect(scaleSlider, &QSlider::valueChanged, this, [scaleLabel](int value){
      m_logger.Log(AtlasLogger::LogLevel::Info, "Scale slider adjusted in GUI. New value: {}", value);
      const double scale_value = value / 100.0;
      scaleLabel->setText(QString("Scale: %1x").arg(scale_value));
      auto messenger = &AtlasMessenger::Messenger::Instance();
      messenger->UpdateState(AtlasCommon::AtlasImageViewerState::ScaleImage, AtlasCommon::AtlasClasses::AtlasImageViewer, value);
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
      auto messenger = &AtlasMessenger::Messenger::Instance();
      messenger->UpdateState(AtlasCommon::AtlasImageViewerState::SaveCurrentImage, AtlasCommon::AtlasClasses::AtlasImageViewer);
    });

    this->addWidget(m_saveImageWidget);
  }

  auto AtlasControlWidgetLayout::LoadModel() -> void
  {
    auto comboBox = m_ratModelWidget->findChild<QComboBox*>();
    auto label = m_ratModelWidget->findChild<QLabel*>();
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
    
    if(m_isModelLoaded)
    {
      label->setText(comboBox->currentText() + " Model Loaded");
      label->setStyleSheet("QLabel { color : green; }");
    }
  }

}