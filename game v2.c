#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#define SPI_PORT spi0
#define PIN_SCK 18
#define PIN_MOSI 19
#define PIN_MISO 16
#define PIN_CS 17
#define PIN_DC 20
#define PIN_RST 21
#define BUFFER_SIZE 1024

void tft_send_command(uint8_t cmd);
void tft_send_data(uint8_t data);

void tft_init();
void tft_reset();
void tft_set_address_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);


void tft_draw_pixel(uint16_t x, uint16_t y, uint16_t color);
void tft_send_data_buffer(uint8_t *data, size_t length);
void tft_fill_screen(uint16_t color);
void tft_fill_rect(uint16_t x0, uint16_t y0, uint16_t width, uint16_t height, uint16_t color);

int main() {
  stdio_init_all();
  spi_init(SPI_PORT, 1000*1000);
  gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
  gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
  gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);

  gpio_init(PIN_CS);
  gpio_set_dir(PIN_CS, GPIO_OUT);
  gpio_put(PIN_CS, 1);

  gpio_init(PIN_DC);
  gpio_set_dir(PIN_DC, GPIO_OUT);

  gpio_init(PIN_RST);
  gpio_set_dir(PIN_RST, GPIO_OUT);
  gpio_put(PIN_RST, 1);
  sleep_ms(20);

  tft_reset();
  tft_init();
  tft_fill_screen(0x0000);

  tft_fill_rect(50, 100, 80, 30, 0xF800);
  tft_fill_rect(150, 150, 40, 40, 0x07E0);


  while (true) {
    sleep_ms(250);
  }
}

void tft_send_command(uint8_t cmd){
  gpio_put(PIN_DC, 0);
  spi_write_blocking(SPI_PORT, &cmd, 1);
}

void tft_send_data(uint8_t data){
  gpio_put(PIN_DC, 1);
  spi_write_blocking(SPI_PORT, &data, 1);
}

void tft_reset(){
  gpio_put(PIN_RST, 1);
  sleep_ms(5);

  gpio_put(PIN_RST, 0);
  sleep_ms(20);

  gpio_put(PIN_RST, 1);
  sleep_ms(120);
}

void tft_init(void){
  gpio_put(PIN_CS, 0);
  tft_send_command(0x01);
  sleep_ms(120);
  tft_send_command(0x11);
  sleep_ms(120);
  tft_send_command(0x3A);
  tft_send_data(0x55);
  tft_send_command(0x36);
  tft_send_data(0x48);
  tft_send_command(0x29);
  sleep_ms(20);
  gpio_put(PIN_CS, 1);
}

void tft_set_address_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1){
  uint8_t x0_high = x0>>8;
  uint8_t x0_low  = x0 & 0xff;

  uint8_t x1_high = x1>>8;
  uint8_t x1_low  = x1 & 0xff;

  uint8_t y0_high = y0>>8;
  uint8_t y0_low  = y0 & 0xff;

  uint8_t y1_high = y1>>8;
  uint8_t y1_low  = y1 & 0xff;

  tft_send_command(0x2A);
  tft_send_data(x0_high);
  tft_send_data(x0_low);
  tft_send_data(x1_high);
  tft_send_data(x1_low);

  tft_send_command(0x2B);
  tft_send_data(y0_high);
  tft_send_data(y0_low);
  tft_send_data(y1_high);
  tft_send_data(y1_low);

  tft_send_command(0x2C);
}

void tft_draw_pixel(uint16_t x, uint16_t y, uint16_t color)
{
  gpio_put(PIN_CS, 0);
  uint8_t color_high = color>>8;
  uint8_t color_low = color & 0xff;
  tft_set_address_window(x, y, x, y);
  tft_send_data(color_high);
  tft_send_data(color_low);
  gpio_put(PIN_CS, 1);
}


void tft_send_data_buffer(uint8_t *data, size_t length){
  gpio_put(PIN_DC, 1);
  spi_write_blocking(SPI_PORT, data, length);
}

void tft_fill_screen(uint16_t color)
{
  gpio_put(PIN_CS, 0);
  uint8_t color_high = color>>8;
  uint8_t color_low = color & 0xff;
  tft_set_address_window(0, 0, 239, 319);
  uint8_t buffer[BUFFER_SIZE];
  for(int i=0; i<BUFFER_SIZE; i+=2){
    buffer[i] = color_high;
    buffer[i+1] = color_low;
  }
  for(int i=0; i<75*2; i++){
    tft_send_data_buffer(buffer, 1024);
  }
  gpio_put(PIN_CS, 1);
}

void tft_fill_rect(uint16_t x0, uint16_t y0, uint16_t width, uint16_t height, uint16_t color){
  gpio_put(PIN_CS, 0);
  uint8_t color_high = color>>8;
  uint8_t color_low  = color & 0xff;
  tft_set_address_window(x0, y0, x0+width-1, y0+height-1);
  
  
  uint8_t buffer[BUFFER_SIZE];


  for(int i=0; i<BUFFER_SIZE; i+=2){
    buffer[i] = color_high;
    buffer[i+1] = color_low;
  }
  uint16_t total_bytes = width*height*2;
  while(total_bytes >= BUFFER_SIZE){
    tft_send_data_buffer( buffer, 1024);
    total_bytes -= BUFFER_SIZE;
  }
  if(total_bytes > 0){
    tft_send_data_buffer(buffer, total_bytes);
  }
  gpio_put(PIN_CS, 1);
}
