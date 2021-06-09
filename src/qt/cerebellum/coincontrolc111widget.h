// Copyright (c) 2019 The CEREBELLUM developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COINCONTROLC111WIDGET_H
#define COINCONTROLC111WIDGET_H

#include <QDialog>

namespace Ui {
class CoinControlC111Widget;
}

class CoinControlC111Widget : public QDialog
{
    Q_OBJECT

public:
    explicit CoinControlC111Widget(QWidget *parent = nullptr);
    ~CoinControlC111Widget();

private:
    Ui::CoinControlC111Widget *ui;
};

#endif // COINCONTROLC111WIDGET_H
