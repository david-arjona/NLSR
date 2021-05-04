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

#ifndef NLSR_MIDST_PREFIX_LIST_HPP
#define NLSR_MIDST_PREFIX_LIST_HPP

#include <list>
#include <string>
#include <boost/cstdint.hpp>
#include <ndn-cxx/name.hpp>


namespace nlsr {
class MidstPrefixList
{
public:
  using NameTuple = std::tuple<ndn::Name, double, ndn::Name, uint32_t>;
  enum MidstIndex {
    NAME,
    DISTANCE,
    ANCHOR,
    SEQNO
  };

  MidstPrefixList();

  MidstPrefixList(const std::initializer_list<ndn::Name>& names);

  MidstPrefixList(const std::initializer_list<MidstPrefixList::NameTuple>& namesAndData);

  template<class ContainerType>
  MidstPrefixList(const ContainerType& names)
  {
    for (const auto& elem : names) {
      m_names.push_back(NameTuple{elem, 0, {""}, 0});
    }
  }

  ~MidstPrefixList();

  /*! \brief inserts name into NamePrefixList
      \retval true If the name was successfully inserted.
      \retval false If the name could not be inserted.
   */
  bool
  insert(const ndn::Name& name, const double distance,
         const ndn::Name& anchor, const uint32_t seqNo);
  /*! \brief removes name from NamePrefixList
      \retval true If the name is removed
      \retval false If the name failed to be removed.
   */
  bool
  remove(const ndn::Name& name);

  void
  sort();

  size_t
  size() const
  {
    return m_names.size();
  }

  void
  setExtraDistance(double distance) const;

  template<ndn::encoding::Tag TAG>
  size_t
  wireEncode(ndn::EncodingImpl<TAG>& block) const;

  const ndn::Block&
  wireEncode() const;

  void
  wireDecode(const ndn::Block& wire);

  std::list<ndn::Name>
  getNames() const;

  bool
  operator==(const MidstPrefixList& other) const;

  double
  getDistance(const ndn::Name& name) const;

  const ndn::Name&
  getAnchor(const ndn::Name& name) const;

  ndn::Name&
  getAnchor(const ndn::Name& name);

  uint32_t
  getSeqNo(const ndn::Name& name) const;

private:
  /*! Obtain an iterator to the entry matching name.

    \note We could do this quite easily inline with a lambda, but this
    is slightly more efficient.
   */
  std::vector<NameTuple>::iterator
  get(const ndn::Name& name);

  ndn::Name INVALID_NAME1;
  const ndn::Name INVALID_NAME2;

  std::vector<NameTuple> m_names;

  mutable double extraDistance;
  mutable ndn::Block m_wire;
};

NDN_CXX_DECLARE_WIRE_ENCODE_INSTANTIATIONS(MidstPrefixList);

std::ostream&
operator<<(std::ostream& os, const MidstPrefixList& list);

} // namespace nlsr

#endif // NLSR_MIDST_PREFIX_LIST_HPP

