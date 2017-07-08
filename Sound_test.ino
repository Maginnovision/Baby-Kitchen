#include <Audio.h>
#include <SD.h>
#include <Adafruit_TPA2016.h>
#include <Adafruit_GFX.h>
#include <Adafruit_IS31FL3731.h>

Adafruit_IS31FL3731 ledmatrix = Adafruit_IS31FL3731();

// The lookup table to make the brightness changes be more visible
uint8_t sweep[] = { 5, 10, 15, 20, 40, 55, 70, 85, 105, 125, 140, 215, 215, 140, 125, 105, 85, 70, 55, 40, 20, 15, 10, 5 };
#define volume 4
#define beep_volume -10

AudioPlaySdWav           playSdWav1;
AudioOutputAnalogStereo  dacs1;
AudioConnection          patchCord1(playSdWav1, 0, dacs1, 0);
AudioConnection          patchCord2(playSdWav1, 1, dacs1, 1);
Adafruit_TPA2016 audioamp = Adafruit_TPA2016();

elapsedMillis sinceOpen = 0;
elapsedMillis sinceClose = 30000;

unsigned dimmer = 0;
bool cooking = false;
bool thisRead = false, lastRead = false;

void setup() {
  AudioMemory(8);
  if (!SD.begin(BUILTIN_SDCARD) || !audioamp.begin() || !ledmatrix.begin()) {
	  Serial.println("Unable to start all objects!");
	  while (1);
  }
  audioamp.setAGCCompression(TPA2016_AGC_OFF);
  audioamp.setReleaseControl(0);
  audioamp.setGain(volume);

  pinMode(17, INPUT);
  thisRead = lastRead = digitalReadFast(17);
}

void loop() {
	microwave_switch();

	//Closed 25 seconds ago and we're cooking, ding
	if (sinceClose >= 25000 && sinceClose <= 27000 && cooking) {
		audioamp.setGain(beep_volume);
		playSdWav1.play("BEEP.WAV");
		delay(10);
		lights_off();
	}

	if (cooking) {
		for (uint8_t x = 0; x < 16; ++x)
			for (uint8_t y = 0; y < 9; ++y) ledmatrix.drawPixel(x, y, sweep[(x + y + dimmer) % 24]);
		dimmer = dimmer == 23 ? 0 : dimmer + 1;
	}
}

void microwave_switch() {
	thisRead = digitalReadFast(17); //Reads true with open door
	if (thisRead == lastRead) return;
	delay(10);

	if (thisRead) {
		if (cooking) {
			audioamp.setGain(beep_volume);
			playSdWav1.play("BEEP.WAV");
			delay(10);
			lights_off();
		}
		sinceOpen = 0;
		cooking = false;
	}
	else {
		if (sinceOpen >= 2500 && !cooking) {
			audioamp.setGain(volume);
			playSdWav1.play("MWAVE.WAV");
			delay(10);
			cooking = true;
		}
		sinceClose = 0;
	}
	lastRead = thisRead;
}

inline void lights_off() {
	cooking = false;
	dimmer = 0;
	for (size_t x = 0; x < 16; ++x)
		for (size_t y = 0; y < 9; ++y) ledmatrix.drawPixel(x, y, 0);
}