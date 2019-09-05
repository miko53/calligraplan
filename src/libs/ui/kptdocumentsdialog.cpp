/* This file is part of the KDE project
 * Copyright (C) 2019 Dag Andersen <danders@get2net.dk>
 * Copyright (C) 2007, 2012 Dag Andersen <danders@get2net.dk>
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
#include "kptdocumentsdialog.h"
#include "kptdocumentspanel.h"
#include "kptnode.h"
#include "kptcommand.h"

#include <KLocalizedString>
#include <kactioncollection.h>

using namespace KPlato;

DocumentsDialog::DocumentsDialog(Node &node, QWidget *p, bool readOnly)
    : KoDialog(p)
    , m_panel(new DocumentsPanel(node, this))
{
    m_panel->addNodeName();
    setCaption(i18n("Task Documents"));
    if (readOnly) {
        setButtons(Close);
    } else {
        setButtons(Ok|Cancel);
        setDefaultButton(Ok);
    }
    showButtonSeparator(true);

    setMainWidget(m_panel);

    enableButtonOk(false);

    connect(m_panel, &DocumentsPanel::changed, this, &KoDialog::enableButtonOk);
}

MacroCommand *DocumentsDialog::buildCommand()
{
    return m_panel->buildCommand();
}

void DocumentsDialog::slotButtonClicked(int button)
{
    if (button == KoDialog::Ok) {
        accept();
    } else {
        KoDialog::slotButtonClicked(button);
    }
}
