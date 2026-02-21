#include "main.h"
TIM_HandleTypeDef htim2;
UART_HandleTypeDef huart1;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
static void MX_USART1_UART_Init(void);

/*函数声明*/
void UART1_Send_Any_Byte(uint8_t *pData,uint16_t len);
uint8_t UART_Receive_With_HeadTail(void);
void Parse_Commend(uint8_t *buf,uint16_t len);
void Dianjispeeddir(void) ;
void Dianjiangle(void);

/*串口参数*/
#define RECV_BUF_MAX 64
#define PACKET_HEAD  0XAA
#define PACKET_TAIL  0X5A
#define RX_TIMEOUT   100
uint8_t g_rev_frame[RECV_BUF_MAX];
uint16_t g_rev_frame_len=0;
char sendchar[]="success";

/*电机参数*/
int16_t dir;          
uint16_t arr = 20; // 错误修正：初始化默认值，避免随机值导致PWM异常
uint16_t crr = 10; // 错误修正：初始化默认值，避免随机值导致PWM异常
uint16_t count=0;
uint16_t maichong;

/*指令解析*/
//0x01 电机调速
//0x02 电机停止
//0x03 电机角度


int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_TIM2_Init();
  MX_USART1_UART_Init();
 
   HAL_GPIO_WritePin(GPIOA,GPIO_PIN_1,GPIO_PIN_RESET);
	 HAL_GPIO_WritePin(GPIOA,GPIO_PIN_2,GPIO_PIN_RESET);
	 HAL_GPIO_WritePin(GPIOA,GPIO_PIN_3,GPIO_PIN_RESET);//第3个led对应一个转向
	 HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_RESET);//第4个led对应一个转向
	 HAL_GPIO_WritePin(GPIOA,GPIO_PIN_5,GPIO_PIN_RESET);//每一次停下，闪烁一次
	 HAL_GPIO_WritePin(GPIOA,GPIO_PIN_6,GPIO_PIN_RESET);//对应的精准转几圈，就闪烁几次
	
	while (1)
  {
		if( UART_Receive_With_HeadTail()==1)
		{
		 Parse_Commend(g_rev_frame,g_rev_frame_len);
		 UART1_Send_Any_Byte((uint8_t *)sendchar, strlen(sendchar)); // 错误修正：字符串强制转换为uint8_t*，原写法是单字节转换
		
		// 错误修正：原循环每次重置len，导致只清空第一个字节，调整清空逻辑
		for(uint16_t i=0;i<g_rev_frame_len;i++)
     {
		   g_rev_frame[i]=0;
		 }
		 g_rev_frame_len=0;			
			
		}
		
		
  }
}




/*串口发送任意字节函数*/
void UART1_Send_Any_Byte(uint8_t *pData,uint16_t len)
{
 HAL_UART_Transmit(&huart1,pData,len,500);
}

//调用示例：
//uint8_t send[]={0x01,0x0a,0xaa};
// UART1_Send_Any_Byte(send,sizeof(send));
//char send[]="susccess"
// UART1_Send_Any_Byte((uint8_t)send, strlen(send));

/*串口接收任意字节函数*/
uint8_t UART_Receive_With_HeadTail(void)
{
uint8_t tempbyte;
uint8_t recv_start=0;
g_rev_frame_len=0;
	
// 错误修正：原逻辑只读取1个字节，后续无读取，调整为循环内读取
while(1)
{
 if(HAL_UART_Receive(&huart1,&tempbyte,1,RX_TIMEOUT)!=HAL_OK)
 {
  recv_start=0;
  g_rev_frame_len=0;
  return 0;
 }

 if(recv_start==0)
 {
  if(tempbyte==PACKET_HEAD)
  {
	 recv_start=1;
	 continue;
	}		
 }
  
  else
	{
	 if(tempbyte==PACKET_TAIL)
	 {
	  break;
	 }
	 if(g_rev_frame_len<RECV_BUF_MAX-1)
	 {
	  g_rev_frame[g_rev_frame_len]=tempbyte;
		g_rev_frame_len=g_rev_frame_len+1;
	 }
	 else
	 {
	  recv_start=0; g_rev_frame_len=0; return 0;
	 }
	}
}
	
return (g_rev_frame_len >0)?1:0;
}

/*任务解析函数*/
void Parse_Commend(uint8_t *buf,uint16_t len)
{
 if(len<1)
 return;
 switch(buf[0])
 {
	 case 0x01: //控制转速与转向 01代表进入这个任务，04和03分别对应一个转向
	 {
	    // 错误修正：增加长度判断，防止buf[1]/buf[2]越界
	    if(len < 3)
	    {
	      UART1_Send_Any_Byte((uint8_t *)"param err",8);
	      return;
	    }
	    if(buf[1]==0x04)
			{
			 HAL_GPIO_WritePin(GPIOA,GPIO_PIN_2,GPIO_PIN_RESET);
			 HAL_GPIO_WritePin(GPIOA,GPIO_PIN_3,GPIO_PIN_RESET);
			 crr=buf[2]; arr=crr*2;
			 Dianjispeeddir();  
			 HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET); // 亮
       HAL_Delay(500);
       HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET); // 灭
      
			}
			if(buf[1]==0x05)
			{
			 HAL_GPIO_WritePin(GPIOA,GPIO_PIN_2,GPIO_PIN_SET);
			 HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_RESET);
			 crr=buf[2]; arr=crr*2;
			 Dianjispeeddir();  
			 HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET); // 亮
       HAL_Delay(500);
       HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET); // 灭
			}
	 }
   break;
	 
	 case 0x02:  //停止电机，同时让电机恢复原速度
	 {    
		    __HAL_TIM_SET_AUTORELOAD(&htim2, 20);              
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 10); 
	      HAL_GPIO_WritePin(GPIOA,GPIO_PIN_1,GPIO_PIN_SET);
		    HAL_TIM_PWM_Stop(&htim2,TIM_CHANNEL_1);
		    // 错误修正：停止定时器中断，防止角度控制的中断残留
		    HAL_TIM_Base_Stop_IT(&htim2);
		    count = 0;
		    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET); // 亮
        HAL_Delay(500);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET); // 灭
	 }
	 break;
	 
	 case 0x03:  //输入指定脉冲指令，让电机转动对应角度
	 {
	     // 错误修正：增加长度判断，防止buf[1]越界
	     if(len < 2)
	     {
	       UART1_Send_Any_Byte((uint8_t *)"angle err",9);
	       return;
	     }
	       uint16_t m=buf[1];
		     maichong=m*100;
		     Dianjiangle();
			   
	 }
	 break;
	 
	 default:
	 UART1_Send_Any_Byte((uint8_t *)"error",5);
	 return ;
 }
}

/*电机转速转向控制*/
void Dianjispeeddir(void)  
{
	  // 错误修正：停止可能残留的中断，重置计数
    HAL_TIM_Base_Stop_IT(&htim2);
    count = 0;
	  
	  //先启动PWM输出，确保电机先转起来
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    HAL_GPIO_WritePin(GPIOA,GPIO_PIN_1,GPIO_PIN_RESET);
	
    __HAL_TIM_SET_AUTORELOAD(&htim2, arr);              
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, crr); 
    
    // 兜底：确保定时器是运行状态（避免中断影响）
    HAL_TIM_Base_Start(&htim2);
}

/*电机指定角度控制*/
void Dianjiangle(void)
{
  count = 0; 
  
  // 错误修正：ARR/CRR兜底，防止未调速时为0导致定时器异常
  uint16_t temp_arr = (arr == 0) ? 20 : arr;
  uint16_t temp_crr = (crr == 0) ? 10 : crr;
  
  // 强制设置PWM参数（优先级最高，覆盖随机值）
  __HAL_TIM_SET_AUTORELOAD(&htim2, temp_arr);    // 用初始化好的ARR值
  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, temp_crr); // 固定占空比50%
  
  // 先启动PWM输出，再启动定时器中断（顺序关键，确保脉冲能发出去）
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1); 
  HAL_TIM_Base_Start_IT(&htim2);
}  

/*对应定时器的中断函数*/
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
 if(htim->Instance==TIM2)
 {
   count+=1;
   if(count>=maichong)
	 //达到指定脉冲后停止输出pwm，停止中断
   {
    HAL_TIM_PWM_Stop(&htim2,TIM_CHANNEL_1);
    HAL_TIM_Base_Stop_IT(&htim2);
    count = 0;
    // 新增：还原m值（总脉冲数/100），用于LED闪烁次数
    uint16_t m = maichong / 100;
    // 新增：每圈亮1次（5圈亮5次），替代亮灭循环
    for(uint16_t n=0; n<m; n++)
    {
      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET); // 亮
      for(uint32_t i=0; i<500000; i++); // 延时
      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET); // 灭
      for(uint32_t i=0; i<200000; i++); // 短延时，避免视觉重叠
    }
    // 新增：闪烁完成后强制熄灭LED
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
   }
 }
}

























void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 7200;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 100;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 50;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4
                          |GPIO_PIN_5|GPIO_PIN_6, GPIO_PIN_RESET);

  /*Configure GPIO pins : PA1 PA2 PA3 PA4
                           PA5 */
  GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4
                          |GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PA6 */
  GPIO_InitStruct.Pin = GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
