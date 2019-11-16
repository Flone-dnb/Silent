#pragma once


// Qt
#include <QMainWindow>


namespace Ui
{
    class SingleUserSettings;
}


// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------


class SingleUserSettings : public QMainWindow
{
    Q_OBJECT

public:

    explicit SingleUserSettings                   (QString userName, float fCurrentVolume, QWidget *parent = nullptr);

    ~SingleUserSettings                           ();

signals:

    void  signalChangeUserVolume                  (QString userName, float fVolume);

private slots:

    void  on_horizontalSlider_volume_valueChanged (int value);
    void  on_pushButton_clicked                   ();

private:

    Ui::SingleUserSettings* ui;

    QString                 userName;
    float                   fCurrentUserVolume;
};
