//
// Created by Robert on 11/20/2019.
// Edited by five 04/2020

#ifndef MINACALC_SOLOCALC_H
#define MINACALC_SOLOCALC_H

#include "NoteDataStructures.h"

typedef std::vector<float> SDiffs;
typedef std::vector<SDiffs> MinaSD;

// This is a very basic difficulty calculator for solo files that I am putting
// together as a proof of concept
std::vector<float>
SoloCalc(const std::vector<NoteInfo>& NoteInfo, float musicrate, float goal);

// Simple wrapper for allrates stolen from MinaCalc -five
MinaSD
SoloCalc(const std::vector<NoteInfo>& notes);

#endif // MINACALC_SOLOCALC_H
