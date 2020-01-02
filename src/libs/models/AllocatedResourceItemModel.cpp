/* This file is part of the KDE project
 * Copyright (C) 2007 Dag Andersen <danders@get2net.dk>
 * Copyright (C) 2011, 2012 Dag Andersen <danders@get2net.dk>
 * Copyright (C) 2016 Dag Andersen <danders@get2net.dk>
 * Copyright (C) 2019 Dag Andersen <danders@get2net.dk>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 * 
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

// clazy:excludeall=qstring-arg
#include "AllocatedResourceItemModel.h"

#include "ResourceItemModel.h"
#include "kptlocale.h"
#include "kptcommonstrings.h"
#include <AddResourceCmd.h>
#include "kptcommand.h"
#include "kptitemmodelbase.h"
#include "kptcalendar.h"
#include "kptduration.h"
#include "kptnode.h"
#include "kptproject.h"
#include "kpttask.h"
#include "kptresource.h"
#include "kptdatetime.h"
#include "kptdebug.h"

#include <KoIcon.h>

#include <QMimeData>
#include <QMimeDatabase>
#include <QStringList>
#include <QLocale>

#include <KIO/TransferJob>
#include <KIO/StatJob>

#ifdef PLAN_KCONTACTS_FOUND
#include <KContacts/Addressee>
#include <KContacts/VCardConverter>
#endif


using namespace KPlato;

AllocatedResourceItemModel::AllocatedResourceItemModel(QObject *parent)
    : QSortFilterProxyModel(parent),
    m_task(0)
{
    setDynamicSortFilter(true);
    setSourceModel(new ResourceItemModel(this));
}

int AllocatedResourceItemModel::columnCount(const QModelIndex &idx) const
{
    Q_UNUSED(idx);
    return 2;
}

Project *AllocatedResourceItemModel::project() const
{
    return static_cast<ResourceItemModel*>(sourceModel())->project();
}

void AllocatedResourceItemModel::setProject(Project *project)
{
    debugPlan<<this->project()<<"="<<project;
    Project *p =this->project();
    if (p) {
        disconnect(p, &Project::nodeChanged, this, &AllocatedResourceItemModel::slotNodeChanged);
    }
    static_cast<ResourceItemModel*>(sourceModel())->setProject(project);
    if (project) {
        connect(project, &Project::nodeChanged, this, &AllocatedResourceItemModel::slotNodeChanged);
    }
    debugPlan<<rowCount()<<":"<<sourceModel()->rowCount();
}

void AllocatedResourceItemModel::reset()
{
    beginResetModel();
    endResetModel();
    emit expandAll();
    emit resizeColumnToContents(0);
}

void AllocatedResourceItemModel::slotNodeChanged(Node *n)
{
    debugPlan<<(n==m_task)<<n<<n->name();
    if (n != m_task) {
        return;
    }
    reset();
}

Task *AllocatedResourceItemModel::task() const
{
    return m_task;
}

void AllocatedResourceItemModel::setTask(Task *task)
{
    debugPlan<<m_task<<"="<<task<<(task?task->name():"");
    m_task = task;
    reset();
    debugPlan<<rowCount()<<":"<<sourceModel()->rowCount();
}

QObject* AllocatedResourceItemModel::object(const QModelIndex& idx) const
{
    return nullptr; //TODO static_cast<ResourceItemModel*>(sourceModel())->object(mapToSource(idx));
}

Resource *AllocatedResourceItemModel::resource(const QModelIndex &idx) const
{
    QModelIndex sidx = mapToSource(idx);
    if (sidx.isValid() && sidx.internalPointer() == nullptr) {
        return project()->resourceAt(sidx.row());
    }
    return nullptr;
}

QModelIndex AllocatedResourceItemModel::index(Resource *r) const
{
    return mapFromSource(static_cast<ResourceItemModel*>(sourceModel())->index(project()->indexOf(r), 0));
}

Qt::ItemFlags AllocatedResourceItemModel::flags(const QModelIndex & index) const
{
    Qt::ItemFlags f = QSortFilterProxyModel::flags(index);
    f &= ~Qt::ItemIsUserCheckable;
    return f;
}


QVariant AllocatedResourceItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (section == 1) {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
            return xi18nc("@title:column", "Allocation");
        }
        return QVariant();
    }
    return QSortFilterProxyModel::headerData(section, orientation, role);
}

QVariant AllocatedResourceItemModel::allocation(const Resource *res, int role) const
{
    ResourceRequest *rr = m_task->requests().find(res);
    ResourceGroupRequest *gr = m_task->requests().find(res->parentGroups().value(0));
    if (rr == 0 || gr == 0) {
        return QVariant();
    }
    switch (role) {
        case Qt::DisplayRole: {
        case Qt::EditRole:
            // xgettext: no-c-format
            return i18nc("<value>%", "%1%",rr->units());
        }
        case Qt::ToolTipRole: {
            if (rr->units() == 0) {
                return xi18nc("@info:tooltip", "Not allocated");
            }
            return xi18nc("@info:tooltip", "%1 allocated out of %2 available", gr->count(), res->parentGroups().value(0)->numResources());
        }
        default:
            break;
    }
    return QVariant();
}

QVariant AllocatedResourceItemModel::allocation(const ResourceGroup *res, int role) const
{
    ResourceGroupRequest *gr = m_task->requests().find(res);
    if (gr == 0) {
        return QVariant();
    }
    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return QString("%1 (%2)").arg(gr->units()).arg(gr->count());
        case Qt::ToolTipRole: {
            QString s1 = i18ncp("@info:tooltip",
                                 "%1 resource requested for dynamic allocation",
                                 "%1 resources requested for dynamic allocation",
                                 gr->units());
            QString s2 = i18ncp("@info:tooltip",
                                 "%1 resource allocated",
                                 "%1 resources allocated",
                                 gr->count());

            return xi18nc("@info:tooltip", "%1<nl/>%2", s1, s2);
        }
        case Qt::WhatsThisRole: {
            return xi18nc("@info:whatsthis",
                          "<title>Group allocations</title>"
                          "<para>You can allocate a number of resources from a group and let"
                          " the scheduler select from the available resources at the time of scheduling.</para>"
                          " These dynamically allocated resources will be in addition to any resource you have allocated specifically.");
        }
        case Role::Minimum: {
            return 0;
        }
        case Role::Maximum: {
            return res->numResources() - gr->units();
        }
        default:
            break;
    }
    return QVariant();
}

QVariant AllocatedResourceItemModel::data(const QModelIndex& idx, int role) const
{
    if (m_task == 0 || role == Qt::CheckStateRole || role == Qt::DecorationRole) {
        return QVariant();
    }
    if (idx.column() == 1) {
        switch (role) {
            case Qt::TextAlignmentRole:
                return Qt::AlignLeft;
            default: {
                QObject *o = object(idx);
                Resource *r = qobject_cast<Resource*>(o);
                if (r) {
                    return allocation(r, role);
                }
                ResourceGroup *g = qobject_cast<ResourceGroup*>(o);
                if (g) {
                    return allocation(g, role);
                }
                break;
            }
            return QVariant();
        }
    }
    return QSortFilterProxyModel::data(idx, role);
}

bool AllocatedResourceItemModel::filterAcceptsRow(int source_row, const QModelIndex & source_parent) const
{
    if (m_task == 0) {
        return false;
    }
    QModelIndex idx = sourceModel()->index(source_row, 0, source_parent);
    if (! idx.isValid()) {
        return false;
    }
    bool result = false;
    const ResourceRequestCollection &req = m_task->requests();
    if (source_parent.isValid()) {
        const Resource *r = static_cast<ResourceItemModel*>(sourceModel())->resource(idx);
        result = (bool) req.find(r);
    } else {
        const ResourceGroup *g = static_cast<ResourceItemModel*>(sourceModel())->group(idx);
        ResourceGroupRequest *gr = req.find(g);
        result = (bool) gr && (gr->units() > 0 || gr->count() > 0);
    }
    debugPlan<<result<<":"<<source_parent<<idx;
    return result;
}
