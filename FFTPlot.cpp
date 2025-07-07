#include "FFTPlot.h"
#include <QDebug>
#include <QMouseEvent>
#include <QPen>
#include <QBrush>
#include <QToolTip>
#include <cmath>
#include <algorithm>

FFTPlot::FFTPlot(QWidget *parent)
    : QWidget(parent)
    , m_sampleRate(1000000) // 1 MHz default sample rate
    , m_fftSize(512)
    , m_logScale(true)
    , m_showGrid(true)
    , m_maxMagnitude(100.0f)
    , m_minMagnitude(0.01f)
    , m_marginLeft(80)
    , m_marginRight(20)
    , m_marginTop(30)
    , m_marginBottom(60)
    , m_showCursor(false)
    , m_cursorFrequency(0.0f)
    , m_cursorMagnitude(0.0f)
    , m_currentFrameNumber(0)
{
    setMinimumSize(400, 300);
    setMouseTracking(true);
    
    // Initialize fonts
    m_labelFont = QFont("Arial", 9);
    m_titleFont = QFont("Arial", 11, QFont::Bold);
    
    // Initialize colors
    m_backgroundColor = QColor(15, 15, 25);
    m_gridColor = QColor(60, 60, 80);
    m_spectrumColor = QColor(100, 255, 100);
    m_textColor = QColor(200, 200, 200);
    m_cursorColor = QColor(255, 255, 100);
    
    updatePlotBounds();
}

void FFTPlot::updateFFTData(const RawADCFrame& frame)
{
    m_currentFrameNumber = frame.frame_number;
    
    // Update frame info
    m_frameInfo = QString("Frame: %1, Chirps: %2, RX: %3, Samples: %4")
                 .arg(frame.frame_number)
                 .arg(frame.num_chirps)
                 .arg(frame.num_rx_antennas)
                 .arg(frame.num_samples_per_chirp);
    
    // For simplicity, use the first chirp from the first antenna
    if (!frame.sample_data.empty()) {
        // Extract I/Q samples from the first chirp
        std::vector<float> iSamples, qSamples;
        uint32_t samplesPerChirp = frame.num_samples_per_chirp;
        
        // Assuming interleaved I/Q data: I0, Q0, I1, Q1, ...
        for (uint32_t i = 0; i < samplesPerChirp * 2 && i < frame.sample_data.size(); i += 2) {
            iSamples.push_back(frame.sample_data[i]);
            qSamples.push_back(frame.sample_data[i + 1]);
        }
        
        // Combine I and Q into complex samples
        std::vector<float> complexSamples;
        for (size_t i = 0; i < std::min(iSamples.size(), qSamples.size()); ++i) {
            complexSamples.push_back(iSamples[i]);
            complexSamples.push_back(qSamples[i]);
        }
        
        if (!complexSamples.empty()) {
            performFFT(complexSamples);
        }
    }
    
    update();
}

void FFTPlot::clearPlot()
{
    m_magnitudeSpectrum.clear();
    m_frequencies.clear();
    m_currentFrameNumber = 0;
    m_frameInfo.clear();
    update();
}

void FFTPlot::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    drawBackground(painter);
    
    if (m_showGrid) {
        drawGrid(painter);
    }
    
    if (!m_magnitudeSpectrum.empty()) {
        drawSpectrum(painter);
    }
    
    drawLabels(painter);
    
    if (m_showCursor) {
        drawCursor(painter);
    }
}

void FFTPlot::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)
    updatePlotBounds();
    QWidget::resizeEvent(event);
}

void FFTPlot::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_plotArea.contains(event->pos())) {
        float frequency = xToFrequency(event->pos().x());
        emit frequencySelected(frequency);
    }
    QWidget::mousePressEvent(event);
}

void FFTPlot::mouseMoveEvent(QMouseEvent *event)
{
    if (m_plotArea.contains(event->pos())) {
        m_showCursor = true;
        m_cursorPos = event->pos();
        m_cursorFrequency = xToFrequency(event->pos().x());
        m_cursorMagnitude = yToMagnitude(event->pos().y());
        
        emit magnitudeAtFrequency(m_cursorFrequency, m_cursorMagnitude);
        
        // Show tooltip
        QString tooltip = QString("Freq: %1 kHz\nMag: %2 dB")
                         .arg(m_cursorFrequency / 1000.0f, 0, 'f', 1)
                         .arg(m_cursorMagnitude, 0, 'f', 1);
        QToolTip::showText(event->globalPos(), tooltip);
        
        update();
    } else {
        m_showCursor = false;
        QToolTip::hideText();
        update();
    }
    
    QWidget::mouseMoveEvent(event);
}

void FFTPlot::performFFT(const std::vector<float>& inputData)
{
    if (inputData.empty()) {
        return;
    }
    
    // Determine FFT size (power of 2)
    size_t dataSize = inputData.size() / 2; // Complex samples
    m_fftSize = 1;
    while (m_fftSize < dataSize && m_fftSize < 2048) {
        m_fftSize *= 2;
    }
    
    // Prepare complex data for FFT
    std::vector<std::complex<float>> fftData(m_fftSize);
    for (size_t i = 0; i < m_fftSize && (i * 2 + 1) < inputData.size(); ++i) {
        fftData[i] = std::complex<float>(inputData[i * 2], inputData[i * 2 + 1]);
    }
    
    // Apply window function (Hamming)
    for (size_t i = 0; i < m_fftSize; ++i) {
        float window = 0.54f - 0.46f * std::cos(2.0f * M_PI * i / (m_fftSize - 1));
        fftData[i] *= window;
    }
    
    // Compute FFT
    computeFFT(fftData);
    
    // Compute magnitude spectrum
    m_magnitudeSpectrum.clear();
    m_frequencies.clear();
    m_magnitudeSpectrum.reserve(m_fftSize / 2);
    m_frequencies.reserve(m_fftSize / 2);
    
    float freqResolution = static_cast<float>(m_sampleRate) / m_fftSize;
    
    for (size_t i = 0; i < m_fftSize / 2; ++i) {
        float magnitude = std::abs(fftData[i]);
        
        // Convert to dB
        float magnitudeDB = 20.0f * std::log10(std::max(magnitude, 1e-10f));
        
        m_magnitudeSpectrum.push_back(magnitudeDB);
        m_frequencies.push_back(i * freqResolution);
    }
    
    // Auto-scale if needed
    if (!m_magnitudeSpectrum.empty()) {
        auto minMax = std::minmax_element(m_magnitudeSpectrum.begin(), m_magnitudeSpectrum.end());
        m_minMagnitude = *minMax.first - 10.0f;
        m_maxMagnitude = *minMax.second + 10.0f;
    }
}

void FFTPlot::computeFFT(std::vector<std::complex<float>>& data)
{
    size_t n = data.size();
    if (n <= 1) return;
    
    // Simple radix-2 FFT implementation
    // Bit-reverse ordering
    size_t j = 0;
    for (size_t i = 1; i < n; ++i) {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) {
            j ^= bit;
        }
        j ^= bit;
        
        if (i < j) {
            std::swap(data[i], data[j]);
        }
    }
    
    // FFT computation
    for (size_t len = 2; len <= n; len <<= 1) {
        float angle = -2.0f * M_PI / len;
        std::complex<float> wlen(std::cos(angle), std::sin(angle));
        
        for (size_t i = 0; i < n; i += len) {
            std::complex<float> w(1.0f, 0.0f);
            for (size_t j = 0; j < len / 2; ++j) {
                std::complex<float> u = data[i + j];
                std::complex<float> v = data[i + j + len / 2] * w;
                data[i + j] = u + v;
                data[i + j + len / 2] = u - v;
                w *= wlen;
            }
        }
    }
}

void FFTPlot::drawBackground(QPainter& painter)
{
    painter.fillRect(rect(), m_backgroundColor);
    updatePlotBounds();
}

void FFTPlot::drawGrid(QPainter& painter)
{
    painter.setPen(QPen(m_gridColor, 1, Qt::DotLine));
    
    // Vertical grid lines (frequency)
    float maxFreq = m_sampleRate / 2.0f;
    float freqStep = maxFreq / 10.0f;
    
    for (int i = 0; i <= 10; ++i) {
        float freq = i * freqStep;
        float x = frequencyToX(freq);
        painter.drawLine(QPointF(x, m_plotArea.top()), QPointF(x, m_plotArea.bottom()));
    }
    
    // Horizontal grid lines (magnitude)
    float magRange = m_maxMagnitude - m_minMagnitude;
    float magStep = magRange / 8.0f;
    
    for (int i = 0; i <= 8; ++i) {
        float mag = m_minMagnitude + i * magStep;
        float y = magnitudeToY(mag);
        painter.drawLine(QPointF(m_plotArea.left(), y), QPointF(m_plotArea.right(), y));
    }
}

void FFTPlot::drawSpectrum(QPainter& painter)
{
    if (m_magnitudeSpectrum.empty() || m_frequencies.empty()) {
        return;
    }
    
    painter.setPen(QPen(m_spectrumColor, 2));
    
    QPolygonF spectrumPoly;
    
    for (size_t i = 0; i < m_magnitudeSpectrum.size(); ++i) {
        float x = frequencyToX(m_frequencies[i]);
        float y = magnitudeToY(m_magnitudeSpectrum[i]);
        spectrumPoly << QPointF(x, y);
    }
    
    if (!spectrumPoly.isEmpty()) {
        painter.drawPolyline(spectrumPoly);
        
        // Fill under the curve
        painter.setPen(Qt::NoPen);
        QBrush fillBrush(m_spectrumColor);
        fillBrush.setColor(QColor(m_spectrumColor.red(), m_spectrumColor.green(), 
                                 m_spectrumColor.blue(), 50));
        painter.setBrush(fillBrush);
        
        QPolygonF fillPoly = spectrumPoly;
        fillPoly << QPointF(spectrumPoly.last().x(), m_plotArea.bottom());
        fillPoly << QPointF(spectrumPoly.first().x(), m_plotArea.bottom());
        painter.drawPolygon(fillPoly);
    }
}

void FFTPlot::drawLabels(QPainter& painter)
{
    painter.setPen(QPen(m_textColor, 1));
    painter.setFont(m_titleFont);
    
    // Title
    QString title = QString("FFT Magnitude Spectrum - %1").arg(m_frameInfo);
    painter.drawText(QPoint(m_marginLeft, 20), title);
    
    painter.setFont(m_labelFont);
    
    // X-axis labels (frequency)
    float maxFreq = m_sampleRate / 2.0f;
    float freqStep = maxFreq / 10.0f;
    
    for (int i = 0; i <= 10; ++i) {
        float freq = i * freqStep;
        float x = frequencyToX(freq);
        QString freqText = QString("%1").arg(freq / 1000.0f, 0, 'f', 0);
        painter.drawText(QPointF(x - 15, height() - m_marginBottom + 15), freqText);
    }
    
    // X-axis title
    painter.drawText(QPointF(width() / 2 - 30, height() - 10), "Frequency (kHz)");
    
    // Y-axis labels (magnitude)
    float magRange = m_maxMagnitude - m_minMagnitude;
    float magStep = magRange / 8.0f;
    
    for (int i = 0; i <= 8; ++i) {
        float mag = m_minMagnitude + i * magStep;
        float y = magnitudeToY(mag);
        QString magText = QString("%1").arg(mag, 0, 'f', 0);
        painter.drawText(QPointF(10, y + 3), magText);
    }
    
    // Y-axis title
    painter.save();
    painter.translate(15, height() / 2);
    painter.rotate(-90);
    painter.drawText(QPoint(-40, 0), "Magnitude (dB)");
    painter.restore();
}

void FFTPlot::drawCursor(QPainter& painter)
{
    if (!m_showCursor) {
        return;
    }
    
    painter.setPen(QPen(m_cursorColor, 1, Qt::DashLine));
    
    // Vertical cursor line
    painter.drawLine(QPointF(m_cursorPos.x(), m_plotArea.top()),
                    QPointF(m_cursorPos.x(), m_plotArea.bottom()));
    
    // Horizontal cursor line
    painter.drawLine(QPointF(m_plotArea.left(), m_cursorPos.y()),
                    QPointF(m_plotArea.right(), m_cursorPos.y()));
    
    // Cursor info box
    painter.setPen(QPen(m_textColor, 1));
    painter.setBrush(QBrush(QColor(50, 50, 50, 200)));
    
    QString cursorText = QString("F: %1 kHz\nM: %2 dB")
                        .arg(m_cursorFrequency / 1000.0f, 0, 'f', 1)
                        .arg(m_cursorMagnitude, 0, 'f', 1);
    
    QFontMetrics fm(m_labelFont);
    QRect textRect = fm.boundingRect(cursorText);
    textRect.adjust(-5, -5, 5, 5);
    
    QPoint infoPos = m_cursorPos + QPoint(10, -textRect.height() - 10);
    if (infoPos.x() + textRect.width() > width()) {
        infoPos.setX(m_cursorPos.x() - textRect.width() - 10);
    }
    if (infoPos.y() < 0) {
        infoPos.setY(m_cursorPos.y() + 10);
    }
    
    textRect.moveTo(infoPos);
    painter.drawRect(textRect);
    
    painter.setFont(m_labelFont);
    painter.drawText(textRect.adjusted(5, 5, -5, -5), cursorText);
}

float FFTPlot::frequencyToX(float frequency) const
{
    float maxFreq = m_sampleRate / 2.0f;
    return m_plotArea.left() + (frequency / maxFreq) * m_plotArea.width();
}

float FFTPlot::xToFrequency(float x) const
{
    float maxFreq = m_sampleRate / 2.0f;
    return ((x - m_plotArea.left()) / m_plotArea.width()) * maxFreq;
}

float FFTPlot::magnitudeToY(float magnitude) const
{
    float range = m_maxMagnitude - m_minMagnitude;
    float normalized = (magnitude - m_minMagnitude) / range;
    return m_plotArea.bottom() - normalized * m_plotArea.height();
}

float FFTPlot::yToMagnitude(float y) const
{
    float range = m_maxMagnitude - m_minMagnitude;
    float normalized = (m_plotArea.bottom() - y) / m_plotArea.height();
    return m_minMagnitude + normalized * range;
}

void FFTPlot::updatePlotBounds()
{
    m_plotArea = QRectF(m_marginLeft, m_marginTop,
                       width() - m_marginLeft - m_marginRight,
                       height() - m_marginTop - m_marginBottom);
}

#include "FFTPlot.moc"