/*
 *  SPDX-FileCopyrightText: 2016 Boudewijn Rempt <boud@valdyas.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */
#include "View.h"
#include <QPointer>

#include <KoPattern.h>
#include <KoAbstractGradient.h>
#include <kis_paintop_preset.h>
#include <KisView.h>
#include <KisViewManager.h>
#include <kis_node_manager.h>
#include <kis_selection_manager.h>
#include <kis_canvas_resource_provider.h>
#include <kis_exposure_gamma_correction_interface.h>
#include <kis_paintop_box.h>
#include <KisMainWindow.h>
#include <KoCanvasBase.h>
#include <kis_canvas2.h>
#include <KisResourceTypes.h>
#include <KisDocument.h>
#include "Document.h"
#include "Canvas.h"
#include "Window.h"
#include "Resource.h"
#include "ManagedColor.h"

#include "LibKisUtils.h"


struct View::Private {
    Private() {}
    QPointer<KisView> view;
};

View::View(KisView* view, QObject *parent)
    : QObject(parent)
    , d(new Private)
{
    d->view = view;
}

View::~View()
{
    delete d;
}

bool View::operator==(const View &other) const
{
    return (d->view == other.d->view);
}

bool View::operator!=(const View &other) const
{
    return !(operator==(other));
}

Window* View::window() const
{
    if (!d->view) return 0;
    KisMainWindow *mainwin = d->view->mainWindow();
    Window *win = new Window(mainwin);
    return win;
}


Document* View::document() const
{
    if (!d->view) return 0;
    Document *doc = new Document(d->view->document(), false);
    return doc;
}

void View::setDocument(Document *document)
{
    if (!d->view || !document || !document->document()) return;
    d->view = d->view->replaceBy(document->document());
}

bool View::visible() const
{
    if (!d->view) return false;
    return d->view->isVisible();
}

void View::setVisible()
{
    if (!d->view) return;
    KisMainWindow *mainwin = d->view->mainWindow();
    mainwin->setActiveView(d->view);
    mainwin->subWindowActivated();
}

Canvas* View::canvas() const
{
    if (!d->view) return 0;
    Canvas *c = new Canvas(d->view->canvasBase());
    return c;
}

KisView *View::view()
{
    return d->view;
}

void View::activateResource(Resource *resource)
{
    if (!d->view) return;
    if (!resource) return;

    KoResourceSP r = resource->resource();
    if (!r) return;

    if (KoPatternSP pattern = r.dynamicCast<KoPattern>()) {
        QVariant value = QVariant::fromValue(pattern);
        d->view->canvasBase()->resourceManager()->setResource(KoCanvasResource::CurrentPattern, value);
    } else if (KoAbstractGradientSP gradient = r.dynamicCast<KoAbstractGradient>()) {
        QVariant value = QVariant::fromValue(gradient);
        d->view->canvasBase()->resourceManager()->setResource(KoCanvasResource::CurrentGradient, value);
    } else if (KoResourceSP preset = r.dynamicCast<KisPaintOpPreset>()) {
        d->view->viewManager()->paintOpBox()->resourceSelected(preset);
    }
}


ManagedColor *View::foregroundColor() const
{
    if (!d->view) return 0;
    return new ManagedColor(d->view->resourceProvider()->fgColor());
}

void View::setForeGroundColor(ManagedColor *color)
{
    if (!d->view) return;
    d->view->resourceProvider()->setFGColor(color->color());
}

ManagedColor *View::backgroundColor() const
{
    if (!d->view) return 0;
    return new ManagedColor(d->view->resourceProvider()->bgColor());
}

void View::setBackGroundColor(ManagedColor *color)
{
    if (!d->view) return;
    d->view->resourceProvider()->setBGColor(color->color());
}

Resource *View::currentBrushPreset() const
{
    if (!d->view) return 0;
    return new Resource(d->view->resourceProvider()->currentPreset(), ResourceType::PaintOpPresets);
}

void View::setCurrentBrushPreset(Resource *resource)
{
    activateResource(resource);
}

Resource *View::currentPattern() const
{
    if (!d->view) return 0;
    return new Resource(d->view->resourceProvider()->currentPattern(), ResourceType::Patterns);
}

void View::setCurrentPattern(Resource *resource)
{
    activateResource(resource);
}

Resource *View::currentGradient() const
{
    if (!d->view) return 0;
    return new Resource(d->view->resourceProvider()->currentGradient(), ResourceType::Gradients);
}

void View::setCurrentGradient(Resource *resource)
{
    activateResource(resource);
}

QString View::currentBlendingMode() const
{
    if (!d->view) return "";
    return d->view->resourceProvider()->currentCompositeOp();
}

void View::setCurrentBlendingMode(const QString &blendingMode)
{
    if (!d->view) return;
    d->view->resourceProvider()->setCurrentCompositeOp(blendingMode);
}

float View::HDRExposure() const
{
    if (!d->view) return 0.0;
    KisExposureGammaCorrectionInterface *iface = d->view->canvasBase()->exposureGammaCorrectionInterface();
    return iface->currentExposure();
}

void View::setHDRExposure(float exposure)
{
    if (!d->view) return;
    KisExposureGammaCorrectionInterface *iface = d->view->canvasBase()->exposureGammaCorrectionInterface();
    iface->setCurrentExposure(exposure);
}

float View::HDRGamma() const
{
    if (!d->view) return 0.0;
    KisExposureGammaCorrectionInterface *iface = d->view->canvasBase()->exposureGammaCorrectionInterface();
    return iface->currentGamma();
}

void View::setHDRGamma(float gamma)
{
    if (!d->view) return;
    KisExposureGammaCorrectionInterface *iface = d->view->canvasBase()->exposureGammaCorrectionInterface();
    return iface->setCurrentGamma(gamma);
}

qreal View::paintingOpacity() const
{
    if (!d->view) return 0.0;
    return d->view->resourceProvider()->opacity();
}

void View::setPaintingOpacity(qreal opacity)
{
    if (!d->view) return;
    d->view->resourceProvider()->setOpacity(opacity);
}

qreal View::brushSize() const
{
    if (!d->view) return 0.0;
    return d->view->resourceProvider()->size();
}

void View::setBrushSize(qreal brushSize)
{
    if (!d->view) return;
    d->view->resourceProvider()->setSize(brushSize);
}

qreal View::brushRotation() const
{
    if (!d->view) return 0.0;
    return d->view->resourceProvider()->brushRotation();
}

void View::setBrushRotation(qreal brushRotation)
{
    if (!d->view) return;
    d->view->resourceProvider()->setBrushRotation(brushRotation);
}

qreal View::paintingFlow() const
{
    if (!d->view) return 0.0;
    return d->view->resourceProvider()->flow();
}

void View::setPaintingFlow(qreal flow)
{
    if (!d->view) return;
    d->view->resourceProvider()->setFlow(flow);
}

qreal View::patternSize() const
{
    if (!d->view) return 0.0;
    return d->view->resourceProvider()->patternSize();
}

void View::setPatternSize(qreal size)
{
    if (!d->view) return;
    d->view->resourceProvider()->setPatternSize(size);
}

bool View::eraserMode() const
{
    if (!d->view) {
        return false;
    }
    return d->view->resourceProvider()->eraserMode();
}

void View::setEraserMode(bool value)
{
    if (!d->view) {
        return;
    }
    d->view->resourceProvider()->setEraserMode(value);
}

bool View::globalAlphaLock() const
{
    if (!d->view) {
        return false;
    }
    return d->view->resourceProvider()->globalAlphaLock();
}

void View::setGlobalAlphaLock(bool value)
{
    if (!d->view) {
        return;
    }
    d->view->resourceProvider()->setGlobalAlphaLock(value);
}

bool View::disablePressure() const
{
    if (!d->view) {
        return false;
    }
    return d->view->resourceProvider()->disablePressure();
}

void View::setDisablePressure(bool value)
{
    if (!d->view) {
        return;
    }
    d->view->resourceProvider()->setDisablePressure(value);
}


QList<Node *> View::selectedNodes() const
{
    if (!d->view) return QList<Node *>();
    if (!d->view->viewManager()) return QList<Node *>();
    if (!d->view->viewManager()->nodeManager()) return QList<Node *>();

    KisNodeList selectedNodes = d->view->viewManager()->nodeManager()->selectedNodes();
    return LibKisUtils::createNodeList(selectedNodes, d->view->image());
}

void View::showFloatingMessage(const QString &message, const QIcon& icon, int timeout, int priority)
{
    if (!d->view) return;

    KisFloatingMessage::Priority p;
    p = static_cast<KisFloatingMessage::Priority>(priority);

    d->view->showFloatingMessage(message, icon, timeout, p);
}

QTransform View::flakeToDocumentTransform() const
{
    if (!d->view->document()) return QTransform();
    return d->view->canvasBase()->coordinatesConverter()->documentToFlakeTransform().inverted();
}

QTransform View::flakeToCanvasTransform() const
{
    if (!d->view->document()) return QTransform();
    return d->view->canvasBase()->coordinatesConverter()->flakeToWidgetTransform();
}

QTransform View::flakeToImageTransform() const
{
    if (!d->view->document()) return QTransform();

    const KisCoordinatesConverter* coordinatesConverter(d->view->canvasBase()->coordinatesConverter());
    QTransform imageToFlakeTransform = coordinatesConverter->imageToDocumentTransform() * coordinatesConverter->documentToFlakeTransform();

    return imageToFlakeTransform.inverted();
}
