// Copyright (c) 2011-2013 The PPCoin developers
// Copyright (c) 2013-2014 The NovaCoin Developers
// Copyright (c) 2014-2018 The BlackCoin Developers
// Copyright (c) 2015-2020 The CEREBELLUM developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/assign/list_of.hpp>

#include "db.h"
#include "kernel.h"
#include "script/interpreter.h"
#include "util.h"
#include "stakeinput.h"
#include "utilmoneystr.h"

/*
 * PoS Validation
 */

bool GetHashProofOfStake(const CBlockIndex* pindexPrev, CStakeInput* stake, const unsigned int nTimeTx, const bool fVerify, uint256& hashProofOfStakeRet) {
    // Grab the stake data
    CBlockIndex* pindexfrom = stake->GetIndexFrom();
    if (!pindexfrom) return error("%s : Failed to find the block index for stake origin", __func__);
    const CDataStream& ssUniqueID = stake->GetUniqueness();
    const unsigned int nTimeBlockFrom = pindexfrom->nTime;
    CDataStream modifier_ss(SER_GETHASH, 0);

    // Hash the modifier
    if (!Params().GetConsensus().IsStakeModifierV2(pindexPrev->nHeight + 1)) {
        // Modifier v1
        uint64_t nStakeModifier = 0;
        if (!GetOldStakeModifier(stake, nStakeModifier))
            return error("%s : Failed to get kernel stake modifier", __func__);
        modifier_ss << nStakeModifier;
    } else {
        // Modifier v2
        modifier_ss << pindexPrev->GetStakeModifierV2();
    }

    CDataStream ss(modifier_ss);
    // Calculate hash
    ss << nTimeBlockFrom << ssUniqueID << nTimeTx;
    hashProofOfStakeRet = Hash(ss.begin(), ss.end());

    if (fVerify) {
        LogPrint("staking", "%s : nStakeModifier=%s\nnTimeBlockFrom=%d\nssUniqueIDD=%s\n-->DATA=%s",
            __func__, HexStr(modifier_ss), nTimeBlockFrom, HexStr(ssUniqueID), HexStr(ss));
    }
    return true;
}

bool CheckStakeKernelHash(const CBlockIndex* pindexPrev, const unsigned int nBits, CStakeInput* stake, const unsigned int nTimeTx, uint256& hashProofOfStake, const bool fVerify)
{
    // Calculate the proof of stake hash
    if (!GetHashProofOfStake(pindexPrev, stake, nTimeTx, fVerify, hashProofOfStake)) {
        return error("%s : Failed to calculate the proof of stake hash", __func__);
    }

    const CAmount& nValueIn = stake->GetValue();
    const CDataStream& ssUniqueID = stake->GetUniqueness();

    // Base target
    uint256 bnTarget;
    bnTarget.SetCompact(nBits);
    bnTarget *= uint256(nValueIn) / 100;

    bool res = hashProofOfStake < bnTarget;
        LogPrint("staking", "%s : Proof Of Stake:"
                            "\nssUniqueID=%s"
                            "\nnTimeTx=%d"
                            "\nhashProofOfStake=%s"
                            "\nnBits=%d"
                            "\nweight=%d"
                            "\nbnTarget=%s (res: %d)\n\n",
            __func__, HexStr(ssUniqueID), nTimeTx, hashProofOfStake.GetHex(),
            nBits, nValueIn, bnTarget.GetHex(), res);
    return res;
}

bool CheckProofOfStake(const CBlock& block, uint256& hashProofOfStake, std::unique_ptr<CStakeInput>& stake, int nPreviousBlockHeight)
{
    // Initialize the stake object
    if(!initStakeInput(block, stake, nPreviousBlockHeight))
        return error("%s : stake input object initialization failed", __func__);

    const CTransaction tx = block.vtx[1];
    // Kernel (input 0) must match the stake hash target per coin age (nBits)
    const CTxIn& txin = tx.vin[0];
    CBlockIndex* pindexPrev = mapBlockIndex[block.hashPrevBlock];
    CBlockIndex* pindexfrom = stake->GetIndexFrom();
    if (!pindexfrom)
        return error("%s : Failed to find the block index for stake origin", __func__);

    unsigned int nBlockFromTime = pindexfrom->nTime;
    unsigned int nTxTime = block.nTime;
    const int nBlockFromHeight = pindexfrom->nHeight;
    const Consensus::Params& consensus = Params().GetConsensus();

    if (nPreviousBlockHeight >= consensus.height_start_StakeModifierV2 - 1) {
        //check for maturity (min age/depth) requirements
        if (!consensus.HasStakeMinAgeOrDepth(nPreviousBlockHeight+1, nTxTime, nBlockFromHeight, nBlockFromTime))
            return error("%s : min age violation - height=%d - nTimeTx=%d, nTimeBlockFrom=%d, nHeightBlockFrom=%d",
                             __func__, nPreviousBlockHeight, nTxTime, nBlockFromTime, nBlockFromHeight);
    }

    if (!CheckStakeKernelHash(pindexPrev, block.nBits, stake.get(), nTxTime, hashProofOfStake, true))
        return error("%s : INFO: check kernel failed on coinstake %s, hashProof=%s", __func__,
                     tx.GetHash().GetHex(), hashProofOfStake.GetHex());

    return true;
}

// Initialize the stake input object
bool initStakeInput(const CBlock& block, std::unique_ptr<CStakeInput>& stake, int nPreviousBlockHeight) {
    const CTransaction tx = block.vtx[1];
    if (!tx.IsCoinStake())
        return error("%s : called on non-coinstake %s", __func__, tx.GetHash().ToString().c_str());

    // Kernel (input 0) must match the stake hash target per coin age (nBits)
    const CTxIn& txin = tx.vin[0];
        // First try finding the previous transaction in database
        uint256 hashBlock;
        CTransaction txPrev;
        if (!GetTransaction(txin.prevout.hash, txPrev, hashBlock, true))
            return error("%s : INFO: read txPrev failed, tx id prev: %s, block id %s",
                         __func__, txin.prevout.hash.GetHex(), block.GetHash().GetHex());

        //verify signature and script
        ScriptError serror;
        if (!VerifyScript(txin.scriptSig, txPrev.vout[txin.prevout.n].scriptPubKey, STANDARD_SCRIPT_VERIFY_FLAGS, TransactionSignatureChecker(&tx, 0), &serror)) {
            std::string strErr = "";
            if (serror && ScriptErrorString(serror))
                strErr = strprintf("with the following error: %s", ScriptErrorString(serror));
            return error("%s : VerifyScript failed on coinstake %s %s", __func__, tx.GetHash().ToString(), strErr);
        }

        CC111Stake* c111Input = new CC111Stake();
        c111Input->SetInput(txPrev, txin.prevout.n);
        stake = std::unique_ptr<CStakeInput>(c111Input);
    return true;
}

// Stake Modifier (hash modifier of proof-of-stake):
// The purpose of stake modifier is to prevent a txout (coin) owner from
// computing future proof-of-stake generated by this txout at the time
// of transaction confirmation. To meet kernel protocol, the txout
// must hash with a future stake modifier to generate the proof.
uint256 ComputeStakeModifier(const CBlockIndex* pindexPrev, const uint256& kernel)
{
    // genesis block's modifier is 0
    if (!pindexPrev) return uint256();

    CHashWriter ss(SER_GETHASH, 0);
    ss << kernel;

    // switch with old modifier on upgrade block
    if (!Params().GetConsensus().IsStakeModifierV2(pindexPrev->nHeight + 1))
        ss << pindexPrev->GetStakeModifierV1();
    else
        ss << pindexPrev->GetStakeModifierV2();

    return ss.GetHash();
}

bool Stake(const CBlockIndex* pindexPrev, CStakeInput* stakeInput, unsigned int nBits, int64_t& nTimeTx, uint256& hashProofOfStake)
{
    const int nHeight = pindexPrev->nHeight + 1;
    // get stake input pindex
    CBlockIndex* pindexFrom = stakeInput->GetIndexFrom();
    if (!pindexFrom || pindexFrom->nHeight < 1) return error("%s : no pindexfrom", __func__);

    // check required min depth for stake
    const int nHeightBlockFrom = pindexFrom->nHeight;
    if (nHeight < nHeightBlockFrom + Params().GetConsensus().nStakeMinDepth)
        return error("%s : min depth violation, nHeight=%d, nHeightBlockFrom=%d", __func__, nHeight, nHeightBlockFrom);

    const bool fRegTest = Params().IsRegTestNet();
    nTimeTx = (fRegTest ? GetAdjustedTime() : GetCurrentTimeSlot());
    //double check that we are not on the same slot as prev block
    if (nTimeTx <= pindexPrev->nTime && !fRegTest) return false;

    // check stake kernel
    return CheckStakeKernelHash(pindexPrev, nBits, stakeInput, nTimeTx, hashProofOfStake);
}


/*
 * UTILS
 */

// Timestamp for time protocol V2: slot duration 15 seconds
int64_t GetTimeSlot(const int64_t nTime)
{
    const int slotLen = Params().GetConsensus().nTimeSlotLength;
    return (nTime / slotLen) * slotLen;
}

int64_t GetCurrentTimeSlot()
{
    return GetTimeSlot(GetAdjustedTime());
}

/*
 * OLD MODIFIER
 */
static const unsigned int MODIFIER_INTERVAL = 60;
static const int MODIFIER_INTERVAL_RATIO = 3;
static const int64_t OLD_MODIFIER_INTERVAL = 2087;


// Get selection interval section (in seconds)
static int64_t GetStakeModifierSelectionIntervalSection(int nSection)
{
    assert(nSection >= 0 && nSection < 64);
    int64_t a = MODIFIER_INTERVAL  * 63 / (63 + ((63 - nSection) * (MODIFIER_INTERVAL_RATIO - 1)));
    return a;
}

// select a block from the candidate blocks in vSortedByTimestamp, excluding
// already selected blocks in vSelectedBlocks, and with timestamp up to
// nSelectionIntervalStop.
static bool SelectBlockFromCandidates(
    std::vector<std::pair<int64_t, uint256> >& vSortedByTimestamp,
    std::map<uint256, const CBlockIndex*>& mapSelectedBlocks,
    int64_t nSelectionIntervalStop,
    uint64_t nStakeModifierPrev,
    const CBlockIndex** pindexSelected)
{
    bool fModifierV2 = false;
    bool fFirstRun = true;
    bool fSelected = false;
    uint256 hashBest;
    *pindexSelected = (const CBlockIndex*)0;
    for (const PAIRTYPE(int64_t, uint256) & item : vSortedByTimestamp) {
        if (!mapBlockIndex.count(item.second))
            return error("%s : failed to find block index for candidate block %s", __func__, item.second.ToString().c_str());

        const CBlockIndex* pindex = mapBlockIndex[item.second];
        if (fSelected && pindex->GetBlockTime() > nSelectionIntervalStop)
            break;

        //if the lowest block height (vSortedByTimestamp[0]) is >= switch height, use new modifier calc
        if (fFirstRun){
            fModifierV2 = pindex->nHeight >= Params().GetConsensus().height_start_StakeModifierNewSelection;
            fFirstRun = false;
        }

        if (mapSelectedBlocks.count(pindex->GetBlockHash()) > 0)
            continue;

        // compute the selection hash by hashing an input that is unique to that block
        uint256 hashProof;
        if(fModifierV2)
            hashProof = pindex->GetBlockHash();
        else
            hashProof = pindex->IsProofOfStake() ? 0 : pindex->GetBlockHash();

        CDataStream ss(SER_GETHASH, 0);
        ss << hashProof << nStakeModifierPrev;
        uint256 hashSelection = Hash(ss.begin(), ss.end());

        // the selection hash is divided by 2**32 so that proof-of-stake block
        // is always favored over proof-of-work block. this is to preserve
        // the energy efficiency property
        if (pindex->IsProofOfStake())
            hashSelection >>= 32;

        if (fSelected && hashSelection < hashBest) {
            hashBest = hashSelection;
            *pindexSelected = (const CBlockIndex*)pindex;
        } else if (!fSelected) {
            fSelected = true;
            hashBest = hashSelection;
            *pindexSelected = (const CBlockIndex*)pindex;
        }
    }
    if (GetBoolArg("-printstakemodifier", false))
        LogPrintf("%s : selection hash=%s\n", __func__, hashBest.ToString().c_str());
    return fSelected;
}

bool GetOldStakeModifier(CStakeInput* stake, uint64_t& nStakeModifier)
{
    if(Params().IsRegTestNet()) {
        nStakeModifier = 0;
        return true;
    }
    CBlockIndex* pindexFrom = stake->GetIndexFrom();
    if (!pindexFrom) return error("%s : failed to get index from", __func__);
    if (!GetOldModifier(pindexFrom->GetBlockHash(), nStakeModifier))
        return error("%s : failed to get kernel stake modifier", __func__);

    return true;
}

// The stake modifier used to hash for a stake kernel is chosen as the stake
// modifier about a selection interval later than the coin generating the kernel
bool GetOldModifier(const uint256& hashBlockFrom, uint64_t& nStakeModifier)
{
    if (!mapBlockIndex.count(hashBlockFrom))
        return error("%s : block not indexed", __func__);
    const CBlockIndex* pindexFrom = mapBlockIndex[hashBlockFrom];
    int64_t nStakeModifierTime = pindexFrom->GetBlockTime();
    const CBlockIndex* pindex = pindexFrom;
    CBlockIndex* pindexNext = chainActive[pindex->nHeight + 1];

    // loop to find the stake modifier later by a selection interval
    do {
        if (!pindexNext) {
            // Should never happen
            return error("%s : Null pindexNext, current block %s ", __func__, pindex->phashBlock->GetHex());
        }
        pindex = pindexNext;
        if (pindex->GeneratedStakeModifier()) nStakeModifierTime = pindex->GetBlockTime();
        pindexNext = chainActive[pindex->nHeight + 1];
    } while (nStakeModifierTime < pindexFrom->GetBlockTime() + OLD_MODIFIER_INTERVAL);

    nStakeModifier = pindex->GetStakeModifierV1();
    return true;
}

// Stake Modifier (hash modifier of proof-of-stake):
// The purpose of stake modifier is to prevent a txout (coin) owner from
// computing future proof-of-stake generated by this txout at the time
// of transaction confirmation. To meet kernel protocol, the txout
// must hash with a future stake modifier to generate the proof.
// Stake modifier consists of bits each of which is contributed from a
// selected block of a given block group in the past.
// The selection of a block is based on a hash of the block's proof-hash and
// the previous stake modifier.
// Stake modifier is recomputed at a fixed time interval instead of every
// block. This is to make it difficult for an attacker to gain control of
// additional bits in the stake modifier, even after generating a chain of
// blocks.
bool ComputeNextStakeModifier(const CBlockIndex* pindexPrev, uint64_t& nStakeModifier, bool& fGeneratedStakeModifier)
{
    nStakeModifier = 0;
    fGeneratedStakeModifier = false;

    if (!pindexPrev) {
        fGeneratedStakeModifier = true;
        return true; // genesis block's modifier is 0
    }
    if (pindexPrev->nHeight == 0) {
        //Give a stake modifier to the first block
        fGeneratedStakeModifier = true;
        nStakeModifier = uint64_t("stakemodifier");
        return true;
    }

    // First find current stake modifier and its generation block time
    // if it's not old enough, return the same stake modifier
    int64_t nModifierTime = 0;
    const CBlockIndex* p = pindexPrev;
    while (p && p->pprev && !p->GeneratedStakeModifier()) p = p->pprev;
    if (!p->GeneratedStakeModifier()) return error("%s : unable to get last modifier", __func__);
    nStakeModifier = p->GetStakeModifierV1();
    nModifierTime = p->GetBlockTime();

    if (GetBoolArg("-printstakemodifier", false))
        LogPrintf("%s : prev modifier= %s time=%s\n", __func__, std::to_string(nStakeModifier).c_str(), DateTimeStrFormat("%Y-%m-%d %H:%M:%S", nModifierTime).c_str());

    if (nModifierTime / MODIFIER_INTERVAL >= pindexPrev->GetBlockTime() / MODIFIER_INTERVAL)
        return true;

    // Sort candidate blocks by timestamp
    std::vector<std::pair<int64_t, uint256> > vSortedByTimestamp;
    vSortedByTimestamp.reserve(64 * MODIFIER_INTERVAL  / Params().GetConsensus().nTargetSpacing);
    int64_t nSelectionIntervalStart = (pindexPrev->GetBlockTime() / MODIFIER_INTERVAL ) * MODIFIER_INTERVAL  - OLD_MODIFIER_INTERVAL;
    const CBlockIndex* pindex = pindexPrev;

    while (pindex && pindex->GetBlockTime() >= nSelectionIntervalStart) {
        vSortedByTimestamp.push_back(std::make_pair(pindex->GetBlockTime(), pindex->GetBlockHash()));
        pindex = pindex->pprev;
    }

    int nHeightFirstCandidate = pindex ? (pindex->nHeight + 1) : 0;
    std::reverse(vSortedByTimestamp.begin(), vSortedByTimestamp.end());
    std::sort(vSortedByTimestamp.begin(), vSortedByTimestamp.end());

    // Select 64 blocks from candidate blocks to generate stake modifier
    uint64_t nStakeModifierNew = 0;
    int64_t nSelectionIntervalStop = nSelectionIntervalStart;
    std::map<uint256, const CBlockIndex*> mapSelectedBlocks;
    for (int nRound = 0; nRound < std::min(64, (int)vSortedByTimestamp.size()); nRound++) {
        // add an interval section to the current selection round
        nSelectionIntervalStop += GetStakeModifierSelectionIntervalSection(nRound);

        // select a block from the candidates of current round
        if (!SelectBlockFromCandidates(vSortedByTimestamp, mapSelectedBlocks, nSelectionIntervalStop, nStakeModifier, &pindex))
            return error("%s : unable to select block at round %d", __func__, nRound);

        // write the entropy bit of the selected block
        nStakeModifierNew |= (((uint64_t)pindex->GetStakeEntropyBit()) << nRound);

        // add the selected block from candidates to selected list
        mapSelectedBlocks.insert(std::make_pair(pindex->GetBlockHash(), pindex));
        if (GetBoolArg("-printstakemodifier", false))
            LogPrintf("%s : selected round %d stop=%s height=%d bit=%d\n", __func__,
                nRound, DateTimeStrFormat("%Y-%m-%d %H:%M:%S", nSelectionIntervalStop).c_str(), pindex->nHeight, pindex->GetStakeEntropyBit());
    }

    // Print selection map for visualization of the selected blocks
    if (GetBoolArg("-printstakemodifier", false)) {
        std::string strSelectionMap = "";
        // '-' indicates proof-of-work blocks not selected
        strSelectionMap.insert(0, pindexPrev->nHeight - nHeightFirstCandidate + 1, '-');
        pindex = pindexPrev;
        while (pindex && pindex->nHeight >= nHeightFirstCandidate) {
            // '=' indicates proof-of-stake blocks not selected
            if (pindex->IsProofOfStake())
                strSelectionMap.replace(pindex->nHeight - nHeightFirstCandidate, 1, "=");
            pindex = pindex->pprev;
        }
        for (const std::pair<const uint256, const CBlockIndex*> &item : mapSelectedBlocks) {
            // 'S' indicates selected proof-of-stake blocks
            // 'W' indicates selected proof-of-work blocks
            strSelectionMap.replace(item.second->nHeight - nHeightFirstCandidate, 1, item.second->IsProofOfStake() ? "S" : "W");
        }
        LogPrintf("%s : selection height [%d, %d] map %s\n", __func__, nHeightFirstCandidate, pindexPrev->nHeight, strSelectionMap.c_str());
    }
    if (GetBoolArg("-printstakemodifier", false)) {
        LogPrintf("%s : new modifier=%s time=%s\n", __func__, std::to_string(nStakeModifierNew).c_str(), DateTimeStrFormat("%Y-%m-%d %H:%M:%S", pindexPrev->GetBlockTime()).c_str());
    }

    nStakeModifier = nStakeModifierNew;
    fGeneratedStakeModifier = true;
    return true;
}
