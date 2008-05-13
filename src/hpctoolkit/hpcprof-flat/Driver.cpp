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

//************************ System Include Files ******************************

#include <iostream>
using std::hex;
using std::dec;
using std::endl;

#include <string>
using std::string;

#include <map>
using std::map;

#include <vector>
using std::vector;

//************************* User Include Files *******************************

#include "Driver.hpp"

#include <lib/prof-juicy-x/PGMReader.hpp>
#include <lib/prof-juicy-x/XercesSAX2.hpp>
#include <lib/prof-juicy-x/XercesErrorHandler.hpp>

#include <lib/prof-juicy/PgmScopeTreeInterface.hpp>
#include <lib/prof-juicy/PgmScopeTree.hpp>
#include <lib/prof-juicy/FlatProfileReader.hpp>

#include <lib/binutils/LM.hpp>

#include <lib/support/diagnostics.h>
#include <lib/support/Trace.hpp>
#include <lib/support/StrUtil.hpp>
#include <lib/support/pathfind.h>

//************************ Forward Declarations ******************************


//****************************************************************************

Driver::Driver(int deleteUnderscores, bool _cpySrcFiles)
  : Unique("Driver") 
{
  path = ".";
  deleteTrailingUnderscores = deleteUnderscores;
  cpySrcFiles = _cpySrcFiles;
} 


Driver::~Driver()
{
} 


void
Driver::AddPath(const char* _path, const char* _viewname)
{
  path += string(":") + _path;
  pathVec.push_back( PathTuple(string(_path), string(_viewname)) );
}


void
Driver::AddReplacePath(const char* inPath, const char* outPath)
{
  // Assumes error and warning check has been performed
  // (cf. ConfigParser)

  // Add a '/' at the end of the in path; it's good when testing for
  // equality, to make sure that the last directory in the path is not
  // a prefix of the tested path.  
  // If we need to add a '/' to the in path, then add one to the out
  // path, too because when is time to replace we don't know if we
  // added one or not to the IN path.
  if (strlen(inPath) > 0 && inPath[strlen(inPath)-1] != '/') {
    replaceInPath.push_back(string(inPath) + '/');
    replaceOutPath.push_back(string(outPath) + '/');
  }
  else {
    replaceInPath.push_back(string(inPath));
    replaceOutPath.push_back(string(outPath)); 
  }
  DIAG_Msg(3, "AddReplacePath: " << inPath << " -to- " << outPath);
}


/* Test the specified path against each of the paths in the database. 
 * Replace with the pair of the first matching path.
 */
string 
Driver::ReplacePath(const char* oldpath)
{
  DIAG_Assert(replaceInPath.size() == replaceOutPath.size(), "");
  for (uint i = 0 ; i<replaceInPath.size() ; i++ ) {
    uint length = replaceInPath[i].length();
    // it makes sense to test for matching only if 'oldpath' is strictly longer
    // than this replacement inPath.
    if (strlen(oldpath) > length &&  
	strncmp(oldpath, replaceInPath[i].c_str(), length) == 0 ) { 
      // it's a match
      string s = replaceOutPath[i] + &oldpath[length];
      DIAG_Msg(3, "ReplacePath: Found a match! New path: " << s);
      return s;
    }
  }
  // If nothing matched, return the original path
  DIAG_Msg(3, "ReplacePath: Nothing matched! Init path: " << oldpath);
  return string(oldpath);
}


bool
Driver::MustDeleteUnderscore()
{
  return (deleteTrailingUnderscores > 0);
}


void
Driver::Add(PerfMetric *m) 
{
  dataSrc.push_back(m); 
  DIAG_Msg(3, "Add:: dataSrc[" << dataSrc.size()-1 << "]=" << m->ToString());
} 


string
Driver::ToString() const 
{
  string s =  string("Driver: " ) + "title=" + title + " " + "path=" + path;
  s += "\ndataSrc::\n"; 
  for (uint i =0; i < dataSrc.size(); i++) {
    s += dataSrc[i]->ToString() + "\n"; 
  }
  return s; 
} 


void
Driver::ScopeTreeInitialize(PgmScopeTree& scopes) 
{
  NodeRetriever ret(scopes.GetRoot(), path);
  
  //-------------------------------------------------------
  // if a PGM/Structure document has been provided, use it to 
  // initialize the structure of the scope tree
  //-------------------------------------------------------
  if (NumberOfStructureFiles() > 0) {
    ProcessPGMFile(&ret, PGMDocHandler::Doc_STRUCT, &structureFiles);
  }

  //-------------------------------------------------------
  // if a PGM/Group document has been provided, use it to form the 
  // group partitions (as wall as initialize/extend the scope tree)
  //-------------------------------------------------------
  if (NumberOfGroupFiles() > 0) {
    ProcessPGMFile(&ret, PGMDocHandler::Doc_GROUP, &groupFiles);
  }
}


void
Driver::ScopeTreeComputeMetrics(PgmScopeTree& scopes) 
{
  // -------------------------------------------------------
  // Create metrics, file and computed metrics. 
  //
  // (File metrics are read from PROFILE/HPCRUN files and will update
  // the scope tree with any new information; computed metrics are
  // expressions of file metrics).
  // -------------------------------------------------------
  try {
    ScopeTreeComputeHPCRUNMetrics(scopes);
    ScopeTreeComputeOtherMetrics(scopes);
  } 
  catch (const MetricException &x) {
    DIAG_EMsg(x.message());
    throw;
  }
  catch (...) {
    throw;
  }
}


void
Driver::ScopeTreeComputeHPCRUNMetrics(PgmScopeTree& scopes) 
{
  typedef std::map<string, MetricList_t> StringToMetricListMap_t;
  StringToMetricListMap_t fileToMetricMap;

  // create HPCRUN file metrics.  Process all metrics associated with
  // a certain file.

  PgmScope* pgmScope = scopes.GetRoot();
  NodeRetriever ret(pgmScope, path);
  for (uint i = 0; i < dataSrc.size(); i++) {
    PerfMetric* m = dataSrc[i];
    if (IsHPCRUNFilePerfMetric(m)) {
      DIAG_Assert(pgmScope && pgmScope->ChildCount() > 0, "Attempting to correlate HPCRUN type profile-files without STRUCTURE information.");
      
      FilePerfMetric* fm = dynamic_cast<FilePerfMetric*>(m);
      fileToMetricMap[fm->FileName()].push_back(fm);
    }
  }
  
  for (StringToMetricListMap_t::iterator it = fileToMetricMap.begin();
       it != fileToMetricMap.end(); ++it) {
    const string& fnm = it->first;
    const MetricList_t& metrics = it->second;
    ScopeTreeInsertHPCRUNData(scopes, fnm, metrics);
  }
}


void
Driver::ScopeTreeComputeOtherMetrics(PgmScopeTree& scopes) 
{
  // create PROFILE file and computed metrics
  NodeRetriever ret(scopes.GetRoot(), path);
  
  for (uint i = 0; i < dataSrc.size(); i++) {
    PerfMetric* m = dataSrc[i];
    if (!IsHPCRUNFilePerfMetric(m)) {
      m->Make(ret);
    }
  } 
}


void
Driver::ScopeTreeInsertHPCRUNData(PgmScopeTree& scopes,
				  const string& profFilenm,
				  const MetricList_t& metricList)
{
  PgmScope* pgm = scopes.GetRoot();
  NodeRetriever nodeRet(pgm, path);
  
  vector<PerfMetric*> eventToPerfMetricMap;

  //-------------------------------------------------------
  // Read the profile and insert the data
  //-------------------------------------------------------
  Prof::Flat::Profile prof;
  try {
    prof.read(profFilenm);
  }
  catch (...) {
    DIAG_EMsg("While reading '" << profFilenm << "'");
    throw;
  }

  uint num_samples = 0;
  
  for (uint i = 0; i < prof.num_load_modules(); ++i) {
    const Prof::Flat::LM& proflm = prof.load_module(i);
    std::string lmname = proflm.name();
    lmname = ReplacePath(lmname);

    LoadModScope* lmScope = nodeRet.MoveToLoadMod(lmname);
    if (lmScope->ChildCount() == 0) {
      DIAG_WMsg(1, "No STRUCTURE for " << lmname << ".");
    }


    binutils::LM* lm = NULL;
    try {
      lm = new binutils::LM();
      lm->open(lmname.c_str());
    }
    catch (const binutils::Exception& x) {
      DIAG_EMsg("While opening " << lmname.c_str() << ":\n" << x.message());
      continue;
    }
    catch (...) {
      DIAG_EMsg("Exception encountered while opening " << lmname.c_str());
      throw;
    }
    
    //-------------------------------------------------------
    // For each metric, insert performance data into scope tree
    //-------------------------------------------------------
    for (MetricList_t::const_iterator it = metricList.begin(); 
	 it != metricList.end(); ++it) {
      FilePerfMetric* m = *it;
      uint mIdx = (uint)StrUtil::toUInt64(m->NativeName());
      
      const Prof::Flat::Event& profevent = proflm.event(mIdx);
      uint64_t period = profevent.period();
      double mval = 0.0;
      double mval_nostruct = 0.0;

      for (uint k = 0; k < profevent.num_data(); ++k) {
	const Prof::Flat::Datum& dat = profevent.datum(k);
	VMA vma = dat.first; // relocated VMA
	uint32_t samples = dat.second;
	double events = samples * period; // samples * (events/sample)
	
	// 1. Unrelocate vma.
	VMA ur_vma = vma;
	if (lm->type() == binutils::LM::SharedLibrary 
	    && vma > proflm.load_addr()) {
	  ur_vma = vma - proflm.load_addr();
	}
	
	// 2. Find associated scope and insert into scope tree
	ScopeInfo* scope = lmScope->findByVMA(ur_vma);
	if (!scope) {
	  if (NumberOfStructureFiles() > 0 && lmScope->ChildCount() > 0) {
	    DIAG_WMsg(3, "Cannot find STRUCTURE for " << lmname << ":0x" << hex << ur_vma << dec << " [" << m->Name() << ", " << events << "]");
	  }
	  scope = lmScope;
	  mval_nostruct += events;
	}

	mval += events;
	scope->SetPerfData(m->Index(), events); // implicit add!
	DIAG_DevMsg(6, "Metric associate: " 
		    << m->Name() << ":0x" << hex << ur_vma << dec 
		    << " --> +" << events << "=" 
		    << scope->PerfData(m->Index()) << " :: " 
		    << scope->toXML());
      }

      num_samples += profevent.num_data();
      if (mval_nostruct > 0.0) {
	// INVARIANT: division by 0 is impossible since mval >= mval_nostruct
	double percent = (mval_nostruct / mval) * 100;
	DIAG_WMsg(1, "Cannot find STRUCTURE for " << m->Name() << ":" << mval_nostruct << " (" << percent << "%) in " << lmname << ". (Large percentages indicate useless STRUCTURE information. Stale STRUCTURE file? Did STRUCTURE binary have debugging info?)");
      }
    }

    delete lm;
  }

  if (num_samples == 0) {
    DIAG_WMsg(1, profFilenm << " contains no samples!");
  }

  DIAG_If(3) {
    DIAG_Msg(3, "Initial scope tree, before aggregation:");
    XML_Dump(pgm, 0, std::cerr);
  }
  
  //-------------------------------------------------------
  // Accumulate metrics
  //-------------------------------------------------------
  for (MetricList_t::const_iterator it = metricList.begin(); 
       it != metricList.end(); ++it) {
    FilePerfMetric* m = *it;
    AccumulateMetricsFromChildren(pgm, m->Index());
  }
}


void
Driver::ProcessPGMFile(NodeRetriever* nretriever, 
		       PGMDocHandler::Doc_t docty, 
		       std::vector<string*>* files)
{
  if (!files) {
    return;
  }
  
  for (uint i = 0; i < files->size(); ++i) {
    const string& fnm = *((*files)[i]);
    DriverDocHandlerArgs args(this);
    read_PGM(nretriever, fnm.c_str(), docty, args);
  }
}


void
Driver::XML_Dump(PgmScope* pgm, std::ostream &os, const char *pre) const
{
  int dumpFlags = 0; // no dump flags
  XML_Dump(pgm, dumpFlags, os, pre);
}


void
Driver::XML_Dump(PgmScope* pgm, int dumpFlags, std::ostream &os,
		 const char *pre) const
{
  string pre1 = string(pre) + "  ";
  string pre2 = string(pre1) + "  ";
  
  os << pre << "<HPCVIEWER>" << endl;

  // Dump CONFIG
  os << pre << "<CONFIG>" << endl;

  os << pre1 << "<TITLE name=\042" << Title() << "\042/>" << endl;

  const PathTupleVec& pVec = PathVec();
  for (uint i = 0; i < pVec.size(); i++) {
    const string& pathStr = pVec[i].first;
    os << pre1 << "<PATH name=\042" << pathStr << "\042/>" << endl;
  }
  
  os << pre1 << "<METRICS>" << endl;
  for (uint i = 0; i < NumberOfPerfDataInfos(); i++) {
    const PerfMetric& metric = IndexToPerfDataInfo(i); 
    os << pre2 << "<METRIC shortName=\042" << i
       << "\042 nativeName=\042"  << metric.NativeName()
       << "\042 displayName=\042" << metric.DisplayInfo().Name()
       << "\042 display=\042" << ((metric.Display()) ? "true" : "false")
       << "\042 percent=\042" << ((metric.Percent()) ? "true" : "false")
#if 0   // hpcviewer does dynamic sorting, no need for this flag
       << "\042 sortBy=\042"  << ((metric.SortBy())  ? "true" : "false")
#endif
       << "\042/>" << endl;
  }
  os << pre1 << "</METRICS>" << endl;
  os << pre << "</CONFIG>" << endl;
  
  // Dump SCOPETREE
  os << pre << "<SCOPETREE>" << endl;
  pgm->XML_DumpLineSorted(os, dumpFlags, pre);
  os << pre << "</SCOPETREE>" << endl;

  os << pre << "</HPCVIEWER>" << endl;
}


void
Driver::CSV_Dump(PgmScope* pgm, std::ostream &os) const
{
  os << "File name,Routine name,Start line,End line,Loop level";
  for (uint i = 0; i < NumberOfPerfDataInfos(); i++) {
    const PerfMetric& metric = IndexToPerfDataInfo(i); 
    os << "," << metric.DisplayInfo().Name();
    if (metric.Percent())
      os << "," << metric.DisplayInfo().Name() << " (%)";
  }
  os << endl;
  
  // Dump SCOPETREE
  pgm->CSV_TreeDump(os);
}


void
Driver::TSV_Dump(PgmScope* pgm, std::ostream &os) const
{
  os << "LineID";
  for (uint i = 0; i < NumberOfPerfDataInfos(); i++) {
    const PerfMetric& metric = IndexToPerfDataInfo(i); 
    os << "\t" << metric.DisplayInfo().Name();
    /*
    if (metric.Percent())
      os << "\t" << metric.DisplayInfo().Name() << " (%)";
    */
  }
  os << endl;
  
  // Dump SCOPETREE
  pgm->TSV_TreeDump(os);
}
