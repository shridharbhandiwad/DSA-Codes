#include "MainWindow.h"
#include <QApplication>
#include <QMessageBox>
#include <QCloseEvent>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QDateTime>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_frameCount(0)
    , m_targetCount(0)
    , m_lastUpdateTime(QDateTime::currentMSecsSinceEpoch())
{
    setWindowTitle("Radar Visualization Application");
    setMinimumSize(1200, 800);
    resize(1600, 1000);

    setupUI();
    setupMenuBar();
    setupStatusBar();
    setupConnections();

    // Initialize UDP receiver
    m_udpReceiver = std::make_unique<UDPReceiver>(5000, this);
    
    // Connect UDP receiver signals
    connect(m_udpReceiver.get(), &UDPReceiver::targetTrackDataReceived,
            this, &MainWindow::onTargetTrackDataReceived);
    connect(m_udpReceiver.get(), &UDPReceiver::rawADCFrameReceived,
            this, &MainWindow::onRawADCFrameReceived);
    connect(m_udpReceiver.get(), &UDPReceiver::errorOccurred,
            this, &MainWindow::onUDPError);

    // Start statistics timer
    m_statisticsTimer = new QTimer(this);
    connect(m_statisticsTimer, &QTimer::timeout, this, &MainWindow::updateStatusBar);
    m_statisticsTimer->start(1000); // Update every second

    // Start in simulation mode by default
    m_udpReceiver->setSimulationMode(true);
    m_udpReceiver->start();
    m_simulationButton->setText("Stop Simulation");
    m_startStopButton->setText("Stop UDP");
}

void MainWindow::setupUI()
{
    // Create main widget and layout
    auto centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // Create main splitter (horizontal)
    m_mainSplitter = std::make_unique<QSplitter>(Qt::Horizontal, this);

    // Create left splitter (vertical) for plots and table
    m_leftSplitter = std::make_unique<QSplitter>(Qt::Vertical, this);

    // Create visualization widgets
    m_ppiPlot = std::make_unique<PPIPlot>(this);
    m_fftPlot = std::make_unique<FFTPlot>(this);

    // Create target table
    m_targetTable = std::make_unique<QTableWidget>(this);
    m_targetTable->setColumnCount(8);
    QStringList headers = {"ID", "Level", "Range (m)", "Azimuth (°)", 
                          "Elevation (°)", "Radial Speed (m/s)", 
                          "Az Speed (°/s)", "El Speed (°/s)"};
    m_targetTable->setHorizontalHeaderLabels(headers);
    m_targetTable->horizontalHeader()->setStretchLastSection(true);
    m_targetTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_targetTable->setAlternatingRowColors(true);
    m_targetTable->setMinimumHeight(200);

    // Add plots to left splitter
    m_leftSplitter->addWidget(m_ppiPlot.get());
    m_leftSplitter->addWidget(m_fftPlot.get());
    m_leftSplitter->setSizes({400, 300});

    // Create right panel for controls and table
    auto rightPanel = new QWidget(this);
    auto rightLayout = new QVBoxLayout(rightPanel);

    // Create control groups
    setupControlGroups(rightLayout);

    // Add target table
    rightLayout->addWidget(m_targetTable.get());

    // Add panels to main splitter
    m_mainSplitter->addWidget(m_leftSplitter.get());
    m_mainSplitter->addWidget(rightPanel);
    m_mainSplitter->setSizes({1000, 400});

    // Set main layout
    auto mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->addWidget(m_mainSplitter.get());
    mainLayout->setContentsMargins(5, 5, 5, 5);
}

void MainWindow::setupControlGroups(QVBoxLayout* rightLayout)
{
    // Main control group
    m_controlGroup = std::make_unique<QGroupBox>("System Control", this);
    auto controlLayout = new QVBoxLayout(m_controlGroup.get());

    m_startStopButton = std::make_unique<QPushButton>("Start UDP", this);
    m_simulationButton = std::make_unique<QPushButton>("Start Simulation", this);
    m_clearButton = std::make_unique<QPushButton>("Clear Data", this);

    controlLayout->addWidget(m_startStopButton.get());
    controlLayout->addWidget(m_simulationButton.get());
    controlLayout->addWidget(m_clearButton.get());

    // PPI control group
    m_ppiControlGroup = std::make_unique<QGroupBox>("PPI Display", this);
    auto ppiLayout = new QGridLayout(m_ppiControlGroup.get());

    ppiLayout->addWidget(new QLabel("Max Range (m):", this), 0, 0);
    m_maxRangeSpinBox = std::make_unique<QSpinBox>(this);
    m_maxRangeSpinBox->setRange(100, 50000);
    m_maxRangeSpinBox->setValue(5000);
    m_maxRangeSpinBox->setSingleStep(500);
    ppiLayout->addWidget(m_maxRangeSpinBox.get(), 0, 1);

    m_showGridCheckBox = std::make_unique<QCheckBox>("Show Grid", this);
    m_showGridCheckBox->setChecked(true);
    ppiLayout->addWidget(m_showGridCheckBox.get(), 1, 0);

    m_showRangeRingsCheckBox = std::make_unique<QCheckBox>("Show Range Rings", this);
    m_showRangeRingsCheckBox->setChecked(true);
    ppiLayout->addWidget(m_showRangeRingsCheckBox.get(), 1, 1);

    // FFT control group
    m_fftControlGroup = std::make_unique<QGroupBox>("FFT Display", this);
    auto fftLayout = new QVBoxLayout(m_fftControlGroup.get());

    m_logScaleCheckBox = std::make_unique<QCheckBox>("Log Scale", this);
    m_logScaleCheckBox->setChecked(true);
    fftLayout->addWidget(m_logScaleCheckBox.get());

    // Status group
    m_statusGroup = std::make_unique<QGroupBox>("Status", this);
    auto statusLayout = new QGridLayout(m_statusGroup.get());

    statusLayout->addWidget(new QLabel("Connection:", this), 0, 0);
    m_connectionStatusLabel = std::make_unique<QLabel>("Disconnected", this);
    statusLayout->addWidget(m_connectionStatusLabel.get(), 0, 1);

    statusLayout->addWidget(new QLabel("Frames:", this), 1, 0);
    m_frameCountLabel = std::make_unique<QLabel>("0", this);
    statusLayout->addWidget(m_frameCountLabel.get(), 1, 1);

    statusLayout->addWidget(new QLabel("Targets:", this), 2, 0);
    m_targetCountLabel = std::make_unique<QLabel>("0", this);
    statusLayout->addWidget(m_targetCountLabel.get(), 2, 1);

    statusLayout->addWidget(new QLabel("Data Rate:", this), 3, 0);
    m_dataRateLabel = std::make_unique<QLabel>("0 Hz", this);
    statusLayout->addWidget(m_dataRateLabel.get(), 3, 1);

    // Add all groups to right layout
    rightLayout->addWidget(m_controlGroup.get());
    rightLayout->addWidget(m_ppiControlGroup.get());
    rightLayout->addWidget(m_fftControlGroup.get());
    rightLayout->addWidget(m_statusGroup.get());
    rightLayout->addStretch();
}

void MainWindow::setupMenuBar()
{
    auto fileMenu = menuBar()->addMenu("&File");
    
    m_exitAction = std::make_unique<QAction>("E&xit", this);
    m_exitAction->setShortcut(QKeySequence::Quit);
    m_exitAction->setStatusTip("Exit the application");
    connect(m_exitAction.get(), &QAction::triggered, this, &QWidget::close);
    fileMenu->addAction(m_exitAction.get());

    auto helpMenu = menuBar()->addMenu("&Help");
    
    m_aboutAction = std::make_unique<QAction>("&About", this);
    m_aboutAction->setStatusTip("Show information about the application");
    connect(m_aboutAction.get(), &QAction::triggered, this, &MainWindow::showAbout);
    helpMenu->addAction(m_aboutAction.get());

    m_aboutQtAction = std::make_unique<QAction>("About &Qt", this);
    m_aboutQtAction->setStatusTip("Show information about Qt");
    connect(m_aboutQtAction.get(), &QAction::triggered, qApp, &QApplication::aboutQt);
    helpMenu->addAction(m_aboutQtAction.get());
}

void MainWindow::setupStatusBar()
{
    statusBar()->showMessage("Ready");
}

void MainWindow::setupConnections()
{
    // Control button connections
    connect(m_startStopButton.get(), &QPushButton::clicked,
            this, &MainWindow::toggleUDPReceiver);
    connect(m_simulationButton.get(), &QPushButton::clicked,
            this, &MainWindow::toggleSimulationMode);
    connect(m_clearButton.get(), &QPushButton::clicked,
            this, &MainWindow::clearAllData);

    // PPI plot connections
    connect(m_ppiPlot.get(), &PPIPlot::targetSelected,
            this, &MainWindow::onTargetSelected);
    connect(m_ppiPlot.get(), &PPIPlot::targetHovered,
            this, &MainWindow::onTargetHovered);

    // FFT plot connections
    connect(m_fftPlot.get(), &FFTPlot::frequencySelected,
            this, &MainWindow::onFrequencySelected);
    connect(m_fftPlot.get(), &FFTPlot::magnitudeAtFrequency,
            this, &MainWindow::onMagnitudeAtFrequency);

    // Settings connections
    connect(m_maxRangeSpinBox.get(), QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MainWindow::onMaxRangeChanged);
    connect(m_showGridCheckBox.get(), &QCheckBox::toggled,
            this, &MainWindow::onShowGridToggled);
    connect(m_showRangeRingsCheckBox.get(), &QCheckBox::toggled,
            this, &MainWindow::onShowRangeRingsToggled);
    connect(m_logScaleCheckBox.get(), &QCheckBox::toggled,
            this, &MainWindow::onLogScaleToggled);
}

void MainWindow::onTargetTrackDataReceived(const TargetTrackData& data)
{
    m_ppiPlot->updateTargetTracks(data);
    updateTargetTable(data);
    m_targetCount = data.numTracks;
}

void MainWindow::onRawADCFrameReceived(const RawADCFrame& frame)
{
    m_fftPlot->updateFFTData(frame);
    m_frameCount++;
}

void MainWindow::onUDPError(const QString& error)
{
    statusBar()->showMessage(QString("UDP Error: %1").arg(error), 5000);
    qWarning() << "UDP Error:" << error;
}

void MainWindow::onTargetSelected(uint32_t targetId)
{
    // Highlight selected target in table
    for (int row = 0; row < m_targetTable->rowCount(); ++row) {
        auto item = m_targetTable->item(row, 0);
        if (item && item->text().toUInt() == targetId) {
            m_targetTable->selectRow(row);
            break;
        }
    }
    statusBar()->showMessage(QString("Selected target ID: %1").arg(targetId), 2000);
}

void MainWindow::onTargetHovered(uint32_t targetId, const QPoint& position)
{
    Q_UNUSED(position)
    statusBar()->showMessage(QString("Hovering over target ID: %1").arg(targetId), 1000);
}

void MainWindow::onFrequencySelected(float frequency)
{
    statusBar()->showMessage(QString("Selected frequency: %1 kHz").arg(frequency / 1000.0, 0, 'f', 1), 2000);
}

void MainWindow::onMagnitudeAtFrequency(float frequency, float magnitude)
{
    statusBar()->showMessage(QString("Freq: %1 kHz, Mag: %2 dB")
                           .arg(frequency / 1000.0, 0, 'f', 1)
                           .arg(magnitude, 0, 'f', 1), 500);
}

void MainWindow::toggleUDPReceiver()
{
    if (m_udpReceiver->isRunning()) {
        m_udpReceiver->stop();
        m_startStopButton->setText("Start UDP");
        m_connectionStatusLabel->setText("Stopped");
    } else {
        if (m_udpReceiver->start()) {
            m_startStopButton->setText("Stop UDP");
            if (m_udpReceiver->isSimulationMode()) {
                m_connectionStatusLabel->setText("Simulation Mode");
            } else {
                m_connectionStatusLabel->setText("Listening on Port 5000");
            }
        }
    }
}

void MainWindow::toggleSimulationMode()
{
    bool wasRunning = m_udpReceiver->isRunning();
    
    if (m_udpReceiver->isSimulationMode()) {
        m_udpReceiver->setSimulationMode(false);
        m_simulationButton->setText("Start Simulation");
        if (wasRunning) {
            m_connectionStatusLabel->setText("Listening on Port 5000");
        }
    } else {
        m_udpReceiver->setSimulationMode(true);
        m_simulationButton->setText("Stop Simulation");
        if (wasRunning) {
            m_connectionStatusLabel->setText("Simulation Mode");
        }
    }
}

void MainWindow::clearAllData()
{
    m_ppiPlot->clearTracks();
    m_fftPlot->clearPlot();
    m_targetTable->setRowCount(0);
    m_frameCount = 0;
    m_targetCount = 0;
    statusBar()->showMessage("Data cleared", 2000);
}

void MainWindow::showAbout()
{
    QMessageBox::about(this, "About Radar Visualization",
                      "Radar Visualization Application\n\n"
                      "A Qt C++ application for real-time radar data visualization.\n"
                      "Features:\n"
                      "• Semi-circular PPI plot for target tracks\n"
                      "• Real-time FFT spectrum display\n"
                      "• UDP data reception on port 5000\n"
                      "• Simulation mode for testing\n\n"
                      "Built with Qt and modern C++ practices.");
}

void MainWindow::onMaxRangeChanged(int range)
{
    m_ppiPlot->setMaxRange(static_cast<float>(range));
}

void MainWindow::onShowGridToggled(bool show)
{
    m_ppiPlot->setShowGrid(show);
    m_fftPlot->setShowGrid(show);
}

void MainWindow::onShowRangeRingsToggled(bool show)
{
    m_ppiPlot->setShowRangeRings(show);
}

void MainWindow::onLogScaleToggled(bool logScale)
{
    m_fftPlot->setLogScale(logScale);
}

void MainWindow::updateTargetTable(const TargetTrackData& data)
{
    m_targetTable->setRowCount(static_cast<int>(data.numTracks));
    
    for (size_t i = 0; i < data.targets.size(); ++i) {
        const auto& track = data.targets[i];
        int row = static_cast<int>(i);
        
        m_targetTable->setItem(row, 0, new QTableWidgetItem(QString::number(track.target_id)));
        m_targetTable->setItem(row, 1, new QTableWidgetItem(QString::number(track.level, 'f', 1)));
        m_targetTable->setItem(row, 2, new QTableWidgetItem(QString::number(track.radius, 'f', 1)));
        m_targetTable->setItem(row, 3, new QTableWidgetItem(QString::number(track.azimuth, 'f', 1)));
        m_targetTable->setItem(row, 4, new QTableWidgetItem(QString::number(track.elevation, 'f', 1)));
        m_targetTable->setItem(row, 5, new QTableWidgetItem(QString::number(track.radial_speed, 'f', 1)));
        m_targetTable->setItem(row, 6, new QTableWidgetItem(QString::number(track.azimuth_speed, 'f', 1)));
        m_targetTable->setItem(row, 7, new QTableWidgetItem(QString::number(track.elevation_speed, 'f', 1)));
        
        // Color code based on radial speed
        QColor rowColor;
        if (std::abs(track.radial_speed) < 1.0f) {
            rowColor = QColor(220, 220, 255); // Light blue for stationary
        } else if (track.radial_speed > 0) {
            rowColor = QColor(255, 220, 220); // Light red for approaching
        } else {
            rowColor = QColor(220, 255, 220); // Light green for receding
        }
        
        for (int col = 0; col < 8; ++col) {
            if (auto item = m_targetTable->item(row, col)) {
                item->setBackground(rowColor);
            }
        }
    }
}

void MainWindow::updateStatusBar()
{
    m_frameCountLabel->setText(QString::number(m_frameCount));
    m_targetCountLabel->setText(QString::number(m_targetCount));
    
    // Calculate data rate
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    qint64 timeDiff = currentTime - m_lastUpdateTime;
    if (timeDiff > 0) {
        float dataRate = 1000.0f / timeDiff; // Approximate rate based on update interval
        m_dataRateLabel->setText(QString("%1 Hz").arg(dataRate, 0, 'f', 1));
    }
    m_lastUpdateTime = currentTime;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_udpReceiver && m_udpReceiver->isRunning()) {
        m_udpReceiver->stop();
    }
    event->accept();
}

#include "MainWindow.moc"