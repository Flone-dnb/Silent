#pragma once

#include <QMainWindow>

namespace Ui {
class SingleUserSettings;
}

class SingleUserSettings : public QMainWindow
{
    Q_OBJECT

signals:

    void signalChangeUserVolume(QString userName, float fVolume);

public:
    explicit SingleUserSettings(QString userName, float fCurrentVolume, QWidget *parent = nullptr);
    ~SingleUserSettings();

private slots:
    void on_horizontalSlider_valueChanged(int value);

    void on_pushButton_clicked();

private:

    Ui::SingleUserSettings *ui;

    QString userName;
    float fCurrentUserVolume;
};
