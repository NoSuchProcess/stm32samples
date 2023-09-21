/*
 * This file is part of the shutter project.
 * Copyright 2022 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include "adc.h"
#include "hardware.h"
#include "proto.h"
#include "shutter.h"
#include "usb.h"

char *omit_spaces(const char *buf){
    while(*buf){
        if(*buf > ' ') break;
        ++buf;
    }
    return (char*)buf;
}

// In case of overflow return `buf` and N==0xffffffff
// read decimal number & return pointer to next non-number symbol
static char *getdec(const char *buf, uint32_t *N){
    char *start = (char*)buf;
    uint32_t num = 0;
    while(*buf){
        char c = *buf;
        if(c < '0' || c > '9'){
            break;
        }
        if(num > 429496729 || (num == 429496729 && c > '5')){ // overflow
            *N = 0xffffff;
            return start;
        }
        num *= 10;
        num += c - '0';
        ++buf;
    }
    *N = num;
    return (char*)buf;
}
// read hexadecimal number (without 0x prefix!)
static char *gethex(const char *buf, uint32_t *N){
    char *start = (char*)buf;
    uint32_t num = 0;
    while(*buf){
        char c = *buf;
        uint8_t M = 0;
        if(c >= '0' && c <= '9'){
            M = '0';
        }else if(c >= 'A' && c <= 'F'){
            M = 'A' - 10;
        }else if(c >= 'a' && c <= 'f'){
            M = 'a' - 10;
        }
        if(M){
            if(num & 0xf0000000){ // overflow
                *N = 0xffffff;
                return start;
            }
            num <<= 4;
            num += c - M;
        }else{
            break;
        }
        ++buf;
    }
    *N = num;
    return (char*)buf;
}
// read octal number (without 0 prefix!)
static char *getoct(const char *buf, uint32_t *N){
    char *start = (char*)buf;
    uint32_t num = 0;
    while(*buf){
        char c = *buf;
        if(c < '0' || c > '7'){
            break;
        }
        if(num & 0xe0000000){ // overflow
            *N = 0xffffff;
            return start;
        }
        num <<= 3;
        num += c - '0';
        ++buf;
    }
    *N = num;
    return (char*)buf;
}
// read binary number (without b prefix!)
static char *getbin(const char *buf, uint32_t *N){
    char *start = (char*)buf;
    uint32_t num = 0;
    while(*buf){
        char c = *buf;
        if(c < '0' || c > '1'){
            break;
        }
        if(num & 0x80000000){ // overflow
            *N = 0xffffff;
            return start;
        }
        num <<= 1;
        if(c == '1') num |= 1;
        ++buf;
    }
    *N = num;
    return (char*)buf;
}

/**
 * @brief getnum - read uint32_t from string (dec, hex or bin: 127, 0x7f, 0b1111111)
 * @param buf - buffer with number and so on
 * @param N   - the number read
 * @return pointer to first non-number symbol in buf
 *      (if it is == buf, there's no number or if *N==0xffffffff there was overflow)
 */
char *getnum(const char *txt, uint32_t *N){
    char *nxt = NULL;
    char *s = omit_spaces(txt);
    if(*s == '0'){ // hex, oct or 0
        if(s[1] == 'x' || s[1] == 'X'){ // hex
            nxt = gethex(s+2, N);
            if(nxt == s+2) nxt = (char*)txt;
        }else if(s[1] > '0'-1 && s[1] < '8'){ // oct
            nxt = getoct(s+1, N);
            if(nxt == s+1) nxt = (char*)txt;
        }else{ // 0
            nxt = s+1;
            *N = 0;
        }
    }else if(*s == 'b' || *s == 'B'){
        nxt = getbin(s+1, N);
        if(nxt == s+1) nxt = (char*)txt;
    }else{
        nxt = getdec(s, N);
        if(nxt == s) nxt = (char*)txt;
    }
    return nxt;
}

const char* helpmsg =
    "'0' - shutter CLO\n"
    "'1' - shutter OPE\n"
    "'2' - shutter HIZ\n"
    "'A' - get raw ADC values\n"
    "'C' - close shutter\n"
    //"'N' - read number (dec, 0xhex, 0oct, bbin) and show it in decimal\n"
    "'O' - open shutter\n"
    "'R' - software reset\n"
    "'S' - get shutter state; also hall and ccd inputs state (1 - active)\n"
    "'t' - get MCU temperature (/10degC)\n"
    "'T' - get Tms\n"
    "'v' - get Vdd (/100V)\n"
    "'V' - get shutter voltage (/100V)\n"
    "'W' - test watchdog\n"
;

static char stbuf[256], *bptr = NULL;
static int blen = 0;
static void initbuf(){bptr = stbuf; blen = 255; *bptr = 0;}
void add2buf(const char *s){
    while(blen && *s){
        *bptr++ = *s++;
        --blen;
    }
    *bptr = 0;
}
void bufputchar(char c){
    if(blen == 0) return;
    *bptr++ = c;
    --blen;
    *bptr = 0;
}

extern uint8_t usbON;
const char *parse_cmd(const char *buf){
    uint32_t u3;
    initbuf();
    if(buf[1] == '\n' || buf[1] == '\r' || !buf[1]){ // one symbol commands
        switch(*buf){
            case '0':
                SHTRCLOSE();
                add2buf("regstate=close");
            break;
            case '1':
                SHTROPEN();
                add2buf("regstate=open");
            break;
            case '2':
                SHTRHIZ();
                add2buf("regstate=hiz");
            break;
            case 'A':
                for(int i = 0; i < NUMBER_OF_ADC_CHANNELS; ++i){
                    add2buf("\nadc");
                    bufputchar('0' + i);
                    bufputchar('=');
                    add2buf(u2str(getADCval(i)));
                }
            break;
            case 'C':
                if(close_shutter()) add2buf("OK");
                else add2buf("ERR");
            break;
            case 'O':
                if(open_shutter()) add2buf("OK");
                else add2buf("ERR");
            break;
            case 'R':
                USB_sendstr("Soft reset\n");
                USB_sendall();
                NVIC_SystemReset();
            break;
            case 'S':
                print_shutter_state();
                add2buf("hall=");
                bufputchar('0' + CHKHALL());
                add2buf("\nccd=");
                bufputchar('0' + CHKCCD());
            break;
            break;
            case 't':
                add2buf("mcut=");
                add2buf(u2str(getMCUtemp()));
            break;
            case 'T':
                add2buf("tms=");
                add2buf(u2str(Tms));
            break;
            case 'v':
                add2buf("vdd=");
                add2buf(u2str(getVdd()));
            break;
            case 'V':
                u3 = getADCvoltage(CHSHTR) * SHTRVMUL;
                add2buf("voltage=");
                add2buf(u2str(u3));
            break;
            case 'W':
                USB_sendstr("Wait for reboot\n");
                USB_sendall();
                while(1){nop();};
            break;
            default:
                return helpmsg;
        }
        bufputchar('\n');
        return stbuf;
    }
    uint32_t Num = 0;
    char *nxt;
    switch(*buf){ // long messages
        case 'N':
            ++buf;
            nxt = getnum(buf, &Num);
            if(buf == nxt){
                if(Num == 0) return "Wrong number\n";
                return "Integer32 overflow\n";
            }
            add2buf("You give: ");
            add2buf(u2str(Num));
            if(*nxt && *nxt != '\n'){
                add2buf(", the rest of string: ");
                add2buf(nxt);
            }else add2buf("\n");
        break;
        default:
            return buf;
    }
    return stbuf;
}


// return string with number `val`
char *u2str(uint32_t val){
    static char strbuf[11];
    char *bufptr = &strbuf[10];
    *bufptr = 0;
    if(!val){
        *(--bufptr) = '0';
    }else{
        while(val){
            *(--bufptr) = val % 10 + '0';
            val /= 10;
        }
    }
    return bufptr;
}

char *u2hexstr(uint32_t val){
    static char strbuf[11] = "0x";
    char *sptr = strbuf + 2;
    uint8_t *ptr = (uint8_t*)&val + 3;
    int8_t i, j, z=1;
    for(i = 0; i < 4; ++i, --ptr){
        if(*ptr == 0){ // omit leading zeros
            if(i == 3) z = 0;
            if(z) continue;
        }
        else z = 0;
        for(j = 1; j > -1; --j){
            uint8_t half = (*ptr >> (4*j)) & 0x0f;
            if(half < 10) *sptr++ = half + '0';
            else *sptr++ = half - 10 + 'a';
        }
    }
    *sptr = 0;
    return strbuf;
}
