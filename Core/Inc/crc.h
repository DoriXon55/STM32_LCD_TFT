/*
 * crc.h
 *
 *  Created on: Nov 3, 2024
 *      Author: doria
 */
#pragma once


//======================================INCLUDES=================================================
#include <stdint.h>
#include <stddef.h>
#include "frame.h"
//==============================================================================================

//=====================================FUNKCJE==================================================
void calculateCrc16(uint8_t *data, size_t length, uint8_t crc_out[2]);
//==============================================================================================

