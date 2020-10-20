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
    pRoomNameLabel = nullptr;
    pRoomPropsLabel = nullptr;

    pUIWidget = nullptr;

    setupUI();

    bIsWelcomeRoom = false;

    this->pList = pList;

    sRoomName = sName;
    this->sPassword = sPassword;

    this->iMaxUsers = iMaxUsers;

    updateText();
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
    updateText();
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
    updateText();
}

void SListItemRoom::setupUI()
{
    if (pRoomNameLabel)
    {
        delete pRoomNameLabel;
    }

    if (pRoomPropsLabel)
    {
       delete pRoomPropsLabel;
    }

    if (pUIWidget)
    {
        delete pUIWidget;
    }

    pUIWidget = new QWidget();
    QHBoxLayout* pLayout = new QHBoxLayout(pUIWidget);

    QFont font;
    font.setFamily("Segoe UI");
    font.setPointSize(11);
    font.setKerning(true);

    pRoomNameLabel = new QLabel("");
    pRoomNameLabel->setFont(font);
    pRoomNameLabel->setAlignment(Qt::AlignmentFlag::AlignLeft | Qt::AlignmentFlag::AlignVCenter);
    pRoomNameLabel->setMargin(0);

    pRoomPropsLabel = new QLabel("");
    pRoomPropsLabel->setFont(font);
    pRoomPropsLabel->setAlignment(Qt::AlignmentFlag::AlignRight | Qt::AlignmentFlag::AlignVCenter);
    pRoomPropsLabel->setMargin(0);

    pLayout->addWidget(pRoomNameLabel);
    pLayout->addWidget(pRoomPropsLabel);

    pLayout->setStretch(0, 80);
    pLayout->setStretch(1, 20);
    pLayout->setContentsMargins(5, 2, 15, 2);

    pLayout->setSizeConstraint(QLayout::SetDefaultConstraint);

    setSizeHint(pUIWidget->sizeHint());
}

void SListItemRoom::deleteAll()
{
    for (size_t i = 0; i < vUsers.size(); i++)
    {
        delete vUsers[i];
    }

    vUsers.clear();

    // Update info.
    updateText();
}

void SListItemRoom::setRoomName(QString sName)
{
    sRoomName = sName;

    updateText();
}

void SListItemRoom::setRoomPassword(QString sPassword)
{
    this->sPassword = sPassword;

    updateText();
}

void SListItemRoom::setRoomMaxUsers(size_t iMaxUsers)
{
    this->iMaxUsers = iMaxUsers;

    updateText();
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

            break;
        }
    }

    // Update info.
    updateText();
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

QWidget* SListItemRoom::getUIWidget()
{
    return pUIWidget;
}

SListItemRoom::~SListItemRoom()
{
    delete pRoomNameLabel;
    delete pRoomPropsLabel;

    delete pUIWidget;
}

void SListItemRoom::updateText()
{
    pRoomNameLabel->setText(sRoomName);

    if (iMaxUsers != 0)
    {
        pRoomPropsLabel->setText(QString::number(vUsers.size()) + " / " + QString::number(iMaxUsers));
    }
    else
    {
        pRoomPropsLabel->setText(QString::number(vUsers.size()));
    }
}
