/*
 * crc.h
 *
 *  Created on: Nov 3, 2024
 *      Author: doria
 */
#pragma once

#include <stdint.h>
#include <stddef.h>
#include "frame.h"

void calculateCrc16(uint8_t *data, size_t length, char crc_out[2]);

