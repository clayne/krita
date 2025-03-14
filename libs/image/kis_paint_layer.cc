/*
 *  SPDX-FileCopyrightText: 2005 C. Boemann <cbo@boemann.dk>
 *  SPDX-FileCopyrightText: 2006 Bart Coppens <kde@bartcoppens.be>
 *  SPDX-FileCopyrightText: 2007 Boudewijn Rempt <boud@valdyas.org>
 *  SPDX-FileCopyrightText: 2009 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_paint_layer.h"

#include <kis_debug.h>
#include <klocalizedstring.h>

#include <KoIcon.h>
#include <kis_icon.h>
#include <KoColorSpace.h>
#include <KoColorProfile.h>
#include <KoCompositeOpRegistry.h>
#include <KoProperties.h>

#include "kis_image.h"
#include "kis_painter.h"
#include "kis_paint_device.h"
#include "kis_node_visitor.h"
#include "kis_processing_visitor.h"
#include "kis_default_bounds.h"

#include "kis_onion_skin_compositor.h"
#include "kis_raster_keyframe_channel.h"

#include "kis_signal_auto_connection.h"
#include "kis_layer_properties_icons.h"

#include "KisFrameChangeUpdateRecipe.h"
#include "kis_onion_skin_cache.h"
#include "kis_time_span.h"


struct Q_DECL_HIDDEN KisPaintLayer::Private
{
public:
    Private(KisPaintLayer *_q) : q(_q), contentChannel(0) {}

    KisPaintLayer *q;

    KisPaintDeviceSP paintDevice;
    QBitArray        paintChannelFlags;

    // the real pointer is owned by the paint device
    KisRasterKeyframeChannel *contentChannel;

    KisSignalAutoConnectionsStore onionSkinConnection;
    KisOnionSkinCache onionSkinCache;

    bool onionSkinVisibleOverride = true;

    [[nodiscard]]
    KisFrameChangeUpdateRecipe
    handleRasterKeyframeChannelUpdateImpl(const KisKeyframeChannel *channel, int time);
};

KisPaintLayer::KisPaintLayer(KisImageWSP image, const QString& name, quint8 opacity, KisPaintDeviceSP dev)
    : KisLayer(image, name, opacity)
    , m_d(new Private(this))
{
    Q_ASSERT(dev);

    m_d->paintDevice = dev;
    m_d->paintDevice->setDefaultBounds(new KisDefaultBounds(image));
    m_d->paintDevice->setSupportsWraparoundMode(true);
    m_d->paintDevice->setParentNode(this);
}


KisPaintLayer::KisPaintLayer(KisImageWSP image, const QString& name, quint8 opacity)
    : KisLayer(image, name, opacity)
    , m_d(new Private(this))
{
    Q_ASSERT(image);

    m_d->paintDevice = new KisPaintDevice(this, image->colorSpace(), new KisDefaultBounds(image));
    m_d->paintDevice->setSupportsWraparoundMode(true);
}

KisPaintLayer::KisPaintLayer(KisImageWSP image, const QString& name, quint8 opacity, const KoColorSpace * colorSpace)
    : KisLayer(image, name, opacity)
    , m_d(new Private(this))
{
    if (!colorSpace) {
        Q_ASSERT(image);
        colorSpace = image->colorSpace();
    }
    Q_ASSERT(colorSpace);
    m_d->paintDevice = new KisPaintDevice(this, colorSpace, new KisDefaultBounds(image));
    m_d->paintDevice->setSupportsWraparoundMode(true);
}

KisPaintLayer::KisPaintLayer(const KisPaintLayer& rhs)
    : KisLayer(rhs)
    , KisIndirectPaintingSupport()
    , m_d(new Private(this))
{
    const bool copyFrames = (rhs.m_d->contentChannel != 0);
    if (!copyFrames) {
        m_d->paintDevice = new KisPaintDevice(*rhs.m_d->paintDevice.data(), KritaUtils::CopySnapshot, this);
        m_d->paintDevice->setSupportsWraparoundMode(true);
        m_d->paintChannelFlags = rhs.m_d->paintChannelFlags;
    } else {
        m_d->paintDevice = new KisPaintDevice(*rhs.m_d->paintDevice.data(), KritaUtils::CopyAllFrames, this);
        m_d->paintDevice->setSupportsWraparoundMode(true);
        m_d->paintChannelFlags = rhs.m_d->paintChannelFlags;

        m_d->contentChannel = m_d->paintDevice->keyframeChannel();
        addKeyframeChannel(m_d->contentChannel);

        m_d->contentChannel->setOnionSkinsEnabled(rhs.onionSkinEnabled());

        KisLayer::enableAnimation();
    }
}

KisPaintLayer::~KisPaintLayer()
{
    delete m_d;
}

bool KisPaintLayer::allowAsChild(KisNodeSP node) const
{
    return node->inherits("KisMask");
}

KisPaintDeviceSP KisPaintLayer::original() const
{
    return m_d->paintDevice;
}

KisPaintDeviceSP KisPaintLayer::paintDevice() const
{
    return m_d->paintDevice;
}

bool KisPaintLayer::needProjection() const
{
    return hasTemporaryTarget() || (isAnimated() && onionSkinEnabled());
}

void KisPaintLayer::copyOriginalToProjection(const KisPaintDeviceSP original,
                                             KisPaintDeviceSP projection,
                                             const QRect& rect) const
{
    KisIndirectPaintingSupport::ReadLocker l(this);

    KisPainter::copyAreaOptimized(rect.topLeft(), original, projection, rect);

    if (hasTemporaryTarget()) {
        KisPainter gc(projection);
        setupTemporaryPainter(&gc);
        gc.bitBlt(rect.topLeft(), temporaryTarget(), rect);
    }

    if (m_d->contentChannel &&
            m_d->contentChannel->keyframeCount() > 1 &&
            onionSkinEnabled() &&
            m_d->onionSkinVisibleOverride &&
            !m_d->paintDevice->defaultBounds()->externalFrameActive()) {

        KisPaintDeviceSP skins = m_d->onionSkinCache.projection(m_d->paintDevice);

        KisPainter gcDest(projection);
        gcDest.setCompositeOpId(COMPOSITE_BEHIND);
        gcDest.bitBlt(rect.topLeft(), skins, rect);
        gcDest.end();
    }

    if (!m_d->contentChannel ||
        (m_d->contentChannel->keyframeCount() <= 1) || !onionSkinEnabled()) {
        m_d->onionSkinCache.reset();
    }
}

QIcon KisPaintLayer::icon() const
{
    return KisIconUtils::loadIcon("paintLayer");
}

void KisPaintLayer::setImage(KisImageWSP image)
{
    m_d->paintDevice->setDefaultBounds(new KisDefaultBounds(image));
    KisLayer::setImage(image);
}

KisBaseNode::PropertyList KisPaintLayer::sectionModelProperties() const
{
    KisBaseNode::PropertyList l = KisLayer::sectionModelProperties();

    l << KisLayerPropertiesIcons::getProperty(KisLayerPropertiesIcons::alphaLocked, alphaLocked());

    if (isAnimated()) {
        l << KisLayerPropertiesIcons::getProperty(KisLayerPropertiesIcons::onionSkins, onionSkinEnabled());
    }

    const KoColorSpace *cs = m_d->paintDevice->colorSpace();
    KisImageSP image = this->image();
    if (image && *image->colorSpace() != *cs) {
        l << KisLayerPropertiesIcons::getColorSpaceMismatchProperty(cs);
    }

    return l;
}

void KisPaintLayer::setSectionModelProperties(const KisBaseNode::PropertyList &properties)
{
    Q_FOREACH (const KisBaseNode::Property &property, properties) {
        if (property.name == i18n("Alpha Locked")) {
            setAlphaLocked(property.state.toBool());
        }
        else if (property.name == i18n("Onion Skins")) {
            setOnionSkinEnabled(property.state.toBool());
        }
    }

    KisLayer::setSectionModelProperties(properties);
}

bool KisPaintLayer::accept(KisNodeVisitor &v)
{
    return v.visit(this);
}

void KisPaintLayer::accept(KisProcessingVisitor &visitor, KisUndoAdapter *undoAdapter)
{
    return visitor.visit(this, undoAdapter);
}

void KisPaintLayer::setChannelLockFlags(const QBitArray& channelFlags)
{
    Q_ASSERT(((quint32)channelFlags.count() == colorSpace()->channelCount() || channelFlags.isEmpty()));
    m_d->paintChannelFlags = channelFlags;
}

const QBitArray& KisPaintLayer::channelLockFlags() const
{
    return m_d->paintChannelFlags;
}

QRect KisPaintLayer::extent() const
{
    KisPaintDeviceSP t = temporaryTarget();
    QRect rect = t ? t->extent() : QRect();
    if (onionSkinEnabled() && m_d->onionSkinVisibleOverride) rect |= KisOnionSkinCompositor::instance()->calculateExtent(m_d->paintDevice);
    return rect | KisLayer::extent();
}

QRect KisPaintLayer::exactBounds() const
{
    KisPaintDeviceSP t = temporaryTarget();
    QRect rect = t ? t->extent() : QRect();
    if (onionSkinEnabled() && m_d->onionSkinVisibleOverride) rect |= KisOnionSkinCompositor::instance()->calculateExtent(m_d->paintDevice);
    return rect | KisLayer::exactBounds();
}

bool KisPaintLayer::alphaLocked() const
{
    QBitArray flags = colorSpace()->channelFlags(false, true) & m_d->paintChannelFlags;
    return flags.count(true) == 0 && !m_d->paintChannelFlags.isEmpty();
}

void KisPaintLayer::setAlphaLocked(bool lock)
{
    const bool oldAlphaLocked = alphaLocked();
    if (oldAlphaLocked == lock) return;

    if(m_d->paintChannelFlags.isEmpty())
        m_d->paintChannelFlags = colorSpace()->channelFlags(true, true);

    if(lock)
        m_d->paintChannelFlags &= colorSpace()->channelFlags(true, false);
    else
        m_d->paintChannelFlags |= colorSpace()->channelFlags(false, true);

    baseNodeChangedCallback();
}

bool KisPaintLayer::onionSkinEnabled() const
{
    return nodeProperties().boolProperty("onionskin", false);
}

void KisPaintLayer::setOnionSkinEnabled(bool state)
{
    const auto oldState = onionSkinEnabled();
    if (oldState == state) return;

    if (!state && oldState) {
        // FIXME: change ordering! race condition possible!

        // Turning off onionskins shrinks our extent. Let's clean up the onion skins first
        setDirty(KisOnionSkinCompositor::instance()->calculateExtent(m_d->paintDevice));
    }

    if (state) {
        m_d->onionSkinConnection.addConnection(KisOnionSkinCompositor::instance(),
                                               SIGNAL(sigOnionSkinChanged()),
                                               this,
                                               SLOT(slotExternalUpdateOnionSkins()));
    } else {
        m_d->onionSkinConnection.clear();
    }

    if (m_d->contentChannel) {
        m_d->contentChannel->setOnionSkinsEnabled(state);
    }

    setNodeProperty("onionskin", state);
}

void KisPaintLayer::flushOnionSkinCache() {
    m_d->onionSkinCache.reset();
}

void KisPaintLayer::slotExternalUpdateOnionSkins()
{
    if (!onionSkinEnabled()) return;

    const QRect dirtyRect =
            KisOnionSkinCompositor::instance()->calculateFullExtent(m_d->paintDevice);

    setDirty(dirtyRect);
}

KisKeyframeChannel *KisPaintLayer::requestKeyframeChannel(const QString &id)
{
    if (id == KisKeyframeChannel::Raster.id()) {
        m_d->contentChannel = m_d->paintDevice->createKeyframeChannel(KisKeyframeChannel::Raster);
        m_d->contentChannel->setOnionSkinsEnabled(onionSkinEnabled());

        enableAnimation();
        return m_d->contentChannel;
    }

    return KisLayer::requestKeyframeChannel(id);
}

KisFrameChangeUpdateRecipe KisPaintLayer::Private::handleRasterKeyframeChannelUpdateImpl(const KisKeyframeChannel *channel, int time)
{
    KisFrameChangeUpdateRecipe recipe;

    recipe.affectedRange = channel->affectedFrames(time);
    recipe.affectedRect = channel->affectedRect(time);

    KisImageWSP image = q->image();
    if (image) {
        KisDefaultBoundsSP bounds(new KisDefaultBounds(image));
        if (recipe.affectedRange.contains(bounds->currentTime())) {
            recipe.totalDirtyRect = recipe.affectedRect;
        }
    }

    if (contentChannel->onionSkinsEnabled()) {
        recipe.totalDirtyRect |= KisOnionSkinCompositor::instance()->updateExtentOnAddition(paintDevice, time);
    }

    return recipe;
}

void KisPaintLayer::handleKeyframeChannelFrameChange(const KisKeyframeChannel *channel, int time)
{
    if (channel->id() == KisKeyframeChannel::Raster.id()) {
        KIS_SAFE_ASSERT_RECOVER_NOOP(0 && "raster channel is not supposed to Q_EMIT sigKeyframeChanged");
    } else {
        KisLayer::handleKeyframeChannelFrameChange(channel, time);
    }
}

void KisPaintLayer::handleKeyframeChannelFrameAdded(const KisKeyframeChannel *channel, int time)
{
    if (channel->id() == KisKeyframeChannel::Raster.id()) {
        m_d->handleRasterKeyframeChannelUpdateImpl(channel, time).notify(this);
    } else {
        KisLayer::handleKeyframeChannelFrameAdded(channel, time);
    }
}

KisFrameChangeUpdateRecipe KisPaintLayer::handleKeyframeChannelFrameAboutToBeRemovedImpl(const KisKeyframeChannel *channel, int time)
{
    if (channel->id() == KisKeyframeChannel::Raster.id()) {
        return m_d->handleRasterKeyframeChannelUpdateImpl(channel, time);
    } else {
        return KisLayer::handleKeyframeChannelFrameAboutToBeRemovedImpl(channel, time);
    }
}

bool KisPaintLayer::supportsKeyframeChannel(const QString &id)
{
     if (id == KisKeyframeChannel::Raster.id()) {
         return true;
     }

     return KisLayer::supportsKeyframeChannel(id);
}

KisPaintDeviceList KisPaintLayer::getLodCapableDevices() const
{
    KisPaintDeviceList list = KisLayer::getLodCapableDevices();

    KisPaintDeviceSP onionSkinsDevice = m_d->onionSkinCache.lodCapableDevice();
    if (onionSkinsDevice) {
        list << onionSkinsDevice;
    }

    return list;
}

bool KisPaintLayer::decorationsVisible() const
{
    return m_d->onionSkinVisibleOverride;
}

void KisPaintLayer::setDecorationsVisible(bool value, bool update)
{
    if (value == decorationsVisible()) return;

    const QRect oldExtent = extent();

    m_d->onionSkinVisibleOverride = value;

    if (update && onionSkinEnabled()) {
        setDirty(oldExtent | extent());
    }
}
