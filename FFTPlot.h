#pragma once

#include <QWidget>
#include <QTimer>
#include <QPainter>
#include <QResizeEvent>
#include <QPaintEvent>
#include <QColor>
#include <QFont>
#include <vector>
#include <complex>
#include <memory>
#include "DataStructures.h"

class FFTPlot : public QWidget
{
    Q_OBJECT

public:
    explicit FFTPlot(QWidget *parent = nullptr);
    ~FFTPlot() override = default;

    void setLogScale(bool logScale) { m_logScale = logScale; update(); }
    bool logScale() const { return m_logScale; }

    void setShowGrid(bool show) { m_showGrid = show; update(); }
    bool showGrid() const { return m_showGrid; }

    void setMaxMagnitude(float maxMag) { m_maxMagnitude = maxMag; update(); }
    float maxMagnitude() const { return m_maxMagnitude; }

public slots:
    void updateFFTData(const RawADCFrame& frame);
    void clearPlot();

signals:
    void frequencySelected(float frequency);
    void magnitudeAtFrequency(float frequency, float magnitude);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    void performFFT(const std::vector<float>& inputData);
    void computeFFT(std::vector<std::complex<float>>& data);
    void drawBackground(QPainter& painter);
    void drawGrid(QPainter& painter);
    void drawSpectrum(QPainter& painter);
    void drawLabels(QPainter& painter);
    void drawCursor(QPainter& painter);

    float frequencyToX(float frequency) const;
    float xToFrequency(float x) const;
    float magnitudeToY(float magnitude) const;
    float yToMagnitude(float y) const;

    void updatePlotBounds();

    std::vector<float> m_magnitudeSpectrum;
    std::vector<float> m_frequencies;
    uint32_t m_sampleRate;
    uint32_t m_fftSize;
    
    // Display settings
    bool m_logScale;
    bool m_showGrid;
    float m_maxMagnitude;
    float m_minMagnitude;
    
    // Plot geometry
    QRectF m_plotArea;
    int m_marginLeft;
    int m_marginRight;
    int m_marginTop;
    int m_marginBottom;
    
    // Fonts and colors
    QFont m_labelFont;
    QFont m_titleFont;
    QColor m_backgroundColor;
    QColor m_gridColor;
    QColor m_spectrumColor;
    QColor m_textColor;
    QColor m_cursorColor;
    
    // Interaction
    bool m_showCursor;
    QPoint m_cursorPos;
    float m_cursorFrequency;
    float m_cursorMagnitude;
    
    // Frame information
    uint32_t m_currentFrameNumber;
    QString m_frameInfo;
};