//=================================================================================================================
// The TeleInfo is a class that stores the data retrieved from the teleinfo frames, and displays them on a LCD
// Various displays are available : 
//     + Instant values : Actual color, actual mode, instant current, actual color counter, instant power
//     + To morrow color : Actual color and to morrow color when known
//     + A display for each color (blue, white, red) and both modes (HC, HP)
//
// The various displays can be directly selected by pressing the buttons placed below the LCD, so button handling
// routine that generates interrupts is also part of this class
//
//=================================================================================================================
#include "Teleinfo.h"

//=================================================================================================================
// Basic constructor
//=================================================================================================================
TeleInfo::TeleInfo(String version)
{
  cptSerial = new SoftwareSerial(32, 33);

  //Serial1.begin(1200,SERIAL_7E1);
  cptSerial->begin(1200);
  
  pgmVersion = version;
  // variables initializations
  ADCO = "270622224349";
  OPTARIF = "----";
  ISOUSC = 0;
  HCHC = 0L;  // compteur Heures Creuses en W
  HCHP = 0L;  // compteur Heures Pleines en W
  HBASE = 0L;  // compteur Base en W
  PTEC = "----";    // Régime actuel : HPJB, HCJB, HPJW, HCJW, HPJR, HCJR
  HHPHC = '-';
  IINST = 0;        // intensité instantanée en A
  IMAX = 0;         // intensité maxi en A
  PAPP = 0;         // puissance apparente en VA
  MOTDETAT = "------";
}

//=================================================================================================================
// Capture des trames de Teleinfo
//=================================================================================================================
boolean TeleInfo::readTeleInfo()
{
#define startFrame 0x02
#define endFrame 0x03
#define startLine 0x0A
#define endLine 0x0D
#define maxFrameLen 280

  int comptChar=0; // variable de comptage des caractères reçus 
  char charIn=0; // variable de mémorisation du caractère courant en réception

  char bufferTeleinfo[21] = "";
  int bufferLen = 0;
  int checkSum;

  int sequenceNumnber= 0;    // number of information group

 if (cptSerial->available()) {
  //--- wait for starting frame character 
  while (charIn != startFrame)
  { // "Start Text" STX (002 h) is the beginning of the frame
    if (cptSerial->available()) //Serial1.available())
      charIn = cptSerial->read() & 0x7F; //Serial1.read(); // Serial.read() vide buffer au fur et à mesure
  } // fin while (tant que) pas caractère 0x02
  //  while (charIn != endFrame and comptChar<=maxFrameLen)
  while (charIn != endFrame)
  { // tant que des octets sont disponibles en lecture : on lit les caractères
    if (cptSerial->available()) //Serial1.available())
    {
      charIn = cptSerial->read() & 0x7F; //Serial1.read();
      // incrémente le compteur de caractère reçus
      comptChar++;
      if (charIn == startLine)
        bufferLen = 0;
      bufferTeleinfo[bufferLen] = charIn;
      // on utilise une limite max pour éviter String trop long en cas erreur réception
      // ajoute le caractère reçu au String pour les N premiers caractères
      if (charIn == endLine)
      {
        checkSum = bufferTeleinfo[bufferLen -1];
        if (chksum(bufferTeleinfo, bufferLen) == checkSum)
        {// we clear the 1st character
          strncpy(&bufferTeleinfo[0], &bufferTeleinfo[1], bufferLen -3);
          bufferTeleinfo[bufferLen -3] =  0x00;
#ifdef debug
  Serial.println(bufferTeleinfo);
#endif
          sequenceNumnber++;
          if (! handleBuffer(bufferTeleinfo, sequenceNumnber))
          {
            Serial.println(F("Sequence error ..."));
            return false;
          }
          if (bufferTeleinfo[0]=='B') // Cas forfait base : on décale de 1 / HPHC
            sequenceNumnber++;        
        }
        else
        {
          Serial.println(F("Checksum error ..."));
          return false;
        }
      }
      else
        bufferLen++;
    }
    if (comptChar > maxFrameLen)
    {
      Serial.println(F("Overflow error ..."));
      return false;
    }
  }
  return true;
 } // char not available
 return false;
}

//=================================================================================================================
// Frame parsing
//=================================================================================================================
//void handleBuffer(char *bufferTeleinfo, uint8_t len)
boolean TeleInfo::handleBuffer(char *bufferTeleinfo, int sequenceNumnber)
{
  // create a pointer to the first char after the space
  char* resultString = strchr(bufferTeleinfo,' ') + 1;
  boolean sequenceIsOK;

  switch(sequenceNumnber)
  {
  case 1:
    if (sequenceIsOK = bufferTeleinfo[0]=='A')
      ADCO = String(resultString);
    break;
  case 2:
    if (sequenceIsOK = bufferTeleinfo[0]=='O')
      OPTARIF = String(resultString);
    break;
  case 3:
    if (sequenceIsOK = bufferTeleinfo[1]=='S')
      ISOUSC = atol(resultString);
    break;
  case 4:
    if (sequenceIsOK = bufferTeleinfo[3]=='C') {
      HCHC = atol(resultString);
      HeureCreuse=HCHC;
      }
    if (sequenceIsOK = bufferTeleinfo[0]=='B') {
      HBASE = atol(resultString);
      Base=HBASE;
      }
    break;
  case 5:
    if (sequenceIsOK = bufferTeleinfo[3]=='P')
      HCHP = atol(resultString);
      HeurePleine=HCHP;
    break;
  case 6:
    if (sequenceIsOK = bufferTeleinfo[1]=='T')
      PTEC = String(resultString);
    break;
  case 7:
    if (sequenceIsOK = bufferTeleinfo[1]=='I')
      IINST =atol(resultString);
    break;
  case 8:
    if (sequenceIsOK = bufferTeleinfo[1]=='M')
      IMAX =atol(resultString);
    break;
  case 9:
    if (sequenceIsOK = bufferTeleinfo[1]=='A')
      PAPP =atol(resultString);
    break;
  case 10:
    if (sequenceIsOK = bufferTeleinfo[1]=='H')
      HHPHC = resultString[0];
    break;
  case 11:
    if (sequenceIsOK = bufferTeleinfo[1]=='O')
      MOTDETAT = String(resultString);
    break;
  }

#ifdef debug
  if(!sequenceIsOK)
  {
    Serial.print(F("Out of sequence ..."));
    Serial.println(bufferTeleinfo);
  }
#endif
  return sequenceIsOK;
}

//=================================================================================================================
// Calculates teleinfo Checksum
//=================================================================================================================
char TeleInfo::chksum(char *buff, uint8_t len)
{
  int i;
  char sum = 0;
  for (i=1; i<(len-2); i++) 
    sum = sum + buff[i];
  sum = (sum & 0x3F) + 0x20;
  return(sum);
}

//=================================================================================================================
// This function displays the TeleInfo Internal counters
// It's usefull for debug purpose
//=================================================================================================================
void TeleInfo::displayTeleInfo()
{  /*
ADCO 270622224349 B
 OPTARIF HC.. <
 ISOUSC 30 9
(
 HCHC 014460852 $
 HCHP 012506372 -
ou
 BASE xxxx
)
 PTEC HP..  
 IINST 002 Y
 IMAX 035 G
 PAPP 00520 (
 HHPHC C .
 MOTDETAT 000000 B
 */
#ifdef debug
  Serial.print(F(" "));
  Serial.println();
  Serial.print(F("ADCO "));
  Serial.println(ADCO);
  Serial.print(F("OPTARIF "));
  Serial.println(OPTARIF);
  Serial.print(F("ISOUSC "));
  Serial.println(ISOUSC);
  Serial.print(F("HCHC "));
  Serial.println(HCHC);
  Serial.print(F("HCHP "));
  Serial.println(HCHP);
  Serial.print(F("HBASE "));
  Serial.println(HBASE);
  Serial.print(F("PTEC "));
  Serial.println(PTEC);
  Serial.print(F("IINST "));
  Serial.println(IINST);
  Serial.print(F("IMAX "));
  Serial.println(IMAX);
  Serial.print(F("PAPP "));
  Serial.println(PAPP);
  Serial.print(F("HHPHC "));
  Serial.println(HHPHC);
  Serial.print(F("MOTDETAT "));
  Serial.println(MOTDETAT);
#endif
}

//=================================================================================================================
// Send results to zibase PHP server
//=================================================================================================================
/*
boolean TeleInfo::recordTeleInfoOnMySQLServer()
{
  char remoteServer[]="xx.xx.xx.xx"; // Internet address of your server
  int port = 45001;  // port of the http server

  char httpRequest[180];
  char hostString[25];
  char ptec[5];
  char optarif[5];

  // build HostString
  sprintf(hostString, "Host : %s", remoteServer);

  PTEC.toCharArray(ptec,5);
  OPTARIF.toCharArray(optarif,5);

  // zibase01/teleInfo.php?HCJB=10828&HPJB=7345&HCJW=0&HPJW=0&HCJR=0&HPJR=0&PTEC=HPJB&DEMAIN=----&IINST=2&IMAX=30&PAPP=430
  sprintf(httpRequest,"GET /zibase01/teleInfo.php?OPTARIF=%s&HCHC=%lu&HCHP=%lu&PTEC=%s&IINST=%u&IMAX=%u&PAPP=%u HTTP/1.1"
  ,optarif,HCHC,HCHP,ptec,IINST,IMAX,PAPP);
#ifdef debug
  Serial.println(hostString);
  Serial.println(httpRequest);
//  return true;
#endif

  EthernetClient client;
  client.connect(remoteServer, port);
  if (client.connected()) 
  {
    // Make a HTTP request:
    client.println(httpRequest);
    client.println(hostString);
    client.println(F("Accept : text/html"));
    client.println(F("User-Agent : ARDUINO, TeleInfo"));
    client.println(F("Connection: close"));
    client.println();
    // receiving data back from the HTTP server
    client.stop();
    Serial.println(F("Frame sent ..."));
    return true;
  }
  else 
  {
    // if you didn't get a connection to the server:
    client.flush();
    Serial.println(F("connection failed"));
    return false;
  }
}

*/
