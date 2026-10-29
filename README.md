# About
Repository containing projects for COMS BC3930 Creative Embedded Systems. Each file corresponds to one module project.
Links to blog posts for each project:
- (Module 1 Generative Art)[https://sideways-danthus-922.notion.site/Module-1-Art-Installation-Blog-3f3cf4070bd98008b7c8d9451a395e77?source=copy_link]
- (Module 2 Interactive Devices)[https://app.notion.com/p/Module-2-Interactive-Devices-Project-408cf4070bd98090b96ddb3b5c5e1255?source=copy_link]

# Module 1 
The project file includes an Arduino sketch of randomly generated multi-colored emoticons streaming across a black screen for an ESP32. The goal with this project was for my art piece to reference older internet chat culture, and evoke nostalgia associated with those times. Simply clone this sketch onto your device and load it in Arduino and connect your ESP32. 

<img width="480" height="422" alt="gif" src="https://github.com/user-attachments/assets/c31c0a0b-e750-4f93-80c8-e2f1ed5a3b29" />

# Module 2 
The project files include an Arduino sketch to capture joystick, potentiometer, and button data from a breadboard circuit built using a LILYGO TTGO ESP32 and a Processing media generation program that works as a pet grooming simulator. The hardware used involves an ESP32, wires, a joystick, a potentiometer, a button, and a breadboard and usb cable. 
Here is a recreation of the breadboard circuit in Fritzing:

<img width="695" height="380" alt="Screenshot 2026-10-29 at 7 15 47 PM" src="https://github.com/user-attachments/assets/22fb972b-c947-4b1d-89b8-96275be343ce" />

Design goals for this project: I wanted to give the user the ability to switch between some set of objects (brush, sponge, hose, etc.) with the button, and offer the satisfying element of interaction with the tools with the joystick. The design should evoke nostalgia for old childhood online games, and I wanted to give the options of grooming multiple animals and having them come in and out, similar to a salon, so I decided the button would signal the end of the grooming of one pet and the entrance of another. I also decided that audio integration for the tools was necessary for providing the experience that I was inspired by. 

The finished project is a cute pet simulator game that includes multiple objects and grooming choices: cutting fur, washing, scrubbing, blowdrying. The animal sprites were purchased from this (link)[https://assets2d.itch.io/kawaii-pixel-pet-pack on itch.io] and a free retro bathroom pixel sprite pack was also used to design the salon from (here)[https://0-mem0ry.itch.io/retro-bathroom-furniture-set-free]. I designed the background using images on free images on the internet, and the game title screen and salon name were made by me in Canva:

<img img width="600" height="220" alt="startgame" src="https://github.com/user-attachments/assets/06396732-cd1a-46df-bdd5-c92f8e88ba04" />
<img width="400" height="108" alt="salonname" src="https://github.com/user-attachments/assets/b0cb8528-deb0-4eaf-a73e-7a8b3944dd48" />
<img width="300" height="311" alt="salonbackground" src="https://github.com/user-attachments/assets/5b30a936-7731-41cf-9f0b-a3799ee67c7d" />

The tool objects were generic free ones found (here)[https://www.magnific.com/app]. Simply upload the files to your Processing sketch and you're good to go.

<img width="400" height="429" alt="pawlished_salon" src="https://github.com/user-attachments/assets/4585caa3-e72c-44db-a9f2-b539f99744f0" />













