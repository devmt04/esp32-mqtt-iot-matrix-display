#include "MAX7219.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "soc/soc_caps.h"
#include <string.h>

#define NUM_MODULES 12  

#define CS_LOW()  gpio_set_level(CS_PIN, 0)
#define CS_HIGH() gpio_set_level(CS_PIN, 1)


const weather_time_font7x3_t weather_time_font7x3[] = {
     [0] = {{
        0b00000000,
        0b11100000,
        0b10100000,
        0b10100000,
        0b10100000,
        0b10100000,
        0b10100000,
        0b11100000
    }},

    [1]= {{
        0b00000000,
        0b01000000,
        0b11000000,
        0b01000000,
        0b01000000,
        0b01000000,
        0b01000000,
        0b11100000
    }},

    [2] = {{
        0b00000000,
        0b11100000,
        0b10100000,
        0b00100000,
        0b11100000,
        0b10000000,
        0b10100000,
        0b11100000
    }},

    [3] = {{
        0b00000000,
        0b11100000,
        0b10100000,
        0b00100000,
        0b01100000,
        0b00100000,
        0b10100000,
        0b11100000
    }},

    [4] = {{
        0b00000000,
        0b10000000,
        0b10100000,
        0b10100000,
        0b10100000,
        0b11100000,
        0b00100000,
        0b00100000
    }},

    [5] = {{
        0b00000000,
        0b11100000,
        0b10000000,
        0b11100000,
        0b10100000,
        0b00100000,
        0b10100000,
        0b11100000
    }},

    [6] = {{
        0b00000000,
        0b11100000,
        0b10100000,
        0b10000000,
        0b11100000,
        0b10100000,
        0b10100000,
        0b11100000
    }},

    [7] = {{
        0b00000000,
        0b11100000,
        0b10100000, // 1 at front here?
        0b00100000,
        0b00100000,
        0b00100000,
        0b00100000,
        0b00100000
    }},

    [8] = {{
        0b00000000,
        0b11100000,
        0b10100000,
        0b10100000,
        0b11100000,
        0b10100000,
        0b10100000,
        0b11100000
    }},

    [9] = {{
        0b00000000,
        0b11100000,
        0b10100000,
        0b10100000,
        0b11100000,
        0b00100000,
        0b10100000,
        0b11100000
    }},

    [10] = {{  // Degree Symbol
        0b11100000,
        0b10100000,
        0b11100000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000
    }},

    [11] = {{ // Celsius Symbol
        0b00000000,
        0b11100000,
        0b10100000,
        0b10000000,
        0b10000000,
        0b10000000,
        0b10100000,
        0b11100000
    }},
    
    [12] = {{
        0b11100000,
        0b10101110,
        0b11101010,
        0b00001000,
        0b00001000,
        0b00001000,
        0b00001010,
        0b00001110
    }}

    // Colon symbol
};

const string_font6x5_t string_font6x5[] = {
    // [0] = 'A'
    [0] = {{
        0b01111100,
        0b00010010,
        0b00010010,
        0b00010010,
        0b01111100,
        0b00000000,
        0b00000000,
        0b00000000
    }},
    // [1] = 'B'
    [1] = {{
        0b01111110,
        0b01001010,
        0b01001010,
        0b01011010,
        0b00110100,
        0b00000000,
        0b00000000,
        0b00000000
    }},
    // [2] = 'C'
    [2] = {{
        0b00111100,
        0b01000010,
        0b01000010,
        0b01000010,
        0b00100100,
        0b00000000,
        0b00000000,
        0b00000000
    }},
    // [3] = 'D'
    [3] = {{
        0b01111110,
        0b01000010,
        0b01000010,
        0b01000010,
        0b00111100,
        0b00000000,
        0b00000000,
        0b00000000
    }},
    // [4] = 'E'
    [4] = {{
        0b01111110,
        0b01001010,
        0b01001010,
        0b01001010,
        0b01000010,
        0b00000000,
        0b00000000,
        0b00000000
    }},
    // [5] = 'F'
    [5] = {{
        0b01111110,
        0b00001010,
        0b00001010,
        0b00001010,
        0b00000010,
        0b00000000,
        0b00000000,
        0b00000000
    }},
    // [6] = 'G'
    [6] = {{
        0b00111100,
        0b01000010,
        0b01000010,
        0b01001000,
        0b01111000,
        0b00000000,
        0b00000000,
        0b00000000
    }},
    // [7] = 'H'
    [7] = {{
        0b01111110,
        0b00001000,
        0b00001000,
        0b00001000,
        0b01111110,
        0b00000000,
        0b00000000,
        0b00000000
    }},
    // [8] = 'I'
    [8] = {{
        0b01000010,
        0b01000010,
        0b01111110,
        0b01000010,
        0b01000010,
        0b00000000,
        0b00000000,
        0b00000000
    }},
    // [9] = 'J'
    [9] = {{
        0b00110000,
        0b01000000,
        0b01000010,
        0b01000010,
        0b00111110,
        0b00000000,
        0b00000000,
        0b00000000
    }},
    // [10] = 'K'
    [10] = {{
        0b01111110,
        0b00001000,
        0b00010100,
        0b00100010,
        0b01100010,
        0b00000000,
        0b00000000,
        0b00000000
    }},
    // [11] = 'L'
    [11] = {{
        0b01111110,
        0b01000000,
        0b01000000,
        0b01000000,
        0b01000000,
        0b00000000,
        0b00000000,
        0b00000000
    }},
    // [12] = 'M'
    [12] = {{
        0b01111110,
        0b00000100,
        0b00001000,
        0b00000100,
        0b01111110,
        0b00000000,
        0b00000000,
        0b00000000
    }},
    // [13] = 'N'
    [13] = {{
        0b01111110,
        0b00000100,
        0b00001000,
        0b00010000,
        0b01111110,
        0b00000000,
        0b00000000,
        0b00000000
    }},
    // [14] = 'O'
    [14] = {{
        0b00111100,
        0b01000010,
        0b01000010,
        0b01000010,
        0b00111100,
        0b00000000,
        0b00000000,
        0b00000000
    }},
    // [15] = 'P'
    [15] = {{
        0b01111110,
        0b00010010,
        0b00010010,
        0b00010010,
        0b00001100,
        0b00000000,
        0b00000000,
        0b00000000
    }},
    // [16] = 'Q'
    [16] = {{
        0b00111100,
        0b01000010,
        0b01000010,
        0b01100010,
        0b10111100,
        0b00000000,
        0b00000000,
        0b00000000
    }},
    // [17] = 'R'
    [17] = {{
        0b01111110,
        0b00010010,
        0b00010010,
        0b00110010,
        0b01001100,
        0b00000000,
        0b00000000,
        0b00000000
    }},
    // [18] = 'S'
    [18] = {{
        0b00100100,
        0b01001010,
        0b01001010,
        0b01001010,
        0b00110000,
        0b00000000,
        0b00000000,
        0b00000000
    }},
    // [19] = 'T'
    [19] = {{
        0b00000010,
        0b00000010,
        0b01111110,
        0b00000010,
        0b00000010,
        0b00000000,
        0b00000000,
        0b00000000
    }},
    // [20] = 'U'
    [20] = {{
        0b00111110,
        0b01000000,
        0b01000000,
        0b01000000,
        0b00111110,
        0b00000000,
        0b00000000,
        0b00000000
    }},
    // [21] = 'V'
    [21] = {{
        0b00001110,
        0b00110000,
        0b01000000,
        0b00110000,
        0b00001110,
        0b00000000,
        0b00000000,
        0b00000000
    }},
    // [22] = 'W'
    [22] = {{
        0b00111110,
        0b01000000,
        0b00111000,
        0b01000000,
        0b00111110,
        0b00000000,
        0b00000000,
        0b00000000
    }},
    // [23] = 'X'
    [23] = {{
        0b01100110,
        0b00011000,
        0b00011000,
        0b00011000,
        0b01100110,
        0b00000000,
        0b00000000,
        0b00000000
    }},
    // [24] = 'Y'
    [24] = {{
        0b00000110,
        0b00001000,
        0b01110000,
        0b00001000,
        0b00000110,
        0b00000000,
        0b00000000,
        0b00000000
    }},
    // [25] = 'Z'
    [25] = {{
        0b01100010,
        0b01010010,
        0b01001010,
        0b01000110,
        0b01000110,
        0b00000000,
        0b00000000,
        0b00000000
    }},
    // [26] = ' ' (space)
    [26] = {{
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000
    }},
    // [27] = '!'
    [27] = {{
        0b00000000,
        0b01011110,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000
    }},
    // [28] = '.'
    [28] = {{
        0b00000000,
        0b01100000,
        0b01100000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000
    }}
};

spi_device_handle_t spi;

// init CS pin
static esp_err_t init_cs(){
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << CS_PIN),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    ESP_ERROR_CHECK(CS_HIGH());   // Deselect MAX7219
    return ESP_OK;
}

// Send one register+data pair to ALL cascaded modules
void max7219_send_all(uint8_t reg, uint8_t data)
{
    uint8_t buf[NUM_MODULES * 2];

    for (int i = 0; i < NUM_MODULES; i++) {
        buf[i*2 + 0] = reg;
        buf[i*2 + 1] = data;
    }

    spi_transaction_t t = {
        .length = NUM_MODULES * 16,
        .tx_buffer = buf
    };

    CS_LOW();
    spi_device_polling_transmit(spi, &t);
    CS_HIGH();
    esp_rom_delay_us(2); // Small delay to ensure data is latched
}

void max7219_send(int module, uint8_t reg, uint8_t data)
{
    uint8_t buf[NUM_MODULES * 2];

    for (int i = 0; i < NUM_MODULES; i++) {
        if (i == module) {
            buf[i*2 + 0] = reg;
            buf[i*2 + 1] = data;
        } else {
            buf[i*2 + 0] = 0x00;   // NO-OP register
            buf[i*2 + 1] = 0x00;
        }
    }

    spi_transaction_t t = {
        .length = NUM_MODULES * 16,
        .tx_buffer = buf,
        // .tx_length = 0
    };

    CS_LOW();
    spi_device_polling_transmit(spi, &t);
    CS_HIGH();
    esp_rom_delay_us(2); // Small delay to ensure data is latched
}

static void max7219_basic_init()
{
    max7219_send_all(0x0C, 0x01);  // Normal operation
    max7219_send_all(0x09, 0x00);  // Decode OFF
    max7219_send_all(0x0A, 0x0F);  // Brightness MAX
    max7219_send_all(0x0B, 0x07);  // Scan limit = 8 rows
    max7219_send_all(0x0F, 0x00);  // Test mode OFF
}

void max7219_set_brightness(uint8_t module, uint8_t intensity) {
    // intensity: 0x00 (min) to 0x0F (max)
    if (intensity > 0x0F) intensity = 0x0F;  // Clamp to max
    max7219_send(module, 0x0A, intensity);
}

void set_all_brightness(uint8_t intensity) {
    for (int module = 0; module < NUM_MODULES; module++){
        max7219_set_brightness(module, intensity);
    }
}

void drawClear(int module){
    for (int row = 1; row <=8; row++){
        max7219_send(module, row, 0x00);
    }
}

void drawClear_range(int from, int to){
    for(int i = from; i< to+1; i++){
        drawClear(i);
    }
}

void drawClear_all(){
    for (int module = 0; module < NUM_MODULES; module++){
        drawClear(module);
    }
}


void draw_buffer(uint8_t buf[32]) {
    drawClear_range(8, 11);
    for (int module = 4; module < 8; module++) {
        int start = (module - 4) * 8;
        for (int row = 0; row < 8; row++) {
            max7219_send(module+4, row+1, buf[start+row]);
        }
    }
}

void draw_time(int hr, int min, int sec){
    drawClear_range(4, 7);
    int hr0 = hr / 10;
    int hr1 = hr % 10;
    int min0 = min / 10;
    int min1 = min % 10;
    int sec0 = sec / 10;
    int sec1 = sec % 10;
    uint8_t min_pattern[8] = {
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000
    };
    uint8_t hr_pattern[8] = {
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000
    };
    uint8_t sec_pattern[8] = {
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000
    };

    const uint8_t *min1_rows = weather_time_font7x3[min1].rows;
    const uint8_t *min0_rows = weather_time_font7x3[min0].rows;
    const uint8_t *hr1_rows  = weather_time_font7x3[hr1].rows;
    const uint8_t *hr0_rows  = weather_time_font7x3[hr0].rows;
    const uint8_t *sec1_rows  = weather_time_font7x3[sec1].rows;
    const uint8_t *sec0_rows  = weather_time_font7x3[sec0].rows;

    // Build second pattern
    for (int row = 0; row < 8; row++) {
        uint8_t d1 = sec1_rows[row] & 0b11100000;
        uint8_t d0 = sec0_rows[row] & 0b11100000;
        sec_pattern[row] = (d1 >> 5) | (d0 >> 1);
    }

    // Build minute pattern
    for (int row = 0; row < 8; row++) {
        uint8_t d1 = min1_rows[row] & 0b11100000;
        uint8_t d0 = min0_rows[row] & 0b11100000;
        min_pattern[row] = (d1 >> 5) | (d0 >> 1);
    }

    // Build hour pattern
    for (int row = 0; row < 8; row++) {

        uint8_t d0 = (hr0_rows[row] >> 5) & 0b111;   // hr0 (tens)
        uint8_t d1 = (hr1_rows[row] >> 5) & 0b111;   // hr1 (ones)

        hr_pattern[row] =
            (d0 << 5)     // puts hr0 into bits 7..5
            | (d1 << 1);    // puts hr1 into bits 3..1
                        // bit 4 and bit 0 are empty "spaces"
    }

    // draw in 8x8
    for (int row = 0; row < 8; row++) {
        max7219_send(4, row + 1, hr_pattern[row]);
        max7219_send(5, row + 1, min_pattern[row]);
        max7219_send(6, row + 1, sec_pattern[row]);
    }
}

void draw_weather(weather_data_t weather_data){
    drawClear_range(0, 3);
    uint8_t temp_pattern[8] = {
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000
    };

    uint8_t wind_speed_pattern[8] = {
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000
    };

    const uint8_t *temp1_rows  = weather_time_font7x3[(weather_data.temp)%10].rows;
    const uint8_t *temp0_rows  = weather_time_font7x3[(weather_data.temp)/10].rows;

    const uint8_t *wind_speed1_rows  = weather_time_font7x3[(weather_data.wind_speed)%10].rows;
    const uint8_t *wind_speed0_rows  = weather_time_font7x3[(weather_data.wind_speed)/10].rows;
 
    for (int row = 0; row < 8; row++) {
        uint8_t d0 = (temp0_rows[row] >> 5) & 0b111;   
        uint8_t d1 = (temp1_rows[row] >> 5) & 0b111;   
        temp_pattern[row] = (d0 << 5)  | (d1 << 1);                 
    }

    for (int row = 0; row < 8; row++) {
        uint8_t d0 = (wind_speed0_rows[row] >> 5) & 0b111;   
        uint8_t d1 = (wind_speed1_rows[row] >> 5) & 0b111;   
        wind_speed_pattern[row] = (d0 << 5)  | (d1 << 1);                 
    }

     // draw in 8x8
    for (int row = 0; row < 8; row++) {
        max7219_send(3, row + 1, wind_speed_pattern[row]);
        max7219_send(1, row + 1, weather_time_font7x3[12].rows[row]);
        max7219_send(0, row + 1, temp_pattern[row]);
    }
}


void draw_init(void){
    for (int row = 0; row < 8; row++) {
        max7219_send(0, row + 1, font8x8['L' - 'A'].rows[row]);
        max7219_send(1, row + 1, font8x8['P' - 'A'].rows[row]);
        max7219_send(2, row + 1, font8x8['U' - 'A'].rows[row]);
        drawClear(3);
        
        max7219_send(4, row + 1, font8x8['S' - 'A'].rows[row]);
        max7219_send(5, row + 1, font8x8['Y' - 'A'].rows[row]);
        max7219_send(6, row + 1, font8x8['S' - 'A'].rows[row]);
        drawClear(7);

        max7219_send(8, row + 1, font8x8['I' - 'A'].rows[row]);
        max7219_send(9, row + 1, font8x8['N' - 'A'].rows[row]);
        max7219_send(10, row + 1, font8x8['I' - 'A'].rows[row]);
        max7219_send(11, row + 1, font8x8['I' - 'A'].rows[row]);
    }
}

esp_err_t init_spi(){
    init_cs();
    const spi_bus_config_t bus_config = {
        .miso_io_num = -1,
        .mosi_io_num = GPIO_NUM_11,
        .sclk_io_num = GPIO_NUM_12,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .data4_io_num = -1,
        .data5_io_num = -1,
        .data6_io_num = -1,
        .data7_io_num = -1,
        .max_transfer_sz = SOC_SPI_MAXIMUM_BUFFER_SIZE,
        .data_io_default_level = 0,
        .flags = 0,
        .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,
        .intr_flags = 0,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus_config, SPI_DMA_DISABLED));

    spi_device_interface_config_t dev_config = {
        .clock_speed_hz = 10 * 1000 * 1000,   // MAX7219 supports up to 10 MHz
        .mode = 0,                            // CPOL=0, CPHA=0 (SPI mode 0)
        .spics_io_num = -1,                   // MANUAL CS (mandatory for cascaded modules)
        .queue_size = 1,                      // Only 1 transaction needed
        .flags = SPI_DEVICE_HALFDUPLEX,       // MAX7219 is write-only
        .command_bits = 0,                    // MAX7219 uses simple 16-bit frames
        .address_bits = 0,                    // No address phase
        .dummy_bits = 0,                      // No dummy cycles
    };

    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &dev_config, &spi));
    max7219_basic_init();
    
    return ESP_OK;
}