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

#include "midst-prefix-list.hpp"
#include "common.hpp"
#include "tlv-nlsr.hpp"


namespace nlsr {

MidstPrefixList::MidstPrefixList() = default;

MidstPrefixList::MidstPrefixList(const std::initializer_list<ndn::Name>& names)
{
  std::vector<MidstPrefixList::NameTuple> nameTuples;
  std::transform(names.begin(), names.end(), std::back_inserter(nameTuples),
    [] (const ndn::Name& name) {
      return MidstPrefixList::NameTuple{name, 0, {""}, 0};
    });
  m_names = std::move(nameTuples);
}

MidstPrefixList::MidstPrefixList(const std::initializer_list<MidstPrefixList::NameTuple>& namesAndData)
  : m_names(namesAndData)
{
}

MidstPrefixList::~MidstPrefixList()
{
}

std::vector<MidstPrefixList::NameTuple>::iterator
MidstPrefixList::get(const ndn::Name& name)
{
  return std::find_if(m_names.begin(), m_names.end(),
                      [&] (const MidstPrefixList::NameTuple& tuple) {
                        return name == std::get<MidstPrefixList::MidstIndex::NAME>(tuple);
                      });
}

bool
MidstPrefixList::insert(const ndn::Name& name, const double distance,
                       const ndn::Name& anchor, const uint32_t seqNo)
{
  auto pairItr = get(name);

  // If the name is not found, add it at the end of the vector.
  if (pairItr == m_names.end()) {
    m_names.push_back(std::tie(name, distance, anchor, seqNo));
    return true;
  }
  else {
  // If the name is found, *** GENERATE LOG *** and replace it.
    *pairItr = std::tie(name, distance, anchor, seqNo);
    return true;
  }
  return false;
}

bool
MidstPrefixList::remove(const ndn::Name& name)
{
  auto pairItr = get(name);
  if (pairItr != m_names.end()) {
    m_names.erase(pairItr);
    return true;
  }
  return false;
}

bool
MidstPrefixList::operator==(const MidstPrefixList& other) const
{
  return m_names == other.m_names;
}

void
MidstPrefixList::sort()
{
  std::sort(m_names.begin(), m_names.end());
}

void
MidstPrefixList::setExtraDistance(double distance) const
{
  extraDistance = distance;
}

NDN_CXX_DEFINE_WIRE_ENCODE_INSTANTIATIONS(MidstPrefixList);

template<ndn::encoding::Tag TAG>
size_t
MidstPrefixList::wireEncode(ndn::EncodingImpl<TAG>& block) const
{
  double m_distance  = 0;
  size_t totalLength = 0;

  for (const auto& a_name : m_names) {
    totalLength += prependDoubleBlock(block, ndn::tlv::nlsr::SeqNo, 
                   std::get<MidstPrefixList::MidstIndex::SEQNO>(a_name));

    totalLength += std::get<MidstPrefixList::MidstIndex::ANCHOR>(a_name).
                   wireEncode(block);

    m_distance   = std::get<MidstPrefixList::MidstIndex::DISTANCE>(a_name) +
                   extraDistance;
    totalLength += prependDoubleBlock(block, ndn::tlv::nlsr::Distance, 
                   m_distance);

    totalLength += std::get<MidstPrefixList::MidstIndex::NAME>(a_name).
                   wireEncode(block);
  }

  totalLength += block.prependVarNumber(totalLength);
  totalLength += block.prependVarNumber(ndn::tlv::nlsr::MidstPrefixList);

  return totalLength;
}

const ndn::Block&
MidstPrefixList::wireEncode() const
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
MidstPrefixList::wireDecode(const ndn::Block& wire)
{
  ndn::Name  name;
  ndn::Name  anchor;
  double     distance = 0;
  uint32_t   seqNo = 0;
  ndn::Block dataBlock = wire;
  
  dataBlock.parse();
  auto val2 = dataBlock.elements_begin();

  while (val2 != dataBlock.elements_end()) {
    if (val2 != m_wire.elements_end() && val2->type() == ndn::tlv::Name) {
      name.wireDecode(*val2);
      ++val2;
    }
    else {
      NDN_THROW(ndn::tlv::Error("Missing required Name field"));
    }

    if (val2 != m_wire.elements_end() && val2->type() == ndn::tlv::nlsr::Distance) {
      distance = ndn::encoding::readDouble(*val2);
      ++val2;
    }
    else {
      NDN_THROW(ndn::tlv::Error("Missing required Distance field"));
   }

    if (val2 != m_wire.elements_end() && val2->type() == ndn::tlv::Name) {
      anchor.wireDecode(*val2);
      ++val2;
    }
    else {
      NDN_THROW(ndn::tlv::Error("Missing required Anchor field"));
    }

    if (val2 != m_wire.elements_end() && val2->type() == ndn::tlv::nlsr::SeqNo) {
      seqNo = ndn::encoding::readDouble(*val2);
      ++val2;
    }
    else {
      NDN_THROW(ndn::tlv::Error("Missing required SeqNo field"));
    }

    insert(name, distance, anchor, seqNo);
  }
}

std::list<ndn::Name>
MidstPrefixList::getNames() const
{
  std::list<ndn::Name> names;
  for (const auto& a_name : m_names) {
    names.push_back(std::get<MidstPrefixList::MidstIndex::NAME>(a_name));
  }
  return names;
}

double
MidstPrefixList::getDistance(const ndn::Name& name) const
{
  auto Itr = std::find_if(m_names.begin(), m_names.end(),
                         [&] (const MidstPrefixList::NameTuple& tuple) {
                           return name == std::get<MidstPrefixList::MidstIndex::NAME>(tuple);
                         });
  if (Itr != m_names.end()) {
    return std::get<MidstPrefixList::MidstIndex::DISTANCE>(*Itr);
  }
  return -1; // I would prefer an INVALID value
}

const ndn::Name&
MidstPrefixList::getAnchor(const ndn::Name& name) const
{
  auto Itr = std::find_if(m_names.begin(), m_names.end(),
                         [&] (const MidstPrefixList::NameTuple& tuple) {
                           return name == std::get<MidstPrefixList::MidstIndex::NAME>(tuple);
                         });
  if (Itr != m_names.end()) {
    return std::get<MidstPrefixList::MidstIndex::ANCHOR>(*Itr);
  }
  return INVALID_NAME2;
}

ndn::Name&
MidstPrefixList::getAnchor(const ndn::Name& name)
{
  auto Itr = std::find_if(m_names.begin(), m_names.end(),
                         [&] (const MidstPrefixList::NameTuple& tuple) {
                           return name == std::get<MidstPrefixList::MidstIndex::NAME>(tuple);
                         });
  if (Itr != m_names.end()) {
    return std::get<MidstPrefixList::MidstIndex::ANCHOR>(*Itr);
  }
  return INVALID_NAME1;
}

uint32_t
MidstPrefixList::getSeqNo(const ndn::Name& name) const
{
  auto Itr = std::find_if(m_names.begin(), m_names.end(),
                         [&] (const MidstPrefixList::NameTuple& tuple) {
                           return name == std::get<MidstPrefixList::MidstIndex::NAME>(tuple);
                         });
  if (Itr != m_names.end()) {
    return std::get<MidstPrefixList::MidstIndex::SEQNO>(*Itr);
  }
  return 500; // I would prefer an INVALID value
}

std::ostream&
operator<<(std::ostream& os, const MidstPrefixList& list) {
  os << "MIDST prefix list: {\n";
  for (const auto& name : list.getNames()) {
    os << name << "\n"
       << "Distance: " << list.getDistance(name) << "\n"
       << "Anchor: " << list.getAnchor(name) <<  "\n"
       << "Sequence Number: " << list.getSeqNo(name) <<  "\n";
  }
  os << "}" << std::endl;
  return os;
}

} // namespace nlsr

