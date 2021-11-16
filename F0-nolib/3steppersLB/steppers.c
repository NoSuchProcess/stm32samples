/*
 * This file is part of the 3steppers project.
 * Copyright 2021 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include "flash.h"
#include "hardware.h"
#include "steppers.h"
#include "strfunct.h"

// goto zero stages
typedef enum{
    M0RELAX,      // normal moving
    M0FAST,  // fast move to zero
    M0PLUS,   // move 200 steps +
    M0SLOW   // slowest move to zero
} mvto0state;

typedef enum{
    STALL_NO,       // moving OK
    STALL_ONCE,     // Nstalled < limit
    STALL_STOP      // Nstalled >= limit
} t_stalled;

// motors' direction: 1 for positive, -1 for negative (we need it as could be reverse)
static int8_t motdir[MOTORSNO];
// current position (in steps) by STP counter
static volatile int32_t stppos[MOTORSNO] = {0};
// previous position when check (set to current in start of moving)
static int32_t prevstppos[MOTORSNO];
// target stepper position
static int32_t targstppos[MOTORSNO] = {0};
// position to start deceleration
static int32_t decelstartpos[MOTORSNO];
// current encoder position (4 per ticks) (without TIM->CNT)
static volatile int32_t encpos[MOTORSNO] = {0};
// previous encoder position
static int32_t prevencpos[MOTORSNO] = {0};
// encoders' ticks per step (calculates @ init)
static int32_t encperstep[MOTORSNO];

// current speed
static uint16_t curspeed[MOTORSNO];
static uint16_t startspeed[MOTORSNO]; // speed when deceleration starts
// ==1 to stop @ nearest step
static uint8_t stopflag[MOTORSNO];
// motor state
static stp_state state[MOTORSNO];
// move to zero state
static mvto0state mvzerostate[MOTORSNO];

static uint8_t Nstalled = 0; // counter of STALL

// lowest ARR value (highest speed), highest (lowest speed)
//static uint16_t stphighARR[MOTORSNO];
// microsteps=1<<ustepsshift
static uint16_t ustepsshift[MOTORSNO];
// amount of steps for full acceleration/deceleration
static uint32_t accdecsteps[MOTORSNO];

// time when acceleration or deceleration starts
static uint32_t Taccel[MOTORSNO] = {0};

// recalculate ARR according to new speed
TRUE_INLINE void recalcARR(int i){
    uint32_t ARR = (((PCLK/(MOTORTIM_PSC+1)) / curspeed[i]) >> ustepsshift[i]) - 1;
    if(ARR < MOTORTIM_ARRMIN) ARR = MOTORTIM_ARRMIN;
    else if(ARR > 0xffff) ARR = 0xffff;
    mottimers[i]->ARR = ARR;
    curspeed[i] = (((PCLK/(MOTORTIM_PSC+1)) / (ARR+1)) >> ustepsshift[i]); // recalculate speed due to new val
}

// run this function after each steppers parameters changing
void init_steppers(){
    timers_setup(); // reinit timers & stop them
    // init variables
    for(int i = 0; i < MOTORSNO; ++i){
        stopflag[i] = 0;
        motdir[i] = 1;
        curspeed[i] = 0;
        accdecsteps[i] = (the_conf.maxspd[i] * the_conf.maxspd[i]) / the_conf.accel[i] / 2;
        state[i] = STP_RELAX;
        ustepsshift[i] = MSB(the_conf.microsteps[i]);
        encperstep[i] = the_conf.encrev[i] / STEPSPERREV;
        if(!the_conf.motflags[i].donthold) MOTOR_EN(i);
        else MOTOR_DIS(i);
    }
}

// get absolute position by encoder
int32_t encoder_position(uint8_t i){
    int32_t pos = encpos[i];
    if(the_conf.motflags[i].encreverse) pos -= enctimers[i]->CNT;
    else pos += enctimers[i]->CNT;
    return pos;
}

// set encoder's position, return 0 if false (motor's state != relax)
int setencpos(uint8_t i, int32_t position){
    if(state[i] != STP_RELAX) return FALSE;
    int32_t remain = position % the_conf.encrev[i];
    if(remain < 0) remain += the_conf.encrev[i];
    enctimers[i]->CNT = remain;
    prevencpos[i] = encpos[i] = position - remain;
    SEND("MOTOR"); bufputchar('0'+i); SEND(", position="); printi(position); SEND(", remain=");
    printi(remain); SEND(", CNT="); printu(enctimers[i]->CNT); SEND(", pos="); printi(encpos[i]); NL();
    return TRUE;
}

// get current position
errcodes getpos(uint8_t i, int32_t *position){
    if(the_conf.motflags[i].haveencoder){
        int32_t p = encoder_position(i);
        if(p < 0) p -= encperstep[i]>>1;
        else p += encperstep[i]>>1;
        p /= encperstep[i];
        *position = p;
    }else
        *position = stppos[i];
    return ERR_OK;
}

errcodes getremainsteps(uint8_t i, int32_t *position){
    *position = targstppos[i] - stppos[i];
    return ERR_OK;
}

// calculate acceleration/deceleration parameters for motor i
static void calcacceleration(uint8_t i){
    int32_t delta = targstppos[i] - stppos[i];
    if(delta > 0){ // positive direction
        if(delta > 2*(int32_t)accdecsteps[i]){ // can move by trapezoid
            decelstartpos[i] = targstppos[i] - accdecsteps[i];
        }else{ // triangle speed profile
            decelstartpos[i] = stppos[i] + delta/2;
        }
        motdir[i] = 1;
        if(the_conf.motflags[i].reverse) MOTOR_CCW(i);
        else MOTOR_CW(i);
    }else{ // negative direction
        delta = -delta;
        if(delta > 2*(int32_t)accdecsteps[i]){ // can move by trapezoid
            decelstartpos[i] = targstppos[i] + accdecsteps[i];
        }else{ // triangle speed profile
            decelstartpos[i] = stppos[i] - delta/2;
        }
        motdir[i] = -1;
        if(the_conf.motflags[i].reverse) MOTOR_CW(i);
        else MOTOR_CCW(i);
    }
    state[i] = STP_ACCEL;
    startspeed[i] = curspeed[i];
    Taccel[i] = Tms;
    recalcARR(i);
}

// move to absolute position
errcodes motor_absmove(uint8_t i, int32_t newpos){
    //if(i >= MOTORSNO) return ERR_BADPAR; // bad motor number
    if(state[i] != STP_RELAX) return ERR_CANTRUN; // can't move: motor isn't stopping
    if(newpos > (int32_t)the_conf.maxsteps[i] || newpos < -(int32_t)the_conf.maxsteps[i] || newpos == stppos[i])
        return ERR_BADVAL; // too big position or zero
    Nstalled = 0;
    targstppos[i] = newpos;
    prevencpos[i] = encoder_position(i);
    prevstppos[i] = stppos[i];
    curspeed[i] = the_conf.minspd[i];
    calcacceleration(i);
    SEND("MOTOR"); bufputchar('0'+i);
    SEND(" targstppos="); printi(targstppos[i]);
    SEND(", decelstart="); printi(decelstartpos[i]);
    SEND(", accdecsteps="); printu(accdecsteps[i]); NL();
    MOTOR_EN(i);
    mottimers[i]->CR1 |= TIM_CR1_CEN; // start timer
    return ERR_OK;
}

// move i'th motor for relsteps
errcodes motor_relmove(uint8_t i, int32_t relsteps){
    return motor_absmove(i, stppos[i] + relsteps);
}

void stopmotor(uint8_t i){
    stopflag[i] = 1;
}

stp_state getmotstate(uint8_t i){
    return state[i];
}

// count steps @tim 14/15/16
void addmicrostep(uint8_t i){
    static volatile uint16_t microsteps[MOTORSNO] = {0}; // current microsteps position
    if(ESW_state(i)){ // ESW active
        switch(the_conf.ESW_reaction[i]){
            case ESW_ANYSTOP: // stop motor in any direction
                stopflag[i] = 1;
            break;
            case ESW_STOPMINUS: // stop only @ minus
                if(motdir[i] == -1) stopflag[i] = 1;
            break;
            default: // ESW_IGNORE
            break;
        }
    }
    if(++microsteps[i] == the_conf.microsteps[i]){
        microsteps[i] = 0;
        stppos[i] += motdir[i];
        uint8_t stop_at_pos = 0;
        if(motdir[i] > 0){
            if(stppos[i] >= targstppos[i]){ // reached start of deceleration
                stop_at_pos = 1;
            }
        }else{
            if(stppos[i] <= targstppos[i]){
                stop_at_pos = 1;
            }
        }
        if(stopflag[i] || stop_at_pos){ // stop NOW
            if(stopflag[i]) targstppos[i] = stppos[i]; // keep position (for keep flag)
            stopflag[i] = 0;
            mottimers[i]->CR1 &= ~TIM_CR1_CEN; // stop timer
            if(the_conf.motflags[i].donthold)
                MOTOR_DIS(i); // turn off power
            state[i] = STP_RELAX;
            SEND("MOTOR"); bufputchar('0'+i); SEND(" stop @"); printi(stppos[i]); newline();
        }
    }
}

void encoders_UPD(uint8_t i){
    if(enctimers[i]->SR & TIM_SR_UIF){
        int8_t d = 1; // positive (-1 - negative)
        if(enctimers[i]->CR1 & TIM_CR1_DIR) d = -d; // negative
        if(the_conf.motflags[i].encreverse) d = -d;
        if(d == 1) encpos[i] += the_conf.encrev[i];
        else encpos[i] -= the_conf.encrev[i];
    }
    enctimers[i]->SR = 0;
}

// check if motor is stalled
// @return 0 if moving OK,
static t_stalled chkSTALL(uint8_t i){
    if(!the_conf.motflags[i].haveencoder) return STALL_NO;
    int32_t curencpos = encoder_position(i), Denc = curencpos - prevencpos[i];
    int32_t curstppos = stppos[i], Dstp = curstppos - prevstppos[i];
    int difsign = 1;
    if(Dstp < 0){
        Dstp = -Dstp;
        difsign = -difsign;
    }
    if(Dstp < 10){ // didn't move even @ 10 steps
        return STALL_NO;
    }
    if(Denc < 0){
        Denc = -Denc;
        difsign = -difsign;
    }
    if(difsign == -1){ // motor and encoder moves to different sides!!!
        Denc = -Denc; // init STALL state
    }
    prevencpos[i] = curencpos;
    getpos(i, &curstppos); // recalculate current position
    stppos[i] = curstppos;
    prevstppos[i] = curstppos;
    if(Denc < the_conf.encperstepmin[i]*Dstp || the_conf.encperstepmax[i]*Dstp < Denc){ // stall?
        SEND("MOTOR"); bufputchar('0'+i); SEND(" Denc="); printi(Denc); SEND(", Dstp="); printu(Dstp);
        SEND(", speed="); printu(curspeed[i]);
        if(++Nstalled > NSTALLEDMAX){
            stopflag[i] = 1;
            Nstalled = 0;
            SEND("  ---  STALL!"); NL();
            return STALL_STOP;
        }else{
            uint16_t spd = curspeed[i] >> 1; // speed / 2
            curspeed[i] = (spd > the_conf.minspd[i]) ? spd : the_conf.minspd[i];
            // now recalculate acc/dec parameters
            calcacceleration(i);
            SEND("  ---  pre-stall, newspeed="); printu(curspeed[i]); NL();
            return STALL_ONCE;
        }
    }
    prevstppos[i] = curstppos;
    Nstalled = 0;
    return STALL_NO;
}

#define TODECEL() do{state[i] = STP_DECEL;  \
        startspeed[i] = curspeed[i];        \
        Taccel[i] = Tms;                    \
        SEND("MOTOR"); bufputchar('0'+i);   \
        SEND("  -> DECEL@"); printi(stppos[i]); SEND(", V="); printu(curspeed[i]); NL(); \
        }while(0)

// check state of i`th stepper
static void chkstepper(int i){
    int32_t newspeed;
    switch(state[i]){
        case STP_RELAX: // check if need to keep current position
            if(the_conf.motflags[i].haveencoder && the_conf.motflags[i].keeppos){
                getpos(i, &newspeed);
                int32_t diff = stppos[i] - newspeed; // correct `curpos` counter by encoder
                if(diff){
                    SEND("MOTOR"); bufputchar('0'+i);
                    SEND(" diff="); printi(diff);
                    SEND(", change stppos from "); printi(stppos[i]); SEND(" to "); printi(newspeed); NL();
                    stppos[i] = newspeed;
                }
                diff = targstppos[i] - newspeed; // check whether we need to change position
                if(diff){ // try to correct position
                    SEND("MOTOR"); bufputchar('0'+i);
                    SEND(" curpos="); printi(newspeed); SEND(", need="); printi(targstppos[i]); NL();
                    motor_absmove(i, targstppos[i]);
                }
            }
        break;
        case STP_ACCEL: // acceleration to max speed
            if(STALL_NO == chkSTALL(i)){
                //newspeed = curspeed[i] + dV[i];
                newspeed = the_conf.minspd[i] + (the_conf.accel[i] * (Tms - Taccel[i])) / 1000;
                if(newspeed >= the_conf.maxspd[i]){ // max speed reached -> move with it
                    curspeed[i] = the_conf.maxspd[i];
                    state[i] = STP_MOVE;
                    SEND("MOTOR"); bufputchar('0'+i);
                    SEND("  -> MOVE@"); printi(stppos[i]); SEND(", V="); printu(curspeed[i]); NL();
                }else{ // increase speed
                    curspeed[i] = newspeed;
                }
                recalcARR(i);
            }
            // check position for triangle profile
            if(motdir[i] > 0){
                if(stppos[i] >= decelstartpos[i]){ // reached end of acceleration
                    TODECEL();
                }
            }else{
                if(stppos[i] <= decelstartpos[i]){
                    TODECEL();
                }
            }
        break;
        case STP_MOVE: // move @ constant speed until need to decelerate
            if(STALL_NO == chkSTALL(i)){
                // check position
                if(motdir[i] > 0){
                    if(stppos[i] >= decelstartpos[i]){ // reached start of deceleration
                        TODECEL();
                    }
                }else{
                    if(stppos[i] <= decelstartpos[i]){
                        TODECEL();
                    }
                }
            }
        break;
        case STP_DECEL:
            if(STALL_NO == chkSTALL(i)){
                //newspeed = curspeed[i] - dV[i];
                newspeed = startspeed[i] - (the_conf.accel[i] * (Tms - Taccel[i])) / 1000;
                if(newspeed > the_conf.minspd[i]){
                    curspeed[i] = newspeed;
                }else{
                    curspeed[i] = the_conf.minspd[i];
                    state[i] = STP_MVSLOW;
                    SEND("MOTOR"); bufputchar('0'+i);
                    SEND("  -> MVSLOW@"); printi(stppos[i]); NL();
                }
                recalcARR(i);
                //SEND("spd="); printu(curspeed[i]); SEND(", pos="); printi(stppos[i]); newline();
            }
        break;
        default: // STP_MVSLOW, STALL, ERR -> do nothing
            return;
    }
    switch(mvzerostate[i]){
        case M0FAST:
            if(state[i] == STP_RELAX){ // stopped -> move to +
                if(ERR_OK != motor_relmove(i, 50)){
                    state[i] = STP_ERR;
                    mvzerostate[i] = M0RELAX;
                }else
                    mvzerostate[i] = M0PLUS;
            }
        break;
        case M0PLUS:
            if(state[i] == STP_RELAX){ // stopped -> move
                if(ERR_OK != motor_relmove(i, -100)){
                    state[i] = STP_ERR;
                    mvzerostate[i] = M0RELAX;
                }else{
                    state[i] = STP_MVSLOW;
                    mvzerostate[i] = M0SLOW;
                }
            }
        break;
        case M0SLOW:
            if(state[i] == STP_RELAX){
                encpos[i] = 0;
                stppos[i] = 0;
                mottimers[i]->CNT = 0; // set encoder counter to zero
                mvzerostate[i] = M0RELAX;
            }
        break;
        default: // RELAX: do nothing
        break;
    }
}

void process_steppers(){
    static uint32_t Tlast = 0;
    if(Tms - Tlast < MOTCHKINTERVAL) return; // hit every 10ms
    Tlast = Tms;
    for(int i = 0; i < MOTORSNO; ++i){
        chkstepper(i);
    }
}
