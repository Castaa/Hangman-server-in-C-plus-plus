#include "HiScores.h"
#include "assert.h"
#include "sstream"
#include "iostream"


using namespace std;


HiScoreList::HiScoreList()
{
// load high score config or create config file if needed
  char configFilename[] = "config.txt";
	m_XmlDocument = new TiXmlDocument( configFilename );

  if( !m_XmlDocument )
  {		
    assert(0);
  }

  bool bConfigFileCorrupted = false;

  if( m_XmlDocument->LoadFile() ) // if the file loaded correctly  
  {
    TiXmlNode * configNode = m_XmlDocument->FirstChild("config");
    if( configNode )
    {
      TiXmlNode * nodeHiScore = m_XmlDocument->FirstChild("hiscores");
      if( nodeHiScore )
      {
        if( nodeHiScore->FirstChild() )
        {
          for( TiXmlNode* it = nodeHiScore->FirstChild(); it != NULL; it = it->NextSibling() )
          {
            TiXmlElement * ScoreElement = it->ToElement();
            if ( ScoreElement )
            {
              string MACAddr;
              HiScore ScoreTemp;

              QueryString( *ScoreElement,      "MAC", MACAddr );
              ScoreElement->QueryIntAttribute( "Score", &ScoreTemp.Score );
              QueryString( *ScoreElement,      "Name", ScoreTemp.Name );

              UpdateHiScoreList( MACAddr, ScoreTemp, false );  // add to high score list

            }
          }

          SortHighScoreList(); // sort all the data we just loaded //
        }
      } // if hi score node
      else
      {
        bConfigFileCorrupted = true;
      }
    }
    else
    {
      bConfigFileCorrupted = true;
    }

    if( !bConfigFileCorrupted )
    {
      cout << "Config loaded:  " << m_ScoresUMap.size() << " hi-score(s) found." << endl;
      for( ScoresUMap::iterator it = m_ScoresUMap.begin(); it != m_ScoresUMap.end(); it++)
      {
        cout << (*it).first << " = " << (*it).second.Score << " = " << (*it).second.Name << endl;
      }
    }
  } // if loaded
  else
  {
      bConfigFileCorrupted = true;
  }


  if ( bConfigFileCorrupted )  // if file is missing or empty for some reason
  {
    cout << "Config file missing or corrupted, creating new config file." << endl;

    delete m_XmlDocument;
    m_XmlDocument = NULL;

    // config file is missing or corrupted, create it from scratch using our defaults //
	  m_XmlDocument = new TiXmlDocument( configFilename );
    
    // add xml version info to the first line //
    TiXmlDeclaration * decl = new TiXmlDeclaration( "1.0", "", "" );
	  m_XmlDocument->LinkEndChild( decl );

    //TiXmlElement* elemConfig = 
    m_XmlDocument->LinkEndChild( new TiXmlElement("config") )->ToElement();
    //TiXmlElement* elemScore = 
    m_XmlDocument->LinkEndChild( new TiXmlElement("hiscores") )->ToElement();
      //stringstream temp;
      //string tmpStr;
      //int j = 1;
      //m_HighScores.clear();
      //for( int i = cHighScoreSize; i >= 1; i--, j++ ) // set default high scores
      //{
      //  m_HighScores.push_back( i*10 ); // 2500 to 100
      //  // create s# tag name
      //  temp << 's' << j;
      //  tmpStr = temp.str();
      //  temp.clear();
      //  temp.str(std::string()); // clear buffer
      //  TiXmlElement* elemRank = elemScore->LinkEndChild( new TiXmlElement(tmpStr.c_str()) )->ToElement();
      //  // convert int value to string
      //  temp << m_HighScores.back();
      //  tmpStr = temp.str();
      //  temp.clear();
      //  temp.str(std::string()); // clear buffer
      //  elemRank->LinkEndChild( new TiXmlText(tmpStr.c_str()) );
      //}

	  m_XmlDocument->SaveFile();
  }
}

HiScoreList::~HiScoreList()
{
  cout << "SAVING HI SCORE DATA." << endl;
  SaveConfig();
  delete m_XmlDocument;
}

void HiScoreList::SaveConfig()
{
   // save config file
  if( m_XmlDocument )
  {
    TiXmlElement* elemScore = m_XmlDocument->FirstChildElement("hiscores");
    if( elemScore )
    {
      elemScore->Clear();
      stringstream temp;
      string tmpStr;

      int i = 1;
      for( ScoresUMap::iterator it = m_ScoresUMap.begin(); it != m_ScoresUMap.end(); it++, i++ )
      {
        // build custom tag for this score
        temp << 's' << i;
        tmpStr = temp.str();
        temp.clear();
        temp.str(std::string()); // clear buffer
 
        TiXmlElement* elemRank = elemScore->LinkEndChild( new TiXmlElement(tmpStr.c_str()) )->ToElement();

        elemRank->SetAttribute( "MAC",   (*it).first.c_str() );
	      elemRank->SetAttribute( "Score", (*it).second.Score );
	      elemRank->SetAttribute( "Name",  (*it).second.Name.c_str() );
      }
    }
    m_XmlDocument->SaveFile();
  }
}

void HiScoreList::UpdateHiScoreList( const std::string& MACAddr, const HiScore& ScoreData, bool bSort /*=true*/)
{
  if( ScoreData.Score > 0 ) // only save scores above zero
  { 
    HiScore& ScoreDataRefInMap = m_ScoresUMap[MACAddr];
    if( ScoreDataRefInMap.Score < ScoreData.Score )  // new high score for this user, update the list
    {
      bool bNew = false;
      if( ScoreDataRefInMap.Score == 0 ) // if it's a new player, the inital value will be zero
      {
        bNew = true;
      }

      ScoreDataRefInMap = ScoreData;
      
      if( bNew )
      {
        m_ScoresSorted.push_back( &ScoreDataRefInMap ); // loading update, sort later
      }

      if( bSort ) // used to defer sorting for a later time
      {
        SortHighScoreList();
      }

      //SaveConfig(); // TO DO: remove, only save on server shut down
    }
  }
}


void HiScoreList::SortHighScoreList()
{
  CompFunctor f;
  m_ScoresSorted.sort(f); 

  for( ScoresSorted::iterator it = m_ScoresSorted.begin(); it != m_ScoresSorted.end(); it++ )
  {
    cout << (*it)->Score << " " << (*it)->Name << endl;
  }
}

int HiScoreList::GetHiScore( const std::string& MACAddr )
{
// find high score in list, if it exists
 ScoresUMap::iterator it = m_ScoresUMap.find( MACAddr );

 if( it == m_ScoresUMap.end() )
 {
   return 0;
 }
 else
 {
   return it->second.Score;
 }
}


void HiScoreList::QueryString( const TiXmlElement& ele, const char* name, std::string& result )
{
  const char * val = ele.Attribute(name);
  if ( val ) 
    result = val;
}


// this returns the player's best rank of highscore on the server
int HiScoreList::ReturnRank( const std::string& MACAddr, bool& bTied )
{
  bTied = false;
  int hiScore = GetHiScore( MACAddr );

  if( hiScore == 0 ) // a returned value of zero means no high score yet
  {
    return 0;
  }
  
  return ReturnRank( hiScore, bTied, true );
}


// this returns the player's ranking of the given score
int  HiScoreList::ReturnRank( int hiScore, bool& bTied, bool bMatch/*=false*/ )
{
  bTied = false;
  int Rank = 1;

  ScoresSorted::iterator it;
  if( bMatch ) // find the exact score in the high score list
  {
    for( it = m_ScoresSorted.begin(); it != m_ScoresSorted.end(); it++, Rank++)
    {
      if( (*it)->Score == hiScore )
      {
        it++;
        if( it != m_ScoresSorted.end() )
        {
          if( (*it)->Score == hiScore )
          {
            bTied = true;
          }
        }
        break;
      }
    }

    // no match, return rank 0
    if( it == m_ScoresSorted.end() )
    {
      Rank = 0;
    }
  }
  else // find the position this score would rank if it were put into the list //
  {
    for( it = m_ScoresSorted.begin(); it != m_ScoresSorted.end(); it++, Rank++)
    {
      if( hiScore >= (*it)->Score )
      {
        if( (*it)->Score == hiScore )
        {
          bTied = true;
        }
        break;
      }
    }
  }
  return Rank;
}

void HiScoreList::GetRelativeHighScoreList( const string& MACAddrStr, vector<int>& List, int& Rank )
{
  int Score = GetHiScore( MACAddrStr );
  List.clear();
  const int MaxListSize = 5; // max number of scores to return to the client

  ScoresSorted::iterator it;
  Rank = 1;
  bool bFound = false;
  if( Score ) // find the exact score in the high score list
  {
    for( it = m_ScoresSorted.begin(); it != m_ScoresSorted.end(); it++, Rank++)
    {
      if( (*it)->Score == Score )
      {
        bFound = true;
        break;
      }
    }
  }

  int HiScoreListSize = m_ScoresSorted.size();

  // return top of high score list under these following conditions:
  if( Score == 0 // no high score at all
      || (!bFound) // score not found
         || HiScoreListSize <= MaxListSize // the requested size is larger than the list itself, so just return the entire list
           || Rank <= (MaxListSize/2) ) // High score found at the top of the list, return a top scores
  {
    cout << "Top of the High Score List returned" << endl;
    Rank = 1;
    int count = 0;
    for( it = m_ScoresSorted.begin(); it != m_ScoresSorted.end() && count < MaxListSize; it++, count++)
    {
      List.push_back((*it)->Score);
    }
  }
  else if( Rank+(MaxListSize/2) >= HiScoreListSize ) // else if the score is found at very near the bottom of the list, return the bottom most scores
  {
    cout << "Bottom of the High Score List returned" << endl;

    List.resize(MaxListSize);
    ScoresSorted::reverse_iterator rit;

    int index = MaxListSize-1;
    for( rit = m_ScoresSorted.rbegin(); rit != m_ScoresSorted.rend() && index >= 0; rit++, index--)
    {
      List[index] = (*rit)->Score;
    }

    Rank = HiScoreListSize - (MaxListSize - 1);
  }
  else // else the ranked score is in the middle of the high score list
  {
    List.resize(MaxListSize);

    cout << "Middle of the High Score List returned" << endl;

    // 10  [0] 1
    //  9  [1] 2
    //  8  [2] 3
    //  7  [3] 4
    //  6  [4] 5
    //  *5 [5] 6
    //  4  [6] 7
    //  3  [7] 8
    //  2  [8] 9
    //  1  [9] 10
    ScoresSorted::iterator it2 = it;
    it2--; // decrement the interator to score just before our matched score

    int index = (MaxListSize/2); 
    for( ; it != m_ScoresSorted.end() && index < MaxListSize; it++, index++)
    {
      List[index] = (*it)->Score;
    }

    index = (MaxListSize/2)-1;
    for( ; index >= 0; it2--, index-- )
    {
      Rank--;
      List[index] = (*it2)->Score;
    }
  }
}