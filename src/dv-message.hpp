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

#ifndef NLSR_DV_MESSAGE_HPP
#define NLSR_DV_MESSAGE_HPP

#include "statistics.hpp"
#include "conf-parameter.hpp"
#include "lsdb.hpp"

namespace nlsr {

class DvMessage
{
public:
  DvMessage(ndn::Face& face, ndn::KeyChain& keyChain,
            ConfParameter& confParam, Lsdb& lsdb);

  ndn::Name
  buildMidstInterestPrefix(ndn::Name neighbor) const;

  ndn::Name
  buildAdjInterestPrefix(ndn::Name neighbor) const;

  void
  expressInterest(const ndn::Name& neighbor, uint32_t seconds);

  void
  processInterest(const ndn::Name& name, const ndn::Interest& interest);

  void
  onContent(const ndn::Interest& interest, const ndn::Data& data);

  ndn::util::signal::Signal<DvMessage, Statistics::PacketType> dvMsgIncrementSignal;

private:
  void
  processInterestTimedOut(const ndn::Interest& interest);

  void
  onContentValidated(const ndn::Data& data);

  void
  onContentValidationFailed(const ndn::Data& data,
                            const ndn::security::v2::ValidationError& ve);

  void
  findAndUpdateTableForActiveNeighbors(const ndn::Name& origNeighbor);

  void
  expressInterestActiveNeighbor(const ndn::Name& neighbor, uint32_t seconds);

  void
  onContentActiveNeighbor(const ndn::Interest& interest, const ndn::Data& data);

  void
  insertNeighborsVector(const ndn::Name router, uint64_t seqNo);

  void
  increaseNeighborsVector(const ndn::Name router);

  bool
  isThisAnUpdateTableMessage(const ndn::Name& neighbor, uint64_t seqNo) const;

  ndn::Face& m_face;
  ndn::security::KeyChain& m_keyChain;
  const ndn::security::SigningInfo& m_signingInfo;
  ConfParameter& m_confParam;
  Lsdb& m_lsdb;

  static const std::string NLSR_COMPONENT;
  static const std::string DIST_VECTOR_COMPONENT;

  enum ProcTupleIndex {
    NEIGHBOR,
    SEQNUMBER
  };
  
  using ProcTuple = std::tuple<ndn::Name, uint64_t>;
  std::vector<ProcTuple> processed_neighbors_vctr;
};

} // namespace nlsr

#endif // NLSR_DV_MESSAGE_HPP
