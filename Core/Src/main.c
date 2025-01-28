#include "main.h"
#include "ssd1306.h"
#include "ssd1306_tests.h"
#include "ssd1306_fonts.h"
#include "sx1509.h"
#include <stdbool.h>

I2C_HandleTypeDef hi2c1;
SPI_HandleTypeDef hspi1;
UART_HandleTypeDef huart2;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);
void HAL_UART_RxCpltCallback(UART_HandleTypeDef * huart);

#define PLAYER_WIDTH 8
#define PLAYER_HEIGHT 8
#define BULLET_WIDTH 1
#define BULLET_HEIGHT 3
#define ALIEN_WIDTH 8
#define ALIEN_HEIGHT 8
#define NUM_ALIENS_PER_ROW 6
#define ALIEN_ROWS 3
#define PLAYER_LIVES 3
#define SHIELD_WIDTH 12
#define SHIELD_HEIGHT 2
#define NUM_SHIELDS 3
#define ALIEN_FIRE_SPEED 5
#define ALIEN_SPEED_TICKS 2

typedef struct {
    int x;
    int y;
    bool alive;
    int fireCooldown;
}
Alien;

typedef struct {
    int x;
    int y;
    bool active;
}
Bullet;

typedef struct {
    bool pixels[SHIELD_WIDTH][SHIELD_HEIGHT];
    int x;
    int y;
}
Shield;

Alien aliens[ALIEN_ROWS][NUM_ALIENS_PER_ROW];
Bullet bullet;
Bullet alienBullet;
bool bulletActive = false;
bool alienBulletActive = false;
int playerX = 60;
int playerY = 56;
int alienDirection = 1;
bool gameOver = false;
bool playerWon = false;
int playerLives = PLAYER_LIVES;

int alienFireTimer = 0;

Shield shields[NUM_SHIELDS];

void initializeAliens(void);
void moveAliens(void);
void drawGame(void);
void checkCollisions(void);
void shootBullet(void);
void handleInput(void);
void alienShoot(void);
void checkAlienBulletCollision(void);
void handlePlayerDeath(void);
void drawShields(void);
void initShields(void);
void checkShieldCollisions(int bulletX, int bulletY, bool isAlienBullet);
void destroyShieldPixel(int shieldIndex, int x, int y);

void initializeAliens(void) {
    for (int row = 0; row < ALIEN_ROWS; row++) {
        for (int i = 0; i < NUM_ALIENS_PER_ROW; i++) {
            aliens[row][i].x = 30 + i * (ALIEN_WIDTH + 2);
            aliens[row][i].y = 3 + row * (ALIEN_HEIGHT + 2);
            aliens[row][i].alive = true;
            aliens[row][i].fireCooldown = 0;
        }
    }
}
void initShields() {
    int startX = 20;
    for (int i = 0; i < NUM_SHIELDS; ++i) {
        shields[i].x = startX + i * 35;
        shields[i].y = 52;
        for (int x = 0; x < SHIELD_WIDTH; x++) {
            for (int y = 0; y < SHIELD_HEIGHT; y++) {
                shields[i].pixels[x][y] = true;
            }
        }
    }
}
void drawShields(void) {
    for (int i = 0; i < NUM_SHIELDS; ++i) {
        for (int x = 0; x < SHIELD_WIDTH; ++x) {
            for (int y = 0; y < SHIELD_HEIGHT; ++y) {
                if (shields[i].pixels[x][y]) {
                    ssd1306_DrawPixel(shields[i].x + x, shields[i].y + y, White);
                }
            }
        }
    }
}

float alienMoveOffset = 0.0f;
int alienTickCounter = 0;

void moveAliens(void) {
    alienTickCounter++;
    if (alienTickCounter < ALIEN_SPEED_TICKS) {
        return;
    }
    alienTickCounter = 0;

    bool changeDirection = false;

    for (int row = 0; row < ALIEN_ROWS; row++) {
        for (int i = 0; i < NUM_ALIENS_PER_ROW; i++) {
            if (aliens[row][i].alive) {
                aliens[row][i].x += alienDirection;

                if (aliens[row][i].x < 10) {
                    aliens[row][i].x = 10;
                    changeDirection = true;
                } else if (aliens[row][i].x + ALIEN_WIDTH > 110) {
                    aliens[row][i].x = 110 - ALIEN_WIDTH;
                    changeDirection = true;
                }
            }
        }
    }

    if (changeDirection) {
        alienDirection = -alienDirection;

        for (int row = 0; row < ALIEN_ROWS; row++) {
            for (int i = 0; i < NUM_ALIENS_PER_ROW; i++) {
                if (aliens[row][i].alive) {
                    aliens[row][i].y += ALIEN_HEIGHT / 2;
                    if (aliens[row][i].y >= 52 - ALIEN_HEIGHT) {
                        gameOver = true;
                        return;
                    }
                }
            }
        }
    }
}

int score = 0;
void checkCollisions(void) {
    if (bulletActive) {
        for (int row = 0; row < ALIEN_ROWS; row++) {
            for (int i = 0; i < NUM_ALIENS_PER_ROW; i++) {
                if (aliens[row][i].alive &&
                    bullet.x < aliens[row][i].x + ALIEN_WIDTH &&
                    bullet.x + BULLET_WIDTH > aliens[row][i].x &&
                    bullet.y < aliens[row][i].y + ALIEN_HEIGHT &&
                    bullet.y + BULLET_HEIGHT > aliens[row][i].y) {

                    aliens[row][i].alive = false;
                    score += 100;
                    bulletActive = false;
                    return;
                }
            }
        }

        if (bullet.y <= 0) {
            bulletActive = false;
            return;
        }
        checkShieldCollisions(bullet.x, bullet.y, false);
    }
}

void checkShieldCollisions(int bulletX, int bulletY, bool isAlienBullet) {
    for (int bx = 0; bx < BULLET_WIDTH; bx++) {
        for (int by = 0; by < BULLET_HEIGHT; by++) {
            int currentX = bulletX + bx;
            int currentY = bulletY + by;

            for (int i = 0; i < NUM_SHIELDS; i++) {
                if (currentX >= shields[i].x &&
                    currentX < shields[i].x + SHIELD_WIDTH &&
                    currentY >= shields[i].y &&
                    currentY < shields[i].y + SHIELD_HEIGHT)
                {
                    int localX = currentX - shields[i].x;
                    int localY = currentY - shields[i].y;

                    if(shields[i].pixels[localX][localY] == true) {
                        destroyShieldPixel(i, localX, localY);

                        if (!isAlienBullet) bulletActive = false;
                        else alienBulletActive = false;

                    }

                    return;
                }
            }
        }
    }
}

void destroyShieldPixel(int shieldIndex, int x, int y) {
    if (x >= 0 && x < SHIELD_WIDTH && y >= 0 && y < SHIELD_HEIGHT) {
        shields[shieldIndex].pixels[x][y] = false;
        if (y - 1 >= 0) {
            shields[shieldIndex].pixels[x][y - 1] = false;
        }
        if (y + 1 < SHIELD_HEIGHT) {
            shields[shieldIndex].pixels[x][y + 1] = false;
        }
    }
}
const unsigned char ufo_bitmap[] = {
    0x18,
    0x3C,
    0x7E,
    0xDB,
    0xDB,
    0x7E,
    0x24,
    0x42
};

const unsigned char ufo_alternative[] = {
    0x18,
    0x3C,
    0x7E,
    0xDB,
    0xDB,
    0x7E,
    0x24,
    0x18
};

unsigned char player_bitmap[] = {
    0x18,
    0x18,
    0x3C,
    0x3C,
    0x7E,
    0xFF,
    0xFF,
    0xFF
};

void drawHealth(void) {
    char lives_str[4];
    sprintf(lives_str, "%d", playerLives);
    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString(lives_str, Font_6x8, White);
}

bool animationFlag = false;
void drawGame(void) {
    ssd1306_Fill(Black);

    ssd1306_DrawBitmap(playerX, playerY, player_bitmap, PLAYER_WIDTH, PLAYER_HEIGHT, White);
    for (int row = 0; row < ALIEN_ROWS; row++) {
        for (int i = 0; i < NUM_ALIENS_PER_ROW; i++) {
            if (aliens[row][i].alive) {
            	if(animationFlag) {
                    ssd1306_DrawBitmap(aliens[row][i].x, aliens[row][i].y,
                        ufo_bitmap, ALIEN_WIDTH, ALIEN_HEIGHT, White);
            	} else {
                    ssd1306_DrawBitmap(aliens[row][i].x, aliens[row][i].y,
                        ufo_alternative, ALIEN_WIDTH, ALIEN_HEIGHT, White);
            	}
            }
        }
    }
    animationFlag = !animationFlag;

    drawShields();
    if (bulletActive) {
        ssd1306_FillRectangle(bullet.x, bullet.y, bullet.x + BULLET_WIDTH, bullet.y + BULLET_HEIGHT, White);
    }
    if (alienBulletActive) {
        ssd1306_FillRectangle(alienBullet.x, alienBullet.y, alienBullet.x + BULLET_WIDTH - 1, alienBullet.y + BULLET_HEIGHT, White);
    }

    drawHealth();

    ssd1306_UpdateScreen();
}

void shootBullet(void) {
    if (!bulletActive) {
        bullet.x = playerX + PLAYER_WIDTH / 2 - BULLET_WIDTH / 2;
        bullet.y = playerY - BULLET_HEIGHT;
        bulletActive = true;
    }
}
void handlePlayerDeath(void) {
    playerLives--;
    playerX = 60;
    playerY = 56;
    HAL_Delay(500);
    if (playerLives <= 0) {
        gameOver = true;
    }
}

void alienShoot(void) {
    if (alienBulletActive) return;

    int alienToFireRow = -1;
    int alienToFireCol = -1;

    int randomColumn = HAL_GetTick() % NUM_ALIENS_PER_ROW;
    for (int row = ALIEN_ROWS - 1; row >= 0; row--) {
        if (aliens[row][randomColumn].alive) {
            alienToFireRow = row;
            alienToFireCol = randomColumn;
            break;
        }
    }
    if (alienToFireRow == -1) {
        return;
    }

    alienBullet.x = aliens[alienToFireRow][alienToFireCol].x + ALIEN_WIDTH / 2 - BULLET_WIDTH / 2;
    alienBullet.y = aliens[alienToFireRow][alienToFireCol].y + ALIEN_HEIGHT;
    alienBulletActive = true;
}

void handleInput(void) {
    uint8_t user_input;
    if (HAL_UART_Receive( & huart2, & user_input, 1, 100) == HAL_OK) {
        switch (user_input) {
        case 'a':
            if (playerX > 0) playerX -= 2;
            break;
        case 'd':
            if (playerX + PLAYER_WIDTH < 128) playerX += 2;
            break;
        case ' ':
            shootBullet();
            break;
        default:
            break;
        }
    }
}

void checkAlienBulletCollision(void) {
    if (alienBulletActive &&
        alienBullet.x < playerX + PLAYER_WIDTH &&
        alienBullet.x + BULLET_WIDTH > playerX &&
        alienBullet.y < playerY + PLAYER_HEIGHT &&
        alienBullet.y + BULLET_HEIGHT > playerY) {
        alienBulletActive = false;
        handlePlayerDeath();
        return;
    }
    if (alienBullet.y >= 64) {
        alienBulletActive = false;
        return;
    }
    checkShieldCollisions(alienBullet.x, alienBullet.y, true);
}

void checkIfPlayerWon(void) {
    int aliveAliens = 0;
    for (int row = 0; row < ALIEN_ROWS; row++) {
        for (int i = 0; i < NUM_ALIENS_PER_ROW; i++) {
            if (aliens[row][i].alive) {
                aliveAliens++;
            }
        }
    }

    if (aliveAliens == 0) {
        playerWon = true;
    }
}

void drawEndGame(char* endTitle) {
	ssd1306_Fill(Black);

	char lives_str[12];
	char score_str[12];
	char final_str[50];

	sprintf(lives_str, "Health: %d", playerLives);
	sprintf(score_str, "Score: %d", score);

	ssd1306_SetCursor(10, 20);
	ssd1306_WriteString(endTitle, Font_7x10, White);

	ssd1306_SetCursor(10, 35);
	ssd1306_WriteString(score_str, Font_6x8, White);

	ssd1306_SetCursor(10, 45);
	ssd1306_WriteString(lives_str, Font_6x8, White);

	ssd1306_UpdateScreen();
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_UART_Init();
    MX_I2C1_Init();
    MX_SPI1_Init();

    ssd1306_Init();
    initializeAliens();
    initShields();
    alienFireTimer = 0;

    while (1) {
        if (!gameOver && !playerWon) {
            handleInput();
            checkIfPlayerWon();
            if (bulletActive) {
                bullet.y -= 2;
            }
            if (alienBulletActive) {
                alienBullet.y += 3;
            }
            moveAliens();
            checkCollisions();
            checkAlienBulletCollision();

            alienFireTimer++;
            if (alienFireTimer >= ALIEN_FIRE_SPEED) {
                alienShoot();
                alienFireTimer = 0;
            }

            drawGame();
            HAL_Delay(50);
        } else if (gameOver) {
        	drawEndGame("Game Over!");
        } else if (playerWon) {
        	drawEndGame("You won!");
        }
    }
}

void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {
        0
    };
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {
        0
    };

    if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK) {
        Error_Handler();
    }

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 1;
    RCC_OscInitStruct.PLL.PLLN = 10;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
    RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
    RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
    if (HAL_RCC_OscConfig( & RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
        RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig( & RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) {
        Error_Handler();
    }
}

static void MX_I2C1_Init(void) {

    GPIO_InitTypeDef GPIO_InitStruct;
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {
        0
    };

    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_I2C1;
    PeriphClkInit.Usart2ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;
    HAL_RCCEx_PeriphCLKConfig( & PeriphClkInit);

    __HAL_RCC_I2C1_CLK_ENABLE();

    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, & GPIO_InitStruct);

    hi2c1.Instance = I2C1;
    hi2c1.Init.Timing = 0x10909CEC;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init( & hi2c1) != HAL_OK) {
        Error_Handler();
    }

    if (HAL_I2CEx_ConfigAnalogFilter( & hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK) {
        Error_Handler();
    }

    if (HAL_I2CEx_ConfigDigitalFilter( & hi2c1, 0) != HAL_OK) {
        Error_Handler();
    }

}

static void MX_SPI1_Init(void) {

    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial = 7;
    hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
    hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
    if (HAL_SPI_Init( & hspi1) != HAL_OK) {
        Error_Handler();
    }

}

static void MX_USART2_UART_Init(void) {

    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init( & huart2) != HAL_OK) {
        Error_Handler();
    }

}

static void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {
        0
    };

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_RESET);

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);

    HAL_GPIO_WritePin(SSD1306_CS_Port, SSD1306_CS_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SSD1306_DC_Port, SSD1306_DC_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SSD1306_Reset_Port, SSD1306_Reset_Pin, GPIO_PIN_RESET);

    GPIO_InitStruct.Pin = B1_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(B1_GPIO_Port, & GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOA, & GPIO_InitStruct);

    GPIO_InitStruct.Pin = SSD1306_CS_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(SSD1306_CS_Port, & GPIO_InitStruct);

    GPIO_InitStruct.Pin = SSD1306_DC_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(SSD1306_DC_Port, & GPIO_InitStruct);

    GPIO_InitStruct.Pin = SX1509_OSC_Pin | SX1509_nRST_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(SX1509_nINIT_PORT, & GPIO_InitStruct);

    GPIO_InitStruct.Pin = SX1509_nINIT_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(SX1509_nINIT_PORT, & GPIO_InitStruct);

    GPIO_InitStruct.Pin = USART_TX_Pin | USART_RX_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, & GPIO_InitStruct);

    GPIO_InitStruct.Pin = SSD1306_Reset_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(SSD1306_Reset_Port, & GPIO_InitStruct);

    HAL_GPIO_WritePin(SX1509_nRST_PORT, SX1509_nRST_Pin, GPIO_PIN_SET);

    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

}

void Error_Handler(void) {

    __disable_irq();
    while (1) {}

}

#ifdef USE_FULL_ASSERT

void assert_failed(uint8_t * file, uint32_t line) {

}
#endif
