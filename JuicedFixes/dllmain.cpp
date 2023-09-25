#include <bit>

#include <IniReader.h>
#include <injector/injector.hpp>

#include "input.h"
#include "controller.h"

#define CHECK_TIMEOUT 1000 // Check controllers every n ms

bool pressed[14];

// Patches
bool PatchVirtualMemory = false;
bool PatchCalendarCrash = false;
bool PatchInput = false;

int MenuCodes[16];
int RaceCodes[20];

Controllers controllers;

int controlType = 1;

DWORD WINAPI Background(LPVOID unused)
{
	while (true)
	{
		controllers.Tick(1);
		Sleep(1);
	}
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

extern "C" void /*__declspec(dllexport)*/ __cdecl SetControlType(char* fmt, int i)
{
	__asm push eax
	controlType = i;
	__asm pop eax
}

extern "C" float __declspec(dllexport) __stdcall GetAxle()
{
	__asm push ecx
	int a, b;
	__asm mov a, edi
	__asm mov b, esi
	__asm pop ecx
	if (controlType == ControlType::Menu)
	{
		switch (a)
		{
		case AxlesMenu::LookDown:
		case AxlesMenu::LookUp:
			return controllers.getValue(0, ControllerButtons::RightThumbY);
		case AxlesMenu::LookLeft:
		case AxlesMenu::LookRight:
			return controllers.getValue(0, ControllerButtons::RightThumbX);
		case 0:
			return controllers.getValue(0, ControllerButtons::LeftThumbX);
		case 1:
			return controllers.getValue(0, ControllerButtons::RightTrigger);
		}
	}
	else
	{
		switch (a)
		{
		case AxlesRace::Steering:
			return controllers.getValue(0, ControllerButtons::LeftThumbX);
		case AxlesRace::Throttle:
			return controllers.getValue(0, ControllerButtons::RightTrigger);
		case AxlesRace::Brake:
			return controllers.getValue(0, ControllerButtons::LeftTrigger);
		case AxlesRace::Reverse:
			return controllers.getValue(0, ControllerButtons::Y);
		case AxlesRace::Handbrake:
			return controllers.getValue(0, ControllerButtons::X);
		case AxlesRace::LookAround:
			return controllers.getValue(0, ControllerButtons::RightThumbX);
		case AxlesRace::LookAroundY:
			return controllers.getValue(0, ControllerButtons::RightThumbY);
		}
	}
	return 0.0f;
}

void RaceEnd()
{
	SetControlType(nullptr, ControlType::Menu);
}

void RaceStart()
{
	SetControlType(nullptr, ControlType::Race);
}

__declspec(noinline) int ProcessMenuInput(int key)
{
	if (key >= ControllerButtons::End || key < 0)
		return 0;
	switch (key)
	{
	case MenuButtons::Down:
	case MenuButtons::DownDigital:
		return controllers.getPressed(0, ControllerButtons::DPadDown);
	case MenuButtons::Up:
	case MenuButtons::UpDigital:
		return controllers.getPressed(0, ControllerButtons::DPadUp);
	case MenuButtons::Left:
	case MenuButtons::LeftDigital:
		return controllers.getPressed(0, ControllerButtons::DPadLeft);
	case MenuButtons::Right:
	case MenuButtons::RightDigital:
		return controllers.getPressed(0, ControllerButtons::DPadRight);
	default:
		return controllers.getSinglePress(0, static_cast<ControllerButtons>(MenuCodes[key]));
	}
	return 0;
}

__declspec(noinline) int ProcessRaceInput(int key)
{
	switch (key)
	{
	case RaceButtons::Horn:
	case RaceButtons::LookBack:
	case RaceButtons::Nitro:
		return std::bit_cast<int>(controllers.getValue(0, static_cast<ControllerButtons>(RaceCodes[key])));
	default:
		return controllers.getPressed(0, static_cast<ControllerButtons>(RaceCodes[key]));
	}
	return 0;
}

extern "C" int __declspec(dllexport) __stdcall GetButton()
{
	int a, b;
	__asm mov a, eax
	__asm mov b, edi
	return controlType == ControlType::Race ? ProcessRaceInput(a) : ProcessMenuInput(a);
}

void ReadConfig()
{
	CIniReader iniReader("fixes.ini");

	PatchInput = iniReader.ReadBoolean("Fixes", "PatchInput", false);
	PatchVirtualMemory = iniReader.ReadBoolean("Fixes", "PatchVirtualMemory", false);
	PatchCalendarCrash = iniReader.ReadBoolean("Fixes", "PatchCalendarCrash", false);

	MenuCodes[0] = iniReader.ReadInteger("MenuControls", "Left", JOY_DPAD_LEFT);
	//MenuCodes[1] = iniReader.ReadInteger("MenuControls", "LeftDifital", JOY_DPAD_LEFT);
	MenuCodes[2] = iniReader.ReadInteger("MenuControls", "Right", JOY_DPAD_RIGHT);
	//MenuCodes[3] = iniReader.ReadInteger("MenuControls", "RightDigital", JOY_DPAD_LEFT);
	MenuCodes[4] = iniReader.ReadInteger("MenuControls", "Up", JOY_DPAD_UP);
	//MenuCodes[5] = iniReader.ReadInteger("MenuControls", "UpDigital", JOY_DPAD_UP);
	MenuCodes[6] = iniReader.ReadInteger("MenuControls", "Down", JOY_DPAD_DOWN);
	//MenuCodes[7] = iniReader.ReadInteger("MenuControls", "DownDigital", JOY_DPAD_DOWN);
	MenuCodes[8] = iniReader.ReadInteger("MenuControls", "Accept", JOY_BTN_A);
	MenuCodes[9] = iniReader.ReadInteger("MenuControls", "Menu9", 110);
	MenuCodes[10] = iniReader.ReadInteger("MenuControls", "PageDown", JOY_BTN_Y);
	MenuCodes[11] = iniReader.ReadInteger("MenuControls", "Tab", JOY_BTN_X);
	MenuCodes[12] = iniReader.ReadInteger("MenuControls", "Back", JOY_BTN_B);
	MenuCodes[13] = iniReader.ReadInteger("MenuControls", "Back2", 110);
	MenuCodes[14] = iniReader.ReadInteger("MenuControls", "Menu14", 110);
	MenuCodes[15] = iniReader.ReadInteger("MenuControls", "Menu15", 110);

	RaceCodes[0] = iniReader.ReadInteger("RaceControls", "Race0", 110);
	RaceCodes[1] = iniReader.ReadInteger("RaceControls", "Pause", JOY_BTN_START);
	RaceCodes[2] = iniReader.ReadInteger("RaceControls", "ChangeView", JOY_BTN_BACK);
	RaceCodes[3] = iniReader.ReadInteger("RaceControls", "Nitro", JOY_BTN_X);
	RaceCodes[4] = iniReader.ReadInteger("RaceControls", "Horn", JOY_DPAD_UP);
	RaceCodes[5] = iniReader.ReadInteger("RaceControls", "LookBack", JOY_DPAD_DOWN);
	RaceCodes[6] = iniReader.ReadInteger("RaceControls", "GearDown", JOY_BTN_B);
	RaceCodes[7] = iniReader.ReadInteger("RaceControls", "GearUp", JOY_BTN_A);
	RaceCodes[8] = iniReader.ReadInteger("RaceControls", "Skip", 110);
	RaceCodes[9] = iniReader.ReadInteger("RaceControls", "Race9", 110);
	RaceCodes[10] = iniReader.ReadInteger("RaceControls", "Race10", 110);
	RaceCodes[11] = iniReader.ReadInteger("RaceControls", "Race11", 110);
	RaceCodes[12] = iniReader.ReadInteger("RaceControls", "Race12", 110);
	RaceCodes[13] = iniReader.ReadInteger("RaceControls", "Race13", 110);
	RaceCodes[14] = iniReader.ReadInteger("RaceControls", "Race14", 110);
	RaceCodes[15] = iniReader.ReadInteger("RaceControls", "Race15", 110);
	RaceCodes[16] = iniReader.ReadInteger("RaceControls", "Handbrake", 110);
	RaceCodes[17] = iniReader.ReadInteger("RaceControls", "Reverse", 110);
	RaceCodes[18] = iniReader.ReadInteger("RaceControls", "Axle2", 110);
	RaceCodes[19] = iniReader.ReadInteger("RaceControls", "Axle3", 110);
}

int WINAPI DllMain(HMODULE hInstance, DWORD reason, LPVOID lpReserved)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		// Clear controller mapping
		memset(MenuCodes, 110, sizeof(MenuCodes));
		memset(RaceCodes, 110, sizeof(RaceCodes));

		for (int i = 0; i < 14; i++)
			pressed[i] = false;
		ReadConfig();

		controlType = ControlType::Menu;
		if(PatchCalendarCrash) FixCrashOnCalendar();
		if(PatchVirtualMemory) FixVirtualMemory();

		if (PatchInput)
		{
			injector::MakeJMP(0x00401640, GetAxle);
			injector::MakeJMP(0x004015D0, GetButton);
			injector::MakeCALL(0x0046175E, RaceStart);	// Switch control type for race
			injector::MakeCALL(0x00450B3B, RaceEnd);	// Switch control type for race end
			injector::MakeCALL(0x005BB0FE, SetControlType); // Switch control type for pause menu
		}

		CreateThread(0, 0, Background, NULL, 0, NULL);
	}
	return TRUE;
}
