#include "system_sam3x.h"
#include "at91sam3x8.h"
#include <stdio.h>
#include <math.h>

/*-----------------------Global konstants with at91sam3x8.h file--------------*/
volatile unsigned int *PIOD_PER = AT91C_PIOD_PER; //Pin enable
volatile unsigned int *PIOD_OER = AT91C_PIOD_OER; //Output enable register
volatile unsigned int *PIOD_ODR = AT91C_PIOD_ODR; //Output disable register
volatile unsigned int *PIOD_PDSR = AT91C_PIOD_PDSR; //Pin data status register
volatile unsigned int *PIOD_PUDR = AT91C_PIOD_PPUDR; //Pull-up disable resistor
volatile unsigned int *PIOD_PUER = AT91C_PIOD_PPUER; //Pull-up enable resistor 
volatile unsigned int *PIOD_SODR = AT91C_PIOD_SODR; //Set output data register
volatile unsigned int *PIOD_CODR = AT91C_PIOD_CODR; //Clear output data register
volatile unsigned int *PIOD_ODSR = AT91C_PIOD_ODSR; //Output data status register
volatile unsigned int *PIOD_PDR = AT91C_PIOD_PDR; //Pin disable register
volatile unsigned int *PIOD_PSR = AT91C_PIOD_PSR; //Pin status register

volatile unsigned int *PIOC_PER = AT91C_PIOC_PER; //Perphial master clock
volatile unsigned int *PIOC_OER = AT91C_PIOC_OER; //Perphial master clock
volatile unsigned int *PIOC_ODR = AT91C_PIOC_ODR; //Perphial master clock
volatile unsigned int *PIOC_PUER = AT91C_PIOC_PPUER; //Perphial master clock
volatile unsigned int *PIOC_PUDR = AT91C_PIOC_PPUDR; //Perphial master clock
volatile unsigned int *PIOC_SODR = AT91C_PIOC_SODR; //Perphial master clock
volatile unsigned int *PIOC_PDSR = AT91C_PIOC_PDSR; //Pin data status register
volatile unsigned int *PIOC_CODR = AT91C_PIOC_CODR; //Pin data status register

volatile unsigned int *PIOB_PER = AT91C_PIOB_PER; //Pin data status register
volatile unsigned int *PIOB_OER = AT91C_PIOB_OER; //Pin data status register
volatile unsigned int *PIOB_ODR = AT91C_PIOB_ODR; //Pin data status register
volatile unsigned int *PIOB_SODR = AT91C_PIOB_SODR; //Pin data status register
volatile unsigned int *PIOB_CODR = AT91C_PIOB_CODR; //Pin data status register

volatile unsigned int *PIOA_PER = AT91C_PIOA_PER; //Pin data status register
volatile unsigned int *PIOA_ODR = AT91C_PIOA_ODR; //Pin data status register
volatile unsigned int *PIOA_PUER = AT91C_PIOA_PPUER; //Pin data status register


int keypad(void);
int Value;
int colArr[] = {0x100,0x200,0x80};
int rowArr[]= {0x20,0x4,0x8,0x10};
int temp;
int statusDisp;
int TC_RA;
int TC_RB;
float tempCel;
float timeL1;
float ldrValue;
int tempChar;
float voltage;
int turn;

//PIOA = Disp/LDR
//PIOB = Temp/Servo
//PIOC = Keypad/PartDisp
//PIOD = SN75...(Keypad)

/*------------------------------Initialize Pins-------------------------------*/
void initPin()
{
  *AT91C_PMC_PCER = 0x6000;
  *PIOD_PER = 0xF;//(1<<3) + (1);//Pin 25 and Pin 27
  *PIOD_OER = 0x5;//(1<<3) + (1) ;  
  *PIOC_PER = 0xFF3FC; //Pin 34-41 + Pin 50-51
  *PIOC_ODR = 0xFF3FC; 

 
}
/*------------------------------Keypad----------------------------------------*/
int keypad(void)
{
  
  Value = 13;

  *PIOD_CODR = 1<<2; //Clear OE KEY BUS (Active Low), Pin 27
  *PIOC_OER = 0x380; //Make all Column pin as output, Pin 39-41 
  *PIOC_SODR = 0x380; //Set all Column pin as high
  *PIOC_PUDR = 0x380; //Disable column pins pull up resistor
  
  for(int Column = 0;Column<3;Column++){
    *PIOC_SODR = 0x380;
    *PIOC_CODR = colArr[Column]; //Clear one column at the time
      
    for(int Row = 0; Row<4;Row++){
      temp = (*PIOC_PDSR) & rowArr[Row];
      if(temp == 0){ //Read row and check if bit is zero
        Value = Row*3+Column+1;
       // printf("%d\n",Value);
      }
    }
  }
  
  *PIOC_SODR = 0x380; //Pin 39-41
  *PIOC_ODR = 0x380; //Make all Column pin as input
  *PIOD_SODR = 1<<2; //Set OE KEY BUS (Active Low), Pin 27
  return Value; 
}
/*------------------------------Delay-----------------------------------------*/
void Delay(int Value)
{
int i;
for(i=0;i<Value;i++)
    asm("nop");
} 
/*------------------------------Read status display---------------------------*/
unsigned char Read_Status_Display(void)
{

  *PIOC_ODR = 0x3FC; //Make databus as input
  *PIOC_SODR = 1<<13; //Set dir as input (74chip, 1 = input)
  *PIOC_CODR = 1<<12; //Clear/enable output (74chip 0 = enable)
  *PIOC_SODR = 1<<17; //Set C/D
  *PIOC_CODR = (1<<15) + (1<<16); //Clear chip select display - Clear read display, Pin 47-48
  *PIOC_SODR = (1 << 14); //Set wd
  
  Delay(10);//Make a Delay
  
  statusDisp = (*PIOC_PDSR >> 2);   //Read data bus and save it in temp
   
  *PIOC_SODR =(1<<15) + (1<<16) ; //Set chip select display - Set read display
  *PIOC_SODR = 1<<12; //Disable output (74chip)
  *PIOC_CODR = 1<<13; //Set dir as output (74chip)
  
  return statusDisp;
}
/*------------------------------Write command to display----------------------*/
void Write_Command_2_Display(unsigned char Command)
{
  while((Read_Status_Display() & 0x3) != 0x3){}//Wait until Read_Status_Display returns an OK
  
  *PIOC_OER = 0x3FC; //Set databus as output
  *PIOC_CODR = 0x3FC; //Clear databus
  
  *PIOC_SODR = (Command << 2);//Set Command to databus
  
  *PIOC_CODR = 1<<13; //Set dir as output (74chip), Pin 50
  *PIOC_CODR = 1<<12; //Clear/enable output (74chip 0 = enable), Pin 51
  *PIOC_SODR = 1<<17; //Set C/D signal High (1 = Command), Pin 46
  *PIOC_CODR = (1<<16) + (1<<14); //Pin 49+47, Clear chip select display -Clear write display
  *PIOC_SODR = (1 << 15); //Set RD
  Delay(10);//Make a Delay
  *PIOC_SODR = (1<<16) + (1<<14); //Set chip enable display- Set write display
  *PIOC_SODR = 1<<12; //Disable output (74chip)
  *PIOC_ODR = 0x3FC; //Make databus as input
}
/*------------------------------Write data to display-------------------------*/
void Write_Data_2_Display(unsigned char Data)
{ 
  while((Read_Status_Display() & 0x3) != 0x3){}//Wait until Read_Status_Display returns an OK
  
  *PIOC_OER = 0x3FC; //Set databus as output, Pin 34-41
  *PIOC_CODR = 0x3FC; //Clear databus
  
  *PIOC_SODR  = (Data << 2);//Set Data to databus
  
  *PIOC_CODR = 1<<13; //Set dir as output (74chip), Pin 50
  *PIOC_CODR = 1<<12; //Enable output (74chip), Pin 51
  *PIOC_CODR = 1<<17; //Clear C/D signal High (0 = Data), Pin 46
  *PIOC_CODR = (1<<16) + (1<<14); //Clear chip select display -Clear write display, Pin 47 och Pin 49
  *PIOC_SODR = (1 << 15); //Set RD, Pin 48 
  Delay(10);//Make a Delay      
  *PIOC_SODR = (1<<16) + (1<<14); //Set chip enable display- Set write display
  *PIOC_SODR = 1<<12; //Disable output (74chip), Pin 51
  *PIOC_ODR = 0x3FC; //Make databus as input
}
/*------------------------------Initialize display----------------------------*/
void Init_Display(void)
{
  *PIOD_CODR = 0x1;//Clear Reset display!
  Delay(25);//Make a Delay
  *PIOD_SODR = 0x1;//Set Reset display!
  Write_Data_2_Display(0x00);
  Write_Data_2_Display(0x00);
  Write_Command_2_Display(0x40);//Set text home address
  Write_Data_2_Display(0x00);
  Write_Data_2_Display(0x40);
  Write_Command_2_Display(0x42); //Set graphic home address
  Write_Data_2_Display(0x1e);
  Write_Data_2_Display(0x00);
  Write_Command_2_Display(0x41); // Set text area
  Write_Data_2_Display(0x1e);
  Write_Data_2_Display(0x00);
  Write_Command_2_Display(0x43); // Set graphic area
  Write_Command_2_Display(0x80); // text mode
  Write_Command_2_Display(0x94); // Text on graphic off*/
  Write_Data_2_Display(0x00);
  Write_Data_2_Display(0x00);
  Write_Command_2_Display(0x24);
  
  for(int i = 0;i<640;i++){
    Write_Data_2_Display(0x00);
    Write_Command_2_Display(0xC0);
  }
  
  Write_Data_2_Display(0x00);
  Write_Data_2_Display(0x00);
  Write_Command_2_Display(0x24);
}
/*------------------------------Print Display---------------------------------*/
void print_Display(void)
{
  int number = keypad();
  if(number != 13){
    if(number == 10)
    {
      number = 0xA;
    }
    else if(number == 11)
    {
      number = 0x10;
    }
    else if(number == 12)
    {
      number = 0x3;
    }
    else{
      number = number+0x10;
    }
  Write_Data_2_Display(number);
  Write_Command_2_Display(0xC0);
  }
}
/*-----------------------Initialize Timer-------------------------------------*/
void Init_Timer(void)
{
  *AT91C_PMC_PCER = 0x8001000; //PMC = TC0 (0x8000000) & PIOB (0x1000) 
  *AT91C_TC0_CMR  = 1; //Select Timer_Clock1 as TCCLK
  *AT91C_TC0_CCR  = 0x1; //Enable counter and make a sw_reset in TC0_CCR0
  *AT91C_TC0_CMR  = 3<<17;////Load counter to A when TIOA falling in (TC0_CMR0) and B when TIOA rising (TCO_CMR0)
  *PIOB_PER = 1<<25;
  *PIOB_OER = 1<<25;
  *PIOB_SODR = 1<<25;  
  
  //Enable the interrupt (NVIC) with the inline declared function
  NVIC_ClearPendingIRQ(TC0_IRQn);                       
  NVIC_SetPriority(TC0_IRQn, 0);
  NVIC_EnableIRQ(TC0_IRQn);
}
/*-----------------------Temp Measurement-------------------------------------*/
void tempMeasure()
{  
   *AT91C_TC0_IER = 1<<6; //enable interrupt TC_IER_LDRBS
   *PIOB_OER = 1<<25;
    *AT91C_TC0_IER = 1<<6; //Reenable interrupts
   *PIOB_CODR = 1<<25; //Set pin as low
   Delay(25);   
   *AT91C_TC0_CCR = 1<<2; //make a sw_reset in TC0_CCR0 
   *PIOB_SODR = 1<<25;
   *PIOB_ODR = 1<<25; //Create a startpuls with a Delay(25)...
  
   Delay(30000); 
}
/*-----------------------TC0 Interrupt handler--------------------------------*/
void TC0_Handler()
{
  *AT91C_TC0_IDR = 1<<6;
  *AT91C_TC0_SR;
  TC_RA = *AT91C_TC0_RA;
  TC_RB = *AT91C_TC0_RB;
  NVIC_ClearPendingIRQ(TC0_IRQn);
  timeL1 = (TC_RB - TC_RA);  
}
/*-----------------------Systick Handler--------------------------------------*/
void SysTick_Handler(void)
{
   print_Display();
}
/*-----------------------Move Cursor------------------------------------------*/
void moveCursor()
{
  Write_Data_2_Display(0x00);
  Write_Data_2_Display(0x00);
  Write_Command_2_Display(0x24);
}
/*-----------------------Initialize ADC---------------------------------------*/
void Init_ADC()
{
  
  *AT91C_PMC_PCER = 1<<11; //Ljuskännaren, PIOA
  *AT91C_PMC_PCER1 = 1<<5; //ADC, PID 37, PMC1
  *PIOA_PER = 3<<3; //Pin 59-60
  *PIOA_ODR = 3<<3;
  *PIOA_PUER = 3<<3;
  *AT91C_ADCC_MR = 1<<9; //Analog-to-digital converter (Mode Register)
   
}
/*-----------------------Measure ADC------------------------------------------*/
void measureADC()
{
  *AT91C_ADCC_IER = 1<<24;  
  *AT91C_ADCC_CHER = 0x2; //ADC Channel 1 enable register
  *AT91C_ADCC_CR = 0x2; //Start an ADC in ADC_CR
  
  while((*AT91C_ADCC_SR &(1<<24)) == 0)
  {
    asm("nop");
  }
  ldrValue = (*AT91C_ADCC_LCDR & 0xFFF);
  Delay(25);   
 // printf("%f\n", ldrValue);
}
/*-----------------------Initialize PWM---------------------------------------*/
void Init_Servo(){

  *AT91C_PMC_PCER = 0x2000;                           //PMC -> PIOC
  *AT91C_PMC_PCER1 = 0x10;                            //PCM -> PWM
  *AT91C_PIOB_PDR = 1<<17; //PIO Disable register
  *AT91C_PIOB_ABMR = 1<<17;
  *AT91C_PWMC_ENA = 1<<1; //Power with modulation controller (Enable)
  *AT91C_PWMC_CH1_CMR = (*AT91C_PWMC_CH1_CMR | 0x5);  //Sync channels mode register 
  *AT91C_PWMC_CH1_CPRDR = 52500; //Channel period register read
  *AT91C_PWMC_CH1_CDTYR = 3938; //Channel duty cycle register 
}
/*---------------------------------Turn Servo---------------------------------*/
void turnServo(int turn){
  //turn = turn/10;
  //1838 min värde
  //6038 max värde
  if(turn == 13){
    return;
  }
  if(turn == 12){
    return;
  }
  if(turn == 11){
    return;
  }
  if(turn == 10){
    return;
  }
  printf("%d\n", turn);
  *AT91C_PWMC_CH1_CDTYUPDR = (1838 + (250*turn)); //Channel duty cycle update register           

  //1838 =low
  //6038 = Neutral
  //3938 = high
  //10 grader = 2088
  //90 grader = 4088
  //180 grader = 6338
  

}
/*-----------------------Main Menu--------------------------------------------*/
void main(void)
{ 
  *AT91C_PMC_PCER = 0x6000;
  *PIOD_PER = 0x5;//Pin 25 and Pin 27
  *PIOC_PER = 0xFF3FC;
  *PIOD_OER = 0x5;//Pin 25 and Pin 27
  *PIOC_OER = 0xFF3FC;
  
  //initPin();
  SystemInit();
  Init_Display();
  Init_Timer();
  Init_ADC();
  Init_Servo();
  SysTick_Config(11999999); //0,5Hz
  while(1)
  {
    Write_Data_2_Display(0x21);
    Write_Command_2_Display(0xC0);
  }
  
    
/*   while(1)
    {
    char voltArr[5];
    measureADC();
    voltage = (((8200/(ldrValue+8200))*3.3));
    printf("%f\n", voltage);
    sprintf(voltArr, "%f", voltage);
    for(int i = 0; i < sizeof(voltArr) ; i++)
    {
          tempChar = (int) (voltArr[i] - 32);
          Write_Data_2_Display(tempChar);
          Write_Command_2_Display(0xC0);

    }
    Write_Data_2_Display(0x36);
    Write_Command_2_Display(0xC0);
    moveCursor();
  }
  
 /* while(1){ 
  char tempArr[5];
  tempMeasure();
  tempCel = ((timeL1/210) - 273.15);
  printf("%f\n", tempCel);
  sprintf(tempArr,"%f" , tempCel);
    for(int i = 0; i < sizeof(tempArr) ; i++)
    {
      tempChar = (int) (tempArr[i] - 32);
      Write_Data_2_Display(tempChar);
      Write_Command_2_Display(0xC0);

    }
      Write_Data_2_Display(0x23);
      Write_Command_2_Display(0xC0);
  moveCursor();
 }*/
}