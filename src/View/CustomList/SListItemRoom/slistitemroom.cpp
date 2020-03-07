// This file is part of the Silent.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#include "slistitemroom.h"

#include <QMessageBox>

#include "View/CustomList/SListWidget/slistwidget.h"
#include "View/CustomList/SListItemUser/slistitemuser.h"


SListItemRoom::SListItemRoom(QString sName, SListWidget* pList, QString sPassword, size_t iMaxUsers)
{
    bIsWelcomeRoom = false;

    this->pList = pList;

    sRoomName = sName;
    this->sPassword = sPassword;
    this->iMaxUsers = iMaxUsers;

    setText(getRoomFullName());
}

void SListItemRoom::addUser(SListItemUser *pUser)
{
    if (vUsers.size() == iMaxUsers && iMaxUsers != 0)
    {
        QMessageBox::warning(nullptr, "Error", "Reached the maximum amount of users in this room.");
        return;
    }

    vUsers.push_back(pUser);

    pUser->setRoom(this);


    int iStartRow = pList->row(this);

    int iInsertRowIndex = -1;

    for (int i = iStartRow + 1; i < pList->count(); i++)
    {
        if (dynamic_cast<SListItem*>(pList->item(i))->isRoom())
        {
            // Found next room.
            iInsertRowIndex = i;
            break;
        }
    }


    if (iInsertRowIndex == -1)
    {
        // Next room wasn't found.
        pList->addItem(pUser);
    }
    else
    {
        pList->insertItem(iInsertRowIndex, pUser);
    }

    // Update info.
    setText(getRoomFullName());
}

void SListItemRoom::deleteUser(SListItemUser *pUser)
{
    delete pUser;

    for (size_t i = 0; i < vUsers.size(); i++)
    {
        if (vUsers[i] == pUser)
        {
            vUsers.erase( vUsers.begin() + i );
        }
    }

    // Update info.
    setText(getRoomFullName());
}

void SListItemRoom::deleteAll()
{
    for (size_t i = 0; i < vUsers.size(); i++)
    {
        delete vUsers[i];
    }

    vUsers.clear();

    // Update info.
    setText(getRoomFullName());
}

void SListItemRoom::setRoomName(QString sName)
{
    sRoomName = sName;

    setText(getRoomFullName());
}

void SListItemRoom::setRoomPassword(QString sPassword)
{
    this->sPassword = sPassword;

    setText(getRoomFullName());
}

void SListItemRoom::setRoomMaxUsers(size_t iMaxUsers)
{
    this->iMaxUsers = iMaxUsers;

    setText(getRoomFullName());
}

void SListItemRoom::setIsWelcomeRoom(bool bIsWelcomeRoom)
{
    this->bIsWelcomeRoom = bIsWelcomeRoom;
}

void SListItemRoom::eraseUserFromRoom(SListItemUser *pUser)
{
    for (size_t i = 0; i < vUsers.size(); i++)
    {
        if (vUsers[i] == pUser)
        {
            vUsers.erase( vUsers.begin() + i );
            pList->takeItem(pList->row(pUser));
        }
    }

    // Update info.
    setText(getRoomFullName());
}

std::vector<SListItemUser *> SListItemRoom::getUsers()
{
    return vUsers;
}

size_t SListItemRoom::getUsersCount()
{
    return vUsers.size();
}

QString SListItemRoom::getRoomName()
{
    return sRoomName;
}

QString SListItemRoom::getPassword()
{
    return sPassword;
}

size_t SListItemRoom::getMaxUsers()
{
    return iMaxUsers;
}

bool SListItemRoom::getIsWelcomeRoom()
{
    return bIsWelcomeRoom;
}

SListItemRoom::~SListItemRoom()
{

}

QString SListItemRoom::getRoomFullName()
{
    QFont f = font();
    f.setPointSize(11);
    setFont(f);


    QString sFullName = sRoomName;

    sFullName += "    ";

    sFullName += "(";
    if (iMaxUsers == 0)
    {
        sFullName += QString::number(vUsers.size()) + ")";
    }
    else
    {
        sFullName += QString::number(vUsers.size()) + "/" + QString::number(iMaxUsers) + ")";
    }

    return sFullName;
}
