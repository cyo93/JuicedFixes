#include "stdafx.h"

#include "IniReader.h"
#include "injector/injector.hpp"

#include "xinput.h"

#define JOY_BTN_Y      0
#define JOY_BTN_X      1
#define JOY_BTN_A      2
#define JOY_BTN_B      3
#define JOY_DPAD_DOWN  4
#define JOY_DPAD_UP    5
#define JOY_DPAD_LEFT  6
#define JOY_DPAD_RIGHT 7
#define JOY_BTN_LS     8
#define JOY_BTN_RS     9
#define JOY_BTN_LT     10
#define JOY_BTN_RT     11
#define JOY_BTN_START  12
#define JOY_BTN_BACK   13

#define AXIS_LT       0x01000000
#define AXIS_RT       0x02000000
#define AXIS_LX       0x03000000
#define AXIS_LY       0x04000000
#define AXIS_RX       0x05000000
#define AXIS_RY       0x06000000

#define CHECK_TIMEOUT 1000 // Check controllers every n ms

bool pressed[14];


int JoyBtnToXInputBtn(int joyBtn)
{
	switch (joyBtn)
	{
	case JOY_BTN_A:
		return  XINPUT_GAMEPAD_A;
	case JOY_BTN_B:
		return XINPUT_GAMEPAD_B;
	case JOY_BTN_Y:
		return XINPUT_GAMEPAD_Y;
	case JOY_BTN_X:
		return XINPUT_GAMEPAD_X;
	case JOY_DPAD_DOWN:
		return XINPUT_GAMEPAD_DPAD_DOWN;
	case JOY_DPAD_LEFT:
		return XINPUT_GAMEPAD_DPAD_LEFT;
	case JOY_DPAD_RIGHT:
		return XINPUT_GAMEPAD_DPAD_RIGHT;
	case JOY_DPAD_UP:
		return XINPUT_GAMEPAD_DPAD_UP;
	case JOY_BTN_START:
		return XINPUT_GAMEPAD_START;
	case JOY_BTN_BACK:
		return XINPUT_GAMEPAD_BACK;
	case JOY_BTN_LS:
		return XINPUT_GAMEPAD_LEFT_SHOULDER;
	case JOY_BTN_RS:
		return XINPUT_GAMEPAD_RIGHT_SHOULDER;
	case JOY_BTN_LT:
		return XINPUT_GAMEPAD_LEFT_THUMB;
	case JOY_BTN_RT:
		return XINPUT_GAMEPAD_RIGHT_THUMB;
	}
	return 0;
}

enum Axles
{
	Steering = 0
};

enum Buttons
{
	/* Missing stuff:
	3 -- has no idea what it does
	9 -- shows drivers names?
	13 -- IDK

	*/
	Up = 4, // ALSO HORN
	Down = 6, // ALSO GEAR DOWN
	Left = 0,//	ALSO
	Right = 2, // ALSO CHANGE VIEW
	Accept = 8, //	--ACCEPT
	Back = 12, //	--BACK
	Tab = 11, //	--TAB(option)
	PageDown = 10, //	--PGDN(option)
	Pause = 1,
	LookBack = 5,
	GearUp = 7,
	Unspecified = 14,
	Unspecified2 = 15
};

XINPUT_STATE states[4];
bool         connected[4];
int          timeout = 0;

XINPUT_STATE check;

/*
*  Buttons check order
*  Deatils on the exact order are in joystick.h
*/
int checkOrder[] =
{
	XINPUT_GAMEPAD_Y,
	XINPUT_GAMEPAD_X,
	XINPUT_GAMEPAD_A,
	XINPUT_GAMEPAD_B,
	XINPUT_GAMEPAD_DPAD_DOWN,
	XINPUT_GAMEPAD_DPAD_UP,
	XINPUT_GAMEPAD_DPAD_LEFT,
	XINPUT_GAMEPAD_DPAD_RIGHT,
	XINPUT_GAMEPAD_LEFT_SHOULDER,
	XINPUT_GAMEPAD_RIGHT_SHOULDER,
	XINPUT_GAMEPAD_LEFT_THUMB,
	XINPUT_GAMEPAD_RIGHT_THUMB,
	XINPUT_GAMEPAD_START,
	XINPUT_GAMEPAD_BACK
};

float throttle = 0.0f;
float steering = 0.0f;

void CheckControllers(void)
{
	XINPUT_STATE state;
	for (int i = 0; i < 4; i++)
	{
		if (connected[i])
			continue;
		if (XInputGetState(i, &state) == ERROR_SUCCESS)
		{
			connected[i] = TRUE;
			states[i] = state;
		}
	}
}


void SignalHandler(int signal)
{
	printf("Signal %d", signal);
	throw "!Access Violation!";
}

void SetThrottle(float t)
{
	throttle = t;
}

void SetBrake(float t)
{
	try
	{
		unsigned int _a = injector::ReadMemory<unsigned int>(0x007746FC);
		if (_a < 0x0000FFFF || _a == 0xCCCCCCCC)
			return;
		injector::WriteMemory<float>(_a + 0x194, t);
	}
	catch (...)
	{

	}
}

int GetButtonState(int joyBtn, int joyIdx)
{
	bool actuallyPressed = states[joyIdx].Gamepad.wButtons & JoyBtnToXInputBtn(joyBtn);
	if (joyBtn > 3)
		return actuallyPressed;
	if (joyBtn == JOY_DPAD_DOWN && actuallyPressed)
		MessageBox(NULL, "IT WAS PRESSED YOU BITCH", "DPAD_DOWN", MB_ICONWARNING);

	if (pressed[joyBtn])
	{
		pressed[joyBtn] = actuallyPressed;
		return false;
	}
	if (actuallyPressed)
	{
		pressed[joyBtn] = true;
		return true;
	}
	pressed[joyBtn] = false;
	return false;

	switch (joyBtn)
	{
	case JOY_BTN_A:
		return states[joyIdx].Gamepad.wButtons & XINPUT_GAMEPAD_A & !(check.Gamepad.wButtons & XINPUT_GAMEPAD_A);
	case JOY_BTN_B:
		return states[joyIdx].Gamepad.wButtons & XINPUT_GAMEPAD_B;
	case JOY_BTN_Y:
		return states[joyIdx].Gamepad.wButtons & XINPUT_GAMEPAD_Y;
	case JOY_BTN_X:
		return states[joyIdx].Gamepad.wButtons & XINPUT_GAMEPAD_X;
	case JOY_DPAD_DOWN:
		return states[joyIdx].Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
	case JOY_DPAD_LEFT:
		return states[joyIdx].Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
	case JOY_DPAD_RIGHT:
		return states[joyIdx].Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;
	case JOY_DPAD_UP:
		return states[joyIdx].Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP;
	}
}

void RaiseEvents(void)
{
	XINPUT_STATE state;
	for (int i = 0; i < 4; i++)
	{
		if (!connected[i])
			continue;

		if (XInputGetState(i, &state) != ERROR_SUCCESS)
		{
			connected[i] = FALSE;
			continue;
		}
		/*for(int j = 0; j < sizeof(checkOrder) / sizeof(int); j++)
		{
		bool status = state.Gamepad.wButtons & checkOrder[j];
		bool was = states[i].Gamepad.wButtons & checkOrder[j];
		if(was == status)
		continue;
		if(!was)
		RaiseEvent(EVT_JOY_BTN_DOWN, i, j);
		else
		RaiseEvent(EVT_JOY_BTN_UP, i, j);
		}*/

		/*if(state.Gamepad.bLeftTrigger != states[i].Gamepad.bLeftTrigger)
		RaiseEvent(EVT_JOY_AXIS, i, AXIS_LT | (state.Gamepad.bLeftTrigger & ~0xFFFF0000));
		if(state.Gamepad.bRightTrigger != states[i].Gamepad.bRightTrigger)
		RaiseEvent(EVT_JOY_AXIS, i, AXIS_RT | (state.Gamepad.bRightTrigger& ~0xFFFF0000));
		if(state.Gamepad.sThumbLX != states[i].Gamepad.sThumbLX)
		RaiseEvent(EVT_JOY_AXIS, i, AXIS_LX | (state.Gamepad.sThumbLX& ~0xFFFF0000));
		if(state.Gamepad.sThumbRX != states[i].Gamepad.sThumbRX)
		RaiseEvent(EVT_JOY_AXIS, i, AXIS_RX | (state.Gamepad.sThumbRX & ~0xFFFF0000));
		if(state.Gamepad.sThumbLY != states[i].Gamepad.sThumbLY)
		RaiseEvent(EVT_JOY_AXIS, i, AXIS_LY | (state.Gamepad.sThumbLY& ~0xFFFF0000));
		if(state.Gamepad.sThumbRY != states[i].Gamepad.sThumbRY)
		RaiseEvent(EVT_JOY_AXIS, i, AXIS_RY | (state.Gamepad.sThumbRY& ~0xFFFF0000));*/

		float deadzone = 0.2;

		SetThrottle((float)state.Gamepad.bRightTrigger / 255.0);
		SetBrake((float)state.Gamepad.bLeftTrigger / 255.0);

		float normLX = fmaxf(-1, (float)state.Gamepad.sThumbLX / 32767);

		if (fabs(normLX) < deadzone)
			steering = 0.0f;
		else
			steering = (normLX > 0 ? (normLX - deadzone) : (normLX + deadzone)) * (1.0 / (1.0 - deadzone));

		/*
		unsigned int _a = injector::ReadMemory<unsigned int>(0x00727C20);
		if(!_a)
		return;
		unsigned int _b = injector::ReadMemory<unsigned int>(_a + 0x60);
		if(!_b)
		return;
		unsigned int _c = injector::ReadMemory<unsigned int>(_b + 0x3C4);
		if(_c < 0x0000FFFF)
		return;
		try
		{
		if(fabs(normLX) <= deadzone)
		injector::WriteMemory<float>(_c + 0x2C, 0);
		else
		injector::WriteMemory<float>(_c + 0x2C, ((normLX > 0 ? (normLX-deadzone) : (normLX+deadzone)) * (1.0/(1.0-deadzone))) * 1.0/3.0);
		}
		catch(...)
		{

		}*/

		/*if(state.Gamepad.sThumbLX != states[i].Gamepad.sThumbLX)
		RaiseEvent(EVT_JOY_AXIS, i, AXIS_LX | (state.Gamepad.sThumbLX& ~0xFFFF0000));*/

		states[i] = state;
	}
}

void joystick_init(void)
{
	CheckControllers();
}

void joystick_cycle(int passed)
{
	timeout += passed;
	if (timeout > CHECK_TIMEOUT)
	{
		timeout -= CHECK_TIMEOUT;
		CheckControllers();
	}
	RaiseEvents();
}

int NewInput()
{
	int result;
	float deadzone = 0.8;
	float normLX = fmaxf(-1, (float)states[0].Gamepad.sThumbLX / 32767);
	if (fabs(normLX) <= deadzone)
		result = 0;
	else
		result = (normLX > 0 ? 1 : -1);

	return result;
}

DWORD WINAPI Background(LPVOID unused)
{

	while (true)
	{
		joystick_cycle(1);
		Sleep(1);
	}
}

char rets[]
{
	0xC3,
	0xC3,
	0xC3,
	0xC3,
	0xC3,
	0xC3,
	0xC3,
	0xC3,
	0xC3,
	0xC3,
	0xC3,
	0xC3,
	0xC3,
	0xC3,
	0xC3,
	0xC3,
	0xC3,
	0xC3,
	0xC3,
	0xC3,
	0xC3,
};

int GetSteering()
{
	float result;
	float deadzone = 0.2;
	float normLX = fmaxf(-1, (float)states[0].Gamepad.sThumbLX / 32767);
	if (fabs(normLX) <= deadzone)
		result = 0;
	else
		result = (normLX > 0 ? (normLX - deadzone) : (normLX + deadzone)) * (1.0 / (1.0 - deadzone));
	return *(int*)(&result);
}

void FixCrashOnCalendar()
{
	char crashFix[] =
	{
		0xB0, 0x00, 0x90
	};
	injector::WriteMemoryRaw(0x004C14CF, crashFix, sizeof(crashFix), true); // Fix crash on calendar
}

void FixVirtualMemory()
{
	char memFix[] =
	{
		0xEB, 0x20
	};

	// Fix "Juiced requires virtual memory to be enabled"
	injector::MakeNOP(0x0059BE39, 2, true);
	injector::WriteMemoryRaw(0x0059BE40, memFix, sizeof(memFix), true);
}

extern "C" float __declspec(dllexport) __stdcall GetAxle()
{
	__asm push ecx
	int a, b;
	__asm mov a, edi
	__asm mov b, esi

	if (a == 1)
	{
		__asm pop ecx
		return throttle;
	}
	else if (a == 0)
	{
		__asm pop ecx
		return steering;
	}
	else
	{
		__asm pop ecx
		return 0.0f;
	}
}

extern "C" int __declspec(dllexport) __stdcall GetButton()
{
	int a, b;
	__asm mov a, eax
	__asm mov b, edi
	/*if (!b)
	{
	return 0;
	}*/
	switch (a)
	{
	case Buttons::Accept:
		return GetButtonState(JOY_BTN_A, 0);
	case Buttons::Back:
		return GetButtonState(JOY_BTN_B, 0);
	case Buttons::Tab:
		return GetButtonState(JOY_BTN_Y, 0);
	case Buttons::PageDown:
		return GetButtonState(JOY_BTN_X, 0);
	case Buttons::Down:
		return GetButtonState(JOY_DPAD_DOWN, 0);
	case Buttons::Up:
		return GetButtonState(JOY_DPAD_UP, 0);
	case Buttons::Left:
		return GetButtonState(JOY_DPAD_LEFT, 0);
	case Buttons::Right:
		return GetButtonState(JOY_DPAD_RIGHT, 0);
	case Buttons::Pause:
		return GetButtonState(JOY_BTN_START, 0);
	case Buttons::LookBack:
		return GetButtonState(JOY_BTN_RT, 0);
	case Buttons::Unspecified:
		return GetButtonState(JOY_BTN_LS, 0);
		return 0;
	case Buttons::Unspecified2:
		return GetButtonState(JOY_BTN_RS, 0);
	}
	return 0;
}

bool PatchInput = false;

void ReadConfig()
{
	CIniReader iniReader("fixes.ini");

	PatchInput = iniReader.ReadBoolean("Fixes", "PatchInput", false);
}

int WINAPI DllMain(HMODULE hInstance, DWORD reason, LPVOID lpReserved)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		for (int i = 0; i < 14; i++)
			pressed[i] = false;
		ReadConfig();
		uintptr_t base = (uintptr_t)GetModuleHandleA(NULL);
		IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)(base);
		IMAGE_NT_HEADERS* nt = (IMAGE_NT_HEADERS*)(base + dos->e_lfanew);

		char dis[] =
		{
			0x90, 0x90, 0x90
		};

		char pushEdi[] =
		{
			0x57
		};


		FixCrashOnCalendar();
		FixVirtualMemory();

		if (PatchInput)
		{
			injector::MakeJMP(0x00401640, GetAxle);
			injector::MakeJMP(0x004015D0, GetButton);
		}

		/*
		injector::MakeCALL(0x0041E2F4, GetAxle); // Throttle #1
		injector::MakeCALL(0x0041E311, GetAxle); // Throttle #2

		injector::MakeCALL(0x0041E3A4, GetAxle); // Steering

		injector::MakeCALL(0x0041E277, GetButton); // HORN
		*/
		/*
		injector::WriteMemoryRaw(0x00401641, rets, sizeof(rets), true);
		injector::WriteMemory<char>(0x00401640, 0x51, true); // push ecx
		injector::WriteMemoryRaw(0x00401641, pushEdi, 1, true); // push edi
		injector::MakeCALL(0x00401642, &NewInput, true);  // call NewInput
		injector::WriteMemory<char>(0x00401647, 0x59, true); // pop ecx*/






		//injector::MakeNOP(0x0041E626, 10, true);
		//injector::MakeCALL(0x0041E620, GetSteering, true);   // call GetSteering
		//injector::WriteMemory<char>(0x0041E625, 0x50, true); // push eax

		//injector::MakeCALL(0x0041E277, NewInput, true); // Puts "Horn" value somehow
		/*injector::MakeNOP(0x0041E37C, 6, true); // Disable KB brake override ?
		injector::MakeNOP(0x0041E38F, 10, true); // Disable brake reset
		injector::MakeNOP(0x0041E336, 10, true); // Disable throttle reset
		injector::MakeNOP(0x0041E323, 6, true); // Disable KB throttle override*/


		//injector::WriteMemoryRaw(0x0041E5F2, dis, sizeof(dis), true);

		// Copy-paste from MWExtraOptions
		/*if ((base + nt->OptionalHeader.AddressOfEntryPoint + (0x400000 - base)) == 0x7C4040) // Check if .exe file is compatible - Thanks to thelink2012 and MWisBest
		{
		Init();
		CreateThread(0, 0, Background, NULL, 0, NULL);
		}
		else
		{
		MessageBoxA(NULL, "This .exe is not supported.\nPlease use v1.3 English speed.exe (5,75 MB (6.029.312 bytes)).", "MWFixes", MB_ICONERROR);
		return FALSE;
		}*/
		//signal(SIGSEGV, SignalHandler);
		CreateThread(0, 0, Background, NULL, 0, NULL);
		MessageBoxA(NULL, "XInput support added!", "XI4J", MB_ICONINFORMATION);
	}
	return TRUE;
}