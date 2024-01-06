/*
 * This file is part of the canbus4bta project.
 * Copyright 2024 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include <stdint.h>
#include <string.h>

void hexdump(int (*sendfun)(const char *s), uint8_t *arr, uint16_t len);
char *u2str(uint32_t val);
char *i2str(int32_t i);
char *uhex2str(uint32_t val);
char *float2str(float x, uint8_t prec);
const char *getnum(const char *txt, uint32_t *N);
const char *omit_spaces(const char *buf);
const char *getint(const char *txt, int32_t *I);

