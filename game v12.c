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
#define LEFT_BUTTON  14
#define RIGHT_BUTTON 15
#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240
#define GROUND_Y 200
#define JUMP_BUTTON 13
#define PLATFORM_COUNT 3
#define ENEMY_COUNT 2


void tft_send_command(uint8_t cmd);
void tft_send_data(uint8_t data);

void tft_init();
void tft_reset();
void tft_set_address_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);


void tft_draw_pixel(uint16_t x, uint16_t y, uint16_t color);
void tft_send_data_buffer(uint8_t *data, size_t length);
void tft_fill_screen(uint16_t color);
void tft_fill_rect(uint16_t x0, uint16_t y0, uint16_t width, uint16_t height, uint16_t color);

typedef struct
{
  int16_t x, y;
  uint16_t width, height;
  int16_t vx, vy;
  bool grounded;
} Player;

typedef struct
{
  int16_t x, y;
  uint16_t width, height;
  int16_t vx;
  bool alive;
} Enemy;

typedef struct
{
  int16_t x;
  int16_t y;
  uint16_t width;
  uint16_t height;
} Platform;

void player_draw(Player *player);
void draw_lives(int lives);
void draw_score(int score);

int main() {
  stdio_init_all();
  spi_init(SPI_PORT, 1000 * 1000);
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

  gpio_init(LEFT_BUTTON);
  gpio_set_dir(LEFT_BUTTON, GPIO_IN);
  gpio_pull_up(LEFT_BUTTON);

  gpio_init(RIGHT_BUTTON);
  gpio_set_dir(RIGHT_BUTTON, GPIO_IN);
  gpio_pull_up(RIGHT_BUTTON);

  gpio_init(JUMP_BUTTON);
  gpio_set_dir(JUMP_BUTTON, GPIO_IN);
  gpio_pull_up(JUMP_BUTTON);

  tft_reset();
  tft_init();
  tft_fill_rect(0, GROUND_Y, SCREEN_WIDTH, SCREEN_HEIGHT - GROUND_Y, 0x07E0);

  //player structure
  Player player = {
    .x = 0,
    .y = 184,
    .width = 16,
    .height = 16,
    .vx = 0,
    .vy = 0,
    .grounded = true
  };

  //enemy structure
  Enemy enemies[ENEMY_COUNT] = {
    {
      .x = 250,
      .y = 184,
      .width = 16,
      .height = 16,
      .vx = -1,
      .alive = true
    },
    {
      .x = 150,
      .y = 184,
      .width = 16,
      .height = 16,
      .vx = 1,
      .alive = true
    }
  };

  Platform platforms[PLATFORM_COUNT] = {
    {100, 150, 100, 10},
    {220, 120, 70, 10},
    {40, 90, 80, 10}
  };

  for (int i = 0; i < PLATFORM_COUNT; i++)
  {
    tft_fill_rect(
      platforms[i].x,
      platforms[i].y,
      platforms[i].width,
      platforms[i].height,
      0x07E0
    );
  }
  tft_fill_rect(player.x, player.y, player.width, player.height, 0xFFFF);
  for (int i = 0; i < ENEMY_COUNT; i++)
  {
    tft_fill_rect(
      enemies[i].x,
      enemies[i].y,
      enemies[i].width,
      enemies[i].height,
      0xF800
    );
  }


  //no of lives
  int lives = 3;
  draw_lives(lives);

  //score
  int score = 0;
  draw_score(score);

  // hit cooldown
  bool hit_cooldown = false;

  while (true) {
    int16_t old_x = player.x;
    int16_t old_y = player.y;

    int16_t old_enemy_x[ENEMY_COUNT];
    int16_t old_enemy_y[ENEMY_COUNT];

    for (int i = 0; i < ENEMY_COUNT; i++)
    {
      old_enemy_x[i] = enemies[i].x;
      old_enemy_y[i] = enemies[i].y;
    }

    if (!gpio_get(LEFT_BUTTON))
    {
      player.vx = -2;

    }
    else if (!gpio_get(RIGHT_BUTTON))
    {
      player.vx = 2;
    }
    else
    {
      player.vx = 0;
    }

    player.x += player.vx;


    for (int i = 0; i < ENEMY_COUNT; i++)
    {
      if (enemies[i].alive)
      {
        enemies[i].x += enemies[i].vx;

        if (enemies[i].x <= 0)
        {
          enemies[i].x = 0;
          enemies[i].vx = -enemies[i].vx;
        }
        else if (enemies[i].x >= SCREEN_WIDTH - enemies[i].width)
        {
          enemies[i].x = SCREEN_WIDTH - enemies[i].width;
          enemies[i].vx = -enemies[i].vx;
        }
      }
    }





    // Erase old enemy positions
    for (int i = 0; i < ENEMY_COUNT; i++)
    {
      if (enemies[i].alive)
      {
        tft_fill_rect(
          old_enemy_x[i],
          old_enemy_y[i],
          enemies[i].width,
          enemies[i].height,
          0x0000
        );
      }
    }

    // Draw enemies at new positions
    for (int i = 0; i < ENEMY_COUNT; i++)
    {
      if (enemies[i].alive)
      {
        tft_fill_rect(
          enemies[i].x,
          enemies[i].y,
          enemies[i].width,
          enemies[i].height,
          0xF800
        );
      }
    }



    // Enemy stomp collision
    for (int i = 0; i < ENEMY_COUNT; i++)
    {
      if (enemies[i].alive &&
          player.vy > 0 &&
          player.y + player.height >= enemies[i].y &&
          player.x < enemies[i].x + enemies[i].width &&
          player.x + player.width > enemies[i].x)
      {
        // Erase the enemy
        tft_fill_rect(
          enemies[i].x,
          enemies[i].y,
          enemies[i].width,
          enemies[i].height,
          0x0000
        );

        enemies[i].alive = false;

        // Bounce player upward
        score += 100;
        draw_score(score);
        player.vy = -8;
      }
    }

    //win condition
    // Check whether all enemies are dead
    bool all_enemies_dead = true;

    for (int i = 0; i < ENEMY_COUNT; i++)
    {
      if (enemies[i].alive)
      {
        all_enemies_dead = false;
        break;
      }
    }

    // If all enemies are dead
    if (all_enemies_dead)
    {
      tft_fill_screen(0x07E0);

      while (gpio_get(JUMP_BUTTON))
      {
        sleep_ms(10);
      }

      lives = 3;
      score = 0;

      // Reset player
      player.x = 0;
      player.y = GROUND_Y - player.height;
      player.vx = 0;
      player.vy = 0;
      player.grounded = true;

      // Reset all enemies
      enemies[0].x = 250;
      enemies[0].y = GROUND_Y - enemies[0].height;
      enemies[0].vx = -1;
      enemies[0].alive = true;

      enemies[1].x = 150;
      enemies[1].y = GROUND_Y - enemies[1].height;
      enemies[1].vx = 1;
      enemies[1].alive = true;

      // Clear the green victory screen
      tft_fill_screen(0x0000);

      // Redraw ground
      tft_fill_rect(
        0,
        GROUND_Y,
        SCREEN_WIDTH,
        SCREEN_HEIGHT - GROUND_Y,
        0x07E0
      );

      // Redraw platforms
      for (int i = 0; i < PLATFORM_COUNT; i++)
      {
        tft_fill_rect(
          platforms[i].x,
          platforms[i].y,
          platforms[i].width,
          platforms[i].height,
          0x07E0
        );
      }

      // Redraw player
      player_draw(&player);

      // Redraw enemies
      for (int i = 0; i < ENEMY_COUNT; i++)
      {
        tft_fill_rect(
          enemies[i].x,
          enemies[i].y,
          enemies[i].width,
          enemies[i].height,
          0xF800
        );
      }

      draw_lives(lives);
      draw_score(score);
    }



    //enemy collision side
    // Enemy side collision
    for (int i = 0; i < ENEMY_COUNT; i++)
    {
      if (enemies[i].alive &&
          !hit_cooldown &&
          player.x < enemies[i].x + enemies[i].width &&
          player.x + player.width > enemies[i].x &&
          player.y < enemies[i].y + enemies[i].height &&
          player.y + player.height > enemies[i].y)
      {
        player.x -= player.vx;

        lives--;
        draw_lives(lives);

        // Reset player
        player.x = 0;
        player.y = GROUND_Y - player.height;
        player.vx = 0;
        player.vy = 0;
        player.grounded = true;

        hit_cooldown = true;

        if (lives <= 0)
        {
          tft_fill_screen(0x0000);

          while (gpio_get(JUMP_BUTTON))
          {
            sleep_ms(10);
          }

          lives = 3;

          // Redraw ground
          tft_fill_rect(
            0,
            GROUND_Y,
            SCREEN_WIDTH,
            SCREEN_HEIGHT - GROUND_Y,
            0x07E0
          );

          // Redraw platforms
          for (int j = 0; j < PLATFORM_COUNT; j++)
          {
            tft_fill_rect(
              platforms[j].x,
              platforms[j].y,
              platforms[j].width,
              platforms[j].height,
              0x07E0
            );
          }

          // Reset player
          player.x = 0;
          player.y = GROUND_Y - player.height;
          player.vx = 0;
          player.vy = 0;
          player.grounded = true;

          // Reset all enemies
          enemies[0].x = 250;
          enemies[0].y = GROUND_Y - enemies[0].height;
          enemies[0].vx = -1;
          enemies[0].alive = true;

          enemies[1].x = 150;
          enemies[1].y = GROUND_Y - enemies[1].height;
          enemies[1].vx = 1;
          enemies[1].alive = true;

          draw_lives(lives);
        }

        break; // Only one enemy can damage the player this frame
      }
    }

    //damage cooldown
    for (int i = 0; i < ENEMY_COUNT; i++)
    {
      if (hit_cooldown &&
          (player.x + player.width < enemies[i].x ||
           player.x > enemies[i].x + enemies[i].width))
      {
        hit_cooldown = false;
      }

      if (enemies[i].alive)
      {
        tft_fill_rect(
          enemies[i].x,
          enemies[i].y,
          enemies[i].width,
          enemies[i].height,
          0xF800
        );
      }
    }

    //jump
    if (!gpio_get(JUMP_BUTTON) && player.grounded)
    {
      player.vy = -14;
      player.grounded = false;
    }

    // Gravity
    if (!player.grounded)
    {
      player.vy += 1;
      player.y += player.vy;
    }

    // PLATFORM COLLISION falling
    for (int i = 0; i < PLATFORM_COUNT; i++)
    {
      if (player.vy > 0 &&
          old_y + player.height <= platforms[i].y &&
          player.y + player.height >= platforms[i].y &&
          player.x < platforms[i].x + platforms[i].width &&
          player.x + player.width > platforms[i].x)
      {
        player.y = platforms[i].y - player.height;
        player.vy = 0;
        player.grounded = true;
      }
    }

    //platform collision jumping
    for (int i = 0; i < PLATFORM_COUNT; i++)
    {
      if (player.vy < 0 &&
          old_y >= platforms[i].y + platforms[i].height &&
          player.y <= platforms[i].y + platforms[i].height &&
          player.x < platforms[i].x + platforms[i].width &&
          player.x + player.width > platforms[i].x)
      {
        player.y = platforms[i].y + platforms[i].height;
        player.vy = 0;
      }
    }


    // Check if player walked off the platform
    if (player.grounded)
    {
      bool on_platform = false;

      for (int i = 0; i < PLATFORM_COUNT; i++)
      {
        if (player.y + player.height == platforms[i].y &&
            player.x + player.width > platforms[i].x &&
            player.x < platforms[i].x + platforms[i].width)
        {
          on_platform = true;
          break;
        }
      }

      bool on_ground = (player.y + player.height == GROUND_Y);

      if (!on_platform && !on_ground)
      {
        player.grounded = false;
      }
    }

    //ground collision
    if (player.y + player.height >= GROUND_Y)
    {
      player.y = GROUND_Y - player.height;
      player.vy = 0;
      player.grounded = true;
    }


    //screen boundary left
    if (player.x < 0)
    {
      player.x = 0;
    }

    //screen boundary right
    if (player.x > SCREEN_WIDTH - player.width)
    {
      player.x = SCREEN_WIDTH - player.width;
    }


    if (player.x != old_x || player.y != old_y)
    {
      tft_fill_rect(old_x, old_y, player.width, player.height, 0x0000);
      player_draw(&player);
    }
    sleep_ms(16);
  }
}

void tft_send_command(uint8_t cmd) {
  gpio_put(PIN_DC, 0);
  spi_write_blocking(SPI_PORT, &cmd, 1);
}

void tft_send_data(uint8_t data) {
  gpio_put(PIN_DC, 1);
  spi_write_blocking(SPI_PORT, &data, 1);
}

void tft_reset() {
  gpio_put(PIN_RST, 1);
  sleep_ms(5);

  gpio_put(PIN_RST, 0);
  sleep_ms(20);

  gpio_put(PIN_RST, 1);
  sleep_ms(120);
}

void tft_init(void) {
  gpio_put(PIN_CS, 0);
  tft_send_command(0x01);
  sleep_ms(120);
  tft_send_command(0x11);
  sleep_ms(120);
  tft_send_command(0x3A);
  tft_send_data(0x55);
  tft_send_command(0x36);
  tft_send_data(0xE8);
  tft_send_command(0x29);
  sleep_ms(20);
  gpio_put(PIN_CS, 1);
}

void tft_set_address_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
  uint8_t x0_high = x0 >> 8;
  uint8_t x0_low  = x0 & 0xff;

  uint8_t x1_high = x1 >> 8;
  uint8_t x1_low  = x1 & 0xff;

  uint8_t y0_high = y0 >> 8;
  uint8_t y0_low  = y0 & 0xff;

  uint8_t y1_high = y1 >> 8;
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
  uint8_t color_high = color >> 8;
  uint8_t color_low = color & 0xff;
  tft_set_address_window(x, y, x, y);
  tft_send_data(color_high);
  tft_send_data(color_low);
  gpio_put(PIN_CS, 1);
}


void tft_send_data_buffer(uint8_t *data, size_t length) {
  gpio_put(PIN_DC, 1);
  spi_write_blocking(SPI_PORT, data, length);
}

void tft_fill_screen(uint16_t color)
{
  gpio_put(PIN_CS, 0);
  uint8_t color_high = color >> 8;
  uint8_t color_low = color & 0xff;
  tft_set_address_window(0, 0, 319, 239);
  uint8_t buffer[BUFFER_SIZE];
  for (int i = 0; i < BUFFER_SIZE; i += 2) {
    buffer[i] = color_high;
    buffer[i + 1] = color_low;
  }
  for (int i = 0; i < 75 * 2; i++) {
    tft_send_data_buffer(buffer, 1024);
  }
  gpio_put(PIN_CS, 1);
}

void tft_fill_rect(uint16_t x0, uint16_t y0, uint16_t width, uint16_t height, uint16_t color) {
  gpio_put(PIN_CS, 0);
  uint8_t color_high = color >> 8;
  uint8_t color_low  = color & 0xff;
  tft_set_address_window(x0, y0, x0 + width - 1, y0 + height - 1);


  uint8_t buffer[BUFFER_SIZE];


  for (int i = 0; i < BUFFER_SIZE; i += 2) {
    buffer[i] = color_high;
    buffer[i + 1] = color_low;
  }
  uint16_t total_bytes = width * height * 2;
  while (total_bytes >= BUFFER_SIZE) {
    tft_send_data_buffer( buffer, 1024);
    total_bytes -= BUFFER_SIZE;
  }
  if (total_bytes > 0) {
    tft_send_data_buffer(buffer, total_bytes);
  }
  gpio_put(PIN_CS, 1);
}

void player_draw(Player *player)
{
  tft_fill_rect(
    player->x,
    player->y,
    player->width,
    player->height,
    0xFFFF
  );
}

void draw_lives(int lives)
{
  for (int i = 0; i < 3; i++)
  {
    if (i < lives)
    {
      tft_fill_rect(
        10 + i * 20,
        10,
        12,
        12,
        0xF800
      );
    }
    else
    {
      tft_fill_rect(
        10 + i * 20,
        10,
        12,
        12,
        0x0000
      );
    }
  }
}

void draw_score(int score)
{
  // Clear score area
  tft_fill_rect(250, 10, 60, 12, 0x0000);

  // One block for every 100 points
  int blocks = score / 100;

  for (int i = 0; i < blocks; i++)
  {
    tft_fill_rect(
      250 + i * 15,
      10,
      10,
      10,
      0xFFE0
    );
  }
}
