/*
 * Copyright 2026 Radio Sound, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This file is a typed Android-native representation of the A2B register
 * routines used by Salted Caramel Vanilla. The register sequence is aligned
 * with the latest fetched main branch of:
 *
 *   mr-data/assets/adi_a2b_generated/adi_a2b_i2c_commandlist.h
 *   mr-data/assets/adi_a2b_generated/adi_a2b_i2c_commandlist_1_node.h
 *
 * The source headers are Analog Devices SigmaStudio-generated artifacts. They
 * are not included here; this table keeps only the Android execution model
 * and the register values needed by the Radio Sound product.
 */

#pragma once

#include <cstddef>
#include <cstdint>

namespace aidl::android::hardware::audio::core::a2b {

enum class A2bOperationType : uint8_t {
    Write,
    Read,
    Delay,
};

struct A2bOperation {
    A2bOperationType type;
    uint8_t deviceAddress;
    uint8_t registerAddress;
    uint8_t value;
};

struct A2bRoutine {
    const char* name;
    const A2bOperation* operations;
    size_t operationCount;
};

#define A2B_INIT_ROUTINE_MR_DATA_MAIN_2NODE_TDM4 1
#define A2B_INIT_ROUTINE_MR_DATA_MAIN_1NODE 2

#ifndef A2B_INIT_ROUTINE
#define A2B_INIT_ROUTINE A2B_INIT_ROUTINE_MR_DATA_MAIN_2NODE_TDM4
#endif

inline constexpr A2bOperation kMrDataMain2NodeTdm4[] = {
    {A2bOperationType::Write, 0x68, 0x12, 0x84}, // CONTROL
    {A2bOperationType::Delay, 0x00, 0x00, 0x19}, // A2B_Delay
    {A2bOperationType::Read, 0x68, 0x17, 0x00}, // INTTYPE
    {A2bOperationType::Write, 0x68, 0x1B, 0x77}, // INTMSK0
    {A2bOperationType::Write, 0x68, 0x1C, 0x78}, // INTMSK1
    {A2bOperationType::Write, 0x68, 0x1D, 0x0F}, // INTMSK2
    {A2bOperationType::Write, 0x68, 0x0F, 0x7E}, // RESPCYCS
    {A2bOperationType::Write, 0x68, 0x12, 0x01}, // CONTROL
    {A2bOperationType::Write, 0x68, 0x41, 0xE0}, // I2SGCFG — TDM2, 32-bit (Pi I2S native)
    {A2bOperationType::Write, 0x68, 0x09, 0x01}, // SWCTL
    {A2bOperationType::Write, 0x68, 0x13, 0x7E}, // DISCVRY
    {A2bOperationType::Delay, 0x00, 0x00, 0x32}, // A2B_Delay
    {A2bOperationType::Read, 0x68, 0x1A, 0x00}, // INTPND2
    {A2bOperationType::Write, 0x68, 0x1A, 0x01}, // INTPND2
    {A2bOperationType::Write, 0x68, 0x01, 0x00}, // NODEADR
    {A2bOperationType::Read, 0x69, 0x02, 0x00}, // VENDOR
    {A2bOperationType::Read, 0x69, 0x03, 0x00}, // PRODUCT
    {A2bOperationType::Read, 0x69, 0x04, 0x00}, // VERSION
    {A2bOperationType::Write, 0x68, 0x09, 0x21}, // SWCTL
    {A2bOperationType::Write, 0x68, 0x01, 0x00}, // NODEADR
    {A2bOperationType::Write, 0x69, 0x12, 0x00}, // CONTROL
    {A2bOperationType::Write, 0x69, 0x09, 0x01}, // SWCTL
    {A2bOperationType::Write, 0x68, 0x13, 0x7A}, // DISCVRY
    {A2bOperationType::Delay, 0x00, 0x00, 0x32}, // A2B_Delay
    {A2bOperationType::Read, 0x68, 0x1A, 0x00}, // INTPND2
    {A2bOperationType::Write, 0x68, 0x1A, 0x01}, // INTPND2
    {A2bOperationType::Write, 0x68, 0x01, 0x01}, // NODEADR
    {A2bOperationType::Read, 0x69, 0x02, 0x00}, // VENDOR
    {A2bOperationType::Read, 0x69, 0x03, 0x00}, // PRODUCT
    {A2bOperationType::Read, 0x69, 0x04, 0x00}, // VERSION
    {A2bOperationType::Write, 0x68, 0x01, 0x00}, // NODEADR
    {A2bOperationType::Write, 0x69, 0x09, 0x21}, // SWCTL
    {A2bOperationType::Write, 0x68, 0x01, 0x01}, // NODEADR
    {A2bOperationType::Write, 0x69, 0x0A, 0x00}, // BCDNSLOTS
    {A2bOperationType::Write, 0x69, 0x0B, 0x80}, // LDNSLOTS
    {A2bOperationType::Write, 0x69, 0x0C, 0x00}, // LUPSLOTS
    {A2bOperationType::Write, 0x69, 0x3F, 0x01}, // I2CCFG
    {A2bOperationType::Write, 0x69, 0x46, 0x00}, // SYNCOFFSET
    {A2bOperationType::Write, 0x69, 0x41, 0xE1}, // I2SGCFG — TDM4, 32-bit slots (TDMSS=0)
    {A2bOperationType::Write, 0x69, 0x42, 0x89}, // I2SCFG
    {A2bOperationType::Write, 0x69, 0x43, 0x00}, // I2SRATE
    {A2bOperationType::Write, 0x69, 0x47, 0x18}, // PDMCTL
    {A2bOperationType::Write, 0x69, 0x5D, 0x00}, // PDMCTL2
    {A2bOperationType::Write, 0x69, 0x48, 0x00}, // ERRMGMT
    {A2bOperationType::Write, 0x69, 0x4A, 0x00}, // GPIODAT
    {A2bOperationType::Write, 0x69, 0x4D, 0x20}, // GPIOOEN — IO5 output only
    {A2bOperationType::Write, 0x69, 0x4E, 0x00}, // GPIOIEN
    {A2bOperationType::Write, 0x69, 0x50, 0x00}, // PINTEN
    {A2bOperationType::Write, 0x69, 0x51, 0x00}, // PINTINV
    {A2bOperationType::Write, 0x69, 0x52, 0x00}, // PINCFG
    {A2bOperationType::Write, 0x69, 0x20, 0x00}, // TESTMODE
    {A2bOperationType::Write, 0x69, 0x59, 0x00}, // CLK1CFG
    {A2bOperationType::Write, 0x69, 0x5A, 0xC1}, // CLK2CFG
    {A2bOperationType::Write, 0x69, 0x60, 0x00}, // UPMASK0
    {A2bOperationType::Write, 0x69, 0x61, 0x00}, // UPMASK1
    {A2bOperationType::Write, 0x69, 0x62, 0x00}, // UPMASK2
    {A2bOperationType::Write, 0x69, 0x63, 0x00}, // UPMASK3
    {A2bOperationType::Write, 0x69, 0x64, 0x00}, // UPOFFSET
    {A2bOperationType::Write, 0x69, 0x65, 0x03}, // DNMASK0
    {A2bOperationType::Write, 0x69, 0x66, 0x00}, // DNMASK1
    {A2bOperationType::Write, 0x69, 0x67, 0x00}, // DNMASK2
    {A2bOperationType::Write, 0x69, 0x68, 0x00}, // DNMASK3
    {A2bOperationType::Write, 0x69, 0x69, 0x00}, // DNOFFSET
    {A2bOperationType::Write, 0x69, 0x81, 0x00}, // GPIOD0MSK
    {A2bOperationType::Write, 0x69, 0x82, 0x00}, // GPIOD1MSK
    {A2bOperationType::Write, 0x69, 0x83, 0x00}, // GPIOD2MSK
    {A2bOperationType::Write, 0x69, 0x84, 0x00}, // GPIOD3MSK
    {A2bOperationType::Write, 0x69, 0x85, 0x00}, // GPIOD4MSK
    {A2bOperationType::Write, 0x69, 0x86, 0x00}, // GPIOD5MSK
    {A2bOperationType::Write, 0x69, 0x87, 0x00}, // GPIOD6MSK
    {A2bOperationType::Write, 0x69, 0x88, 0x00}, // GPIOD7MSK
    {A2bOperationType::Write, 0x69, 0x8A, 0x00}, // GPIODINV
    {A2bOperationType::Write, 0x69, 0x80, 0x00}, // GPIODEN
    {A2bOperationType::Write, 0x69, 0x90, 0x00}, // MBOX0CTL
    {A2bOperationType::Write, 0x69, 0x96, 0x00}, // MBOX1CTL
    {A2bOperationType::Write, 0x69, 0x5C, 0x00}, // SUSCFG
    {A2bOperationType::Write, 0x69, 0x58, 0x00}, // I2SRRSOFFS
    {A2bOperationType::Write, 0x69, 0x57, 0x00}, // I2SRRCTL
    {A2bOperationType::Write, 0x69, 0x2E, 0x00}, // TXACTL
    {A2bOperationType::Write, 0x69, 0x30, 0x00}, // TXBCTL
    {A2bOperationType::Write, 0x69, 0x1B, 0x77}, // INTMSK0
    {A2bOperationType::Write, 0x69, 0x1C, 0x7F}, // INTMSK1
    {A2bOperationType::Write, 0x69, 0x1E, 0xEF}, // BECCTL
    {A2bOperationType::Write, 0x68, 0x01, 0x00}, // NODEADR
    {A2bOperationType::Write, 0x69, 0x0A, 0x00}, // BCDNSLOTS
    {A2bOperationType::Write, 0x69, 0x0B, 0x80}, // LDNSLOTS
    {A2bOperationType::Write, 0x69, 0x0C, 0x00}, // LUPSLOTS
    {A2bOperationType::Write, 0x69, 0x3F, 0x01}, // I2CCFG
    {A2bOperationType::Write, 0x69, 0x46, 0x00}, // SYNCOFFSET
    {A2bOperationType::Write, 0x69, 0x41, 0xE1}, // I2SGCFG — TDM4, 32-bit slots (TDMSS=0)
    {A2bOperationType::Write, 0x69, 0x42, 0x89}, // I2SCFG
    {A2bOperationType::Write, 0x69, 0x43, 0x00}, // I2SRATE
    {A2bOperationType::Write, 0x69, 0x47, 0x18}, // PDMCTL
    {A2bOperationType::Write, 0x69, 0x5D, 0x00}, // PDMCTL2
    {A2bOperationType::Write, 0x69, 0x48, 0x00}, // ERRMGMT
    {A2bOperationType::Write, 0x69, 0x4A, 0x00}, // GPIODAT
    {A2bOperationType::Write, 0x69, 0x4D, 0x20}, // GPIOOEN — IO5 output only
    {A2bOperationType::Write, 0x69, 0x4E, 0x00}, // GPIOIEN
    {A2bOperationType::Write, 0x69, 0x50, 0x00}, // PINTEN
    {A2bOperationType::Write, 0x69, 0x51, 0x00}, // PINTINV
    {A2bOperationType::Write, 0x69, 0x52, 0x00}, // PINCFG
    {A2bOperationType::Write, 0x69, 0x20, 0x00}, // TESTMODE
    {A2bOperationType::Write, 0x69, 0x59, 0x00}, // CLK1CFG
    {A2bOperationType::Write, 0x69, 0x5A, 0xC1}, // CLK2CFG
    {A2bOperationType::Write, 0x69, 0x60, 0x00}, // UPMASK0
    {A2bOperationType::Write, 0x69, 0x61, 0x00}, // UPMASK1
    {A2bOperationType::Write, 0x69, 0x62, 0x00}, // UPMASK2
    {A2bOperationType::Write, 0x69, 0x63, 0x00}, // UPMASK3
    {A2bOperationType::Write, 0x69, 0x64, 0x00}, // UPOFFSET
    {A2bOperationType::Write, 0x69, 0x65, 0x03}, // DNMASK0
    {A2bOperationType::Write, 0x69, 0x66, 0x00}, // DNMASK1
    {A2bOperationType::Write, 0x69, 0x67, 0x00}, // DNMASK2
    {A2bOperationType::Write, 0x69, 0x68, 0x00}, // DNMASK3
    {A2bOperationType::Write, 0x69, 0x69, 0x00}, // DNOFFSET
    {A2bOperationType::Write, 0x69, 0x81, 0x00}, // GPIOD0MSK
    {A2bOperationType::Write, 0x69, 0x82, 0x00}, // GPIOD1MSK
    {A2bOperationType::Write, 0x69, 0x83, 0x00}, // GPIOD2MSK
    {A2bOperationType::Write, 0x69, 0x84, 0x00}, // GPIOD3MSK
    {A2bOperationType::Write, 0x69, 0x85, 0x00}, // GPIOD4MSK
    {A2bOperationType::Write, 0x69, 0x86, 0x00}, // GPIOD5MSK
    {A2bOperationType::Write, 0x69, 0x87, 0x00}, // GPIOD6MSK
    {A2bOperationType::Write, 0x69, 0x88, 0x00}, // GPIOD7MSK
    {A2bOperationType::Write, 0x69, 0x8A, 0x00}, // GPIODINV
    {A2bOperationType::Write, 0x69, 0x80, 0x00}, // GPIODEN
    {A2bOperationType::Write, 0x69, 0x90, 0x00}, // MBOX0CTL
    {A2bOperationType::Write, 0x69, 0x96, 0x00}, // MBOX1CTL
    {A2bOperationType::Write, 0x69, 0x5C, 0x00}, // SUSCFG
    {A2bOperationType::Write, 0x69, 0x58, 0x00}, // I2SRRSOFFS
    {A2bOperationType::Write, 0x69, 0x57, 0x00}, // I2SRRCTL
    {A2bOperationType::Write, 0x69, 0x2E, 0x00}, // TXACTL
    {A2bOperationType::Write, 0x69, 0x30, 0x00}, // TXBCTL
    {A2bOperationType::Write, 0x69, 0x1B, 0x77}, // INTMSK0
    {A2bOperationType::Write, 0x69, 0x1C, 0x7F}, // INTMSK1
    {A2bOperationType::Write, 0x69, 0x1E, 0xEF}, // BECCTL
    {A2bOperationType::Write, 0x68, 0x3F, 0x00}, // I2CCFG
    {A2bOperationType::Write, 0x68, 0x42, 0x18}, // I2SCFG
    {A2bOperationType::Write, 0x68, 0x44, 0x00}, // I2STXOFFSET
    {A2bOperationType::Write, 0x68, 0x45, 0x00}, // I2SRXOFFSET
    {A2bOperationType::Write, 0x68, 0x47, 0x00}, // PDMCTL
    {A2bOperationType::Write, 0x68, 0x5D, 0x00}, // PDMCTL2
    {A2bOperationType::Write, 0x68, 0x48, 0x00}, // ERRMGMT
    {A2bOperationType::Write, 0x68, 0x4A, 0x00}, // GPIODAT
    {A2bOperationType::Write, 0x68, 0x4D, 0x00}, // GPIOOEN
    {A2bOperationType::Write, 0x68, 0x4E, 0x00}, // GPIOIEN
    {A2bOperationType::Write, 0x68, 0x50, 0x00}, // PINTEN
    {A2bOperationType::Write, 0x68, 0x51, 0x00}, // PINTINV
    {A2bOperationType::Write, 0x68, 0x52, 0x00}, // PINCFG
    {A2bOperationType::Write, 0x68, 0x20, 0x00}, // TESTMODE
    {A2bOperationType::Write, 0x68, 0x59, 0x00}, // CLK1CFG
    {A2bOperationType::Write, 0x68, 0x5A, 0x00}, // CLK2CFG
    {A2bOperationType::Write, 0x68, 0x81, 0x00}, // GPIOD0MSK
    {A2bOperationType::Write, 0x68, 0x82, 0x00}, // GPIOD1MSK
    {A2bOperationType::Write, 0x68, 0x83, 0x00}, // GPIOD2MSK
    {A2bOperationType::Write, 0x68, 0x84, 0x00}, // GPIOD3MSK
    {A2bOperationType::Write, 0x68, 0x85, 0x00}, // GPIOD4MSK
    {A2bOperationType::Write, 0x68, 0x86, 0x00}, // GPIOD5MSK
    {A2bOperationType::Write, 0x68, 0x87, 0x00}, // GPIOD6MSK
    {A2bOperationType::Write, 0x68, 0x88, 0x00}, // GPIOD7MSK
    {A2bOperationType::Write, 0x68, 0x8A, 0x00}, // GPIODINV
    {A2bOperationType::Write, 0x68, 0x80, 0x00}, // GPIODEN
    {A2bOperationType::Write, 0x68, 0x57, 0x00}, // I2SRRCTL
    {A2bOperationType::Write, 0x68, 0x2E, 0x00}, // TXACTL
    {A2bOperationType::Write, 0x68, 0x30, 0x00}, // TXBCTL
    {A2bOperationType::Write, 0x68, 0x1E, 0xEF}, // BECCTL
    {A2bOperationType::Write, 0x68, 0x01, 0x00}, // NODEADR
    {A2bOperationType::Write, 0x69, 0x0D, 0x02}, // DNSLOTS
    {A2bOperationType::Write, 0x69, 0x0E, 0x00}, // UPSLOTS
    {A2bOperationType::Write, 0x68, 0x0D, 0x02}, // DNSLOTS
    {A2bOperationType::Write, 0x68, 0x0E, 0x00}, // UPSLOTS
    {A2bOperationType::Write, 0x68, 0x01, 0x00}, // NODEADR
    {A2bOperationType::Write, 0x69, 0x09, 0x01}, // SWCTL
    {A2bOperationType::Write, 0x68, 0x09, 0x01}, // SWCTL
    {A2bOperationType::Write, 0x68, 0x40, 0x00}, // PLLCTL
    {A2bOperationType::Write, 0x68, 0x01, 0x80}, // NODEADR
    {A2bOperationType::Write, 0x69, 0x40, 0x00}, // PLLCTL
    {A2bOperationType::Write, 0x68, 0x01, 0x00}, // NODEADR
    {A2bOperationType::Write, 0x68, 0x10, 0x66}, // SLOTFMT — 32-bit DN/UP slots for TDM 32-bit
    {A2bOperationType::Write, 0x68, 0x11, 0x03}, // DATCTL
    {A2bOperationType::Write, 0x68, 0x56, 0x00}, // I2SRRATE
    {A2bOperationType::Write, 0x68, 0x12, 0x01}, // CONTROL
    {A2bOperationType::Write, 0x68, 0x01, 0x81}, // NODEADR: broadcast to nodes 0-1
    {A2bOperationType::Write, 0x69, 0x4A, 0x00}, // GPIODAT: IO5 LOW (clean baseline)
    {A2bOperationType::Write, 0x69, 0x4A, 0x20}, // GPIODAT: IO5 HIGH (ETM trigger) — stays HIGH
};

inline constexpr A2bOperation kMrDataMain1Node[] = {
    {A2bOperationType::Write, 0x68, 0x12, 0x84}, // CONTROL
    {A2bOperationType::Delay, 0x00, 0x00, 0x19}, // A2B_Delay
    {A2bOperationType::Read, 0x68, 0x17, 0x00}, // INTTYPE
    {A2bOperationType::Write, 0x68, 0x1B, 0x77}, // INTMSK0
    {A2bOperationType::Write, 0x68, 0x1C, 0x78}, // INTMSK1
    {A2bOperationType::Write, 0x68, 0x1D, 0x0F}, // INTMSK2
    {A2bOperationType::Write, 0x68, 0x0F, 0x80}, // RESPCYCS — tuned timing (ported from main 6d44fc3)
    {A2bOperationType::Write, 0x68, 0x12, 0x01}, // CONTROL
    {A2bOperationType::Write, 0x68, 0x41, 0xE0}, // I2SGCFG — TDM2, 32-bit (Pi I2S native)
    {A2bOperationType::Write, 0x68, 0x09, 0x01}, // SWCTL
    {A2bOperationType::Write, 0x68, 0x13, 0x80}, // DISCVRY — tuned timing (ported from main 6d44fc3)
    {A2bOperationType::Delay, 0x00, 0x00, 0x32}, // A2B_Delay
    {A2bOperationType::Read, 0x68, 0x1A, 0x00}, // INTPND2
    {A2bOperationType::Write, 0x68, 0x1A, 0x01}, // INTPND2
    {A2bOperationType::Write, 0x68, 0x01, 0x00}, // NODEADR
    {A2bOperationType::Read, 0x69, 0x02, 0x00}, // VENDOR
    {A2bOperationType::Read, 0x69, 0x03, 0x00}, // PRODUCT
    {A2bOperationType::Read, 0x69, 0x04, 0x00}, // VERSION
    {A2bOperationType::Write, 0x68, 0x09, 0x21}, // SWCTL
    {A2bOperationType::Write, 0x68, 0x01, 0x00}, // NODEADR
    {A2bOperationType::Write, 0x69, 0x0A, 0x00}, // BCDNSLOTS
    {A2bOperationType::Write, 0x69, 0x0B, 0x80}, // LDNSLOTS
    {A2bOperationType::Write, 0x69, 0x0C, 0x00}, // LUPSLOTS
    {A2bOperationType::Write, 0x69, 0x3F, 0x01}, // I2CCFG
    {A2bOperationType::Write, 0x69, 0x46, 0x00}, // SYNCOFFSET
    {A2bOperationType::Write, 0x69, 0x41, 0xE1}, // I2SGCFG — TDM4, 32-bit slots (TDMSS=0)
    {A2bOperationType::Write, 0x69, 0x42, 0x89}, // I2SCFG
    {A2bOperationType::Write, 0x69, 0x43, 0x00}, // I2SRATE
    {A2bOperationType::Write, 0x69, 0x47, 0x18}, // PDMCTL
    {A2bOperationType::Write, 0x69, 0x5D, 0x00}, // PDMCTL2
    {A2bOperationType::Write, 0x69, 0x48, 0x00}, // ERRMGMT
    {A2bOperationType::Write, 0x69, 0x4A, 0x00}, // GPIODAT
    {A2bOperationType::Write, 0x69, 0x4D, 0x20}, // GPIOOEN — IO5 output only
    {A2bOperationType::Write, 0x69, 0x4E, 0x00}, // GPIOIEN
    {A2bOperationType::Write, 0x69, 0x50, 0x00}, // PINTEN
    {A2bOperationType::Write, 0x69, 0x51, 0x00}, // PINTINV
    {A2bOperationType::Write, 0x69, 0x52, 0x00}, // PINCFG
    {A2bOperationType::Write, 0x69, 0x20, 0x00}, // TESTMODE
    {A2bOperationType::Write, 0x69, 0x59, 0x00}, // CLK1CFG
    {A2bOperationType::Write, 0x69, 0x5A, 0xC1}, // CLK2CFG
    {A2bOperationType::Write, 0x69, 0x60, 0x00}, // UPMASK0
    {A2bOperationType::Write, 0x69, 0x61, 0x00}, // UPMASK1
    {A2bOperationType::Write, 0x69, 0x62, 0x00}, // UPMASK2
    {A2bOperationType::Write, 0x69, 0x63, 0x00}, // UPMASK3
    {A2bOperationType::Write, 0x69, 0x64, 0x00}, // UPOFFSET
    {A2bOperationType::Write, 0x69, 0x65, 0x03}, // DNMASK0
    {A2bOperationType::Write, 0x69, 0x66, 0x00}, // DNMASK1
    {A2bOperationType::Write, 0x69, 0x67, 0x00}, // DNMASK2
    {A2bOperationType::Write, 0x69, 0x68, 0x00}, // DNMASK3
    {A2bOperationType::Write, 0x69, 0x69, 0x00}, // DNOFFSET
    {A2bOperationType::Write, 0x69, 0x81, 0x00}, // GPIOD0MSK
    {A2bOperationType::Write, 0x69, 0x82, 0x00}, // GPIOD1MSK
    {A2bOperationType::Write, 0x69, 0x83, 0x00}, // GPIOD2MSK
    {A2bOperationType::Write, 0x69, 0x84, 0x00}, // GPIOD3MSK
    {A2bOperationType::Write, 0x69, 0x85, 0x00}, // GPIOD4MSK
    {A2bOperationType::Write, 0x69, 0x86, 0x00}, // GPIOD5MSK
    {A2bOperationType::Write, 0x69, 0x87, 0x00}, // GPIOD6MSK
    {A2bOperationType::Write, 0x69, 0x88, 0x00}, // GPIOD7MSK
    {A2bOperationType::Write, 0x69, 0x8A, 0x00}, // GPIODINV
    {A2bOperationType::Write, 0x69, 0x80, 0x00}, // GPIODEN
    {A2bOperationType::Write, 0x69, 0x90, 0x00}, // MBOX0CTL
    {A2bOperationType::Write, 0x69, 0x96, 0x00}, // MBOX1CTL
    {A2bOperationType::Write, 0x69, 0x5C, 0x00}, // SUSCFG
    {A2bOperationType::Write, 0x69, 0x58, 0x00}, // I2SRRSOFFS
    {A2bOperationType::Write, 0x69, 0x57, 0x00}, // I2SRRCTL
    {A2bOperationType::Write, 0x69, 0x2E, 0x00}, // TXACTL
    {A2bOperationType::Write, 0x69, 0x30, 0x00}, // TXBCTL
    {A2bOperationType::Write, 0x69, 0x1B, 0x77}, // INTMSK0
    {A2bOperationType::Write, 0x69, 0x1C, 0x7F}, // INTMSK1
    {A2bOperationType::Write, 0x69, 0x1E, 0xEF}, // BECCTL
    {A2bOperationType::Write, 0x68, 0x3F, 0x00}, // I2CCFG
    {A2bOperationType::Write, 0x68, 0x42, 0x18}, // I2SCFG
    {A2bOperationType::Write, 0x68, 0x44, 0x00}, // I2STXOFFSET
    {A2bOperationType::Write, 0x68, 0x45, 0x00}, // I2SRXOFFSET
    {A2bOperationType::Write, 0x68, 0x47, 0x00}, // PDMCTL
    {A2bOperationType::Write, 0x68, 0x5D, 0x00}, // PDMCTL2
    {A2bOperationType::Write, 0x68, 0x48, 0x00}, // ERRMGMT
    {A2bOperationType::Write, 0x68, 0x4A, 0x00}, // GPIODAT
    {A2bOperationType::Write, 0x68, 0x4D, 0x00}, // GPIOOEN
    {A2bOperationType::Write, 0x68, 0x4E, 0x00}, // GPIOIEN
    {A2bOperationType::Write, 0x68, 0x50, 0x00}, // PINTEN
    {A2bOperationType::Write, 0x68, 0x51, 0x00}, // PINTINV
    {A2bOperationType::Write, 0x68, 0x52, 0x00}, // PINCFG
    {A2bOperationType::Write, 0x68, 0x20, 0x00}, // TESTMODE
    {A2bOperationType::Write, 0x68, 0x59, 0x00}, // CLK1CFG
    {A2bOperationType::Write, 0x68, 0x5A, 0x00}, // CLK2CFG
    {A2bOperationType::Write, 0x68, 0x81, 0x00}, // GPIOD0MSK
    {A2bOperationType::Write, 0x68, 0x82, 0x00}, // GPIOD1MSK
    {A2bOperationType::Write, 0x68, 0x83, 0x00}, // GPIOD2MSK
    {A2bOperationType::Write, 0x68, 0x84, 0x00}, // GPIOD3MSK
    {A2bOperationType::Write, 0x68, 0x85, 0x00}, // GPIOD4MSK
    {A2bOperationType::Write, 0x68, 0x86, 0x00}, // GPIOD5MSK
    {A2bOperationType::Write, 0x68, 0x87, 0x00}, // GPIOD6MSK
    {A2bOperationType::Write, 0x68, 0x88, 0x00}, // GPIOD7MSK
    {A2bOperationType::Write, 0x68, 0x8A, 0x00}, // GPIODINV
    {A2bOperationType::Write, 0x68, 0x80, 0x00}, // GPIODEN
    {A2bOperationType::Write, 0x68, 0x57, 0x00}, // I2SRRCTL
    {A2bOperationType::Write, 0x68, 0x2E, 0x00}, // TXACTL
    {A2bOperationType::Write, 0x68, 0x30, 0x00}, // TXBCTL
    {A2bOperationType::Write, 0x68, 0x1E, 0xEF}, // BECCTL
    {A2bOperationType::Write, 0x68, 0x01, 0x00}, // NODEADR
    {A2bOperationType::Write, 0x69, 0x0D, 0x02}, // DNSLOTS
    {A2bOperationType::Write, 0x69, 0x0E, 0x00}, // UPSLOTS
    {A2bOperationType::Write, 0x68, 0x0D, 0x02}, // DNSLOTS
    {A2bOperationType::Write, 0x68, 0x0E, 0x00}, // UPSLOTS
    {A2bOperationType::Write, 0x68, 0x01, 0x00}, // NODEADR
    {A2bOperationType::Write, 0x69, 0x09, 0x01}, // SWCTL
    {A2bOperationType::Write, 0x68, 0x09, 0x01}, // SWCTL
    {A2bOperationType::Write, 0x68, 0x40, 0x00}, // PLLCTL
    {A2bOperationType::Write, 0x68, 0x01, 0x80}, // NODEADR
    {A2bOperationType::Write, 0x69, 0x40, 0x00}, // PLLCTL
    {A2bOperationType::Write, 0x68, 0x01, 0x00}, // NODEADR
    {A2bOperationType::Write, 0x68, 0x10, 0x66}, // SLOTFMT — 32-bit DN/UP slots for TDM 32-bit
    {A2bOperationType::Write, 0x68, 0x11, 0x03}, // DATCTL
    {A2bOperationType::Write, 0x68, 0x56, 0x00}, // I2SRRATE
    {A2bOperationType::Write, 0x68, 0x12, 0x01}, // CONTROL
    {A2bOperationType::Write, 0x68, 0x01, 0x00}, // NODEADR: node 0
    {A2bOperationType::Write, 0x69, 0x4A, 0x00}, // GPIODAT: IO5 LOW (clean baseline)
    {A2bOperationType::Write, 0x69, 0x4A, 0x20}, // GPIODAT: IO5 HIGH (ETM trigger) — stays HIGH
};

inline constexpr A2bRoutine getSelectedRoutine() {
#if A2B_INIT_ROUTINE == A2B_INIT_ROUTINE_MR_DATA_MAIN_2NODE_TDM4
    return {
        "mr-data-main-2node-tdm4",
        kMrDataMain2NodeTdm4,
        sizeof(kMrDataMain2NodeTdm4) / sizeof(kMrDataMain2NodeTdm4[0]),
    };
#elif A2B_INIT_ROUTINE == A2B_INIT_ROUTINE_MR_DATA_MAIN_1NODE
    return {
        "mr-data-main-1node",
        kMrDataMain1Node,
        sizeof(kMrDataMain1Node) / sizeof(kMrDataMain1Node[0]),
    };
#else
#error "Unsupported A2B_INIT_ROUTINE"
#endif
}

}  // namespace aidl::android::hardware::audio::core::a2b
