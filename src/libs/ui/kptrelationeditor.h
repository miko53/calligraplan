/* This file is part of the KDE project
  Copyright (C) 2007 Dag Andersen <danders@get2net.dk>

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

#ifndef KPTRELATIONEDITOR_H
#define KPTRELATIONEDITOR_H

#include "planui_export.h"

#include "kptglobal.h"
#include "kptviewbase.h"
#include "kptrelationmodel.h"

class KoDocument;

namespace KPlato
{

class Project;
class Node;
class RelationItemModel;
class Relation;

class PLANUI_EXPORT RelationTreeView : public DoubleTreeViewBase
{
    Q_OBJECT
public:
    explicit RelationTreeView(QWidget *parent = 0);
    
    RelationItemModel *model() const { return static_cast<RelationItemModel*>(DoubleTreeViewBase::model()); }
    
    Project *project() const { return model()->project(); }
    void setProject(Project *project) { model()->setProject(project); }
    
    void setNode(Node *node) { model()->setNode(node); }
    Relation *currentRelation() const { return model()->relation(selectionModel()->currentIndex()); }
Q_SIGNALS:
    void currentColumnChanged(const QModelIndex&, const QModelIndex&);
    
protected Q_SLOTS:
    void slotCurrentChanged(const QModelIndex &curr, const QModelIndex&);
};

class PLANUI_EXPORT RelationEditor : public ViewBase
{
    Q_OBJECT
public:
    /// Create a relation editor
    RelationEditor(KoPart *part, KoDocument *doc, QWidget *parent);
    
    void setupGui();
    void draw(Project &project) override;
    void draw() override;

    Relation *currentRelation() const override;
    Relation *selectedRelation() const;

    void updateReadWrite(bool readwrite) override;

    RelationItemModel *model() const { return m_view->model(); }

    /// Loads context info into this view. Reimplement.
    bool loadContext(const KoXmlElement &/*context*/) override;
    /// Save context info from this view. Reimplement.
    void saveContext(QDomElement &/*context*/) const override;
    
    KoPrintJob *createPrintJob() override;

Q_SIGNALS:
    void openNode();
    void addRelation();
    void deleteRelation(KPlato::Relation *);

public Q_SLOTS:
    /// Activate/deactivate the gui
    void setGuiActive(bool activate) override;

protected Q_SLOTS:
    void slotOptions() override;

protected:
    void updateActionsEnabled(bool on);

private Q_SLOTS:
    void slotSelectionChanged(const QModelIndexList&);
    void slotCurrentChanged(const QModelIndex&, const QModelIndex&);
    void slotContextMenuRequested(const QModelIndex &index, const QPoint& pos);
    
    void slotEnableActions();

    void slotAddRelation();
    void slotDeleteRelation(KPlato::Relation *r);

    void slotSplitView();
    
    void slotHeaderContextMenuRequested(const QPoint&) override;
    
private:
    void edit(const QModelIndex &index);

private:
    RelationTreeView *m_view;
};


} //namespace KPlato

#endif
