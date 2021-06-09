// Copyright (c) 2020 The CEREBELLUM developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CEREBELLUM_CONSENSUS_ZEROCOIN_VERIFY_H
#define CEREBELLUM_CONSENSUS_ZEROCOIN_VERIFY_H

#include "consensus/consensus.h"
#include "main.h"
#include "script/interpreter.h"

bool RecalculateC111Supply(int nHeightStart, bool fSkipZc111 = true);

#endif //CEREBELLUM_CONSENSUS_ZEROCOIN_VERIFY_H
