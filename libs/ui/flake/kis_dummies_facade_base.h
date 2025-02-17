/*
 *  SPDX-FileCopyrightText: 2011 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef __KIS_DUMMIES_FACADE_BASE_H
#define __KIS_DUMMIES_FACADE_BASE_H

#include <QObject>

#include "kis_types.h"
#include "kritaui_export.h"
#include <KisNodeAdditionFlags.h>

class KisNodeDummy;

/**
 * Keeps track of the node stack and manages local (UI-wide) representation
 * of the node stack. It uses KisNodeDummy objects to represent the stack.
 * This is done to break synchronization tie between UI and Image threads,
 * caused by the fact that KisNodeModel must be synchronously notified
 * when a node is removed/deleted.
 */

class KRITAUI_EXPORT KisDummiesFacadeBase : public QObject
{
    Q_OBJECT

public:
    KisDummiesFacadeBase(QObject *parent = 0);
    ~KisDummiesFacadeBase() override;

    void setImage(KisImageWSP image);
    virtual void setImage(KisImageWSP image, KisNodeSP activeNode);

    virtual bool hasDummyForNode(KisNodeSP node) const = 0;
    virtual KisNodeDummy* dummyForNode(KisNodeSP node) const = 0;

    /**
     * Return the root dummy of the graph. Since the call to setImage() causes
     * an **asynchronous** update of the dummies graph, it may cause
     * rootDummy() to be null at some moments, which is a valid state.
     */
    virtual KisNodeDummy* rootDummy() const = 0;
    virtual int dummiesCount() const = 0;

    /**
     * Sometimes (e.g. on setImage() call) sigActivateNode() signal may be emitted
     * while the dummies facade is not yet connected to the node managed. In this
     * case, the activation signal would be lost. Hence we should provide the last
     * emitted value, so that node manager could cold-initialize the current node
     * value on connection.
     */
    KisNodeSP lastActivatedNode() const;

protected:
    KisImageWSP image() const;

    virtual void addNodeImpl(KisNodeSP node, KisNodeSP parent, KisNodeSP aboveThis) = 0;
    virtual void removeNodeImpl(KisNodeSP node) = 0;

Q_SIGNALS:
    /**
     * The signals for controlling the node model
     */

    void sigBeginInsertDummy(KisNodeDummy *parent, int index, const QString &metaObjectType);
    void sigEndInsertDummy(KisNodeDummy *dummy);

    void sigBeginRemoveDummy(KisNodeDummy *dummy);
    void sigEndRemoveDummy();

    void sigDummyChanged(KisNodeDummy *dummy);

    /**
     * This signal is emitted when the shape controller wants to request
     * the change of an active layer. E.g. when a new layer is added or
     * when the root layer of the image is changed. It should be forwarded
     * through a signal to allow queueing and synchronization of threads.
     */
    void sigActivateNode(KisNodeSP node);

private Q_SLOTS:
    void slotLayersChanged();
    void slotNodeChanged(KisNodeSP node);

    void slotNodeActivationRequested(KisNodeSP node, KisNodeAdditionFlags flags);

    void slotNodeAdded(KisNodeSP node, KisNodeAdditionFlags flags);
    void slotRemoveNode(KisNodeSP node);

    void slotContinueAddNode(KisNodeSP node, KisNodeSP parent, KisNodeSP aboveThis);
    void slotContinueRemoveNode(KisNodeSP node);

private:
    static KisNodeSP findFirstLayer(KisNodeSP root);

private:
    struct Private;
    Private * const m_d;
};

#endif /* __KIS_DUMMIES_FACADE_BASE_H */
