/*
to the key pad, the signal is sended by the column pins, and collected by the
row pins.

keys meaning
A = right
B = left
C = up
D = down
# = exit
* = enter

keypad schematic
1 = C1+R1;
2 = C2+R1;
3 = C3+R1;
4 = C1+R2;
5 = C2+R2;
6 = C3+R2;
7 = C1+R3;
8 = C2+R3;
9 = C3+R3;
0 = C3+R4;
A(right) = C4+R1;
B(left) = C4+R2;
C(up) = C4+R3;
D(down) = C4+R4;
#(exit) = C3+R4;
*(enter) = C1+R4;

keypad pins connection
R1 PIN_B4
R2 PIN_B5
R3 PIN_B6
R4 PIN_B7
C1 PIN_B0
C2 PIN_B1
C3 PIN_B2
C4 PIN_B3

*/


//#include <projetoPIC.h>
#include <16F877A.h>
#device ADC=16

#FUSES NOWDT                    //No Watch Dog Timer
#FUSES NOBROWNOUT               //No brownout reset
#FUSES NOLVP                    //No low voltage prgming, B3(PIC16) or B5(PIC18) used for I/O

#use delay(crystal=20000000)

//comunicacao pelo protocolo I2C
#use I2C(MASTER, I2C1, SLOW = 100000, STREAM = DS3231_STREAM)

//Esquema dos pinos do LCD
#ifndef lcd_enable
   #define lcd_enable pin_E1 // pino enable do LCD
   #define lcd_rs pin_E2 // pino rs do LCD
   //#define lcd_rw pin_e2 // pino rw do LCD
   #define lcd_d4 pin_d4 // pino de dados d4 do LCD
   #define lcd_d5 pin_d5 // pino de dados d5 do LCD
   #define lcd_d6 pin_d6 // pino de dados d6 do LCD
   #define lcd_d7 pin_d7 // pino de dados d7 do LCD
#endif

//Pinos dos reles
#define RELE1 PIN_C0
#define RELE2 PIN_C1
#define RELE3 PIN_C2
#define RELE4 PIN_C5

#include "mod_lcd.c"
#include "kbd_ext_board2.c"
#include "DS3231.c" //RTC library

unsigned char k;
unsigned int sol;
unsigned int aux = 0;

//tabelas de dados dos solenoides: hi,mi,hf,mf.
unsigned int s1[4][4] = {{13,0,14,30},
                         {17,30,19,0},
                         {0,0,2,20},
                         {0,0,0,0}};
                         
unsigned int s2[4][4] = {{0,0,0,0},
                         {0,0,0,0},
                         {0,0,0,0},
                         {0,0,0,0}};
                         
unsigned int s3[4][4] = {{0,0,0,0},
                         {0,0,0,0},
                         {0,0,0,0},
                         {0,0,0,0}};
                         
unsigned int s4[4][4] = {{0,0,0,0},
                         {0,0,0,0},
                         {0,0,0,0},
                         {0,0,0,0}};
                         
unsigned int period[4]; //variavel auxiliar q guardara temporariamente um periodo
RTC_Time *myTime; //variavel struct da biblioteca do RTC DS3231.C

#INT_RB
void  RB_isr(void) 
{
   clear_interrupt(INT_RB);
   //disable_interrupts(INT_RB);
   if(!input(PIN_B4)||!input(PIN_B5)||!input(PIN_B6)||!input(PIN_B7)){
      //printf(lcd_escreve, "\fEntrando int");
      output_high(PIN_D0);
      k = tc_tecla(15);
      output_low(PIN_B0);
      output_low(PIN_B1);
      output_low(PIN_B2);
      output_low(PIN_B3);
      //printf(lcd_escreve, "\fSaindo int");
   }
   output_low(PIN_D0);
   //enable_interrupts(INT_RB);
}


void store_data(unsigned int i, unsigned int s){
   if(s == 1){
      s1[i][0] = period[0];
      s1[i][1] = period[1];
      s1[i][2] = period[2];
      s1[i][3] = period[3];
   }else if(s == 2){
      s2[i][0] = period[0];
      s2[i][1] = period[1];
      s2[i][2] = period[2];
      s2[i][3] = period[3];
   }else if(s == 3){
      s3[i][0] = period[0];
      s3[i][1] = period[1];
      s3[i][2] = period[2];
      s3[i][3] = period[3];
   }else if(s == 4){
      s4[i][0] = period[0];
      s4[i][1] = period[1];
      s4[i][2] = period[2];
      s4[i][3] = period[3];
   }else{
      printf(lcd_escreve, "\fERROR. Sol %u\nnot recgnzd.",s);
   }
}

//funcao q decide se o estamos dentro do periodo de irrigacao
//vars: hora inicia, minuto inicial, hora final, minuto final.
int1 switchSprinkler(unsigned int hi, unsigned int mi, unsigned int hf, unsigned int mf){
   if(hi<hf){ //hora inicial < hora final
      if(myTime->hours >= hi){
         if(myTime->hours > hf) return 0;
         else if(myTime->hours < hf) return 1;
         else{
            if(myTime->minutes < mf) return 1;
            else return 0;
         }
      }else return 0;
   }else if(hi == hf){ //hora inicial e hora final iguais
      if(mi<=mf){
         if(myTime->hours == hi && myTime->minutes >= mi && myTime->minutes < mf) return 1;
         else return 0; // <-- os periodos nao programados vai entrar aqui
      }else{ //Quase o dia inteiro ligado
         if(myTime->hours != hi || myTime->minutes >= mi || myTime->minutes <= mf) return 1;
         else return 0;
      }
   }else{ //horario de irrigacao passa pela meia noite
      if(myTime->hours == hi){
         if(myTime->minutes >= mi) return 1;
         else return 0;
      }else if(myTime->hours > hi || myTime->hours < hf) return 1;
      else if(myTime->hours == hf){
         if(myTime->minutes < mf) return 1;
         else return 0;
      } return 0;
   }
}

void irrigate(){
   int1 relS1 = 0;
   int1 relS2 = 0;
   int1 relS3 = 0;
   int1 relS4 = 0;
   
   unsigned int j;
   
   //s1
   for(j = 0; j < 4 && !relS1; j++){
      relS1 = switchSprinkler(s1[j][0], s1[j][1], s1[j][2], s1[j][3]);
   }
   
   //s2
   for(j = 0; j < 4 && !relS2; j++){
      relS2 = switchSprinkler(s2[j][0], s2[j][1], s2[j][2], s2[j][3]);
   }
   
   //s3
   for(j = 0; j < 4 && !relS3; j++){
      relS3 = switchSprinkler(s3[j][0], s3[j][1], s3[j][2], s3[j][3]);
   }
   
   //s4
   for(j = 0; j < 4 && !relS4; j++){
      relS4 = switchSprinkler(s4[j][0], s4[j][1], s4[j][2], s4[j][3]);
   }
   
   if(relS1) output_high(RELE1);
   else output_low(RELE1);
   
   if(relS2) output_high(RELE2);
   else output_low(RELE2);
   
   if(relS3) output_high(RELE3);
   else output_low(RELE3);
   
   if(relS4) output_high(RELE4);
   else output_low(RELE4);
   
   output_toggle(PIN_D1);
}
          
//funcao que permite o usuario alterar o horario inicial de irrigacao pelo keypad
int alt_i(unsigned int i, unsigned int s){
   int1 f = 1;
   unsigned int num;
   
   period[0] = 0;
   period[1] = 0;
   period[2] = 0;
   period[3] = 0;
   
   k=255;
   //dezenas da h inicial
   while(f){
      printf(lcd_escreve, "\fset hi for t%u\n%02u:%02u-%02u:%02u", (i+1), period[0], period[1], period[2], period[3]);
      if(k>=48 && k<=57){ //se for digito
         num = k-48;
         period[0] = num*10;
         k=255;
         f = 0;  
      }else if(k == '#'){
         k=255;
         return 0;
      }
      delay_ms(250);
      printf(lcd_escreve, "\fset hi for t%u\n  :%02u-%02u:%02u",(i+1), period[1], period[2], period[3]);
      delay_ms(250);
   }
   f = 1;
   
   //unidades da h inicial
   while(f){
      printf(lcd_escreve, "\fset hi for t%u\n%02u:%02u-%02u:%02u",(i+1), period[0], period[1], period[2], period[3]);
      if(k>=48 && k<=57){ //se for digito
         num = k-48;
         period[0] += num%10;
         k=255;
         f = 0;
         if(period[0]>23){ //horas nao podem ultrapassar 23
            printf(lcd_escreve, "\fERRO. %02u > 23",period[0]);
            delay_ms(3000);
            k = 255;
            return 0;
         }
      }else if(k == '#'){
         k=255;
         return 0;
      }
      delay_ms(250);
      printf(lcd_escreve, "\fset hi for t%u\n  :%02u-%02u:%02u",(i+1), period[1], period[2], period[3]);
      delay_ms(250);
   }
   f=1;
   
   //dezenas do m inicial
   while(f){
      printf(lcd_escreve, "\fset mi for t%u\n%02u:%02u-%02u:%02u",(i+1), period[0], period[1], period[2], period[3]);
      if(k>=48 && k<=57){ //se for digito
         num = k-48;
         period[1] = num*10;
         k=255;
         f = 0;
      }else if(k == '#'){
         k=255;
         return 0;
      }
      delay_ms(250);
      printf(lcd_escreve, "\fset mi for t%u\n%02u:  -%02u:%02u",(i+1), period[0], period[2], period[3]);
      delay_ms(250);
   }
   f=1;
   
   //unidades do m inicial
   while(f){
      printf(lcd_escreve, "\fset mi for t%u\n%02u:%02u-%02u:%02u",(i+1), period[0], period[1], period[2], period[3]);
      if(k>=48 && k<=57){ //se for digito
         num = k-48;
         period[1] += num%10;
         k=255;
         f = 0;
         if(period[1]>59){ //minutos nao podem ultrapassar 59
            printf(lcd_escreve, "\fERRO. %u > 59",period[1]);
            delay_ms(3000);
            k = 255;
            return 0;
         }
      }else if(k == '#'){
         k=255;
         return 0;
      }
      delay_ms(250);
      printf(lcd_escreve, "\fset mi for t%u\n%02u:  -%02u:%02u",(i+1), period[0], period[2], period[3]);
      delay_ms(250);
   }
   f = 1; 
   return 0;
}

//Funcao q altera o horario final do periodo de irrigacao
int alt_f(unsigned int i, unsigned int s){
   int1 f = 1;
   unsigned int num;
   k=255;

//dezenas da h final
   while(f){
      printf(lcd_escreve, "\fset hf for t%u\n%02u:%02u-%02u:%02u",(i+1), period[0], period[1], period[2], period[3]);
      if(k>=48 && k<=57){ //se for digito
         num = k-48;
         period[2] = num*10;
         k=255;
         f = 0;
      }else if(k == '#'){
         k=255;
         return 0;
      }
      delay_ms(250);
      printf(lcd_escreve, "\fset hf for t%u\n%02u:%02u-  :%02u",(i+1), period[0], period[1], period[3]);
      delay_ms(250);
   }
   f=1;
   
   //unidades da h final
   while(f){
      printf(lcd_escreve, "\fset hf for t%u\n%02u:%02u-%02u:%02u",(i+1), period[0], period[1], period[2], period[3]);
      if(k>=48 && k<=57){ //se for digito
         num = k-48;
         period[2] += num%10;
         k=255;
         f = 0;
         if(period[2]>23){
            printf(lcd_escreve, "\fERRO. %02u > 23",period[2]);
            delay_ms(3000);
            k = 255;
            return 0;
         }
      }else if(k == '#'){
         k=255;
         return 0;
      }
      delay_ms(250);
      printf(lcd_escreve, "\fset hf for t%u\n%02u:%02u-  :%02u",(i+1), period[0], period[1], period[3]);
      delay_ms(250);
   }
   f=1;
   
   //dezenas do m final
   while(f){
      printf(lcd_escreve, "\fset mf for t%u\n%02u:%02u-%02u:%02u",(i+1), period[0], period[1], period[2], period[3]);
      if(k>=48 && k<=57){ //se for digito
         num = k-48;
         period[3] = num*10;
         k=255;
         f = 0;
      }else if(k == '#'){
         k=255;
         return 0;
      }
      delay_ms(250);
      printf(lcd_escreve, "\fset mf for t%u\n%02u:%02u-%02u:  ",(i+1), period[0], period[1], period[2]);
      delay_ms(250);
   }
   f=1;
   
   //unidades do m final
   while(f){
      printf(lcd_escreve, "\fset mf for t%u\n%02u:%02u-%02u:%02u",(i+1), period[0], period[1], period[2], period[3]);
      if(k>=48 && k<=57){ //se for digito
         num = k-48;
         period[3] += num%10;
         k=255;
         f = 0;
         if(period[3]>59){ //minutos nao podem ultrapassar 59
            printf(lcd_escreve, "\fERRO. %u > 59",period[3]);
            delay_ms(3000);
            k = 255;
            return 0;
         }
      }else if(k == '#'){
         k=255;
         return 0;
      }
      delay_ms(250);
      printf(lcd_escreve, "\fset mf for t%u\n%02u:%02u-%02u:  ",(i+1), period[0], period[1], period[2]);
      delay_ms(250);
   }
   f = 1;
   
   printf(lcd_escreve, "\f    t%u DONE!\n%02u:%02u-%02u:%02u",(i+1), period[0], period[1], period[2], period[3]);
   delay_ms(3000); 
   store_data(i, s);
   return 0;
}

/*
Funcoes sol exibem os dados das tabelas dos solenoides
*/
void sol1(){
   unsigned int i = 0;
   int1 f = 1;
   while(f){
      //exibe o periodo de irrigacao selecionado do solenoide 1
      printf(lcd_escreve, "\fSol1 T%u\n%02u:%02u-%02u:%02u",(i+1), s1[i][0], s1[i][1], s1[i][2], s1[i][3]);
      //D: seleciona o proximo periodo de irrigacao (maximo 4 periodos)
      if(k=='D'&&i<3){
         //printf(lcd_escreve, "\fEntrou D"); //debug
         i++; 
         k = 255;
      //C: seleciona o periodo de irrigacao anterior //debug
      }else if(k=='C'&&i>0){
         //printf(lcd_escreve, "\fEntrou C");
         i--;
         k = 255; 
      //A: seleciona o proximo solenoide. B: seleciona o solenoide anterior
      }else if(k=='A'){
         sol = 2;
         k = 255;
         f = 0;
      }else if(k=='*'){
         alt_i(i, 1);
         alt_f(i, 1);
      }else if(k=='#'){
         s1[i][0] = 0;
         s1[i][1] = 0;
         s1[i][2] = 0;
         s1[i][3] = 0;
      }
      delay_ms(150);
   } 
}

void sol2(){
   unsigned int i = 0;
   int1 f = 1;
   while(f){
      printf(lcd_escreve, "\fSol2 T%u\n%02u:%02u-%02u:%02u",(i+1), s2[i][0], s2[i][1], s2[i][2], s2[i][3]);
      if(k=='D'&&i<3){
         //printf(lcd_escreve, "\fEntrou D");
         i++; 
         k = 255;
      }else if(k=='C'&&i>0){
         //printf(lcd_escreve, "\fEntrou C");
         i--;
         k = 255; 
      }else if(k=='A'){
         sol = 3;
         k = 255;
         f = 0;
      }else if(k=='B'){
         sol = 1;
         k = 255;
         f = 0;
      }else if(k=='*'){
         alt_i(i, 2);
         alt_f(i, 2);
      }else if(k=='#'){
         s2[i][0] = 0;
         s2[i][1] = 0;
         s2[i][2] = 0;
         s2[i][3] = 0;
      }
      delay_ms(150);
   } 
}

void sol3(){
   unsigned int i = 0;
   int1 f = 1;
   while(f){
      printf(lcd_escreve, "\fSol3 T%u\n%02u:%02u-%02u:%02u",(i+1), s3[i][0], s3[i][1], s3[i][2], s3[i][3]);
      if(k=='D'&&i<3){
         //printf(lcd_escreve, "\fEntrou D");
         i++; 
         k = 255;
      }else if(k=='C'&&i>0){
         //printf(lcd_escreve, "\fEntrou C");
         i--;
         k = 255;
      }else if(k=='A'){
         sol = 4;
         k = 255;
         f = 0;
      }else if(k=='B'){
         sol = 2;
         k = 255;
         f = 0;
      }else if(k=='*'){
         alt_i(i, 3);
         alt_f(i, 3);
      }else if(k=='#'){
         s3[i][0] = 0;
         s3[i][1] = 0;
         s3[i][2] = 0;
         s3[i][3] = 0;
      }
      delay_ms(150);
   } 
}

void sol4(){
   unsigned int i = 0;
   int1 f = 1;
   while(f){
      printf(lcd_escreve, "\fSol4 T%u\n%02u:%02u-%02u:%02u",(i+1), s4[i][0], s4[i][1], s4[i][2], s4[i][3]);
      if(k=='D'&&i<3){
         //printf(lcd_escreve, "\fEntrou D");
         i++; 
         k = 255;
      }else if(k=='C'&&i>0){
         //printf(lcd_escreve, "\fEntrou C");
         i--;
         k = 255; 
      }else if(k=='B'){
         sol = 3;
         k = 255;
         f = 0;
      }else if(k=='*'){
         alt_i(i, 4);
         alt_f(i, 4);
      }else if(k=='#'){
         s4[i][0] = 0;
         s4[i][1] = 0;
         s4[i][2] = 0;
         s4[i][3] = 0;
      }
      delay_ms(150);
   } 
}

//interrupcao q vai ativar a funcao de irrigacao
#INT_TIMER1 
void  TIMER1_isr(void) 
{
   aux++;
   if(aux>=50){//a cada 5 segundos
      aux=0;
      myTime = RTC_Get(); //leitura do relogio
      irrigate(); //funcao kernel do codigo
   }
}

void main()
{
   //setando portas b
   set_tris_b(0xF0);
   port_b_pullups(true);
   output_low(PIN_B0);
   output_low(PIN_B1);
   output_low(PIN_B2);
   output_low(PIN_B3);
   
   clear_interrupt(INT_RB);
   enable_interrupts(INT_RB);
   enable_interrupts(GLOBAL);
   
   delay_ms(100);
   lcd_ini();
   delay_ms(100);
   printf(lcd_escreve, "\fIFMT 2022");
   delay_ms(1000);
   
   sol = 1;
   k = 255;
   
   //timer 1 que vai executar o ligamento-desligamento dos reles.
   setup_timer_1(T1_INTERNAL|T1_DIV_BY_8);      //104 ms overflow
   enable_interrupts(INT_TIMER1);
   enable_interrupts(GLOBAL);
   
   myTime = RTC_Get(); //leitura do relogio
   
   output_low(RELE1);
   output_low(RELE2);
   output_low(RELE3);
   output_low(RELE4);

   while(TRUE)
   {
         //alternando entre a selecao dos solenoides. Depende do dado na var sol.
         if(sol == 1){
            sol1();
         }else if(sol == 2){
            sol2();
         }else if(sol == 3){
            sol3();
         }else if(sol == 4){
            sol4();
         }else{
               printf(lcd_escreve, "\fERRO:\nNo Sol Selected");
               delay_ms(200);
         }
   }
}
