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

#ifndef Driver_hpp
#define Driver_hpp 

//************************ System Include Files ******************************

#include <iostream>
#include <ostream>
#include <list>    // STL
#include <vector>  // STL
#include <utility> // STL
#include <string>

//************************* User Include Files *******************************

#include <include/general.h>

#include "DerivedPerfMetrics.hpp" // for FilePerfMetric

#include <lib/prof-juicy-x/PGMDocHandler.hpp>
#include <lib/prof-juicy-x/DocHandlerArgs.hpp>

#include <lib/prof-juicy/PgmScopeTree.hpp>

#include <lib/support/Unique.hpp>

//************************ Forward Declarations ******************************

//****************************************************************************

// PathTuple: a {path, viewname} pair.
//   PathTuple.first = path; PathTuple.second = viewname
// PathTupleVec: the vector of all 'PathTuple'
typedef std::pair<std::string, std::string> PathTuple;
typedef std::vector<PathTuple> PathTupleVec;

class Driver : public Unique { // at most one instance 
public: 
  Driver(int deleteUnderscores, bool _cpySrcFiles); 
  ~Driver(); 
  
  void SetTitle(const char* tit)        { title = tit; }
  void SetTitle(const std::string& tit) { title = tit; }
  const std::string& Title() const      { return title; }

  void AddStructureFile(const char* pf) 
    { structureFiles.push_back(new std::string(pf)); }
  void AddStructureFile(const std::string& pf) 
    { AddStructureFile(pf.c_str()); }
  const std::string& GetStructureFile(int i) const 
    { return *structureFiles[i]; }
  int NumberOfStructureFiles() const { return structureFiles.size(); }

  void AddGroupFile(const char* pf) 
    { groupFiles.push_back(new std::string(pf)); }
  void AddGroupFile(const std::string& pf) 
    { AddGroupFile(pf.c_str()); }
  const std::string& GetGroupFile(int i) const { return *groupFiles[i]; }
  int NumberOfGroupFiles() const { return groupFiles.size(); }

  void AddPath(const char* _path, const char* _viewname);
  void AddPath(const std::string& _path, const std::string& _viewname)
    { AddPath(_path.c_str(), _viewname.c_str()); }

  const std::string& Path() const { return path; }
  const PathTupleVec& PathVec() const { return pathVec; }

  void AddReplacePath(const char* inPath, const char* outPath); 
  void AddReplacePath(const std::string& inPath, const std::string& outPath)
    { AddReplacePath(inPath.c_str(), outPath.c_str()); }

  std::string ReplacePath(const char* path);
  std::string ReplacePath(const std::string& path)
    { return ReplacePath(path.c_str()); }
  
  bool MustDeleteUnderscore( void );
  bool CopySrcFiles() { return cpySrcFiles; }
  
  unsigned int NumberOfMetrics() const       { return dataSrc.size(); }
  const PerfMetric& PerfDataSrc(int i) const { return *dataSrc[i]; }
  void Add(PerfMetric* metric); 
  
  void ScopeTreeInitialize(PgmScopeTree& scopes);
  void ScopeTreeComputeMetrics(PgmScopeTree& scopes);

  std::string ToString() const; 
  void Dump() const { std::cerr << ToString() << std::endl; }

  // see 'ScopeInfo' class for dump flags
  void XML_Dump(PgmScope* pgm, std::ostream &os = std::cout, 
		const char *pre = "") const;
  void XML_Dump(PgmScope* pgm, int dumpFlags, std::ostream &os = std::cout, 
		const char *pre = "") const;
  void CSV_Dump(PgmScope* pgm, std::ostream &os = std::cout) const;
  void TSV_Dump(PgmScope* pgm, std::ostream &os = std::cout) const;

private:
  typedef std::list<FilePerfMetric*> MetricList_t;
  
  void ProcessPGMFile(NodeRetriever* nretriever, 
		      PGMDocHandler::Doc_t docty, 
		      std::vector<std::string*>* files);

  void ScopeTreeComputeHPCRUNMetrics(PgmScopeTree& scopes);
  void ScopeTreeComputeOtherMetrics(PgmScopeTree& scopes);
  void ScopeTreeInsertHPCRUNData(PgmScopeTree& scopes,
				 const string& profFilenm,
				 const MetricList_t& metricList);
  
private:
  std::string title;
  int deleteTrailingUnderscores;
  bool cpySrcFiles;

  std::string path;        // a string-list of paths (includes '.') 
  PathTupleVec pathVec;    // a list of {path, viewname} 

  std::vector<std::string> replaceInPath;
  std::vector<std::string> replaceOutPath;

  std::vector<PerfMetric*>  dataSrc;
  std::vector<std::string*> structureFiles;
  std::vector<std::string*> groupFiles;
};

//****************************************************************************

class DriverDocHandlerArgs : public DocHandlerArgs {
public:
  DriverDocHandlerArgs(Driver* driver) 
    : m_driver(driver) { }
  
  ~DriverDocHandlerArgs() { }
  
  virtual string ReplacePath(const char* oldpath) const { 
    return m_driver->ReplacePath(oldpath);
  };
  
  virtual bool MustDeleteUnderscore() const { 
    return m_driver->MustDeleteUnderscore();
  }

private:
  Driver* m_driver;
};

#endif 