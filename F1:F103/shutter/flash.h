/*
 * This file is part of the shutter project.
 * Copyright 2023 Edward V. Emelianov <edward.emelianoff@gmail.com>.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <stm32f1.h>

#define FLASH_SIZE_REG      ((uint32_t)0x1FFFF7E0)
#define FLASH_SIZE          *((uint16_t*)FLASH_SIZE_REG)

/*
 * struct to save user configurations
 */
typedef struct __attribute__((packed, aligned(4))){
    uint16_t userconf_sz;       // "magick number"
    uint8_t ccdactive : 1;      // ccd active (open shutter at): 0 - low, 1 - high
    uint8_t hallactive : 1;     // hall sensor active (shutter is opened when): 0 - low, 1 - high
    uint16_t minvoltage;        // minimal voltage on C (*100)
    uint16_t workvoltage;       // working voltage (*100)
    uint16_t shutterrime;       // opening/closing time (ms)
    uint16_t waitingtime;       // time to wait for mechanical work done (ms)
    uint16_t shtrVmul;          // multiplier of shutter voltage calculation
    uint16_t shtrVdiv;          // divider -//-
} user_conf;

extern user_conf the_conf;

void flashstorage_init();
int store_userconf();
int erase_storage(int npage);
void dump_userconf();
