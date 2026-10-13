/*
File: Lab6.c
Author: Thiven Anderson
Email: thanderson@hmc.edu
Date: 10/12/2026
*/


#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "main.h"
#include "DS1722.h"

/////////////////////////////////////////////////////////////////
// Provided Constants and Functions
/////////////////////////////////////////////////////////////////

//Defining the web page in two chunks: everything before the current time, and everything after the current time
char* webpageStart = "<!DOCTYPE html><html><head><title>E155 Web Server Demo Webpage</title>\
	<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\
	</head>\
	<body><h1>E155 Web Server Demo Webpage</h1>";
char* ledStr = "<p>LED Control:</p><form action=\"ledon\"><input type=\"submit\" value=\"Turn the LED on!\"></form>\
	<form action=\"ledoff\"><input type=\"submit\" value=\"Turn the LED off!\"></form>";
char* resStr = "<h2>Temperature Resolution</h2>\
    <form action=\"res8\"><button>8-bit</button></form>\
    <form action=\"res9\"><button>9-bit</button></form>\
    <form action=\"res10\"><button>10-bit</button></form>\
    <form action=\"res11\"><button>11-bit</button></form>\
    <form action=\"res12\"><button>12-bit</button></form>";
        
char* webpageEnd   = "</body></html>";

//determines whether a given character sequence is in a char array request, returning 1 if present, -1 if not present
int inString(char request[], char des[]) {
	if (strstr(request, des) != NULL) {return 1;}
	return -1;
}

int updateLEDStatus(char request[])
{
	static int led_status = 0;
	// The request has been received. now process to determine whether to turn the LED on or off
	if (inString(request, "ledoff")==1) {
		digitalWrite(LED_PIN, PIO_LOW);
		led_status = 0;
	}
	else if (inString(request, "ledon")==1) {
		digitalWrite(LED_PIN, PIO_HIGH);
		led_status = 1;
	}

	return led_status;
}

/////////////////////////////////////////////////////////////////
// Solution Functions
/////////////////////////////////////////////////////////////////

int main(void) {
  configureFlash();
  configureClock();

  gpioEnable(GPIO_PORT_A);
  gpioEnable(GPIO_PORT_B);
  gpioEnable(GPIO_PORT_C);

  pinMode(PB3, GPIO_OUTPUT);
  digitalWrite(LED_PIN, PIO_LOW); //start led low
  
  RCC->APB2ENR |= (RCC_APB2ENR_TIM15EN);
  initTIM(TIM15);
  
  USART_TypeDef * USART = initUSART(USART1_ID, 125000);

  // SPI initialization code
  initSPI(4, 0, 1);

  volatile uint8_t received = spiSendReceive(0xA5);

  while (1);
  initDS1722();
  uint8_t resolution = 12;

  while(1) {
    /* Wait for ESP8266 to send a request.
    Requests take the form of '/REQ:<tag>\n', with TAG begin <= 10 characters.
    Therefore the request[] array must be able to contain 18 characters.
    */

    // Receive web request from the ESP
    char request[BUFF_LEN] = "                  "; // initialize to known value
    int charIndex = 0;
  
    // Keep going until you get end of line character
    while(inString(request, "\n") == -1) {
      // Wait for a complete request to be transmitted before processing
      while(!(USART->ISR & USART_ISR_RXNE));
      request[charIndex++] = readChar(USART);
    }
    // Check for resolution change request
    if (strncmp(request, "/REQ:res", 8) == 0) {
        int newResolution = atoi(request + 8);

        if (newResolution >= 8 && newResolution <= 12) {
            setDS1722Resolution((uint8_t)newResolution);
            resolution = (uint8_t)newResolution;
        }
    }
    // SPI code for reading temperature
    
    float temperature = readDS1722Temp();

    char tempStr[32];
    sprintf(tempStr, "%.2f C", temperature);
    // Update string with current LED state
  
    int led_status = updateLEDStatus(request);

    char ledStatusStr[20];
    if (led_status == 1)
      sprintf(ledStatusStr,"LED is on!");
    else if (led_status == 0)
      sprintf(ledStatusStr,"LED is off!");

    // finally, transmit the webpage over UART
    sendString(USART, webpageStart); // webpage header code
    sendString(USART, ledStr); // button for controlling LED

    sendString(USART, "<h2>LED Status</h2>");


    sendString(USART, "<p>");
    sendString(USART, ledStatusStr);
    sendString(USART, "</p>");
    //temp read and res control
    sendString(USART, "<h2>Temperature</h2>");

    sendString(USART, "<p>");
    sendString(USART, tempStr);
    sendString(USART, "</p>");

    // Resolution controls and current setting
    sendString(USART, resStr);

    char resStatusStr[40];
    sprintf(resStatusStr, "<p>Current resolution: %d-bit</p>", resolution);
    sendString(USART, resStatusStr);
    sendString(USART, webpageEnd);
  }
}
