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

// opening/closing time (ms)
#define SHUTTER_TIME    (20)
// waiting for getting status time (ms)
#define WAITING_TIME    (60)

typedef enum{
    SHUTTER_ERROR,          // shutter is absent?
    SHUTTER_RELAX,          // powered off
    SHUTTER_PROCESS,        // opened or closed
    SHUTTER_WAIT,           // wait in off state before turn to hi-z
    SHUTTER_STATE_AMOUNT
} shutter_state;

extern shutter_state shutterstate;

void process_shutter();
int open_shutter();
int close_shutter();
void print_shutter_state();
