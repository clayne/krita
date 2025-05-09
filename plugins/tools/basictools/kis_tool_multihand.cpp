/*
 *  SPDX-FileCopyrightText: 2011 Lukáš Tvrdý <lukast.dev@gmail.com>
 *  SPDX-FileCopyrightText: 2011 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "kis_tool_multihand.h"

#include <QTransform>

#include <QPushButton>
#include <QFormLayout>
#include <QStackedWidget>
#include <kis_slider_spin_box.h>
#include "kis_aspect_ratio_locker.h"
#include "kis_canvas2.h"
#include "kis_cursor.h"
#include "KisViewManager.h"
#include "kis_selection.h"

#include "kis_tool_multihand_helper.h"

#include <QtGlobal>


static const int MAXIMUM_BRUSHES = 50;

KisToolMultihand::KisToolMultihand(KoCanvasBase *canvas)
    : KisToolBrush(canvas),
      m_transformMode(SYMMETRY),
      m_angle(0),
      m_handsCount(6),
      m_mirrorVertically(false),
      m_mirrorHorizontally(false),
      m_showAxes(false),
      m_translateRadius(100),
      m_setupAxesFlag(false),
      m_addSubbrushesMode(false),
      m_intervalX(0),
      m_intervalY(0)
    , m_randomGenerator(QRandomGenerator::global()->generate())
    , customUI(0)
{


    m_helper =
        new KisToolMultihandHelper(paintingInformationBuilder(),
                                   canvas->resourceManager(),
                                   kundo2_i18n("Multibrush Stroke"));
    resetHelper(m_helper);
    if (image()) {
        m_axesPoint = QPointF(0.5 * image()->width(), 0.5 * image()->height());
    }

}

KisToolMultihand::~KisToolMultihand()
{
}

void KisToolMultihand::beginPrimaryAction(KoPointerEvent *event)
{
    if(m_setupAxesFlag) {
        setMode(KisTool::OTHER);
        m_axesPoint = convertToPixelCoord(event->point);
        requestUpdateOutline(event->point, 0);
        updateCanvas();
    }
    else if (m_addSubbrushesMode &&  m_transformMode == COPYTRANSLATE){
        QPointF newPoint = convertToPixelCoord(event->point);
        m_subbrOriginalLocations << newPoint;
        requestUpdateOutline(event->point, 0);
        updateCanvas();
    }
    else if (m_transformMode == COPYTRANSLATEINTERVALS) {
        m_axesPoint = convertToPixelCoord(event->point);
        initTransformations();
        KisToolFreehand::beginPrimaryAction(event);
    }
    else {
        initTransformations();
        KisToolFreehand::beginPrimaryAction(event);
    }
}

void KisToolMultihand::continuePrimaryAction(KoPointerEvent *event)
{
    if(mode() == KisTool::OTHER) {
        m_axesPoint = convertToPixelCoord(event->point);
        requestUpdateOutline(event->point, 0);
        updateCanvas();
    }
    else {
        requestUpdateOutline(event->point, 0);
        KisToolFreehand::continuePrimaryAction(event);
    }
}

void KisToolMultihand::endPrimaryAction(KoPointerEvent *event)
{
    if(mode() == KisTool::OTHER) {
        setMode(KisTool::HOVER_MODE);
        requestUpdateOutline(event->point, 0);
        finishAxesSetup();
    }
    else {
        KisToolFreehand::endPrimaryAction(event);
    }
}

void KisToolMultihand::beginAlternateAction(KoPointerEvent* event, AlternateAction action)
{
    if ((action != ChangeSize && action != ChangeSizeSnap) ||
            m_transformMode != COPYTRANSLATE ||
            !m_addSubbrushesMode) {
        KisToolBrush::beginAlternateAction(event, action);
        return;
    }
    setMode(KisTool::OTHER_1);
    m_axesPoint = convertToPixelCoord(event->point);
    requestUpdateOutline(event->point, 0);
    updateCanvas();
}

void KisToolMultihand::continueAlternateAction(KoPointerEvent* event, AlternateAction action)
{
    if ((action != ChangeSize && action != ChangeSizeSnap) ||
            m_transformMode != COPYTRANSLATE ||
            !m_addSubbrushesMode) {
        KisToolBrush::continueAlternateAction(event, action);
        return;
    }
    if (mode() == KisTool::OTHER_1) {
        m_axesPoint = convertToPixelCoord(event->point);
        requestUpdateOutline(event->point, 0);
        updateCanvas();
    }
}

void KisToolMultihand::endAlternateAction(KoPointerEvent* event, AlternateAction action)
{
    if ((action != ChangeSize && action != ChangeSizeSnap) ||
            m_transformMode != COPYTRANSLATE ||
            !m_addSubbrushesMode) {
        KisToolBrush::endAlternateAction(event, action);
        return;
    }
    if (mode() == KisTool::OTHER_1) {
        setMode(KisTool::HOVER_MODE);
    }
}

void KisToolMultihand::mouseMoveEvent(KoPointerEvent* event)
{
    if (mode() == HOVER_MODE) {
        m_lastToolPos=convertToPixelCoord(event->point);
    }
    KisToolBrush::mouseMoveEvent(event);
}

void KisToolMultihand::paint(QPainter& gc, const KoViewConverter &converter)
{
    QPainterPath path;

    if (m_showAxes) {
        const int axisLength = currentImage()->height() + currentImage()->width();

        // add division guide lines if using multiple brushes
        if ((m_handsCount > 1 && m_transformMode == SYMMETRY) ||
            (m_handsCount > 1 && m_transformMode == SNOWFLAKE) ) {
            int axesCount;
            if (m_transformMode == SYMMETRY){
                 axesCount = m_handsCount;
            }
            else {
                axesCount = m_handsCount*2;
            }
            const qreal axesAngle = 360.0 / float(axesCount);
            float currentAngle = 0.0;
            const float startingInsetLength = 20; // don't start each line at the origin so we can see better when all points converge

            // draw lines radiating from the origin
            for( int i=0; i < axesCount; i++) {

                currentAngle = i*axesAngle;

                // convert angles to radians since cos and sin need that
                currentAngle = currentAngle * 0.017453 + m_angle; // m_angle is current rotation set on UI

                const QPoint startingSpot = QPoint(m_axesPoint.x()+ (sin(currentAngle)*startingInsetLength), m_axesPoint.y()- (cos(currentAngle))*startingInsetLength );
                path.moveTo(startingSpot.x(), startingSpot.y());
                QPointF symmetryLinePoint(m_axesPoint.x()+ (sin(currentAngle)*axisLength), m_axesPoint.y()- (cos(currentAngle))*axisLength );
                path.lineTo(symmetryLinePoint);
            }

        }
        else if(m_transformMode == MIRROR) {

            if (m_mirrorHorizontally) {
                path.moveTo(m_axesPoint.x()-axisLength*cos(m_angle+M_PI_2), m_axesPoint.y()-axisLength*sin(m_angle+M_PI_2));
                path.lineTo(m_axesPoint.x()+axisLength*cos(m_angle+M_PI_2), m_axesPoint.y()+axisLength*sin(m_angle+M_PI_2));
            }

            if(m_mirrorVertically) {
                path.moveTo(m_axesPoint.x()-axisLength*cos(m_angle), m_axesPoint.y()-axisLength*sin(m_angle));
                path.lineTo(m_axesPoint.x()+axisLength*cos(m_angle), m_axesPoint.y()+axisLength*sin(m_angle));
            }
        }
        else if (m_transformMode == COPYTRANSLATE) {

            const int ellipsePreviewSize = 10;
            // draw ellipse at origin to emphasize this is a drawing point
            path.addEllipse(m_axesPoint.x()-(ellipsePreviewSize),
                            m_axesPoint.y()-(ellipsePreviewSize),
                            ellipsePreviewSize*2,
                            ellipsePreviewSize*2);

            Q_FOREACH (QPointF dPos, m_subbrOriginalLocations) {
                path.addEllipse(dPos, ellipsePreviewSize, ellipsePreviewSize);  // Show subbrush reference locations while in add mode
            }

            // draw the horiz/vertical line for axis  origin
            path.moveTo(m_axesPoint.x()-axisLength*cos(m_angle), m_axesPoint.y()-axisLength*sin(m_angle));
            path.lineTo(m_axesPoint.x()+axisLength*cos(m_angle), m_axesPoint.y()+axisLength*sin(m_angle));
            path.moveTo(m_axesPoint.x()-axisLength*cos(m_angle+M_PI_2), m_axesPoint.y()-axisLength*sin(m_angle+M_PI_2));
            path.lineTo(m_axesPoint.x()+axisLength*cos(m_angle+M_PI_2), m_axesPoint.y()+axisLength*sin(m_angle+M_PI_2));

        }
        else if (m_transformMode == COPYTRANSLATEINTERVALS) {
            const int ellipsePreviewSize = 10;

            Q_FOREACH (QPointF dPos, intervalLocations()) {
                path.addEllipse(dPos, ellipsePreviewSize, ellipsePreviewSize);
            }
        }
        else {

            // draw the horiz/vertical line for axis  origin
            path.moveTo(m_axesPoint.x()-axisLength*cos(m_angle), m_axesPoint.y()-axisLength*sin(m_angle));
            path.lineTo(m_axesPoint.x()+axisLength*cos(m_angle), m_axesPoint.y()+axisLength*sin(m_angle));
            path.moveTo(m_axesPoint.x()-axisLength*cos(m_angle+M_PI_2), m_axesPoint.y()-axisLength*sin(m_angle+M_PI_2));
            path.lineTo(m_axesPoint.x()+axisLength*cos(m_angle+M_PI_2), m_axesPoint.y()+axisLength*sin(m_angle+M_PI_2));
        }

    } else {

        // not showing axis
        if (m_transformMode == COPYTRANSLATE) {

            Q_FOREACH (QPointF dPos, m_subbrOriginalLocations) {
                // Show subbrush reference locations while in add mode
                if (m_addSubbrushesMode) {
                    path.addEllipse(dPos, 10, 10);
                }
            }
        }
    }

    KisToolFreehand::paint(gc, converter);

    // origin point preview line/s
    gc.save();
    QPen outlinePen;
    outlinePen.setColor(QColor(100,100,100,150));
    outlinePen.setStyle(Qt::PenStyle::SolidLine);
    gc.setPen(outlinePen);
    paintToolOutline(&gc, pixelToView(path));
    gc.restore();


    // fill in a dot for the origin if showing axis
    if (m_showAxes && m_transformMode != COPYTRANSLATEINTERVALS) {
        // draw a dot at the origin point to help with precisely moving
        QPainterPath dotPath;
        const int dotRadius = 4;
        dotPath.moveTo(m_axesPoint.x(), m_axesPoint.y());
        dotPath.addEllipse(m_axesPoint.x()- dotRadius*0.25, m_axesPoint.y()- dotRadius*0.25, dotRadius, dotRadius); // last 2 parameters are dot's size

        QBrush fillBrush;
        fillBrush.setColor(QColor(255, 255, 255, 255));
        fillBrush.setStyle(Qt::SolidPattern);
        gc.fillPath(pixelToView(dotPath), fillBrush);


        // add slight offset circle for contrast to help show it on
        dotPath = QPainterPath(); // resets path
        dotPath.addEllipse(m_axesPoint.x() - dotRadius*0.75, m_axesPoint.y()- dotRadius*0.75, dotRadius, dotRadius); // last 2 parameters are dot's size
        fillBrush.setColor(QColor(120, 120, 120, 255));
        gc.fillPath(pixelToView(dotPath), fillBrush);
    }

}

void KisToolMultihand::initTransformations()
{
    QVector<QTransform> transformations;
    QTransform m;

    if(m_transformMode == SYMMETRY) {
        qreal angle = 0;
        const qreal angleStep = (2 * M_PI) / m_handsCount;

        for(int i = 0; i < m_handsCount; i++) {
            m.translate(m_axesPoint.x(), m_axesPoint.y());
            m.rotateRadians(angle);
            m.translate(-m_axesPoint.x(), -m_axesPoint.y());

            transformations << m;
            m.reset();
            angle += angleStep;
        }
    }
    else if(m_transformMode == MIRROR) {
        transformations << m;

        if (m_mirrorHorizontally) {
            m.translate(m_axesPoint.x(),m_axesPoint.y());
            m.rotateRadians(m_angle);
            m.scale(-1,1);
            m.rotateRadians(-m_angle);
            m.translate(-m_axesPoint.x(), -m_axesPoint.y());
            transformations << m;
            m.reset();
        }

        if (m_mirrorVertically) {
            m.translate(m_axesPoint.x(),m_axesPoint.y());
            m.rotateRadians(m_angle);
            m.scale(1,-1);
            m.rotateRadians(-m_angle);
            m.translate(-m_axesPoint.x(), -m_axesPoint.y());
            transformations << m;
            m.reset();
        }

        if (m_mirrorVertically && m_mirrorHorizontally){
            m.translate(m_axesPoint.x(),m_axesPoint.y());
            m.rotateRadians(m_angle);
            m.scale(-1,-1);
            m.rotateRadians(-m_angle);
            m.translate(-m_axesPoint.x(), -m_axesPoint.y());
            transformations << m;
            m.reset();
        }

    }
    else if(m_transformMode == SNOWFLAKE) {
        qreal angle = 0;
        const qreal angleStep = (2 * M_PI) / m_handsCount/4;

        for(int i = 0; i < m_handsCount*4; i++) {
           if ((i%2)==1) {

               m.translate(m_axesPoint.x(), m_axesPoint.y());
               m.rotateRadians(m_angle-angleStep);
               m.rotateRadians(angle);
               m.scale(-1,1);
               m.rotateRadians(-m_angle+angleStep);
               m.translate(-m_axesPoint.x(), -m_axesPoint.y());

               transformations << m;
               m.reset();
               angle += angleStep*2;
           } else {
               m.translate(m_axesPoint.x(), m_axesPoint.y());
               m.rotateRadians(m_angle-angleStep);
               m.rotateRadians(angle);
               m.rotateRadians(-m_angle+angleStep);
               m.translate(-m_axesPoint.x(), -m_axesPoint.y());

               transformations << m;
               m.reset();
               angle += angleStep*2;
            }
        }
    }
    else if(m_transformMode == TRANSLATE) {
        /**
         * TODO: currently, the seed is the same for all the
         * strokes
         */
        for (int i = 0; i < m_handsCount; i++){
            const qreal angle = m_randomGenerator.bounded(2.0 * M_PI);
            const qreal length = m_randomGenerator.bounded(1.0);

            // convert the Polar coordinates to Cartesian coordinates
            qreal nx = (m_translateRadius * cos(angle) * length);
            qreal ny = (m_translateRadius * sin(angle) * length);

            m.translate(m_axesPoint.x(),m_axesPoint.y());
            m.rotateRadians(m_angle);
            m.translate(nx,ny);
            m.rotateRadians(-m_angle);
            m.translate(-m_axesPoint.x(), -m_axesPoint.y());
            transformations << m;
            m.reset();
        }
    } else if (m_transformMode == COPYTRANSLATE) {
        transformations << m;
        Q_FOREACH (QPointF dPos, m_subbrOriginalLocations) {
            const QPointF resPos = dPos-m_axesPoint; // Calculate the difference between subbrush reference position and "origin" reference
            m.translate(resPos.x(), resPos.y());
            transformations << m;
            m.reset();
        }
    } else if (m_transformMode == COPYTRANSLATEINTERVALS) {
        KisCanvas2 *kisCanvas = dynamic_cast<KisCanvas2*>(canvas());
        Q_ASSERT(kisCanvas);
        const QRect bounds = kisCanvas->viewManager()->selection() ?
            kisCanvas->viewManager()->selection()->selectedExactRect() :
            kisCanvas->currentImage()->bounds();
        const QPoint dPos = bounds.topLeft() +
                      QPoint(m_intervalX ? m_intervalX * floor((m_axesPoint.x() - bounds.left()) / m_intervalX) : 0,
                             m_intervalY ? m_intervalY * floor((m_axesPoint.y() - bounds.top()) / m_intervalY) : 0);

        Q_FOREACH (QPoint pos, intervalLocations()) {
                const QPointF resPos = pos - dPos;
                m.translate(resPos.x(), resPos.y());
                transformations << m;
                m.reset();
        }
    }

    m_helper->setupTransformations(transformations);
}

QWidget* KisToolMultihand::createOptionWidget()
{
    QWidget *widget = KisToolBrush::createOptionWidget();

    customUI = new KisToolMultiHandConfigWidget();

    // brush smoothing option.
    //customUI->layout()->addWidget(widget);
    customUI->smoothingOptionsLayout->addWidget(widget);


    // setup common parameters that all of the modes will see
    connect(customUI->showAxesCheckbox, SIGNAL(toggled(bool)), this, SLOT(slotSetAxesVisible(bool)));
    customUI->showAxesCheckbox->setChecked((bool)m_configGroup.readEntry("showAxes", false));

    connect(image(), SIGNAL(sigSizeChanged(QPointF,QPointF)), this, SLOT(resetAxes()));

    customUI->moveOriginButton->setCheckable(true);
    connect(customUI->moveOriginButton, SIGNAL(clicked(bool)),this, SLOT(activateAxesPointModeSetup()));

    connect(customUI->resetOriginButton, SIGNAL(released()), this, SLOT(resetAxes()));

    customUI->multihandTypeCombobox->addItem(i18n("Symmetry"),int(SYMMETRY));  // axis mode
    customUI->multihandTypeCombobox->addItem(i18nc("Label of Mirror in Multihand brush tool options", "Mirror"),int(MIRROR));
    customUI->multihandTypeCombobox->addItem(i18n("Translate"),int(TRANSLATE));
    customUI->multihandTypeCombobox->addItem(i18n("Snowflake"),int(SNOWFLAKE));
    customUI->multihandTypeCombobox->addItem(i18n("Copy Translate"),int(COPYTRANSLATE));
    customUI->multihandTypeCombobox->addItem(i18n("Copy Translate at Intervals"),int(COPYTRANSLATEINTERVALS));
    connect(customUI->multihandTypeCombobox,SIGNAL(currentIndexChanged(int)),this, SLOT(slotSetTransformMode(int)));
    customUI->multihandTypeCombobox->setCurrentIndex(m_configGroup.readEntry("transformMode", 0));


    customUI->axisRotationAngleSelector->setRange(0.0, 90.0);
    customUI->axisRotationAngleSelector->setDecimals(1);
    customUI->axisRotationAngleSelector->setWrapping(false);
    customUI->axisRotationAngleSelector->setFlipOptionsMode(KisAngleSelector::FlipOptionsMode_NoFlipOptions);
    connect(customUI->axisRotationAngleSelector, SIGNAL(angleChanged(qreal)), this, SLOT(slotSetAxesAngle(qreal)));
    customUI->axisRotationAngleSelector->setAngle(m_configGroup.readEntry("axesAngle", 0.0));


    // symmetry mode options
    customUI->brushCountSpinBox->setRange(1, MAXIMUM_BRUSHES);
    connect(customUI->brushCountSpinBox, SIGNAL(valueChanged(int)),this, SLOT(slotSetHandsCount(int)));
    customUI->brushCountSpinBox->setValue(m_configGroup.readEntry("handsCount", 4));

    // mirror mode specific options
    connect(customUI->horizontalCheckbox, SIGNAL(toggled(bool)), this, SLOT(slotSetMirrorHorizontally(bool)));
    customUI->horizontalCheckbox->setChecked((bool)m_configGroup.readEntry("mirrorHorizontally", false));

    connect(customUI->verticalCheckbox, SIGNAL(toggled(bool)), this, SLOT(slotSetMirrorVertically(bool)));
    customUI->verticalCheckbox->setChecked((bool)m_configGroup.readEntry("mirrorVertically", false));

    // translate mode options
    customUI->translationRadiusSpinbox->setRange(0, 200);
    customUI->translationRadiusSpinbox->setSuffix(i18n(" px"));

    connect(customUI->translationRadiusSpinbox,SIGNAL(valueChanged(int)),this,SLOT(slotSetTranslateRadius(int)));
    customUI->translationRadiusSpinbox->setValue(m_configGroup.readEntry("translateRadius", 0));

    // Copy translate mode options and actions
    connect(customUI->addSubbrushButton, &QPushButton::clicked, this, &KisToolMultihand::slotAddSubbrushesMode);
    connect(customUI->removeSubbrushButton, &QPushButton::clicked, this, &KisToolMultihand::slotRemoveAllSubbrushes);

    // Copy translate at intervals mode options and actions
    customUI->intervalXSpinBox->setRange(0, 2000);
    customUI->intervalXSpinBox->setSuffix(i18n(" px"));
    customUI->intervalYSpinBox->setRange(0, 2000);
    customUI->intervalYSpinBox->setSuffix(i18n(" px"));

    KisAspectRatioLocker *intervalAspectLocker = new KisAspectRatioLocker(this);
    intervalAspectLocker->connectSpinBoxes(customUI->intervalXSpinBox, customUI->intervalYSpinBox, customUI->intervalAspectButton);

    customUI->intervalXSpinBox->setValue(m_configGroup.readEntry("intervalX", 0));
    customUI->intervalYSpinBox->setValue(m_configGroup.readEntry("intervalY", 0));
    connect(intervalAspectLocker, SIGNAL(sliderValueChanged()), this, SLOT(slotSetIntervals()));
    slotSetIntervals(); // X and Y need to be set at the same time.
    connect(intervalAspectLocker, SIGNAL(aspectButtonChanged()), this, SLOT(slotSetKeepAspect()));
    customUI->intervalAspectButton->setKeepAspectRatio(m_configGroup.readEntry("intervalKeepAspect", false));

    // snowflake re-uses the existing options, so there is no special parameters for that...


    return static_cast<QWidget*>(customUI); // keeping it in the native class until the end allows us to access the UI components
}

void KisToolMultihand::activateAxesPointModeSetup()
{
    if (customUI->moveOriginButton->isChecked()){
        m_setupAxesFlag = true;
        useCursor(KisCursor::crossCursor());
        updateCanvas();
    } else {
        finishAxesSetup();
    }
}

void KisToolMultihand::resetAxes()
{
    m_axesPoint = QPointF(0.5 * image()->width(), 0.5 * image()->height());
    finishAxesSetup();
}


void KisToolMultihand::finishAxesSetup()
{
    m_setupAxesFlag = false;
    customUI->moveOriginButton->setChecked(false);
    resetCursorStyle();
    updateCanvas();
}

void KisToolMultihand::updateCanvas()
{
    KisCanvas2 *kisCanvas = dynamic_cast<KisCanvas2*>(canvas());
    Q_ASSERT(kisCanvas);
    kisCanvas->updateCanvas();
    if(customUI->moveOriginButton->isChecked())
    {
        kisCanvas->viewManager()->showFloatingMessage(i18n("X: %1 px\nY: %2 px"
                , QString::number(this->m_axesPoint.x(),'f',1),QString::number(this->m_axesPoint.y(),'f',1))
                , QIcon(), 1000, KisFloatingMessage::High, Qt::AlignLeft | Qt::TextWordWrap | Qt::AlignVCenter);
    }
}

QVector<QPoint> KisToolMultihand::intervalLocations()
{
    QVector<QPoint> intervalLocations;

    KisCanvas2 *kisCanvas = dynamic_cast<KisCanvas2*>(canvas());
    Q_ASSERT(kisCanvas);
    const QRect bounds = kisCanvas->viewManager()->selection() ?
        kisCanvas->viewManager()->selection()->selectedExactRect() :
        kisCanvas->currentImage()->bounds();

    const int intervals = m_intervalX ? (bounds.width() / m_intervalX) : 0 +
                    m_intervalY ? (bounds.height() / m_intervalY) : 0;
    if (intervals > MAXIMUM_BRUSHES) {
        kisCanvas->viewManager()->showFloatingMessage(
            i18n("Multibrush Tool does not support more than %1 brushes; use a larger interval.",
            QString::number(MAXIMUM_BRUSHES)), QIcon());
        return intervalLocations;
    }

    for (int x = bounds.left(); x <= bounds.right(); x += m_intervalX) {
        for (int y = bounds.top(); y <= bounds.bottom(); y += m_intervalY) {
            intervalLocations << QPoint(x,y);

            if (m_intervalY == 0) { break; }
        }
        if (m_intervalX == 0) { break; }
    }

    return intervalLocations;
}

void KisToolMultihand::slotSetHandsCount(int count)
{
    m_handsCount = count;
    m_configGroup.writeEntry("handsCount", count);
    updateCanvas();
}

void KisToolMultihand::slotSetAxesAngle(qreal angle)
{
    //negative so axes rotates counter clockwise
    m_angle = -angle*M_PI/180;
    updateCanvas();
    m_configGroup.writeEntry("axesAngle", angle);
}

void KisToolMultihand::slotSetTransformMode(int index)
{
    m_transformMode = enumTransformModes(customUI->multihandTypeCombobox->itemData(index).toInt());
    m_configGroup.writeEntry("transformMode", index);

    // turn on or off what we need

    bool vis = index == MIRROR;
    customUI->horizontalCheckbox->setVisible(vis);
    customUI->verticalCheckbox->setVisible(vis);

    vis = index == TRANSLATE;
    customUI->translationRadiusSpinbox->setVisible(vis);
    customUI->radiusLabel->setVisible(vis);
    customUI->brushCountSpinBox->setVisible(vis);
    customUI->brushesLabel->setVisible(vis);

    vis = index == SYMMETRY || index == SNOWFLAKE || index == TRANSLATE;
    customUI->brushCountSpinBox->setVisible(vis);
    customUI->brushesLabel->setVisible(vis);
    
    vis = index == COPYTRANSLATE;
    customUI->subbrushLabel->setVisible(vis);
    customUI->addSubbrushButton->setVisible(vis);
    customUI->addSubbrushButton->setChecked(m_addSubbrushesMode);
    customUI->removeSubbrushButton->setVisible(vis);

    vis = index == COPYTRANSLATEINTERVALS;
    customUI->intervalXLabel->setVisible(vis);
    customUI->intervalYLabel->setVisible(vis);
    customUI->intervalXSpinBox->setVisible(vis);
    customUI->intervalYSpinBox->setVisible(vis);
    customUI->intervalAspectButton->setVisible(vis);

    vis = index != COPYTRANSLATEINTERVALS;
    customUI->label->setVisible(vis); // the origin label
    customUI->moveOriginButton->setVisible(vis);
    customUI->resetOriginButton->setVisible(vis);
    customUI->axisRotationLabel->setVisible(vis);
    customUI->axisRotationAngleSelector->setVisible(vis);

}

void KisToolMultihand::slotSetAxesVisible(bool vis)
{
    m_showAxes = vis;
    updateCanvas();
    m_configGroup.writeEntry("showAxes", vis);
}


void KisToolMultihand::slotSetMirrorVertically(bool mirror)
{
    m_mirrorVertically = mirror;
    updateCanvas();
    m_configGroup.writeEntry("mirrorVertically", mirror);
}

void KisToolMultihand::slotSetMirrorHorizontally(bool mirror)
{
    m_mirrorHorizontally = mirror;
    updateCanvas();
    m_configGroup.writeEntry("mirrorHorizontally", mirror);
}

void KisToolMultihand::slotSetTranslateRadius(int radius)
{
    m_translateRadius = radius;
    m_configGroup.writeEntry("translateRadius", radius);
}

void KisToolMultihand::slotAddSubbrushesMode(bool checked)
{
    m_addSubbrushesMode = checked;
    updateCanvas();
}

void KisToolMultihand::slotRemoveAllSubbrushes()
{
    m_subbrOriginalLocations.clear();
    updateCanvas();
}

void KisToolMultihand::slotSetIntervals()
{
    m_intervalX = customUI->intervalXSpinBox->value();
    m_configGroup.writeEntry("intervalX", m_intervalX);

    m_intervalY = customUI->intervalYSpinBox->value();
    m_configGroup.writeEntry("intervalY", m_intervalY);

    updateCanvas();
}

void KisToolMultihand::slotSetKeepAspect()
{
    m_configGroup.writeEntry("intervalKeepAspect", customUI->intervalAspectButton->keepAspectRatio());
}
