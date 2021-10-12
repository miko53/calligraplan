/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "ResourceUsageView.h"
#include "ResourceUsageModel.h"
#include "ResourceModel.h"
#include "MainDocument.h"

#include <KoIcon.h>

#include <ScrollableChart.h>

#include <KActionCollection>
#include <KXMLGUIClient>
#include <KXMLGUIFactory>
#include <KToolBar>

#include <KChartBarDiagram>
#include <KChartLineDiagram>
#include <KChartLegend>
#include <KChartDataValueAttributes>

#include <QWidgetAction>
#include <QSpinBox>

ResourceUsageView::ResourceUsageView(KoPart *part, KoDocument *doc, QWidget *parent)
    : KoView(part, doc, parent)
    , m_readWrite(false)
    , m_numDays(nullptr)
{
    //debugPlan;
    ui.setupUi(this);

    if (doc && doc->isReadWrite()) {
        setXMLFile("Portfolio_ResourceUsageViewUi.rc");
    } else {
        setXMLFile("Portfolio_ResourceUsageViewUi_readonly.rc");
    }
    setupGui();

    auto resourceModel = new ResourceModel(this);
    resourceModel->setPortfolio(qobject_cast<MainDocument*>(doc));
    ui.resourceView->setModel(resourceModel);

    m_resourceUsageModel.setPortfolio(qobject_cast<MainDocument*>(doc));

    auto bar = qobject_cast<KChart::BarDiagram*>(ui.chart->chart()->coordinatePlane()->diagram());
    bar->setType(KChart::BarDiagram::Stacked);
    ui.chart->legend()->setTitleText(i18n("Tasks"));

    auto *m = new ResourceAvailableModel(this);
    m->setSourceModel(&ui.chart->filterModel());
    m_available = new LineDiagram();
    m_available->setCenterDataPoints(true);
    m_available->setPen(0,  Qt::NoPen);
    KChart::DataValueAttributes dva(m_available->dataValueAttributes());
    KChart::TextAttributes ta( dva.textAttributes() );
    ta.setVisible(false);
    dva.setTextAttributes(ta);
    KChart::MarkerAttributes ma(dva.markerAttributes());
    ma.setMarkerStyle(KChart::MarkerAttributes::MarkerSquare);
    ma.setMarkerSize(QSize(10, 2));
    ma.setVisible(true);
    dva.setMarkerAttributes(ma);
    dva.setVisible( true );
    m_available->setDataValueAttributes(dva);
    connect(m, &ResourceAvailableModel::modelReset, this, &ResourceUsageView::updateMarker);
    connect(m_available, &LineDiagram::sizeChanged, this, &ResourceUsageView::updateMarker);
    m_available->setModel(m);
    ui.chart->addDiagram(m_available);

    ui.chart->setDataModel(&m_resourceUsageModel);

    //ui.chart->chart()->coordinatePlane()->setRubberBandZoomingEnabled(true);

    connect(ui.resourceView->selectionModel(), &QItemSelectionModel::currentChanged, this, &ResourceUsageView::slotCurrentIndexChanged);

    connect(&m_resourceUsageModel, &ResourceUsageModel::modelReset, this, &ResourceUsageView::slotUpdateNumDays);
    slotUpdateNumDays();
}

ResourceUsageView::~ResourceUsageView()
{
}

void ResourceUsageView::setupGui()
{
    auto s = new KSelectAction(koIcon("office-chart-bar-stacked"), i18n("Diagram Types"), this);
    actionCollection()->addAction("diagramtypes", s);
    connect(s, &KSelectAction::indexTriggered, ui.chart, &KPlato::ScrollableChart::setDiagramFlavor);

    auto a = new QAction(koIcon("office-chart-bar-normal"), i18n("Normal"), this);
    a->setObjectName("charttype_normal");
    a->setCheckable(true);
    s->addAction(a);
    a = new QAction(koIcon("office-chart-bar-stacked"), i18n("Stacked"), this);
    a->setObjectName("charttype_stacked");
    a->setCheckable(true);
    s->addAction(a);
    s->setCurrentAction(a);

    auto numDays = new QWidgetAction(this);
    numDays->setObjectName("diagramrange");
    numDays->setText(i18n("Range"));

    m_numDays = new QSpinBox();
    m_numDays->setObjectName(("diagramrange"));
    m_numDays->setToolTip(numDays->text());
    m_numDays->setSpecialValueText(i18n("All"));
    m_numDays->setSpecialValueText(m_numDays->specialValueText());
    connect(m_numDays, QOverload<int>::of(&QSpinBox::valueChanged), this, &ResourceUsageView::slotUpdateNumDays);

    numDays->setDefaultWidget(m_numDays);
    actionCollection()->addAction(numDays->objectName(), numDays);
}

void ResourceUsageView::guiActivateEvent(bool activated)
{
    Q_UNUSED(activated);
}

void ResourceUsageView::updateReadWrite(bool readwrite)
{
    m_readWrite = readwrite;
}

QMenu * ResourceUsageView::popupMenu(const QString& name)
{
    return nullptr;
}

KoPrintJob *ResourceUsageView::createPrintJob()
{
    return nullptr;
}

void ResourceUsageView::slotCurrentIndexChanged(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(previous);
    m_resourceUsageModel.setCurrentResource(current.data(RESOURCEID_ROLE).toString());
}

void ResourceUsageView::slotUpdateNumDays()
{
    int end = m_resourceUsageModel.rowCount();
    m_numDays->setMaximum(end);
    auto a = qobject_cast<QWidgetAction*>(actionCollection()->action("diagramrange"));
    if (!a) {
        return;
    }
    auto spinBox = qobject_cast<QSpinBox*>(a->defaultWidget());
    if (spinBox) {
        spinBox->setMaximum(end);
    }
    slotNumDaysChanged(m_numDays->value());
}

void ResourceUsageView::slotNumDaysChanged(int value)
{
    ui.chart->setNumRows(value);
    updateMarker();
}

void ResourceUsageView::updateMarker()
{
    const auto width = m_available->geometry().width() / std::max(m_available->model()->rowCount(), 1);
    KChart::DataValueAttributes dva(m_available->dataValueAttributes());
    KChart::MarkerAttributes ma(dva.markerAttributes());
    ma.setMarkerSize(QSize(width, 1));
    dva.setMarkerAttributes(ma);
    m_available->setDataValueAttributes(dva);
    ui.chart->chart()->update();
}

//--------------------
LineDiagram::LineDiagram(QWidget *parent, KChart::CartesianCoordinatePlane* plane)
    : KChart::LineDiagram(parent, plane)
{
}

void LineDiagram::resize(const QSizeF &size)
{
    KChart::LineDiagram::resize(size);
    Q_EMIT sizeChanged(size);
}
