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

#include "analyze.h"
#include <set>
#include <algorithm>

SolutionAnalysis::SolutionAnalysis(
    const Board &b,
    const Solver::SolutionList &solutions) :
    board_letters(b.get_letters()) {

    /* Expects a SolutionList sorted by word then by point value descending */
    std::string last_word = "";
    std::set<int> last_word_positions;
    
    for(auto const &i : solutions) {
        size_t score = i.get_score();
        const std::string &word = i.get_word();
        size_t word_length = word.size();

        if (word != last_word) {
            last_word_positions.clear();
        }

        /* Update best scoring n-letter word */
        if (best_word_points[word_length] < score) {
            best_words[word_length] = word;
            best_word_points[word_length] = score;
        }

        /* Update best scoring overall word */
        if (best_word_points[0] < score) {
            best_words[0] = word;
            best_word_points[0] = score;
        }

        if (word != last_word) {
            // Update the number of words of this length found
            word_length_counts[word_length]++;
            word_length_counts[0]++;

            // Update the number of points found for words of this length.
            // Duplicate words are not counted multiple times, we just want
            // to consider the highest scoring instance which should always
            // be the first one in the list if it is properly sorted.
            point_length_counts[word_length] += score;
            point_length_counts[0] += score;

            // Update word and point counts for p(plus) counters.
            for (size_t j = 0; j <= word_length; ++j) {
                word_lengthp_counts[j]++;
                point_lengthp_counts[j] += score;
            }

            // Update the total number or words and points.
            // Duplicate words are not counted here.
            position_words[0]++;
            position_points[0] += score;
        }

        for (const unsigned char *pos = i.positions_begin();
                pos != i.positions_end(); ++pos) {

            unsigned char p = *pos + 1; // 0-based to 1-based

            if (last_word_positions.find(p) != last_word_positions.end()) {
                // Continue if we have already accounted for one instance of
                // this word at this position
                continue;
            }

            // Multiple instances of words are accounted for but the same
            // word or its points are never counted more than once for each
            // position.
            position_words[p]++;
            position_points[p] += score;
            last_word_positions.insert(p);
        }

        last_word = word;
    }
}


std::string SolutionAnalysis::format(const std::string fmt, size_t star_value) {
    // Return a string containing board solution analysis information formatted
    // according to the the provided fmt string.

    // The format string consists of printf-like format specifiers which are
    // replace with their respective value in the output string, and anything
    // else which is written out unaltered.

    // The valid format specifiers are:

    // %B Board letters
    // %nW (unique) Words found in the board or at position n
    // %nS Score of the whole board or score of words using letter at position n
    // %nC Count of n-letter words
    // %nP Points for all (unique) n-letter words
    // %n+C Count of all (unique) words with n or more letters
    // %n+P Points for all (unique) words with n or more letters
    // %nX Highest scoring n-letter word
    // %nY Score of highest scoring n-letter word

    std::stringstream result;
    
    for (std::string::const_iterator i = fmt.begin(); i != fmt.end(); ++i) {
        if (*i == '%') {            // the start of a format specifier
            size_t counter = 0;     // the optional "n" parameter
            while (++i != fmt.end() && isdigit(*i)) {
                counter = counter * 10 + (*i - '0');
            }

            bool plus_flag = false;
            if (i != fmt.end() && *i == '+') {
                plus_flag = true;
                ++i;
            }

            if (i != fmt.end() && *i == '*') {
                // Use the optional star value as the counter.
                // Useful in GUI interfaces where the output of a static
                // format string can change depending on the tile being
                // examined.
                counter = star_value;
                ++i;
            }

            if (i == fmt.end()) break;
            char fmt_specifier = *i;

            switch (fmt_specifier) {
                case 'B':   // board letters
                    result << board_letters;
                    break;
                case 'W':   // words found
                    result << position_words[counter];
                    break;
                case 'S':   // board score
                    result << position_points[counter];
                    break;
                case 'X':   // board letters
                    result << best_words[counter];
                    break;
                case 'Y':   // board letters
                    result << best_word_points[counter];
                    break;
                case 'C':   // count of words
                    if (plus_flag) {
                        result << word_lengthp_counts[counter];
                    }
                    else {
                        result << word_length_counts[counter];
                    }
                    break;
                case 'P':   // count of points
                    if (plus_flag) {
                        result << point_lengthp_counts[counter];
                    }
                    else {
                        result << point_length_counts[counter];
                    }
                    break;
                case '%':   // literal %
                    result << '%';
                    break;
                default:
                    result << '%' << fmt_specifier;
                    break;
            } // switch(fmt_specifier)
        } // if format specifier
        else if (*i == '\\') {
            if (++i == fmt.end()) break;
            char escape_char = *i;
            switch (escape_char) {
                case '\\':
                    result << '\\';
                    break;
                case 't':
                    result << '\t';
                    break;
                case 'n':
                    result << '\n';
                    break;
                default:
                    result << '\\' << escape_char;
                    break;
            }
        }
        else {
            result << *i;
        }
    } // for each character in fmt

    return result.str();
}
