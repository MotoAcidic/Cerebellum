// Copyright (c) 2017-2019 The CEREBELLUM developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CEREBELLUM_STAKEINPUT_H
#define CEREBELLUM_STAKEINPUT_H

#include "chain.h"
#include "streams.h"
#include "uint256.h"

class CKeyStore;
class CWallet;
class CWalletTx;

class CStakeInput
{
protected:
    CBlockIndex* pindexFrom = nullptr;

public:
    virtual ~CStakeInput(){};
    virtual CBlockIndex* GetIndexFrom() = 0;
    virtual bool CreateTxIn(CWallet* pwallet, CTxIn& txIn, uint256 hashTxOut = 0) = 0;
    virtual bool GetTxFrom(CTransaction& tx) const = 0;
    virtual int64_t GetValue() const = 0;
    virtual bool CreateTxOuts(CWallet* pwallet, std::vector<CTxOut>& vout, CAmount nTotal) = 0;
    virtual bool IsZC111() const = 0;
    virtual CDataStream GetUniqueness() const = 0;

};

class CC111Stake : public CStakeInput
{
private:
    CTransaction txFrom;
    unsigned int nPosition;

public:
    CC111Stake(){}

    bool SetInput(CTransaction txPrev, unsigned int n);

    CBlockIndex* GetIndexFrom() override;
    bool GetTxFrom(CTransaction& tx) const override;
    int64_t GetValue() const override;
    CDataStream GetUniqueness() const override;
    bool CreateTxIn(CWallet* pwallet, CTxIn& txIn, uint256 hashTxOut = 0) override;
    bool CreateTxOuts(CWallet* pwallet, std::vector<CTxOut>& vout, CAmount nTotal) override;
    bool IsZC111() const override { return false; }
};


#endif //CEREBELLUM_STAKEINPUT_H
