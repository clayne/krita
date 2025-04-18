/*
 *  SPDX-FileCopyrightText: 2016 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoAnchorSelectionWidget.h"

#include <array>
#include <QToolButton>
#include <QButtonGroup>
#include <QGridLayout>
#include <QFontMetrics>
#include "kis_icon_utils.h"

#include "kis_debug.h"
#include "kis_signals_blocker.h"
#include "kis_algebra_2d.h"


struct Q_DECL_HIDDEN KoAnchorSelectionWidget::Private {
    std::array<QToolButton*, KoFlake::NumAnchorPositions> buttons;
    QButtonGroup *buttonGroup;

};

KoAnchorSelectionWidget::KoAnchorSelectionWidget(QWidget *parent)
    : QWidget(parent),
      m_d(new Private)
{
    QVector<QIcon> icons;

    icons << KisIconUtils::loadIcon("arrow-topleft");
    icons << KisIconUtils::loadIcon("arrow-up");
    icons << KisIconUtils::loadIcon("arrow-topright");
    icons << KisIconUtils::loadIcon("arrow-left");
    icons << QIcon(); // center
    icons << KisIconUtils::loadIcon("arrow-right");
    icons << KisIconUtils::loadIcon("arrow-downleft");
    icons << KisIconUtils::loadIcon("arrow-down");
    icons << KisIconUtils::loadIcon("arrow-downright");
    icons << QIcon(); // no anchor

    QGridLayout *gridLayout = new QGridLayout(this);
    gridLayout->setSpacing(0);
    gridLayout->setContentsMargins(0,0,0,0);

    m_d->buttonGroup = new QButtonGroup(this);

    for (int i = 0; i < KoFlake::NumAnchorPositions; i++) {
        QToolButton *button = new QToolButton(this);
        button->setCheckable(true);
        //button->setAutoRaise(true);
        button->setAutoExclusive(true);
        button->setIcon(icons[i]);
        button->setFocusPolicy(Qt::NoFocus);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        if (i != KoFlake::NoAnchor) {
            gridLayout->addWidget(button, i / 3, i % 3, Qt::AlignCenter);
        } else {
            button->setVisible(false);
        }

        m_d->buttonGroup->addButton(button, i);
        m_d->buttons[i] = button;
    }

    connect(m_d->buttonGroup, SIGNAL(idClicked(int)), SLOT(slotGroupClicked(int)));
}

KoAnchorSelectionWidget::~KoAnchorSelectionWidget()
{
}

KoFlake::AnchorPosition KoAnchorSelectionWidget::value() const
{
    return KoFlake::AnchorPosition(m_d->buttonGroup->checkedId());
}

QPointF KoAnchorSelectionWidget::value(const QRectF rect, bool *valid) const
{
    KoFlake::AnchorPosition anchor = this->value();
    return anchorToPoint(anchor, rect, valid);
}

void KoAnchorSelectionWidget::setValue(KoFlake::AnchorPosition value)
{
    if (value == this->value()) return;

    KisSignalsBlocker b(m_d->buttonGroup);

    if (value >= 0) {
        m_d->buttonGroup->button(int(value))->setChecked(true);
    } else {
        QAbstractButton *button = m_d->buttonGroup->checkedButton();
        if (button) {
            button->setChecked(false);
        }
    }

    Q_EMIT valueChanged(value);
}

QSize KoAnchorSelectionWidget::sizeHint() const
{
    const QSize minSize = minimumSizeHint();
    const int preferredHint = qMax(minSize.height(), height());
    return QSize(preferredHint, preferredHint);
}

QSize KoAnchorSelectionWidget::minimumSizeHint() const
{
    QFontMetrics metrics(this->font());
    const int minHeight = 3 * (metrics.height() + 5);
    return QSize(minHeight, minHeight);
}

void KoAnchorSelectionWidget::slotGroupClicked(int id)
{
    KIS_SAFE_ASSERT_RECOVER_RETURN(id >= 0 && id < KoFlake::NumAnchorPositions);
    Q_EMIT valueChanged(KoFlake::AnchorPosition(id));
}

