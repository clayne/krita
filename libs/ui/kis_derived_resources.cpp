/*
 *  SPDX-FileCopyrightText: 2016 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_derived_resources.h"

#include "kis_signal_auto_connection.h"
#include "kis_canvas_resource_provider.h"
#include "kis_paintop_preset.h"
#include "kis_paintop_settings.h"
#include "KisPaintOpPresetUpdateProxy.h"
#include "KisResourceModel.h"


struct KisPresetUpdateMediator::Private
{
    Private() : model(ResourceType::PaintOpPresets) {}
    KisSignalAutoConnectionsStore connections;
    KisResourceModel model;
    QModelIndex linkedResourceIndex;
};

KisPresetUpdateMediator::KisPresetUpdateMediator()
    : KoResourceUpdateMediator(KoCanvasResource::CurrentPaintOpPreset),
      m_d(new Private)
{
    connect(&m_d->model, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&, const QVector<int>&)),
            this, SLOT(slotResourceChanged(const QModelIndex&, const QModelIndex&, const QVector<int>&)));
}

KisPresetUpdateMediator::~KisPresetUpdateMediator()
{
}

void KisPresetUpdateMediator::connectResource(QVariant sourceResource)
{
    KisPaintOpPresetSP preset = sourceResource.value<KisPaintOpPresetSP>();
    if (!preset) return;

    m_d->connections.clear();
    m_d->connections.addUniqueConnection(
        preset->updateProxy(),
        SIGNAL(sigSettingsChanged()),
        this,
        SLOT(slotSettingsChanged()));

    m_d->linkedResourceIndex = m_d->model.indexForResource(preset);
}

void KisPresetUpdateMediator::slotSettingsChanged()
{
    Q_EMIT sigResourceChanged(key());
}

void KisPresetUpdateMediator::slotResourceChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
    Q_UNUSED(roles)

    /// the name of the preset is stored in KoResource, not
    /// in KisPaintOpSettings, so we should track it separately

    if (m_d->linkedResourceIndex.row() >= topLeft.row() &&
            m_d->linkedResourceIndex.row() <= bottomRight.row()) {

        Q_EMIT sigResourceChanged(key());
    }
}


/*********************************************************************/
/*          KisCompositeOpResourceConverter                          */
/*********************************************************************/


KisCompositeOpResourceConverter::KisCompositeOpResourceConverter()
    : KoDerivedResourceConverter(KoCanvasResource::CurrentCompositeOp,
                                 KoCanvasResource::CurrentPaintOpPreset)
{
}

QVariant KisCompositeOpResourceConverter::fromSource(const QVariant &value)
{
    KisPaintOpPresetSP preset = value.value<KisPaintOpPresetSP>();
    return preset ? preset->settings()->paintOpCompositeOp() : QVariant();
}

QVariant KisCompositeOpResourceConverter::toSource(const QVariant &value, const QVariant &sourceValue)
{
    KisPaintOpPresetSP preset = sourceValue.value<KisPaintOpPresetSP>();
    if (!preset) return sourceValue;

    preset->settings()->setPaintOpCompositeOp(value.toString());
    return QVariant::fromValue(preset);
}

/*********************************************************************/
/*          KisEffectiveCompositeOpResourceConverter                 */
/*********************************************************************/


KisEffectiveCompositeOpResourceConverter::KisEffectiveCompositeOpResourceConverter()
    : KoDerivedResourceConverter(KoCanvasResource::CurrentEffectiveCompositeOp,
                                 KoCanvasResource::CurrentPaintOpPreset)
{
}

QVariant KisEffectiveCompositeOpResourceConverter::fromSource(const QVariant &value)
{
    KisPaintOpPresetSP preset = value.value<KisPaintOpPresetSP>();
    return preset ? preset->settings()->effectivePaintOpCompositeOp() : QVariant();
}

QVariant KisEffectiveCompositeOpResourceConverter::toSource(const QVariant &value, const QVariant &sourceValue)
{
    Q_UNUSED(value);

    // WARNING: we don't save that!
    KisPaintOpPresetSP preset = sourceValue.value<KisPaintOpPresetSP>();
    if (!preset) return sourceValue;

    return QVariant::fromValue(preset);
}

/*********************************************************************/
/*          KisOpacityToPresetOpacityResourceConverter               */
/*********************************************************************/

KisOpacityToPresetOpacityResourceConverter::KisOpacityToPresetOpacityResourceConverter()
    : KoDerivedResourceConverter(KoCanvasResource::Opacity,
                                 KoCanvasResource::CurrentPaintOpPreset)
{
}

QVariant KisOpacityToPresetOpacityResourceConverter::fromSource(const QVariant &value)
{
    KisPaintOpPresetSP preset = value.value<KisPaintOpPresetSP>();
    return preset ? preset->settings()->paintOpOpacity() : QVariant(1.0);
}

QVariant KisOpacityToPresetOpacityResourceConverter::toSource(const QVariant &value, const QVariant &sourceValue)
{
    KisPaintOpPresetSP preset = sourceValue.value<KisPaintOpPresetSP>();
    if (!preset) return sourceValue;

    preset->settings()->setPaintOpOpacity(value.toReal());
    return QVariant::fromValue(preset);
}

/*********************************************************************/
/*          KisFlowResourceConverter                                 */
/*********************************************************************/

KisFlowResourceConverter::KisFlowResourceConverter()
    : KoDerivedResourceConverter(KoCanvasResource::Flow,
                                 KoCanvasResource::CurrentPaintOpPreset)
{
}

QVariant KisFlowResourceConverter::fromSource(const QVariant &value)
{
    KisPaintOpPresetSP preset = value.value<KisPaintOpPresetSP>();
    return preset ? preset->settings()->paintOpFlow() : QVariant();
}

QVariant KisFlowResourceConverter::toSource(const QVariant &value, const QVariant &sourceValue)
{
    KisPaintOpPresetSP preset = sourceValue.value<KisPaintOpPresetSP>();
    if (!preset) return sourceValue;

    preset->settings()->setPaintOpFlow(value.toReal());
    return QVariant::fromValue(preset);
}

/*********************************************************************/
/*          KisFadeResourceConverter                                 */
/*********************************************************************/

KisFadeResourceConverter::KisFadeResourceConverter()
    : KoDerivedResourceConverter(KoCanvasResource::Fade,
                                 KoCanvasResource::CurrentPaintOpPreset)
{
}

QVariant KisFadeResourceConverter::fromSource(const QVariant &value)
{
    KisPaintOpPresetSP preset = value.value<KisPaintOpPresetSP>();
    return preset ? preset->settings()->paintOpFade() : QVariant();
}

QVariant KisFadeResourceConverter::toSource(const QVariant &value, const QVariant &sourceValue)
{
    KisPaintOpPresetSP preset = sourceValue.value<KisPaintOpPresetSP>();
    if (!preset) return sourceValue;

    preset->settings()->setPaintOpFade(value.toReal());
    return QVariant::fromValue(preset);
}

/*********************************************************************/
/*          KisScatterResourceConverter                              */
/*********************************************************************/

KisScatterResourceConverter::KisScatterResourceConverter()
    : KoDerivedResourceConverter(KoCanvasResource::Scatter,
                                 KoCanvasResource::CurrentPaintOpPreset)
{
}

QVariant KisScatterResourceConverter::fromSource(const QVariant &value)
{
    KisPaintOpPresetSP preset = value.value<KisPaintOpPresetSP>();
    return preset ? preset->settings()->paintOpScatter() : QVariant();
}

QVariant KisScatterResourceConverter::toSource(const QVariant &value, const QVariant &sourceValue)
{
    KisPaintOpPresetSP preset = sourceValue.value<KisPaintOpPresetSP>();
    if (!preset) return sourceValue;

    preset->settings()->setPaintOpScatter(value.toReal());
    return QVariant::fromValue(preset);
}

/*********************************************************************/
/*          KisSizeResourceConverter                                 */
/*********************************************************************/

KisSizeResourceConverter::KisSizeResourceConverter()
    : KoDerivedResourceConverter(KoCanvasResource::Size,
                                 KoCanvasResource::CurrentPaintOpPreset)
{
}

QVariant KisSizeResourceConverter::fromSource(const QVariant &value)
{
    KisPaintOpPresetSP preset = value.value<KisPaintOpPresetSP>();
    return (preset && preset->settings()) ? preset->settings()->paintOpSize() : QVariant();
}

QVariant KisSizeResourceConverter::toSource(const QVariant &value, const QVariant &sourceValue)
{
    KisPaintOpPresetSP preset = sourceValue.value<KisPaintOpPresetSP>();
    if (!preset) return sourceValue;

    preset->settings()->setPaintOpSize(value.toReal());
    return QVariant::fromValue(preset);
}

/*********************************************************************/
/*          KisBrushRotationResourceConverter                        */
/*********************************************************************/

KisBrushRotationResourceConverter::KisBrushRotationResourceConverter()
    : KoDerivedResourceConverter(KoCanvasResource::BrushRotation,
                                 KoCanvasResource::CurrentPaintOpPreset)
{
}

QVariant KisBrushRotationResourceConverter::fromSource(const QVariant &value)
{
    KisPaintOpPresetSP preset = value.value<KisPaintOpPresetSP>();
    return (preset && preset->settings()) ? preset->settings()->paintOpAngle() : QVariant();
}

QVariant KisBrushRotationResourceConverter::toSource(const QVariant &value, const QVariant &sourceValue)
{
    KisPaintOpPresetSP preset = sourceValue.value<KisPaintOpPresetSP>();
    if (!preset) return sourceValue;

    preset->settings()->setPaintOpAngle(value.toReal());
    return QVariant::fromValue(preset);
}

///*********************************************************************/
///*          KisPatternSizeResourceConverter                          */
///*********************************************************************/
//
KisPatternSizeResourceConverter::KisPatternSizeResourceConverter()
    : KoDerivedResourceConverter(KoCanvasResource::PatternSize,
        KoCanvasResource::CurrentPaintOpPreset)
{
}

QVariant KisPatternSizeResourceConverter::fromSource(const QVariant& value)
{
    KisPaintOpPresetSP preset = value.value<KisPaintOpPresetSP>();
    return preset && preset->settings()->hasPatternSettings() ? preset->settings()->paintOpPatternSize() : QVariant::fromValue(1.0);
}

QVariant KisPatternSizeResourceConverter::toSource(const QVariant& value, const QVariant& sourceValue)
{
    KisPaintOpPresetSP preset = sourceValue.value<KisPaintOpPresetSP>();
    if (!preset) return sourceValue;

    if (preset->settings()->hasPatternSettings()) {
        preset->settings()->setProperty("Texture/Pattern/Scale", value.toReal());
    }

    return QVariant::fromValue(preset);
}

/*********************************************************************/
/*          KisLodAvailabilityResourceConverter                      */
/*********************************************************************/

KisLodAvailabilityResourceConverter::KisLodAvailabilityResourceConverter()
    : KoDerivedResourceConverter(KoCanvasResource::LodAvailability,
                                 KoCanvasResource::CurrentPaintOpPreset)
{
}

QVariant KisLodAvailabilityResourceConverter::fromSource(const QVariant &value)
{
    KisPaintOpPresetSP preset = value.value<KisPaintOpPresetSP>();
    return preset ? KisPaintOpSettings::isLodUserAllowed(preset->settings()) : QVariant();
}

QVariant KisLodAvailabilityResourceConverter::toSource(const QVariant &value, const QVariant &sourceValue)
{
    KisPaintOpPresetSP preset = sourceValue.value<KisPaintOpPresetSP>();
    if (!preset) return sourceValue;

    KisPaintOpSettings::setLodUserAllowed(preset->settings().data(), value.toBool());
    return QVariant::fromValue(preset);
}

/*********************************************************************/
/*          KisLodSizeThresholdResourceConverter                     */
/*********************************************************************/

KisLodSizeThresholdResourceConverter::KisLodSizeThresholdResourceConverter()
    : KoDerivedResourceConverter(KoCanvasResource::LodSizeThreshold,
                                 KoCanvasResource::CurrentPaintOpPreset)
{
}

QVariant KisLodSizeThresholdResourceConverter::fromSource(const QVariant &value)
{
    KisPaintOpPresetSP preset = value.value<KisPaintOpPresetSP>();
    return preset ? preset->settings()->lodSizeThreshold() : QVariant();
}

QVariant KisLodSizeThresholdResourceConverter::toSource(const QVariant &value, const QVariant &sourceValue)
{
    KisPaintOpPresetSP preset = sourceValue.value<KisPaintOpPresetSP>();
    if (!preset) return sourceValue;

    preset->settings()->setLodSizeThreshold(value.toDouble());
    return QVariant::fromValue(preset);
}

/*********************************************************************/
/*          KisLodSizeThresholdSupportedResourceConverter            */
/*********************************************************************/

KisLodSizeThresholdSupportedResourceConverter::KisLodSizeThresholdSupportedResourceConverter()
    : KoDerivedResourceConverter(KoCanvasResource::LodSizeThresholdSupported,
                                 KoCanvasResource::CurrentPaintOpPreset)
{
}

QVariant KisLodSizeThresholdSupportedResourceConverter::fromSource(const QVariant &value)
{
    KisPaintOpPresetSP preset = value.value<KisPaintOpPresetSP>();
    return preset ? preset->settings()->lodSizeThresholdSupported() : QVariant();
}

QVariant KisLodSizeThresholdSupportedResourceConverter::toSource(const QVariant &value, const QVariant &sourceValue)
{
    // this property of the preset is immutable

    Q_UNUSED(value);
    return sourceValue;
}


/*********************************************************************/
/*          KisEraserModeResourceConverter                           */
/*********************************************************************/

KisEraserModeResourceConverter::KisEraserModeResourceConverter()
    : KoDerivedResourceConverter(KoCanvasResource::EraserMode,
                                 KoCanvasResource::CurrentPaintOpPreset)
{
}

QVariant KisEraserModeResourceConverter::fromSource(const QVariant &value)
{
    KisPaintOpPresetSP preset = value.value<KisPaintOpPresetSP>();
    return preset ? preset->settings()->eraserMode() : QVariant();
}

QVariant KisEraserModeResourceConverter::toSource(const QVariant &value, const QVariant &sourceValue)
{
    KisPaintOpPresetSP preset = sourceValue.value<KisPaintOpPresetSP>();
    if (!preset) return sourceValue;

    preset->settings()->setEraserMode(value.toBool());
    return QVariant::fromValue(preset);
}

KisBrushNameResourceConverter::KisBrushNameResourceConverter()
    : KoDerivedResourceConverter(KoCanvasResource::CurrentPaintOpPresetName,
                                 KoCanvasResource::CurrentPaintOpPreset)
{
}

QVariant KisBrushNameResourceConverter::fromSource(const QVariant &value)
{
    KisPaintOpPresetSP preset = value.value<KisPaintOpPresetSP>();
    return preset ? preset->name() : QVariant();
}

QVariant KisBrushNameResourceConverter::toSource(const QVariant &value, const QVariant &sourceValue)
{
    KisPaintOpPresetSP preset = sourceValue.value<KisPaintOpPresetSP>();
    if (!preset) return sourceValue;

    preset->setName(value.toString());
    return QVariant::fromValue(preset);
}
