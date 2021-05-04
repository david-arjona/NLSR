/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2020,  The University of Memphis,
 *                           Regents of the University of California,
 *                           Arizona Board of Regents.
 *
 * This file is part of NLSR (Named-data Link State Routing).
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
 **/

#include "sequencing-manager.hpp"
#include "logger.hpp"

#include <string>
#include <fstream>
#include <pwd.h>
#include <cstdlib>
#include <unistd.h>

namespace nlsr {

INIT_LOGGER(SequencingManager);

SequencingManager::SequencingManager(const std::string& filePath, int hypState,
                                     int mState)
  : m_hyperbolicState(hypState)
  , m_midstState(mState)
{
  setSeqFileDirectory(filePath);
  initiateSeqNoFromFile();
}

void
SequencingManager::writeSeqNoToFile() const
{
  writeLog();
  std::ofstream outputFile(m_seqFileNameWithPath.c_str());
  std::ostringstream os;
  os << "NameLsaSeq " << std::to_string(m_nameLsaSeq) << "\n"
     << "AdjLsaSeq "  << std::to_string(m_adjLsaSeq)  << "\n"
     << "CorLsaSeq "  << std::to_string(m_corLsaSeq)  << "\n"
     << "MidstLsaSeq " << std::to_string(m_midstLsaSeq);
  outputFile << os.str();
  outputFile.close();
}

void
SequencingManager::initiateSeqNoFromFile()
{
  NLSR_LOG_DEBUG("Seq File Name: " << m_seqFileNameWithPath);
  std::ifstream inputFile(m_seqFileNameWithPath.c_str());

  std::string seqType;
  // Good checks that file is not (bad or eof or fail)
  if (inputFile.good()) {
    inputFile >> seqType >> m_nameLsaSeq;
    inputFile >> seqType >> m_adjLsaSeq;
    inputFile >> seqType >> m_corLsaSeq;
    inputFile >> seqType >> m_midstLsaSeq;

    inputFile.close();

    // Increment by 10 in case last run of NLSR was not able to write to file
    // before crashing
    // Increment the Name LSA seq. no. if MIDST is NOT enabled
    if (m_midstState == MIDST_STATE_OFF) {
      m_nameLsaSeq += 10;
    }

    // Send a warning message if hyperbolic routing and MIDST are enabled
    if ((m_hyperbolicState != HYPERBOLIC_STATE_OFF) &&
    	(m_midstState == MIDST_STATE_ON)) {
      NLSR_LOG_WARN("Hyperbolic routing and MIDST should not be enabled"
                 << " at the same time.");
    }

    // Increment the adjacency LSA seq. no. if link-state or dry HR is enabled
    if (m_hyperbolicState != HYPERBOLIC_STATE_ON) {
      if (m_corLsaSeq != 0) {
        NLSR_LOG_WARN("This router was previously configured for hyperbolic " <<
                      "routing without clearing the seq. no. file.");
        m_corLsaSeq = 0;
      }
      if (m_midstLsaSeq != 0) {
        NLSR_LOG_WARN("This router was previously configured for MIDST"
                  << " without clearing the seq. no. file.");
        m_midstLsaSeq = 0;
      }
      m_adjLsaSeq += 10;
    }

    // Similarly, increment the coordinate LSA seq. no only if link-state is disabled.
    if (m_hyperbolicState != HYPERBOLIC_STATE_OFF) {
      if (m_adjLsaSeq != 0) {
        NLSR_LOG_WARN("This router was previously configured for link-state " <<
                      "routing without clearing the seq. no. file.");
        m_adjLsaSeq = 0;
      }
      if (m_midstLsaSeq != 0) {
        NLSR_LOG_WARN("This router was previously configured for MIDST"
                  << " without clearing the seq. no. file.");
        m_midstLsaSeq = 0;
      }
      m_corLsaSeq += 10;
    }

    // Increment the MIDST LSA seq. no. if MIDST is enabled
    if (m_midstState == MIDST_STATE_ON) {
      if (m_corLsaSeq != 0) {
        NLSR_LOG_WARN("This router was previously configured for hyperbolic"
                   << " routing without clearing the seq. no. file.");
        m_corLsaSeq = 0;
      }
      if (m_nameLsaSeq != 0) {
        NLSR_LOG_WARN("This router was previously configured for link-state"
                   << " routing without clearing the name seq. no. file.");
        m_nameLsaSeq = 0;
      }
      m_midstLsaSeq += 10;
    }
  }
  writeLog();
}

void
SequencingManager::setSeqFileDirectory(const std::string& filePath)
{
  m_seqFileNameWithPath = filePath;

  if (m_seqFileNameWithPath.empty()) {
    std::string homeDirPath(getpwuid(getuid())->pw_dir);
    if (homeDirPath.empty()) {
      homeDirPath = getenv("HOME");
    }
    m_seqFileNameWithPath = homeDirPath;
  }
  m_seqFileNameWithPath = m_seqFileNameWithPath + "/nlsrSeqNo.txt";
}

void
SequencingManager::writeLog() const
{
  if (m_hyperbolicState == HYPERBOLIC_STATE_OFF ||
      m_hyperbolicState == HYPERBOLIC_STATE_DRY_RUN) {
    NLSR_LOG_DEBUG("Adj LSA seq no: " << m_adjLsaSeq);
  }
  if (m_hyperbolicState == HYPERBOLIC_STATE_ON ||
      m_hyperbolicState == HYPERBOLIC_STATE_DRY_RUN) {
    NLSR_LOG_DEBUG("Cor LSA Seq no: " << m_corLsaSeq);
  }

  if (m_midstState == MIDST_STATE_ON){
    NLSR_LOG_DEBUG("MIDST LSA Seq no: " << m_midstLsaSeq);
  }
  else{
    NLSR_LOG_DEBUG("Name LSA Seq no: " << m_nameLsaSeq);
  }
}

} // namespace nlsr
