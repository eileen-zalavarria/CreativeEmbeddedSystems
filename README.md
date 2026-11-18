# About
Repository containing projects for COMS BC3930 Creative Embedded Systems. Each file corresponds to one module project.
Links to blog posts for each project:
- Module 1 Generative Art: https://sideways-danthus-922.notion.site/Module-1-Art-Installation-Blog-3f3cf4070bd98008b7c8d9451a395e77?source=copy_link
- Module 2 Interactive Devices: https://app.notion.com/p/Module-2-Interactive-Devices-Project-408cf4070bd98090b96ddb3b5c5e1255?source=copy_link
- Module 3 Color Lab: https://app.notion.com/p/Module-3-Distributed-Communication-Partner-Project-41bcf4070bd9804b9d38db6e1c68d957?source=copy_link

# Module 1 
The project file includes an Arduino sketch of randomly generated multi-colored emoticons streaming across a black screen for an ESP32. The goal with this project was for my art piece to reference older internet chat culture, and evoke nostalgia associated with those times. Simply clone this sketch onto your device and load it in Arduino and connect your ESP32. 

<img width="480" height="422" alt="gif" src="https://github.com/user-attachments/assets/c31c0a0b-e750-4f93-80c8-e2f1ed5a3b29" />

# Module 2 
The project files include an Arduino sketch to capture joystick, potentiometer, and button data from a breadboard circuit built using a LILYGO TTGO ESP32 and a Processing media generation program that works as a pet grooming simulator. The hardware used involves an ESP32, wires, a joystick, a potentiometer, a button, and a breadboard and usb cable. 
Here is a recreation of the breadboard circuit in Fritzing:

<img width="695" height="380" alt="Screenshot 2026-10-29 at 7 15 47 PM" src="https://github.com/user-attachments/assets/22fb972b-c947-4b1d-89b8-96275be343ce" />

Design goals for this project: I wanted to give the user the ability to switch between some set of objects (brush, sponge, hose, etc.) with the button, and offer the satisfying element of interaction with the tools with the joystick. The design should evoke nostalgia for old childhood online games, and I wanted to give the options of grooming multiple animals and having them come in and out, similar to a salon, so I decided the button would signal the end of the grooming of one pet and the entrance of another. I also decided that audio integration for the tools was necessary for providing the experience that I was inspired by. 

The finished project is a cute pet simulator game that includes multiple objects and grooming choices: cutting fur, washing, scrubbing, blowdrying. The animal sprites were purchased from this link: https://assets2d.itch.io/kawaii-pixel-pet-pack on itch.io and a free retro bathroom pixel sprite pack was also used to design the salon from here: https://0-mem0ry.itch.io/retro-bathroom-furniture-set-free . I designed the background using images on free images on the internet, and the game title screen and salon name were made by me in Canva. The tool objects were generic free ones found (here)[https://www.magnific.com/app]. Simply upload the files to your Processing sketch and you're good to go. All is included in the Module 2 data package.

<img width="400" height="429" alt="pawlished_salon" src="https://github.com/user-attachments/assets/4585caa3-e72c-44db-a9f2-b539f99744f0" />

# Module 3: Color Lab

Color Lab is a two-player color matching game made with two ESP32 LILYGO T-Display boards. The boards talk to each other over ESP-NOW, so both players see the same thing on their screens.

<img width="420" height="399" alt="color_lab_demo" src="https://github.com/user-attachments/assets/7b122a78-4f58-4ad1-bd31-1f396ef0f7ce" />


### Design Goals

We wanted to make a co-op version of those color matching games where you try to get as close as you can to a target color. Here's how our game works:

- Red is locked to the target, so you only have to worry about two colors
- Player 0 controls green and player 1 controls blue
- The left bottle shows the target color, and the right bottle shows your team's mix, which updates live when either of you changes your color
- When you both submit, a bar shows how close you got as a percentage
- We went for a cozy pixel art look for the screen

### What You Need

- 2 LILYGO T-Display ESP32 boards
- 2 USB-C cables
- Arduino IDE
- ESP32 board package (version 3.x)
- TFT_eSPI library

Make sure `module3.ino` and `color_lab_background.h` are in the same folder. The `.h` file is the background image saved as pixel data, and the code won't compile without it.

### How to Build It

1. In Arduino IDE, go to Boards Manager, search "esp32", and install the one by Espressif. You need version 3.x or the code will throw an error.
2. Install TFT_eSPI from Library Manager.
3. Set up TFT_eSPI for the T-Display. Find `User_Setup_Select.h` in the TFT_eSPI library folder, comment out `#include <User_Setup.h>`, and uncomment this line:

   ```cpp
   #include <User_Setups/Setup25_TTGO_T_Display.h>
   ```

   If you skip this, the screen will probably just stay blank.

4. Change `TEAM_ID` to a random number that other people nearby aren't using. It has to be the same on both boards.

   ```cpp
   #define TEAM_ID 74
   ```

5. For the first board, set `PLAYER_ID` to 0 and upload (we used **ESP32 Dev Module** as the board). This board is the host and picks the target colors.

   ```cpp
   #define PLAYER_ID 0
   ```

6. For the second board, change it to `#define PLAYER_ID 1` and upload again.
7. Turn both boards on. The blue board will say "Waiting for target..." for a second until it hears from the host.

### How to Play

- Tap the **right button** to add 5 to your color
- Tap the **left button** to subtract 5
- **Hold** either button for 2 seconds to submit
- Once you both submit, your score shows up for about 8 seconds, and then a new round starts on its own

### How It Works

- The host (player 0) picks a random target color and sends it to the other board
- Each board sends its color value whenever it changes, and a backup message every second in case one gets lost
- The screen gets drawn in memory first and then pushed all at once, which stopped the flickering issue we encountered
