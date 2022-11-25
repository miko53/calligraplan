/* This file is part of the KDE project
 *  SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 *  SPDX-FileCopyrightText: 2005 Dag Andersen <dag.andersen@kdemail.net>
 *  SPDX-FileCopyrightText: 2006 Raphael Langerhorst <raphael.langerhorst@kdemail.net>
 * 
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KPTGANTTVIEW_H
#define KPTGANTTVIEW_H

#include "planui_export.h"

#include "kptviewbase.h"

class KoDocument;

class QSplitter;
class QActionGroup;

class KoPrintJob;

namespace KGantt
{
    class TreeViewRowController;
}

namespace KPlato
{

class Node;
class Project;
class MyKGanttView;



class PLANUI_EXPORT GanttView : public ViewBase
{
    Q_OBJECT
public:
    GanttView(KoPart *part, KoDocument *doc, QWidget *parent, bool readWrite = true);

    //~GanttView();

    virtual void setZoom(double zoom);
    void setupGui();
    void setProject(Project *project) override;

    using ViewBase::draw;
    void draw(Project &project) override;
    void drawChanges(Project &project) override;

    Node *currentNode() const override;

    void clear();

    bool loadContext(const KoXmlElement &context) override;
    void saveContext(QDomElement &context) const override;

    void updateReadWrite(bool on) override;

    KoPrintJob *createPrintJob() override;

    void setShowSpecialInfo(bool on);
    bool showSpecialInfo() const;

public Q_SLOTS:
    void setScheduleManager(KPlato::ScheduleManager *sm) override;
    void setShowResources(bool on);
    void setShowTaskName(bool on);
    void setShowTaskLinks(bool on);
    void setShowProgress(bool on);
    void setShowPositiveFloat(bool on);
    void setShowCriticalTasks(bool on);
    void setShowCriticalPath(bool on);
    void setShowNoInformation(bool on);
    void setShowAppointments(bool on);

    void slotEditCopy() override;

    void slotProjectCalculated(Project *prjoect, ScheduleManager *sm);

protected Q_SLOTS:
    void slotContextMenuRequested(const QModelIndex&, const QPoint &pos);
    void slotGanttHeaderContextMenuRequested(const QPoint &pt);
    void slotDateTimeGridChanged();
    void slotOptions() override;
    void slotOptionsFinished(int result) override;
    void ganttActions();
    void itemDoubleClicked(const QPersistentModelIndex &idx);

private Q_SLOTS:
    void slotOpenCurrentNode();
    void slotOpenNode(KPlato::Node *node);
    void slotTaskProgress();
    void slotOpenProjectDescription();
    void slotTaskDescription();
    void slotOpenTaskDescription(bool);
    void slotDocuments();

    void slotTaskEditFinished(int result);
    void slotSummaryTaskEditFinished(int result);
    void slotTaskProgressFinished(int result);
    void slotMilestoneProgressFinished(int result);
    void slotTaskDescriptionFinished(int result);
    void slotDocumentsFinished(int result);

    void slotSelectionChanged();

private:
    void updateActionsEnabled(bool on);

private:
    bool m_readWrite;
    int m_defaultFontSize;
    QSplitter *m_splitter;
    MyKGanttView *m_gantt;
    Project *m_project;

    QAction *actionShowProject;
    QAction *actionShowUnscheduled;
    QDomDocument m_domdoc;

    QActionGroup *m_scalegroup;
};

}  //KPlato namespace

#endif
