/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2007 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/
// clazy:excludeall=qstring-arg
#include "ScheduleTester.h"

#include "kptdatetime.h"
#include "kptschedule.h"

#include <QTest>

namespace QTest
{
    template<>
            char *toString(const KPlato::DateTime &dt)
    {
        return toString(dt.toString());
    }
    template<>
            char *toString(const KPlato::DateTimeInterval &dt)
    {
        if (dt.first.isValid() && dt.second.isValid())
            return toString(dt.first.toString() + " - " + dt.second.toString());

        return toString("invalid interval");
    }
}

namespace KPlato
{

void ScheduleTester::initTestCase()
{
    date = QDate::currentDate();
    t1 = QTime(9, 0, 0);
    t2 = QTime(12, 0, 0);
    t3 = QTime(17, 0, 0);
    DateTime dt1(date, t1);
    DateTime dt2(date, t3);
    resourceSchedule.addAppointment(&nodeSchedule, dt1, dt2, 100.);
    dt1 = DateTime(date.addDays(1), t1);
    dt2 = DateTime(date.addDays(1), t2);
    resourceSchedule.addAppointment(&nodeSchedule, dt1, dt2, 100.);
    dt1 = DateTime(date.addDays(1), t2);
    dt2 = DateTime(date.addDays(1), t3);
    resourceSchedule.addAppointment(&nodeSchedule, dt1, dt2, 100.);
    dt1 = DateTime(date.addDays(2), t1);
    dt2 = DateTime(date.addDays(2), t3);
    resourceSchedule.addAppointment(&nodeSchedule, dt1, dt2, 100.);
}


void ScheduleTester::available()
{
    // before any interval
    DateTimeInterval i1(DateTime(date.addDays(-1), t1), DateTime(date.addDays(-1), t2));
    QCOMPARE(i1, resourceSchedule.available(i1));

    // upto first interval
    i1 = DateTimeInterval(DateTime(date, QTime(0, 0, 0)), DateTime(date, t1));
    QCOMPARE(i1, resourceSchedule.available(i1));

    // partly into first interval
    i1 = DateTimeInterval(DateTime(date, QTime(0, 0, 0)), DateTime(date, t2));
    DateTimeInterval res(DateTime(date, QTime(0, 0, 0)), DateTime(date, t1));
    QCOMPARE(res, resourceSchedule.available(i1));

    // between two intervals, start at end of first
    i1 = DateTimeInterval(DateTime(date, t3), DateTime(date, t3.addSecs(100)));
    QCOMPARE(i1, resourceSchedule.available(i1));

    // between two intervals, free of both
    i1 = DateTimeInterval(DateTime(date, t3.addSecs(10)), DateTime(date, t3.addSecs(100)));
    QCOMPARE(i1, resourceSchedule.available(i1));

    // between two intervals, fill whole hole
    i1 = DateTimeInterval(DateTime(date, t3), DateTime(date.addDays(1), t1));
    QCOMPARE(i1, resourceSchedule.available(i1));

    // between two intervals, end at start of second
    i1 = DateTimeInterval(DateTime(date, t3.addSecs(1)), DateTime(date.addDays(1), t1));
    QCOMPARE(i1, resourceSchedule.available(i1));

    // start into first interval, end into second -> end of first to start of second
    i1 = DateTimeInterval(DateTime(date, t1.addSecs(1)), DateTime(date.addDays(1), t1.addSecs(1)));
    res = DateTimeInterval(DateTime(date, t3), DateTime(date.addDays(1), t1));
    QCOMPARE(res, resourceSchedule.available(i1));

}

void ScheduleTester::busy()
{
    // whole first interval
    DateTimeInterval i1(DateTime(date, t1), DateTime(date, t3));
    i1 = resourceSchedule.available(i1);
    QVERIFY(! i1.first.isValid() && ! i1.second.isValid());

    // start into interval, end at end
    i1 = DateTimeInterval(DateTime(date, t1.addSecs(100)), DateTime(date, t3));
    i1 = resourceSchedule.available(i1);
    QVERIFY(! i1.first.isValid() && ! i1.second.isValid());

    // start/end into interval
    i1 = DateTimeInterval(DateTime(date, t1.addSecs(100)), DateTime(date, t3.addSecs(-100)));
    i1 = resourceSchedule.available(i1);
    QVERIFY(! i1.first.isValid() && ! i1.second.isValid());

    // span 2 adjacent intervals, whole intervals
    i1 = DateTimeInterval(DateTime(date.addDays(1), t1), DateTime(date.addDays(1), t3));
    i1 = resourceSchedule.available(i1);
    QVERIFY(! i1.first.isValid() && ! i1.second.isValid());

    // span 2 adjacent intervals, start into first
    i1 = DateTimeInterval(DateTime(date.addDays(1), t1.addSecs(100)), DateTime(date.addDays(1), t3));
    i1 = resourceSchedule.available(i1);
    QVERIFY(! i1.first.isValid() && ! i1.second.isValid());

    // span 2 adjacent intervals, start into first, end into second
    i1 = DateTimeInterval(DateTime(date.addDays(1), t1.addSecs(100)), DateTime(date.addDays(1), t3.addSecs(-100)));
    i1 = resourceSchedule.available(i1);
    QVERIFY(! i1.first.isValid() && ! i1.second.isValid());

    // span 2 adjacent intervals, start at first, end into second
    i1 = DateTimeInterval(DateTime(date.addDays(1), t1), DateTime(date.addDays(1), t3.addSecs(-100)));
    i1 = resourceSchedule.available(i1);
    QVERIFY(! i1.first.isValid() && ! i1.second.isValid());

}

} //namespace KPlato

QTEST_GUILESS_MAIN(KPlato::ScheduleTester)
