#include "posi.h"

#include <iostream>

bool pressedButtons[numInputButtons];
bool lastPressedButtons[numInputButtons];
bool justPressedButtons[numInputButtons];
bool justReleasedButtons[numInputButtons];

void posiUpdateButton(int buttonNumber, bool state){
	if(buttonNumber < 0 || buttonNumber >= numInputButtons) {
		return;
	}
	lastPressedButtons[buttonNumber] = pressedButtons[buttonNumber];
	pressedButtons[buttonNumber] = state;
	justPressedButtons[buttonNumber] = pressedButtons[buttonNumber] && !lastPressedButtons[buttonNumber];
	justReleasedButtons[buttonNumber] = !pressedButtons[buttonNumber] && lastPressedButtons[buttonNumber];
}

bool API_isPressed(int buttonNumber) {
	if(buttonNumber < 0 || buttonNumber >= numInputButtons) {
		return false;
	}
	return pressedButtons[buttonNumber];
}

bool API_isJustPressed(int buttonNumber) {
	if(buttonNumber < 0 || buttonNumber >= numInputButtons) {
		return false;
	}
	return justPressedButtons[buttonNumber];
}

bool API_isJustReleased(int buttonNumber) {
	if(buttonNumber < 0 || buttonNumber >= numInputButtons) {
		return false;
	}
	return justReleasedButtons[buttonNumber];
}