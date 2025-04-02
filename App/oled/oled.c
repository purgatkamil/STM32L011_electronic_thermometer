#include "oled.h"
#include "font.h"

#define SSD1306_I2C_ADDR (0x3C << 1)
#define SSD1306_CMD      0x00
#define SSD1306_DATA     0x40

#define OLED_WIDTH       128
#define OLED_HEIGHT      64
#define OLED_PAGES       (OLED_HEIGHT / 8)

extern I2C_HandleTypeDef hi2c1;

static uint8_t buffer[OLED_WIDTH * OLED_PAGES];

static void oled_send_cmd(uint8_t cmd) {
    uint8_t data[2] = { SSD1306_CMD, cmd };
    HAL_I2C_Master_Transmit(&hi2c1, SSD1306_I2C_ADDR, data, 2, HAL_MAX_DELAY);
}

static void oled_send_data(uint8_t *data, uint16_t size) {
    HAL_I2C_Mem_Write(&hi2c1, SSD1306_I2C_ADDR, SSD1306_DATA, I2C_MEMADD_SIZE_8BIT, data, size, HAL_MAX_DELAY);
}

void oled_init(void) {
    HAL_Delay(100);
    oled_send_cmd(0xAE); // Display off
    oled_send_cmd(0x20); oled_send_cmd(0x00); // Horizontal addressing mode
    oled_send_cmd(0xB0); // Page start
    oled_send_cmd(0xC8);
    oled_send_cmd(0x00);
    oled_send_cmd(0x10);
    oled_send_cmd(0x40);
    oled_send_cmd(0x81); oled_send_cmd(0x7F);
    oled_send_cmd(0xA1);
    oled_send_cmd(0xA6);
    oled_send_cmd(0xA8); oled_send_cmd(0x3F);
    oled_send_cmd(0xA4);
    oled_send_cmd(0xD3); oled_send_cmd(0x00);
    oled_send_cmd(0xD5); oled_send_cmd(0x80);
    oled_send_cmd(0xD9); oled_send_cmd(0xF1);
    oled_send_cmd(0xDA); oled_send_cmd(0x12);
    oled_send_cmd(0xDB); oled_send_cmd(0x40);
    oled_send_cmd(0x8D); oled_send_cmd(0x14);
    oled_send_cmd(0xAF); // Display on

    oled_clear();
    oled_display();
}

void oled_clear(void) {
    for (uint16_t i = 0; i < sizeof(buffer); i++)
        buffer[i] = 0x00;
}

void oled_display(void) {
    for (uint8_t page = 0; page < OLED_PAGES; page++) {
        oled_send_cmd(0xB0 + page);
        oled_send_cmd(0x00);
        oled_send_cmd(0x10);
        oled_send_data(&buffer[OLED_WIDTH * page], OLED_WIDTH);
    }
}

void oled_putc(uint8_t x, uint8_t y, char c) {
    if (x >= OLED_WIDTH || y >= OLED_PAGES) return;

    uint8_t index;
    switch (c) {
        case ' ': index = 0; break;
        case '.': index = 1; break;
        case ',': index = 2; break;
        case '0': index = 3; break;
        case '1': index = 4; break;
        case '2': index = 5; break;
        case '3': index = 6; break;
        case '4': index = 7; break;
        case '5': index = 8; break;
        case '6': index = 9; break;
        case '7': index = 10; break;
        case '8': index = 11; break;
        case '9': index = 12; break;
        case '%': index = 13; break;
        case 'h': index = 14; break;
        case 'P': index = 15; break;
        case 'a': index = 16; break;
        case 'R': index = 17; break;
        case 'C': index = 18; break;
        case 'H': index = 19; break;
        case '\x60':
        case 0xB0:
            index = 20; break;
        default:
            return;
    }

    uint16_t buf_index = y * OLED_WIDTH + x;
    for (uint8_t i = 0; i < 5; i++) {
        if (buf_index + i < sizeof(buffer))
            buffer[buf_index + i] = font5x8[index][i];
    }
    if (buf_index + 5 < sizeof(buffer))
        buffer[buf_index + 5] = 0x00;
}

void oled_print(uint8_t x, uint8_t y, const char *str) {
    while (*str && x < (OLED_WIDTH - 6)) {
        oled_putc(x, y, *str++);
        x += 6;
    }
}
