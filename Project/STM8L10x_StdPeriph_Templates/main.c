#include "stm8l10x.h"

__IO uint32_t TimingDelay = 0;

uint8_t get_address(void)
{
  uint8_t address =0;
  
  I2C_AcknowledgeConfig( ENABLE);
  I2C_GenerateSTART(ENABLE);
  while (!I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECT));
  I2C_Send7bitAddress(0x3a, I2C_Direction_Transmitter);
  while (!I2C_CheckEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
  I2C_SendData(0x0d);
  while (!I2C_CheckEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED));
  I2C_GenerateSTART(ENABLE);
  while (!I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECT));
  I2C_Send7bitAddress(0x3a, I2C_Direction_Receiver);
  while (!I2C_CheckEvent(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));
  I2C_AcknowledgeConfig(DISABLE);
  I2C_GenerateSTOP(ENABLE);
  while (I2C_GetFlagStatus(I2C_FLAG_RXNE) == RESET);
  address = I2C_ReceiveData();
  
  return address;
}

void send_I2C(uint8_t slave_address, uint8_t reg_address, uint8_t data)
{
  I2C_GenerateSTART(ENABLE);
  while (!I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECT));
  I2C_Send7bitAddress(slave_address, I2C_Direction_Transmitter);
  while (!I2C_CheckEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
  I2C_SendData(reg_address);
  while (!I2C_CheckEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED));
  I2C_SendData(data);
  while (!I2C_CheckEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED));
  I2C_GenerateSTOP(ENABLE);
}

void GPIO_Config(void)
{

/* Port A in output push-pull low all pins */
  GPIO_Init(GPIOA,GPIO_Pin_All ,GPIO_Mode_Out_PP_Low_Slow);
	
/* Port B in output push-pull low all pins */
  GPIO_Init(GPIOB, GPIO_Pin_All, GPIO_Mode_Out_PP_Low_Slow);

/* Port C in output push-pull low pins 5-7 */
  GPIO_Init(GPIOC, GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7, GPIO_Mode_Out_PP_Low_Slow);
  
/* Port C in output push-pull high pin 4(led)*/  
  GPIO_Init(GPIOC, GPIO_Pin_4, GPIO_Mode_Out_PP_High_Slow);
  
/* Port C in input floating no interrupt pin 2(vcc) */
  GPIO_Init(GPIOC, GPIO_Pin_2, GPIO_Mode_In_FL_No_IT);
  
/* Port C in input push-pull interrupt pin 3(interrupt) */
  GPIO_Init(GPIOC, GPIO_Pin_3, GPIO_Mode_In_PU_IT);
  
/* Port D in output push-pull low all pins */
  GPIO_Init(GPIOD, GPIO_Pin_All, GPIO_Mode_Out_PP_Low_Slow);

}

void Halt_Init(void)
{
  GPIO_SetBits(GPIOC, GPIO_Pin_4);
  sim();
  halt();
}

void CLK_init(void)
{
  CLK->PCKENR |= 0x04;
}

void TIM4_init(void)
{
  TIM4_DeInit();
  TIM4_TimeBaseInit(TIM4_Prescaler_16384, 0x7A);
  TIM4_ITConfig(TIM4_IT_Update, ENABLE);
  TIM4_Cmd(ENABLE);
}

uint16_t get_X(void)
{
  uint8_t ACCEL_XOUT_L = 0;
  uint8_t ACCEL_XOUT_H = 0;
  uint16_t ACCEL_XOUT = 0;
  
  I2C_AcknowledgeConfig(ENABLE);
  I2C_GenerateSTART(ENABLE);
  while (!I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECT));
  I2C_Send7bitAddress(0x3a, I2C_Direction_Transmitter);
  while (!I2C_CheckEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
  I2C_SendData(0x01);
  while (!I2C_CheckEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED));
  I2C_GenerateSTART(ENABLE);
  while (!I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECT));
  I2C_Send7bitAddress(0x3a, I2C_Direction_Receiver);
  while (!I2C_CheckEvent(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));
  while (!I2C_CheckEvent(I2C_EVENT_MASTER_BYTE_RECEIVED));
  ACCEL_XOUT_H = I2C_ReceiveData();  
  I2C_AcknowledgeConfig(DISABLE);
  I2C_GenerateSTOP(ENABLE);
  while (I2C_GetFlagStatus(I2C_FLAG_RXNE) == RESET);
  ACCEL_XOUT_L = I2C_ReceiveData();   
  
  ACCEL_XOUT = ((ACCEL_XOUT_H<<8)|ACCEL_XOUT_L);
  
  return ACCEL_XOUT;
}

void Setup_MMA8653FC(void)
{
    send_I2C(0x3a, 0x2d, 0x04); //interrupt enable
    send_I2C(0x3a, 0x2b, 0x03); //low power
    //send_I2C(0x3a, 0x0E, 0x02); //range
    send_I2C(0x3a, 0x15, 0x78); //motion detection mode
    send_I2C(0x3a, 0x2c, 0x00); //interrupt pin polarity
    send_I2C(0x3a, 0x2e, 0x04); //interrupt pin select
    send_I2C(0x3a, 0x17, 0x11); //motion threshold
    send_I2C(0x3a, 0x18, 0x01); //motion counter
    send_I2C(0x3a, 0x2a, 0x29); //data rate, active mode on
}

int main( void )
{
  uint8_t address = 0;
  
  GPIO_Config();
  EXTI_SetPinSensitivity(EXTI_Pin_3, EXTI_Trigger_Falling);
 
  CLK_MasterPrescalerConfig(CLK_MasterPrescaler_HSIDiv8);
  CLK_PeripheralClockConfig(CLK_Peripheral_I2C, ENABLE);
  I2C_DeInit();
  I2C_Init(100000, 0, I2C_DutyCycle_2, I2C_Ack_Enable, I2C_AcknowledgedAddress_7bit);
  I2C_Cmd(ENABLE);

  address = get_address();
  
  CLK_init();
  TIM4_init();
  enableInterrupts();

  while(TimingDelay != 1);
  GPIO_ResetBits(GPIOC, GPIO_Pin_4);  
  TimingDelay = 0;
  while(TimingDelay != 1);
  GPIO_SetBits(GPIOC, GPIO_Pin_4); 
  TimingDelay = 0;
  while(TimingDelay != 1);
  GPIO_ResetBits(GPIOC, GPIO_Pin_4);    
  TimingDelay = 0;
  while(TimingDelay != 1);
  GPIO_SetBits(GPIOC, GPIO_Pin_4); 
  Setup_MMA8653FC();
  TimingDelay = 0;
  Halt_Init();
  
  while(1)  
  {
     GPIO_ResetBits(GPIOC, GPIO_Pin_4); 
     if(TimingDelay == 10)
     {
      Halt_Init();
     }
  }
}