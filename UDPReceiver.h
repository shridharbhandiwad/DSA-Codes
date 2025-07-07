#pragma once

#include <QObject>
#include <QUdpSocket>
#include <QTimer>
#include <QByteArray>
#include <QHostAddress>
#include <memory>
#include "DataStructures.h"

class UDPReceiver : public QObject
{
    Q_OBJECT

public:
    explicit UDPReceiver(quint16 port = 5000, QObject *parent = nullptr);
    ~UDPReceiver();

    bool start();
    void stop();
    bool isRunning() const { return m_running; }

    // Enable simulation mode for testing
    void setSimulationMode(bool enabled);
    bool isSimulationMode() const { return m_simulationMode; }

signals:
    void targetTrackDataReceived(const TargetTrackData& data);
    void rawADCFrameReceived(const RawADCFrame& frame);
    void errorOccurred(const QString& error);

private slots:
    void processPendingDatagrams();
    void generateSimulatedData();

private:
    void parseTargetTrackData(const QByteArray& data);
    void parseRawADCFrame(const QByteArray& data);
    
    // Simulation data generators
    TargetTrackData generateSimulatedTargetTracks();
    RawADCFrame generateSimulatedADCFrame();

    std::unique_ptr<QUdpSocket> m_socket;
    std::unique_ptr<QTimer> m_simulationTimer;
    quint16 m_port;
    bool m_running;
    bool m_simulationMode;
    
    // Simulation state
    uint32_t m_simulationFrameNumber;
    uint32_t m_simulationTargetIdCounter;
};