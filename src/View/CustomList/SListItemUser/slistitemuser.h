// This file is part of the Silent.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#pragma once

#include "View/CustomList/SListItem/slistitem.h"

class SListItemRoom;


class SListItemUser : public SListItem
{
public:

    SListItemUser(QString sName);


    void setRoom(SListItemRoom* pRoom);
    void setPing(int iPing);
    void setUserTalking(bool bTalking);

    SListItemRoom* getRoom();


    ~SListItemUser() override;

private:

    void setColor();

    SListItemRoom* pRoom;

    QString sName;

    int iCurrentPing;

    bool bTalking;
};
