// Copyright (c) 2019 The CEREBELLUM developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ADDRESSESWIDGET_H
#define ADDRESSESWIDGET_H

#include "qt/cerebellum/pwidget.h"
#include "addresstablemodel.h"
#include "qt/cerebellum/tooltipmenu.h"
#include "furabstractlistitemdelegate.h"
#include "qt/cerebellum/addressfilterproxymodel.h"

#include <QWidget>

class AddressViewDelegate;
class TooltipMenu;
class CEREBELLUMGUI;
class WalletModel;

namespace Ui {
class AddressesWidget;
}

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

class AddressesWidget : public PWidget
{
    Q_OBJECT

public:
    explicit AddressesWidget(CEREBELLUMGUI* parent);
    ~AddressesWidget();

    void loadWalletModel() override;
    void onNewContactClicked();

private Q_SLOTS:
    void handleAddressClicked(const QModelIndex &index);
    void onStoreContactClicked();
    void onEditClicked();
    void onDeleteClicked();
    void onCopyClicked();
    void onAddContactShowHideClicked();

    void changeTheme(bool isLightTheme, QString &theme) override;
private:
    Ui::AddressesWidget *ui;

    FurAbstractListItemDelegate* delegate = nullptr;
    AddressTableModel* addressTablemodel = nullptr;
    AddressFilterProxyModel *filter = nullptr;

    bool isOnMyAddresses = true;
    TooltipMenu* menu = nullptr;

    // Cached index
    QModelIndex index;

    void updateListView();
};

#endif // ADDRESSESWIDGET_H
