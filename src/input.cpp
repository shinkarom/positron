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