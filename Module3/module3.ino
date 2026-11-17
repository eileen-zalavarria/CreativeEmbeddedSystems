/* Module 3: Color Lab
ESP32 LILYGO T-Display


Two-player color matching game.
Player 0 generates the target and controls green; player 1 controls blue.
Tap A/B to decrease/increase. Hold either button for two seconds to submit.
Both players see the score, then the host starts another round.
*/


#include "color_lab_background.h"


#include <WiFi.h>
#include <esp_now.h>
#include <TFT_eSPI.h>
#include <esp_system.h>


#define TEAM_ID       74  // Must match on both boards
#define TARGET_GENERATOR_ID 0


// CHANGE TO 0 FOR GREEN ESP32
// CHANGE TO 1 FOR BLUE ESP32
#define PLAYER_ID     1


#define MAX_PLAYERS   2


#define BUTTON_A      0
#define BUTTON_B      35
#define HEARTBEAT_MS  1000


// Liquid inside bottles
#define TARGET_X   34
#define MIX_X      95
#define LIQUID_Y   64
#define LIQUID_W   38
#define LIQUID_H   24


// Progress bar
#define BAR_X      24
#define BAR_Y      116
#define BAR_W      142
#define BAR_H      6


uint8_t broadcastAddress[] = {
 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};


TFT_eSPI tft = TFT_eSPI();
TFT_eSprite frame = TFT_eSprite(&tft);


// Two players
volatile uint16_t channel[MAX_PLAYERS] = {0, 0};


volatile bool needsRedraw = true;
bool submitted[MAX_PLAYERS] = {false, false};
bool peerSawResult = false;
uint32_t resultAt = 0;
#define RESULT_TIME 8000
bool waitForRelease = true;


uint32_t lastHeartbeat = 0;


// Each button has its own debounce and hold timer.
#define HOLD_TIME 2000
#define DEBOUNCE_MS 20
struct ButtonState {
 bool raw = HIGH;
 bool stable = HIGH;
 uint32_t changedAt = 0;
 uint32_t pressedAt = 0;
 bool longHandled = false;
};
ButtonState buttonA;
ButtonState buttonB;


int red = 0;
int green = 0;
int blue = 0;


volatile int targRed = 0;
volatile int targGreen = 0;
volatile int targBlue = 0;


volatile bool targetReady = false;


// Track the current target round
volatile uint32_t roundId = 0;


// Unique session for generator restarts
volatile uint32_t sessionId = 0;


// Receive callback passes data to loop
portMUX_TYPE stateMux = portMUX_INITIALIZER_UNLOCKED;
volatile bool pendingMessage = false;


// ESP-NOW message
typedef struct {
 uint8_t teamId;
 uint8_t playerId;
 uint8_t flags; // bit 0: submitted; bit 1: both submissions received
 uint8_t value;


 uint8_t targR;
 uint8_t targG;
 uint8_t targB;


 uint32_t roundNumber;
 uint32_t sessionNumber;
} Message;


Message receivedMessage = {};


// Function declarations
void getTargetColor();
void startNewRound();
void sendState();
void resetRoundState();
int readButton(uint8_t pin, ButtonState &button);
void drawBackground();
void setupEspNow();
void processReceivedMessage();
void drawGame();


// ========================================
// RECEIVE DATA
// ========================================


void onReceive(
const esp_now_recv_info_t *info,
const uint8_t *data,
int len
) {
 if (len != sizeof(Message)) return;


 Message msg;
 memcpy(&msg, data, sizeof(msg));


 if (msg.teamId != TEAM_ID) return;
 if (msg.playerId >= MAX_PLAYERS) return;
 if (msg.playerId == PLAYER_ID) return;


 // Keep callback short; process in loop
 portENTER_CRITICAL(&stateMux);
 receivedMessage = msg;
 pendingMessage = true;
 portEXIT_CRITICAL(&stateMux);
}


// ========================================
// PROCESS RECEIVED DATA
// ========================================


void processReceivedMessage() {
 Message msg;


 portENTER_CRITICAL(&stateMux);


 if (!pendingMessage) {
   portEXIT_CRITICAL(&stateMux);
   return;
 }


 msg = receivedMessage;
 pendingMessage = false;


 portEXIT_CRITICAL(&stateMux);


 // Only the designated ESP32 generates target RGB
 if (msg.playerId == TARGET_GENERATOR_ID &&
 PLAYER_ID != TARGET_GENERATOR_ID) {


   bool newTarget =
   !targetReady ||
   sessionId != msg.sessionNumber ||
   roundId != msg.roundNumber;


   if (newTarget) {
     sessionId = msg.sessionNumber;
     roundId = msg.roundNumber;


     targRed = msg.targR;
     targGreen = msg.targG;
     targBlue = msg.targB;


     // Reset both colors for the new target
     resetRoundState();


     targetReady = true;
     needsRedraw = true;


     Serial.println("TARGET RECEIVED");


     Serial.printf(
     "RGB: %d, %d, %d\n",
     targRed, targGreen, targBlue
     );


     // Immediately send our current state
     sendState();
   }
 }


 // Do not accept values for an old round
 if (!targetReady) return;


 if (msg.playerId != TARGET_GENERATOR_ID) {
   if (msg.sessionNumber != sessionId ||
   msg.roundNumber != roundId) {
     return;
   }
 }


 // Freeze the teammate's value once their submission arrives.
 if (!submitted[msg.playerId]) {
   if (channel[msg.playerId] != msg.value || (msg.flags & 1)) {
     channel[msg.playerId] = msg.value;
     submitted[msg.playerId] = (msg.flags & 1) != 0;
     needsRedraw = true;
   }
 }
 if (msg.flags & 2) peerSawResult = true;


}


// ========================================
// ESP-NOW SETUP
// ========================================


void setupEspNow() {
 WiFi.mode(WIFI_STA);


 if (esp_now_init() != ESP_OK) {
   tft.fillScreen(TFT_BLACK);
   tft.setTextColor(TFT_RED);
   tft.drawString("ESP-NOW init failed", 10, 60, 2);


   while (true) {
     delay(1000);
   }
 }


 esp_now_register_recv_cb(onReceive);


 esp_now_peer_info_t peer = {};
 memcpy(peer.peer_addr, broadcastAddress, 6);


 peer.channel = 0;
 peer.encrypt = false;


 if (esp_now_add_peer(&peer) != ESP_OK) {
   Serial.println("ESP-NOW peer failed");
 }
}


// ========================================
// SEND DATA
// ========================================


void sendState() {
 Message msg = {};


 msg.teamId = TEAM_ID;
 msg.playerId = PLAYER_ID;
 msg.flags = (submitted[PLAYER_ID] ? 1 : 0) |
             ((submitted[0] && submitted[1]) ? 2 : 0);
 msg.value = channel[PLAYER_ID];


 msg.targR = targRed;
 msg.targG = targGreen;
 msg.targB = targBlue;


 msg.roundNumber = roundId;
 msg.sessionNumber = sessionId;


 esp_err_t status = esp_now_send(broadcastAddress, (uint8_t *)&msg, sizeof(msg));
 if (status != ESP_OK) Serial.printf("ESP-NOW send error: %d\n", (int)status);
}


// ========================================
// BACKGROUND
// ========================================


void drawBackground() {
 tft.setSwapBytes(true);


 tft.pushImage(
 0, 0, 240, 135,
 colorLabBackground
 );
}


// ========================================
// GENERATE RANDOM TARGET
// ========================================


void getTargetColor() {
 if (PLAYER_ID != TARGET_GENERATOR_ID) return;


 targRed = random(0, 52) * 5;
 targGreen = random(0, 52) * 5;
 targBlue = random(0, 52) * 5;


 roundId++;


 targetReady = true;
 needsRedraw = true;


 Serial.println("NEW RANDOM TARGET");


 Serial.printf(
 "RGB: %d, %d, %d\n",
 targRed, targGreen, targBlue
 );
}


// ========================================
// NEW ROUND
// ========================================


void resetRoundState() {
 channel[0] = channel[1] = 0;
 submitted[0] = submitted[1] = false;
 peerSawResult = false;
 resultAt = 0;
 waitForRelease = true;
 buttonA = ButtonState{};
 buttonB = ButtonState{};
}


void startNewRound() {
 if (PLAYER_ID != TARGET_GENERATOR_ID) return;


 resetRoundState();


 getTargetColor();
 sendState();


 needsRedraw = true;
}


// ========================================
// BUTTON PRESS
// ========================================


// Return 1 for a short press, 2 for a long press, or 0 for no event.
// Short presses are detected on release so holding does not change the color.
int readButton(uint8_t pin, ButtonState &button) {
 uint32_t now = millis();
 bool raw = digitalRead(pin);
 if (raw != button.raw) {
   button.raw = raw;
   button.changedAt = now;
 }
 if (raw != button.stable && now - button.changedAt >= DEBOUNCE_MS) {
   button.stable = raw;
   if (raw == LOW) {
     button.pressedAt = now;
     button.longHandled = false;
   } else if (!button.longHandled) {
     button.longHandled = true;
     return (now - button.pressedAt >= HOLD_TIME) ? 2 : 1;
   }
 }
 // Report a long press once, while the button is still held.
 if (button.stable == LOW && !button.longHandled &&
 now - button.pressedAt >= HOLD_TIME) {
   button.longHandled = true;
   return 2;
 }
 return 0;
}


// ========================================
// SETUP
// ========================================


void setup() {
 Serial.begin(115200);


 pinMode(BUTTON_A, INPUT_PULLUP);


 // GPIO 35 requires an external resistor
 // for a stable button input.
 pinMode(BUTTON_B, INPUT);


 tft.init();
 tft.setRotation(1);
 frame.createSprite(240, 135);
 frame.setSwapBytes(true);


 drawBackground();


 randomSeed(esp_random());


 setupEspNow();


 // Identify a fresh generator session
 if (PLAYER_ID == TARGET_GENERATOR_ID) {
   sessionId = esp_random();


   startNewRound();
 }


 Serial.println("Color Lab loaded!");


 Serial.printf(
 "Player %d: %s\n",
 PLAYER_ID,
 PLAYER_ID == 0 ? "GREEN" : "BLUE"
 );
}


// ========================================
// MAIN LOOP
// ========================================


void loop() {


 // Process wireless messages safely
 processReceivedMessage();


 // Ignore a button held over from the previous round.
 if (waitForRelease) {
   if (digitalRead(BUTTON_A) == HIGH && digitalRead(BUTTON_B) == HIGH) {
     waitForRelease = false;
   }
 } else {
   int a = readButton(BUTTON_A, buttonA);
   int b = readButton(BUTTON_B, buttonB);
   if (targetReady && !submitted[PLAYER_ID]) {
     if (a == 2 || b == 2) {
       submitted[PLAYER_ID] = true;
       Serial.println("SUBMITTED");
       sendState();
       needsRedraw = true;
     } else if (a == 1 || b == 1) {
       int v = channel[PLAYER_ID] + (b == 1 ? 5 : -5);
       channel[PLAYER_ID] = constrain(v, 0, 255);
       sendState();
       needsRedraw = true;
       Serial.printf("My value: %d\n", (int)channel[PLAYER_ID]);
     }
   }
 }


 // The host waits until the teammate has also received the final result.
 // Then both screens get at least eight seconds to display their score.
 if (submitted[0] && submitted[1]) {
   if (resultAt == 0 && (PLAYER_ID != TARGET_GENERATOR_ID || peerSawResult)) {
     resultAt = millis();
     sendState();
     needsRedraw = true;
   }
   if (PLAYER_ID == TARGET_GENERATOR_ID && peerSawResult && resultAt != 0 &&
       millis() - resultAt >= RESULT_TIME) {
     startNewRound();
   }
 }


 // Heartbeat to recover missed messages
 if (millis() - lastHeartbeat > HEARTBEAT_MS) {
   lastHeartbeat = millis();
   sendState();
 }


 if (needsRedraw) {
   needsRedraw = false;
   drawGame();
 }
 delay(1); // Yield to the ESP32 wireless tasks.
}


// ========================================
// DRAW GAME
// ========================================


void drawGame() {


 // Red is automatically locked
 red = targRed;


 // Two player-controlled colors
 green = channel[0];
 blue = channel[1];


 frame.pushImage(0, 0, 240, 135, colorLabBackground);


 if (targetReady) {


   uint16_t targetColor =
   frame.color565(targRed, targGreen, targBlue);


   uint16_t mixColor =
   frame.color565(red, green, blue);


   // Target bottle
   frame.fillRoundRect(
   TARGET_X, LIQUID_Y,
   LIQUID_W, LIQUID_H,
   3, targetColor
   );


   // Mixed bottle
   frame.fillRoundRect(
   MIX_X, LIQUID_Y,
   LIQUID_W, LIQUID_H,
   3, mixColor
   );


   // Shine marks
   frame.fillRect(
   TARGET_X + 3, LIQUID_Y + 2,
   3, 3, TFT_WHITE
   );


   frame.fillRect(
   MIX_X + 3, LIQUID_Y + 2,
   3, 3, TFT_WHITE
   );




 }


 // Draw all overlays on the sprite to avoid screen flicker.
 frame.fillRect(BAR_X, BAR_Y, BAR_W, BAR_H, TFT_BLACK);
 frame.fillRect(185, 108, 50, 20, TFT_BLACK);
 frame.setTextDatum(TL_DATUM);
 frame.setTextPadding(25);
 frame.setTextColor(TFT_RED, TFT_BLACK);
 frame.drawString(String(red), 170, 41, 2);
 frame.setTextColor(TFT_GREEN, TFT_BLACK);
 frame.drawString(String(green), 170, 60, 2);
 frame.setTextColor(TFT_BLUE, TFT_BLACK);
 frame.drawString(String(blue), 170, 79, 2);
 frame.setTextPadding(0);


 // Red is already matched, so only green and blue count toward accuracy.
 bool complete = submitted[0] && submitted[1];
 if (complete) {
   int diff = abs(green - targGreen) + abs(blue - targBlue);
   int percent = 100 - (diff * 100 + 255) / 510;
   frame.fillRect(BAR_X, BAR_Y, BAR_W * percent / 100, BAR_H,
                  frame.color565(180, 140, 240));
   frame.setTextColor(TFT_GREEN, TFT_BLACK);
   frame.drawString(String(percent) + "%", 185, 108, 2);
 }


 // Show submission state directly on the ESP32, not just in Serial Monitor.
 frame.fillRect(0, 24, 160, 16, TFT_BLACK);
 frame.setTextDatum(MC_DATUM);
 frame.setTextColor(TFT_YELLOW, TFT_BLACK);
 if (!targetReady) {
   frame.drawString("Waiting for target...", 120, 32, 2);
 } else if (complete) {
   frame.drawString("Round complete!", 120, 32, 2);
 } else if (submitted[PLAYER_ID]) {
   frame.drawString("Submitted! Waiting...", 120, 32, 2);
 } else if (submitted[1 - PLAYER_ID]) {
   frame.drawString("Partner submitted", 120, 32, 2);
 } else {
   frame.drawString("Hold to submit", 120, 32, 2);
 }
 frame.setTextDatum(TL_DATUM);
 frame.pushSprite(0, 0);
}


