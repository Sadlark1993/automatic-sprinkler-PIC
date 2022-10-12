/*###################################################################### 
 Rotina utilizaçãoo do teclado da placa PicSim board4
 Adaptada para o compilador CCS
 Autor: Alberto Willia Mascarenhas (adaptou para o compilador)
 For e-mail suggestions : awmascarenhas@gmail.com
######################################################################## */

//Alterei os pinos das colunas para poder utilizar interrupcao pelo B0

//Keypad connection:
#define row0 PIN_B4
#define row1 PIN_B5
#define row2 PIN_B6
#define row3 PIN_B7
#define col0 PIN_B0
#define col1 PIN_B1
#define col2 PIN_B2
#define col3 PIN_B3


unsigned char tc_tecla(unsigned int timeout)
{
    unsigned int to=0;
    unsigned char k = 0; 
    while(((to < timeout)||(!timeout))&&(!k)){
       //-------------------------------------------------------------------------
       //inicio do programa de varredura do teclado matricial
       //--------------------------------------------------------------------------
       //habilita primeira coluna do teclado
       output_low(col0);output_high(col1);output_high(col2);output_high(col3);
       delay_ms(5);
       
       //So sai do while quando o usuario soltar o dedo do teclado
       if (input(row0) == 0){while(input(row0) == 0);k='1';};
       if (input(row1) == 0){while(input(row1) == 0);k='4';};
       if (input(row2) == 0){while(input(row2) == 0);k='7';};
       if (input(row3) == 0){while(input(row3) == 0);k='*';};
       //habilita segunda coluna do teclado
       output_high(col0);output_low(col1);output_high(col2);output_high(col3);
       delay_ms(5); 
       if (input(row0) == 0){while(input(row0) == 0);k='2';};
       if (input(row1) == 0){while(input(row1) == 0);k='5';};
       if (input(row2) == 0){while(input(row2) == 0);k='8';};
       if (input(row3) == 0){while(input(row3) == 0);k='0';};
       
       //habilita terceira coluna do teclado
       output_high(col0);output_high(col1);output_low(col2);output_high(col3);
       delay_ms(5); 
       if (input(row0) == 0){while(input(row0) == 0);k='3';};
       if (input(row1) == 0){while(input(row1) == 0);k='6';};
       if (input(row2) == 0){while(input(row2) == 0);k='9';};
       if (input(row3) == 0){while(input(row3) == 0);k='#';};
       
       //habilita quarta coluna do teclado
       output_high(col0);output_high(col1);output_high(col2);output_low(col3);
       delay_ms(5); 
       if (input(row0) == 0){while(input(row0) == 0);k='A';};
       if (input(row1) == 0){while(input(row1) == 0);k='B';};
       if (input(row2) == 0){while(input(row2) == 0);k='C';};
       if (input(row3) == 0){while(input(row3) == 0);k='D';};
       
       delay_ms(1);
       to+=5;
    }
    if(!k)k=255;
    return k; 
}
