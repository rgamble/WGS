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

#ifndef WGS_ANALYZE_H
#define WGS_ANALYZE_H

#include <vector>
#include <string>
#include <sstream>
#include "scramble.h"

class SolutionAnalysis {
public:
    SolutionAnalysis(const Board &b, const Solver::SolutionList &solutions);
    std::string format(const std::string fmt, size_t star_value = 0); 

private:
    // Counts of n-letter words
    // index n contains the number of distinct n-letter words in solution
    std::map<size_t, size_t> word_length_counts;

    // Sum of points for words of n-letter words
    // index n contains the total points of the distinct n-letter words
    std::map<size_t, size_t> point_length_counts;

    // Count of n+ letter words
    // index n contains the number of distinct n+ letter words
    std::map<size_t, size_t> word_lengthp_counts;

    // Sum of points for n+ letter words
    // index n contains the total poins of the distinct n+ letters words
    std::map<size_t, size_t> point_lengthp_counts;

    // The number of distinct words that contain the tile at position n
    std::map<size_t, size_t> position_words;

    // The number of points for distinct words containing tile n
    std::map<size_t, size_t> position_points;

    // The highest scoring n-letter word
    std::map<size_t, std::string> best_words;

    // The points for the highest scoring n-letter word
    std::map<size_t, size_t> best_word_points;

    std::string board_letters;
};

#endif
