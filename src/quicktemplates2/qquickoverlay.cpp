/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Labs Templates module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickoverlay_p.h"
#include "qquickpopup_p_p.h"
#include "qquickdrawer_p.h"
#include <QtQml/qqmlinfo.h>
#include <QtQml/qqmlproperty.h>
#include <QtQuick/private/qquickitem_p.h>

QT_BEGIN_NAMESPACE

class QQuickOverlayPrivate : public QQuickItemPrivate
{
    Q_DECLARE_PUBLIC(QQuickOverlay)

public:
    QQuickOverlayPrivate();

    void popupAboutToShow();
    void popupAboutToHide();
    void drawerPositionChange();
    void resizeBackground();

    QQuickItem *background;
    QVector<QQuickDrawer *> drawers;
    QVector<QQuickPopup *> popups;
    QPointer<QQuickPopup> mouseGrabberPopup;
    int modalPopups;
};

void QQuickOverlayPrivate::popupAboutToShow()
{
    Q_Q(QQuickOverlay);
    if (!background)
        return;

    QQuickPopup *popup = qobject_cast<QQuickPopup *>(q->sender());
    if (!popup || !popup->isModal())
        return;

    // use QQmlProperty instead of QQuickItem::setOpacity() to trigger QML Behaviors
    QQmlProperty::write(background, QStringLiteral("opacity"), 1.0);
}

void QQuickOverlayPrivate::popupAboutToHide()
{
    Q_Q(QQuickOverlay);
    if (!background || modalPopups > 1)
        return;

    QQuickPopup *popup = qobject_cast<QQuickPopup *>(q->sender());
    if (!popup || !popup->isModal())
        return;

    // use QQmlProperty instead of QQuickItem::setOpacity() to trigger QML Behaviors
    QQmlProperty::write(background, QStringLiteral("opacity"), 0.0);
}

void QQuickOverlayPrivate::drawerPositionChange()
{
    Q_Q(QQuickOverlay);
    QQuickDrawer *drawer = qobject_cast<QQuickDrawer *>(q->sender());
    if (!background || !drawer || !drawer->isModal())
        return;

    // call QQuickItem::setOpacity() directly to avoid triggering QML Behaviors
    // which would make the fading feel laggy compared to the drawer movement
    background->setOpacity(drawer->position());
}

void QQuickOverlayPrivate::resizeBackground()
{
    Q_Q(QQuickOverlay);
    background->setWidth(q->width());
    background->setHeight(q->height());
}

QQuickOverlayPrivate::QQuickOverlayPrivate() :
    background(nullptr),
    modalPopups(0)
{
}

QQuickOverlay::QQuickOverlay(QQuickItem *parent)
    : QQuickItem(*(new QQuickOverlayPrivate), parent)
{
    setAcceptedMouseButtons(Qt::AllButtons);
    setFiltersChildMouseEvents(true);
    setVisible(false);
}


QQuickItem *QQuickOverlay::background() const
{
    Q_D(const QQuickOverlay);
    return d->background;
}

void QQuickOverlay::setBackground(QQuickItem *background)
{
    Q_D(QQuickOverlay);
    if (d->background == background)
        return;

    delete d->background;
    d->background = background;
    if (background) {
        background->setOpacity(0.0);
        background->setParentItem(this);
        if (qFuzzyIsNull(background->z()))
            background->setZ(-1);
        if (isComponentComplete())
            d->resizeBackground();
    }
    emit backgroundChanged();
}

void QQuickOverlay::itemChange(ItemChange change, const ItemChangeData &data)
{
    Q_D(QQuickOverlay);
    QQuickItem::itemChange(change, data);

    QQuickPopup *popup = nullptr;
    if (change == ItemChildAddedChange || change == ItemChildRemovedChange) {
        popup = qobject_cast<QQuickPopup *>(data.item->parent());
        setVisible(!childItems().isEmpty());
    }
    if (!popup)
        return;

    if (change == ItemChildAddedChange) {
        d->popups.append(popup);

        QQuickDrawer *drawer = qobject_cast<QQuickDrawer *>(popup);
        if (drawer) {
            QObjectPrivate::connect(drawer, &QQuickDrawer::positionChanged, d, &QQuickOverlayPrivate::drawerPositionChange);
            d->drawers.append(drawer);
        } else {
            if (popup->isModal())
                ++d->modalPopups;

            QObjectPrivate::connect(popup, &QQuickPopup::aboutToShow, d, &QQuickOverlayPrivate::popupAboutToShow);
            QObjectPrivate::connect(popup, &QQuickPopup::aboutToHide, d, &QQuickOverlayPrivate::popupAboutToHide);
        }
    } else if (change == ItemChildRemovedChange) {
        d->popups.removeOne(popup);

        QQuickDrawer *drawer = qobject_cast<QQuickDrawer *>(popup);
        if (drawer) {
            QObjectPrivate::disconnect(drawer, &QQuickDrawer::positionChanged, d, &QQuickOverlayPrivate::drawerPositionChange);
            d->drawers.removeOne(drawer);
        } else {
            if (popup->isModal())
                --d->modalPopups;

            QObjectPrivate::disconnect(popup, &QQuickPopup::aboutToShow, d, &QQuickOverlayPrivate::popupAboutToShow);
            QObjectPrivate::disconnect(popup, &QQuickPopup::aboutToHide, d, &QQuickOverlayPrivate::popupAboutToHide);
        }
    }
}

void QQuickOverlay::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickOverlay);
    QQuickItem::geometryChanged(newGeometry, oldGeometry);
    if (d->background)
        d->resizeBackground();
}

bool QQuickOverlay::event(QEvent *event)
{
    Q_D(QQuickOverlay);
    switch (event->type()) {
    case QEvent::MouseButtonPress:
        emit pressed();
        for (auto it = d->popups.crbegin(), end = d->popups.crend(); it != end; ++it) {
            if ((*it)->overlayEvent(this, event)) {
                d->mouseGrabberPopup = *it;
                return true;
            }
        }
        break;
    case QEvent::MouseMove:
        if (d->mouseGrabberPopup) {
            if (d->mouseGrabberPopup->overlayEvent(this, event))
                return true;
        } else {
            for (auto it = d->popups.crbegin(), end = d->popups.crend(); it != end; ++it) {
                if ((*it)->overlayEvent(this, event))
                    return true;
            }
        }
        break;
    case QEvent::MouseButtonRelease:
        emit released();
        if (d->mouseGrabberPopup) {
            QQuickPopup *grabber = d->mouseGrabberPopup;
            d->mouseGrabberPopup = nullptr;
            if (grabber->overlayEvent(this, event))
                return true;
        } else {
            for (auto it = d->popups.crbegin(), end = d->popups.crend(); it != end; ++it) {
                if ((*it)->overlayEvent(this, event))
                    return true;
            }
        }
        break;
    default:
        break;
    }

    return QQuickItem::event(event);
}

bool QQuickOverlay::childMouseEventFilter(QQuickItem *item, QEvent *event)
{
    Q_D(QQuickOverlay);
    if (d->modalPopups == 0)
        return false;
    // TODO Filter touch events
    if (event->type() != QEvent::MouseButtonPress)
        return false;
    while (item->parentItem() != this)
        item = item->parentItem();

    const QList<QQuickItem *> sortedChildren = d->paintOrderChildItems();
    for (auto it = sortedChildren.rbegin(), end = sortedChildren.rend(); it != end; ++it) {
        QQuickItem *popupItem = *it;
        if (popupItem == item)
            break;

        QQuickPopup *popup = qobject_cast<QQuickPopup *>(popupItem->parent());
        if (popup && popup->overlayEvent(item, event))
            return true;
    }

    return false;
}

QT_END_NAMESPACE