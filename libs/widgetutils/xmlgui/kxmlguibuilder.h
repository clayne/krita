/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2000 Simon Hausmann <hausmann@kde.org>
   SPDX-FileCopyrightText: 2000 David Faure <faure@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef kxmlguibuilder_h
#define kxmlguibuilder_h

#include <kritawidgetutils_export.h>

#include <KisQStringListFwd.h>

class KisKXMLGUIBuilderPrivate;
class KisKXMLGUIClient;

class QAction;
class QDomElement;
class QWidget;

/**
 * Implements the creation of the GUI (menubar, menus and toolbars)
 * as requested by the GUI factory.
 *
 * The virtual methods are mostly for historical reasons, there isn't really
 * a need to derive from KisKXMLGUIBuilder anymore.
 */
class KRITAWIDGETUTILS_EXPORT KisKXMLGUIBuilder
{
public:

    explicit KisKXMLGUIBuilder(QWidget *widget);
    virtual ~KisKXMLGUIBuilder();

    /* @internal */
    KisKXMLGUIClient *builderClient() const;
    /* @internal */
    void setBuilderClient(KisKXMLGUIClient *client);
    /* @internal */
    QWidget *widget();

    virtual QStringList containerTags() const;

    /**
     * Creates a container (menubar/menu/toolbar/statusbar/separator/...)
     * from an element in the XML file
     *
     * @param parent The parent for the container
     * @param index The index where the container should be inserted
     *              into the parent container/widget
     * @param element The element from the DOM tree describing the
     *                container (use it to access container specified
     *                attributes or child elements)
     * @param containerAction The action created for this container; used for e.g. passing to removeContainer.
     */
    virtual QWidget *createContainer(QWidget *parent, int index,
                                     const QDomElement &element, QAction *&containerAction);

    /**
     * Removes the given (and previously via createContainer )
     * created container.
     *
     */
    virtual void removeContainer(QWidget *container, QWidget *parent,
                                 QDomElement &element, QAction *containerAction);

    virtual QStringList customTags() const;

    virtual QAction *createCustomElement(QWidget *parent, int index, const QDomElement &element);

    virtual void removeCustomElement(QWidget *parent, QAction *action);

    virtual void finalizeGUI(KisKXMLGUIClient *client);

protected:
    virtual void virtual_hook(int id, void *data);
private:
    KisKXMLGUIBuilderPrivate *const d;
};

#endif

