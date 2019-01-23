/* This file is part of the KDE project
   Copyright (C) 2004-2006 David Faure <faure@kde.org>
   Copyright (C) 2007 Thorsten Zachmann <zachmann@kde.org>
   Copyright (C) 2010 Thomas Zander <zander@kde.org>
   Copyright (C) 2011 Inge Wallin <inge@lysator.liu.se>

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
#include "KoEmbeddedDocumentSaver.h"

#include <QList>

#include <OdfDebug.h>
#include <QUrl>

#include <KoStore.h>
#include <KoXmlWriter.h>
#include <KoOdfWriteStore.h>

#include "KoDocumentBase.h"
#include <KoOdfManifestEntry.h>


#define INTERNAL_PROTOCOL "intern"

struct FileEntry {
    QString path;
    QByteArray mimeType;       // QBA because this is what addManifestEntry wants
    QByteArray contents;
};


class Q_DECL_HIDDEN KoEmbeddedDocumentSaver::Private
{
public:
    Private() {}

    QHash<QString, int> prefixes; // Used in getFilename();

    // These will be saved when saveEmbeddedDocuments() is called.
    QList<KoDocumentBase*> documents; // Embedded documents
    QList<FileEntry*> files;    // Embedded files.
    QList<KoOdfManifestEntry*> manifestEntries;
};

KoEmbeddedDocumentSaver::KoEmbeddedDocumentSaver()
    : d(new Private())
{
}

KoEmbeddedDocumentSaver::~KoEmbeddedDocumentSaver()
{
    qDeleteAll(d->files);
    qDeleteAll(d->manifestEntries);
    delete d;
}


QString KoEmbeddedDocumentSaver::getFilename(const QString &prefix)
{
    int index = 1;
    if (d->prefixes.contains(prefix)) {
        index = d->prefixes.value(prefix);
    }

    // This inserts prefix into the map if it's not there.
    d->prefixes[prefix] = index + 1;

    //return prefix + QString("%1").arg(index, 4, 10, QChar('0'));
    return prefix + QString("%1").arg(index);
}

void KoEmbeddedDocumentSaver::embedDocument(KoXmlWriter &writer, KoDocumentBase * doc)
{
    Q_ASSERT(doc);
    d->documents.append(doc);

    QString ref;
    if (!doc->isStoredExtern()) {
        const QString name = getFilename("Object ");

        // set URL in document so that saveEmbeddedDocuments will save
        // the actual embedded object with the right name in the store.
        QUrl u;
        u.setScheme(INTERNAL_PROTOCOL);
        u.setPath(name);
        debugOdf << u;
        doc->setUrl(u);
        ref = "./" + name;
    } else {
        ref = doc->url().url();
    }

    debugOdf << "saving reference to embedded document as" << ref;
    writer.addAttribute("xlink:href", /*"#" + */ref);

    //<draw:object xlink:type="simple" xlink:show="embed"
    //    xlink:actuate="onLoad" xlink:href="#./Object 1"/>
    writer.addAttribute("xlink:type", "simple");
    writer.addAttribute("xlink:show", "embed");
    writer.addAttribute("xlink:actuate", "onLoad");

}

// Examples:
// Videos/Video1.mov  ← the number is autogenerated
// Videos/Video2.mov
// Object1/foo  ← the number is autogenerated
// Object1/bar

// Note: The contents QByteArray is implicitly shared.  It needs to be
//       copied since otherwise the actual array may disappear before
//       the real saving is done.
//
void KoEmbeddedDocumentSaver::embedFile(KoXmlWriter &writer, const char *element,
                                        const QString &path, const QByteArray &mimeType,
                                        const QByteArray &contents)
{
    // Put the file in the list of files to be written to the store later.
    FileEntry  *entry = new FileEntry;
    entry->mimeType = mimeType;
    entry->path = path;
    entry->contents = contents;
    d->files.append(entry);

    writer.startElement(element);
    // Write the attributes that refer to the file.

    //<draw:object xlink:href="#./Object 1" xlink:type="simple" xlink:show="embed"
    //             xlink:actuate="onLoad"/>
    writer.addAttribute("xlink:type", "simple");
    writer.addAttribute("xlink:show", "embed");
    writer.addAttribute("xlink:actuate", "onLoad");

    debugOdf << "saving reference to embedded file as" << path;
    writer.addAttribute("xlink:href", path);
    writer.endElement();
}

void KoEmbeddedDocumentSaver::saveFile(const QString &path, const QByteArray &mimeType,
                                       const QByteArray &contents)
{
    // Put the file in the list of files to be written to the store later.
    FileEntry  *entry = new FileEntry;
    entry->mimeType = mimeType;
    entry->path     = path;
    entry->contents = contents;
    d->files.append(entry);

    debugOdf << "saving reference to embedded file as" << path;
}

/**
 *
 */
void KoEmbeddedDocumentSaver::saveManifestEntry(const QString &fullPath, const QString &mediaType,
                                                const QString &version)
{
    d->manifestEntries.append(new KoOdfManifestEntry(fullPath, mediaType, version));
}


bool KoEmbeddedDocumentSaver::saveEmbeddedDocuments(KoDocumentBase::SavingContext & documentContext)
{
    KoStore *store = documentContext.odfStore.store();

    // Write embedded documents.
    foreach(KoDocumentBase *doc, d->documents) {
        QString path;
        if (doc->isStoredExtern()) {
            debugOdf << " external (don't save) url:" << doc->url().url();
            path = doc->url().url();
        } else {
            // The name comes from addEmbeddedDocument (which was set while saving the document).
            Q_ASSERT(doc->url().scheme() == INTERNAL_PROTOCOL);
            const QString name = doc->url().path();
            debugOdf << "saving" << name;

            if (doc->nativeOasisMimeType().isEmpty()) {
                // Embedded object doesn't support OpenDocument, save in the old format.
                debugOdf << "Embedded object doesn't support OpenDocument, save in the old format.";

                if (!doc->saveToStore(store, name)) {
                    return false;
                }
            } else {
                // To make the children happy cd to the correct directory
                store->pushDirectory();
                store->enterDirectory(name);

                bool ok = doc->saveOdf(documentContext);

                // Now that we're done leave the directory again
                store->popDirectory();

                if (!ok) {
                    warnOdf << "KoEmbeddedDocumentSaver::saveEmbeddedDocuments failed";
                    return false;
                }
            }

            Q_ASSERT(doc->url().scheme() == INTERNAL_PROTOCOL);
            path = store->currentPath();
            if (!path.isEmpty()) {
                path += '/';
            }
            path += doc->url().path();
            if (path.startsWith(QLatin1Char('/'))) {
                path.remove(0, 1);   // remove leading '/', no wanted in manifest
            }
        }

        // OOo uses a trailing slash for the path to embedded objects (== directories)
        if (!path.endsWith('/')) {
            path += '/';
        }
        QByteArray mimetype = doc->nativeOasisMimeType();
        documentContext.odfStore.manifestWriter()->addManifestEntry(path, mimetype);
    }

    // Write the embedded files.
    foreach(FileEntry *entry, d->files) {
        QString path = entry->path;
        debugOdf << "saving" << path;

        // To make the children happy cd to the correct directory
        store->pushDirectory();

        int index = path.lastIndexOf('/');
        const QString dirPath = path.left(index);
        const QString fileName = path.right(path.size() - index - 1);
        store->enterDirectory(dirPath);

        if (!store->open(fileName)) {
            return false;
        }
        store->write(entry->contents);
        store->close();

        // Now that we're done leave the directory again
        store->popDirectory();

        // Create the manifest entry.
        if (path.startsWith(QLatin1String("./"))) {
            path.remove(0, 2);   // remove leading './', not wanted in manifest
        }
        documentContext.odfStore.manifestWriter()->addManifestEntry(path, entry->mimeType);
    }

    // Write the manifest entries.
    KoXmlWriter *manifestWriter = documentContext.odfStore.manifestWriter();
    foreach(KoOdfManifestEntry *entry, d->manifestEntries) {
        manifestWriter->startElement("manifest:file-entry");
        manifestWriter->addAttribute("manifest:version", entry->version());
        manifestWriter->addAttribute("manifest:media-type", entry->mediaType());
        manifestWriter->addAttribute("manifest:full-path", entry->fullPath());
        manifestWriter->endElement(); // manifest:file-entry
    }

    return true;
}
