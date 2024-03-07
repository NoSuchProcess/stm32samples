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

#include "encoder.h"
#include "flash.h"
#include "spi.h"
#include "usart.h"


void encoder_setup(){
    if(FLAG(ENC_IS_SSI)) spi_setup(ENCODER_SPI);
    else usart_setup();
}

// read encoder value into buffer `outbuf`
// return TRUE if all OK
int read_encoder(uint8_t outbuf[4]){
    if(!FLAG(ENC_IS_SSI)) return FALSE;
    *((uint32_t*)outbuf) = 0;
    return spi_writeread(ENCODER_SPI, outbuf, 4);
}