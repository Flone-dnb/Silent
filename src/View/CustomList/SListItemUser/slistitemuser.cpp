// This file is part of the Silent.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#include "slistitemuser.h"

#include "View/StyleAndInfoPaths.h"
#include <QFont>

SListItemUser::SListItemUser(QString sName)
{
    QString sFormatedName = "    ";
    sFormatedName += sName;

    bTalking = false;

    pRoom = nullptr;

    this->sName = sName;

    setText(sFormatedName);
}

void SListItemUser::setRoom(SListItemRoom *pRoom)
{
    this->pRoom = pRoom;
}

void SListItemUser::setPing(int iPing)
{
    QString sFormatedName = "    ";
    sFormatedName += this->sName;
    sFormatedName += " [" + QString::number(iPing) + " ms]";

    setColor();

    setText(sFormatedName);

    this->iCurrentPing = iPing;
}

void SListItemUser::setUserTalking(bool bTalking)
{
    this->bTalking = bTalking;
}

SListItemRoom *SListItemUser::getRoom()
{
    return pRoom;
}

SListItemUser::~SListItemUser()
{
}

void SListItemUser::setColor()
{
    if (bTalking)
    {
        setForeground(QBrush(QColor(240, 0, 0)));
    }
    else
    {
        setForeground(QBrush(QColor(Qt::white)));
    }
}
