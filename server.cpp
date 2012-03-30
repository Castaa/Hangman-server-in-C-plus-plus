
// server.cpp
// main() hangman server code


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <iostream>
#include <fstream>
#include <fcntl.h>
#include <time.h>

#include "tinyxml.h"
#include "HiScores.h"
#include "kbhit.h"
#include "LoadWordFile.h"


#define PROTOPORT          20003         // default protocol port number
#define QLEN               5             // size of request queue 
#define CURRENT_CLIENT_VER 0x00010001    // Current most client version
#define MINIMUM_CLIENT_VER 0x00010000    // Lowest version still supported
#define PID_FILE "hserver_pid"           // Used to limit the number of active server instanses to 1

enum  { CL_REQ_HI_SCORE           = 1, 
        CL_SEND_SCORE             = 2,
        CL_REQ_HI_SCORE_LIST      = 3,
        CL_KILL_SERVER            = 4,
        CL_REQ_RANK               = 5,
        CL_REQ_RANK_OF_THIS_SCORE = 6,
        CL_VERSION_CHECK          = 7,
        CL_REQ_WORDS              = 8,
};


int visits     =   0;  /* counts client connections */

bool CheckForAnotherServerInstance();

using namespace std;

int main(int argc, char *argv[])
{
  struct protoent *ptrp;       /* pointer to a protocol table entry */
  struct sockaddr_in sad;      /* structure to hold server's address */
  struct sockaddr_in cad;      /* structure to hold client's address */
  int sd, sd2;                 /* socket descriptors   */
  int port;                    /* protocol port number  */
  socklen_t alen;              /* length of address   */
  int n;                       /* temporary variable                  */
  char buffer[2048];           /* buffer for string the server sends */
  int bIsTCP;                  /* flag storing whether we are using tcp */
  long lRecvd,lEchoed;         /*for statistics*/
  //int iRecvdp, iEchoedp;       /*for statistics*/


  if( CheckForAnotherServerInstance() )
  {
    cout << "ERROR: Another instance of the server is already running." << endl;
    return -1;
  }

  srand( (unsigned)time(NULL) );

  HiScoreList HighScores;

  vector<string> wordVector;
  if( !LoadWordFile(wordVector) || wordVector.empty() )
  {
    cout << "ERROR: Fail to load dictionary data file." << endl;
    return -1;
  }

  //iRecvdp=0;  
  //iEchoedp=0;    
  
  memset((char *)&sad, 0, sizeof(sad)); /* clear sockaddr structure */
  sad.sin_family = AF_INET;             /* set family to Internet */
  sad.sin_addr.s_addr = INADDR_ANY;     /* set the local IP address */

  /* Check command-line argument for protocol port and extract */
  /* port number if one is specified.  Otherwise, use the default */
  /* port value given by constant PROTOPORT   */

  port = PROTOPORT; /* use default port number */

  sad.sin_port = htons((u_short)port);

  printf("Listening on port %d\n",port);

  /* Check command-line argument for which protocol to use.  If 
  none specify then start both tcp and udp */

  bIsTCP = 1; //-jc: force TCP to true
  char pstrzWhichProtocol[]  = "tcp"; //-jc

  printf("Listening for %s packets\n",pstrzWhichProtocol);

  /* Map protocol name to protocol number */
  if ( ((int)(ptrp = getprotobyname(pstrzWhichProtocol))) == 0) {
    fprintf(stderr, "cannot map protocol to protocol number");
    exit(1);
  }

  /* Create a socket, depending on whether we are running as tcp or udp server.  If tcp, then create one that uses 
  k_stream, otherwise, create one that uses datagram.*/ 
  // tcp socket
  sd = socket( PF_INET, SOCK_STREAM, ptrp->p_proto );
  
  if (sd < 0) 
  {
    fprintf(stderr, "socket creation failed\n");
    exit(1);
  }

  /* Bind a local address to the socket */
  /*In the udp and tcp servers running at the same time case, we also bind the two servers' sockets to listen to 
  the same port.  This is ok because tcp connection require both endpoints to define the connection, and it would 
  not accept a udp connection - and vice versa.*/
  if (bind(sd, (struct sockaddr *)&sad, sizeof(sad)) < 0) 
  {
    fprintf(stderr,"bind failed\n");
    exit(1);
  }

  /* Specify size of request queue */
  /* we only call the listen function if we're running as a tcp server*/
  if( bIsTCP && listen(sd, QLEN) < 0 ) {
    fprintf(stderr,"listen failed\n");
    exit(1);
  }

  bool bExit = false;

  /* Main server loop for tcp - accept and handle requests */
  while (!bExit) 
  { /*bIsTCP will not change once defined from the beginning*/
    alen = sizeof(cad);
    if ( (sd2=accept(sd, (struct sockaddr *)&cad, &alen)) < 0) 
    {
      fprintf(stderr, "accept() failed\n");
      exit(1);
    }
    //iRecvdp=0; 
    //iEchoedp=0;
    visits++;

    sprintf(buffer,"This server has been contacted %d time%s\n",
      visits,visits==1?".":"s.");

    int    result;
    fd_set readset;

    // poll for data to come in on the socket we are listening on?
    do {
       FD_ZERO(&readset);
       FD_SET(sd2, &readset);
       result = select(sd2 + 1, &readset, NULL, NULL, NULL);
    } while (result == -1 && errno == EINTR);
    
    cout << "Socket Data detected!" << endl;

    n = recv(sd2, buffer, sizeof(buffer), 0); /*recieves from client*/
    
    lRecvd=(long)n;
    lEchoed=0l;

    while (n > 0) 
    {
      //iRecvdp++; 
      //iEchoedp++;
      //write(1, "Client says:\n", 13); /* write it out to screen */
      //write(1, buffer, n);
      // message format: MESSAGE ID (1 byte) + Hex string MAC ADDRESS (17 BYTES) XX:XX:XX:XX:XX:XX + Extra data as needed

      char* ptr = buffer;
      char MessageID = buffer[0];
      ptr += sizeof(MessageID);

      char MACAddr[18];
      memcpy(MACAddr, ptr, 17);
      ptr += 17;
      MACAddr[17]= '\0';
      string MACAddrStr = MACAddr;

      cout << "MSGID: <" << (unsigned int)MessageID << "> MAC: " << MACAddrStr << endl;

      switch( MessageID )
      {
        case CL_REQ_HI_SCORE:
        {
          char ScoreBuff[4];
          int Score = HighScores.GetHiScore( MACAddrStr );
          
          memcpy( ScoreBuff, &Score, sizeof(Score) );

          lEchoed += (long)send(sd2, ScoreBuff, sizeof(Score), 0);
          cout << "Sent High Score: " << Score << endl;
          break;
        }
        case CL_SEND_SCORE:
        {
          int ReceivedScore;
          memcpy( &ReceivedScore, ptr, sizeof(ReceivedScore) );

          HiScore hiscore;
          hiscore.Score = ReceivedScore;
          //hiscore.Name = ReceivedName;

          cout << "Recieved Score: " << hiscore.Score << endl;

          HighScores.UpdateHiScoreList( MACAddrStr, hiscore );
          //lEchoed+=(long)send(sd2, buffer, size , 0);
          break;
        }
        case CL_REQ_HI_SCORE_LIST:
        {
          char buffer[1024];
          vector<int> List;
          int StartingRank;
          HighScores.GetRelativeHighScoreList( MACAddrStr, List, StartingRank );
          char* ptr = buffer;

          cout << "High Score Relative List Req" << endl;

          memcpy(ptr, &StartingRank, sizeof(StartingRank));
          ptr += sizeof(StartingRank);

          for( unsigned int i=0; i < List.size(); i++ )
          {
            cout << StartingRank++ << ". " << List[i] << endl;
            memcpy(ptr, &(List[i]), sizeof(int));
            ptr+=sizeof(int);
          }


          lEchoed += (long)send(sd2, buffer, ptr-buffer, 0);
          break;
        }
        case CL_KILL_SERVER:
        {
          // TODO: check my MAC address for authorization
          cout << "SERVER KILLED REMOTELY" << endl;
          bExit = true;
          break;
        }
        case CL_REQ_RANK:
        {
          bool bTied;
          int  Rank = HighScores.ReturnRank( MACAddrStr, bTied );
          int size = 0;
          char RankBuff[64];

          memcpy(&RankBuff[size], &Rank, sizeof(Rank));
          size += sizeof(Rank);

          if( bTied )
          {
            RankBuff[size] = static_cast<char>(bTied);
            size += sizeof(char);
          }

          lEchoed += (long)send(sd2, &RankBuff, size, 0);
   
          cout << "Rank Request. Returned: " << Rank << endl;
          break;
        }
        case CL_REQ_RANK_OF_THIS_SCORE:
        {
          int Score;
          memcpy( &Score, ptr, sizeof(Score) );

          bool bTied;
          int  Rank = HighScores.ReturnRank( Score, bTied );
          int size = 0;
          char RankBuff[64];

          memcpy(&RankBuff[size], &Rank, sizeof(Rank));
          size += sizeof(Rank);

          if( bTied )
          {
            RankBuff[size] = static_cast<char>(bTied);
            size += sizeof(char);
          }

          lEchoed += (long)send(sd2, &RankBuff, size, 0);

          cout << "Rank Request from Score. Returned: " << Rank << endl;
          break;
        }
        case CL_VERSION_CHECK:
        {
          unsigned int Version;
          unsigned int VersionInfo;
          memcpy( &Version, ptr, sizeof(Version) );

          if( Version >= MINIMUM_CLIENT_VER )
          {
            VersionInfo = CURRENT_CLIENT_VER;
          }
          else
          {
            VersionInfo = 0; // client is too out of date, reject it
          }
          lEchoed += (long)send(sd2, &VersionInfo, sizeof(VersionInfo), 0);

          cout << "Version Check Reported by Client: " << hex << Version << endl;
          cout << "Current Version: " << hex << CURRENT_CLIENT_VER << endl;
          cout << (VersionInfo > 0?"Client Allowed.":"Client REJECTED!") << endl << dec;
          break;
        }

        case CL_REQ_WORDS:
        {
          int RequestedNumber = *ptr;

          char sendBuffer[1024];
          char* sptr = sendBuffer;

          cout << "Word(s) sent: ";

          for( int i = 0; i < RequestedNumber; i++ )
          {
            // pick a random index into our word list
            int randIndex = (int)((double)rand() / RAND_MAX * wordVector.size());

            memcpy(sptr, wordVector[randIndex].c_str(), wordVector[randIndex].length()+1);
            sptr += wordVector[randIndex].length() + 1;

             cout << "\""<< wordVector[randIndex] << "'\" ";
          }
          cout << endl;
          lEchoed += (long)send(sd2, sendBuffer, sptr-sendBuffer, 0);

          break;
        }
        default:
          break;
      }
      //if( 
      
      //printf("delaying response\n");
      //sleep(5);
      //printf("response sent\n");

//  while(!kbhit())
//    puts("Press a key!");
//  printf("You pressed '%c'!\n", getchar());

      /////////////lEchoed+=(long)send(sd2,buffer,n,0); /*echoed back to client*/

      n = recv(sd2, buffer, sizeof(buffer), 0); /*see if there's more*/
      lRecvd+=(long)n;
    }
    /*output statistics*/
    printf("\n\nNumber of data bits received: %ld\n",lRecvd);
    printf("Number of data bits sent: %ld\n",lEchoed);
    //printf("Number of segments received: %d\n",iRecvdp);
    //printf("Number of segments echoed: %d\n",iEchoedp);

    // Segments indicated may not be the true number of segments received and echoed.
    // This is because we counting the number of calls of send(...) and recv(...), which
    // also depends on the application buffer size, just the tcp buffer size and MTU.

    cout << "-------------------------------" << endl;
    close(sd2);
  }

  //exit(0);
  return 0;
}

// check for and lock the process ID file we create //
bool CheckForAnotherServerInstance()
{
  ofstream os(PID_FILE);

  int pid = getpid();
  if( pid > 0 )
  {
    os << pid;
  }
 
  os.close();

  int fd;
  struct flock fl;

  fd = open(PID_FILE, O_RDWR);
  if(fd == -1)
  {
    //cout << "fd failed." << endl;
    return false;
  }

  fl.l_type   = F_WRLCK;  /* F_RDLCK, F_WRLCK, F_UNLCK    */
  fl.l_whence = SEEK_SET; /* SEEK_SET, SEEK_CUR, SEEK_END */
  fl.l_start  = 0;        /* Offset from l_whence         */
  fl.l_len    = 0;        /* length, 0 = to EOF           */
  fl.l_pid    = getpid(); /* our PID                      */

  // try to create a file lock
  if( fcntl(fd, F_SETLK, &fl) == -1)   /* F_GETLK, F_SETLK, F_SETLKW */
  {
    // we failed to create a file lock, meaning it's already locked //
    if( errno == EACCES || errno == EAGAIN)
    {
      return true;
    }
  }

  return false;
}
