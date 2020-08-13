//
// Created by Robert on 11/20/2019.
// Edited by five 04/2020

#ifndef MINACALC_SOLOCALC_H
#define MINACALC_SOLOCALC_H

#include "Etterna/MinaCalc/MinaCalc.h"

// This is a very basic difficulty calculator for solo files that I am putting
// together as a proof of concept
auto
SoloCalc(const std::vector<NoteInfo>& NoteInfo, float musicrate, float goal)
  -> std::vector<float>;

// Simple wrapper for allrates stolen from MinaCalc -five
auto
SoloCalc(const std::vector<NoteInfo>& notes) -> MinaSD;

#endif // MINACALC_SOLOCALC_H
