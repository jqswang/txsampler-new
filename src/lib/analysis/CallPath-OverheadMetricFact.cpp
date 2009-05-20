// -*-Mode: C++;-*-
// $Id: CallPath.cpp 2183 2009-05-20 02:44:38Z tallent $

// * BeginRiceCopyright *****************************************************
// 
// Copyright ((c)) 2002-2007, Rice University 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// 
// * Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
// 
// * Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the distribution.
// 
// * Neither the name of Rice University (RICE) nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
// 
// This software is provided by RICE and contributors "as is" and any
// express or implied warranties, including, but not limited to, the
// implied warranties of merchantability and fitness for a particular
// purpose are disclaimed. In no event shall RICE or contributors be
// liable for any direct, indirect, incidental, special, exemplary, or
// consequential damages (including, but not limited to, procurement of
// substitute goods or services; loss of use, data, or profits; or
// business interruption) however caused and on any theory of liability,
// whether in contract, strict liability, or tort (including negligence
// or otherwise) arising in any way out of the use of this software, even
// if advised of the possibility of such damage. 
// 
// ******************************************************* EndRiceCopyright *

//***************************************************************************
//
// File:
//   $Source$
//
// Purpose:
//   [The purpose of this file]
//
// Description:
//   [The set of functions, macros, etc. defined in the file]
//
//***************************************************************************

//************************* System Include Files ****************************

#include <iostream>

#include <string>
using std::string;

#include <typeinfo>


//*************************** User Include Files ****************************

#include "CallPath-OverheadMetricFact.hpp"

#include <lib/support/diagnostics.h>
#include <lib/support/Logic.hpp>

//*************************** Forward Declarations ***************************


//****************************************************************************
// 
//****************************************************************************

namespace Analysis {

namespace CallPath {

//***************************************************************************

const string OverheadMetricFact::s_tag = "lush:parallel-overhead";


//***************************************************************************


// Assumes: metrics are still only at leaves (CCT::Stmt)
void 
OverheadMetricFact::make(Prof::CallPath::Profile* prof)
{
  Prof::CCT::Tree* cct = prof->cct();
  if (!cct) { return; }

  // ------------------------------------------------------------
  // Create parallel overhead metric descriptor
  // Create mapping from source metrics to overhead metrics
  // ------------------------------------------------------------
  std::vector<uint> metric_src;
  std::vector<uint> metric_dst;
  
  uint numMetrics_orig = prof->numMetrics();
  for (uint m_id = 0; m_id < numMetrics_orig; ++m_id) {
    Prof::SampledMetricDesc* m_desc = prof->metric(m_id);
    if (OverheadMetricFact::isMetricSrc(m_desc)) {
      OverheadMetricFact::convertToWorkMetric(m_desc);
      metric_src.push_back(m_id);

      Prof::SampledMetricDesc* m_new = 
	new Prof::SampledMetricDesc("overhead", "parallel overhead", 
				    m_desc->period());
      uint m_new_id = prof->addMetric(m_new);
      DIAG_Assert(m_new_id >= numMetrics_orig, "Currently, we assume new metrics are added at the end of the metric vector.");
      metric_dst.push_back(m_new_id);
    }
  }

  // ------------------------------------------------------------
  // Create space for metric values
  // ------------------------------------------------------------
  uint n_new_metrics = metric_dst.size();

  for (Prof::CCT::ANodeIterator it(cct->root()); it.Current(); ++it) {
    Prof::CCT::ANode* x = it.CurNode();
    Prof::CCT::ADynNode* x_dyn = dynamic_cast<Prof::CCT::ADynNode*>(x);
    if (x_dyn) {
      x_dyn->expandMetrics_after(n_new_metrics);
    }
  }

  make(cct->root(), metric_src, metric_dst, false);
}


void 
OverheadMetricFact::make(Prof::CCT::ANode* node, 
			 const std::vector<uint>& m_src, 
			 const std::vector<uint>& m_dst, 
			 bool isOverheadCtxt)
{
  if (!node) { return; }

  // ------------------------------------------------------------
  // Visit CCT::Stmt nodes (Assumes metrics are only at leaves)
  // ------------------------------------------------------------
  if (isOverheadCtxt && (typeid(*node) == typeid(Prof::CCT::Stmt))) {
    Prof::CCT::Stmt* stmt = dynamic_cast<Prof::CCT::Stmt*>(node);
    for (uint i = 0; i < m_src.size(); ++i) {
      uint src_idx = m_src[i];
      uint dst_idx = m_dst[i];
      hpcfile_metric_data_t mval = stmt->metric(src_idx);
      
      stmt->metricDecr(src_idx, mval);
      stmt->metricIncr(dst_idx, mval);
    }
  }

  // ------------------------------------------------------------
  // Recur
  // ------------------------------------------------------------
  
  // Note: once set, isOverheadCtxt should remain true for all descendents
  bool isOverheadCtxt_nxt = isOverheadCtxt;
  if (!isOverheadCtxt && typeid(*node) == typeid(Prof::CCT::ProcFrm)) {
    Prof::CCT::ProcFrm* x = dynamic_cast<Prof::CCT::ProcFrm*>(node);
    isOverheadCtxt_nxt = OverheadMetricFact::isOverhead(x);
  }

  for (Prof::CCT::ANodeChildIterator it(node); it.Current(); ++it) {
    Prof::CCT::ANode* x = it.CurNode();
    make(x, m_src, m_dst, isOverheadCtxt_nxt);
  }
}


void
OverheadMetricFact::convertToWorkMetric(Prof::SampledMetricDesc* mdesc)
{
  const string& nm = mdesc->name();
  if (nm.find("PAPI_TOT_CYC") == 0) {
    mdesc->name("work (cyc)");
  }
  else if (nm.find("WALLCLOCK") == 0) {
    mdesc->name("work (us)");
  }
  else {
    DIAG_Die(DIAG_Unimplemented);
  }
}


} // namespace CallPath

} // namespace Analysis

//***************************************************************************