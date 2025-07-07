#include "PPIPlot.h"
#include <QDebug>
#include <QDateTime>
#include <QPen>
#include <QBrush>
#include <QApplication>
#include <QToolTip>
#include <cmath>

PPIPlot::PPIPlot(QWidget *parent)
    : QWidget(parent)
    , m_maxRange(5000.0f)
    , m_showGrid(true)
    , m_showRangeRings(true)
    , m_targetSize(8)
    , m_hoveredTargetSize(12)
    , m_hoveredTarget(nullptr)
    , m_selectedTarget(nullptr)
{
    setMinimumSize(400, 300);
    setMouseTracking(true);
    
    // Initialize fonts
    m_labelFont = QFont("Arial", 10);
    m_targetFont = QFont("Arial", 8, QFont::Bold);
    
    // Initialize colors
    m_backgroundColor = QColor(20, 20, 40);
    m_gridColor = QColor(80, 80, 120);
    m_rangeRingColor = QColor(60, 60, 100);
    m_azimuthLineColor = QColor(60, 60, 100);
    m_textColor = QColor(200, 200, 200);
    m_approachingColor = QColor(255, 100, 100);  // Red for approaching targets
    m_recedingColor = QColor(100, 255, 100);     // Green for receding targets
    m_stationaryColor = QColor(100, 100, 255);   // Blue for stationary targets
    
    updateTargetPositions();
}

void PPIPlot::updateTargetTracks(const TargetTrackData& trackData)
{
    // Clear old targets
    m_targets.clear();
    m_hoveredTarget = nullptr;
    m_selectedTarget = nullptr;
    
    // Add new targets
    for (const auto& track : trackData.targets) {
        // Only show targets within azimuth range (-90° to +90°)
        if (track.azimuth >= -90.0f && track.azimuth <= 90.0f && 
            track.radius <= m_maxRange) {
            m_targets.emplace_back(track);
        }
    }
    
    updateTargetPositions();
    update();
}

void PPIPlot::clearTracks()
{
    m_targets.clear();
    m_hoveredTarget = nullptr;
    m_selectedTarget = nullptr;
    update();
}

void PPIPlot::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    drawBackground(painter);
    
    if (m_showRangeRings) {
        drawRangeRings(painter);
    }
    
    drawAzimuthLines(painter);
    
    if (m_showGrid) {
        drawGrid(painter);
    }
    
    drawTargets(painter);
    drawLabels(painter);
}

void PPIPlot::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)
    updateTargetPositions();
    QWidget::resizeEvent(event);
}

void PPIPlot::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        PlotTarget* target = findTargetAt(event->pos());
        if (target) {
            m_selectedTarget = target;
            emit targetSelected(target->track.target_id);
            update();
        }
    }
    QWidget::mousePressEvent(event);
}

void PPIPlot::mouseMoveEvent(QMouseEvent *event)
{
    PlotTarget* target = findTargetAt(event->pos());
    
    if (target != m_hoveredTarget) {
        if (m_hoveredTarget) {
            m_hoveredTarget->isHovered = false;
        }
        
        m_hoveredTarget = target;
        
        if (m_hoveredTarget) {
            m_hoveredTarget->isHovered = true;
            emit targetHovered(target->track.target_id, event->pos());
            
            // Show tooltip
            QString tooltip = QString("ID: %1\nRange: %2 m\nAzimuth: %3°\nSpeed: %4 m/s")
                             .arg(target->track.target_id)
                             .arg(target->track.radius, 0, 'f', 1)
                             .arg(target->track.azimuth, 0, 'f', 1)
                             .arg(target->track.radial_speed, 0, 'f', 1);
            QToolTip::showText(event->globalPos(), tooltip);
        } else {
            QToolTip::hideText();
        }
        
        update();
    }
    
    QWidget::mouseMoveEvent(event);
}

void PPIPlot::leaveEvent(QEvent *event)
{
    if (m_hoveredTarget) {
        m_hoveredTarget->isHovered = false;
        m_hoveredTarget = nullptr;
        QToolTip::hideText();
        update();
    }
    QWidget::leaveEvent(event);
}

void PPIPlot::drawBackground(QPainter& painter)
{
    painter.fillRect(rect(), m_backgroundColor);
    
    // Calculate plot area (semi-circle)
    int margin = 50;
    int availableWidth = width() - 2 * margin;
    int availableHeight = height() - margin;
    
    m_plotRadius = std::min(availableWidth / 2.0f, static_cast<float>(availableHeight));
    m_center = QPointF(width() / 2.0f, height() - margin);
    
    m_plotArea = QRectF(m_center.x() - m_plotRadius, m_center.y() - m_plotRadius,
                        2 * m_plotRadius, m_plotRadius);
}

void PPIPlot::drawGrid(QPainter& painter)
{
    painter.setPen(QPen(m_gridColor, 1, Qt::DotLine));
    
    // Draw radial grid lines every 15 degrees
    for (int angle = -90; angle <= 90; angle += 15) {
        float radians = qDegreesToRadians(static_cast<float>(angle));
        QPointF endPoint(m_center.x() + m_plotRadius * std::sin(radians),
                        m_center.y() - m_plotRadius * std::cos(radians));
        painter.drawLine(m_center, endPoint);
    }
}

void PPIPlot::drawRangeRings(QPainter& painter)
{
    painter.setPen(QPen(m_rangeRingColor, 1));
    
    // Draw range rings
    float ringSpacing = m_maxRange / 5.0f; // 5 range rings
    for (int i = 1; i <= 5; ++i) {
        float range = i * ringSpacing;
        float radius = (range / m_maxRange) * m_plotRadius;
        
        // Draw semi-circular arc
        QRectF ellipse(m_center.x() - radius, m_center.y() - radius,
                      2 * radius, 2 * radius);
        painter.drawArc(ellipse, 0, 180 * 16); // 180 degrees in 1/16th degree units
        
        // Draw range labels
        painter.setPen(QPen(m_textColor, 1));
        painter.setFont(m_labelFont);
        QString rangeText = QString("%1").arg(range, 0, 'f', 0);
        QPointF labelPos(m_center.x() + radius - 20, m_center.y() - 5);
        painter.drawText(labelPos, rangeText);
        painter.setPen(QPen(m_rangeRingColor, 1));
    }
}

void PPIPlot::drawAzimuthLines(QPainter& painter)
{
    painter.setPen(QPen(m_azimuthLineColor, 1));
    
    // Draw major azimuth lines every 30 degrees
    for (int angle = -90; angle <= 90; angle += 30) {
        float radians = qDegreesToRadians(static_cast<float>(angle));
        QPointF endPoint(m_center.x() + m_plotRadius * std::sin(radians),
                        m_center.y() - m_plotRadius * std::cos(radians));
        painter.drawLine(m_center, endPoint);
        
        // Draw azimuth labels
        painter.setPen(QPen(m_textColor, 1));
        painter.setFont(m_labelFont);
        QString angleText = QString("%1°").arg(angle);
        QPointF labelPos(endPoint.x() - 15, endPoint.y() + 15);
        painter.drawText(labelPos, angleText);
        painter.setPen(QPen(m_azimuthLineColor, 1));
    }
}

void PPIPlot::drawTargets(QPainter& painter)
{
    for (auto& target : m_targets) {
        QColor color = getTargetColor(target.track);
        
        int size = target.isHovered ? m_hoveredTargetSize : m_targetSize;
        if (&target == m_selectedTarget) {
            size += 2;
            painter.setPen(QPen(Qt::white, 2));
        } else {
            painter.setPen(QPen(color.darker(), 1));
        }
        
        painter.setBrush(QBrush(color));
        
        // Draw target symbol (circle for now, could be customized based on target type)
        QRectF targetRect(target.screenPos.x() - size/2, target.screenPos.y() - size/2, size, size);
        painter.drawEllipse(targetRect);
        
        // Draw target ID near the target
        painter.setPen(QPen(m_textColor, 1));
        painter.setFont(m_targetFont);
        QString targetId = QString::number(target.track.target_id);
        QPointF textPos(target.screenPos.x() + size/2 + 3, target.screenPos.y() - size/2);
        painter.drawText(textPos, targetId);
        
        // Draw velocity vector if significant
        if (std::abs(target.track.radial_speed) > 1.0f) {
            float vectorLength = std::min(30.0f, std::abs(target.track.radial_speed) * 2.0f);
            float azimuthRad = qDegreesToRadians(target.track.azimuth);
            
            QPointF velocityEnd;
            if (target.track.radial_speed > 0) {
                // Approaching (toward radar)
                velocityEnd = QPointF(target.screenPos.x() - vectorLength * std::sin(azimuthRad),
                                     target.screenPos.y() + vectorLength * std::cos(azimuthRad));
            } else {
                // Receding (away from radar)
                velocityEnd = QPointF(target.screenPos.x() + vectorLength * std::sin(azimuthRad),
                                     target.screenPos.y() - vectorLength * std::cos(azimuthRad));
            }
            
            painter.setPen(QPen(color, 2));
            painter.drawLine(target.screenPos, velocityEnd);
            
            // Draw arrow head
            float arrowSize = 4.0f;
            QPointF arrowP1, arrowP2;
            float angle = std::atan2(velocityEnd.y() - target.screenPos.y(), 
                                   velocityEnd.x() - target.screenPos.x());
            arrowP1 = QPointF(velocityEnd.x() - arrowSize * std::cos(angle - M_PI/6),
                             velocityEnd.y() - arrowSize * std::sin(angle - M_PI/6));
            arrowP2 = QPointF(velocityEnd.x() - arrowSize * std::cos(angle + M_PI/6),
                             velocityEnd.y() - arrowSize * std::sin(angle + M_PI/6));
            
            painter.drawLine(velocityEnd, arrowP1);
            painter.drawLine(velocityEnd, arrowP2);
        }
    }
}

void PPIPlot::drawLabels(QPainter& painter)
{
    painter.setPen(QPen(m_textColor, 1));
    painter.setFont(m_labelFont);
    
    // Draw title
    QString title = QString("PPI Display - Range: %1 m").arg(m_maxRange, 0, 'f', 0);
    painter.drawText(QPoint(10, 20), title);
    
    // Draw legend
    int legendY = 40;
    painter.drawText(QPoint(10, legendY), "Legend:");
    legendY += 20;
    
    // Approaching targets
    painter.setBrush(QBrush(m_approachingColor));
    painter.setPen(QPen(m_approachingColor.darker(), 1));
    painter.drawEllipse(QRect(10, legendY - 5, 10, 10));
    painter.setPen(QPen(m_textColor, 1));
    painter.drawText(QPoint(25, legendY + 3), "Approaching");
    legendY += 15;
    
    // Receding targets
    painter.setBrush(QBrush(m_recedingColor));
    painter.setPen(QPen(m_recedingColor.darker(), 1));
    painter.drawEllipse(QRect(10, legendY - 5, 10, 10));
    painter.setPen(QPen(m_textColor, 1));
    painter.drawText(QPoint(25, legendY + 3), "Receding");
    legendY += 15;
    
    // Stationary targets
    painter.setBrush(QBrush(m_stationaryColor));
    painter.setPen(QPen(m_stationaryColor.darker(), 1));
    painter.drawEllipse(QRect(10, legendY - 5, 10, 10));
    painter.setPen(QPen(m_textColor, 1));
    painter.drawText(QPoint(25, legendY + 3), "Stationary");
}

QPointF PPIPlot::polarToCartesian(float range, float azimuth) const
{
    float azimuthRad = qDegreesToRadians(azimuth);
    float x = range * std::sin(azimuthRad);
    float y = -range * std::cos(azimuthRad); // Negative because screen Y increases downward
    return QPointF(x, y);
}

QPointF PPIPlot::cartesianToScreen(const QPointF& cartesian) const
{
    float scale = m_plotRadius / m_maxRange;
    float screenX = m_center.x() + cartesian.x() * scale;
    float screenY = m_center.y() + cartesian.y() * scale;
    return QPointF(screenX, screenY);
}

QPointF PPIPlot::screenToPolar(const QPointF& screen) const
{
    float dx = screen.x() - m_center.x();
    float dy = screen.y() - m_center.y();
    
    float range = std::sqrt(dx * dx + dy * dy) * (m_maxRange / m_plotRadius);
    float azimuth = qRadiansToDegrees(std::atan2(dx, -dy));
    
    return QPointF(range, azimuth);
}

QColor PPIPlot::getTargetColor(const TargetTrack& track) const
{
    if (std::abs(track.radial_speed) < 1.0f) {
        return m_stationaryColor;
    } else if (track.radial_speed > 0) {
        return m_approachingColor;
    } else {
        return m_recedingColor;
    }
}

QString PPIPlot::getTargetText(const TargetTrack& track) const
{
    return QString("ID:%1\nR:%2m\nA:%3°\nV:%4m/s")
           .arg(track.target_id)
           .arg(track.radius, 0, 'f', 1)
           .arg(track.azimuth, 0, 'f', 1)
           .arg(track.radial_speed, 0, 'f', 1);
}

PPIPlot::PlotTarget* PPIPlot::findTargetAt(const QPoint& pos)
{
    for (auto& target : m_targets) {
        float dx = pos.x() - target.screenPos.x();
        float dy = pos.y() - target.screenPos.y();
        float distance = std::sqrt(dx * dx + dy * dy);
        
        int clickRadius = target.isHovered ? m_hoveredTargetSize : m_targetSize;
        if (distance <= clickRadius) {
            return &target;
        }
    }
    return nullptr;
}

void PPIPlot::updateTargetPositions()
{
    for (auto& target : m_targets) {
        QPointF cartesian = polarToCartesian(target.track.radius, target.track.azimuth);
        target.screenPos = cartesianToScreen(cartesian);
        target.color = getTargetColor(target.track);
    }
}

#include "PPIPlot.moc"