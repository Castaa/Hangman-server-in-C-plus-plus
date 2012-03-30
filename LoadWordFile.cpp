#include <fstream>
#include "LoadWordFile.h"

using namespace std;

bool LoadWordFile(vector<string>& wordVector)
{
  ifstream wordFile("dictionary.txt"); // NOTE: app expects file name to be in the same directory as exe
  
  // file found and opened successfully?
  if( !wordFile.is_open() )
  {
    return false;
  }
  
  string tempStr;
  
  while( wordFile.good() )
  {
    wordFile >> tempStr; 
    wordVector.push_back(tempStr); 
  }

  wordFile.close();
  
  return true;
}