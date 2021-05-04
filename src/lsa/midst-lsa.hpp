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

#ifndef NLSR_LSA_MIDST_LSA_HPP
#define NLSR_LSA_MIDST_LSA_HPP

#include "lsa.hpp"

namespace nlsr {

/*!
   \brief Data abstraction for MidstLsa
   MidstLsa := MIDST-LSA-TYPE TLV-LENGTH
                Lsa
                Name+
 */
class MidstLsa : public Lsa
{
public:
  MidstLsa() = default;

  MidstLsa(const ndn::Name& originRouter, uint64_t seqNo,
           const ndn::time::system_clock::TimePoint& timepoint,
           const MidstPrefixList& mpl);

  MidstLsa(const ndn::Block& block);

  Lsa::Type
  getType() const override
  {
    return type();
  }

  static constexpr Lsa::Type
  type()
  {
    return Lsa::Type::MIDST;
  }

  MidstPrefixList&
  getNpl()
  {
    return m_mpl;
  }

  const MidstPrefixList&
  getNpl() const
  {
    return m_mpl;
  }

  void
  addName(const ndn::Name& name, double distance, ndn::Name& anchor,
          uint32_t seqNo)
  {
    m_wire.reset();
    m_mpl.insert(name, distance, anchor, seqNo);
  }

  void
  removeName(const ndn::Name& name)
  {
    m_wire.reset();
    m_mpl.remove(name);
  }

  bool
  isEqualContent(const MidstLsa& other) const;

  template<ndn::encoding::Tag TAG>
  size_t
  wireEncode(ndn::EncodingImpl<TAG>& block) const;

  const ndn::Block&
  wireEncode() const override;

  void
  wireDecode(const ndn::Block& wire);

  void
  setTempDistance(double distance);

  std::string
  toString() const override;

  std::tuple<bool, std::list<ndn::Name>, std::list<ndn::Name>>
  update(const std::shared_ptr<Lsa>& lsa) override;

  size_t
  mplSize();

private:
  MidstPrefixList m_mpl;
  
  double tempDistance;
};

NDN_CXX_DECLARE_WIRE_ENCODE_INSTANTIATIONS(MidstLsa);

std::ostream&
operator<<(std::ostream& os, const MidstLsa& lsa);

} // namespace nlsr

#endif // NLSR_LSA_MIDST_LSA_HPP
