// This file is part of the Silent.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#include "slistitemuser.h"

#include "View/StyleAndInfoPaths.h"

SListItemUser::SListItemUser(QString sName)
{
    QString sFormatedName = "    ";
    sFormatedName += sName;

    setText(sFormatedName);

    pRoom = nullptr;

    this->sName = sName;
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

    setText(sFormatedName);

    this->iCurrentPing = iPing;
}

void SListItemUser::setUserTalking(bool bTalking)
{
    // Set color on ping circle

    if (bTalking)
    {
        setIcon(QIcon(RES_ICONS_USERPING_NORMAL_TALK));
    }
    else
    {
        setIcon(QIcon(RES_ICONS_USERPING_NORMAL));
    }
}

SListItemRoom *SListItemUser::getRoom()
{
    return pRoom;
}

SListItemUser::~SListItemUser()
{
}
