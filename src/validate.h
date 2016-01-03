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

#ifndef VALIDATE_H
#define VALIDATE_H

#include "wgs.h"
#include "dice.h"

class Validator {
public:
    Validator() : debug(0), ff_used(0), ff_found(0), dlx_used(0), dlx_found(0), long_words(0)
        { }
    bool validate(const GameRuleSet &grs, std::string to_check,
        bool interpret);
    int getDebug() { return debug; }
    void setDebug(int _debug) { debug = _debug; }
    void printStats();

private:
    bool validateDiceBoard(const std::vector<std::vector<std::string>> &dice,
        const std::vector<std::string> &board_tiles);

    bool validateDiceWord(const std::vector<std::vector<std::string>> &dice,
        const std::string &word);

    bool validatePropensityBoard(const std::vector<std::string> &prop_letters,
        const std::vector<std::string> &board_tiles, bool sample_without_replace);

    bool validatePropensityWord(const std::vector<std::string> &prop_letters,
        const std::string &word, bool sample_without_replace);

    bool multiLetterDice(std::vector<std::vector<std::string>> dice);
    bool multiLetterTiles(std::vector<std::string> tiles);
    void debug_log(std::string s);
    int debug;

    // Statistics
    size_t ff_used;     // The number of times Ford Fulkerson is employed
    size_t ff_found;    // The number of times Ford Fulkerson finds a match
    size_t dlx_used;    // The number of times Dancing Links is employed
    size_t dlx_found;   // The number of times Dancing Links finds a match
    size_t long_words;  // The number of times long word optimization
                        // determines the word is too long to be spelled
};

#endif
