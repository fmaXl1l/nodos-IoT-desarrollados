#include <16f877a.h>
#device PASS_STRINGS = IN_RAM
#fuses XT, NOWDT, NOPROTECT, PUT, NOLVP, BROWNOUT 
//! XT -> oscilador frecuencia entre 100KHz y 4MHz
//! PUT -> Power Up Timer. El PIC se retrasa al iniciar hasta alcanzar el voltaje operativo completo.
//! NOLVP -> Desactiva el modo de programacion de bajo consumo
//! BROWNOUT -> PIC se resetea si la alimentacion baja de cierto limite.
#use delay(clock=4M)
#use RS232(BAUD=9600, BITS=8, PARITY=N, XMIT=PIN_C6, RCV=PIN_C7)     // TX = PIN_C6, RX = PIN_C7

#use standard_io(A)
#use standard_io(D)

#define LCD_DB4   PIN_D4                        // Pines de la pantalla LCD
#define LCD_DB5   PIN_D5
#define LCD_DB6   PIN_D6
#define LCD_DB7   PIN_D7
#define LCD_RS    PIN_D2
#define LCD_E     PIN_D3

#define MFRC522_CS  PIN_A0                      // Pin SDA
#define MFRC522_SCK PIN_A1                      // Pin SCK
#define MFRC522_SI  PIN_A2                      // Pin MOSI
#define MFRC522_SO  PIN_A3                      // Pin MISO
#define MFRC522_RST PIN_A4                      // Pin RST

#define timbre PIN_D0
#define buzzer PIN_D1

#include <RC522.h>                              // Libreria para el manejo del modulo MFRC522
#include <LCD_16X2.c>                           // Libreria para el manejo de la pantalla LCD
#include <get_string.c>                      // Libreria para recepcion de cadenas por UART

char cadena[20];                             // Arreglo donde se almacena la cadena de caracteres
char user[5] = {0xE3,0xC3,0xDD,0x3E,0xC3};      // ID para acceder
char user1[5] = {0x07,0x77,0xB6,0xB5,0x73};
int i;                                         // Contador
char UID[10];                                   // Almacena los digitos de la ID
unsigned int TagType;                           // Variable de verificacion de tag


void mqtt_init(){
   printf("AT+MQTTUSERCFG=0,1,\"ESP01_PIC\",\"mqtt-user\",\"mqtt-password\",0,0,\"\"\r\n");
   do{
      read_string(cadena, 20);
   }while(strcmp(cadena, "OK"));
   
   printf("AT+MQTTCONN=0,\"192.168.30.50\",1883,1\r\n");
   do{
      read_string(cadena, 20);
   }while(strcmp(cadena, "OK"));
}

void main()
{
   mqtt_init();
   lcd_init();                                  // Inicializa la pantalla LCD
   MFRC522_Init();                              // Inicializa el modulo RFID MFRC522
   
   while(true)
   {
      lcd_gotoxy(2,1);                          // Imprime un texto breve en la pantalla LCD
      lcd_putc("IDENTIFIQUESE");
      
      if(MFRC522_isCard(&TagType))              // Verificacion si hay un tag disponible
      {
         if(MFRC522_ReadCardSerial(&UID))       // Lectura y verificacion si encontro algun tag
         {
            output_high(buzzer);
            delay_ms(100);
            lcd_clear();
            lcd_gotoxy(1,1);
            lcd_putc("ID: ");
            lcd_gotoxy(5,1);
            for(i=0; i<5; i++)                  // Imprime la ID en la pantalla LCD
            {
               printf(lcd_putc, "%X", UID[i]);
            }
            output_low(buzzer);
            
            if(MFRC522_Compare_UID(UID, user) || MFRC522_Compare_UID(UID, user1))  // Compara la ID del usuario si es correcta
            {
               printf("AT+MQTTPUB=0,\"PIC/ACCESO\",\"1\",0,0\r\n");
               do{
                  read_string(cadena, 20);
               }while(strcmp(cadena, "OK"));
               lcd_gotoxy(1,2);
               lcd_putc("Autorizado");
            }
            else                                // Si la ID no es correcta, el usuario no tiene acceso
            {
               printf("AT+MQTTPUB=0,\"PIC/ACCESO\",\"0\",0,0\r\n");
               do{
                  read_string(cadena, 20);
               }while(strcmp(cadena, "OK"));
               lcd_gotoxy(1,2);
               lcd_putc("Acceso Denegado");
            }
            delay_ms(2500);
            lcd_clear();
            MFRC522_Clear_UID(UID);             // Limpia temporalmente la ID
            delay_ms(100);
         }
         MFRC522_Halt();                        // Apaga la antena
      }
      
      if(input(timbre) == 0){
         printf("AT+MQTTPUB=0,\"PIC/TIMBRE\",\"1\",0,0\r\n");
         do{
            read_string(cadena, 20);
         }while(strcmp(cadena, "OK"));
         lcd_clear();
         lcd_gotoxy(1,1);
         lcd_putc("TIMBRANDO");
         delay_ms(2500);
         lcd_clear();
      }
   }
}

