// This file is part of the Silent.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#pragma once

#include <QWidget>
#include <QSlider>
#include <QPainter>

namespace Ui
{
    class SVoiceMeterWidget;
}

class SVoiceMeterWidget : public QSlider
{
    Q_OBJECT

public:

    explicit SVoiceMeterWidget(QWidget *parent = nullptr);

    void setStartValueInDBFS(int iValue);

    ~SVoiceMeterWidget() override;

protected:

    virtual void paintEvent(QPaintEvent *event) override;

private:

    Ui::SVoiceMeterWidget *ui;

    double iWidthScale = 0.333;
};
