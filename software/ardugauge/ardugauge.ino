#include "Arduino.h"
#include "speeduino_data.h"
#include "Pages.h"
#include "Comms.h"
#include "ardugauge.h"

void setup()
{
  pinMode(2, INPUT_PULLUP);
  pinMode(0, INPUT);
  
  initDisplay();

  Serial.begin(115200);
}

void loop()
{
  static uint8_t pageNum = PAGE_AUTO;
  static uint8_t engineStatus = ENGINE_OFF;
  static uint32_t engineIdling = 0;
  static uint8_t oilTemp = getOilTemp();
  static bool ecuConnected = false;
  static bool shiftLight = false;

  // Button operation
  static uint32_t btnPressed = 0;
  if(!digitalRead(2))
  {
    // Button is pressed
    if(btnPressed == 0)
      btnPressed = millis();
  }
  else
  {
    // Button is not pressed...
    if(btnPressed && (millis() - btnPressed > 100))
    {
      // ...but it was
      pageNum++;
      if(pageNum > PAGE_AFRGRAPH)
         pageNum = PAGE_AUTO;
    }

    btnPressed = 0;
  }

  // Serial operation, frequency based request
  static uint32_t lastUpdate = millis();
  if (millis() - lastUpdate > SERIAL_UPDATE_MS)
  {
    // Update Oil temperature
    oilTemp = getOilTemp();
    
    ecuConnected = requestData(50);
    lastUpdate = millis();
  }

  // TEST //
  #ifdef TEST
    ecuConnected = true;
    pageNum = PAGE_AUTO;
    bitSet(buffer[SPEEDUINO_ENGINE_BITFIELD], SPEEDUINO_ENGINE_RUNNING_BIT);
    bitSet(buffer[SPEEDUINO_ENGINE_BITFIELD], SPEEDUINO_ENGINE_WARMUP_BIT);
    #define TEST_RPM 1200
    buffer[SPEEDUINO_RPM_WORD] = TEST_RPM % 256;
    buffer[SPEEDUINO_RPM_WORD+1] = TEST_RPM / 256;
    //shiftLight = true;
    #define TEST_MAP 50
    buffer[SPEEDUINO_MAP_WORD] = TEST_MAP % 256;
    buffer[SPEEDUINO_MAP_WORD+1] = TEST_MAP / 256;
    buffer[SPEEDUINO_CURRENTVE_BYTE] = 155;
    buffer[SPEEDUINO_BARO_BYTE] = 100;
    buffer[SPEEDUINO_CLT_BYTE] = 30;
    buffer[SPEEDUINO_IAT_BYTE] = 25;
    //oilTemp = 100;
    buffer[SPEEDUINO_WUECORR_BYTE] = 100;
    buffer[SPEEDUINO_AFRTARGET_BYTE] = 145;
    buffer[SPEEDUINO_AFR_BYTE] = 147;
    buffer[SPEEDUINO_VOLTAGE_BYTE] = 120;
    buffer[SPEEDUINO_EGOCORR_BYTE] = 100;
    buffer[SPEEDUINO_TPSDOT_BYTE] = 0;
    buffer[SPEEDUINO_AECORR_BYTE] = 100;
    buffer[SPEEDUINO_GAMMAE_BYTE] = 100;
    buffer[SPEEDUINO_ASECORR_BYTE] = 105;
  #endif
  
  // Check/update engine status
  if(getBit(SPEEDUINO_ENGINE_BITFIELD, SPEEDUINO_ENGINE_RUNNING_BIT))
  {
	  // Running
	  if(engineStatus == ENGINE_OFF)
	  {
		  if(getBit(SPEEDUINO_ENGINE_BITFIELD, SPEEDUINO_ENGINE_WARMUP_BIT))
			  engineStatus = ENGINE_WARMUP; // Off -> Warmup
		  else
			  engineStatus = ENGINE_RUN; // Off -> Run
	  }
	  else if(engineStatus == ENGINE_WARMUP)
	  {
		  if(!getBit(SPEEDUINO_ENGINE_BITFIELD, SPEEDUINO_ENGINE_WARMUP_BIT))
			  engineStatus = ENGINE_RUN; // Warmup -> Run
	  }
	  
	  // Check idle condition
	  if(engineIdling == 0 && getWord(SPEEDUINO_RPM_WORD) < IDLE_RPM_ENTER)
		  engineIdling = millis();
	  else if(getWord(SPEEDUINO_RPM_WORD) > IDLE_RPM_EXIT)
		  engineIdling = 0; 

    #ifndef TEST
      // Check if shift light must be lit
      if(!shiftLight && (getBit(SPEEDUINO_SPARK_BITFIELD, SPEEDUINO_SPARK_SOFTLIMIT_BIT) || getBit(SPEEDUINO_SPARK_BITFIELD, SPEEDUINO_SPARK_HARDLIMIT_BIT) || getWord(SPEEDUINO_RPM_WORD) > SHIFTLIGHT_ON_RPM))
        shiftLight = true;
      else if(shiftLight && (!getBit(SPEEDUINO_SPARK_BITFIELD, SPEEDUINO_SPARK_SOFTLIMIT_BIT) && !getBit(SPEEDUINO_SPARK_BITFIELD, SPEEDUINO_SPARK_HARDLIMIT_BIT) && getWord(SPEEDUINO_RPM_WORD) < SHIFTLIGHT_OFF_RPM))
        shiftLight = false;
    #endif
  }
  else
  {
	  // Off
	  engineStatus = ENGINE_OFF;
  }  

  startPage(shiftLight);

  if(!ecuConnected)
    pageNoECU();
  else 
  {  
    switch(pageNum)
    {
      case PAGE_AUTO:
        switch(engineStatus)
        {
          case ENGINE_OFF:
            topRow_OFF(oilTemp);
            bottomRow_OFF();
            break;
          case ENGINE_WARMUP:
            topRow_WARMUP(oilTemp);
            bottomRow_ON();
            break;
          case ENGINE_RUN:
            if(engineIdling && ((millis() - engineIdling) > IDLE_DELAY_MS))
              topRow_IDLE();
            else
              topRow_DRIVE(oilTemp);
            
            bottomRow_ON();
            break;
          default:
            break;
        }
        break;
      case PAGE_RPM_MAP:
        pageTuning();
        break;
      case PAGE_AE:
        //pageAE();
        break;
      case PAGE_AFRGRAPH:
        pageAFRGraph();
        break;
      default:
        break;
    }
  }

  endPage();
}
