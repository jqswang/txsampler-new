// -*-Mode: C++;-*-
// $Id$

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

//*************************** User Include Files ****************************

#include "Raw.hpp"
#include "Util.hpp"

#include <lib/prof-lean/hpcfile_csproflib.h>
#include <lib/prof-juicy/FlatProfileReader.hpp>

#include <lib/support/diagnostics.h>

//*************************** Forward Declarations ***************************

//****************************************************************************

void 
Analysis::Raw::writeAsText(/*destination,*/ const char* filenm)
{
  using namespace Analysis::Util;

  ProfType_t ty = getProfileType(filenm);
  if (ty == ProfType_CALLPATH) {
    writeAsText_callpath(filenm);
  }
  else if (ty == ProfType_FLAT) {
    writeAsText_flat(filenm);
  }
  else {
    DIAG_Die(DIAG_Unimplemented);
  }
}


void
Analysis::Raw::writeAsText_callpath(const char* filenm) 
{
  if (!filenm) { return; }

  hpcfile_csprof_data_t metadata;
  int ret;

  FILE* fs = hpcfile_open_for_read(filenm);
  if (!fs) { 
    DIAG_Throw(filenm << ": could not open");
  }

  ret = hpcfile_csprof_fprint(fs, stdout, &metadata);
  if (ret != HPCFILE_OK) {
    DIAG_Throw(filenm << ": error reading HPC_CSPROF");
  }
  
  uint num_metrics = metadata.num_metrics;
  
  ret = hpcfile_cstree_fprint(fs, num_metrics, stdout);
  if (ret != HPCFILE_OK) { 
    DIAG_Throw(filenm << ": error reading HPC_CSTREE.");
  }

  hpcfile_close(fs);
}


void
Analysis::Raw::writeAsText_flat(const char* filenm) 
{
  if (!filenm) { return; }
  
  Prof::Flat::Profile prof;
  try {
    prof.read(filenm);
  }
  catch (...) {
    DIAG_EMsg("While reading '" << filenm << "'");
    throw;
  }

  prof.dump(std::cout);
}

