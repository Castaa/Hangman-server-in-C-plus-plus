#pragma once

#include <string>
#include <unordered_map>
#include <map>
#include <list>
#include <vector>
#include "tinyxml.h"
/* HiScore holds its MAC Address, Name and Score */


struct HiScore
{
  std::string Name;
  int         Score;
  HiScore() : Name("Anonymous"), Score(0) {}
  //bool operator<(const HiScore*& rhs) { return Score > rhs->Score; }
};
struct CompFunctor
{
  bool operator()( const HiScore* a, const HiScore* b ) { return a->Score > b->Score; }
};

typedef std::unordered_map<std::string, HiScore> ScoresUMap;
typedef std::list<HiScore*>                      ScoresSorted;
class HiScoreList
{
  ScoresUMap     m_ScoresUMap;
  ScoresSorted   m_ScoresSorted;
  TiXmlDocument* m_XmlDocument;

public:
  HiScoreList();
  ~HiScoreList();
  
  void SaveConfig();
  void UpdateHiScoreList( const std::string& MacAddr, const HiScore& ScoreData, bool bSort = true );
  void SortHighScoreList( void );
  int  GetHiScore( const std::string& MACAddr );
  int  ReturnRank( int Score, bool& bTied, bool Match=false );
  int  ReturnRank( const std::string& MACAddr, bool& bTied );
  void GetRelativeHighScoreList( const std::string& MACAddrStr, std::vector<int>& List, int& Rank );

  // used for XML parsing
  void QueryString( const TiXmlElement& ele, const char* name, std::string& result );
};