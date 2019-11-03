/* This file is part of the KDE project
 *  Copyright (C) 2019 Dag Andersen <danders@get2net.dk>
 *  Copyright (C) 2006-2011, 2012 Dag Andersen <danders@get2net.dk>
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
#include "kptscheduleeditor.h"

#include "kptcommand.h"
#include "kptcalendar.h"
#include "kptduration.h"
#include "kptnode.h"
#include "kptproject.h"
#include "kpttask.h"
#include "kptschedule.h"
#include "kptdatetime.h"
#include "kptpertresult.h"
#include "kptitemviewsettup.h"
#include "kptrecalculatedialog.h"
#include "Help.h"
#include "kptdebug.h"

#include <KoDocument.h>
#include <KoIcon.h>

#include <QList>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QAction>
#include <QMenu>

#include <KLocalizedString>
#include <kactioncollection.h>

#include <ktoggleaction.h>


namespace KPlato
{

ScheduleTreeView::ScheduleTreeView(QWidget *parent)
    : TreeViewBase(parent)
{
    header()->setStretchLastSection (false);

    ScheduleItemModel *m = new ScheduleItemModel(this);
    setModel(m);
    //setSelectionBehavior(QAbstractItemView::SelectItems);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setTreePosition(-1); // always visual index 0

    createItemDelegates(m);
}

void ScheduleTreeView::selectionChanged(const QItemSelection &sel, const QItemSelection &desel)
{
    //debugPlan<<sel.indexes().count();
    foreach(const QModelIndex &i, selectionModel()->selectedIndexes()) {
        Q_UNUSED(i);
        //debugPlan<<i.row()<<","<<i.column();
    }
    QTreeView::selectionChanged(sel, desel);
    emit selectionChanged(selectionModel()->selectedIndexes());
}

void ScheduleTreeView::currentChanged(const QModelIndex & current, const QModelIndex & previous)
{
    //debugPlan<<current.row()<<","<<current.column();
    QTreeView::currentChanged(current, previous);
    emit currentChanged(current);
    // possible bug in qt: in QAbstractItemView::SingleSelection you can select multiple items/rows
    selectionModel()->select(current, QItemSelectionModel::Rows | QItemSelectionModel::ClearAndSelect);
}

ScheduleManager *ScheduleTreeView::manager(const QModelIndex &idx) const
{
    return model()->manager(idx);
}

ScheduleManager *ScheduleTreeView::currentManager() const
{
    return model()->manager(currentIndex());
}

QModelIndexList ScheduleTreeView::selectedRows() const
{
    QModelIndexList lst = selectionModel()->selectedRows();
    debugPlan<<lst;
    return lst;
}

ScheduleManager *ScheduleTreeView::selectedManager() const
{
    ScheduleManager *sm = 0;
    QModelIndexList lst = selectedRows();
    if (lst.count() == 1) {
        sm = model()->manager(lst.first());
    }
    return sm;
}

//-----------------------------------
ScheduleEditor::ScheduleEditor(KoPart *part, KoDocument *doc, QWidget *parent)
    : ViewBase(part, doc, parent)
{
    if (doc && doc->isReadWrite()) {
        setXMLFile("ScheduleEditorUi.rc");
    } else {
        setXMLFile("ScheduleEditorUi_readonly.rc");
    }

    setupGui();
    slotEnableActions();

    QVBoxLayout * l = new QVBoxLayout(this);
    l->setMargin(0);
    m_schedulingRange = new SchedulingRange(doc, this);
    l->addWidget(m_schedulingRange);
    m_view = new ScheduleTreeView(this);
    connect(this, &ViewBase::expandAll, m_view, &TreeViewBase::slotExpand);
    connect(this, &ViewBase::collapseAll, m_view, &TreeViewBase::slotCollapse);

    l->addWidget(m_view);
    m_view->setEditTriggers(m_view->editTriggers() | QAbstractItemView::EditKeyPressed);

    QList<int> show;
    show << ScheduleModel::ScheduleName
        << ScheduleModel::ScheduleState
        << ScheduleModel::ScheduleDirection
        << ScheduleModel::ScheduleOverbooking
        << ScheduleModel::ScheduleDistribution
        << ScheduleModel::SchedulePlannedStart
        << ScheduleModel::SchedulePlannedFinish
        << ScheduleModel::ScheduleScheduler
        << ScheduleModel::ScheduleGranularity
        << ScheduleModel::ScheduleMode
        ;

    QList<int> lst;
    for (int c = 0; c < model()->columnCount(); ++c) {
        if (! show.contains(c)) {
            lst << c;
        }
    }
    m_view->setColumnsHidden(lst);
    m_view->setDefaultColumns(show);


    connect(model(), &ItemModelBase::executeCommand, doc, &KoDocument::addCommand);

    connect(m_view, SIGNAL(currentChanged(QModelIndex)), this, SLOT(slotCurrentChanged(QModelIndex)));

    connect(m_view, SIGNAL(selectionChanged(QModelIndexList)), this, SLOT(slotSelectionChanged(QModelIndexList)));

    connect(model(), &QAbstractItemModel::dataChanged, this, &ScheduleEditor::updateActionsEnabled);

    connect(m_view, &TreeViewBase::contextMenuRequested, this, &ScheduleEditor::slotContextMenuRequested);

    connect(m_view, &TreeViewBase::headerContextMenuRequested, this, &ViewBase::slotHeaderContextMenuRequested);

    Help::add(this,
        xi18nc("@info:whatsthis", 
               "<title>Schedule Editor</title>"
               "<para>"
               "The Schedule Editor is used to create, edit, calculate and delete schedules."
               "</para><para>"
               "If <emphasis>Mode</emphasis> is set to <emphasis>Auto</emphasis>, the schedule is calculated automatically."
               " <note>Only one schedule can be in <emphasis>Auto Mode</emphasis> simultaneously"
               " and it must be a top level schedule without sub-schedules.</note>"
               "</para><para>"
               "A schedule can have sub-schedules. A sub-schedule can use the projects progress data"
               " in order to reschedule only tasks that are not yet finished."
               " Rescheduling will then use e.g. actual start and remaining effort for the tasks."
               "<nl/><link url='%1'>More...</link>"
               "</para>", Help::page("Schedules_Editor")));
}

void ScheduleEditor::draw(Project &project)
{
    m_view->setProject(&project);
    m_schedulingRange->setProject(&project);
}

void ScheduleEditor::draw()
{
}

void ScheduleEditor::setGuiActive(bool activate)
{
    //debugPlan<<activate;
    ViewBase::setGuiActive(activate);
    if (activate && !m_view->currentIndex().isValid()) {
        m_view->selectionModel()->setCurrentIndex(m_view->model()->index(0, 0), QItemSelectionModel::NoUpdate);
    }
}

void ScheduleEditor::slotContextMenuRequested(const QModelIndex &index, const QPoint& pos)
{
    debugPlan<<index.row()<<","<<index.column()<<":"<<pos;
    QString name;
    m_view->setContextMenuIndex(index);
    if (name.isEmpty()) {
        slotHeaderContextMenuRequested(pos);
        m_view->setContextMenuIndex(QModelIndex());
        return;
    }
    debugPlan<<name;
    emit requestPopupMenu(name, pos);
    m_view->setContextMenuIndex(QModelIndex());
}

void ScheduleEditor::slotCurrentChanged(const QModelIndex &)
{
    //debugPlan<<curr.row()<<","<<curr.column();
}

void ScheduleEditor::slotSelectionChanged(const QModelIndexList &/*list*/)
{
    //debugPlan<<list.count();
    // Note: Don't use list as it includes all columns in a row
    QModelIndexList lst = m_view->selectedRows(); // gets column 0 in each row (should be 1 or 0 rows)
    if (lst.count() == 1) {
        ScheduleManager *sm = m_view->model()->manager(lst.first());
        emit scheduleSelectionChanged(sm);
    } else {
        emit scheduleSelectionChanged(0);
    }
    slotEnableActions();

}

void ScheduleEditor::updateActionsEnabled(const QModelIndex &index)
{
    debugPlan<<index;
    slotEnableActions();
}

void ScheduleEditor::slotEnableActions()
{
    if (! isReadWrite()) {
        actionAddSchedule->setEnabled(false);
        actionAddSubSchedule->setEnabled(false);
        actionDeleteSelection->setEnabled(false);
        actionCalculateSchedule->setEnabled(false);
        actionBaselineSchedule->setEnabled(false);
        actionMoveLeft->setEnabled(false);
        return;
    }
    QModelIndexList lst = m_view->selectedRows();
    if (lst.isEmpty()) {
        actionAddSchedule->setEnabled(true);
        actionAddSubSchedule->setEnabled(false);
        actionDeleteSelection->setEnabled(false);
        actionCalculateSchedule->setEnabled(false);
        actionBaselineSchedule->setEnabled(false);
        actionMoveLeft->setEnabled(false);
        return;
    }
    if (lst.count() > 1) {
        actionAddSchedule->setEnabled(false);
        actionAddSubSchedule->setEnabled(false);
        actionDeleteSelection->setEnabled(false);
        actionCalculateSchedule->setEnabled(false);
        actionBaselineSchedule->setEnabled(false);
        actionMoveLeft->setEnabled(false);
        return;
    }
    // one and only one manager selected
    ScheduleManager *sm = m_view->manager(lst.first());
    Q_ASSERT(sm);
    actionAddSchedule->setEnabled(true);
    actionAddSubSchedule->setEnabled(sm->isScheduled());
    actionDeleteSelection->setEnabled(! (sm->isBaselined() || sm->isChildBaselined()));
    actionCalculateSchedule->setEnabled(! sm->scheduling() && sm->childCount() == 0 && ! (sm->isBaselined() || sm->isChildBaselined()));

    const char *const actionBaselineScheduleIconName =
        sm->isBaselined() ? koIconNameCStr("view-time-schedule-baselined-remove") : koIconNameCStr("view-time-schedule-baselined-add");
    actionBaselineSchedule->setIcon(QIcon::fromTheme(QLatin1String(actionBaselineScheduleIconName)));

    // enable if scheduled and no one else is baselined
    bool en = sm->isScheduled() && (sm->isBaselined() || ! m_view->project()->isBaselined());
    actionBaselineSchedule->setEnabled(en);

    actionMoveLeft->setEnabled(sm->parentManager());
}

void ScheduleEditor::setupGui()
{
    actionAddSchedule  = new QAction(koIcon("view-time-schedule-insert"), i18n("Add Schedule"), this);
    actionCollection()->setDefaultShortcut(actionAddSchedule, Qt::CTRL + Qt::Key_I);
    actionCollection()->addAction("add_schedule", actionAddSchedule);
    connect(actionAddSchedule, &QAction::triggered, this, &ScheduleEditor::slotAddSchedule);

    actionAddSubSchedule  = new QAction(koIcon("view-time-schedule-child-insert"), i18n("Add Sub-schedule"), this);
    actionCollection()->setDefaultShortcut(actionAddSubSchedule, Qt::CTRL + Qt::SHIFT + Qt::Key_I);
    actionCollection()->addAction("add_subschedule", actionAddSubSchedule);
    connect(actionAddSubSchedule, &QAction::triggered, this, &ScheduleEditor::slotAddSubSchedule);

    actionDeleteSelection  = new QAction(koIcon("edit-delete"), xi18nc("@action", "Delete"), this);
    actionCollection()->setDefaultShortcut(actionDeleteSelection, Qt::Key_Delete);
    actionCollection()->addAction("delete_selection", actionDeleteSelection);
    connect(actionDeleteSelection, &QAction::triggered, this, &ScheduleEditor::slotDeleteSelection);

    actionCalculateSchedule  = new QAction(koIcon("view-time-schedule-calculus"), i18n("Calculate"), this);
//    actionCollection()->setDefaultShortcut(actionCalculateSchedule, Qt::CTRL + Qt::Key_C);
    actionCollection()->addAction("calculate_schedule", actionCalculateSchedule);
    connect(actionCalculateSchedule, &QAction::triggered, this, &ScheduleEditor::slotCalculateSchedule);

    actionBaselineSchedule  = new QAction(koIcon("view-time-schedule-baselined-add"), i18n("Baseline"), this);
//    actionCollection()->setDefaultShortcut(actionBaselineSchedule, Qt::CTRL + Qt::Key_B);
    actionCollection()->addAction("schedule_baseline", actionBaselineSchedule);
    connect(actionBaselineSchedule, &QAction::triggered, this, &ScheduleEditor::slotBaselineSchedule);

    actionMoveLeft  = new QAction(koIcon("go-first"), xi18nc("@action", "Detach"), this);
    actionCollection()->addAction("schedule_move_left", actionMoveLeft);
    connect(actionMoveLeft, &QAction::triggered, this, &ScheduleEditor::slotMoveLeft);

    // Add the context menu actions for the view options
    createOptionActions(ViewBase::OptionExpand | ViewBase::OptionCollapse | ViewBase::OptionViewConfig);
}

void ScheduleEditor::updateReadWrite(bool readwrite)
{
    debugPlan<<readwrite;
    ViewBase::updateReadWrite(readwrite);
    m_view->setReadWrite(readwrite);
    m_schedulingRange->setReadWrite(readwrite);
    slotEnableActions();
}

void ScheduleEditor::slotOptions()
{
    debugPlan;
    ItemViewSettupDialog *dlg = new ItemViewSettupDialog(this, m_view, true, this);
    connect(dlg, SIGNAL(finished(int)), SLOT(slotOptionsFinished(int)));
    dlg->open();
}

void ScheduleEditor::slotCalculateSchedule()
{
    //debugPlan;
    ScheduleManager *sm = m_view->selectedManager();
    if (sm == 0) {
        return;
    }
    if (sm->parentManager() || (sm->isScheduled() && project()->isStarted())) {
        RecalculateDialog dlg;
        if (dlg.exec() == QDialog::Rejected) {
            return;
        }
        sm->setRecalculate(true);
        sm->setRecalculateFrom(DateTime(dlg.dateTime()));
    }
    emit calculateSchedule(m_view->project(), sm);
}

void ScheduleEditor::slotAddSchedule()
{
    //debugPlan;
    int idx = -1;
    ScheduleManager *sm = m_view->selectedManager();
    if (sm) {
        idx = sm->parentManager() ? sm->parentManager()->indexOf(sm) : m_view->project()->indexOf(sm);
        if (idx >= 0) {
            ++idx;
        }
    }
    if (sm && sm->parentManager()) {
        sm = sm->parentManager();
        ScheduleManager *m = m_view->project()->createScheduleManager(sm->name() + QString(".%1").arg(sm->children().count() + 1));
        part()->addCommand(new AddScheduleManagerCmd(sm, m, idx, kundo2_i18n("Create sub-schedule")));
        QModelIndex idx = model()->index(m);
        if (idx.isValid()) {
            m_view->setFocus();
            m_view->scrollTo(idx);
            m_view->selectionModel()->select(idx, QItemSelectionModel::Rows | QItemSelectionModel::ClearAndSelect);
            m_view->selectionModel()->setCurrentIndex(idx, QItemSelectionModel::NoUpdate);
        }
    } else {
        Project *p = m_view->project();
        ScheduleManager *m = p->createScheduleManager();
        AddScheduleManagerCmd *cmd =  new AddScheduleManagerCmd(*p, m, idx, kundo2_i18n("Add schedule %1", m->name()));
        part() ->addCommand(cmd);
        QModelIndex idx = model()->index(m);
        if (idx.isValid()) {
            m_view->setFocus();
            m_view->scrollTo(idx);
            m_view->selectionModel()->select(idx, QItemSelectionModel::Rows | QItemSelectionModel::ClearAndSelect);
            m_view->selectionModel()->setCurrentIndex(idx, QItemSelectionModel::NoUpdate);
        }
    }
}

void ScheduleEditor::slotAddSubSchedule()
{
    //debugPlan;
    ScheduleManager *sm = m_view->selectedManager();
    if (sm) {
        int row = sm->parentManager() ? sm->parentManager()->indexOf(sm) : m_view->project()->indexOf(sm);
        if (row >= 0) {
            ++row;
        }
        ScheduleManager *m = m_view->project()->createScheduleManager(sm->name() + QString(".%1").arg(sm->children().count() + 1));
        part()->addCommand(new AddScheduleManagerCmd(sm, m, row, kundo2_i18n("Create sub-schedule")));
        m_view->expand(model()->index(sm));
        QModelIndex idx = model()->index(m);
        if (idx.isValid()) {
            m_view->selectionModel()->select(idx, QItemSelectionModel::Rows | QItemSelectionModel::ClearAndSelect);
            m_view->selectionModel()->setCurrentIndex(idx, QItemSelectionModel::NoUpdate);
        }
    } else {
        slotAddSchedule();
    }
}

void ScheduleEditor::slotBaselineSchedule()
{
    //debugPlan;
    ScheduleManager *sm = m_view->selectedManager();
    if (sm) {
        emit baselineSchedule(m_view->project(), sm);
    }
}

void ScheduleEditor::slotDeleteSelection()
{
    //debugPlan;
    ScheduleManager *sm = m_view->selectedManager();
    if (sm) {
        emit deleteScheduleManager(m_view->project(), sm);
    }
}

void ScheduleEditor::slotMoveLeft()
{
    ScheduleManager *sm = m_view->selectedManager();
    if (sm) {
        int index = -1;
        for (ScheduleManager *m = sm; m != 0; m = m->parentManager()) {
            if (m->parentManager() == 0) {
                 index = m->project().indexOf(m) + 1;
            }
        }
        debugPlan<<sm->name()<<index;
        emit moveScheduleManager(sm, 0, index);
    }
}

bool ScheduleEditor::loadContext(const KoXmlElement &context)
{
    debugPlan;
    return m_view->loadContext(model()->columnMap(), context);
}

void ScheduleEditor::saveContext(QDomElement &context) const
{
    m_view->saveContext(model()->columnMap(), context);
}

KoPrintJob *ScheduleEditor::createPrintJob()
{
    return m_view->createPrintJob(this);
}

//-----------------------------------------
ScheduleLogTreeView::ScheduleLogTreeView(QWidget *parent)
    : QTreeView(parent)
{
    header()->setStretchLastSection (true);
    header()->setContextMenuPolicy(Qt::CustomContextMenu);

    m_model = new QSortFilterProxyModel(this);
    m_model->setFilterRole(Qt::UserRole+1);
    m_model->setFilterKeyColumn (2); // severity
    m_model->setFilterWildcard("[^0]"); // Filter out Debug

    m_model->setSourceModel(new ScheduleLogItemModel(this));
    setModel(m_model);

    setRootIsDecorated(false);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setAlternatingRowColors(true);

    connect(header(), &QWidget::customContextMenuRequested, this, &ScheduleLogTreeView::headerContextMenuRequested);

    actionShowDebug = new KToggleAction(xi18nc("@action", "Show Debug Information"), this);
    connect(actionShowDebug, &QAction::toggled, this, &ScheduleLogTreeView::slotShowDebug);
}

void ScheduleLogTreeView::setFilterWildcard(const QString &filter)
{
    m_model->setFilterWildcard(filter);
}

QRegExp ScheduleLogTreeView::filterRegExp() const
{
    return m_model->filterRegExp();
}

void ScheduleLogTreeView::slotShowDebug(bool on)
{
    on ? setFilterWildcard(QString()) : setFilterWildcard("[^0]");
}

void ScheduleLogTreeView::contextMenuEvent (QContextMenuEvent *e)
{
    debugPlan<<indexAt(e->pos())<<" at"<<e->pos();
    emit contextMenuRequested(indexAt(e->pos()), e->globalPos());
}

void ScheduleLogTreeView::headerContextMenuRequested(const QPoint &pos)
{
    //debugPlan<<header()->logicalIndexAt(pos)<<" at"<<pos;
    QMenu *m = new QMenu(this);
    m->addAction(actionShowDebug);
    m->exec(mapToGlobal(pos));
    delete m;
}

void ScheduleLogTreeView::selectionChanged(const QItemSelection &sel, const QItemSelection &desel)
{
    //debugPlan<<sel.indexes().count();
    foreach(const QModelIndex &i, selectionModel()->selectedIndexes()) {
        Q_UNUSED(i);
        //debugPlan<<i.row()<<","<<i.column();
    }
    QTreeView::selectionChanged(sel, desel);
    emit selectionChanged(selectionModel()->selectedIndexes());
}

void ScheduleLogTreeView::currentChanged(const QModelIndex & current, const QModelIndex & previous)
{
    //debugPlan<<current.row()<<","<<current.column();
    QTreeView::currentChanged(current, previous);
    emit currentChanged(current);
//    selectionModel()->select(current, QItemSelectionModel::Rows | QItemSelectionModel::ClearAndSelect);
}

void ScheduleLogTreeView::slotEditCopy()
{
    QStringList lst;
//    int row = 0;
    QString s;
    QHeaderView *h = header();
    foreach(const QModelIndex &i, selectionModel()->selectedRows()) {
        QString s;
        for (int section = 0; section < h->count(); ++section) {
            QModelIndex idx = model()->index(i.row(), h->logicalIndex(section));
            if (! idx.isValid() || isColumnHidden(idx.column())) {
                continue;
            }
            if (! s.isEmpty()) {
                s += ' ';
            }
            s = QString("%1%2").arg(s).arg(idx.data().toString(), -10);
        }
        if (! s.isEmpty()) {
            lst << s;
        }
    }
    if (! lst.isEmpty()) {
        QApplication::clipboard()->setText(lst.join("\n"));
    }
}

//-----------------------------------
ScheduleLogView::ScheduleLogView(KoPart *part, KoDocument *doc, QWidget *parent)
    : ViewBase(part, doc, parent)
{
    setupGui();
    slotEnableActions(0);

    QVBoxLayout * l = new QVBoxLayout(this);
    m_view = new ScheduleLogTreeView(this);
    l->addWidget(m_view);
//    m_view->setEditTriggers(m_view->editTriggers() | QAbstractItemView::EditKeyPressed);


    connect(m_view, SIGNAL(currentChanged(QModelIndex)), this, SLOT(slotCurrentChanged(QModelIndex)));

    connect(m_view, SIGNAL(selectionChanged(QModelIndexList)), this, SLOT(slotSelectionChanged(QModelIndexList)));

    connect(baseModel(), &QAbstractItemModel::dataChanged, this, &ScheduleLogView::updateActionsEnabled);

    connect(m_view, &ScheduleLogTreeView::contextMenuRequested, this, &ScheduleLogView::slotContextMenuRequested);

}

void ScheduleLogView::setProject(Project *project)
{
    m_view->setProject(project);
}

void ScheduleLogView::draw(Project &project)
{
    setProject(&project);
}

void ScheduleLogView::setGuiActive(bool activate)
{
    //debugPlan<<activate;
    ViewBase::setGuiActive(activate);
/*    if (activate && !m_view->currentIndex().isValid()) {
        m_view->selectionModel()->setCurrentIndex(m_view->model()->index(0, 0), QItemSelectionModel::NoUpdate);
    }*/
}

void ScheduleLogView::slotEdit()
{
    QString id = sender()->property("p_identity").toString();
    if (id.isEmpty()) {
        emit editNode(project());
        return;
    }
    Node *n = project()->findNode(id);
    if (n) {
        emit editNode(n);
        return;
    }
    Resource *r = project()->findResource(id);
    if (r) {
        emit editResource(r);
        return;
    }
    warnPlan<<"No object";
}

void ScheduleLogView::slotContextMenuRequested(const QModelIndex &index, const QPoint& pos)
{
    if (! isReadWrite() || ! index.isValid()) {
        return;
    }
    QMenu *m = new QMenu(this);
    QString id = index.data(ScheduleLogItemModel::IdentityRole).toString();
    if (id.isEmpty()) {
        return;
    }
    QAction *a = new QAction(koIcon("document-edit"), i18n("Edit..."), m);
    a->setProperty("p_identity", id);
    m->addAction(a);
    connect(a, &QAction::triggered, this, &ScheduleLogView::slotEdit);
    m->addSeparator();
    m->exec(pos);
    delete m;
}

void ScheduleLogView::slotScheduleSelectionChanged(ScheduleManager *sm)
{
    baseModel()->setManager(sm);
}

void ScheduleLogView::slotCurrentChanged(const QModelIndex &)
{
    //debugPlan<<curr.row()<<","<<curr.column();
}

void ScheduleLogView::slotSelectionChanged(const QModelIndexList &list)
{
    debugPlan<<list.count();
}

void ScheduleLogView::updateActionsEnabled(const QModelIndex &index)
{
    debugPlan<<index;
}

void ScheduleLogView::slotEnableActions(const ScheduleManager *)
{
}

void ScheduleLogView::setupGui()
{
    // Add the context menu actions for the view options
    createOptionActions(0);
}

void ScheduleLogView::updateReadWrite(bool readwrite)
{
    debugPlan<<readwrite;
    ViewBase::updateReadWrite(readwrite);
//    m_view->setReadWrite(readwrite);
    //slotEnableActions(m_view->currentManager());
}

void ScheduleLogView::slotOptions()
{
    debugPlan;
}

void ScheduleLogView::slotEditCopy()
{
    m_view->slotEditCopy();
}

bool ScheduleLogView::loadContext(const KoXmlElement &/*context */)
{
    debugPlan;
    return true;//m_view->loadContext(model()->columnMap(), context);
}

void ScheduleLogView::saveContext(QDomElement &/*context */) const
{
    //m_view->saveContext(model()->columnMap(), context);
}


//---------------------------

ScheduleHandlerView::ScheduleHandlerView(KoPart *part, KoDocument *doc, QWidget *parent)
    : SplitterView(part, doc, parent)
{
    debugPlan<<"---------------- Create ScheduleHandlerView ------------------";
    m_scheduleEditor = new ScheduleEditor(part, doc, this);
    m_scheduleEditor->setObjectName("ScheduleEditor");
    addView(m_scheduleEditor);
    insertChildClient(m_scheduleEditor);

    QTabWidget *tab = addTabWidget();

    PertResult *p = new PertResult(part, doc, tab);
    p->setObjectName("PertResult");
    addView(p, tab, i18n("Result"));
    connect(m_scheduleEditor, &ScheduleEditor::scheduleSelectionChanged, p, &PertResult::slotScheduleSelectionChanged);

    PertCpmView *c = new PertCpmView(part, doc, tab);
    c->setObjectName("PertCpmView");
    addView(c, tab, i18n("Critical Path"));
    connect(m_scheduleEditor, &ScheduleEditor::scheduleSelectionChanged, c, &PertCpmView::slotScheduleSelectionChanged);

    ScheduleLogView *v = new ScheduleLogView(part, doc, tab);
    v->setObjectName("ScheduleLogView");
    addView(v, tab, i18n("Scheduling Log"));
    connect(m_scheduleEditor, SIGNAL(scheduleSelectionChanged(KPlato::ScheduleManager*)), v, SLOT(slotScheduleSelectionChanged(KPlato::ScheduleManager*)));
    connect(v, &ScheduleLogView::editNode, this, &ScheduleHandlerView::editNode);
    connect(v, &ScheduleLogView::editResource, this, &ScheduleHandlerView::editResource);
}

void ScheduleHandlerView::currentTabChanged(int)
{
}

ViewBase *ScheduleHandlerView::hitView(const QPoint &/*glpos */)
{
    //debugPlan<<this<<glpos<<"->"<<mapFromGlobal(glpos)<<"in"<<frameGeometry();
    return this;
}


void ScheduleHandlerView::setGuiActive(bool active) // virtual slot
{
    foreach (ViewBase *v, findChildren<ViewBase*>()) {
        v->setGuiActive(active);
    }
    m_activeview = active ? this : 0;
    emit guiActivated(this, active);
}

void ScheduleHandlerView::slotGuiActivated(ViewBase *, bool)
{
}

SchedulingRange::SchedulingRange(KoDocument *doc, QWidget *parent)
    : QWidget(parent)
    , m_doc(doc)
    , m_project(0)
{
    setupUi(this);

    connect(targetStartTime, &QAbstractSpinBox::editingFinished, this, &SchedulingRange::slotStartChanged);
    connect(targetEndTime, &QAbstractSpinBox::editingFinished, this, &SchedulingRange::slotEndChanged);
}

void SchedulingRange::setReadWrite(bool rw)
{
    targetStartTime->setEnabled(rw);
    targetEndTime->setEnabled(rw);
}

void SchedulingRange::setProject(Project *project)
{
    if (m_project) {
        disconnect(m_project, &Project::nodeChanged, this, &SchedulingRange::slotProjectChanged);
    }
    m_project = project;
    if (m_project) {
        connect(m_project, &Project::nodeChanged, this, &SchedulingRange::slotProjectChanged);
        slotProjectChanged(m_project);
    }
}

void SchedulingRange::slotProjectChanged(Node *node)
{
    if (node != m_project) {
        return;
    }
    if (targetStartTime->dateTime() != m_project->constraintStartTime()) {
        targetStartTime->setDateTime(m_project->constraintStartTime());
    }
    if (targetEndTime->dateTime() != m_project->constraintEndTime()) {
        targetEndTime->setDateTime(m_project->constraintEndTime());
    }
}

void SchedulingRange::slotStartChanged()
{
    if (!m_project || !m_doc) {
        return;
    }
    if (targetStartTime->dateTime() == m_project->constraintStartTime()) {
        return;
    }
    ProjectModifyStartTimeCmd *cmd = new ProjectModifyStartTimeCmd(*m_project, targetStartTime->dateTime(), kundo2_i18n("Modify project target start time"));
    m_doc->addCommand(cmd);
}

void SchedulingRange::slotEndChanged()
{
    if (!m_project || !m_doc) {
        return;
    }
    if (targetEndTime->dateTime() == m_project->constraintEndTime()) {
        return;
    }
    ProjectModifyEndTimeCmd *cmd = new ProjectModifyEndTimeCmd(*m_project, targetEndTime->dateTime(), kundo2_i18n("Modify project target end time"));
    m_doc->addCommand(cmd);
}

} // namespace KPlato
