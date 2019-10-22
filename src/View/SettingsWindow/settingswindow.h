#pragma once

#include <QMainWindow>


class QKeyEvent;
class QMouseEvent;


namespace Ui {
class SettingsWindow;
}

class SettingsWindow : public QMainWindow
{
    Q_OBJECT

public:

    explicit SettingsWindow(QWidget *parent = nullptr);

    ~SettingsWindow();

signals:

    void signalSetPushToTalkButton(int iKey, unsigned short int volume);

protected:

    void keyPressEvent(QKeyEvent* event);
    void mousePressEvent(QMouseEvent *event);

private slots:

    void on_pushButton_clicked();
    void on_pushButton_2_clicked();
    void on_horizontalSlider_valueChanged(int value);

private:

    void showKeyOnScreen(int key);
    void setKeyInSettingsFile(int key);

    void refreshSliderText();



    int iPushToTalkKey;
    bool bWaitingForButton;


    Ui::SettingsWindow *ui;
};
