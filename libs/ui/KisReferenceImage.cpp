/*
 * SPDX-FileCopyrightText: 2017 Boudewijn Rempt <boud@valdyas.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KisReferenceImage.h"
#include "KoColorSpaceRegistry.h"

#include <QImage>
#include <QMessageBox>
#include <QPainter>
#include <QApplication>
#include <QClipboard>
#include <QSharedData>
#include <QFileInfo>
#include <QImageReader>
#include <QUrl>

#include <QColorSpace>

#include <kundo2command.h>
#include <KoStore.h>
#include <KoStoreDevice.h>
#include <KoTosContainer_p.h>
#include <krita_utils.h>
#include <kis_coordinates_converter.h>
#include <kis_dom_utils.h>
#include <SvgUtil.h>
#include <libs/flake/svg/parsers/SvgTransformParser.h>
#include <libs/brush/kis_qimage_pyramid.h>

#include <KisDocument.h>
#include <KisPart.h>

#include "kis_clipboard.h"

struct KisReferenceImage::Private : public QSharedData
{
    // Filename within .kra (for embedding)
    QString internalFilename;

    // File on disk (for linking)
    QString externalFilename;

    QImage image;
    QImage cachedImage;
    KisQImagePyramid mipmap;

    qreal saturation{1.0};
    int id{-1};
    bool embed{true};

    bool loadFromFile() {
        KIS_SAFE_ASSERT_RECOVER_RETURN_VALUE(!externalFilename.isEmpty(), false);
        KIS_SAFE_ASSERT_RECOVER_RETURN_VALUE(QFileInfo(externalFilename).exists(), false);
        KIS_SAFE_ASSERT_RECOVER_RETURN_VALUE(QFileInfo(externalFilename).isReadable(), false);
        {
            QImageReader reader(externalFilename);
            reader.setDecideFormatFromContent(true);
            image = reader.read();

            if (image.isNull()) {
                reader.setAutoDetectImageFormat(true);
                image = reader.read();
            }

        }

        if (image.isNull()) {
            image.load(externalFilename);
        }

        if (image.isNull()) {
            KisDocument * doc = KisPart::instance()->createTemporaryDocument();
            if (doc->openPath(externalFilename, KisDocument::DontAddToRecent)) {
                image = doc->image()->convertToQImage(doc->image()->bounds(), 0);
            }
            KisPart::instance()->removeDocument(doc);
        }

        // See https://bugs.kde.org/show_bug.cgi?id=416515 -- a jpeg image
        // loaded into a qimage cannot be saved to png unless we explicitly
        // convert the colorspace of the QImage
        image.convertToColorSpace(QColorSpace(QColorSpace::SRgb));

        return (!image.isNull());
    }

    bool loadFromQImage(const QImage &img) {
        image = img;
        return !image.isNull();
    }

    void updateCache() {
        if (saturation < 1.0) {
            cachedImage = KritaUtils::convertQImageToGrayA(image);

            if (saturation > 0.0) {
                QPainter gc2(&cachedImage);
                gc2.setOpacity(saturation);
                gc2.drawImage(QPoint(), image);
            }
        } else {
            cachedImage = image;
        }

        mipmap = KisQImagePyramid(cachedImage, false);
    }
};


KisReferenceImage::SetSaturationCommand::SetSaturationCommand(const QList<KoShape *> &shapes, qreal newSaturation, KUndo2Command *parent)
    : KUndo2Command(kundo2_i18n("Set saturation"), parent)
    , newSaturation(newSaturation)
{
    images.reserve(shapes.count());

    Q_FOREACH(auto *shape, shapes) {
        auto *reference = dynamic_cast<KisReferenceImage*>(shape);
        KIS_SAFE_ASSERT_RECOVER_BREAK(reference);
        images.append(reference);
    }

    Q_FOREACH(auto *image, images) {
        oldSaturations.append(image->saturation());
    }
}

void KisReferenceImage::SetSaturationCommand::undo()
{
    auto saturationIterator = oldSaturations.begin();
    Q_FOREACH(auto *image, images) {
        image->setSaturation(*saturationIterator);
        image->update();
        saturationIterator++;
    }
}

void KisReferenceImage::SetSaturationCommand::redo()
{
    Q_FOREACH(auto *image, images) {
        image->setSaturation(newSaturation);
        image->update();
    }
}

KisReferenceImage::KisReferenceImage()
    : d(new Private())
{
    setKeepAspectRatio(true);
}

KisReferenceImage::KisReferenceImage(const KisReferenceImage &rhs)
    : KoTosContainer(rhs)
    , d(rhs.d)
{}

KisReferenceImage::~KisReferenceImage()
{}

KisReferenceImage * KisReferenceImage::fromFile(const QString &filename, const KisCoordinatesConverter &converter, QWidget *parent)
{
    KisReferenceImage *reference = new KisReferenceImage();
    reference->d->externalFilename = filename;
    bool ok = reference->d->loadFromFile();

    if (ok) {
        QRect r = QRect(QPoint(), reference->d->image.size());
        QSizeF shapeSize = converter.imageToDocument(r).size();
        reference->setSize(shapeSize);
    } else {
        delete reference;

        if (parent) {
            QMessageBox::critical(parent, i18nc("@title:window", "Krita"), i18n("Could not load %1.", filename));
        }

        return nullptr;
    }

    return reference;
}

KisReferenceImage *KisReferenceImage::fromClipboard(const KisCoordinatesConverter &converter)
{
    const auto sz = KisClipboard::instance()->clipSize();
    KisPaintDeviceSP clip = KisClipboard::instance()->clip({0, 0, sz.width(), sz.height()}, true);
    return fromPaintDevice(clip, converter, nullptr);
}

KisReferenceImage *
KisReferenceImage::fromPaintDevice(KisPaintDeviceSP src, const KisCoordinatesConverter &converter, QWidget *)
{
    if (!src) {
        return nullptr;
    }

    auto *reference = new KisReferenceImage();
    reference->d->image = src->convertToQImage(KoColorSpaceRegistry::instance()->p709SRGBProfile());

    QRect r = QRect(QPoint(), reference->d->image.size());
    QSizeF size = converter.imageToDocument(r).size();
    reference->setSize(size);

    return reference;
}

KisReferenceImage *KisReferenceImage::fromQImage(const KisCoordinatesConverter &converter, const QImage &img)
{
    KisReferenceImage *reference = new KisReferenceImage();
    bool ok = reference->d->loadFromQImage(img);

    if (ok) {
        QRect r = QRect(QPoint(), reference->d->image.size());
        QSizeF size = converter.imageToDocument(r).size();
        reference->setSize(size);
    } else {
        delete reference;
        reference = 0;
    }

    return reference;
}

void KisReferenceImage::paint(QPainter &gc) const
{
    if (!parent()) return;

    gc.save();

    QSizeF shapeSize = size();
    // scale and rotation done by the user (excluding zoom)
    QTransform transform = QTransform::fromScale(shapeSize.width() / d->image.width(), shapeSize.height() / d->image.height());

    if (d->cachedImage.isNull()) {
        // detach the data
        const_cast<KisReferenceImage*>(this)->d->updateCache();
    }

    qreal scale;
    // scale from the highDPI display
    QTransform devicePixelRatioFTransform = QTransform::fromScale(gc.device()->devicePixelRatioF(), gc.device()->devicePixelRatioF());
    // all three transformations: scale and rotation done by the user, scale from highDPI display, and zoom + rotation of the view
    // order: zoom/rotation of the view; scale to high res; scale and rotation done by the user
    QImage prescaled = d->mipmap.getClosestWithoutWorkaroundBorder(transform * devicePixelRatioFTransform * gc.transform(), &scale);
    transform.scale(1.0 / scale, 1.0 / scale);

    if (scale > 1.0) {
        // enlarging should be done without smooth transformation
        // so the user can see pixels just as they are painted
        gc.setRenderHints(QPainter::Antialiasing);
    } else {
        gc.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    }
    gc.setClipRect(QRectF(QPointF(), shapeSize), Qt::IntersectClip);
    gc.setTransform(transform, true);
    gc.drawImage(QPoint(), prescaled);

    gc.restore();
}

void KisReferenceImage::setSaturation(qreal saturation)
{
    d->saturation = saturation;
    d->cachedImage = QImage();
}

qreal KisReferenceImage::saturation() const
{
    return d->saturation;
}

void KisReferenceImage::setEmbed(bool embed)
{
    KIS_SAFE_ASSERT_RECOVER_RETURN(embed || !d->externalFilename.isEmpty());
    d->embed = embed;
}

bool KisReferenceImage::embed()
{
    return d->embed;
}

bool KisReferenceImage::hasLocalFile()
{
    return !d->externalFilename.isEmpty();
}

QString KisReferenceImage::filename() const
{
    return d->externalFilename;
}

QString KisReferenceImage::internalFile() const
{
    return d->internalFilename;
}


void KisReferenceImage::setFilename(const QString &filename)
{
    d->externalFilename = filename;
}

QColor KisReferenceImage::getPixel(QPointF position)
{
    if (transparency() == 1.0) return Qt::transparent;

    const QSizeF shapeSize = size();
    const QTransform scale = QTransform::fromScale(d->image.width() / shapeSize.width(), d->image.height() / shapeSize.height());

    const QTransform transform = absoluteTransformation().inverted() * scale;
    const QPointF localPosition = position * transform;

    if (d->cachedImage.isNull()) {
        d->updateCache();
    }

    return d->cachedImage.pixelColor(localPosition.toPoint());
}

void KisReferenceImage::saveXml(QDomDocument &document, QDomElement &parentElement, int id)
{
    d->id = id;

    QDomElement element = document.createElement("referenceimage");

    if (d->embed) {
        d->internalFilename = QString("reference_images/%1.png").arg(id);
    }
    
    const QString src = d->embed ? d->internalFilename : (QString("file://") + d->externalFilename);
    element.setAttribute("src", src);

    const QSizeF &shapeSize = size();
    element.setAttribute("width", KisDomUtils::toString(shapeSize.width()));
    element.setAttribute("height", KisDomUtils::toString(shapeSize.height()));
    element.setAttribute("keepAspectRatio", keepAspectRatio() ? "true" : "false");
    element.setAttribute("transform", SvgUtil::transformToString(transform()));

    element.setAttribute("opacity", KisDomUtils::toString(1.0 - transparency()));
    element.setAttribute("saturation", KisDomUtils::toString(d->saturation));

    parentElement.appendChild(element);
}

KisReferenceImage * KisReferenceImage::fromXml(const QDomElement &elem)
{
    auto *reference = new KisReferenceImage();

    const QString &src = elem.attribute("src");

    if (src.startsWith("file://")) {
        reference->d->externalFilename = src.mid(7);
        reference->d->embed = false;
    } else {
        reference->d->internalFilename = src;
        reference->d->embed = true;
    }

    qreal width = KisDomUtils::toDouble(elem.attribute("width", "100"));
    qreal height = KisDomUtils::toDouble(elem.attribute("height", "100"));
    reference->setSize(QSizeF(width, height));
    reference->setKeepAspectRatio(elem.attribute("keepAspectRatio", "true").toLower() == "true");

    auto transform = SvgTransformParser(elem.attribute("transform")).transform();
    reference->setTransformation(transform);

    qreal opacity = KisDomUtils::toDouble(elem.attribute("opacity", "1"));
    reference->setTransparency(1.0 - opacity);

    qreal saturation = KisDomUtils::toDouble(elem.attribute("saturation", "1"));
    reference->setSaturation(saturation);

    return reference;
}

bool KisReferenceImage::saveImage(KoStore *store) const
{
    if (!d->embed) return true;

    if (!store->open(d->internalFilename)) {
        return false;
    }

    bool saved = false;

    KoStoreDevice storeDev(store);
    if (storeDev.open(QIODevice::WriteOnly)) {
        saved = d->image.save(&storeDev, "PNG");
    }

    return store->close() && saved;
}

bool KisReferenceImage::loadImage(KoStore *store)
{
    if (!d->embed) {
        return d->loadFromFile();
    }

    if (!store->open(d->internalFilename)) {
        return false;
    }

    KoStoreDevice storeDev(store);
    if (!storeDev.open(QIODevice::ReadOnly)) {
        return false;
    }

    if (!d->image.load(&storeDev, "PNG")) {
        return false;
    }

    return store->close();
}

QImage KisReferenceImage::getImage()
{
    return d->image;
}

KoShape *KisReferenceImage::cloneShape() const
{
    return new KisReferenceImage(*this);
}
