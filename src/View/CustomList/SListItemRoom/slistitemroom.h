// This file is part of the Silent.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#pragma once

// Qt
#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>

// STL
#include <vector>

// Custom
#include "View/CustomList/SListItem/slistitem.h"


class SListItemUser;
class SListWidget;


class SListItemRoom : public SListItem
{
public:

    SListItemRoom(QString sName, SListWidget* pList, QString sPassword = "", size_t iMaxUsers = 0);

    void addUser(SListItemUser* pUser);
    void deleteUser(SListItemUser* pUser);

    void deleteAll();


    void    setRoomName     (QString sName);
    void    setRoomPassword (QString sPassword);
    void    setRoomMaxUsers (size_t iMaxUsers);

    void    setIsWelcomeRoom(bool bIsWelcomeRoom);


    void    eraseUserFromRoom(SListItemUser* pUser);

    void    updateText();


    std::vector<SListItemUser*> getUsers();
    size_t  getUsersCount();
    QString getRoomName ();
    QString getPassword ();
    size_t  getMaxUsers ();
    bool    getIsWelcomeRoom();
    QWidget* getUIWidget();



    ~SListItemRoom() override;

private:

    friend class SListWidget;

    void setupUI();



    std::vector<SListItemUser*> vUsers;


    QWidget*     pUIWidget;
    QHBoxLayout* pLayout;
    QLabel*      pRoomNameLabel;
    QLabel*      pRoomPropsLabel;
    SListWidget* pList;


    QString sRoomName;
    QString sPassword;


    size_t  iMaxUsers;


    bool    bIsWelcomeRoom;
};
