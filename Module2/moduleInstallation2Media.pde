import processing.serial.*;
import processing.sound.*;

// ---------- SERIAL ----------
Serial myPort;
boolean useMouse = false;     // Claude Opus 5.5: falls back to mouse + keyboard if no board is found
int analogMax = 4095;         // ESP32 analog range

// read from the board
float joyDX = 0, joyDY = 0;   // joystick tilt, -1 to 1
int toolIndex = 0;            // 0 sponge, 1 shower, 2 blow dryer, 3 scissors
int lastButton = 0;
boolean donePressed = false;

// images
PImage background, salonName, startScreen;
PImage tub, counter, rug, curtain;
PImage sponge, shower, dryer, scissors, circle;
PImage[] toolImgs;
PImage[] sheet;
PImage[][] idleAnims, runAnims;

int frameW = 128;
int frameH = 128;

// selection circle x position for each tool (sponge, shower, dryer, scissors)
float[] circleX = { 30, 141, 247, 370 };
float circleY = 415;

// sound
SoundFile[] toolSounds;
int lastScrubFrame = -100;
int lastSnipFrame = -100;

// ---------- TOOL CURSOR ----------
// Claude Opus 5.5: position and feel of the tool that follows the joystick
float toolX = 250, toolY = 200;
float prevToolX, prevToolY;
float toolSpeed = 5;
float deadzone = 0.15;
float cursorScale = 0.2;

// Pets 
final int WAITING = 0, ENTERING = 1, HOPPING = 2, IDLE = 3, LEAVING = 4;
int state = WAITING;
int currentPet = 0;

// size per pet: 0 dog, 1 redcat, 2 purplecat, 3 brownhamster, 4 redhamster, 5 greycat, 6 bunny, 7 fox
float[] petScales = { 0.75, 0.9, 0.9, 0.9, 0.9, 0.9, 0.9, 0.9 };
float petSize;
float petX, petY;             // feet position (bottom center of the sprite)

// stations: where the pet's feet go
float tubX = 232, tubY = 300;
float rugX = 235, rugY = 368;
float counterX = 388, counterY = 265;
float floorY = 400;           // walking height on the floor
float entryX = 130;           // where it stops before hopping into the tub
int station = 0;              // 0 = tub, 1 = rug, 2 = counter

// hop
float hopStartX, hopStartY, hopEndX, hopEndY;
float hopT = 0;
float hopHeight = 60;
int afterHop;

// ---------- PARTICLES ----------
// Claude Opus 5.5: lists holding every bubble, drop, air puff, and fur tuft on screen
ArrayList<Bubble> bubbles = new ArrayList<Bubble>();
ArrayList<Drop> drops = new ArrayList<Drop>();
ArrayList<Puff> puffs = new ArrayList<Puff>();
ArrayList<Fur> furs = new ArrayList<Fur>();


void setup() {
  size(500, 500);
  noSmooth();

  // pet sprite sheets
  sheet = new PImage[8];
  sheet[0] = loadImage("dog.png");
  sheet[1] = loadImage("redcat.png");
  sheet[2] = loadImage("purplecat.png");
  sheet[3] = loadImage("brownhamster.png");
  sheet[4] = loadImage("redhamster.png");
  sheet[5] = loadImage("greycat.png");
  sheet[6] = loadImage("bunny.png");
  sheet[7] = loadImage("fox.png");

  // background, furniture, and tools
  background  = loadImage("salonbackground.jpg");
  salonName   = loadImage("salonname.png");
  startScreen = loadImage("startgame.png");
  sponge   = loadImage("sponge.png");
  shower   = loadImage("shower.png");
  dryer    = loadImage("dryer.png");
  scissors = loadImage("scissors.png");
  circle   = loadImage("circle.png");
  tub     = loadImage("tub.png");
  counter = loadImage("window.png");
  rug     = loadImage("rug.png");
  curtain = loadImage("curtain.png");

  // one sound per tool
  toolSounds = new SoundFile[] {
    new SoundFile(this, "bubbles.mp3"),
    new SoundFile(this, "water.mp3"),
    new SoundFile(this, "dryer.mp3"),
    new SoundFile(this, "snip.mp3")
  };

  // tool images in pot order, so the cursor can show the selected one
  toolImgs = new PImage[] { sponge, shower, dryer, scissors };

  // Claude Opus 5.5: slice idle + run animations out of each sheet (rows/counts differ per pet)
  int[] idleRow   = { 0, 0, 0, 0, 0, 0, 0, 0 };
  int[] idleCount = { 6, 6, 6, 6, 6, 5, 5, 5 };
  int[] runRow    = { 4, 4, 4, 4, 4, 2, 2, 2 };
  int[] runCount  = { 6, 6, 6, 6, 6, 4, 6, 6 };

  idleAnims = new PImage[sheet.length][];
  runAnims  = new PImage[sheet.length][];
  for (int i = 0; i < sheet.length; i++) {
    idleAnims[i] = loadAnim(sheet[i], idleRow[i], idleCount[i]);
    runAnims[i]  = loadAnim(sheet[i], runRow[i], runCount[i]);
  }

  // connect to the board (opened last so everything is loaded before data arrives)
  printArray(Serial.list());
  try {
    String portName = Serial.list()[2];
    println(portName);
    myPort = new Serial(this, portName, 9600);   // baud rate must match the board's sketch
    myPort.bufferUntil('\n');
  }
  catch (Exception e) {
    println("No board found, using mouse. SPACE = button, 1-4 = tool");
    useMouse = true;
  }
}


void draw() {
  // Claude Opus 5.5: move the tool with the joystick (or mouse when testing)
  prevToolX = toolX;
  prevToolY = toolY;
  if (useMouse) {
    toolX = mouseX;
    toolY = mouseY;
  } else {
    toolX += joyDX * toolSpeed;
    toolY += joyDY * toolSpeed;
  }
  toolX = constrain(toolX, 0, width);
  toolY = constrain(toolY, 0, height);

  // Claude Opus 5.5: button starts the game, or sends a groomed pet away
  if (donePressed) {
    if (state == WAITING) startPet();
    else if (state == IDLE) startHop(petX + 50, floorY, LEAVING);   // hop down, then run
    donePressed = false;
  }

  updatePet();

  // background and furniture
  image(background, 0, 0);
  image(salonName, 100, 70, salonName.width * .27, salonName.height * .27);
  image(curtain, 55, 5, curtain.width * 5, curtain.height * 5);
  image(rug, 182, 320, rug.width * 2, rug.height * 2);
  image(counter, 338, 235, counter.width * 3, counter.height * 3);
  image(tub, 170, 198, tub.width * 2, tub.height * 2);

  drawPet();

  // Claude Opus 5.5: spawn effects + snip sound while the tool moves over the pet
  float moved = dist(toolX, toolY, prevToolX, prevToolY);
  if (state == IDLE && overPet() && moved > 1) {
    if (toolIndex == 0) bubbles.add(new Bubble(toolX, toolY));
    if (toolIndex == 1) drops.add(new Drop(toolX, toolY));
    if (toolIndex == 2) puffs.add(new Puff(toolX, toolY));
    if (toolIndex == 3 && frameCount % 3 == 0) furs.add(new Fur(toolX, toolY, furColor()));

    lastScrubFrame = frameCount;

    if (toolIndex == 3 && frameCount - lastSnipFrame > 30 && !toolSounds[3].isPlaying()) {
      toolSounds[3].play();
      lastSnipFrame = frameCount;
    }
  }
  updateSounds();
  updateParticles();

  // tool bar
  image(sponge, 35, 415, sponge.width * .3, sponge.height * .3);
  image(shower, 115, 410, shower.width * .3, shower.height * .3);
  image(dryer, 243, 420, dryer.width * .3, dryer.height * .3);
  image(scissors, 360, 415, scissors.width * .3, scissors.height * .3);

  // circle follows the selected tool
  image(circle, circleX[toolIndex], circleY, circle.width * .3, circle.height * .3);

  // Claude Opus 5.5: draw the selected tool at the cursor
  PImage t = toolImgs[toolIndex];
  float cw = t.width * cursorScale, ch = t.height * cursorScale;
  image(t, toolX - cw/2, toolY - ch/2, cw, ch);

  // start screen
  if (state == WAITING) {
    image(startScreen, 40, 120, startScreen.width * .3, startScreen.height * .3);
  }
}


// ---------- SPRITE HELPERS ----------
// Claude Opus 5.5: cut one 128x128 frame out of a sheet by column and row
PImage getSprite(PImage sheet, int col, int row) {
  return sheet.get(col * frameW, row * frameH, frameW, frameH);
}

// Claude Opus 5.5: grab a run of frames, wrapping to the next row after 4 columns
PImage[] loadAnim(PImage sheet, int startRow, int count) {
  PImage[] frames = new PImage[count];
  for (int i = 0; i < count; i++) {
    frames[i] = getSprite(sheet, i % 4, startRow + i / 4);
  }
  return frames;
}


// sound
// loop bubbles/water/dryer while scrubbing, cut the snip when you stop
void updateSounds() {
  boolean scrubbing = frameCount - lastScrubFrame < 10;   // grace period so it doesn't stutter
  for (int i = 0; i < 3; i++) {
    boolean want = scrubbing && toolIndex == i;
    if (want && !toolSounds[i].isPlaying()) toolSounds[i].loop();
    if (!want && toolSounds[i].isPlaying()) toolSounds[i].pause();
  }
  boolean cutting = scrubbing && toolIndex == 3;
  if (!cutting && toolSounds[3].isPlaying()) toolSounds[3].stop();
}


// ---------- PET ----------
// Claude Opus 5.5: which station each tool sends the pet to
int stationForTool(int tool) {
  if (tool == 2) return 1;   // blow dryer -> rug
  if (tool == 3) return 2;   // scissors -> counter
  return 0;                  // sponge + shower -> tub
}

float stationX(int s) {
  return s == 0 ? tubX : s == 1 ? rugX : counterX;
}

float stationY(int s) {
  return s == 0 ? tubY : s == 1 ? rugY : counterY;
}

// Claude Opus 5.5: reset and walk the current pet in from the left
void startPet() {
  petSize = frameW * petScales[currentPet];
  petX = -petSize / 2;
  petY = floorY;
  station = 0;
  bubbles.clear();
  drops.clear();
  puffs.clear();
  furs.clear();
  state = ENTERING;
}

// Claude Opus 5.5: move on to the next pet in the list
void nextPet() {
  currentPet = (currentPet + 1) % sheet.length;
  startPet();
}

// Claude Opus 5.5: begin an arcing hop to (x, y), then switch to the next state
void startHop(float x, float y, int next) {
  hopStartX = petX;
  hopStartY = petY;
  hopEndX = x;
  hopEndY = y;
  hopT = 0;
  afterHop = next;
  state = HOPPING;
}

// Claude Opus 5.5: move the pet each frame based on its state
void updatePet() {
  if (state == ENTERING) {
    petX += 3;
    if (petX >= entryX) {
      petX = entryX;
      startHop(tubX, tubY, IDLE);              // hop into the tub
    }
  } else if (state == HOPPING) {
    hopT = min(hopT + 0.04, 1);                // bigger number = faster hop
    petX = lerp(hopStartX, hopEndX, hopT);
    petY = lerp(hopStartY, hopEndY, hopT) - sin(PI * hopT) * hopHeight;
    if (hopT >= 1) state = afterHop;
  } else if (state == IDLE) {
    int want = stationForTool(toolIndex);
    if (want != station) {
      station = want;
      startHop(stationX(want), stationY(want), IDLE);
    }
  } else if (state == LEAVING) {
    petX += 5;
    if (petX - petSize / 2 > width) nextPet();
  }
}

// Claude Opus 5.5: draw the idle animation when still, the run animation when moving
void drawPet() {
  if (state == WAITING) return;
  PImage[] anim = (state == IDLE) ? idleAnims[currentPet] : runAnims[currentPet];
  PImage frame = anim[(frameCount / 8) % anim.length];
  image(frame, petX - petSize / 2, petY - petSize, petSize, petSize);
}

// Claude Opus 5.5: is the tool touching the pet?
boolean overPet() {
  return toolX > petX - petSize / 2 && toolX < petX + petSize / 2 &&
    toolY > petY - petSize && toolY < petY;
}


// ---------- PARTICLES ----------
// Claude Opus 5.5: move, draw, and remove finished particles
void updateParticles() {
  for (int i = bubbles.size() - 1; i >= 0; i--) {
    Bubble b = bubbles.get(i);
    b.update();
    b.display();
    if (b.dead()) bubbles.remove(i);
  }
  for (int i = drops.size() - 1; i >= 0; i--) {
    Drop d = drops.get(i);
    d.update();
    d.display();
    if (d.dead()) drops.remove(i);
  }
  for (int i = puffs.size() - 1; i >= 0; i--) {
    Puff p = puffs.get(i);
    p.update();
    p.display();
    if (p.dead()) puffs.remove(i);
  }
  for (int i = furs.size() - 1; i >= 0; i--) {
    Fur f = furs.get(i);
    f.update();
    f.display();
    if (f.dead()) furs.remove(i);
  }
}

// Claude Opus 5.5: sponge bubbles that float up and wobble
class Bubble {
  float x, y, vx, vy, size, life = 255;

  Bubble(float x, float y) {
    this.x = x + random(-15, 15);
    this.y = y + random(-15, 15);
    vx = random(-0.5, 0.5);
    vy = random(-1.5, -0.3);
    size = random(6, 16);
  }

  void update() {
    x += vx + sin(frameCount * 0.1 + y) * 0.3;
    y += vy;
    life -= 2;
  }

  void display() {
    noFill();
    stroke(255, life);
    strokeWeight(1.5);
    ellipse(x, y, size, size);
    noStroke();
    fill(255, life * 0.8);
    ellipse(x - size * 0.2, y - size * 0.2, size * 0.25, size * 0.25);   // shine
  }

  boolean dead() {
    return life <= 0;
  }
}

// Claude Opus 5.5: shower drops that fall with gravity
class Drop {
  float x, y, vx, vy, life = 255;

  Drop(float x, float y) {
    this.x = x + random(-10, 10);
    this.y = y;
    vx = random(-1, 1);
    vy = random(2, 4);
  }

  void update() {
    vy += 0.3;
    x += vx;
    y += vy;
    life -= 4;
  }

  void display() {
    noStroke();
    fill(80, 160, 255, life);
    ellipse(x, y, 4, 7);
  }

  boolean dead() {
    return life <= 0 || y > height;
  }
}

// Claude Opus 5.5: blow dryer swooshes that drift sideways and fade
class Puff {
  float x, y, vx, vy, size, life = 200;

  Puff(float x, float y) {
    this.x = x;
    this.y = y + random(-10, 10);
    vx = random(1.5, 3) * (random(1) < 0.5 ? -1 : 1);   // blows left or right
    vy = random(-0.5, 0.5);
    size = random(10, 20);
  }

  void update() {
    x += vx;
    y += vy + sin(frameCount * 0.2 + x * 0.1) * 0.5;   // wavy
    size += 0.3;                                       // spreads out
    life -= 5;
  }

  void display() {
    noFill();
    stroke(255, life);
    strokeWeight(2);
    arc(x, y, size, size * 0.6, PI * 0.1, PI * 0.9);
    arc(x + 6, y - 4, size * 0.6, size * 0.4, PI * 0.1, PI * 0.9);
  }

  boolean dead() {
    return life <= 0;
  }
}

// Claude Opus 5.5: scissors fur tufts that spin and flutter down
class Fur {
  float x, y, vx, vy, angle, spin, life = 255;
  color c;

  Fur(float x, float y, color c) {
    this.x = x + random(-8, 8);
    this.y = y;
    this.c = c;
    vx = random(-0.8, 0.8);
    vy = random(0.5, 1.2);
    angle = random(TWO_PI);
    spin = random(-0.1, 0.1);
  }

  void update() {
    x += vx + sin(frameCount * 0.1 + y * 0.05) * 0.6;
    y += vy;
    angle += spin;
    life -= 2;
  }

  void display() {
    pushMatrix();
    translate(x, y);
    rotate(angle);
    stroke(red(c), green(c), blue(c), life);
    strokeWeight(2);
    noFill();
    line(-4, 0, 4, 0);
    line(-3, -2, 3, 1);
    line(-3, 2, 3, -1);
    popMatrix();
  }

  boolean dead() {
    return life <= 0 || y > height;
  }
}

// Claude Opus 5.5: pick a fur color from the current pet's sprite so clippings match the animal
color furColor() {
  PImage f = idleAnims[currentPet][0];
  for (int tries = 0; tries < 20; tries++) {
    color c = f.get(int(random(f.width * 0.3, f.width * 0.7)), int(random(f.height * 0.3, f.height * 0.7)));
    if (alpha(c) > 200 && brightness(c) > 60) return c;   // skip see-through and outline pixels
  }
  return color(240);   // fallback: off-white
}


// ---------- INPUT ----------
// Claude Opus 5.5: read "joyX,joyY,pot,button" from the board
void serialEvent(Serial p) {
  String line = p.readStringUntil('\n');
  if (line == null) return;
  String[] v = split(trim(line), ',');
  if (v.length < 4) return;

  float midX = 1870, midY = 1860;   // joystick's resting values

  float rawX = float(v[0]);
  float rawY = float(v[1]);

  // scale each side separately so full tilt = 1 in both directions
  float dx = rawX < midX ? (rawX - midX) / midX : (rawX - midX) / (analogMax - midX);
  float dy = rawY < midY ? (rawY - midY) / midY : (rawY - midY) / (analogMax - midY);
  joyDX = abs(dx) < deadzone ? 0 : dx;
  joyDY = abs(dy) < deadzone ? 0 : dy;

  // potentiometer picks the tool
  toolIndex = constrain(int(map(float(v[2]), 0, analogMax, 0, toolImgs.length)), 0, toolImgs.length - 1);

  // button only counts on a fresh press
  int btn = int(v[3]);
  if (btn == 1 && lastButton == 0) donePressed = true;
  lastButton = btn;
}

// Claude Opus 5.5: keyboard controls for testing without the board
void keyPressed() {
  if (key == ' ') donePressed = true;
  if (key >= '1' && key <= '4') toolIndex = key - '1';
}
