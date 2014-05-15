/*
 *  HelperFunctions.m
 *  CocoaLauncher
 *
 *  Created by Nico Kist on Sun Apr 13 2003.
 *  Copyright (c) 2003 Nico Kist. All rights reserved.
 *
 */

#include "HelperFunctions.h"


bool flip(bool flop){
    if(flop==false){
        flop=true;
    }
    else{
        flop=false;
    }
    return flop;
}