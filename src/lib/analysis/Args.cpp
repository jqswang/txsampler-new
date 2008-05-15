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

#include "Args.hpp"

#include <lib/support/diagnostics.h>

//*************************** Forward Declarations **************************

//***************************************************************************
// Args
//***************************************************************************

Analysis::Args::Args()
{
  Ctor();
}


void
Analysis::Args::Ctor()
{
  // -------------------------------------------------------
  // Correlation arguments
  // -------------------------------------------------------

  // -------------------------------------------------------
  // Output arguments
  // -------------------------------------------------------

  db_dir          = Analysis_EXPERIMENTDB;
  outFilename_XML = Analysis_EXPERIMENTXML;
  outFilename_CSV = "";
  outFilename_TSV = "";

  db_copySrcFiles = true;

  metrics_computeInteriorValues = true; // dump metrics on interior nodes
}


Analysis::Args::~Args()
{
}


string
Analysis::Args::toString() const
{
  std::ostringstream os;
  dump(os);
  return os.str();
}


void 
Analysis::Args::dump(std::ostream& os) const
{
  os << "db_dir= " << db_dir << std::endl;
  os << "outFilename_XML= " << outFilename_XML << std::endl;
  os << "outFilename_CSV= " << outFilename_CSV << std::endl;
  os << "outFilename_TSV= " << outFilename_TSV << std::endl;
}


void 
Analysis::Args::ddump() const
{
  dump(std::cerr);
}


//***************************************************************************
 
