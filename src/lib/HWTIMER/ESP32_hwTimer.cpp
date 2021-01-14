
#ifdef PLATFORM_ESP32
#include "ESP32_hwTimer.h"

static hw_timer_t *timer = NULL;
static portMUX_TYPE isrMutex = portMUX_INITIALIZER_UNLOCKED;

void hwTimer::nullCallback(void) {}
void (*hwTimer::callbackTick)() = &hwTimer::nullCallback;
void (*hwTimer::callbackTock)() = &hwTimer::nullCallback;

volatile uint32_t hwTimer::HWtimerInterval = TimerIntervalUSDefault;
volatile bool hwTimer::TickTock = true;
volatile int16_t hwTimer::PhaseShift = 0;
bool hwTimer::ResetNextLoop = false;
volatile bool hwTimer::running = false;

uint32_t hwTimer::LastCallbackMicrosTick = 0;
uint32_t hwTimer::LastCallbackMicrosTock = 0;

// void ICACHE_RAM_ATTR hwTimer::callback(void)
// {
//     portENTER_CRITICAL_ISR(&isrMutex);
//     if (running)
//     {
//         callbackTock();
//     }
//     portEXIT_CRITICAL_ISR(&isrMutex);
// }

void ICACHE_RAM_ATTR hwTimer::callback()
{
	portENTER_CRITICAL_ISR(&isrMutex);

	if (running)
	{
		if (hwTimer::TickTock)
		{
			if (hwTimer::ResetNextLoop)
			{

				timerAlarmWrite(timer, (hwTimer::HWtimerInterval >> 1) + hwTimer::PhaseShift, true);
				hwTimer::ResetNextLoop = false;
			}

			if (hwTimer::PhaseShift > 0 || hwTimer::PhaseShift < 0)
			{
				timerAlarmWrite(timer, (hwTimer::HWtimerInterval >> 1) + hwTimer::PhaseShift, true);
				hwTimer::ResetNextLoop = true;
				hwTimer::PhaseShift = 0;
			}

			hwTimer::LastCallbackMicrosTick = micros();
			hwTimer::callbackTick();
		}
		else
		{
			hwTimer::LastCallbackMicrosTock = micros();
			hwTimer::callbackTock();
		}
		hwTimer::TickTock = !hwTimer::TickTock;
	}

	portEXIT_CRITICAL_ISR(&isrMutex);
}

void ICACHE_RAM_ATTR hwTimer::init()
{
	if (!timer)
	{
		timer = timerBegin(0, (APB_CLK_FREQ / 1000000), true); // us timer
		timerAttachInterrupt(timer, &callback, true);
		timerAlarmWrite(timer, hwTimer::HWtimerInterval >> 1, true);
		timerAlarmEnable(timer);
		Serial.println("hwTimer Init");
		ResetNextLoop = false;
		TickTock = true;
		running = true;
	}
}

void ICACHE_RAM_ATTR hwTimer::resume()
{
	if (timer)
	{
		running = true;
		timerAlarmWrite(timer, hwTimer::HWtimerInterval >> 1, true);
		timerAlarmEnable(timer);
		Serial.println("hwTimer resume");
	}
}

void ICACHE_RAM_ATTR hwTimer::stop()
{
	if (timer)
	{
		running = false;
		timerAlarmDisable(timer);
		Serial.println("hwTimer stop");
	}
}

void ICACHE_RAM_ATTR hwTimer::updateInterval(uint32_t time)
{
	HWtimerInterval = time;
	if (timer)
	{
		Serial.print("hwTimer interval: ");
		Serial.println(time);
		timerAlarmWrite(timer, hwTimer::HWtimerInterval >> 1, true);
	}
}

void ICACHE_RAM_ATTR hwTimer::phaseShift(int32_t newPhaseShift)
{
	int32_t MaxPhaseShift = hwTimer::HWtimerInterval >> 1;

	if (newPhaseShift > MaxPhaseShift)
	{
		hwTimer::PhaseShift = MaxPhaseShift;
	}
	else if (newPhaseShift < -MaxPhaseShift)
	{
		hwTimer::PhaseShift = -MaxPhaseShift;
	}
	else
	{
		hwTimer::PhaseShift = newPhaseShift;
	}

	hwTimer::PhaseShift = hwTimer::PhaseShift * 5;
}

#endif
