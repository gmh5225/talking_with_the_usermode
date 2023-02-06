#pragma once
#include "KeInterface.h"
#include "offsets.hpp"

class Lumos {
public:
	const int ENT_LOOP_DISTANCE = 0x10;
	const float INITIAL_SENSIVITY = 1.0f;
	const float INCROSS_SENSIVITY = 0.19f;

	bool debug = false;

	Lumos(): Driver("\\\\.\\Protector"){
		std::cout << "Lumos init... :)\n";

		Driver.protectThis();
		process_identifier = Driver.GetTargetPid();
		clientdll = Driver.GetClientModule();

		if (debug == true) {
			std::cout << "Pid : " << process_identifier << std::endl;
			std::cout << "client.dll : " << clientdll << std::endl;
		}
	}

	ULONG getEntityInCh();
	ULONG getLocalPlayer();
	int getEntityTeam();
	int getCrosshairId();
	float getSensivity();
	int getLocalTeam();

	void setSensivity(float x);

	void lumos();
	void lumosRun();

private:
	KeInterface Driver;
	ULONG process_identifier;
	ULONG clientdll;
};