#include "lumos.h"

float Lumos::getSensivity() {
	ULONG thisPtr = (int)(clientdll + hazedumper::signatures::dwSensitivityPtr);
	ULONG sensPtr = thisPtr + 44;

	auto cvar_xor = Driver.ReadVirtualMemory<std::int32_t>(process_identifier, sensPtr, sizeof(std::int32_t)) ^ thisPtr;

	//ULONG sensivity = Driver.ReadVirtualMemory<ULONG>(process_identifier, clientdll + hazedumper::signatures::dwSensitivity, sizeof(ULONG));

	//sensivity ^= thisPtr;

	auto sens = *reinterpret_cast<float*>(&cvar_xor);
	//printf("sens: %f, xored: %i\n", sens, cvar_xor);

	return sens;
}

ULONG Lumos::getLocalPlayer() {
	return Driver.ReadVirtualMemory<ULONG>(process_identifier, clientdll + hazedumper::signatures::dwLocalPlayer, sizeof(ULONG));
}

ULONG Lumos::getEntityInCh() {
	return Driver.ReadVirtualMemory<ULONG>(process_identifier, (clientdll + hazedumper::signatures::dwEntityList + (getCrosshairId() - 1) * ENT_LOOP_DISTANCE), sizeof(ULONG));
}

int Lumos::getEntityTeam() {
	return Driver.ReadVirtualMemory<int>(process_identifier, getEntityInCh() + hazedumper::netvars::m_iTeamNum, sizeof(int));
}

int Lumos::getCrosshairId() {
	return Driver.ReadVirtualMemory<int>(process_identifier, getLocalPlayer() + hazedumper::netvars::m_iCrosshairId, sizeof(int));
}

int Lumos::getLocalTeam() {
	return Driver.ReadVirtualMemory<int>(process_identifier, getLocalPlayer() + hazedumper::netvars::m_iTeamNum, sizeof(int));
}

void Lumos::setSensivity(float x) {
	ULONG thisPtr = (int)(clientdll + hazedumper::signatures::dwSensitivityPtr);
	ULONG sensPtr = thisPtr + 44;

	ULONG new_sens_xor = *reinterpret_cast<std::int32_t*>(&x) ^ thisPtr;
	//printf("new sens: %f, xored: %i\n\n", x, new_sens_xor);

	Driver.WriteVirtualMemory(process_identifier, sensPtr, new_sens_xor, sizeof(std::int32_t));
}

void Lumos::lumos() {
	static int count = 0;

	auto crosshair_id = getCrosshairId();

	auto enemy = getEntityInCh();

	auto entity_team = Driver.ReadVirtualMemory<int>(process_identifier, enemy + hazedumper::netvars::m_iTeamNum , sizeof(int));

	auto sens = getSensivity();

	if (count % 20 == 0 && debug == true) {
		std::cout << "Sens : " << sens << std::endl;
		std::cout << "local Team : " << getLocalTeam() << std::endl;
		std::cout << "Enemy Team : " << entity_team << std::endl;
		std::cout << "Cross Id : " << crosshair_id << std::endl;
	}

	if (crosshair_id > 0 && crosshair_id <= 64) {
		if (getLocalTeam() != entity_team) {
			setSensivity(INCROSS_SENSIVITY);
		}
		else
			setSensivity(INITIAL_SENSIVITY);
	}
	else
		setSensivity(INITIAL_SENSIVITY);

	count++;
}

void Lumos::lumosRun() {
	for (;;) {
		lumos();

		if (GetAsyncKeyState(VK_F3)) {
			std::cout << "Lumos destruct...\n";
			break;
		}

		Sleep(15);
	}
}