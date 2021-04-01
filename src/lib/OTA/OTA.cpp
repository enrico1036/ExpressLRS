/**
 * This file is part of ExpressLRS
 * See https://github.com/AlessandroAU/ExpressLRS
 *
 * This file provides utilities for packing and unpacking the data to
 * be sent over the radio link.
 */

#include "OTA.h"

#if defined HYBRID_SWITCHES_8 or defined UNIT_TEST

#if TARGET_TX or defined UNIT_TEST
/**
 * Hybrid switches packet encoding for sending over the air
 *
 * Analog channels are reduced to 10 bits to allow for switch encoding
 * Switch[0] is sent on every packet.
 * A 3 bit switch index and 2 bit value is used to send the remaining switches
 * in a round-robin fashion.
 * If any of the round-robin switches have changed
 * we take the lowest indexed one and send that, hence lower indexed switches have
 * higher priority in the event that several are changed at once.
 *
 * Inputs: crsf.ChannelDataIn, crsf.currentSwitches
 * Outputs: Radio.TXdataBuffer, side-effects the sentSwitch value
 */
#ifdef ENABLE_TELEMETRY
void ICACHE_RAM_ATTR GenerateChannelDataHybridSwitch8(volatile uint8_t* Buffer, CRSF *crsf, uint8_t addr, bool TelemetryStatus)
#else
void ICACHE_RAM_ATTR GenerateChannelDataHybridSwitch8(volatile uint8_t* Buffer, CRSF *crsf, uint8_t addr)
#endif
{
  uint8_t PacketHeaderAddr;
  PacketHeaderAddr = (addr << 2) | RC_DATA_PACKET;
  Buffer[0] = PacketHeaderAddr;
  Buffer[1] = ((crsf->ChannelDataIn[0]) >> 3);
  Buffer[2] = ((crsf->ChannelDataIn[1]) >> 3);
  Buffer[3] = ((crsf->ChannelDataIn[2]) >> 3);
  Buffer[4] = ((crsf->ChannelDataIn[3]) >> 3);
  Buffer[5] = ((crsf->ChannelDataIn[0] & 0b110) << 5) |
                           ((crsf->ChannelDataIn[1] & 0b110) << 3) |
                           ((crsf->ChannelDataIn[2] & 0b110) << 1) |
                           ((crsf->ChannelDataIn[3] & 0b110) >> 1);

  // switch 0 is sent on every packet - intended for low latency arm/disarm
  Buffer[6] = (crsf->currentSwitches[0] & 0b11) << 5;
#ifdef ENABLE_TELEMETRY
  Buffer[6] |= TelemetryStatus << 7;
#endif

  // find the next switch to send
  uint8_t nextSwitchIndex = crsf->getNextSwitchIndex() & 0b111;      // mask for paranoia
  uint8_t value = crsf->currentSwitches[nextSwitchIndex] & 0b11; // mask for paranoia

  // put the bits into buf[6]. nextSwitchIndex is in the range 1 through 7 so takes 3 bits
  // currentSwitches[nextSwitchIndex] is in the range 0 through 2, takes 2 bits.
  Buffer[6] |= (nextSwitchIndex << 2) | value;

  // update the sent value
  crsf->setSentSwitch(nextSwitchIndex, value);
}

#elif TARGET_RX or defined UNIT_TEST
/**
 * Hybrid switches decoding of over the air data
 *
 * Hybrid switches uses 10 bits for each analog channel,
 * 2 bits for the low latency switch[0]
 * 3 bits for the round-robin switch index and 2 bits for the value
 *
 * Input: Buffer
 * Output: crsf->PackedRCdataOut
 */
void ICACHE_RAM_ATTR UnpackChannelDataHybridSwitch8(volatile uint8_t* Buffer, CRSF *crsf)
{
    // The analog channels
    crsf->PackedRCdataOut.ch0 = (Buffer[1] << 3) | ((Buffer[5] & 0b11000000) >> 5);
    crsf->PackedRCdataOut.ch1 = (Buffer[2] << 3) | ((Buffer[5] & 0b00110000) >> 3);
    crsf->PackedRCdataOut.ch2 = (Buffer[3] << 3) | ((Buffer[5] & 0b00001100) >> 1);
    crsf->PackedRCdataOut.ch3 = (Buffer[4] << 3) | ((Buffer[5] & 0b00000011) << 1);

    // The low latency switch
    crsf->PackedRCdataOut.ch4 = SWITCH2b_to_CRSF((Buffer[6] & 0b01100000) >> 5);

    // The round-robin switch
    uint8_t switchIndex = (Buffer[6] & 0b11100) >> 2;
    uint16_t switchValue = SWITCH2b_to_CRSF(Buffer[6] & 0b11);

    switch (switchIndex) {
        case 0:   // we should never get index 0 here since that is the low latency switch
            Serial.println("BAD switchIndex 0");
            break;
        case 1:
            crsf->PackedRCdataOut.ch5 = switchValue;
            break;
        case 2:
            crsf->PackedRCdataOut.ch6 = switchValue;
            break;
        case 3:
            crsf->PackedRCdataOut.ch7 = switchValue;
            break;
        case 4:
            crsf->PackedRCdataOut.ch8 = switchValue;
            break;
        case 5:
            crsf->PackedRCdataOut.ch9 = switchValue;
            break;
        case 6:
            crsf->PackedRCdataOut.ch10 = switchValue;
            break;
        case 7:
            crsf->PackedRCdataOut.ch11 = switchValue;
            break;
    }
}

#endif
#endif // HYBRID_SWITCHES_8

#if !defined HYBRID_SWITCHES_8 or defined UNIT_TEST

#if TARGET_TX or defined UNIT_TEST

void ICACHE_RAM_ATTR GenerateChannelData10bit(volatile uint8_t* Buffer, CRSF *crsf, uint8_t addr)
{
  uint8_t PacketHeaderAddr;
  PacketHeaderAddr = (addr << 2) | RC_DATA_PACKET;
  Buffer[0] = PacketHeaderAddr;
  Buffer[1] = ((crsf->ChannelDataIn[0]) >> 3);
  Buffer[2] = ((crsf->ChannelDataIn[1]) >> 3);
  Buffer[3] = ((crsf->ChannelDataIn[2]) >> 3);
  Buffer[4] = ((crsf->ChannelDataIn[3]) >> 3);
  Buffer[5] = ((crsf->ChannelDataIn[0] & 0b110) << 5) |
                           ((crsf->ChannelDataIn[1] & 0b110) << 3) |
                           ((crsf->ChannelDataIn[2] & 0b110) << 1) |
                           ((crsf->ChannelDataIn[3] & 0b110) >> 1);
  Buffer[6] = CRSF_to_BIT(crsf->ChannelDataIn[4]) << 7;
  Buffer[6] |= CRSF_to_BIT(crsf->ChannelDataIn[5]) << 6;
  Buffer[6] |= CRSF_to_BIT(crsf->ChannelDataIn[6]) << 5;
  Buffer[6] |= CRSF_to_BIT(crsf->ChannelDataIn[7]) << 4;
  Buffer[6] |= CRSF_to_BIT(crsf->ChannelDataIn[8]) << 3;
  Buffer[6] |= CRSF_to_BIT(crsf->ChannelDataIn[9]) << 2;
  Buffer[6] |= CRSF_to_BIT(crsf->ChannelDataIn[10]) << 1;
  Buffer[6] |= CRSF_to_BIT(crsf->ChannelDataIn[11]) << 0;
}

#elif TARGET_RX or defined UNIT_TEST

void ICACHE_RAM_ATTR UnpackChannelData10bit(volatile uint8_t* Buffer, CRSF *crsf)
{
    crsf->PackedRCdataOut.ch0 = (Buffer[1] << 3) | ((Buffer[5] & 0b11000000) >> 5);
    crsf->PackedRCdataOut.ch1 = (Buffer[2] << 3) | ((Buffer[5] & 0b00110000) >> 3);
    crsf->PackedRCdataOut.ch2 = (Buffer[3] << 3) | ((Buffer[5] & 0b00001100) >> 1);
    crsf->PackedRCdataOut.ch3 = (Buffer[4] << 3) | ((Buffer[5] & 0b00000011) << 1);
    crsf->PackedRCdataOut.ch4 = BIT_to_CRSF(Buffer[6] & 0b10000000);
    crsf->PackedRCdataOut.ch5 = BIT_to_CRSF(Buffer[6] & 0b01000000);
    crsf->PackedRCdataOut.ch6 = BIT_to_CRSF(Buffer[6] & 0b00100000);
    crsf->PackedRCdataOut.ch7 = BIT_to_CRSF(Buffer[6] & 0b00010000);
    crsf->PackedRCdataOut.ch8 = BIT_to_CRSF(Buffer[6] & 0b00001000);
    crsf->PackedRCdataOut.ch9 = BIT_to_CRSF(Buffer[6] & 0b00000100);
    crsf->PackedRCdataOut.ch10 = BIT_to_CRSF(Buffer[6] & 0b00000010);
    crsf->PackedRCdataOut.ch11 = BIT_to_CRSF(Buffer[6] & 0b00000001);
}

#endif

#endif // !HYBRID_SWITCHES_8

void ICACHE_RAM_ATTR GenerateMSPData(volatile uint8_t* Buffer, mspPacket_t *msp, uint8_t addr)
{
  uint8_t PacketHeaderAddr;
  PacketHeaderAddr = (addr << 2) | MSP_DATA_PACKET;
  Buffer[0] = PacketHeaderAddr;
  Buffer[1] = msp->function;
  Buffer[2] = msp->payloadSize;
  Buffer[3] = 0;
  Buffer[4] = 0;
  Buffer[5] = 0;
  Buffer[6] = 0;
  if (msp->payloadSize <= 4)
  {
    msp->payloadReadIterator = 0;
    for (int i = 0; i < msp->payloadSize; i++)
    {
      Buffer[3 + i] = msp->readByte();
    }
  }
  else
  {
    Serial.println("Unable to send MSP command. Packet too long.");
  }
}

void ICACHE_RAM_ATTR UnpackMSPData(volatile uint8_t* Buffer, mspPacket_t *msp)
{
    msp->reset();
    msp->makeCommand();
    msp->flags = 0;
    msp->function = Buffer[1];
    msp->addByte(Buffer[3]);
    msp->addByte(Buffer[4]);
    msp->addByte(Buffer[5]);
    msp->addByte(Buffer[6]);
}

