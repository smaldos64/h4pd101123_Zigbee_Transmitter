#define Display_Attached
#define Use_External_Interrupt
#undef Use_Uart_Communication

#include <Arduino.h>

#include <Ticker.h>

#include "avr_debugger.h"
#include "avr8-stub.h"
#include "app_api.h"

#ifdef Display_Attached
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_RESET LED_BUILTIN  //4
Adafruit_SSD1306 display(OLED_RESET);

	#if (SSD1306_LCDHEIGHT != 64)
	#error("Height incorrect, please fix Adafruit_SSD1306.h!");
	#endif
#endif

#define USE_SERIAL Serial2

#include <XBee.h>

#define LED 9
#define ExternalInterruptNumber 5
#define ExternalInterruptPin 6

//static volatile bool External_Interrupt_Occured = false;
static volatile bool Timer_Interrupt_Occured = false;
static volatile uint32_t Interrupt_Counter = 0;
//=======================================================================
void changeState()
{
	digitalWrite(LED, !(digitalRead(LED)));  //Invert Current State of LED
	Timer_Interrupt_Occured = true;
	Interrupt_Counter++;
}

Ticker Blinker(changeState, 1000);

/*
This example is for Series 2 XBee
Receives a ZB RX packet and sets a PWM value based on packet data.
Error led is flashed if an unexpected packet is received
*/

// XBee xbee = XBee();
// XBeeResponse response = XBeeResponse();
// // create reusable response objects for responses we expect to handle 
// ZBRxResponse rx = ZBRxResponse();
// ModemStatusResponse msr = ModemStatusResponse();

int statusLed = 13;
int errorLed = 12;
int dataLed = 13;

XBee xbee = XBee();

#define AnalogPinToUse 8
uint16_t analogPinValue = 0;
uint8_t payload[] = { 0, 0 };
char payloadCharArray[7];

// SH + SL Address of receiving XBee
XBeeAddress64 addr64 = XBeeAddress64(0x0013a200, 0x4155B9Bf);
ZBTxRequest zbTx = ZBTxRequest(addr64, payload, sizeof(payload));
ZBTxStatusResponse txStatus = ZBTxStatusResponse();


int my_putc(char c, FILE* t)
{
	USE_SERIAL.write(c);
	return 0;
}

void WriteInitText()
{
	#ifdef Display_Attached
	display.clearDisplay();
	display.setTextSize(1);
	display.setCursor(0, 16);
	display.println("Program Start !!!");
	
	display.setCursor(0, 32);
	display.println("Venter - Zigbee - ko");

	display.setCursor(0, 48);
	display.println("Indtast fra UART");

	display.display();
#endif
	printf("\nNu er vi klar\n");
}

void WriteText(char *StringToDisplay)
{
#ifdef Display_Attached
	// char DisplayString[20] = "Adresse : 0x";
	// char *DisplayPointer = (char*)malloc(1);

	//strcat(DisplayString, (const char *)ThisAddressStruct.Address_String);
	display.clearDisplay();
	display.setTextSize(1);
	display.setCursor(0, 16);
	display.println("Tekst fra Uart:");
	
	display.setCursor(0, 32);
	display.println(StringToDisplay);

	display.display();

	//free(DisplayPointer);
#endif
	printf("\nNu udskrevet til Display !!!\n");
	printf("Tekst modtaget fra Uart : ");
	printf(StringToDisplay);
	printf("\n");
}

void flashLed(int pin, int times, int wait) {
    
    for (int i = 0; i < times; i++) {
      digitalWrite(pin, HIGH);
      delay(wait);
      digitalWrite(pin, LOW);
      
      if (i + 1 < times) {
        delay(wait);
      }
    }
}

// void HandleExternalInterrupt()
// {
//   External_Interrupt_Occured = true;
// }

// void Setup_External_Interrupt()
// {
//   pinMode(ExternalInterruptPin, INPUT_PULLUP);
//   attachInterrupt(ExternalInterruptNumber, HandleExternalInterrupt, FALLING);
// }

// ISR(INT4_vect, ISR_BLOCK)
// {
//   External_Interrupt_Occured = true;
// }

void setup() {
  USE_SERIAL.begin(115200);
  debug_init();
  delay(3000);

  fdevopen(&my_putc, 0);    // OBS !!! Har omdirigerer vi printf til at bruge UART gennem den angivne funktion : my_putc. 
                            // Det vil sige, at alle printf sætninger vil blive omdirigeret til UART'en.
  printf("Nu starter vi !!!\n");
  
#ifdef Display_Attached
	display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
	display.clearDisplay();
	display.setTextColor(WHITE);
	WriteInitText();
#endif

  pinMode(statusLed, OUTPUT);
  pinMode(errorLed, OUTPUT);
  pinMode(dataLed,  OUTPUT);
  
  //Setup_External_Interrupt();

  // start serial
  Serial.begin(9600);
  xbee.begin(Serial);

  Blinker.start();
}

// continuously reads packets, looking for ZB Receive or Modem Status
void loop() {
  char analogReadBuffer[54];
  volatile char IncomingChar = '0'; // for incoming serial data
  //char IncomingChar = '0';
 
  Blinker.update();
	if (Timer_Interrupt_Occured)
	{
		Timer_Interrupt_Occured = false;
		USE_SERIAL.print("Interrupt Counter : ");
		USE_SERIAL.println(Interrupt_Counter);
  }

  // if (External_Interrupt_Occured)
  // {
  //   External_Interrupt_Occured = false;
  //   USE_SERIAL.println("External Interrupt Occured !!!");
  // }

	if (USE_SERIAL.available() > 0)
  {
    IncomingChar = USE_SERIAL.read();
    if ('1' == IncomingChar) 
    {
      analogPinValue = analogRead(AnalogPinToUse);
      payload[0] = analogPinValue >> 8 & 0xff;
      payload[1] = analogPinValue & 0xff;

      xbee.send(zbTx);

      sprintf(analogReadBuffer, "\nAnalog vaerdi laest paa Port A%d er : 0x%04x\n", AnalogPinToUse, analogPinValue);
      printf(analogReadBuffer);

      sprintf(payloadCharArray, "0x%04x", analogPinValue);
      WriteText(payloadCharArray);
    
      // after sending a tx request, we expect a status response
      // wait up to half second for the status response
      if (xbee.readPacket(500)) 
      {
        // got a response!

        // should be a znet tx status            	
        if (xbee.getResponse().getApiId() == ZB_TX_STATUS_RESPONSE) 
        {
          xbee.getResponse().getZBTxStatusResponse(txStatus);

          // get the delivery status, the fifth byte
          if (txStatus.getDeliveryStatus() == SUCCESS) 
          {
            // success.  time to celebrate
            flashLed(statusLed, 5, 50);
            printf("\nPakke sendt fra Communicator modtaget hos Router !!!\n");
          } 
          else 
          {
            // the remote XBee did not receive our packet. is it powered on?
            flashLed(errorLed, 3, 500);
            printf("\nPakke sendt fra Communicator IKKE modtaget hos Router !!!\n");
          }
        }
      } 
      else if (xbee.getResponse().isError()) 
      {
        //nss.print("Error reading packet.  Error code: ");  
        //nss.println(xbee.getResponse().getErrorCode());
        printf("\nPakke send Error !!!\n");
      } 
      else 
      {
        // local XBee did not provide a timely TX Status Response -- should not happen
        flashLed(errorLed, 2, 50);
        printf("\nZigBee Communicator har ikke leveret en TX Status Response i tide !!!\n");
      }
    }
  }

  //delay(1000);
    
  // xbee.readPacket();
  
  // if (xbee.getResponse().isAvailable()) 
  // {
  //   // got something
  //   if (xbee.getResponse().getApiId() == ZB_RX_RESPONSE) 
  //   {
  //     // got a zb rx packet
      
  //     // now fill our zb rx class
  //     xbee.getResponse().getZBRxResponse(rx);
          
  //     if (rx.getOption() == ZB_PACKET_ACKNOWLEDGED) {
  //         // the sender got an ACK
  //         flashLed(statusLed, 10, 10);
  //     } else 
  //     {
  //         // we got it (obviously) but sender didn't get an ACK
  //         flashLed(errorLed, 2, 20);
  //     }
  //     // set dataLed PWM to value of the first byte in the data
  //     //analogWrite(dataLed, rx.getData(0));  // LTPE
  //     uint8_t NumberOfBytesReceived = rx.getDataLength();
  //     char ZigBeeDataArray[NumberOfBytesReceived];
  //     for (uint8_t Counter = 0; Counter < NumberOfBytesReceived - 1; Counter++)
  //     {
  //       ZigBeeDataArray[Counter] = (char)rx.getData(Counter);
  //     }
  //     printf("\n");
  //     printf(ZigBeeDataArray);
  //     printf("\n");
  //     WriteText(ZigBeeDataArray);
  //   } 
  //   else if (xbee.getResponse().getApiId() == MODEM_STATUS_RESPONSE) 
  //   {
  //     xbee.getResponse().getModemStatusResponse(msr);
  //     // the local XBee sends this response on certain events, like association/dissociation
      
  //     if (msr.getStatus() == ASSOCIATED) 
  //     {
  //       // yay this is great.  flash led
  //       flashLed(statusLed, 10, 10);
  //     } else if (msr.getStatus() == DISASSOCIATED)
  //     {
  //       // this is awful.. flash led to show our discontent
  //       flashLed(errorLed, 10, 10);
  //     } else 
  //     {
  //       // another status
  //       flashLed(statusLed, 5, 10);
  //     } 
  //   } 
  //   else
  //   {
  //     // not something we were expecting
  //     flashLed(errorLed, 1, 25);    
  //   }
  // } else if (xbee.getResponse().isError()) 
  // {
  //   printf("Error reading packet.  Error code: ");  
  //   uint8_t ErrorCode = xbee.getResponse().getErrorCode();
  //   printf((const char *)ErrorCode);
  //   printf("\n");
  //   //printf(xbee.getResponse().getErrorCode());
  // }
}


// static volatile StateMachineStates StateMachineState = STATE_MACHINE_RECEIVING_ADDRESS;
// static volatile bool PrintOutInfo = true;
// static volatile AddressStruct ThisAddressStruct;

// StateMachineStruct *StateMachineStructInRAM_Pointer;
// char *StringPointer;
// uint16_t NumberOfCharsInString;

// int IncomingByte = 0; // for incoming serial data
// char IncomingChar = '0';

// int my_putc(char c, FILE* t)
// {
// 	USE_SERIAL.write(c);
// 	return 0;
// }

// void WriteInitText()
// {
// 	#ifdef Display_Attached
// 	display.clearDisplay();
// 	display.setTextSize(1);
// 	display.setCursor(0, 16);
// 	display.println("Program Start !!!");
	
// 	display.setCursor(0, 32);
// 	display.println("Venter - Bruger");

// 	display.setCursor(0, 48);
// 	display.println("Indtast fra Seriel(P)");

// 	display.display();
// #endif
// 	printf("\nNu er vi klar\n");
// }

// void WriteText()
// {
// #ifdef Display_Attached
// 	char DisplayString[20] = "Adresse : 0x";
// 	char *DisplayPointer = (char*)malloc(1);

// 	strcat(DisplayString, (const char *)ThisAddressStruct.Address_String);
// 	display.clearDisplay();
// 	display.setTextSize(1);
// 	display.setCursor(0, 16);
// 	display.println(DisplayString);
	
// 	display.setCursor(0, 32);
// 	strcpy(DisplayString, "Bit Position : ");
// 	*DisplayPointer = '0' + ThisAddressStruct.Bit_Position;
// 	strcat(DisplayString, DisplayPointer);
// 	display.println(DisplayString);

// 	display.setCursor(0, 48);
// 	strcpy(DisplayString, "Bit State : ");
// 	*DisplayPointer = '0' + ThisAddressStruct.Bit_State;
// 	strcat(DisplayString, DisplayPointer);
// 	display.println(DisplayString);

// 	display.display();

// 	free(DisplayPointer);
// #endif
// 	printf("\nNu udskrevet til Display !!!\n");
// }

// void GoBackToInitState(void)
// {
//   	StateMachineState = STATE_MACHINE_RECEIVING_ADDRESS;
// 	ThisAddressStruct.Address_StringPointer = 0;
// 	PrintOutInfo = true;
// }

// void ChangeStateInStateMachine(void)
// {
//   	uint8_t StateMachineStateInt = (int)StateMachineState;
//   	StateMachineStateInt = (++StateMachineStateInt) % STATE_MACHINE_UNKNOWN_STATE;
//   	StateMachineState =  (StateMachineStates)StateMachineStateInt;
// 	//StateMachineState = (++StateMachineState) % STATE_MACHINE_UNKNOWN_STATE;
// 	ThisAddressStruct.Address_StringPointer = 0;
// 	PrintOutInfo = true;
// }

// void TreatReceivedCharacterAsAddressCharacter(char *ch)
// {
// 	if (isxdigit(*ch))
// 	{
// 		ThisAddressStruct.Address_String[ThisAddressStruct.Address_StringPointer++] = *ch;
		
// 		if (MaxCharactersInAddressString == ThisAddressStruct.Address_StringPointer)
// 		{
// 			ChangeStateInStateMachine();
// 		}
// 	}
// 	else
// 	{
// 		GoBackToInitState();
// 	}
// }

// void TreatReceivedCharacterAsBitPositionCharacter(char *ch)
// {
// 	if ( (*ch >= '0') && (*ch <= '7') )
// 	{
// 		ThisAddressStruct.Bit_Position = *ch - '0';
// 		ChangeStateInStateMachine();
// 	}
// 	else
// 	{
// 		GoBackToInitState();
// 	}
// }

// void TreatReceivedCharacterAsBitValueCharacter(char *ch)  
// {
// 	uint8_t *Address_Pointer;
		
// 	if ( (*ch >= '0') && (*ch <= '1') )
// 	{
// 		ThisAddressStruct.Bit_State = *ch - '0';
// 		Address_Pointer = (uint8_t *)(uint16_t)strtoul((const char *)ThisAddressStruct.Address_String, NULL, 16);
		
// 		if (0 == ThisAddressStruct.Bit_State)
// 		{
// 			*Address_Pointer &= ~(1 << ThisAddressStruct.Bit_Position);
// 		}
// 		else
// 		{
// 			*Address_Pointer |= 1 << ThisAddressStruct.Bit_Position;
// 		}
		
// 		WriteText();
// 		ChangeStateInStateMachine();
// 	}
// 	else
// 	{
// 		GoBackToInitState();
// 	}
// }

// void ConvertReceivedChar(char *ReceivedChar)
// {
// 	// Den smarte måde at få konverteret små bogstaver om til store bogstaver og
// 	// modsat er ved brug af Xor, som vi tidligere har set. Så kan vi klare det i
// 	// én linje kode.

// 	if ( ((*ReceivedChar >= 0x41) && (*ReceivedChar <= 0x5D)) ||
// 	     ((*ReceivedChar >= 0x61) && (*ReceivedChar <= 0x7D)))
// 	{
// 		*ReceivedChar = *ReceivedChar ^ Upper_Lower_Bit_Value;
// 	}
// }

// void ChangeRecivedCharacterSmallBig(char *ch)
// {
//   if (*ch != '0')
//   {
// 	  ConvertReceivedChar(ch);
// 	  printf("%c", *ch);
//   }	
//   else
//   {
// 	  ChangeStateInStateMachine();
//   }
// }

// void WriteFunnyText(char *ch)
// {
// 	if (Write_Funny_Text_Character == *ch)
// 	{
//     	//USE_SERIAL.println();
//     	//USE_SERIAL.println("Nu kører det for h4pd041122 selv uden debugger !!!!");
// 		printf("\n");
// 		printf("Nu kører det for h4pd041122 selv uden debugger !!!!");
// 		ChangeStateInStateMachine();
// 	}
// }

// const StateMachineStruct PROGMEM StateMachineStructStructArray[]  =
// {
// 	{STATE_MACHINE_RECEIVING_ADDRESS, TreatReceivedCharacterAsAddressCharacter, StateMachineReceivingAddressString},
// 	{STATE_MACHINE_RECEIVING_BIT_POSITION, TreatReceivedCharacterAsBitPositionCharacter, StateMachineReceivingBitPositionString},
// 	{STATE_MACHINE_RECEIVING_BIT_VALUE, TreatReceivedCharacterAsBitValueCharacter, StateMachineReceiveBitValueString}
// 	// {STATE_MACHINE_CONVERT_RECEIVED_CHARACTERS, ChangeRecivedCharacterSmallBig, StateMachineConvertReceivedCharacters},
// 	// {STATE_MACHINE_WRITE_FUNNY_TEXT, WriteFunnyText, StateMachineWriteFunnyText}
// };

// StateMachineStruct* AllocateMemoryInRamAndGetCopyFromFlashProm()
// {
// 	StateMachineStruct *StateMachineStructInRAM_Pointer = (StateMachineStruct *)malloc(sizeof(StateMachineStruct));
//   	memcpy_FlashProm((char *)StateMachineStructInRAM_Pointer, (const char *)&(StateMachineStructStructArray[(uint8_t)StateMachineState]), sizeof(StateMachineStruct));
	
// 	return (StateMachineStructInRAM_Pointer);
// }

// void TreatUartReceivedCharAccordingToState(char InputChar)
// {
// 	StateMachineStruct *StateMachineStructInRAM_Pointer;
	
// 	StateMachineStructInRAM_Pointer = AllocateMemoryInRamAndGetCopyFromFlashProm();
	
// 	StateMachineStructInRAM_Pointer->ThisFunctionPointer(&InputChar);
	
// 	free(StateMachineStructInRAM_Pointer);
// }


// void setup() {
//   USE_SERIAL.begin(115200);
//   debug_init();
//   delay(3000);

//   fdevopen(&my_putc, 0);    // OBS !!! Har omdirigerer vi printf til at bruge UART gennem den angivne funktion : my_putc. 
//                             // Det vil sige, at alle printf sætninger vil blive omdirigeret til UART'en.

// #ifdef Display_Attached
// 	display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
// 	display.clearDisplay();
// 	display.setTextColor(WHITE);
// 	WriteInitText();
// #endif

//   // put your setup code here, to run once:
//   StateMachineState = STATE_MACHINE_RECEIVING_ADDRESS;
//   ThisAddressStruct.Address_StringPointer = 0;
//   ThisAddressStruct.Address_String[MaxCharactersInAddressString] = '\0';

//   //USE_SERIAL.println();
//   //USE_SERIAL.println("Nu starter vi State Maskinen !!!");
//   //USE_SERIAL.println();
//   printf("\n");
//   printf("Nu starter vi State Maskinen !!!");
//   printf("\n");

//   PrintOutInfo = true;
// }

// void loop() {
//   // put your main code here, to run repeatedly:

//     //breakpoint();

//     if (USE_SERIAL.available() > 0) 
//     {
//       // read the incoming byte:
//       IncomingByte = USE_SERIAL.read();
//       IncomingChar = static_cast<char>(IncomingByte);

//       TreatUartReceivedCharAccordingToState(IncomingChar);
//     }

//     if (true == PrintOutInfo)
// 	{
// 		PrintOutInfo = false;
					
// 		StateMachineStructInRAM_Pointer = AllocateMemoryInRamAndGetCopyFromFlashProm();
			
// 		NumberOfCharsInString = strlen_FlashProm((const char *)StateMachineStructInRAM_Pointer->ptrToFlashStatusString);
// 		StringPointer = (char *)malloc(NumberOfCharsInString);
// 		memcpy_FlashProm((char *)StringPointer, (const char *)(StateMachineStructInRAM_Pointer->ptrToFlashStatusString), NumberOfCharsInString);
		
// 		//USE_SERIAL.print(StringPointer);
// 		printf(StringPointer);
					
// 		free(StringPointer);
// 		free(StateMachineStructInRAM_Pointer);
// 	}
// }

