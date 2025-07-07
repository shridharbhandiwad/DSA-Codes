#pragma once

#include <QWidget>
#include <QTimer>
#include <QPainter>
#include <QResizeEvent>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QPoint>
#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <vector>
#include <memory>
#include "DataStructures.h"

class PPIPlot : public QWidget
{
    Q_OBJECT

public:
    explicit PPIPlot(QWidget *parent = nullptr);
    ~PPIPlot() override = default;

    void setMaxRange(float maxRange) { m_maxRange = maxRange; update(); }
    float maxRange() const { return m_maxRange; }

    void setShowGrid(bool show) { m_showGrid = show; update(); }
    bool showGrid() const { return m_showGrid; }

    void setShowRangeRings(bool show) { m_showRangeRings = show; update(); }
    bool showRangeRings() const { return m_showRangeRings; }

public slots:
    void updateTargetTracks(const TargetTrackData& trackData);
    void clearTracks();

signals:
    void targetSelected(uint32_t targetId);
    void targetHovered(uint32_t targetId, const QPoint& position);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    struct PlotTarget {
        TargetTrack track;
        QPointF screenPos;
        QColor color;
        bool isHovered;
        qint64 lastUpdateTime;
        
        PlotTarget(const TargetTrack& t) : track(t), isHovered(false), 
                                          lastUpdateTime(QDateTime::currentMSecsSinceEpoch()) {}
    };

    void drawBackground(QPainter& painter);
    void drawGrid(QPainter& painter);
    void drawRangeRings(QPainter& painter);
    void drawAzimuthLines(QPainter& painter);
    void drawTargets(QPainter& painter);
    void drawLabels(QPainter& painter);

    QPointF polarToCartesian(float range, float azimuth) const;
    QPointF cartesianToScreen(const QPointF& cartesian) const;
    QPointF screenToPolar(const QPointF& screen) const;
    
    QColor getTargetColor(const TargetTrack& track) const;
    QString getTargetText(const TargetTrack& track) const;
    
    PlotTarget* findTargetAt(const QPoint& pos);
    void updateTargetPositions();

    std::vector<PlotTarget> m_targets;
    float m_maxRange;
    bool m_showGrid;
    bool m_showRangeRings;
    
    // Display settings
    QFont m_labelFont;
    QFont m_targetFont;
    int m_targetSize;
    int m_hoveredTargetSize;
    
    // Plot geometry
    QPointF m_center;
    float m_plotRadius;
    QRectF m_plotArea;
    
    // Interaction
    PlotTarget* m_hoveredTarget;
    PlotTarget* m_selectedTarget;
    
    // Colors
    QColor m_backgroundColor;
    QColor m_gridColor;
    QColor m_rangeRingColor;
    QColor m_azimuthLineColor;
    QColor m_textColor;
    QColor m_approachingColor;
    QColor m_recedingColor;
    QColor m_stationaryColor;
};