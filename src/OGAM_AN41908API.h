/**
 * @file OGAM_AN41908.h
 *
 * Panasonics AN41908 motor driver using a simple SPI interface.
 * @author Volkan OKBAY <volkan.okbay@metu.edu.tr>
 *
 * Copyright (C) 2015 QWERTY Embedded Design
 *
 * Based on a similar utility written for EMAC Inc. SPI class.
 *
 * Copyright (C) 2009 EMAC, Inc.
 */
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef OGAM_AN41908_HEADER_FILE
#define OGAM_AN41908_HEADER_FILE

int initDriver(char* opt); // Tüm registerları dokümandaki değerlere set eder. Ve pozisyonları 0'a çeker.
int setIris(int VAL); // İrisi set eder.
int getIris(); // İris değerini döner.
int setFZ(int SPD, int POS, char* mode, int CCWCW, int VD, int numVD); // Focus&zoom motorlarını kontrol eder.
int getFZ(char* mode); // Focus&zoom değerlerini döner.

#endif
