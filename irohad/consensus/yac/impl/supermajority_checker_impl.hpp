/**
 * Copyright Soramitsu Co., Ltd. 2018 All Rights Reserved.
 * http://soramitsu.co.jp
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef IROHA_SUPERMAJORITY_CHECKER_IMPL_HPP
#define IROHA_SUPERMAJORITY_CHECKER_IMPL_HPP

#include "consensus/yac/supermajority_checker.hpp"

namespace iroha {
  namespace consensus {
    namespace yac {
      /**
       * Implementation of SupermajorityChecker interface.
       * Checks supermajority.
       */
      class SupermajorityCheckerImpl : public SupermajorityChecker {
       public:
        virtual ~SupermajorityCheckerImpl() = default;

        /**
         * Check if supermajority is achieved
         */
        virtual bool hasSupermajority(
            const std::vector<model::Signature> &signatures,
            const std::vector<model::Peer> &peers) const override;

        virtual bool checkSize(uint64_t current, uint64_t all) const override;

        virtual bool peersSubset(
            const std::vector<model::Signature> &signatures,
            const std::vector<model::Peer> &peers) const override;

        virtual bool hasReject(uint64_t frequent,
                               uint64_t voted,
                               uint64_t all) const override;
      };
    }  // namespace yac
  }    // namespace consensus
}  // namespace iroha

#endif  // IROHA_SUPERMAJORITY_CHECKER_IMPL_HPP