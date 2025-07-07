#pragma once

#include <QMainWindow>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QSpinBox>
#include <QGroupBox>
#include <QStatusBar>
#include <QTimer>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <memory>

#include "PPIPlot.h"
#include "FFTPlot.h"
#include "UDPReceiver.h"
#include "DataStructures.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void onTargetTrackDataReceived(const TargetTrackData& data);
    void onRawADCFrameReceived(const RawADCFrame& frame);
    void onUDPError(const QString& error);
    
    void onTargetSelected(uint32_t targetId);
    void onTargetHovered(uint32_t targetId, const QPoint& position);
    void onFrequencySelected(float frequency);
    void onMagnitudeAtFrequency(float frequency, float magnitude);
    
    // Control actions
    void toggleUDPReceiver();
    void toggleSimulationMode();
    void clearAllData();
    void showAbout();
    
    // Settings changes
    void onMaxRangeChanged(int range);
    void onShowGridToggled(bool show);
    void onShowRangeRingsToggled(bool show);
    void onLogScaleToggled(bool logScale);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void setupUI();
    void setupMenuBar();
    void setupStatusBar();
    void setupConnections();
    void setupControlGroups(QVBoxLayout* rightLayout);
    void updateTargetTable(const TargetTrackData& data);
    void updateStatusBar();

    // Main layout components
    std::unique_ptr<QSplitter> m_mainSplitter;
    std::unique_ptr<QSplitter> m_leftSplitter;
    
    // Visualization widgets
    std::unique_ptr<PPIPlot> m_ppiPlot;
    std::unique_ptr<FFTPlot> m_fftPlot;
    
    // Data display
    std::unique_ptr<QTableWidget> m_targetTable;
    
    // Control panels
    std::unique_ptr<QGroupBox> m_controlGroup;
    std::unique_ptr<QGroupBox> m_ppiControlGroup;
    std::unique_ptr<QGroupBox> m_fftControlGroup;
    std::unique_ptr<QGroupBox> m_statusGroup;
    
    // Controls
    std::unique_ptr<QPushButton> m_startStopButton;
    std::unique_ptr<QPushButton> m_simulationButton;
    std::unique_ptr<QPushButton> m_clearButton;
    
    std::unique_ptr<QSpinBox> m_maxRangeSpinBox;
    std::unique_ptr<QCheckBox> m_showGridCheckBox;
    std::unique_ptr<QCheckBox> m_showRangeRingsCheckBox;
    std::unique_ptr<QCheckBox> m_logScaleCheckBox;
    
    // Status display
    std::unique_ptr<QLabel> m_connectionStatusLabel;
    std::unique_ptr<QLabel> m_frameCountLabel;
    std::unique_ptr<QLabel> m_targetCountLabel;
    std::unique_ptr<QLabel> m_dataRateLabel;
    
    // Menu and actions
    std::unique_ptr<QAction> m_exitAction;
    std::unique_ptr<QAction> m_aboutAction;
    std::unique_ptr<QAction> m_aboutQtAction;
    
    // Data processing
    std::unique_ptr<UDPReceiver> m_udpReceiver;
    
    // Statistics
    uint32_t m_frameCount;
    uint32_t m_targetCount;
    QTimer* m_statisticsTimer;
    qint64 m_lastUpdateTime;
};