#include <ELClient.h>
#include <ELClientCmd.h>
#include <ELClientMqtt.h>

#include <SPI.h> //SPI.h must be included as DMD is written by SPI (the IDE complains otherwise)
#include <DMD.h>        //

  //Fire up the DMD library as dmd
#define DISPLAYS_ACROSS 3
#define DISPLAYS_DOWN 1
DMD dmd(DISPLAYS_ACROSS, DISPLAYS_DOWN);
  /*--------------------------------------------------------------------------------------
    Interrupt handler for Timer1 (TimerOne) driven DMD refresh scanning, this gets
    called at the period set in Timer1.initialize();
  --------------------------------------------------------------------------------------*/
void ScanDMD()
  { 
    dmd.scanDisplayBySPI();
  }

#include "RusSystemFont5x7.h"
#include "D:\\Mozok\\Documents\\Arduino\\LedDisplayESP2\\RusArial14.h"

static uint32_t last;
long timer;
bool flagM = false;

//Declare some helper function
void modeSwitch(char* str);
void printImg(char* buf);
void drawImg(const int x, const int y );


///
///esp-link init block
///
// Initialize a connection to esp-link using the normal hardware serial port both for
// SLIP and for debug messages.
ELClient esp(&Serial, &Serial);

// Initialize CMD client (for GetTime)
ELClientCmd cmd(&esp);

// Initialize the MQTT client
ELClientMqtt mqtt(&esp);

// Callback made from esp-link to notify of wifi status changes
// Here we just print something out for grins
void wifiCb(void* response) {
  ELClientResponse *res = (ELClientResponse*)response;
  if (res->argc() == 1) {
    uint8_t status;
    res->popArg(&status, 1);

    if(status == STATION_GOT_IP) {
      Serial.println("WIFI CONNECTED");
    } else {
      Serial.print("WIFI NOT READY: ");
      Serial.println(status);
    }
  }
}

bool connected;

// Callback when MQTT is connected
void mqttConnected(void* response) {
  Serial.println("MQTT connected!");
  //mqtt.subscribe("AS/FirstDoor/server_response");
 // mqtt.subscribe("AS/FirstDoor/server_data");
  mqtt.subscribe("/ipanel/command");
  
  connected = true;
}

// Callback when MQTT is disconnected
void mqttDisconnected(void* response) {
  Serial.println("MQTT disconnected");
  connected = false;
}

// Callback when an MQTT message arrives for one of our subscriptions
void mqttData(void* response) {
  ELClientResponse *res = (ELClientResponse *)response;

  Serial.print("Received: topic=");
  String topic = res->popString();
  Serial.println(topic);

  Serial.print("data=");

  char data[res->argLen()+1];
  res->popChar(data);
  
  //String data = res->popString();
  Serial.println(data);

  modeSwitch(data);
    
}

void mqttPublished(void* response) {
  Serial.println("MQTT published");
}

void setup() {
  Serial.begin(115200);
  Serial.println("EL-Client starting!");

   // Sync-up with esp-link, this is required at the start of any sketch and initializes the
  // callbacks to the wifi status change callback. The callback gets called with the initial
  // status right after Sync() below completes.
  esp.wifiCb.attach(wifiCb); // wifi status change callback, optional (delete if not desired)
  bool ok;
  do {
    ok = esp.Sync();      // sync up with esp-link, blocks for up to 2 seconds
    if (!ok) Serial.println("EL-Client sync failed!");
  } while(!ok);
  Serial.println("EL-Client synced!");

  // Set-up callbacks for events and initialize with es-link.
  mqtt.connectedCb.attach(mqttConnected);
  mqtt.disconnectedCb.attach(mqttDisconnected);
  mqtt.publishedCb.attach(mqttPublished);
  mqtt.dataCb.attach(mqttData);
  mqtt.setup();

  Serial.println("EL-MQTT ready");

  //LED panel setup
  //clear/init the DMD pixels held in RAM
  dmd.clearScreen( true );   //true is normal (all pixels off), false is negative (all pixels on)
//      dmd.selectFont(SystemFont5x7);
      
}

void loop() {
  esp.Process();


    if((millis()- last) > 3)
    {
      ScanDMD();
    }

    if(flagM && ((timer + 30) < millis()))
    {
      
      dmd.stepMarquee(-1,0);
      
      timer = millis();
    }
}

void modeSwitch(char* str)
{
  char * pch;
  pch = strtok(str, "#");

  //Serial.println(pch);  //debug
//  while (pch!= NULL)
//      {
//        Serial.println(pch);
//        pch = strtok(NULL, "#");
//      }
     
  int mode = atoi(pch);
 
  switch (mode)
  {
    case 1:
    {
      dmd.selectFont(Arial_14);
      dmd.clearScreen( true );
      
      pch = strtok(NULL, "#");
      //Serial.println(pch);  //debug
      dmd.drawString(0,0,pch,strlen(pch), GRAPHICS_NORMAL);

      flagM = false;
      
      break;
    }
    case 2:
    {
      dmd.selectFont(SystemFont5x7);
      dmd.clearScreen( true );

      
      pch = strtok(NULL, "#");
      //Serial.println(pch);  //debug
      dmd.drawString(0,0,pch,strlen(pch), GRAPHICS_NORMAL);

      pch = strtok(NULL, "#");
      //Serial.println(pch);  //debug
      dmd.drawString(0,8,pch,strlen(pch), GRAPHICS_NORMAL);

      flagM = false;
      
      break;
    }
    case 3:
    {
      dmd.selectFont(Arial_14);
      dmd.clearScreen( true );

      pch = strtok(NULL, "#");
      dmd.drawMarquee(pch, strlen(pch), 0, 0);

      timer = millis();
      flagM = true;

      break;
    }
    case 4:
    {
      dmd.clearScreen( true );
      
      pch = strtok(NULL, "#\n");
      
      printImg(pch);
      flagM = false;
      
      break;
    }
    case 5:
    {
      dmd.clearScreen(true);

      drawImg(0,0);
      flagM = false;
      break;
    }
    case 6:
    {
      
      drawImg(32,0);
      flagM = false;
      break;
    }
  }

return 0; 
}

void printImg(char* buf)
{
//  Serial.println(buf);  //debug
//  Serial.println(strlen(buf));  //debug
  int i = 0;
  int len = strlen(buf); 
  
  for (byte y = 0; y < DMD_PIXELS_DOWN; y++) {
    for (byte x = 0; x < DMD_PIXELS_ACROSS*DISPLAYS_ACROSS; x++) {
      if (buf[i] == '1' && i <= len) {
//        Serial.print("i");  //debug
//        Serial.println(i);  //debug
        dmd.writePixel(x, y, GRAPHICS_NORMAL, true);
        
      }
      i++;
    }
  }
}

const static uint8_t TestImg[] = 
   {0x00, 0x00, 0x00, 0x40, 0x40, 0xC0, 0xC0, 0xC0, 0x60, 0x20, 0x20, 0x30, 0x10, 0x18, 0x08, 0x0C, 0x04, 0x04, 0x0C, 0x18, 0x30,  0x18, 0x08, 0x0D, 0x05, 0x06, 0x06, 0x06, 0x06, 0x00, 0x00, 0x00,  
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x0E, 0x18, 0x10, 0x20, 0x60, 0x40, 0x40, 0x60, 0x20, 0x20, 0x30, 0x10, 0x20, 0x60, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

const static uint8_t TestImg2[] = 
{0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x08,0x1E,0x68,0x1E,0x08,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0xE0,0xB0,0xE0,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 
0x00,0x00,0x20,0x78,0x2C,0x78,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x00,0x00,0x10,0x38,0x6C,0x38,0x10,0x00};

void drawImg(const int x, const int y)
{
  uint8_t height = 16;
  uint8_t width = 32;
  uint8_t bytes = (height+7)/8;

  //draw Image (copy from dmd.drawChar)
  for (uint8_t j = 0; j < width; j++) //Width
  {
    for (uint8_t i = bytes - 1; i<254; i--) // Vertical Bytes
    {
      uint8_t data;
      if( x == 0){data = TestImg[j+(i*width)];} //Test HARDCODE Img
      else {data = TestImg2[j+(i*width)];}  //Test HARDCODE Img

      int offset = (i*8);
      if((i == bytes - 1) && bytes > 1)
      {
        offset = height - 8;
      }
      for (uint8_t k = 0; k<8; k++) // Vertical bits
      {
        if((offset+k >= i*8) && (offset+k <= height))
        {
          if (data & (1 << k))
          {
            dmd.writePixel(x + j, y + offset+k, GRAPHICS_NORMAL, true);
          }
          else 
          {
            dmd.writePixel(x + j, y + offset+k, GRAPHICS_NORMAL, false);
          }
        }
      }
    }
  }
}