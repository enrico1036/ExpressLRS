#pragma once

#include <Arduino.h>
#include <stdio.h>
#include "../../src/targets.h"

#define TimerIntervalUSDefault 4000


class hwTimer
{
public:
	static volatile uint32_t HWtimerInterval;
	static volatile bool TickTock;
	static volatile int16_t PhaseShift;
	static bool ResetNextLoop;
	static volatile bool running;
	static uint32_t LastCallbackMicrosTick;
	static uint32_t LastCallbackMicrosTock;

	static void init();
	static void ICACHE_RAM_ATTR stop();
	static void ICACHE_RAM_ATTR resume();
	static void ICACHE_RAM_ATTR callback();
	static void ICACHE_RAM_ATTR updateInterval(uint32_t newTimerInterval);
	static void ICACHE_RAM_ATTR phaseShift(int32_t newPhaseShift);

	static void nullCallback(void);
	static void (*callbackTick)();
	static void (*callbackTock)();
};