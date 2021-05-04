/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2019-2021,  Universidad Autonoma de San Luis Potosi,
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

#include "midst-lsa.hpp"
#include "tlv-nlsr.hpp"

namespace nlsr {

MidstLsa::MidstLsa(const ndn::Name& originRouter, uint64_t seqNo,
                   const ndn::time::system_clock::TimePoint& timepoint,
                   const MidstPrefixList& mpl)
  : Lsa(originRouter, seqNo, timepoint)
{
  for (const auto& name : mpl.getNames()) {
    double  distance = mpl.getDistance(name);
    ndn::Name anchor = mpl.getAnchor(name);
    uint64_t     rsn = mpl.getSeqNo(name);
    addName(name, distance, anchor, rsn);
  }
}

MidstLsa::MidstLsa(const ndn::Block& block)
{
  wireDecode(block);
}

NDN_CXX_DEFINE_WIRE_ENCODE_INSTANTIATIONS(MidstLsa);

template<ndn::encoding::Tag TAG>
size_t
MidstLsa::wireEncode(ndn::EncodingImpl<TAG>& block) const
{
  size_t totalLength = 0;

  // Is this a partial encode or the final encode.
  if (tempDistance >= 0) {
    // Encode the MidstLsa info.
    m_mpl.setExtraDistance(tempDistance);
    totalLength += m_mpl.wireEncode(block);
  }
  else {
    // Encodes the Lsa in front of the MidstLsa info.
    totalLength += Lsa::wireEncode(block);

    totalLength += block.prependVarNumber(totalLength);
    totalLength += block.prependVarNumber(ndn::tlv::nlsr::MidstLsa);
  }

  return totalLength;
}

const ndn::Block&
MidstLsa::wireEncode() const
{
  //if (m_wire.hasWire()) {
  //  return m_wire;
  //}

  ndn::EncodingEstimator estimator;
  size_t estimatedSize = wireEncode(estimator);

  ndn::EncodingBuffer buffer(estimatedSize, 0);
  wireEncode(buffer);

  m_wire = buffer.block();

  return m_wire;
}

void
MidstLsa::wireDecode(const ndn::Block& wire)
{
  m_wire = wire;
  
  m_wire.parse();
  auto val = m_wire.elements_begin();

  if (val->type() != ndn::tlv::nlsr::MidstLsa) {
    NDN_THROW(Error("ndn::tlv::nlsr::MidstLsa", val->type()));
  }

  ndn::Block dataBlock = *val;
  dataBlock.parse();
  auto val2 = dataBlock.elements_begin();

  if (val2 != dataBlock.elements_end()) { 
    if (val2->type() == ndn::tlv::nlsr::Lsa) {
      Lsa::wireDecode(*val2);
      ++val2;
    }
    else {
      NDN_THROW(Error("ndn::tlv::nlsr::Lsa", m_wire.type()));
    }
  }
  else {
    BOOST_THROW_EXCEPTION(Error("Missing required ndn::tlv::nlsr::Lsa field"));
  }

  ++val;
  while (val != m_wire.elements_end()) {
    m_mpl.wireDecode(*val);
    ++val;
  }
}

void
MidstLsa::setTempDistance(double distance)
{
  tempDistance = distance;
}

bool
MidstLsa::isEqualContent(const MidstLsa& other) const
{
  return m_mpl == other.getNpl();
}

std::string
MidstLsa::toString() const
{
  std::ostringstream os;
  os << getString();
  os << "      MIDST Names:\n";
  int i = 0;
  for (const auto& name : m_mpl.getNames()) {
    os << "        Name " << i++ << ": " << name << "\n";
    os << "          Distance: "  << m_mpl.getDistance(name) << "\n";
    os << "          Anchor: "    << m_mpl.getAnchor(name) << "\n";
    os << "          Seq. Num.: " << m_mpl.getSeqNo(name) << "\n";
  }

  return os.str();
}

std::tuple<bool, std::list<ndn::Name>, std::list<ndn::Name>>
MidstLsa::update(const std::shared_ptr<Lsa>& lsa)
{
  auto mlsa = std::static_pointer_cast<MidstLsa>(lsa);
  bool updated = false;

  // Obtain the set difference of the current and the incoming
  // name prefix sets, and add those.
  std::list<ndn::Name> newNames = mlsa->getNpl().getNames();
  std::list<ndn::Name> oldNames = m_mpl.getNames();
  std::list<ndn::Name> namesToAdd;
  std::set_difference(newNames.begin(), newNames.end(), oldNames.begin(), oldNames.end(),
                      std::inserter(namesToAdd, namesToAdd.begin()));
  for (const auto& name : namesToAdd) {
    double  distance = mlsa->getNpl().getDistance(name);
    ndn::Name anchor = mlsa->getNpl().getAnchor(name);
    uint64_t     rsn = mlsa->getNpl().getSeqNo(name);
    
    addName(name, distance, anchor, rsn);
    updated = true;
  }

  m_mpl.sort();

  // Also remove any names that are no longer being advertised.
  std::list<ndn::Name> namesToRemove;
  std::set_difference(oldNames.begin(), oldNames.end(), newNames.begin(), newNames.end(),
                      std::inserter(namesToRemove, namesToRemove.begin()));
  for (const auto& name : namesToRemove) {
    removeName(name);
    updated = true;
  }

  return std::make_tuple(updated, namesToAdd, namesToRemove);
}

std::ostream&
operator<<(std::ostream& os, const MidstLsa& lsa)
{
  return os << lsa.toString();
}

} // namespace nlsr
