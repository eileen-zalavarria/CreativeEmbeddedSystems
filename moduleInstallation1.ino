#include <TFT_eSPI.h> 
#include <SPI.h>
TFT_eSPI tft = TFT_eSPI(); 

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 135
#define SPAWN_MARGIN 60
#define NUM_ROWS 4
#define SPRITE_WIDTH  (SCREEN_WIDTH + SPAWN_MARGIN)
#define SPRITE_HEIGHT 33
#define TEXT_Y 4
#define BG_COLOR TFT_BLACK

// 4 sprite objects
TFT_eSprite sprite1 = TFT_eSprite(&tft);
TFT_eSprite sprite2 = TFT_eSprite(&tft);
TFT_eSprite sprite3 = TFT_eSprite(&tft);
TFT_eSprite sprite4 = TFT_eSprite(&tft);
TFT_eSprite* rows[NUM_ROWS] = {&sprite1, &sprite2, &sprite3, &sprite4};

const char* emotiCons[] = {
  ":-)",
  "<3",
  "XD",
  ":D",
  "xD",
  "D:",
  ":3",
  ";)",
  ":/",
  "x_x",
  ">_<",
  ">w<",
  ":P",
  ":|",
  ":]",
  ":(",
  ">:)",
  ":^)",
  ":'(",
  "B)",
  ":0",
  ">:(",
  "XP",
  "^_^",
  "^o^",
  "0_0"
};
const int totalEmoticons = sizeof(emotiCons) / sizeof(emotiCons[0]);

const uint32_t colors[] = {
  TFT_WHITE,
  TFT_MAGENTA,
  TFT_SKYBLUE,
  TFT_YELLOW,
  TFT_BLUE,
  TFT_ORANGE,
  TFT_VIOLET,
  TFT_GREEN
};
const int totalColors = sizeof(colors) / sizeof(colors[0]);

// Per-row settings
int rowY[NUM_ROWS]          = {1, 34, 67, 100};
int spawnInterval[NUM_ROWS] = {450, 600, 350, 500};  // ms between new emoticons
int shiftPx[NUM_ROWS]       = {4, 4, 4, 4};          // px moved per step; change per row for different speeds
const int SHIFT_INTERVAL    = 25;                    // ms between steps

unsigned long spawnTimer[NUM_ROWS] = {0};
unsigned long shiftTimer[NUM_ROWS] = {0};

void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(1);          // landscape
  tft.fillScreen(BG_COLOR);

  for (int i = 0; i < NUM_ROWS; i++) {
    if (!rows[i]->createSprite(SPRITE_WIDTH, SPRITE_HEIGHT)) {
      Serial.printf("Sprite %d failed to allocate\n", i + 1);
    }
    rows[i]->fillSprite(BG_COLOR);
    rows[i]->setScrollRect(0, 0, SPRITE_WIDTH, SPRITE_HEIGHT, BG_COLOR);
    rows[i]->setTextFont(4);
    rows[i]->setTextSize(1);
  }
}

void loop() {
  unsigned long now = millis();

  for (int i = 0; i < NUM_ROWS; i++) {
    // Glide the row leftward
    if (now - shiftTimer[i] >= SHIFT_INTERVAL) {
      shiftTimer[i] = now;
      rows[i]->scroll(-shiftPx[i], 0);
      rows[i]->pushSprite(0, rowY[i]);
    }

    // Drop a new emoticon just past the right edge of the screen
    if (now - spawnTimer[i] >= spawnInterval[i]) {
      spawnTimer[i] = now;
      rows[i]->setCursor(SCREEN_WIDTH + 5, TEXT_Y);
      rows[i]->setTextColor(colors[random(totalColors)], BG_COLOR);
      rows[i]->print(emotiCons[random(totalEmoticons)]);
    }
  }
}