#include <windows.h>
#include "../include/joystick.h"

#pragma comment(lib, "Xinput.lib")

button_id j_Buttons;

Joystick::Joystick() {};

Joystick::Joystick(int j_iIndex) {
	j_JoystickIndex = j_iIndex - 1;

	for (int i = 0; i < button_count; i++)
	{
		bStates_prev[i] = false;
		bStates[i] = false;
		b_ButtonsDown[i] = false;
	}
}

XINPUT_STATE Joystick::GetState() {

	XINPUT_STATE js;

	memset(&js, 0, sizeof(XINPUT_STATE));

	XInputGetState(j_JoystickIndex, &js);

	return js;
}

int Joystick::GetIndex() { return j_JoystickIndex; }

bool Joystick::Alive() {

	memset(&j_State, 0, sizeof(XINPUT_STATE));

	DWORD r = XInputGetState(j_JoystickIndex, &j_State);

	if (r == 0)
		return true;
	else
		return false;
}

void Joystick::Update() { 
	j_State = GetState(); 

	for (int i = 0; i < button_count; i++)
	{
		// Set button state for current frame
		bStates[i] = (j_State.Gamepad.wButtons &
			XINPUT_Buttons[i]) == XINPUT_Buttons[i];

		// Set 'DOWN' state for current frame
		b_ButtonsDown[i] = !bStates_prev[i] &&
			bStates[i];
	}
}

void Joystick::RefreshState()
{
	memcpy(bStates_prev, bStates,
		sizeof(bStates_prev));
}

float Joystick::LT() {
	BYTE T = j_State.Gamepad.bLeftTrigger;

	if (T > XINPUT_GAMEPAD_TRIGGER_THRESHOLD) { 
		return T / 255.0f;
	}
	return 0.0;
}

float Joystick::RT() {
	BYTE T = j_State.Gamepad.bRightTrigger;

	if (T > XINPUT_GAMEPAD_TRIGGER_THRESHOLD) {
		return T / 255.0f;
	}
	return 0.0;
}

void Joystick::Rumble(float a_fLM, float a_fRM) {
	XINPUT_VIBRATION VS;

	memset(&VS, 0, sizeof(XINPUT_VIBRATION));

	int LM = int(a_fLM * 65535.0f);
	int RM = int(a_fRM * 65535.0f);

	VS.wLeftMotorSpeed = LM;
	VS.wRightMotorSpeed = RM;

	XInputSetState(j_JoystickIndex, &VS);
}

bool Joystick::buttonPressed(int j_iButton) {
	if (j_State.Gamepad.wButtons & XINPUT_Buttons[j_iButton]) {
		return true;
	}
	return false;
	
}

bool Joystick::buttonDown(int j_iButton)
{
	return b_ButtonsDown[j_iButton];
}

button_id::button_id() {
	A = 0;
	B = 1;
	X = 2;
	Y = 3;

	d_up = 4;
	d_down = 5;
	d_left = 6;
	d_right = 7;

	l_s = 8;
	r_s = 9;

	l_th = 10;
	r_th = 11;

	start = 12;
	back = 13;
}