/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2012 Arjen Hiemstra <ahiemstra@heimr.nl>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_ZOOM_ACTION_H
#define KIS_ZOOM_ACTION_H

#include "kis_abstract_input_action.h"

/**
 * \brief Zoom Canvas implementation of KisAbstractInputAction.
 *
 * The Zoom Canvas action zooms the canvas.
 */
class KisZoomAction : public KisAbstractInputAction
{
public:
    /**
     * The different behaviours for this action.
     */
    enum Shortcuts {
        ZoomModeShortcut, ///< Toggle zoom mode.
        DiscreteZoomModeShortcut, ///< Toggle discrete zoom mode
        ZoomInShortcut, ///< Zoom in by a fixed amount.
        ZoomOutShortcut, ///< Zoom out by a fixed amount.
        Zoom100PctShortcut, ///< Reset zoom to 100%.
        FitToViewShortcut, ///< Zoom fit to page.
        FitToWidthShortcut, ///< Zoom fit to width.
        RelativeZoomModeShortcut, ///< Toggle zoom mode relative to cursor
        RelativeDiscreteZoomModeShortcut, ///< Toggle discrete zoom mode relative to cursor
        FitToHeightShortcut, ///< Zoom fit to height.
        ZoomInToCursorShortcut, ///< Zoom in to cursor
        ZoomOutFromCursorShortcut ///< Zoom out from cursor
    };
    explicit KisZoomAction();
    ~KisZoomAction() override;

    int priority() const override;

    void activate(int shortcut) override;
    void deactivate(int shortcut) override;

    void begin(int shortcut, QEvent *event = 0) override;
    void inputEvent(QEvent* event) override;
    void cursorMovedAbsolute(const QPointF &startPos, const QPointF &pos) override;

    bool isShortcutRequired(int shortcut) const override;
    bool supportsHiResInputEvents(int shortcut) const override;

    KisInputActionGroup inputActionGroup(int shortcut) const override;

private:
    class Private;
    Private * const d {nullptr};
};

#endif // KIS_ZOOM_ACTION_H
