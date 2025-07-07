#include "UDPReceiver.h"
#include <QDebug>
#include <QDataStream>
#include <QRandomGenerator>
#include <cmath>

UDPReceiver::UDPReceiver(quint16 port, QObject *parent)
    : QObject(parent)
    , m_socket(std::make_unique<QUdpSocket>(this))
    , m_simulationTimer(std::make_unique<QTimer>(this))
    , m_port(port)
    , m_running(false)
    , m_simulationMode(true) // Default to simulation for testing
    , m_simulationFrameNumber(0)
    , m_simulationTargetIdCounter(1000)
{
    connect(m_socket.get(), &QUdpSocket::readyRead, 
            this, &UDPReceiver::processPendingDatagrams);
    
    connect(m_simulationTimer.get(), &QTimer::timeout,
            this, &UDPReceiver::generateSimulatedData);
    
    // Set simulation timer to 50ms
    m_simulationTimer->setInterval(50);
}

UDPReceiver::~UDPReceiver()
{
    stop();
}

bool UDPReceiver::start()
{
    if (m_running) {
        return true;
    }

    if (m_simulationMode) {
        m_simulationTimer->start();
        m_running = true;
        qDebug() << "UDP Receiver started in simulation mode";
        return true;
    }

    if (!m_socket->bind(QHostAddress::Any, m_port)) {
        emit errorOccurred(QString("Failed to bind to port %1: %2")
                          .arg(m_port).arg(m_socket->errorString()));
        return false;
    }

    m_running = true;
    qDebug() << "UDP Receiver started on port" << m_port;
    return true;
}

void UDPReceiver::stop()
{
    if (!m_running) {
        return;
    }

    m_simulationTimer->stop();
    m_socket->close();
    m_running = false;
    qDebug() << "UDP Receiver stopped";
}

void UDPReceiver::setSimulationMode(bool enabled)
{
    if (m_simulationMode == enabled) {
        return;
    }

    bool wasRunning = m_running;
    if (wasRunning) {
        stop();
    }

    m_simulationMode = enabled;

    if (wasRunning) {
        start();
    }
}

void UDPReceiver::processPendingDatagrams()
{
    while (m_socket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(static_cast<int>(m_socket->pendingDatagramSize()));
        
        QHostAddress sender;
        quint16 senderPort;
        
        qint64 size = m_socket->readDatagram(datagram.data(), datagram.size(),
                                           &sender, &senderPort);
        
        if (size < 0) {
            emit errorOccurred("Error reading UDP datagram");
            continue;
        }

        // Parse message header
        if (datagram.size() < static_cast<int>(sizeof(MessageHeader))) {
            qWarning() << "Received datagram too small for header";
            continue;
        }

        QDataStream stream(datagram);
        stream.setByteOrder(QDataStream::LittleEndian);
        
        MessageHeader header;
        stream >> reinterpret_cast<quint8&>(header.type);
        stream >> header.data_size;
        stream >> header.timestamp;

        QByteArray payload = datagram.mid(sizeof(MessageHeader));

        switch (header.type) {
        case MessageType::TARGET_TRACK_DATA:
            parseTargetTrackData(payload);
            break;
        case MessageType::RAW_ADC_FRAME:
            parseRawADCFrame(payload);
            break;
        default:
            qWarning() << "Unknown message type:" << static_cast<int>(header.type);
            break;
        }
    }
}

void UDPReceiver::generateSimulatedData()
{
    static int counter = 0;
    
    // Alternate between target track data and ADC frame data
    if (counter % 2 == 0) {
        TargetTrackData trackData = generateSimulatedTargetTracks();
        emit targetTrackDataReceived(trackData);
    } else {
        RawADCFrame adcFrame = generateSimulatedADCFrame();
        emit rawADCFrameReceived(adcFrame);
    }
    
    counter++;
}

void UDPReceiver::parseTargetTrackData(const QByteArray& data)
{
    QDataStream stream(data);
    stream.setByteOrder(QDataStream::LittleEndian);
    
    TargetTrackData trackData;
    stream >> trackData.numTracks;
    
    trackData.targets.resize(trackData.numTracks);
    
    for (uint32_t i = 0; i < trackData.numTracks; ++i) {
        TargetTrack& track = trackData.targets[i];
        stream >> track.target_id;
        stream >> track.level;
        stream >> track.radius;
        stream >> track.azimuth;
        stream >> track.elevation;
        stream >> track.radial_speed;
        stream >> track.azimuth_speed;
        stream >> track.elevation_speed;
    }
    
    emit targetTrackDataReceived(trackData);
}

void UDPReceiver::parseRawADCFrame(const QByteArray& data)
{
    QDataStream stream(data);
    stream.setByteOrder(QDataStream::LittleEndian);
    
    RawADCFrame frame;
    stream >> frame.frame_number;
    stream >> frame.num_chirps;
    stream >> frame.num_rx_antennas;
    stream >> frame.num_samples_per_chirp;
    stream >> frame.rx_mask;
    stream >> frame.adc_resolution;
    stream >> frame.interleaved_rx;
    
    quint8 dataFormat;
    stream >> dataFormat;
    frame.data_format = static_cast<Rx_Data_Format_t>(dataFormat);
    
    uint32_t sampleCount;
    stream >> sampleCount;
    
    frame.sample_data.resize(sampleCount);
    for (uint32_t i = 0; i < sampleCount; ++i) {
        stream >> frame.sample_data[i];
    }
    
    emit rawADCFrameReceived(frame);
}

TargetTrackData UDPReceiver::generateSimulatedTargetTracks()
{
    TargetTrackData trackData;
    
    // Generate 1-5 random targets
    int numTargets = QRandomGenerator::global()->bounded(1, 6);
    
    for (int i = 0; i < numTargets; ++i) {
        TargetTrack track;
        track.target_id = m_simulationTargetIdCounter++;
        track.level = QRandomGenerator::global()->bounded(20.0f, 80.0f);
        track.radius = QRandomGenerator::global()->bounded(100.0f, 5000.0f);
        track.azimuth = QRandomGenerator::global()->bounded(-90.0f, 90.0f);
        track.elevation = QRandomGenerator::global()->bounded(-45.0f, 45.0f);
        track.radial_speed = QRandomGenerator::global()->bounded(-50.0f, 50.0f);
        track.azimuth_speed = QRandomGenerator::global()->bounded(-10.0f, 10.0f);
        track.elevation_speed = QRandomGenerator::global()->bounded(-5.0f, 5.0f);
        
        trackData.addTarget(track);
    }
    
    return trackData;
}

RawADCFrame UDPReceiver::generateSimulatedADCFrame()
{
    RawADCFrame frame;
    frame.frame_number = m_simulationFrameNumber++;
    frame.num_chirps = 128;
    frame.num_rx_antennas = 4;
    frame.num_samples_per_chirp = 512;
    frame.rx_mask = 0x0F;
    frame.adc_resolution = 16;
    frame.interleaved_rx = 0;
    frame.data_format = Rx_Data_Format_t::COMPLEX_2x16BIT;
    
    // Generate complex sinusoidal signals with noise
    uint32_t totalSamples = frame.num_chirps * frame.num_rx_antennas * frame.num_samples_per_chirp * 2; // *2 for I/Q
    frame.sample_data.resize(totalSamples);
    
    for (uint32_t i = 0; i < totalSamples; i += 2) {
        float t = static_cast<float>(i) / 2.0f;
        
        // Generate multiple frequency components
        float signal = 0.0f;
        signal += 0.5f * std::sin(2.0f * M_PI * 0.1f * t);  // Low frequency
        signal += 0.3f * std::sin(2.0f * M_PI * 0.05f * t); // Very low frequency
        signal += 0.2f * std::sin(2.0f * M_PI * 0.2f * t);  // Higher frequency
        
        // Add noise
        float noise_i = QRandomGenerator::global()->bounded(-0.1f, 0.1f);
        float noise_q = QRandomGenerator::global()->bounded(-0.1f, 0.1f);
        
        frame.sample_data[i] = signal + noise_i;     // I component
        frame.sample_data[i + 1] = signal + noise_q; // Q component
    }
    
    return frame;
}

#include "UDPReceiver.moc"