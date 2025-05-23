/*
 *  SPDX-FileCopyrightText: 2016 Jouni Pentikäinen <joupent@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _KIS_TIME_BASED_ITEM_MODEL_H
#define _KIS_TIME_BASED_ITEM_MODEL_H

#include <QAbstractTableModel>
#include <KisKineticScroller.h>

#include "kritaanimationdocker_export.h"

#include "kis_types.h"

class KisTimeSpan;
class KisCanvasAnimationState;
class KisKeyframeChannel;
enum PlaybackState : unsigned int;

class KRITAANIMATIONDOCKER_EXPORT KisTimeBasedItemModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    KisTimeBasedItemModel(QObject *parent);
    ~KisTimeBasedItemModel() override;

    void setImage(KisImageWSP image);
    void setFrameCache(KisAnimationFrameCacheSP cache);
    bool isFrameCached(const int frame);
    void setAnimationPlayer(KisCanvasAnimationState *player);
    void setDocument(class KisDocument* document);
    KisDocument* document() const;

    void setLastVisibleFrame(int time);

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role) override;

    void scrubHorizontalHeaderUpdate(int activeHeader);

    bool removeFrames(const QModelIndexList &indexes);

    bool removeFramesAndOffset(QModelIndexList indicesToRemove);

    bool mirrorFrames(QModelIndexList indexes);

    void setScrubState(bool active);
    bool isScrubbing();

    void setPlaybackRange(const KisTimeSpan &range);
    bool isPlaybackActive() const;
    bool isPlaybackPaused() const;
    void stopPlayback() const;
    int currentTime() const;

    enum ItemDataRole
    {
        ActiveFrameRole = Qt::UserRole + 101,
        ScrubToRole,
        CloneOfActiveFrame,
        CloneCount,
        FrameExistsRole,
        SpecialKeyframeExists,
        FrameCachedRole,
        FrameEditableRole,
        FramesPerSecondRole,
        FrameHasContent,
        WithinClipRange,
        UserRole
    };

protected:
    virtual KisNodeSP nodeAt(QModelIndex index) const = 0;
    virtual QMap<QString, KisKeyframeChannel *> channelsAt(QModelIndex index) const = 0;
    virtual KisKeyframeChannel* channelByID(QModelIndex index, const QString &id) const = 0;
    KisImageWSP image() const;

    KUndo2Command* createOffsetFramesCommand(QModelIndexList srcIndexes, const QPoint &offset,
                                             bool copyFrames, bool moveEmptyFrames,
                                             KUndo2Command *parentCommand = 0);

    bool cloneOfActiveFrame(const QModelIndex &index) const;
    int cloneCount(const QModelIndex &index) const;

protected Q_SLOTS:
    void slotCurrentTimeChanged(int time);

private Q_SLOTS:
    void slotFramerateChanged();
    void slotPlaybackRangeChanged();
    void slotCacheChanged();

    void slotPlaybackFrameChanged();
    void slotPlaybackStateChanged(PlaybackState state);
private:
    struct Private;
    const QScopedPointer<Private> m_d;
};

#endif
