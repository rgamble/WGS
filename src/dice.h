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

#ifndef WGS_DICE_H
#define WGS_DICE_H

#include <vector>
#include <string>
#include <cstdlib>

class Dice {
public:
    Dice(const std::vector<std::vector<std::string> > dice);
    std::string get_letters();
    void swap_dice(int i, int j);
    void roll(int i);
    void roll();

private:
    std::string letters;
    std::vector<std::vector<std::string> > dice;

    // positions[i] = j where i is the board position and j is the die offset
    std::vector<int> positions;

    // positions[i] = j where i is the board position and j is the face of the
    // die at that location
    std::vector<int> die_faces;
    void scramble();
};

#endif
