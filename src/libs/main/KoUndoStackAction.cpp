/* This file is part of the KDE project

   Copyright (C) 2011 Sven Langkamp <sven.langkamp@gmail.com>

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

// clazy:excludeall=qstring-arg
#include "KoUndoStackAction.h"

#include <KoIcon.h>

#include <kundo2stack.h>
#include <klocalizedstring.h>
#include <kstandardshortcut.h>

KoUndoStackAction::KoUndoStackAction(KUndo2Stack* stack, Type type)
    : QAction(stack)
    , m_type(type)
{
    if (m_type == UNDO) {
        connect(this, &QAction::triggered, stack, &KUndo2QStack::undo);
        connect(stack, &KUndo2QStack::canUndoChanged, this, &QAction::setEnabled);
        connect(stack, &KUndo2QStack::undoTextChanged, this, &KoUndoStackAction::slotUndoTextChanged);
        setIcon(koIcon("edit-undo"));
        setText(i18n("Undo"));
        setShortcuts(KStandardShortcut::undo());
        setEnabled(stack->canUndo());
    } else {
        connect(this, &QAction::triggered, stack, &KUndo2QStack::redo);
        connect(stack, &KUndo2QStack::canRedoChanged, this, &QAction::setEnabled);
        connect(stack, &KUndo2QStack::redoTextChanged, this, &KoUndoStackAction::slotUndoTextChanged);
        setIcon(koIcon("edit-redo"));
        setText(i18n("Redo"));
        setShortcuts(KStandardShortcut::redo());
        setEnabled(stack->canRedo());
    }
}

void KoUndoStackAction::slotUndoTextChanged(const QString& text)
{
    QString actionText = (m_type == UNDO) ? i18n("Undo %1", text) : i18n("Redo %1", text);
    setText(actionText);
}
