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

#ifndef SCRAMBLE_H
#define SCRAMBLE_H

#include <cctype>
#include <cstring>
#include <algorithm>
#include <map>
#include <string>
#include "wgs.h"

//lint -sem(Board::parse_board,initializer)

const int ALPHABET_SIZE = 26;

class Trie {
public:
    Trie();
    ~Trie();
    void add_word(const char *word);
    const Trie * find_word(const char *word) const;
    bool is_a_word(const char *word) const;
    bool is_a_word() const { return is_word; }
    const Trie * child(char) const;

private:
    Trie *child_ptr;
    Trie **children;
    char child_letter;
    bool is_word;
};


class Board {
public:
    Board(const std::string _letters, const GameGrid *g);
    ~Board();

    const std::string & tile(size_t i) const 
        { return tile_grid[i]; }
    unsigned char letter_mult(size_t i) const 
        { return letter_mult_grid[i]; }
    unsigned char word_mult(size_t i) const
        { return word_mult_grid[i]; }
    size_t get_board_size() const
        { return board_size; }
    bool is_adjacent(size_t i, size_t j) const
        { return (adj_matrix ? adj_matrix[i * board_size + j] : true); }
    const std::string & get_letters() const
        { return letters; }

private:
    std::string letters;
    bool *adj_matrix;
    std::string *tile_grid;
    unsigned char *letter_mult_grid;
    unsigned char *word_mult_grid;
    size_t board_size;
    void parse_board();
    void build_adjacency_matrix(const GameGrid *g);
    Board(const Board &b);
    Board& operator=(const Board &);
};


class Solution {
public:
    Solution(std::string word, const unsigned char * start_pos, const unsigned char * stop_pos, unsigned word_length, unsigned score, unsigned letter_points, unsigned word_multiplier, double length_bonus);
    Solution(const Solution &s);
    Solution(Solution &&s);
    Solution& operator=(const Solution &s);
    Solution& operator=(Solution &&s);
    ~Solution();
    const std::string & get_word() const { return word; }
    unsigned get_score() const { return score; }
    const unsigned char * get_positions() const { return positions; }
    size_t get_num_positions() const { return num_positions; }
    unsigned get_word_length() const { return word_length; }
    std::string format(const std::string &fmt, bool expand_paren = true) const;
    const unsigned char *positions_begin() const { return positions; }
    const unsigned char *positions_end() const { return positions + num_positions; }
    unsigned int letterPoints() const { return letter_points; }
    unsigned int wordMultiplier() const { return word_multiplier; }
    double lengthBonus() const { return length_bonus; }

private:
    std::string word;
    unsigned int word_length;
    unsigned char *positions;
    size_t num_positions;
    unsigned int score;
    unsigned int letter_points;
    unsigned int word_multiplier;
    double length_bonus;
};


inline bool equal_words(const Solution &a, const Solution &b) {
    return a.get_word() == b.get_word();
}

inline bool operator<(const Solution &a, const Solution &b) {
    if (a.get_word() < b.get_word()) {
        return true;
    }
    else if (a.get_word() > b.get_word()) {
        return false;
    }
    return a.get_score() > b.get_score();
}

class Solver {
public:
    typedef std::multimap<std::string, Solution> SolutionMap;
    typedef std::vector<Solution> SolutionList;
    Solver(): board(0), used(0), path(0), cur_len(0), wildcard(0) {};
    ~Solver() { delete [] used; delete [] path; delete [] wildcard; board = 0; }
    void add_word(const char *word);
    void solve(const Board *b, const GameScoringRules &sr);
    const SolutionList & get_solutions() const { return solutions; }
    Solution score_solution(const Board &b, const GameScoringRules &sr, const unsigned char *begin, const unsigned char *end) const;

private:
    Trie dict;
    SolutionList solutions;
    const Board *board;
    unsigned char *used;
    unsigned char *path;
    size_t cur_len;
    char *wildcard;
    void _solve(size_t pos, const Trie *t, const std::string &l, const GameScoringRules &sr);
};


#endif
