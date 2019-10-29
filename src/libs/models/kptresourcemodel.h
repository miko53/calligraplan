/* This file is part of the KDE project
  Copyright (C) 2007 Dag Andersen <danders@get2net>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
* Boston, MA 02110-1301, USA.
*/

#ifndef KPTRESOURCEMODEL_H
#define KPTRESOURCEMODEL_H

#include "planmodels_export.h"

#include <kptitemmodelbase.h>

#include <QSortFilterProxyModel>
#include <QMetaEnum>

class QByteArray;

namespace KIO {
    class Job;
}
class KJob;

namespace KPlato
{

class Project;
class Resource;
class ResourceGroup;
class Calendar;
class Task;
class Node;

class PLANMODELS_EXPORT ResourceModel : public QObject
{
    Q_OBJECT
public:
    explicit ResourceModel(QObject *parent = 0);
    ~ResourceModel() override;

    enum Properties {
        ResourceName = 0,
        ResourceScope,
        ResourceType,
        ResourceInitials,
        ResourceEmail,
        ResourceCalendar,
        ResourceLimit,
        ResourceAvailableFrom,
        ResourceAvailableUntil,
        ResourceNormalRate,
        ResourceOvertimeRate,
        ResourceAccount
    };
    Q_ENUM(Properties)
    
    const QMetaEnum columnMap() const;
    void setProject(Project *project);
    int propertyCount() const;
    QVariant data(const Resource *resource, int property, int role = Qt::DisplayRole) const;
    QVariant data(const ResourceGroup *group, int property, int role = Qt::DisplayRole) const;
    static QVariant headerData(int section, int role = Qt::DisplayRole);

    QVariant name(const Resource *res, int role) const;
    QVariant scope(const Resource *res, int role) const;
    QVariant type(const Resource *res, int role) const;
    QVariant initials(const Resource *res, int role) const;
    QVariant email(const Resource *res, int role) const;
    QVariant calendar(const Resource *res, int role) const;
    QVariant units(const Resource *res, int role) const;
    QVariant availableFrom(const Resource *res, int role) const;
    QVariant availableUntil(const Resource *res, int role) const;
    QVariant normalRate(const Resource *res, int role) const;
    QVariant overtimeRate(const Resource *res, int role) const;
    QVariant account(const Resource *res, int role) const;
    
    QVariant name(const ResourceGroup *res, int role) const;
    QVariant scope(const ResourceGroup *res, int role) const;
    QVariant type(const ResourceGroup *res, int role) const;

private:
    Project *m_project;
};

class PLANMODELS_EXPORT ResourceItemModel : public ItemModelBase
{
    Q_OBJECT
public:
    explicit ResourceItemModel(QObject *parent = 0);
    ~ResourceItemModel() override;

    const QMetaEnum columnMap() const override { return m_model.columnMap(); }

    void setProject(Project *project) override;

    Qt::ItemFlags flags(const QModelIndex & index) const override;

    QModelIndex parent(const QModelIndex & index) const override;
    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const override;
    QModelIndex index(const ResourceGroup *group, int column = 0) const;
    QModelIndex index(const Resource *resource, int column = 0) const;

    int columnCount(const QModelIndex & parent = QModelIndex()) const override; 
    int rowCount(const QModelIndex & parent = QModelIndex()) const override; 

    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override; 
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) override;


    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    QStringList mimeTypes () const override;
    Qt::DropActions supportedDropActions() const override;
    bool dropAllowed(const QModelIndex &index, int dropIndicatorPosition, const QMimeData *data) override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
    QMimeData *mimeData(const QModelIndexList & indexes) const override;
    
    QAbstractItemDelegate *createDelegate(int col, QWidget *parent) const override;
    
    QObject *object(const QModelIndex &index) const;
    ResourceGroup *group(const QModelIndex &index) const;
    Resource *resource(const QModelIndex &index) const;
    QModelIndex insertGroup(ResourceGroup *g);
    QModelIndex insertResource(ResourceGroup *g, Resource *r, Resource *after = 0);

    int sortRole(int column) const override;

protected Q_SLOTS:
    void slotResourceChanged(KPlato::Resource*);
    void slotResourceGroupChanged(KPlato::ResourceGroup *);
    void slotResourceGroupToBeInserted(const KPlato::ResourceGroup *group, int row);
    void slotResourceGroupInserted(const KPlato::ResourceGroup *group);
    void slotResourceGroupToBeRemoved(const KPlato::ResourceGroup *group);
    void slotResourceGroupRemoved(const KPlato::ResourceGroup *group);
    void slotResourceToBeInserted(const KPlato::ResourceGroup *group, int row);
    void slotResourceInserted(const KPlato::Resource *resource);
    void slotResourceToBeRemoved(const KPlato::Resource *resource);
    void slotResourceRemoved(const KPlato::Resource *resource);
    void slotCalendarChanged(KPlato::Calendar* cal);

    void slotDataArrived(KIO::Job *job, const QByteArray &data  );
    void slotJobFinished(KJob *job);

protected:
    QVariant notUsed(const ResourceGroup *res, int role) const;
    
    QVariant name(const ResourceGroup *res, int role) const;
    bool setName(Resource *res, const QVariant &value, int role);
    bool setName(ResourceGroup *res, const QVariant &value, int role);
    
    QVariant type(const ResourceGroup *res, int role) const;
    bool setType(Resource *res, const QVariant &value, int role);
    bool setType(ResourceGroup *res, const QVariant &value, int role);

    bool setInitials(Resource *res, const QVariant &value, int role);
    bool setEmail(Resource *res, const QVariant &value, int role);
    bool setCalendar(Resource *res, const QVariant &value, int role);
    bool setUnits(Resource *res, const QVariant &value, int role);
    bool setAvailableFrom(Resource *res, const QVariant &value, int role);
    bool setAvailableUntil(Resource *res, const QVariant &value, int role);
    bool setNormalRate(Resource *res, const QVariant &value, int role);
    bool setOvertimeRate(Resource *res, const QVariant &value, int role);
    bool setAccount(Resource *res, const QVariant &value, int role);

    QList<Resource*> resourceList(QDataStream &stream);

    bool createResources(ResourceGroup *group, const QByteArray &data);

private:
    ResourceGroup *m_group; // Used for sanity checks
    Resource *m_resource; // Used for sanity checks
    ResourceModel m_model;

    struct DropData {
        Qt::DropAction action;
        int row;
        int column;
        QModelIndex parent;
        QByteArray data;
    } m_dropData;
    QMap<KJob*, DropData> m_dropDataMap;
};

class PLANMODELS_EXPORT ResourceItemSFModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit ResourceItemSFModel(QObject *parent = 0);

    void setProject(Project *project);
    Resource *resource(const QModelIndex &index) const;
    using QAbstractProxyModel::index;
    QModelIndex index(Resource *r) const;

    Qt::ItemFlags flags(const QModelIndex & index) const override;
    
    void addFilteredResource(const Resource *r);
protected:
    bool filterAcceptsRow(int source_row, const QModelIndex & source_parent) const override;
    
    QList<const Resource*> m_filteredResources;
};

class PLANMODELS_EXPORT AllocatedResourceItemModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit AllocatedResourceItemModel(QObject *parent = 0);

    int columnCount(const QModelIndex &idx) const override;

    Project *project() const;
    Task *task() const;
    Resource *resource(const QModelIndex &index) const;
    using QAbstractProxyModel::index;
    QModelIndex index(Resource *r) const;

    Qt::ItemFlags flags(const QModelIndex & index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QVariant data(const QModelIndex &idx, int role) const override;

public Q_SLOTS:
    void setProject(KPlato::Project *project);
    void setTask(KPlato::Task *task);

Q_SIGNALS:
    void expandAll();
    void resizeColumnToContents(int);

protected Q_SLOTS:
    void slotNodeChanged(KPlato::Node *n);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex & source_parent) const override;
    void reset();
    QObject *object(const QModelIndex &idx) const;

    QVariant allocation(const Resource *r, int role) const;
    QVariant allocation(const ResourceGroup *g, int role) const;

    Task *m_task;
};

}  //KPlato namespace

#endif
