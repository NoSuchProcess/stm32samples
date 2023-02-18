/*
 * This file is part of the flash project.
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

/*
 * struct to save user configurations
 */
typedef struct __attribute__((packed, aligned(8))){
    uint16_t userconf_sz;           // "magick number"
    uint16_t flagU16;
    uint32_t flagU32;
    char str[32];
} user_conf;

extern user_conf the_conf; // global user config (read from FLASH to RAM)
// data from ld-file: start address of storage

void flashstorage_init();
int store_userconf();
void dump_userconf();
int erase_storage();

