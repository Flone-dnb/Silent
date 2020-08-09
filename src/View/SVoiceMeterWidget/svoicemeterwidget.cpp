// This file is part of the Silent.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#include "svoicemeterwidget.h"
#include "ui_svoicemeterwidget.h"

SVoiceMeterWidget::SVoiceMeterWidget(QWidget *parent) :
    QSlider(parent),
    ui(new Ui::SVoiceMeterWidget)
{
    ui->setupUi(this);
}

void SVoiceMeterWidget::setStartValueInDBFS(int iValue)
{
    iWidthScale = (100.0 - (abs(iValue) * 100 / 60.0)) / 100;
}

void SVoiceMeterWidget::paintEvent(QPaintEvent *event)
{
    QSlider::paintEvent(event);

    QBrush brush(Qt::SolidPattern);
    brush.setColor(QColor(0, 0, 0, 150));

    QPainter painter(this);
    painter.setBrush(brush);


    QPen pen;

    pen.setColor(Qt::transparent);
    pen.setWidth(2);

    painter.setPen(pen);

    painter.drawRect(QRect(0,0,width() * iWidthScale,height()));
}

SVoiceMeterWidget::~SVoiceMeterWidget()
{
    delete ui;
}
