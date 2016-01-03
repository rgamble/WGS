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

#ifndef WGS_MAKER_H
#define WGS_MAKER_H

#include <string>
#include "wgs.h"
#include "scramble.h"

std::string generate_simple_board(const GameRuleSet &grs); 
std::string generate_board(const GameRuleSet &grs, Solver &s, size_t min_words, size_t min_score, bool reverse_target = false);

#endif
