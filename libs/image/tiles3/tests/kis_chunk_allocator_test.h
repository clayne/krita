/*
 *  SPDX-FileCopyrightText: 2010 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_CHUNK_ALLOCATOR_TEST_H
#define KIS_CHUNK_ALLOCATOR_TEST_H

#include <simpletest.h>

#include <QRandomGenerator>

class KisChunkAllocatorTest : public QObject
{
    Q_OBJECT

private:
qreal measureFragmentation(qint32 transactions, qint32 chunksAlloc,
                           qint32 chunksFree, bool printDetails);

private Q_SLOTS:
    void testOperations();
    void testFragmentation();

private:
    quint64 getChunkSize();

    QRandomGenerator m_rng{};
};

#endif /* KIS_CHUNK_ALLOCATOR_TEST_H */

