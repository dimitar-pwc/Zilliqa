/*
 * Copyright (C) 2019 Zilliqa
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef ZILLIQA_SRC_LIBVALIDATOR_VALIDATOR_H_
#define ZILLIQA_SRC_LIBVALIDATOR_VALIDATOR_H_

#include <boost/variant.hpp>
#include <memory>
#include <vector>
#include <string>
#include "libData/AccountData/Transaction.h"
#include "libData/AccountData/TransactionReceipt.h"
#include "libData/BlockChainData/BlockLinkChain.h"
#include "libData/BlockData/Block.h"
#include "libNetwork/Peer.h"

#include "libData/AccountData/Account.h"
#include "libMessage/Messenger.h"
#include "libUtils/BitVector.h"

class Mediator;

class Validator {
 public:
  enum TxBlockValidationMsg { VALID = 0, STALEDSINFO, INVALID };
  Validator(Mediator& mediator);
  ~Validator();
  std::string name() const { return "Validator"; }

  static bool VerifyTransaction(const Transaction& tran);

  bool CheckCreatedTransaction(const Transaction& tx,
                               TransactionReceipt& receipt,
                               TxnStatus& error_code) const;

  bool CheckCreatedTransactionFromLookup(const Transaction& tx,
                                         TxnStatus& error_code);

  template <class Container, class DirectoryBlock>
  inline bool CheckBlockCosignature(const DirectoryBlock& block,
		                    const Container& commKeys,
                                    const bool showLogs = true);

  bool CheckDirBlocks(
      const std::vector<boost::variant<DSBlock, VCBlock>>& dirBlocks,
      const DequeOfNode& initDsComm, const uint64_t& index_num,
      DequeOfNode& newDSComm);

  bool CheckDirBlocksNoUpdate(
      const std::vector<boost::variant<DSBlock, VCBlock>>& dirBlocks,
      const DequeOfNode& initDsComm, const uint64_t& index_num,
      DequeOfNode& newDSComm);

  // TxBlocks must be in increasing order or it will fail
  TxBlockValidationMsg CheckTxBlocks(const std::vector<TxBlock>& txBlocks,
                                     const DequeOfNode& dsComm,
                                     const BlockLink& latestBlockLink);
  Mediator& m_mediator;
};

template <class Container, class DirectoryBlock>
inline bool Validator::CheckBlockCosignature(const DirectoryBlock& block,
                                      const Container& commKeys,
                                      const bool showLogs) {
  if (showLogs) {
    LOG_MARKER();
  }

  unsigned int index = 0;
  unsigned int count = 0;

  const std::vector<bool>& B2 = block.GetB2();
  if (commKeys.size() != B2.size()) {
    LOG_GENERAL(WARNING, "Mismatch: committee size = "
                             << commKeys.size()
                             << ", co-sig bitmap size = " << B2.size());
    return false;
  }

  // Generate the aggregated key
  std::vector<PubKey> keys;
  for (auto const& kv : commKeys) {
    if (B2.at(index)) {
      keys.emplace_back(std::get<PubKey>(kv));
      count++;
    }
    index++;
  }

  if (count != ConsensusCommon::NumForConsensus(B2.size())) {
    LOG_GENERAL(WARNING, "Cosig was not generated by enough nodes");
    return false;
  }

  std::shared_ptr<PubKey> aggregatedKey = MultiSig::AggregatePubKeys(keys);
  if (aggregatedKey == nullptr) {
    LOG_GENERAL(WARNING, "Aggregated key generation failed");
    return false;
  }

  // Verify the collective signature
  bytes serializedHeader;
  block.GetHeader().Serialize(serializedHeader, 0);
  block.GetCS1().Serialize(serializedHeader, serializedHeader.size());
  BitVector::SetBitVector(serializedHeader, serializedHeader.size(),
                          block.GetB1());
  if (!MultiSig::MultiSigVerify(serializedHeader, 0, serializedHeader.size(),
                                block.GetCS2(), *aggregatedKey)) {
    LOG_GENERAL(WARNING, "Cosig verification failed");
    for (auto& kv : keys) {
      LOG_GENERAL(WARNING, kv);
    }
    return false;
  }

  return true;
}

#endif  // ZILLIQA_SRC_LIBVALIDATOR_VALIDATOR_H_
