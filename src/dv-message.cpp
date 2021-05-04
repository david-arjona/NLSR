/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2020-2021,  Universidad Autonoma de San Luis Potosi,
 *                           Facultad de Ingeniería.
 *
 * This file is part of a modified version of NLSR (Named-data Link State Routing).
 * See AUTHORS.md for complete list of NLSR authors and contributors.
 *
 * NLSR is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NLSR is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NLSR, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "dv-message.hpp"
#include "logger.hpp"
#include "utility/name-helper.hpp"

#include <boost/lexical_cast.hpp>

namespace nlsr {

const std::string DvMessage::NLSR_COMPONENT = "nlsr";
const std::string DvMessage::DIST_VECTOR_COMPONENT = "DV";

INIT_LOGGER(DvMessage);

DvMessage::DvMessage(ndn::Face& face, ndn::KeyChain& keyChain,
                     ConfParameter& confParam, Lsdb& lsdb)
  : m_face(face)
  , m_keyChain(keyChain)
  , m_signingInfo(confParam.getSigningInfo())
  , m_confParam(confParam)
  , m_lsdb(lsdb)
{
  ndn::Name name(m_confParam.getRouterPrefix());
  name.append(NLSR_COMPONENT);
  name.append(DIST_VECTOR_COMPONENT);

  NLSR_LOG_DEBUG("Setting interest filter for distance-vector: " << name);

  m_face.setInterestFilter(ndn::InterestFilter(name).allowLoopback(false),
    [this] (const auto& name, const auto& interest) {
      processInterest(name, interest);
    },
    [] (const auto& name) {
      NLSR_LOG_DEBUG("Successfully registered prefix: " << name);
    },
    [] (const auto& name, const auto& resp) {
      NLSR_LOG_ERROR("Failed to register prefix " << name);
      NDN_THROW(std::runtime_error("Failed to register DV prefix: " + resp));
    },
    m_signingInfo, ndn::nfd::ROUTE_FLAG_CAPTURE);

  if (m_confParam.getMidstState() != MIDST_STATE_OFF) {
    m_lsdb.buildAndInstallOwnMidstLsa();
  }
}

ndn::Name
DvMessage::buildMidstInterestPrefix(ndn::Name neighbor) const
{
  ndn::Name midstInterest = neighbor;
  midstInterest.append(NLSR_COMPONENT);
  midstInterest.append(DIST_VECTOR_COMPONENT);
  midstInterest.appendNumber(m_lsdb.getMidstLsaSeqNo());
  // The info from this router is added here
  midstInterest.append(m_confParam.getRouterPrefix().wireEncode());
  NLSR_LOG_DEBUG("Building midstInterest: " << midstInterest);
  NLSR_LOG_DEBUG("With seq. number = " << m_lsdb.getMidstLsaSeqNo());

  return midstInterest;
}

ndn::Name
DvMessage::buildAdjInterestPrefix(ndn::Name neighbor) const
{
  ndn::Name adjInterest = m_confParam.getLsaPrefix();
  adjInterest.append(neighbor.getSubName(neighbor.size() - 4,
                                         neighbor.size()));
  adjInterest.append(boost::lexical_cast<std::string>(Lsa::Type::ADJACENCY));
  adjInterest.appendNumber(m_lsdb.getAdjLsaSeqNo());
  NLSR_LOG_DEBUG("Building adjInterest: " << adjInterest);

  return adjInterest;
}

void
DvMessage::expressInterest(const ndn::Name& neighbor, uint32_t seconds)
{
  ndn::Name interestName = buildMidstInterestPrefix(neighbor);
  NLSR_LOG_DEBUG("Expressing DV Interest :" << interestName);

  ndn::Interest interest(interestName);
  interest.setInterestLifetime(ndn::time::seconds(seconds));
  interest.setMustBeFresh(true);
  interest.setCanBePrefix(true);
  m_face.expressInterest(interest,
                   std::bind(&DvMessage::onContent, this, _1, _2),
                   [this] (const ndn::Interest& interest, const ndn::lp::Nack& nack)
                   {
                     NDN_LOG_TRACE("Received Nack with reason " << nack.getReason());
                     NDN_LOG_TRACE("Treating as timeout");
                     processInterestTimedOut(interest);
                   },
                   std::bind(&DvMessage::processInterestTimedOut, this, _1));

  dvMsgIncrementSignal(Statistics::PacketType::SENT_MIDST_DV_INTEREST);
}

void
DvMessage::processInterest(const ndn::Name& name, const ndn::Interest& interest)
{
  dvMsgIncrementSignal(Statistics::PacketType::RCV_MIDST_DV_INTEREST);
  NLSR_LOG_DEBUG("Received DV interest: " << interest);

  // interest name: /<ownRouter>/nlsr/DV/<neighbor>
  ndn::Name interestName = interest.getName();

  ndn::Name neighbor;
  neighbor.wireDecode(interestName.get(-1).blockFromValue());
  uint64_t seqNo = interestName.get(-2).toNumber();

  NLSR_LOG_DEBUG("From neighbor: " << neighbor);
  NLSR_LOG_DEBUG("With Seq. number = " << seqNo);

  if (isThisAnUpdateTableMessage(neighbor, seqNo)) {
    NLSR_LOG_DEBUG("This is an update table message!!!");
    expressInterest(neighbor, m_confParam.getInterestResendTime());
  }

  interestName.appendVersion();
  interestName.appendSegment(0);
  NLSR_LOG_DEBUG("Processing distance-vector interest: " << interestName);

  if (m_confParam.getAdjacencyList().isNeighbor(neighbor)) {
    ndn::Block block = m_lsdb.wireEncode(neighbor);

    std::shared_ptr<ndn::Data> data = std::make_shared<ndn::Data>(interestName);
    data->setFreshnessPeriod(ndn::time::seconds(10)); // 10 sec
    data->setContent(block);

    m_keyChain.sign(*data, m_signingInfo);
    m_face.put(*data);

    dvMsgIncrementSignal(Statistics::PacketType::SENT_MIDST_DV_DATA);
    NLSR_LOG_DEBUG("Sending out DV data: " << data);
    increaseNeighborsVector(neighbor);
  }
}

void
DvMessage::processInterestTimedOut(const ndn::Interest& interest)
{
  // interest name: /<neighbor>/NLSR/INFO/<router>
  const ndn::Name interestName(interest.getName());
  NLSR_LOG_DEBUG("Interest timed out for DV: " << interestName);
}

void

DvMessage::onContent(const ndn::Interest& interest, const ndn::Data& data)
{
  dvMsgIncrementSignal(Statistics::PacketType::RCV_MIDST_DV_DATA);
  NLSR_LOG_DEBUG("Received DV data: " << data.getName());

  auto kl = data.getKeyLocator();
  if (kl && kl->getType() == ndn::tlv::Name) {
    NLSR_LOG_DEBUG("Data signed with: " << kl->getName());
  }

  m_confParam.getValidator().validate(data,
                               std::bind(&DvMessage::onContentValidated, this, _1),
                               std::bind(&DvMessage::onContentValidationFailed,
                                                this, _1, _2));
}

void
DvMessage::onContentValidated(const ndn::Data& data)
{
  // data name: /<neighbor>/NLSR/DV/SeqNo/<OwnRouter>/<version>/<segmentNo>
  ndn::Name dataName = data.getName();
  NLSR_LOG_DEBUG("Data validation successful for MIDST: " << dataName);

  std::string chkString(DIST_VECTOR_COMPONENT);
  int32_t lsaPosition = util::getNameComponentPosition(dataName, chkString);

  if (lsaPosition >= 0) {
    // Extracts the prefix of the originating router from the data.
    ndn::Name originRouter = dataName.getSubName(0, lsaPosition - 1);
    NLSR_LOG_DEBUG("originRouter = " << originRouter);

    if (!data.hasContent()) {
      NLSR_LOG_DEBUG("Data content block is empty.");
      return;
    }

    ndn::Block dataBlock = data.getContent();

    uint64_t n_seqNo = m_lsdb.wireDecode(dataBlock);
    NLSR_LOG_DEBUG("Seq. number from originRouter: " << n_seqNo);

    if (n_seqNo != 0) {
      insertNeighborsVector(originRouter, n_seqNo);
      findAndUpdateTableForActiveNeighbors(originRouter);
    }
  }
}

void
DvMessage::onContentValidationFailed(const ndn::Data& data,
                                 const ndn::security::ValidationError& ve)
{
  NLSR_LOG_DEBUG("Validation Error: " << ve);
}

void
DvMessage::findAndUpdateTableForActiveNeighbors(const ndn::Name& origNeighbor)
{
  auto lsaRange = m_lsdb.getLsdbIterator<AdjLsa>();
  
  for (auto lsaIt = lsaRange.first; lsaIt != lsaRange.second; lsaIt++) {
    auto lsaPtr = std::static_pointer_cast<AdjLsa>(*lsaIt);
    
    // Looking for this router's direct neighbors.
    if (lsaPtr->getOriginRouter() == m_confParam.getRouterPrefix()) {

      for (const auto& adj : lsaPtr->getAdl().getAdjList()) {
        // Only active neighbors that are not the originating neighbor.
        if ((adj.getName() != origNeighbor) &&
            (adj.getStatus() == Adjacent::STATUS_ACTIVE)) {
          NLSR_LOG_DEBUG("Active Neighbor: " << adj.getName());
          expressInterestActiveNeighbor(adj.getName(), m_confParam.getInterestResendTime());
        }
      }
    }
  }
}

void
DvMessage::expressInterestActiveNeighbor(const ndn::Name& neighbor, uint32_t seconds)
{
  ndn::Name interestName = buildMidstInterestPrefix(neighbor);
  NLSR_LOG_DEBUG("Expressing DV Update Table Interest: " << interestName);

  ndn::Interest interest(interestName);
  interest.setInterestLifetime(ndn::time::seconds(seconds));
  interest.setMustBeFresh(true);
  interest.setCanBePrefix(true);
  m_face.expressInterest(interest,
                   std::bind(&DvMessage::onContentActiveNeighbor, this, _1, _2),
                   [this] (const ndn::Interest& interest, const ndn::lp::Nack& nack)
                   {
                     NDN_LOG_TRACE("Received Nack with reason " << nack.getReason());
                     NDN_LOG_TRACE("Treating as timeout");
                     processInterestTimedOut(interest);
                   },
                   std::bind(&DvMessage::processInterestTimedOut, this, _1));

  dvMsgIncrementSignal(Statistics::PacketType::SENT_MIDST_DV_INTEREST);
}

void
DvMessage::onContentActiveNeighbor(const ndn::Interest& interest, const ndn::Data& data)
{
  NLSR_LOG_DEBUG("Received Acknowledgement: " << data.getName());
}

void
DvMessage::insertNeighborsVector(const ndn::Name router, uint64_t seqNo)
{
  for (auto& vector : processed_neighbors_vctr) {
    if (std::get<ProcTupleIndex::NEIGHBOR>(vector) == router) {
      std::get<ProcTupleIndex::SEQNUMBER>(vector) = seqNo;
      NDN_LOG_DEBUG("Updated Seq. number to " << seqNo <<
                    " for neighbor " << router);
      return;
    }
  }
   
  processed_neighbors_vctr.emplace_back(router, seqNo);
}

void
DvMessage::increaseNeighborsVector(const ndn::Name router)
{
  for (auto& vector : processed_neighbors_vctr) {
    if (std::get<ProcTupleIndex::NEIGHBOR>(vector) == router) {
      std::get<ProcTupleIndex::SEQNUMBER>(vector)++;
      NDN_LOG_DEBUG("Increased Seq. number for neighbor " << router);
      return;
    }
  }   
}

bool
DvMessage::isThisAnUpdateTableMessage(const ndn::Name& neighbor, uint64_t seqNo) const
{
  if (processed_neighbors_vctr.size() != 0) {
    for (const auto& vector : processed_neighbors_vctr) {
      if ((std::get<ProcTupleIndex::NEIGHBOR>(vector) == neighbor)
          && (std::get<ProcTupleIndex::SEQNUMBER>(vector) < seqNo)) {
        return true;
      }
    }
  }
  return false;
}

} // namespace nlsr
