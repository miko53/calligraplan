/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "ProgressView.h"
#include "MainDocument.h"
#include "ProjectsModel.h"

#include <kptproject.h>
#include <kpttaskstatusview.h>
#include <kptnodeitemmodel.h>

#include <KoComponentData.h>
#include <KoPart.h>

#include <KRecentFilesAction>

#include <QTreeView>
#include <QVBoxLayout>
#include <QItemSelectionModel>
#include <QSplitter>

ProgressView::ProgressView(KoPart *part, KoDocument *doc, QWidget *parent)
    : KoView(part, doc, parent)
    , m_readWrite(false)
{
    //debugPlan;
    if (doc && doc->isReadWrite()) {
        setXMLFile(QStringLiteral("Portfolio_ProgressViewUi.rc"));
    }
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    QSplitter *sp = new QSplitter(this);
    layout->addWidget(sp);
    m_view = new QTreeView(sp);
    m_view->setRootIsDecorated(false);
    sp->addWidget(m_view);
    ProjectsFilterModel *m = new ProjectsFilterModel(sp);
    m->setPortfolio(qobject_cast<MainDocument*>(doc));
    m->setAcceptedColumns(QList<int>() << KPlato::NodeModel::NodeName);
    m_view->setModel(m);

    m_detailsView = new KPlato::TaskStatusView(part, doc, sp);
    const QList<int> show = QList<int>()
    << KPlato::NodeModel::NodeName
    << KPlato::NodeModel::NodeStatus
    << KPlato::NodeModel::NodeCompleted
    << KPlato::NodeModel::NodeActualEffort
    << KPlato::NodeModel::NodeRemainingEffort
    << KPlato::NodeModel::NodePlannedEffort
    << KPlato::NodeModel::NodePlannedCost
    << KPlato::NodeModel::NodeActualCost
    << KPlato::NodeModel::NodeActualStart
    << KPlato::NodeModel::NodeActualFinish
    << KPlato::NodeModel::NodeDescription;
    m_detailsView->showColumns(show);
    m_detailsView->setViewSplitMode(false);
    insertChildClient(m_detailsView);

    connect(m_view->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ProgressView::selectionChanged);

    sp->setStretchFactor(1, 10);
}

ProgressView::~ProgressView()
{
}

void ProgressView::updateReadWrite(bool readwrite)
{
    m_readWrite = readwrite;
}

QMenu * ProgressView::popupMenu(const QString& name)
{
    Q_UNUSED(name)
    return nullptr;
}

KoPrintJob *ProgressView::createPrintJob()
{
    return m_detailsView->createPrintJob();
}

void ProgressView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(deselected)
    auto selectedIndex = selected.indexes().value(0);
    const ProjectsFilterModel *m = qobject_cast<ProjectsFilterModel*>(m_view->model());
    Q_ASSERT(m);
    KoDocument *doc = m->documentFromIndex(selectedIndex);
    if (doc) {
        m_detailsView->setProject(doc->project());
        m_detailsView->setScheduleManager(m->portfolio()->scheduleManager(doc));
        m_detailsView->setCommandDocument(doc);
        m_detailsView->updateReadWrite(doc->isReadWrite());
        Q_EMIT m_detailsView->expandAll();
    }
}
