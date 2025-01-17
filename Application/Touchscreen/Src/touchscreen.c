/**
 ******************************************************************************
 * @file    BSP/Src/touchscreen.c
 * @author  MCD Application Team
 * @brief   This example code shows how to use the touchscreen driver.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2016 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "user_lcd.h"
#include "flash_persistence.h"
/** @addtogroup STM32F7xx_HAL_Examples
 * @{
 */

/** @addtogroup BSP
 * @{
 */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define  CIRCLE_RADIUS        40
#define  LINE_LENGHT          30
/* Private macro -------------------------------------------------------------*/
#define  CIRCLE_XPOS(i)       ((i * BSP_LCD_GetXSize()) / 5)
#define  CIRCLE_YPOS(i)       (BSP_LCD_GetYSize() - CIRCLE_RADIUS - 40)

uint32_t sampleCounter = 20;
uint8_t togglePaint = 1;

uint32_t initStatus = TS_ERROR;

/* Private Structures and Enumerations ------------------------------------------------------------*/
/* Possible allowed indexes of touchscreen demo */
typedef enum
{
  TOUCHSCREEN_DEMO_1 = 1,
  TOUCHSCREEN_DEMO_2 = 2,
  TOUCHSCREEN_DEMO_3 = 3,
  TOUCHSCREEN_DEMO_MAX = TOUCHSCREEN_DEMO_3,

} TouchScreenDemoTypeDef;

/* Global variables ---------------------------------------------------------*/
TS_StateTypeDef TS_State = {0};

/* Private variables ---------------------------------------------------------*/
/* Static variable holding the current touch color index : used to change color at each touch */
//static uint32_t touchscreen_color_idx = 0;

/* Private function prototypes -----------------------------------------------*/
//static void     Touchscreen_SetHint_Demo(TouchScreenDemoTypeDef demoIndex);
void                    Touchscreen_DrawBackground_Circles(uint8_t state);
#if (TS_MULTI_TOUCH_SUPPORTED == 1)
static uint32_t Touchscreen_Handle_NewTouch(void);
#endif // TS_MULTI_TOUCH_SUPPORTED == 1
/* Private functions ---------------------------------------------------------*/
extern int16_t frequencies[];
extern int16_t bandwidths[];
extern uint32_t divider;
extern bool shouldPrintSamples;
extern bool shouldApplyFilter;
bool areInitialCirclesDrawn = false;

#define CIRCLE_BUTTON_DEBOUNCE_TIMER 100
uint32_t yOffset = 0;
uint32_t xOffset = 150;

void Touchscreen_Init(void)
{
  initStatus = BSP_TS_Init(BSP_LCD_GetXSize(), BSP_LCD_GetYSize());
}

void Touchscreen_ButtonHandler(void)
{
  uint16_t touchXPosition, touchYPosition;
//	uint8_t state = 0;
//	uint8_t exitTsUseCase = 0;

  if(initStatus != TS_OK)
    return;

  initStatus = BSP_TS_GetState(&TS_State);

  if(!TS_State.touchDetected)
  {
    for(uint8_t i = 0; i < NUMBER_OF_CIRCLE_BUTTONS; i++) 
    {
      if(++circleButtons[i].debounceTimer > CIRCLE_BUTTON_DEBOUNCE_TIMER)
        circleButtons[i].debounceTimer = CIRCLE_BUTTON_DEBOUNCE_TIMER;
    }
    return;
  }

  touchXPosition = TS_State.touchX[0];
  touchYPosition = TS_State.touchY[0];

  for(uint8_t i = 0; i < NUMBER_OF_CIRCLE_BUTTONS; i++) 
  {
    if((touchYPosition > circleButtons[i].y - circleButtons[i].radius) && (touchYPosition < circleButtons[i].y + circleButtons[i].radius))
    {
      if((touchXPosition > circleButtons[i].x - circleButtons[i].radius) && (touchXPosition < circleButtons[i].x + circleButtons[i].radius))
      {
        
        if(circleButtons[i].debounceTimer == CIRCLE_BUTTON_DEBOUNCE_TIMER)
          LCD_UpdateButton(i, !circleButtons[i].isPressed, false);

        circleButtons[i].debounceTimer = 0;

        return;
      }
    }
  }

  if(circleButtons[0].isActive)
  {  
    if(touchYPosition > saveButton.y && touchYPosition < saveButton.y + saveButton.height)
    {
      if(touchXPosition > saveButton.x && touchXPosition < saveButton.x + saveButton.width)
      {
        if(!saveButton.isPressed)
        {
          saveButton.isPressed = true;
          saveButton.isActive = true;
          resetButton.isPressed = false;
          resetButton.isActive = false;
          undoButton.isPressed = false;
          undoButton.isActive = false;
          LCD_UpdateRectangleButton(&saveButton);
          LCD_UpdateRectangleButton(&undoButton);
          LCD_UpdateRectangleButton(&resetButton);
          FlashPersistence_Write();
        }

        return;
      }
    }  
    else if(touchYPosition > undoButton.y && touchYPosition < undoButton.y + undoButton.height)
    {
      if(touchXPosition > undoButton.x && touchXPosition < saveButton.x + undoButton.width)
      {
        if(!undoButton.isPressed)
        {
          saveButton.isPressed = false;
          saveButton.isActive = false;
          undoButton.isPressed = true;
          undoButton.isActive = true;
          resetButton.isPressed = false;
          resetButton.isActive = false;
          LCD_UpdateRectangleButton(&saveButton);
          LCD_UpdateRectangleButton(&undoButton);
          LCD_UpdateRectangleButton(&resetButton);
          for(uint8_t i = 0; i < NUMBER_OF_SLIDER_BUTTONS; i++)
            LCD_DisplayKnob(i, FlashPersistence_Read(i));
        }

        return;
      }
    }
    else if(touchYPosition > resetButton.y && touchYPosition < resetButton.y + resetButton.height)
    {
      if(touchXPosition > resetButton.x && touchXPosition < resetButton.x + resetButton.width)
      {
        if(!resetButton.isPressed)
        {
          saveButton.isPressed = false;
          saveButton.isActive = false;
          undoButton.isPressed = false;
          undoButton.isActive = false;
          resetButton.isPressed = true;
          resetButton.isActive = true;
          LCD_UpdateRectangleButton(&saveButton);
          LCD_UpdateRectangleButton(&undoButton);
          LCD_UpdateRectangleButton(&resetButton);
          for(uint8_t i = 0; i < NUMBER_OF_SLIDER_BUTTONS; i++)
          {
            AudioUserDsp_BiquadFilterConfig(&biquadFilters[i], 0, frequencies[i], bandwidths[i]);
            LCD_DisplayKnob(i, LCD_TranslateGainToKnobPosition(i, 0));
          }
        }

        return;
      }
    }

    for(uint8_t i = 0; i < NUMBER_OF_SLIDER_BUTTONS; i++)
    {
      if((touchYPosition > sliderKnobs[i].sliderY + 10) && (touchYPosition < sliderKnobs[i].sliderY + sliderKnobs[i].sliderHeight - 10))
      {
        if((touchXPosition > sliderKnobs[i].sliderX) && (touchXPosition < sliderKnobs[i].sliderX + sliderKnobs[i].sliderWidth))
        {
          if(++sliderKnobs[i].debounceCount >= sliderKnobs[i].debouceLimit)
          {
            LCD_DisplayKnob(i, touchYPosition);
            sliderKnobs[i].isPressed = true;
            sliderKnobs[i].debounceCount = 0;

            resetButton.isPressed = false;
            saveButton.isPressed = false;
            undoButton.isPressed = false;
            
            if(resetButton.isActive)
            {
              resetButton.isActive = false;
              LCD_UpdateRectangleButton(&resetButton);
            }
            if(saveButton.isActive)
            {
              saveButton.isActive = false;
              LCD_UpdateRectangleButton(&saveButton);
            }
            if(undoButton.isActive)
            {
              undoButton.isActive = false;
              LCD_UpdateRectangleButton(&undoButton);
            } 
            return;
          }
        }
      }
    }
  }

}



/**
 * @brief  Touchscreen Demo1 : test touchscreen calibration and single touch in polling mode
 * @param  None
 * @retval None
 */
void Touchscreen_demo1(void)
{
  uint16_t x1, y1;
  uint8_t state = 0;
  uint8_t exitTsUseCase = 0;
  uint32_t ts_status = TS_OK;

  /* Reset touch data information */
//  BSP_TEST_APPLI_ASSERT(BSP_TS_ResetTouchData(&TS_State));

  /* Touchscreen initialization */
  ts_status = BSP_TS_Init(BSP_LCD_GetXSize(), BSP_LCD_GetYSize());

  if(ts_status == TS_OK)
  {
    /* Display touch screen demo description */
    // Touchscreen_SetHint_Demo(TOUCHSCREEN_DEMO_1);
    // Touchscreen_DrawBackground_Circles(state);

    while (exitTsUseCase == 0)
    {
      char desc[50];
      // // BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize()/2, (uint8_t *)desc, CENTER_MODE);
      // BSP_LCD_SetTextColor(LCD_COLOR_BLUE);


      if(sampleCounter < 10)
      {
        BSP_LCD_DisplayStringAt(0, yOffset, (uint8_t *)desc, CENTER_MODE);
        yOffset += 18;
        // if(yOffset > BSP_LCD_GetYSize())
        // {
        // 	yOffset = 0;
        // 	xOffset += 350;
        // }

        // if(xOffset > 500)
        // 	xOffset = 150;

        sampleCounter++;

      }

      if (ts_status == TS_OK)
      {
        /* Check in polling mode in touch screen the touch status and coordinates */
        /* of touches if touch occurred                                           */
        ts_status = BSP_TS_GetState(&TS_State);
        if(TS_State.touchDetected)
        {
          /* One or dual touch have been detected          */
          /* Only take into account the first touch so far */

          /* Get X and Y position of the first touch post calibrated */
          x1 = TS_State.touchX[0];
          y1 = TS_State.touchY[0];

          if ((y1 > (CIRCLE_YPOS(1) - CIRCLE_RADIUS)) &&
              (y1 < (CIRCLE_YPOS(1) + CIRCLE_RADIUS)))
          {
            if ((x1 > (CIRCLE_XPOS(1) - CIRCLE_RADIUS)) &&
                (x1 < (CIRCLE_XPOS(1) + CIRCLE_RADIUS)))
            {
              if ((state & 1) == 0)
              {
                Touchscreen_DrawBackground_Circles(state);

                BSP_LCD_SetTextColor(togglePaint == 1 ? LCD_COLOR_LIGHTRED : LCD_COLOR_WHITE);
                togglePaint = !togglePaint;
                BSP_LCD_FillCircle(CIRCLE_XPOS(1), CIRCLE_YPOS(1), CIRCLE_RADIUS);
                state = 1;
              }
            }
            if ((x1 > (CIRCLE_XPOS(2) - CIRCLE_RADIUS)) &&
                (x1 < (CIRCLE_XPOS(2) + CIRCLE_RADIUS)))
            {
              if ((state & 2) == 0)
              {
                Touchscreen_DrawBackground_Circles(state);
                BSP_LCD_SetTextColor(LCD_COLOR_RED);
                BSP_LCD_FillCircle(CIRCLE_XPOS(2), CIRCLE_YPOS(2), CIRCLE_RADIUS);
                state = 2;
              }
            }

            if ((x1 > (CIRCLE_XPOS(3) - CIRCLE_RADIUS)) &&
                (x1 < (CIRCLE_XPOS(3) + CIRCLE_RADIUS)))
            {
              if ((state & 4) == 0)
              {
                Touchscreen_DrawBackground_Circles(state);
                BSP_LCD_SetTextColor(LCD_COLOR_YELLOW);
                BSP_LCD_FillCircle(CIRCLE_XPOS(3), CIRCLE_YPOS(3), CIRCLE_RADIUS);
                state = 4;
              }
            }

            if ((x1 > (CIRCLE_XPOS(4) - CIRCLE_RADIUS)) &&
                (x1 < (CIRCLE_XPOS(4) + CIRCLE_RADIUS)))
            {
              if ((state & 8) == 0)
              {
                Touchscreen_DrawBackground_Circles(state);
                BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
                BSP_LCD_FillCircle(CIRCLE_XPOS(4), CIRCLE_YPOS(3), CIRCLE_RADIUS);
                state = 8;
              }
            }
          }

        } /* of if(TS_State.TouchDetected) */

      } /* of if (ts_status == TS_OK) */

      /* Wait for a key button press to switch to next test case of BSP validation application */
      /* Otherwise stay in the test */
//      exitTsUseCase = CheckForUserInput();

      HAL_Delay(20);

    } /* of while (exitTsUseCase == 0) */

  } /* of if(status == TS_OK) */
}

#if (TS_MULTI_TOUCH_SUPPORTED == 1)
/**
 * @brief  Touchscreen Demo2 : test touchscreen single and dual touch in polling mode
 * @param  None
 * @retval None
 */
void Touchscreen_demo2(void)
{
  uint8_t exitTsUseCase = 0;
  uint32_t ts_status = TS_OK;

  /* Reset touch data information */
  BSP_TEST_APPLI_ASSERT(BSP_TS_ResetTouchData(&TS_State));

  /* Touchscreen initialization */
  ts_status = BSP_TS_Init(BSP_LCD_GetXSize(), BSP_LCD_GetYSize());

  if(ts_status == TS_OK)
  {
    /* Display touch screen demo2 description */
    Touchscreen_SetHint_Demo(TOUCHSCREEN_DEMO_2);

    while (exitTsUseCase == 0)
    {
      Touchscreen_Handle_NewTouch();

      /* Wait for a key button press to switch to next test case of BSP validation application */
      /* Otherwise stay in the test */
      exitTsUseCase = CheckForUserInput();

      HAL_Delay(100);

    } /* of while (exitTsUseCase == 0) */

  } /* of if(status == TS_OK) */
}

/**
 * @brief  Touchscreen_Handle_NewTouch: factorization of touch management
 * @param  None
 * @retval TS_OK or TS_ERROR
 */
static uint32_t Touchscreen_Handle_NewTouch(void)
{
#define TS_MULTITOUCH_FOOTPRINT_CIRCLE_RADIUS 15
#define TOUCH_INFO_STRING_SIZE                70
  uint16_t x1 = 0;
  uint16_t y1 = 0;
  uint16_t x2 = 0;
  uint16_t y2 = 0;
  uint32_t drawTouch1 = 0; /* activate/deactivate draw of footprint of touch 1 */
  uint32_t drawTouch2 = 0; /* activate/deactivate draw of footprint of touch 2 */
  uint32_t colors[24] = {LCD_COLOR_BLUE, LCD_COLOR_GREEN, LCD_COLOR_RED, LCD_COLOR_CYAN, LCD_COLOR_MAGENTA, LCD_COLOR_YELLOW,
                   LCD_COLOR_LIGHTBLUE, LCD_COLOR_LIGHTGREEN, LCD_COLOR_LIGHTRED, LCD_COLOR_LIGHTCYAN, LCD_COLOR_LIGHTMAGENTA,
                   LCD_COLOR_LIGHTYELLOW, LCD_COLOR_DARKBLUE, LCD_COLOR_DARKGREEN, LCD_COLOR_DARKRED, LCD_COLOR_DARKCYAN,
                   LCD_COLOR_DARKMAGENTA, LCD_COLOR_DARKYELLOW, LCD_COLOR_LIGHTGRAY, LCD_COLOR_GRAY, LCD_COLOR_DARKGRAY,
                   LCD_COLOR_BLACK, LCD_COLOR_BROWN, LCD_COLOR_ORANGE };
  uint32_t ts_status = TS_OK;
  uint8_t lcd_string[TOUCH_INFO_STRING_SIZE] = "";

  /* Check in polling mode in touch screen the touch status and coordinates */
  /* of touches if touch occurred                                           */
  ts_status = BSP_TS_GetState(&TS_State);
  if(TS_State.touchDetected)
  {
    /* One or dual touch have been detected  */

    /* Erase previous information on touchscreen play pad area */
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
    BSP_LCD_FillRect(0, 80, BSP_LCD_GetXSize(), BSP_LCD_GetYSize() - 160);

    /* Re-Draw touch screen play area on LCD */
    BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
    BSP_LCD_DrawRect(10, 90, BSP_LCD_GetXSize() - 20, BSP_LCD_GetYSize() - 180);
    BSP_LCD_DrawRect(11, 91, BSP_LCD_GetXSize() - 22, BSP_LCD_GetYSize() - 182);

    /* Erase previous information on bottom text bar */
    BSP_LCD_FillRect(0, BSP_LCD_GetYSize() - 80, BSP_LCD_GetXSize(), 80);

    /* Desactivate drawing footprint of touch 1 and touch 2 until validated against boundaries of touch pad values */
    drawTouch1 = drawTouch2 = 0;

    /* Get X and Y position of the first touch post calibrated */
    x1 = TS_State.touchX[0];
    y1 = TS_State.touchY[0];

    if((y1 > (90 + TS_MULTITOUCH_FOOTPRINT_CIRCLE_RADIUS)) &&
       (y1 < (BSP_LCD_GetYSize() - 90 - TS_MULTITOUCH_FOOTPRINT_CIRCLE_RADIUS)))
    {
      drawTouch1 = 1;
    }

    /* If valid touch 1 position : inside the reserved area for the use case : draw the touch */
    if(drawTouch1 == 1)
    {
      /* Draw circle of first touch : turn on colors[] table */
      BSP_LCD_SetTextColor(colors[(touchscreen_color_idx++ % 24)]);
      BSP_LCD_FillCircle(x1, y1, TS_MULTITOUCH_FOOTPRINT_CIRCLE_RADIUS);

      BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
      BSP_LCD_SetFont(&Font16);
      BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize() - 70, (uint8_t *)"TOUCH INFO : ", CENTER_MODE);

      BSP_LCD_SetFont(&Font12);
      sprintf((char*)lcd_string, "x1 = %d, y1 = %d, Event = %s, Weight = %d",
              x1,
              y1,
              ts_event_string_tab[TS_State.touchEventId[0]],
              TS_State.touchWeight[0]);
      BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize() - 45, lcd_string, CENTER_MODE);
    } /* of if(drawTouch1 == 1) */

    if(TS_State.touchDetected > 1)
    {
      /* Get X and Y position of the second touch post calibrated */
      x2 = TS_State.touchX[1];
      y2 = TS_State.touchY[1];

      if((y2 > (90 + TS_MULTITOUCH_FOOTPRINT_CIRCLE_RADIUS)) &&
         (y2 < (BSP_LCD_GetYSize() - 90 - TS_MULTITOUCH_FOOTPRINT_CIRCLE_RADIUS)))
      {
        drawTouch2 = 1;
      }

      /* If valid touch 2 position : inside the reserved area for the use case : draw the touch */
      if(drawTouch2 == 1)
      {
        sprintf((char*)lcd_string, "x2 = %d, y2 = %d, Event = %s, Weight = %d",
                x2,
                y2,
                ts_event_string_tab[TS_State.touchEventId[1]],
                TS_State.touchWeight[1]);
        BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize() - 35, lcd_string, CENTER_MODE);

        /* Draw circle of second touch : turn on color[] table */
        BSP_LCD_SetTextColor(colors[(touchscreen_color_idx++ % 24)]);
        BSP_LCD_FillCircle(x2, y2, TS_MULTITOUCH_FOOTPRINT_CIRCLE_RADIUS);
      } /* of if(drawTouch2 == 1) */

    } /* of if(TS_State.touchDetected > 1) */

    if((drawTouch1 == 1) || (drawTouch2 == 1))
    {
      /* Get updated gesture Id in global variable 'TS_State' */
      ts_status = BSP_TS_Get_GestureId(&TS_State);

      sprintf((char*)lcd_string, "Gesture Id = %s", ts_gesture_id_string_tab[TS_State.gestureId]);
      BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
      BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize() - 15, lcd_string, CENTER_MODE);
    }
    else
    {
      /* Invalid touch position */
      BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
      BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize() - 70, (uint8_t *)"Invalid touch position : use drawn touch area : ", CENTER_MODE);
    }
  } /* of if(TS_State.TouchDetected) */

  return(ts_status);
}

#endif // TS_MULTI_TOUCH_SUPPORTED == 1

/**
 * @brief  TouchScreen get touch position
 * @param  None
 * @retval None
 */
uint8_t TouchScreen_GetTouchPosition(void)
{
  uint16_t x1, y1;
  uint8_t circleNr = 0;

  /* Check in polling mode in touch screen the touch status and coordinates */
  /* of touches if touch occurred                                           */
  BSP_TS_GetState(&TS_State);
  if(TS_State.touchDetected)
  {
    /* One or dual touch have been detected          */
    /* Only take into account the first touch so far */

    /* Get X and Y position of the first */
    x1 = TS_State.touchX[0];
    y1 = TS_State.touchY[0];

    if ((y1 > (CIRCLE_YPOS(1) - CIRCLE_RADIUS)) &&
        (y1 < (CIRCLE_YPOS(1) + CIRCLE_RADIUS)))
    {
      if ((x1 > (CIRCLE_XPOS(2) - CIRCLE_RADIUS)) &&
          (x1 < (CIRCLE_XPOS(2) + CIRCLE_RADIUS)))
      {
        sampleCounter = 0;
        yOffset = 0;
        circleNr = 1;
      }

      if ((x1 > (CIRCLE_XPOS(3) - CIRCLE_RADIUS)) &&
          (x1 < (CIRCLE_XPOS(3) + CIRCLE_RADIUS)))
      {
        circleNr = 2;
      }
    }
    else
    {
      if (((y1 < 220) && (y1 > 140)) &&
          ((x1 < 160) && (x1 > 100)))
      {
        circleNr = 0xFE; /* top part of the screen */
      }
      else
      {
        circleNr = 0xFF;
      }
    }
  } /* of if(TS_State.TouchDetected) */
  return circleNr;
}

/**
 * @}
 */

/**
 * @}
 */
