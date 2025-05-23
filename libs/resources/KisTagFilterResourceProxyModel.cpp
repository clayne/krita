/*
 * SPDX-FileCopyrightText: 2018 Boudewijn Rempt <boud@valdyas.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */
#include "KisTagFilterResourceProxyModel.h"

#include <QDebug>

#include <KisResourceModelProvider.h>
#include <KisResourceModel.h>
#include <KisTagResourceModel.h>
#include <KisTagModel.h>
#include <KisResourceMetaDataModel.h>

#include <kis_debug.h>
#include <KisResourceSearchBoxFilter.h>

struct KisTagFilterResourceProxyModel::Private
{
    Private()
        : filter(new KisResourceSearchBoxFilter())
    {
    }

    QString resourceType;

    KisResourceModel *resourceModel {0}; // This is the source model if we are _not_ filtering by tag
    KisTagResourceModel *tagResourceModel {0}; // This is the source model if we are filtering by tag

    QScopedPointer<KisResourceSearchBoxFilter> filter;
    bool filteringWithinCurrentTag {false};

    QMap<QString, QVariant> metaDataMapFilter;
    KisTagSP currentTagFilter;
    KoResourceSP currentResourceFilter;

    int storageId {-1};
    bool useStorageIdFilter {false};

};

KisTagFilterResourceProxyModel::KisTagFilterResourceProxyModel(const QString &resourceType, QObject *parent)
    : QSortFilterProxyModel(parent)
    , d(new Private)
{
    d->resourceType = resourceType;
    d->resourceModel = new KisResourceModel(resourceType);
    d->tagResourceModel = new KisTagResourceModel(resourceType);

    setSourceModel(d->resourceModel);
}

KisTagFilterResourceProxyModel::~KisTagFilterResourceProxyModel()
{
    delete d->resourceModel;
    delete d->tagResourceModel;
    delete d;
}

void KisTagFilterResourceProxyModel::setResourceFilter(ResourceFilter filter)
{
    Q_EMIT beforeFilterChanges();
    d->resourceModel->setResourceFilter(filter);
    d->tagResourceModel->setResourceFilter(filter);
    invalidateFilter();
    Q_EMIT afterFilterChanged();
}

void KisTagFilterResourceProxyModel::setStorageFilter(StorageFilter filter)
{
    Q_EMIT beforeFilterChanges();
    d->resourceModel->setStorageFilter(filter);
    d->tagResourceModel->setStorageFilter(filter);
    invalidateFilter();
    Q_EMIT afterFilterChanged();
}

void KisTagFilterResourceProxyModel::setResourceModel(KisResourceModel *resourceModel)
{
    d->resourceModel = resourceModel;
}

KoResourceSP KisTagFilterResourceProxyModel::resourceForIndex(QModelIndex index) const
{
    KisAbstractResourceModel *source = dynamic_cast<KisAbstractResourceModel*>(sourceModel());
    if (source) {
        return source->resourceForIndex(mapToSource(index));
    }
    return 0;
}

QModelIndex KisTagFilterResourceProxyModel::indexForResource(KoResourceSP resource) const
{
    if (!resource || !resource->valid() || resource->resourceId() < 0) return QModelIndex();

    for (int i = 0; i < rowCount(); ++i)  {
        QModelIndex idx = index(i, 0);
        Q_ASSERT(idx.isValid());
        if (idx.data() == resource->resourceId()) {
            return idx;
        }
    }
    return QModelIndex();
}

QModelIndex KisTagFilterResourceProxyModel::indexForResourceId(int resourceId) const
{
    if (resourceId < 0) return QModelIndex();
    KisAbstractResourceModel *source = dynamic_cast<KisAbstractResourceModel*>(sourceModel());
    if (source) {
        return mapFromSource(source->indexForResourceId(resourceId));
    }
    return QModelIndex();
}

bool KisTagFilterResourceProxyModel::setResourceActive(const QModelIndex &index, bool value)
{
    KisAbstractResourceModel *source = dynamic_cast<KisAbstractResourceModel*>(sourceModel());
    if (source) {
        return source->setResourceActive(mapToSource(index), value);
    }
    return false;
}

KoResourceSP KisTagFilterResourceProxyModel::importResourceFile(const QString &filename, const bool allowOverwrite, const QString &storageId)
{
    KisAbstractResourceModel *source = dynamic_cast<KisAbstractResourceModel*>(sourceModel());
    KoResourceSP res;
    if (source) {
        res = source->importResourceFile(filename, allowOverwrite, storageId);
    }
    return res;
}

KoResourceSP KisTagFilterResourceProxyModel::importResource(const QString &filename, QIODevice *device, const bool allowOverwrite, const QString &storageId)
{
    KisAbstractResourceModel *source = dynamic_cast<KisAbstractResourceModel*>(sourceModel());
    KoResourceSP res;
    if (source) {
        res = source->importResource(filename, device, allowOverwrite, storageId);
    }
    return res;
}

bool KisTagFilterResourceProxyModel::importWillOverwriteResource(const QString &fileName, const QString &storageLocation) const
{
    KisAbstractResourceModel *source = dynamic_cast<KisAbstractResourceModel*>(sourceModel());
    return source && source->importWillOverwriteResource(fileName, storageLocation);
}

bool KisTagFilterResourceProxyModel::exportResource(KoResourceSP resource, QIODevice *device)
{
    KisAbstractResourceModel *source = dynamic_cast<KisAbstractResourceModel*>(sourceModel());
    bool res = false;
    if (source) {
        res = source->exportResource(resource, device);
    }
    return res;
}

bool KisTagFilterResourceProxyModel::addResource(KoResourceSP resource, const QString &storageId)
{
    KisAbstractResourceModel *source = dynamic_cast<KisAbstractResourceModel*>(sourceModel());
    if (source) {
        return source->addResource(resource, storageId);
    }
    return false;
}

bool KisTagFilterResourceProxyModel::updateResource(KoResourceSP resource)
{
    KisAbstractResourceModel *source = dynamic_cast<KisAbstractResourceModel*>(sourceModel());
    if (source) {
        return source->updateResource(resource);
    }
    return false;
}

bool KisTagFilterResourceProxyModel::reloadResource(KoResourceSP resource)
{
    KisAbstractResourceModel *source = dynamic_cast<KisAbstractResourceModel*>(sourceModel());
    if (source) {
        return source->reloadResource(resource);
    }
    return false;
}

bool KisTagFilterResourceProxyModel::renameResource(KoResourceSP resource, const QString &name)
{
    KisAbstractResourceModel *source = dynamic_cast<KisAbstractResourceModel*>(sourceModel());
    if (source) {
        return source->renameResource(resource, name);
    }
    return false;
}

bool KisTagFilterResourceProxyModel::setResourceMetaData(KoResourceSP resource, QMap<QString, QVariant> metadata)
{
    KisAbstractResourceModel *source = dynamic_cast<KisAbstractResourceModel*>(sourceModel());
    if (source) {
        return source->setResourceMetaData(resource, metadata);
    }
    return false;
}

bool KisTagFilterResourceProxyModel::additionalResourceNameChecks(const QModelIndex &index, const KisResourceSearchBoxFilter *filter) const
{
    Q_UNUSED(index)
    Q_UNUSED(filter)
    return false;
}

void KisTagFilterResourceProxyModel::setMetaDataFilter(QMap<QString, QVariant> metaDataMap)
{
    Q_EMIT beforeFilterChanges();
    d->metaDataMapFilter = metaDataMap;
    invalidateFilter();
    Q_EMIT afterFilterChanged();
}

void KisTagFilterResourceProxyModel::setTagFilter(const KisTagSP tag)
{
    d->currentTagFilter = tag;
    updateTagFilter();
}

KisTagSP KisTagFilterResourceProxyModel::currentTagFilter() const
{
    return d->currentTagFilter;
}

void KisTagFilterResourceProxyModel::setStorageFilter(bool useFilter, int storageId)
{
    Q_EMIT beforeFilterChanges();
    d->useStorageIdFilter = useFilter;
    if (useFilter) {
        d->storageId = storageId;
    }
    invalidateFilter();
    Q_EMIT afterFilterChanged();
}

void KisTagFilterResourceProxyModel::updateTagFilter()
{
    Q_EMIT beforeFilterChanges();
    const bool ignoreTagFiltering =
        !d->filteringWithinCurrentTag && !d->filter->isEmpty();

    QAbstractItemModel *desiredModel = 0;

    if (d->currentResourceFilter) {
        QVector<KisTagSP> filter;

        if (d->currentTagFilter &&
            !ignoreTagFiltering &&
            d->currentTagFilter->url() != KisAllTagsModel::urlAll() &&
            d->currentTagFilter->url() != KisAllTagsModel::urlAllUntagged()) {

            filter << d->currentTagFilter;
        } else {
            // combination with for untagged resources in not implemented
            // in KisTagResourceModel
            KIS_SAFE_ASSERT_RECOVER_NOOP(!d->currentTagFilter ||
                                         d->currentTagFilter->url() != KisAllTagsModel::urlAllUntagged());
        }

        d->tagResourceModel->setTagsFilter(filter);
        d->tagResourceModel->setResourcesFilter({d->currentResourceFilter});
        desiredModel = d->tagResourceModel;

    } else {
        d->tagResourceModel->setResourcesFilter(QVector<KoResourceSP>());
        if (ignoreTagFiltering ||
                !d->currentTagFilter ||
                d->currentTagFilter->url() == KisAllTagsModel::urlAll()) {

            d->tagResourceModel->setTagsFilter(QVector<KisTagSP>());
            desiredModel = d->resourceModel;
            d->resourceModel->showOnlyUntaggedResources(false);
        }
        else {
            if (d->currentTagFilter->url() == KisAllTagsModel::urlAllUntagged()) {
                desiredModel = d->resourceModel;
                d->resourceModel->showOnlyUntaggedResources(true);
            }
            else {
                desiredModel = d->tagResourceModel;
                d->tagResourceModel->setTagsFilter(QVector<KisTagSP>() << d->currentTagFilter);
            }
        }
    }

    // TODO: when model changes the current selection in the
    //       view disappears. We should try to keep it somehow.
    if (sourceModel() != desiredModel) {
        setSourceModel(desiredModel);
    }

    invalidateFilter();
    Q_EMIT afterFilterChanged();
}

void KisTagFilterResourceProxyModel::setResourceFilter(const KoResourceSP resource)
{
    d->currentResourceFilter = resource;
    updateTagFilter();
}

void KisTagFilterResourceProxyModel::setSearchText(const QString& searchText)
{
    d->filter->setFilter(searchText);
    updateTagFilter();
}

void KisTagFilterResourceProxyModel::setFilterInCurrentTag(bool filterInCurrentTag)
{
    d->filteringWithinCurrentTag = filterInCurrentTag;
    updateTagFilter();
}

bool KisTagFilterResourceProxyModel::filterInCurrentTag() const
{
    return d->filteringWithinCurrentTag;
}

bool KisTagFilterResourceProxyModel::tagResources(const KisTagSP tag, const QVector<int> &resourceIds)
{
    return d->tagResourceModel->tagResources(tag, resourceIds);
}

bool KisTagFilterResourceProxyModel::untagResources(const KisTagSP tag, const QVector<int> &resourceIds)
{
    return d->tagResourceModel->untagResources(tag, resourceIds);
}

int KisTagFilterResourceProxyModel::isResourceTagged(const KisTagSP tag, const int resourceId)
{
    return d->tagResourceModel->isResourceTagged(tag, resourceId);
}

bool KisTagFilterResourceProxyModel::filterAcceptsColumn(int /*source_column*/, const QModelIndex &/*source_parent*/) const
{
    return true;
}

bool KisTagFilterResourceProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    // if both filters are empty, just accept everything
    if (d->filter->isEmpty() && d->metaDataMapFilter.isEmpty() && !d->useStorageIdFilter) {
        return true;
    }

    // If there's a tag set to filter on, we use the tagResourceModel, so that already filters for the tag
    // Here, we only have to filter by the search string.
    QModelIndex idx = sourceModel()->index(source_row, 0, source_parent);

    if (!idx.isValid()) {
        return false;
    }

    // checking the storage filter
    if (d->useStorageIdFilter) {
        int storageId = sourceModel()->data(idx, Qt::UserRole + KisAbstractResourceModel::StorageId).toInt();
        if (storageId != d->storageId) {
            return false;
        }
    }

    KisResourceMetaDataModel *metaDataModel = KisResourceModelProvider::resourceMetadataModel();
    const int resourceId = sourceModel()->data(idx, Qt::UserRole + KisAbstractResourceModel::Id).toInt();
    Q_FOREACH(const QString &key, d->metaDataMapFilter.keys()) {
        const QVariant value = metaDataModel->metaDataValue(resourceId, key);
        if (value.isValid() && value != d->metaDataMapFilter[key]) {
                return false;
        }
    }

    QString resourceName = sourceModel()->data(idx, Qt::UserRole + KisAbstractResourceModel::Name).toString();
    if (sourceModel()->data(idx, Qt::UserRole + KisAbstractResourceModel::ResourceType).toString() == ResourceType::PaintOpPresets) {
        resourceName = resourceName.replace("_", " ");
    }
    QStringList resourceTags = sourceModel()->data(idx, Qt::UserRole + KisAbstractResourceModel::Tags).toStringList();
    bool resourceNameMatches = d->filter->matchesResource(resourceName, resourceTags);
    if (!resourceNameMatches) {
        resourceNameMatches = additionalResourceNameChecks(idx, d->filter.data());
    }


    return resourceNameMatches;
}

bool KisTagFilterResourceProxyModel::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const
{
    QString nameLeft = sourceModel()->data(source_left, Qt::UserRole + KisAbstractResourceModel::Name).toString();
    QString nameRight = sourceModel()->data(source_right, Qt::UserRole + KisAbstractResourceModel::Name).toString();
    return nameLeft.toLower() < nameRight.toLower();
}

