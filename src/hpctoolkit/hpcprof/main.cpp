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
#include <iomanip>
#include <fstream>
#include <new>

#include <string>
using std::string;

#include <vector>

#include <sys/stat.h>
#include <sys/types.h>

//*************************** User Include Files ****************************

#include "Args.hpp"
#include "CSProfileUtils.hpp"

#include <lib/prof-juicy-x/XercesUtil.hpp>
#include <lib/prof-juicy-x/PGMReader.hpp>

#include <lib/binutils/LM.hpp>

//*************************** Forward Declarations ***************************

static CSProfile* 
readProfileData(std::vector<string>& profileFiles, const string& exe_fnm);

static PgmScopeTree*
readStructureData(std::vector<string>& structureFiles);

static void
dumpProfileData(std::ostream& os, std::vector<string>& profileFiles);

static void
processCallingCtxtTree(CSProfile* profData, VMA begVMA, VMA endVMA, 
		       const string& lm_fnm, LoadModScope* lmScope);

static void
processCallingCtxtTree(CSProfile* profData, VMA begVMA, VMA endVMA, 
		       const string& lm_fnm);


//****************************************************************************

int realmain(int argc, char* const* argv);

int 
main(int argc, char* const* argv) 
{
  int ret;

  try {
    ret = realmain(argc, argv);
  }
  catch (const Diagnostics::Exception& x) {
    DIAG_EMsg(x.message());
    exit(1);
  } 
  catch (const std::bad_alloc& x) {
    DIAG_EMsg("[std::bad_alloc] " << x.what());
    exit(1);
  } 
  catch (const std::exception& x) {
    DIAG_EMsg("[std::exception] " << x.what());
    exit(1);
  } 
  catch (...) {
    DIAG_EMsg("Unknown exception encountered!");
    exit(2);
  }

  return ret;
}


int
realmain(int argc, char* const* argv) 
{
  Args args(argc, argv);

  // ------------------------------------------------------------
  // Special short-circuit behavior
  // ------------------------------------------------------------

  if (args.dumpProfiles) {
    dumpProfileData(cout, args.profileFiles);
    return 0;
  }
  
  // ------------------------------------------------------------
  // Read 'profData', the profiling data file
  // ------------------------------------------------------------
  CSProfile* prof = readProfileData(args.profileFiles, args.exeFile);

  // ------------------------------------------------------------
  // Add source file info
  // ------------------------------------------------------------

  PgmScopeTree* scopeTree = NULL;
  PgmScope* pgmScope = NULL;
  if (!args.structureFiles.empty()) {
    scopeTree = readStructureData(args.structureFiles);
    pgmScope = scopeTree->GetRoot();
  }
  
  try { 
    // for each (used) load module, add information to the calling
    // context tree.
    
    // for efficiency, we add a flag in CSProfLDmodule "used"
    // add one more pass search the callstack tree, to set 
    // the flag -"alpha"- only, since we get info from binary 
    // profile file not from bfd  
    ldmdSetUsedFlag(prof); 

    // Note that this assumes iteration in reverse sorted order
    int num_lm = prof->epoch()->GetNumLdModule();
    VMA endVMA = VMA_MAX;
    
    for (int i = num_lm - 1; i >= 0; i--) {
      CSProfLDmodule* csp_lm = prof->epoch()->GetLdModule(i); 
      VMA begVMA = csp_lm->GetMapaddr(); // for next csploadmodule

      if (csp_lm->GetUsedFlag()) {
	const string& lm_fnm = csp_lm->GetName();
	LoadModScope* lmScope = NULL;
	if (pgmScope) {
	  lmScope = pgmScope->FindLoadMod(lm_fnm);
	}
	
	if (lmScope) {
	  DIAG_Msg(1, "Using STRUCTURE for: " << lm_fnm);
	  processCallingCtxtTree(prof, begVMA, endVMA, lm_fnm, lmScope);
	}
	else {
	  DIAG_Msg(1, "Using debug info for: " << lm_fnm);
	  processCallingCtxtTree(prof, begVMA, endVMA, lm_fnm);
	}
      }

      endVMA = begVMA - 1;
    } /* for each load module */ 
    
    normalizeCSProfile(prof);
  }
  catch (...) {
    DIAG_EMsg("While preparing CSPROFILE...");
    throw;
  }
  
  delete scopeTree;

  // ------------------------------------------------------------
  // Dump
  // ------------------------------------------------------------

  // prepare output directory 
  args.createDatabaseDirectory();
    
  string dbSrcDir = args.dbDir + "/src";
  if (mkdir(dbSrcDir.c_str(),
	    S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) != 0) {
    DIAG_Die("could not create database source code directory " << dbSrcDir);
  }
  
  copySourceFiles(prof, args.searchPaths, dbSrcDir);
  
  string experiment_fnm = args.dbDir + "/" + args.OutFilename_XML;
  writeCSProfileInDatabase(prof, experiment_fnm);
  //writeCSProfile(prof, std::cout, /* prettyPrint */ true);
  

  delete prof;
  return (0);
}

//****************************************************************************

static CSProfile* 
readProfileFile(const string& prof_fnm, const string& exe_fnm);


static CSProfile* 
readProfileData(std::vector<string>& profileFiles, const string& exe_fnm)
{
  CSProfile* prof = readProfileFile(profileFiles[0], exe_fnm);
  
  for (int i = 1; i < profileFiles.size(); ++i) {
    CSProfile* p = readProfileFile(profileFiles[i], exe_fnm);
    prof->merge(*p);
    delete p;
  }
  
  return prof;
}


static CSProfile* 
readProfileFile(const string& prof_fnm, const string& exe_fnm)
{
  CSProfile* prof = NULL;
  try {
    //prof = TheProfileReader.ReadProfileFile(args.profFnm /*type*/);
    prof = ReadProfile_CSPROF(prof_fnm.c_str(), exe_fnm.c_str());
  } 
  catch (...) {
    DIAG_EMsg("While reading profile '" << prof_fnm << "'...");
    throw;
  }
  return prof;
}


static void
dumpProfileData(std::ostream& os, std::vector<string>& profileFiles)
{
  for (int i = 0; i < profileFiles.size(); ++i) {
    const char* fnm = profileFiles[i].c_str();

    os << std::setfill('=') << std::setw(77) << "=" << endl;
    os << fnm << std::endl;
    os << std::setfill('=') << std::setw(77) << "=" << endl;

    DumpProfile_CSPROF(fnm); // stdout
  }
}


//****************************************************************************

  
class MyDocHandlerArgs : public DocHandlerArgs {
public:
  MyDocHandlerArgs()  { }
  ~MyDocHandlerArgs() { }
  
  virtual string ReplacePath(const char* oldpath) const { return oldpath; }
  virtual bool MustDeleteUnderscore() const { return false; }
};


static PgmScopeTree*
readStructureData(std::vector<string>& structureFiles)
{
  InitXerces();

  string path = ".";
  PgmScope* pgm = new PgmScope("");
  PgmScopeTree* scopeTree = new PgmScopeTree("", pgm);
  NodeRetriever scopeTreeInterface(pgm, path);
  MyDocHandlerArgs args;

  for (uint i = 0; i < structureFiles.size(); ++i) {
    const string& fnm = structureFiles[i];
    read_PGM(&scopeTreeInterface, fnm.c_str(), PGMDocHandler::Doc_STRUCT, args);
  }

  FiniXerces();
  
  return scopeTree;
}

//****************************************************************************

static void
processCallingCtxtTree(CSProfile* prof, VMA begVMA, VMA endVMA, 
		       const string& lm_fnm, LoadModScope* lmScope)
{
  VMA relocVMA = 0;

  try {
    binutils::LM* lm = new binutils::LM();
    lm->open(lm_fnm.c_str());
    if (lm->type() != binutils::LM::Executable) {
      relocVMA = begVMA;
    }
    delete lm;
  }
  catch (...) {
    DIAG_EMsg("While reading '" << lm_fnm << "'...");
    throw;
  }
  
  inferCallFrames(prof, begVMA, endVMA, lmScope, relocVMA);
}


static void
processCallingCtxtTree(CSProfile* prof, VMA begVMA, VMA endVMA, 
		       const string& lm_fnm)
{
  binutils::LM* lm = NULL;
  try {
    lm = new binutils::LM();
    lm->open(lm_fnm.c_str());
    lm->read();
  }
  catch (...) {
    DIAG_EMsg("While reading '" << lm_fnm << "'...");
    throw;
  }
  
  // get the start and end PC from the text sections 
  if (lm->type() != binutils::LM::Executable) {
    lm->relocate(begVMA);
  }
  
  inferCallFrames(prof, begVMA, endVMA, lm);
  
  delete lm;
}
