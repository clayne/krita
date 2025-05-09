/*
 *  SPDX-FileCopyrightText: 2010 Sven Langkamp <sven.langkamp@gmail.com>
 *  SPDX-FileCopyrightText: 2011 Jan Hambrecht <jaham@gmx.net>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_shape_selection.h"


#include <QPainter>
#include <kundo2command.h>
#include <QMimeData>
#include <QApplication>
#include <QThread>

#include <KoShapeStroke.h>
#include <KoPathShape.h>
#include <KoShapeGroup.h>
#include <KoCompositeOp.h>
#include <KoShapeManager.h>
#include <KisDocument.h>

#include <KoXmlNS.h>
#include <KoShapeRegistry.h>
#include <KoShapeLoadingContext.h>
#include <KoXmlWriter.h>
#include <KoStore.h>
#include <KoShapeController.h>
#include <KoShapeSavingContext.h>
#include <KoStoreDevice.h>
#include <KoShapeTransformCommand.h>

#include <kis_painter.h>
#include <kis_paint_device.h>
#include <kis_image.h>
#include <kis_iterator_ng.h>
#include <kis_selection.h>

#include "kis_shape_selection_model.h"
#include "kis_shape_selection_canvas.h"
#include "kis_take_all_shapes_command.h"
#include "kis_image_view_converter.h"
#include "kis_shape_layer.h"
#include "kis_lod_transform.h"

#include <kis_debug.h>


KisShapeSelection::KisShapeSelection(KoShapeControllerBase *shapeControllerBase, KisSelectionWSP selection)
    : KoShapeLayer(new KisShapeSelectionModel(selection->resolutionProxy(), selection, this)),
      m_model(static_cast<KisShapeSelectionModel*>(this->model())),
      m_resolutionProxy(m_model->resolutionProxy())
{
    init(m_resolutionProxy, shapeControllerBase);
}

KisShapeSelection::KisShapeSelection(const KisShapeSelection& rhs, KisSelection* selection)
    : KoShapeLayer(new KisShapeSelectionModel(selection->resolutionProxy(), selection, this)),
      m_model(static_cast<KisShapeSelectionModel*>(this->model())),
      m_resolutionProxy(m_model->resolutionProxy())
{
    init(m_resolutionProxy, rhs.m_shapeControllerBase);

    // TODO: refactor shape selection to pass signals
    //       via KoShapeManager, not via the model
    m_canvas->shapeManager()->setUpdatesBlocked(true);
    m_model->setUpdatesEnabled(false);

    m_canvas->shapeManager()->addShape(this);
    Q_FOREACH (KoShape *shape, rhs.shapes()) {
        KoShape *clonedShape = shape->cloneShape();
        KIS_SAFE_ASSERT_RECOVER(clonedShape) { continue; }
        this->addShape(clonedShape);
    }

    m_canvas->shapeManager()->setUpdatesBlocked(false);
    m_model->setUpdatesEnabled(true);
}

KisShapeSelection::~KisShapeSelection()
{
    m_model->setShapeSelection(0);
    delete m_canvas;
    delete m_converter;
}

void KisShapeSelection::init(KisImageResolutionProxySP resolutionProxy, KoShapeControllerBase *shapeControllerBase)
{
    KIS_SAFE_ASSERT_RECOVER_RETURN(resolutionProxy);
    KIS_SAFE_ASSERT_RECOVER_RETURN(shapeControllerBase);

    m_shapeControllerBase = shapeControllerBase;

    setShapeId("KisShapeSelection");
    setSelectable(false);
    m_converter = new KisImageViewConverter(resolutionProxy);
    m_canvas = new KisShapeSelectionCanvas(shapeControllerBase);
    m_canvas->shapeManager()->addShape(this);

    m_model->setObjectName("KisShapeSelectionModel");
    m_model->moveToThread(qApp->thread());
    m_canvas->setObjectName("KisShapeSelectionCanvas");
    m_canvas->moveToThread(qApp->thread());

    connect(this, SIGNAL(sigMoveShapes(QPointF)), SLOT(slotMoveShapes(QPointF)));
}

KisSelectionComponent* KisShapeSelection::clone(KisSelection* selection)
{
    return new KisShapeSelection(*this, selection);
}

bool KisShapeSelection::saveSelection(KoStore * store, const QRect &imageRect) const
{
    const QSizeF sizeInPx = imageRect.size();
    const QSizeF sizeInPt(sizeInPx.width() / m_resolutionProxy->xRes(), sizeInPx.height() / m_resolutionProxy->yRes());

    return KisShapeLayer::saveShapesToStore(store, this->shapes(), sizeInPt);
}

bool KisShapeSelection::loadSelection(KoStore* store, const QRect &imageRect)
{
    QSizeF fragmentSize; // unused!

    // FIXME: we handle xRes() only!
    KIS_SAFE_ASSERT_RECOVER_NOOP(qFuzzyCompare(m_resolutionProxy->xRes(), m_resolutionProxy->yRes()));
    const qreal resolutionPPI = 72.0 * m_resolutionProxy->xRes();

    QList<KoShape*> shapes;

    if (store->open("content.svg")) {
        KoStoreDevice storeDev(store);
        storeDev.open(QIODevice::ReadOnly);

        shapes = KisShapeLayer::createShapesFromSvg(&storeDev,
                                                    "", imageRect,
                                                    resolutionPPI, m_canvas->shapeController()->resourceManager(),
                                                    true,
                                                    &fragmentSize);

        store->close();

        Q_FOREACH (KoShape *shape, shapes) {
            addShape(shape);
        }

        return true;
    }

    return false;
}

void KisShapeSelection::setUpdatesEnabled(bool enabled)
{
    m_model->setUpdatesEnabled(enabled);
}

bool KisShapeSelection::updatesEnabled() const
{
    return m_model->updatesEnabled();
}

KUndo2Command* KisShapeSelection::resetToEmpty()
{
    return new KisTakeAllShapesCommand(this, true, false);
}

bool KisShapeSelection::isEmpty() const
{
    return !m_model->count();
}

QPainterPath KisShapeSelection::outlineCache() const
{
    return m_outline;
}

bool KisShapeSelection::outlineCacheValid() const
{
    return true;
}

void KisShapeSelection::recalculateOutlineCache()
{
    QTransform resolutionMatrix;
    resolutionMatrix.scale(m_resolutionProxy->xRes(), m_resolutionProxy->yRes());

    QList<KoShape*> shapesList = shapes();

    QPainterPath outline;
    Q_FOREACH (KoShape * shape, shapesList) {
        /**
         * WARNING: we should unite all the shapes in image coordinates,
         * not in points. Boolean operations inside the QPainterPath
         * linearize the curves into lines and they use absolute values
         * for thresholds.
         *
         * See KritaUtils::pathShapeBooleanSpaceWorkaround() for more info
         */
        QTransform shapeMatrix = shape->absoluteTransformation();
        outline = outline.united(resolutionMatrix.map(shapeMatrix.map(shape->outline())));
    }

    m_outline = outline;
}

void KisShapeSelection::paintComponent(QPainter& painter) const
{
    Q_UNUSED(painter);
}

void KisShapeSelection::renderToProjection(KisPaintDeviceSP projection)
{
    Q_ASSERT(projection);

    QRectF boundingRect = outlineCache().boundingRect();
    renderSelection(projection, boundingRect.toAlignedRect());
}

void KisShapeSelection::renderToProjection(KisPaintDeviceSP projection, const QRect& r)
{
    Q_ASSERT(projection);
    renderSelection(projection, r);
}

void KisShapeSelection::renderSelection(KisPaintDeviceSP projection, const QRect& requestedRect)
{
    KIS_SAFE_ASSERT_RECOVER_RETURN(projection);

    const qint32 MASK_IMAGE_WIDTH = 256;
    const qint32 MASK_IMAGE_HEIGHT = 256;

    QPainterPath selectionOutline = outlineCache();

    if (projection->defaultBounds()->currentLevelOfDetail() > 0) {
        KisLodTransform t(projection);
        selectionOutline = t.map(selectionOutline);
    }

    if (*projection->defaultPixel().data() == OPACITY_TRANSPARENT_U8) {
        projection->clear(requestedRect);
    } else {
        KoColor transparentColor = KoColor::createTransparent(projection->colorSpace());
        projection->fill(requestedRect, transparentColor);
    }
    const QRect r = requestedRect & selectionOutline.boundingRect().toAlignedRect();

    QImage polygonMaskImage(MASK_IMAGE_WIDTH, MASK_IMAGE_HEIGHT, QImage::Format_ARGB32);
    QPainter maskPainter(&polygonMaskImage);
    maskPainter.setRenderHint(QPainter::Antialiasing, true);

    // Break the mask up into chunks so we don't have to allocate a potentially very large QImage.
    for (qint32 x = r.x(); x < r.x() + r.width(); x += MASK_IMAGE_WIDTH) {
        for (qint32 y = r.y(); y < r.y() + r.height(); y += MASK_IMAGE_HEIGHT) {

            maskPainter.fillRect(polygonMaskImage.rect(), Qt::black);
            maskPainter.translate(-x, -y);
            maskPainter.fillPath(selectionOutline, Qt::white);
            maskPainter.translate(x, y);

            qint32 rectWidth = qMin(r.x() + r.width() - x, MASK_IMAGE_WIDTH);
            qint32 rectHeight = qMin(r.y() + r.height() - y, MASK_IMAGE_HEIGHT);

            KisSequentialIterator it(projection, QRect(x, y, rectWidth, rectHeight));
            while (it.nextPixel()) {
                (*it.rawData()) = qRed(polygonMaskImage.pixel(it.x() - x, it.y() - y));
            }
        }
    }
}

KoShapeManager* KisShapeSelection::shapeManager() const
{
    return m_canvas->shapeManager();
}

KisShapeSelectionFactory::KisShapeSelectionFactory()
    : KoShapeFactoryBase("KisShapeSelection", "selection shape container")
{
    setHidden(true);
}

void KisShapeSelection::moveX(qint32 x)
{
    const QPointF diff(x / m_resolutionProxy->xRes(), 0);
    Q_EMIT sigMoveShapes(diff);
}

void KisShapeSelection::moveY(qint32 y)
{
    const QPointF diff(0, y / m_resolutionProxy->yRes());
    Q_EMIT sigMoveShapes(diff);
}

void KisShapeSelection::slotMoveShapes(const QPointF &diff)
{
    Q_FOREACH (KoShape* shape, shapeManager()->shapes()) {
        if (shape != this) {
            QPointF pos = shape->position();
            shape->setPosition(pos + diff);
        }
    }
}

// TODO same code as in vector layer, refactor!
KUndo2Command* KisShapeSelection::transform(const QTransform &transform) {
    QList<KoShape*> shapes = m_canvas->shapeManager()->shapes();
    if(shapes.isEmpty()) return 0;

    QTransform realTransform = m_converter->documentToView() *
            transform * m_converter->viewToDocument();

    QList<QTransform> oldTransformations;
    QList<QTransform> newTransformations;

    // this code won't work if there are shapes, that inherit the transformation from the parent container.
    // the chart and tree shapes are examples for that, but they aren't used in krita and there are no other shapes like that.
    Q_FOREACH (const KoShape* shape, shapes) {
        QTransform oldTransform = shape->transformation();
        oldTransformations.append(oldTransform);

        // don't transform the container
        if (dynamic_cast<const KoShapeGroup *>(shape) || !shape->parent()) {
            newTransformations.append(oldTransform);
        } else {
            QTransform globalTransform = shape->absoluteTransformation();
            QTransform localTransform = globalTransform * realTransform * globalTransform.inverted();
            newTransformations.append(localTransform*oldTransform);
        }
    }

    return new KoShapeTransformCommand(shapes, oldTransformations, newTransformations);
}

void KisShapeSelection::setResolutionProxy(KisImageResolutionProxySP resolutionProxy)
{
    m_resolutionProxy = resolutionProxy;

    // the model will automatically request update for the
    // canvas and recalculation of the outline
    m_model->setResolutionProxy(resolutionProxy);
}
