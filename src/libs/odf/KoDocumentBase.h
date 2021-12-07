/* This file is part of the KDE project

   SPDX-FileCopyrightText: 1998, 1999 Torben Weis <weis@kde.org>
   SPDX-FileCopyrightText: 2000-2005 David Faure <faure@kde.org>
   SPDX-FileCopyrightText: 2007 Thorsten Zachmann <zachmann@kde.org>
   SPDX-FileCopyrightText: 2009 Boudewijn Rempt <boud@valdyas.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef KODOCUMENTBASE_H
#define KODOCUMENTBASE_H

class KoStore;
class KoOdfReadStore;
class KoOdfWriteStore;
class KoEmbeddedDocumentSaver;

class QUrl;
class QByteArray;
class QString;

#include "koodf_export.h"

/**
 * Base class for documents that can load and save ODF. Most of the
 * implementation is still in KoDocument, though that should probably
 * change.
 */
class KOODF_EXPORT KoDocumentBase
{
public:

    // context passed on saving to saveOdf
    struct SavingContext {
        SavingContext(KoOdfWriteStore &odfStore, KoEmbeddedDocumentSaver &embeddedSaver)
                : odfStore(odfStore)
                , embeddedSaver(embeddedSaver) {}

        KoOdfWriteStore &odfStore;
        KoEmbeddedDocumentSaver &embeddedSaver;
    };

    /**
     * create a new KoDocumentBase
     */
    KoDocumentBase();

    /**
     * delete this document
     */
    virtual ~KoDocumentBase();

    /**
     * Return true if url() is a real filename, false if url() is
     * an internal url in the store, like "tar:/..."
     */
    virtual bool isStoredExtern() const = 0;

    /**
     * @return the current URL
     */
    virtual QUrl url() const = 0;

    virtual void setUrl(const QUrl &url) = 0;

    /**
     * Returns the non OASIS mimetype of the document, if OASIS is not supported
     * This comes from the X-KDE-NativeMimeType key in the .desktop file
     */
    virtual QByteArray nativeFormatMimeType() const = 0;

    /**
     * Returns the OASIS OpenDocument mimetype of the document, if supported
     * This comes from the X-KDE-NativeOasisMimeType key in the .desktop file
     */
    virtual QByteArray nativeOasisMimeType() const = 0;

    /**
     *  @brief Saves a document to a store.
     */
    virtual bool saveToStore(KoStore *store, const QString &path) = 0;

    /**
     *  Reimplement this method to load the odf document. Take care to
     *  make sure styles are loaded before body text is loaded by the
     *  text shape.
     */
    virtual bool loadOdf(KoOdfReadStore &odfStore) = 0;

    /**
     *  Reimplement this method to save the contents of your %Calligra document,
     *  using the ODF format.
     */
    virtual bool saveOdf(SavingContext &documentContext) = 0;

    /**
     * Checks whether the document is currently in the process of autosaving
     */
    virtual bool isAutosaving() const = 0;

    /**
     * Returns true if this document or any of its internal child documents are modified.
     */
    virtual bool isModified() const = 0;

    /**
     *  @return true if the document is empty.
     */
    virtual bool isEmpty() const = 0;

    /**
     * Returns the actual mimetype of the document
     */
    virtual QByteArray mimeType() const = 0;

    /**
     * @brief Sets the mime type for the document.
     *
     * When choosing "save as" this is also the mime type
     * selected by default.
     */
    virtual void setMimeType(const QByteArray & mimeType) = 0;

    virtual QString localFilePath() const = 0;

    /**
     * Return the set of SupportedSpecialFormats that the application wants to
     * offer in the "Save" file dialog.
     */
    virtual int supportedSpecialFormats() const = 0;

    /// Enum values used by specialOutputFlag - note that it's a bitfield for supportedSpecialFormats
    enum { /*SaveAsCalligra1dot1 = 1,*/ // old and removed
        SaveAsDirectoryStore = 2,
        SaveAsFlatXML = 4,
        SaveEncrypted = 8
                        // bitfield! next value is 16
    };
    virtual int specialOutputFlag() const = 0;

    /**
     * @brief Set the format in which the document should be saved.
     *
     * This is called on loading, and in "save as", so you shouldn't
     * have to call it.
     *
     * @param mimeType the mime type (format) to use.
     * @param specialOutputFlag is for "save as older version" etc.
     */
    virtual void setOutputMimeType(const QByteArray & mimeType, int specialOutputFlag = 0) = 0;

    virtual QByteArray outputMimeType() const = 0;

    /**
     * Sets the document URL to empty URL
     * KParts doesn't allow this, but %Calligra apps have e.g. templates
     * After using loadNativeFormat on a template, one wants
     * to set the url to QUrl()
     */
    virtual void resetURL() = 0;

private:
    class Private;
    Private *const d;
};


#endif
