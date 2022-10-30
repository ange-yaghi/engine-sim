
#ifndef ATG_ENGINE_SIM_JOYSTICK_H
#define ATG_ENGINE_SIM_JOYSTICK_H

#include <Xinput.h>

class Joystick
{
public:
	XINPUT_STATE GetState();
	int GetIndex();
	bool Alive();

	void Update();
    void RefreshState();
	void Rumble(float a_fLM, float a_fRM);

	float LT();
	float RT();

    bool buttonPressed(int j_iButton);
    bool buttonDown(int j_iButton);
	Joystick();
	Joystick(int j_iIndex);

private:
	XINPUT_STATE j_State;
	int j_JoystickIndex;

    static const int button_count = 14;
    bool bStates_prev[button_count];
    bool bStates[button_count];

    bool b_ButtonsDown[button_count];
};



static const WORD XINPUT_Buttons[] = {
      XINPUT_GAMEPAD_A,
      XINPUT_GAMEPAD_B,
      XINPUT_GAMEPAD_X,
      XINPUT_GAMEPAD_Y,
      XINPUT_GAMEPAD_DPAD_UP,
      XINPUT_GAMEPAD_DPAD_DOWN,
      XINPUT_GAMEPAD_DPAD_LEFT,
      XINPUT_GAMEPAD_DPAD_RIGHT,
      XINPUT_GAMEPAD_LEFT_SHOULDER,
      XINPUT_GAMEPAD_RIGHT_SHOULDER,
      XINPUT_GAMEPAD_LEFT_THUMB,
      XINPUT_GAMEPAD_RIGHT_THUMB,
      XINPUT_GAMEPAD_START,
      XINPUT_GAMEPAD_BACK
};

struct button_id
{
    button_id();

    int A, B, X, Y;

    int d_up, d_down, d_left , d_right;

    int l_s, r_s;

    int l_th, r_th;

    int start;
    int back;
};
#endif /* ATG_ENGINE_SIM_JOYSTICK_H */

extern button_id j_Buttons;