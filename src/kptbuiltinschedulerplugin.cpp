/* This file is part of the KDE project
  Copyright (C) 2009 Dag Andersen <dag.andersen@kdemail.net>

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
  Boston, MA 02110-1301, USA.
*/

// clazy:excludeall=qstring-arg
#include "kptbuiltinschedulerplugin.h"

#include "kptproject.h"
#include "kptschedule.h"
#include "kptxmlloaderobject.h"

#include <KLocalizedString>

namespace KPlato
{

BuiltinSchedulerPlugin::BuiltinSchedulerPlugin(QObject *parent)
    : SchedulerPlugin(parent)
{
    setName(i18nc("Network = task dependency network", "Network Scheduler"));
    setComment(xi18nc("@info:tooltip", "Built-in network (PERT) based scheduler"));
}
 
BuiltinSchedulerPlugin::~BuiltinSchedulerPlugin()
{
}

QString BuiltinSchedulerPlugin::description() const
{
    return xi18nc("@info:whatsthis", "<title>Network (PERT) Scheduler</title>"
                    "<para>The network scheduler generally schedules tasks according to their dependencies."
                    " When a task is scheduled it is scheduled in full, booking the allocated resources if available."
                    " If overbooking is not allowed, subsequent tasks that requests the same resource"
                    " will be scheduled later in time.</para>"
                    "<para>Tasks with time constraints will be scheduled first to minimize the problem"
                    " with resource conflicts</para>"
                    "<para><note>This scheduler does not handle resource conflicts well."
                    "<nl/>You can try a different scheduler if available."
                    " You may also change resource allocations or add dummy dependencies to avoid the conflicts.</note></para>"
                );
}

void BuiltinSchedulerPlugin::calculate(Project &project, ScheduleManager *sm, bool nothread)
{
    KPlatoScheduler *job = new KPlatoScheduler(&project, sm);
    m_jobs << job;
    connect(job, &SchedulerThread::jobStarted, this, &BuiltinSchedulerPlugin::slotStarted);
    connect(job, &SchedulerThread::jobFinished, this, &BuiltinSchedulerPlugin::slotFinished);

    connect(this, &BuiltinSchedulerPlugin::sigCalculationStarted, &project, &Project::sigCalculationStarted);
    connect(this, &BuiltinSchedulerPlugin::sigCalculationFinished, &project, &Project::sigCalculationFinished);

    sm->setScheduling(true);
    if (nothread) {
        connect(job, &SchedulerThread::maxProgressChanged, sm, &ScheduleManager::setMaxProgress);
        connect(job, &SchedulerThread::progressChanged, sm, &ScheduleManager::setProgress);
        job->doRun();
    } else {
        job->start();
    }
    m_synctimer.start();
}

void BuiltinSchedulerPlugin::slotStarted(SchedulerThread *job)
{
    qDebug()<<"BuiltinSchedulerPlugin::slotStarted:"<<job->mainProject()<<job->mainManager();
    
    Q_EMIT sigCalculationStarted(job->mainProject(), job->mainManager());
}

void BuiltinSchedulerPlugin::slotFinished(SchedulerThread *job)
{
    ScheduleManager *sm = job->mainManager();
    Project *mp = job->mainProject();
    qDebug()<<"BuiltinSchedulerPlugin::slotFinished:"<<mp<<sm<<job->isStopped();
    if (job->isStopped()) {
        sm->setCalculationResult(ScheduleManager::CalculationCanceled);
    } else {
        updateLog(job);
        Project *tp = static_cast<KPlatoScheduler*>(job)->project();
        ScheduleManager *tm = static_cast<KPlatoScheduler*>(job)->manager();
        updateProject(tp, tm, mp, sm);
        sm->setCalculationResult(ScheduleManager::CalculationDone);
    }
    sm->setScheduling(false);

    m_jobs.removeAt(m_jobs.indexOf(job));
    if (m_jobs.isEmpty()) {
        m_synctimer.stop();
    }
    Q_EMIT sigCalculationFinished(mp, sm);

    disconnect(this, &BuiltinSchedulerPlugin::sigCalculationStarted, mp, &Project::sigCalculationStarted);
    disconnect(this, &BuiltinSchedulerPlugin::sigCalculationFinished, mp, &Project::sigCalculationFinished);

    job->deleteLater();
    qDebug()<<"BuiltinSchedulerPlugin::slotFinished: <<<";
}

void BuiltinSchedulerPlugin::schedule(SchedulingContext &context)
{
    Q_EMIT calculateSchedule(context);
}

//--------------------
KPlatoScheduler::KPlatoScheduler(Project *project, ScheduleManager *sm, QObject *parent)
    : SchedulerThread(project, sm, parent)
{
    qDebug()<<"KPlatoScheduler::KPlatoScheduler:"<<m_mainmanager<<m_mainmanager->name()<<m_mainmanagerId;
}

KPlatoScheduler::~KPlatoScheduler()
{
    qDebug()<<"KPlatoScheduler::~KPlatoScheduler:"<<QThread::currentThreadId();
}

void KPlatoScheduler::stopScheduling()
{
    m_stopScheduling = true;
    if (m_project) {
        m_project->stopcalculation = true;
    }
}

void KPlatoScheduler::run()
{
    if (m_haltScheduling) {
        deleteLater();
        return;
    }
    if (m_stopScheduling) {
        return;
    }
    { // mutex -->
        m_projectMutex.lock();
        m_managerMutex.lock();

        m_project = new Project();
        loadProject(m_project, m_pdoc);
        m_project->setName("Schedule: " + m_project->name()); //Debug

        m_manager = m_project->scheduleManager(m_mainmanagerId);
        Q_ASSERT(m_manager);
        Q_ASSERT(m_manager->expected());
        Q_ASSERT(m_manager != m_mainmanager);
        Q_ASSERT(m_manager->scheduleId() == m_mainmanager->scheduleId());
        Q_ASSERT(m_manager->expected() != m_mainmanager->expected());
        m_manager->setName("Schedule: " + m_manager->name()); //Debug

        m_managerMutex.unlock();
        m_projectMutex.unlock();
    } // <--- mutex

    connect(m_project, SIGNAL(maxProgress(int)), this, SLOT(setMaxProgress(int)));
    connect(m_project, SIGNAL(sigProgress(int)), this, SLOT(setProgress(int)));

    bool x = connect(m_manager, SIGNAL(sigLogAdded(KPlato::Schedule::Log)), this, SLOT(slotAddLog(KPlato::Schedule::Log)));
    Q_ASSERT(x); Q_UNUSED(x);
    m_project->calculate(*m_manager);
    if (m_haltScheduling) {
        deleteLater();
    }
}


} //namespace KPlato

