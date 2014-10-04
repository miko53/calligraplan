/* This file is part of the KDE project
  Copyright (C) 2007 - 2010, 2012 Dag Andersen <danders@get2net.dk>

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

#include "kptrelationeditor.h"

#include "kptglobal.h"
#include "kptcommonstrings.h"
#include "kptcommand.h"
#include "kptproject.h"
#include "kptitemviewsettup.h"
#include "kptdebug.h"

#include <KoDocument.h>

#include <QAbstractItemView>
#include <QItemSelectionModel>
#include <QModelIndex>
#include <QVBoxLayout>
#include <QWidget>
#include <QMenu>

#include <kaction.h>
#include <klocale.h>
#include <kactioncollection.h>


namespace KPlato
{

//--------------------
RelationTreeView::RelationTreeView( QWidget *parent )
    : DoubleTreeViewBase( parent )
{
    setViewSplitMode( false );
    RelationItemModel *m = new RelationItemModel( this );
    setModel( m );
    setSelectionMode( QAbstractItemView::ExtendedSelection );
    setSelectionBehavior( QAbstractItemView::SelectRows );
    setArrowKeyNavigation( true );
    setRootIsDecorated ( false );

    createItemDelegates( m );

    //HACK to simulate SingleSelection *and* get indication of current item
    connect( selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)), SLOT(slotCurrentChanged(QModelIndex,QModelIndex)) );
}

void RelationTreeView::slotCurrentChanged(const QModelIndex &curr, const QModelIndex& )
{
    selectionModel()->select( curr, QItemSelectionModel::Rows | QItemSelectionModel::ClearAndSelect );
}

//-----------------------------------
RelationEditor::RelationEditor(KoPart *part, KoDocument *doc, QWidget *parent)
    : ViewBase(part, doc, parent)
{
    kDebug(planDbg())<<"----------------- Create RelationEditor ----------------------";

    QVBoxLayout * l = new QVBoxLayout( this );
    l->setMargin( 0 );
    m_view = new RelationTreeView( this );
    l->addWidget( m_view );
    //kDebug(planDbg())<<m_view->actionSplitView();
    setupGui();

    connect( m_view, SIGNAL(currentChanged(QModelIndex,QModelIndex)), this, SLOT(slotCurrentChanged(QModelIndex,QModelIndex)) );

    connect( m_view, SIGNAL(selectionChanged(QModelIndexList)), this, SLOT(slotSelectionChanged(QModelIndexList)) );

    connect( m_view, SIGNAL(contextMenuRequested(QModelIndex,QPoint)), SLOT(slotContextMenuRequested(QModelIndex,QPoint)) );

    connect( m_view, SIGNAL(headerContextMenuRequested(QPoint)), SLOT(slotHeaderContextMenuRequested(QPoint)) );

    connect(model(), SIGNAL(executeCommand(KUndo2Command*)), doc, SLOT(addCommand(KUndo2Command*)));
}

void RelationEditor::updateReadWrite( bool rw )
{
    m_view->setReadWrite( rw );
}

void RelationEditor::draw( Project &project )
{
    m_view->setProject( &project );
}

void RelationEditor::draw()
{
}

void RelationEditor::setGuiActive( bool /*activate */)
{
}

void RelationEditor::slotCurrentChanged(  const QModelIndex &/*curr*/, const QModelIndex & )
{
    //kDebug(planDbg())<<curr.row()<<","<<curr.column();
    slotEnableActions();
}

void RelationEditor::slotSelectionChanged( const QModelIndexList& /*list*/)
{
    //kDebug(planDbg())<<list.count();
    slotEnableActions();
}

Relation *RelationEditor::currentRelation() const
{
    //kDebug(planDbg());
    return m_view->currentRelation();
}

void RelationEditor::slotContextMenuRequested( const QModelIndex& index, const QPoint& pos )
{
    Relation *rel = m_view->model()->relation( index );
    if ( rel == 0 ) {
        slotHeaderContextMenuRequested( pos );
        return;
    }
    QString name = "relation_popup";
    emit requestPopupMenu( name, pos );
}

void RelationEditor::slotHeaderContextMenuRequested( const QPoint &pos )
{
    kDebug(planDbg());
    QList<QAction*> lst = contextActionList();
    if ( ! lst.isEmpty() ) {
        QMenu::exec( lst, pos,  lst.first() );
    }
}

void RelationEditor::slotEnableActions()
{
    updateActionsEnabled( true );
}

void RelationEditor::updateActionsEnabled( bool /*on */)
{
}

void RelationEditor::setupGui()
{
    // Add the context menu actions for the view options
    connect(m_view->actionSplitView(), SIGNAL(triggered(bool)), SLOT(slotSplitView()));
    addContextAction( m_view->actionSplitView() );

    createOptionAction();
}

void RelationEditor::slotSplitView()
{
    //kDebug(planDbg());
    m_view->setViewSplitMode( ! m_view->isViewSplit() );
}


void RelationEditor::slotOptions()
{
    kDebug(planDbg());
    bool col0 = false;
    TreeViewBase *v = m_view->slaveView();
    if ( v->isHidden() ) {
        v = m_view->masterView();
        col0 = true;
    }
    ItemViewSettupDialog *dlg = new ItemViewSettupDialog( this, v, col0, this );
    connect(dlg, SIGNAL(finished(int)), SLOT(slotOptionsFinished(int)));
    dlg->show();
    dlg->raise();
    dlg->activateWindow();
}

void RelationEditor::slotAddRelation()
{
    kDebug(planDbg());
}

void RelationEditor::edit( QModelIndex i )
{
    if ( i.isValid() ) {
        QModelIndex p = m_view->model()->parent( i );
//        m_view->setExpanded( p );
        m_view->selectionModel()->setCurrentIndex( i, QItemSelectionModel::NoUpdate );
        m_view->edit( i );
    }
}

void RelationEditor::slotDeleteRelation( Relation *r)
{
    emit deleteRelation( r );
}

bool RelationEditor::loadContext( const KoXmlElement &context )
{
    kDebug(planDbg());
    ViewBase::loadContext( context );
    return m_view->loadContext( m_view->model()->columnMap(), context );
}

void RelationEditor::saveContext( QDomElement &context ) const
{
    ViewBase::saveContext( context );
    m_view->saveContext( m_view->model()->columnMap(), context );
}

KoPrintJob *RelationEditor::createPrintJob()
{
    return m_view->createPrintJob( this );
}

} // namespace KPlato

#include "kptrelationeditor.moc"
