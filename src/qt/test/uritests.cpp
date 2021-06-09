// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2017-2019 The CEREBELLUM developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "uritests.h"

#include "guiutil.h"
#include "walletmodel.h"

#include <QUrl>

void URITests::uriTests()
{
    SendCoinsRecipient rv;
    QUrl uri;
    uri.setUrl(QString("cerebellum:ALDF5mjanfU3XZiSmHLhhPMGGPZd44arZn?req-dontexist="));
    QVERIFY(!GUIUtil::parseBitcoinURI(uri, &rv));

    uri.setUrl(QString("cerebellum:ALDF5mjanfU3XZiSmHLhhPMGGPZd44arZn?dontexist="));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("ALDF5mjanfU3XZiSmHLhhPMGGPZd44arZn"));
    QVERIFY(rv.label == QString());
    QVERIFY(rv.amount == 0);

    uri.setUrl(QString("cerebellum:ALDF5mjanfU3XZiSmHLhhPMGGPZd44arZn?label=Some Example Address"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("ALDF5mjanfU3XZiSmHLhhPMGGPZd44arZn"));
    QVERIFY(rv.label == QString("Some Example Address"));
    QVERIFY(rv.amount == 0);

    uri.setUrl(QString("cerebellum:ALDF5mjanfU3XZiSmHLhhPMGGPZd44arZn?amount=0.001"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("ALDF5mjanfU3XZiSmHLhhPMGGPZd44arZn"));
    QVERIFY(rv.label == QString());
    QVERIFY(rv.amount == 100000);

    uri.setUrl(QString("cerebellum:ALDF5mjanfU3XZiSmHLhhPMGGPZd44arZn?amount=1.001"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("ALDF5mjanfU3XZiSmHLhhPMGGPZd44arZn"));
    QVERIFY(rv.label == QString());
    QVERIFY(rv.amount == 100100000);

    uri.setUrl(QString("cerebellum:ALDF5mjanfU3XZiSmHLhhPMGGPZd44arZn?amount=100&label=Some Example"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("ALDF5mjanfU3XZiSmHLhhPMGGPZd44arZn"));
    QVERIFY(rv.amount == 10000000000LL);
    QVERIFY(rv.label == QString("Some Example"));

    uri.setUrl(QString("cerebellum:ALDF5mjanfU3XZiSmHLhhPMGGPZd44arZn?message=Some Example Address"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("ALDF5mjanfU3XZiSmHLhhPMGGPZd44arZn"));
    QVERIFY(rv.label == QString());

    QVERIFY(GUIUtil::parseBitcoinURI("cerebellum://ALDF5mjanfU3XZiSmHLhhPMGGPZd44arZn?message=Some Example Address", &rv));
    QVERIFY(rv.address == QString("ALDF5mjanfU3XZiSmHLhhPMGGPZd44arZn"));
    QVERIFY(rv.label == QString());

    uri.setUrl(QString("cerebellum:ALDF5mjanfU3XZiSmHLhhPMGGPZd44arZn?req-message=Some Example Address"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));

    uri.setUrl(QString("cerebellum:ALDF5mjanfU3XZiSmHLhhPMGGPZd44arZn?amount=1,000&label=Some Example"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));

    uri.setUrl(QString("cerebellum:ALDF5mjanfU3XZiSmHLhhPMGGPZd44arZn?amount=1,000.0&label=Some Example"));
    QVERIFY(!GUIUtil::parseBitcoinURI(uri, &rv));
}
