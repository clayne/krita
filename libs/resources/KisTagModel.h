/*
 * SPDX-FileCopyrightText: 2018 boud <boud@valdyas.org>
 * SPDX-FileCopyrightText: 2020 Agata Cacko <cacko.azh@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KisAllTagsModel_H
#define KisAllTagsModel_H

#include <QObject>
#include <QAbstractTableModel>
#include <QSortFilterProxyModel>

#include <KisTag.h>
#include <KoResource.h>

#include "kritaresources_export.h"

class KRITARESOURCES_EXPORT KisAbstractTagModel
{
public:

    virtual ~KisAbstractTagModel() {}

    virtual QModelIndex indexForTag(KisTagSP tag) const = 0;
    virtual KisTagSP tagForIndex(QModelIndex index = QModelIndex()) const = 0;

    /// Retrieve a tag by url
    virtual KisTagSP tagForUrl(const QString& url) const = 0;

    /// Add a new tag with a possibly empty list of resources to tag
    virtual KisTagSP addTag(const QString &tagName, const bool allowOverwrite, QVector<KoResourceSP> taggedResources)  = 0;

    /// Add a tag, if it doesn't exist yet, with a possibly empty list of resources to tag
    virtual bool addTag(const KisTagSP tag, const bool allowOverwrite, QVector<KoResourceSP> taggedResources = QVector<KoResourceSP>()) = 0;

    virtual bool setTagActive(const KisTagSP tag) = 0;
    virtual bool setTagInactive(const KisTagSP tag) = 0;
    virtual bool renameTag(const KisTagSP tag, const QString &newName, const bool allowOverwrite) = 0;
    virtual bool changeTagActive(const KisTagSP tag, bool active) = 0;
};


class KRITARESOURCES_EXPORT KisAllTagsModel
        : public QAbstractTableModel
        , public KisAbstractTagModel
{
    Q_OBJECT

public:

    enum Columns {
        Id = 0,
        Url,
        Name,
        Comment,
        ResourceType,
        Active,
        KisTagRole,
    };

    enum Ids { // to get actual id, you need to add s_fakeRowsCount
        All = -2, // so it gets on top in the combobox
        AllUntagged = -1,
    };

    ~KisAllTagsModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

// KisAllTagsModel API

    QModelIndex indexForTag(KisTagSP tag) const override;
    KisTagSP tagForIndex(QModelIndex index = QModelIndex()) const override;

    KisTagSP tagForUrl(const QString& tagUrl) const override;

    // TODO: replace ALL occurrences of KoResourceSP here with the resource id's.
    KisTagSP addTag(const QString &tagName, const bool allowOverwrite, QVector<KoResourceSP> taggedResources) override;
    bool addTag(const KisTagSP tag, const bool allowOverwrite, QVector<KoResourceSP> taggedResources = QVector<KoResourceSP>()) override;
    bool setTagActive(const KisTagSP tag) override;
    bool setTagInactive(const KisTagSP tag) override;

    bool renameTag(const KisTagSP tag, const QString &newName, const bool allowOverwrite) override;
    bool changeTagActive(const KisTagSP tag, bool active) override;

    // TODO: they can be static const variables, too, if it's better than functions
    static QString urlAll() { return "All"; }
    static QString urlAllUntagged() { return "All Untagged"; }

private Q_SLOTS:

    void addStorage(const QString &location);
    void removeStorage(const QString &location);

private:

    friend class KisResourceModelProvider;
    friend class TestTagResourceModel;
    friend class KisTagModel;

    KisAllTagsModel(const QString &resourceType, QObject *parent = 0);

    bool tagResourceByUrl(const QString& tagUrl, const int resourceId);
    bool tagResourceById(const int tagId, const int resource);

    void untagAllResources(KisTagSP tag);

    bool resetQuery();
    void closeQuery();

    struct Private;
    Private* const d;

};

class KRITARESOURCES_EXPORT KisTagModel
        : public QSortFilterProxyModel
        , public KisAbstractTagModel
{

    Q_OBJECT

public:
    KisTagModel(const QString &type, QObject *parent = 0);
    ~KisTagModel() override;

    enum TagFilter {
        ShowInactiveTags = 0,
        ShowActiveTags,
        ShowAllTags
    };

    void setTagFilter(TagFilter filter);

    enum StorageFilter {
        ShowInactiveStorages = 0,
        ShowActiveStorages,
        ShowAllStorages
    };

    void setStorageFilter(StorageFilter filter);


    // KisAllTagsModel API

    QModelIndex indexForTag(KisTagSP tag) const override;
    KisTagSP tagForIndex(QModelIndex index = QModelIndex()) const override;
    KisTagSP addTag(const QString &tagName, const bool allowOverwrite, QVector<KoResourceSP> taggedResources) override;
    KisTagSP tagForUrl(const QString& url) const override;
    bool addTag(const KisTagSP tag, const bool allowOverwrite, QVector<KoResourceSP> taggedResources = QVector<KoResourceSP>()) override;
    bool setTagInactive(const KisTagSP tag) override;
    bool setTagActive(const KisTagSP tag) override;
    bool renameTag(const KisTagSP tag, const QString &newName, const bool allowOverwrite) override;
    bool changeTagActive(const KisTagSP tag, bool active) override;

protected:

    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
    bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const override;

private:

    friend class DlgDbExplorer;
    friend class TestTagModel;


    struct Private;
    Private *const d;

    Q_DISABLE_COPY(KisTagModel)

};

typedef QSharedPointer<KisAllTagsModel> KisAllTagsModelSP;

#endif // KisAllTagsModel_H
