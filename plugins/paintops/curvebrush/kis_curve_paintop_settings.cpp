/*
 *  SPDX-FileCopyrightText: 2008, 2009 Lukáš Tvrdý <lukast.dev@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <kis_curve_paintop_settings.h>
#include <KisPaintingModeOptionData.h>
#include "KisCurveOpOptionData.h"

struct KisCurvePaintOpSettings::Private
{
    QList<KisUniformPaintOpPropertyWSP> uniformProperties;
};

KisCurvePaintOpSettings::KisCurvePaintOpSettings(KisResourcesInterfaceSP resourcesInterface)
    : KisPaintOpSettings(resourcesInterface),
      m_d(new Private)
{
}

KisCurvePaintOpSettings::~KisCurvePaintOpSettings()
{
}

void KisCurvePaintOpSettings::setPaintOpSize(qreal value)
{
    KisCurveOpOptionData option;
    option.read(this);
    option.curve_line_width = value;
    option.write(this);
}

qreal KisCurvePaintOpSettings::paintOpSize() const
{
    KisCurveOpOptionData option;
    option.read(this);
    return option.curve_line_width;
}

void KisCurvePaintOpSettings::setPaintOpAngle(qreal value)
{
    Q_UNUSED(value);
}

qreal KisCurvePaintOpSettings::paintOpAngle() const
{
    return 0.0;
}

bool KisCurvePaintOpSettings::paintIncremental()
{
    KisPaintingModeOptionData data;
    data.read(this);
    return data.paintingMode == enumPaintingMode::BUILDUP;
}


#include <brushengine/kis_slider_based_paintop_property.h>
#include "kis_paintop_preset.h"
#include "KisPaintOpPresetUpdateProxy.h"
#include "kis_standard_uniform_properties_factory.h"


QList<KisUniformPaintOpPropertySP> KisCurvePaintOpSettings::uniformProperties(KisPaintOpSettingsSP settings, QPointer<KisPaintOpPresetUpdateProxy> updateProxy)
{
    QList<KisUniformPaintOpPropertySP> props =
        listWeakToStrong(m_d->uniformProperties);

    if (props.isEmpty()) {
        {
            KisIntSliderBasedPaintOpPropertyCallback *prop = new KisIntSliderBasedPaintOpPropertyCallback(KisIntSliderBasedPaintOpPropertyCallback::Int,
                                                                                                          KoID("curve_linewidth", i18n("Line Width")),
                                                                                                          settings,
                                                                                                          0);

            prop->setRange(1, 100);
            prop->setSingleStep(1);
            prop->setSuffix(i18n(" px"));

            prop->setReadCallback(
                [](KisUniformPaintOpProperty *prop) {
                    KisCurveOpOptionData option;
                    option.read(prop->settings().data());

                    prop->setValue(option.curve_line_width);
                });
            prop->setWriteCallback(
                [](KisUniformPaintOpProperty *prop) {
                    KisCurveOpOptionData option;
                    option.read(prop->settings().data());
                    option.curve_line_width = prop->value().toInt();
                    option.write(prop->settings().data());
                });

            QObject::connect(updateProxy, SIGNAL(sigSettingsChanged()), prop, SLOT(requestReadValue()));
            prop->requestReadValue();
            props << toQShared(prop);
        }
        {
            KisIntSliderBasedPaintOpPropertyCallback *prop = new KisIntSliderBasedPaintOpPropertyCallback(KisIntSliderBasedPaintOpPropertyCallback::Int,
                                                                                                          KoID("curve_historysize", i18n("History Size")),
                                                                                                          settings,
                                                                                                          0);

            prop->setRange(2, 300);
            prop->setSingleStep(1);

            prop->setReadCallback(
                [](KisUniformPaintOpProperty *prop) {
                    KisCurveOpOptionData option;
                    option.read(prop->settings().data());

                    prop->setValue(option.curve_stroke_history_size);
                });
            prop->setWriteCallback(
                [](KisUniformPaintOpProperty *prop) {
                    KisCurveOpOptionData option;
                    option.read(prop->settings().data());
                    option.curve_stroke_history_size = prop->value().toInt();
                    option.write(prop->settings().data());
                });
            QObject::connect(updateProxy, SIGNAL(sigSettingsChanged()), prop, SLOT(requestReadValue()));
            prop->requestReadValue();
            props << toQShared(prop);
        }

        {
            KisDoubleSliderBasedPaintOpPropertyCallback *prop =
                new KisDoubleSliderBasedPaintOpPropertyCallback(KisDoubleSliderBasedPaintOpPropertyCallback::Double,
                                                                KoID("curve_lineopacity", i18n("Line Opacity")),
                                                                settings,
                                                                0);

            prop->setRange(0, 100.0);
            prop->setSingleStep(0.01);
            prop->setDecimals(2);
            prop->setSuffix(i18n("%"));

            prop->setReadCallback(
                [](KisUniformPaintOpProperty *prop) {
                    KisCurveOpOptionData option;
                    option.read(prop->settings().data());
                    prop->setValue(option.curve_curves_opacity * 100.0);
                });
            prop->setWriteCallback(
                [](KisUniformPaintOpProperty *prop) {
                    KisCurveOpOptionData option;
                    option.read(prop->settings().data());
                    option.curve_curves_opacity = prop->value().toReal() / 100.0;
                    option.write(prop->settings().data());
                });

            QObject::connect(updateProxy, SIGNAL(sigSettingsChanged()), prop, SLOT(requestReadValue()));
            prop->requestReadValue();
            props << toQShared(prop);
        }

        {
            KisUniformPaintOpPropertyCallback *prop = new KisUniformPaintOpPropertyCallback(KisUniformPaintOpPropertyCallback::Bool,
                                                                                            KoID("curve_connectionline", i18n("Connection Line")),
                                                                                            settings,
                                                                                            0);

            prop->setReadCallback(
                [](KisUniformPaintOpProperty *prop) {
                    KisCurveOpOptionData option;
                    option.read(prop->settings().data());

                    prop->setValue(option.curve_paint_connection_line);
                });
            prop->setWriteCallback(
                [](KisUniformPaintOpProperty *prop) {
                    KisCurveOpOptionData option;
                    option.read(prop->settings().data());
                    option.curve_paint_connection_line = prop->value().toBool();
                    option.write(prop->settings().data());
                });

            QObject::connect(updateProxy, SIGNAL(sigSettingsChanged()), prop, SLOT(requestReadValue()));
            prop->requestReadValue();
            props << toQShared(prop);
        }
    }

    {
        using namespace KisStandardUniformPropertiesFactory;

        Q_FOREACH (KisUniformPaintOpPropertySP prop, KisPaintOpSettings::uniformProperties(settings, updateProxy)) {
            if (prop->id() == opacity.id()) {
                props.prepend(prop);
            }
        }
    }

    return props;
}
