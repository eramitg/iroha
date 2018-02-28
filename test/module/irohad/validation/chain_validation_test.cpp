/**
 * Copyright Soramitsu Co., Ltd. 2017 All Rights Reserved.
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

#include "builders/protobuf/block.hpp"
#include "module/irohad/ametsuchi/ametsuchi_mocks.hpp"
#include "module/irohad/consensus/yac/yac_mocks.hpp"
#include "module/irohad/model/model_mocks.hpp"
#include "validation/impl/chain_validator_impl.hpp"

// TODO: 14-02-2018 Alexey Chernyshov remove after relocation to shared_model
#include "backend/protobuf/from_old_model.hpp"

using namespace iroha;
using namespace iroha::model;
using namespace iroha::validation;
using namespace iroha::ametsuchi;

using ::testing::A;
using ::testing::ByRef;
using ::testing::InvokeArgument;
using ::testing::Return;
using ::testing::_;

class ChainValidationTest : public ::testing::Test {
 public:
  void SetUp() override {
    supermajority_checker =
        std::make_shared<iroha::consensus::yac::MockSupermajorityChecker>();
    validator = std::make_shared<ChainValidatorImpl>(supermajority_checker);
    storage = std::make_shared<MockMutableStorage>();
    query = std::make_shared<MockWsvQuery>();
    peers = std::vector<Peer>{peer};
  }

  /**
   * Get block builder to build blocks for tests
   * @return block builder
   */
  auto getBlockBuilder() const {
    constexpr auto kTotal = (1 << 5) - 1;
    return shared_model::proto::TemplateBlockBuilder<
               kTotal,
               shared_model::validation::DefaultBlockValidator,
               shared_model::proto::Block>()
        .transactions(std::vector<shared_model::proto::Transaction>{})
        .txNumber(0)
        .height(1)
        .prevHash(hash)
        .createdTime(iroha::time::now());
  }

  //  // TODO: 14-02-2018 Alexey Chernyshov make sure peer has valid key after
  //  // replacement with shared_model
  //  https://soramitsu.atlassian.net/browse/IR-903
  Peer peer;
  std::vector<Peer> peers;

  shared_model::crypto::Hash hash = shared_model::crypto::Hash("valid hash");

  std::shared_ptr<iroha::consensus::yac::MockSupermajorityChecker>
      supermajority_checker;
  std::shared_ptr<ChainValidatorImpl> validator;
  std::shared_ptr<MockMutableStorage> storage;
  std::shared_ptr<MockWsvQuery> query;
};

/**
 * @given valid block signed by peers
 * @when apply block
 * @then block is validated
 */
TEST_F(ChainValidationTest, ValidCase) {
  // Valid previous hash, has supermajority, correct peers subset => valid
  auto block = getBlockBuilder().build();

  EXPECT_CALL(*supermajority_checker,
              hasSupermajority(testing::Ref(block.signatures()), _))
      .WillOnce(Return(true));

  EXPECT_CALL(*query, getPeers()).WillOnce(Return(peers));

  EXPECT_CALL(*storage, apply(testing::Ref(block), _))
      .WillOnce(InvokeArgument<1>(ByRef(block), ByRef(*query), ByRef(hash)));

  ASSERT_TRUE(validator->validateBlock(block, *storage));
}

/**
 * @given block with wrong hash signed by peers
 * @when apply block
 * @then block is not validated
 */
TEST_F(ChainValidationTest, FailWhenDifferentPrevHash) {
  // Invalid previous hash, has supermajority, correct peers subset => invalid
  auto block = getBlockBuilder().build();

  shared_model::crypto::Hash another_hash =
      shared_model::crypto::Hash(std::string(32, '1'));

  EXPECT_CALL(*query, getPeers()).WillOnce(Return(peers));

  EXPECT_CALL(*storage, apply(testing::Ref(block), _))
      .WillOnce(
          InvokeArgument<1>(ByRef(block), ByRef(*query), ByRef(another_hash)));

  ASSERT_FALSE(validator->validateBlock(block, *storage));
}

/**
 * @given valid block signed by one peer of two peers (no supermajority)
 * @when apply block
 * @then block is not validated
 */
TEST_F(ChainValidationTest, FailWhenNoSupermajority) {
  // Valid previous hash, no supermajority, correct peers subset => invalid
  auto block = getBlockBuilder().build();

  EXPECT_CALL(*supermajority_checker,
              hasSupermajority(testing::Ref(block.signatures()), _))
      .WillOnce(Return(false));

  peers.push_back(Peer());
  EXPECT_CALL(*query, getPeers()).WillOnce(Return(peers));

  EXPECT_CALL(*storage, apply(testing::Ref(block), _))
      .WillOnce(InvokeArgument<1>(ByRef(block), ByRef(*query), ByRef(hash)));

  ASSERT_FALSE(validator->validateBlock(block, *storage));
}

/**
 * @given block with wrong hash signed by wrong peer key
 * @when apply block
 * @then block is not validated
 */
TEST_F(ChainValidationTest, FailWhenBadPeer) {
  // Valid previous hash, has supermajority, incorrect peers subset => invalid
  auto block = getBlockBuilder().build();

  EXPECT_CALL(*supermajority_checker,
              hasSupermajority(testing::Ref(block.signatures()), _))
      .WillOnce(Return(false));

  EXPECT_CALL(*query, getPeers()).WillOnce(Return(peers));

  EXPECT_CALL(*storage, apply(testing::Ref(block), _))
      .WillOnce(InvokeArgument<1>(ByRef(block), ByRef(*query), ByRef(hash)));

  ASSERT_FALSE(validator->validateBlock(block, *storage));
}

/**
 * @given valid block signed by peer
 * @when apply block
 * @then block is validated via observer
 */
TEST_F(ChainValidationTest, ValidWhenValidateChainFromOnePeer) {
  // TODO: 14-02-2018 Alexey Chernyshov remove hash after
  // replacement with shared_model https://soramitsu.atlassian.net/browse/IR-903
  // now it is needed because makeOldModel expects length of 32
  auto hash = shared_model::crypto::Hash(std::string(32, '0'));

  // Valid previous hash, has supermajority, correct peers subset => valid
  auto block = getBlockBuilder().prevHash(hash).build();

  EXPECT_CALL(*supermajority_checker,
              hasSupermajority(testing::Ref(block.signatures()), _))
      .WillOnce(Return(true));

  EXPECT_CALL(*query, getPeers()).WillOnce(Return(peers));

  // TODO: 14-02-2018 Alexey Chernyshov add argument to EXPECT_CALL after
  // replacement with shared_model https://soramitsu.atlassian.net/browse/IR-903
  std::unique_ptr<iroha::model::Block> old_block(block.makeOldModel());
  auto block_observable = rxcpp::observable<>::just(*old_block);

  EXPECT_CALL(*storage, apply(/* TODO block */ _, _))
      .WillOnce(InvokeArgument<1>(ByRef(block), ByRef(*query), ByRef(hash)));

  ASSERT_TRUE(validator->validateChain(block_observable, *storage));
}
