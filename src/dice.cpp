// This file is part of WGS, a customizable Word Game Solver.
//
// Copyright 2014 Robert Gamble <rgamble99@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <vector>
#include <string>
#include "dice.h"

Dice::Dice(const std::vector<std::vector<std::string> > _dice):
    dice(_dice), positions(dice.size()), die_faces(dice.size()) {
    roll();
}

std::string Dice::get_letters() {
    // Get the letters that correspond to the current board
    std::string s;
    for (size_t i = 0; i < dice.size(); ++i) {
        s += dice[positions[i]][die_faces[i]];
    }
    return s;
}

void Dice::swap_dice(int i, int j) {
    // Swap the positions of two dice
    // TODO: use swap
    int tmp = positions[i];
    positions[i] = positions[j];
    positions[j] = tmp;
    tmp = die_faces[i];
    die_faces[i] = die_faces[j];
    die_faces[j] = tmp;
}

void Dice::roll(int i) {
    // Randomly select a face for die at position i
    die_faces[i] = rand() % dice.at(positions[i]).size();
} 

void Dice::roll() {
    // Roll all dice
    size_t max = dice.size();

    for (size_t i = 0; i < max; ++i) {
        positions[i] = i;
        roll(i);
    } 

    scramble();
}

void Dice::scramble() {
    // Shuffles dice positions
    int max = dice.size() - 1;
    if (max < 0) return;

    int r = 0;

    while (max > 0) {
        r = rand() % max;
        swap_dice(r, max);
        --max;
    }
}
