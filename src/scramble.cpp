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

#include <string>
#include <cctype>
#include <cstring>
#include <cmath>
#include <sstream>
#include <iostream>
#include "scramble.h"

// Trie function implementations
const Trie * Trie::child(char c) const {
    // Assumes uppercase characters have sequential values
    if (!isupper(c)) return 0;
    if (children) {
        return children[c - 'A'];
    }
    else if (child_letter) {
        if (child_letter == c) {
            return child_ptr;
        }
    }
    return 0;
}

Trie::Trie() {
    children = 0;
    child_ptr = 0;
    child_letter = '\0';
    is_word = false;
}

Trie::~Trie() {
    if (child_ptr) {
        delete child_ptr;
    }
    if (children) {
        for (int i = 0; i < ALPHABET_SIZE; i++) {
            if (children[i]) delete children[i]; //lint !e1551
        }
        delete [] children;
    }
}

void Trie::add_word(const char *word) {
    if (!word) return;

    char letter = std::toupper(*word);

    if (letter == '\0') {
        // If we reach the end of the string, the current position represents
        // a full word
        is_word = true;
        return;
    }

    if (!isupper(letter))
        return;

    int idx = letter - 'A';
    
    if (children) {
        // Already has children array
        if (!children[idx])
            children[idx] = new Trie;

        children[idx]->add_word(word + 1);
    }
    else {
        // No children array
        if (child_letter == letter) {
            // Child matches sought letter
            child_ptr->add_word(word + 1);
        }
        else if (!child_letter) {
            // New Node
            child_letter = letter;
            child_ptr = new Trie;
            child_ptr->add_word(word + 1);
        }
        else {
            // Child node does not match sought letter, convert
            // to children array
            children = new Trie *[ALPHABET_SIZE];
            for (int i = 0; i < ALPHABET_SIZE; i++) {
                children[i] = 0;
            }
            children[child_letter - 'A'] = child_ptr;
            child_ptr = 0;
            child_letter = '\0';
            children[idx] = new Trie;
            children[idx]->add_word(word + 1);
        }
    }
}

bool Trie::is_a_word(const char *word) const {
    // lookup a word starting at the root
    if (!word) return false;

    char letter = *word;

    if (letter == '\0') {
        return is_word;
    }

    if (!isupper(letter))
        return false;

    int idx = *word - 'A';
    if (child_letter) {
        if (child_letter == *word) {
            return child_ptr->is_a_word(word + 1);
        }
        return false;
    }

    return children[idx] && children[idx]->is_a_word(word + 1);
}


Board::Board(std::string _letters, const GameGrid *g):
    letters(_letters), adj_matrix(0) {
    parse_board();
    build_adjacency_matrix(g);
}

Board::~Board() {
    delete [] tile_grid;
    delete [] letter_mult_grid;
    delete [] word_mult_grid;
    delete [] adj_matrix;
}

void Board::build_adjacency_matrix(const GameGrid *g) {
    if (!g || g->adjacency() == "Full") return;
    
    int **pos_matrix = new int*[MAX_GRID_WIDTH];
    for (int i = 0; i < MAX_GRID_WIDTH; ++i) pos_matrix[i] = new int[MAX_GRID_WIDTH];

    size_t pos = 0;

    for (int row = 0; row < MAX_GRID_WIDTH; ++row) {
        for (int col = 0; col < MAX_GRID_WIDTH; ++col) {
            if (g->isTileSet(row, col) && pos < board_size) {
                pos_matrix[row][col] = pos++;
            }
            else {
                pos_matrix[row][col] = -1;
            }
        }
    }

    adj_matrix = new bool[board_size * board_size];
    for (size_t i = 0; i < board_size * board_size; ++i) {
        adj_matrix[i] = false;
    }
    
    for (int row = 0; row < MAX_GRID_WIDTH; ++row) {
        for (int col = 0; col < MAX_GRID_WIDTH; ++col) {
            int pos = pos_matrix[row][col];
            if (pos == -1) continue;

            if (g->adjacency() == "Diagonal") {
                if (row > 0 && col > 0 && pos_matrix[row-1][col-1] != -1) {
                    adj_matrix[pos * board_size + pos_matrix[row-1][col-1]] = true;
                }
                if (row > 0 && col < MAX_GRID_WIDTH - 1 && pos_matrix[row-1][col+1] != -1) {
                    adj_matrix[pos * board_size + pos_matrix[row-1][col+1]] = true;
                }
                if (row < MAX_GRID_WIDTH - 1 && col > 0 && pos_matrix[row+1][col-1] != -1) {
                    adj_matrix[pos * board_size + pos_matrix[row+1][col-1]] = true;
                }
                if (row < MAX_GRID_WIDTH - 1 && col < MAX_GRID_WIDTH - 1 && pos_matrix[row+1][col+1] != -1) {
                    adj_matrix[pos * board_size + pos_matrix[row+1][col+1]] = true;
                }
            }

            if (g->adjacency() == "Diagonal" || g->adjacency() == "Straight") {
                if (row > 0 && pos_matrix[row-1][col] != -1) {
                    adj_matrix[pos * board_size + pos_matrix[row-1][col]] = true;
                }
                if (row < MAX_GRID_WIDTH - 1 && pos_matrix[row+1][col] != -1) {
                    adj_matrix[pos * board_size + pos_matrix[row+1][col]] = true;
                }
                if (col > 0 && pos_matrix[row][col-1] != -1) {
                    adj_matrix[pos * board_size + pos_matrix[row][col-1]] = true;
                }
                if (col < MAX_GRID_WIDTH - 1 && pos_matrix[row][col+1] != -1) {
                    adj_matrix[pos * board_size + pos_matrix[row][col+1]] = true;
                }
            }
        }
    }

    for (int i = 0; i < MAX_GRID_WIDTH; ++i) delete [] pos_matrix[i];
    delete [] pos_matrix;
}

void Board::parse_board() {
    unsigned char letter_multiplier = 1;
    unsigned char word_multiplier = 1;
    int pos = 0;
    board_size = 0;

    for (size_t i = 0; i < letters.size(); i++) {
        if (std::isupper(letters[i]) || letters[i] == '?' || letters[i] == '.') {
            board_size++;
        }
    }

    tile_grid = new std::string[board_size];
    letter_mult_grid = new unsigned char[board_size]; 
    word_mult_grid = new unsigned char[board_size];

    if (board_size == 0) return;

    for (size_t i = 0; i < letters.size(); i++) {
        char letter = letters[i];

        if (letter == ':') {
            letter_multiplier++;
        }
        else if (letter == ';') {
            word_multiplier++;
        }
        else if (std::isalpha(letter) || letter == '?' || letter == '.') {
            if (std::islower(letter) && pos > 0) {
                tile_grid[pos-1] += letter;
            }
            else if (std::isupper(letter) || letter == '?') {
                letter_mult_grid[pos] = letter_multiplier;
                word_mult_grid[pos] = word_multiplier;
                letter_multiplier = word_multiplier = 1;
                tile_grid[pos++] = letter;
            }
            else if (letter == '.') {
                letter_mult_grid[pos] = letter_multiplier;
                word_mult_grid[pos] = word_multiplier;
                letter_multiplier = word_multiplier = 1;
                tile_grid[pos++] = "";
            }
        }
    }
}


Solution::Solution(std::string _word, const unsigned char * start_pos,
    const unsigned char * stop_pos, unsigned _word_length, unsigned _score,
    unsigned _letter_points, unsigned _word_multiplier, double _length_bonus):
    word(_word), word_length(_word_length), score(_score),
    letter_points(_letter_points), word_multiplier(_word_multiplier),
    length_bonus(_length_bonus)  {

    num_positions = stop_pos - start_pos;  //lint !e732
    positions = new unsigned char[num_positions];

    for (size_t i = 0; i < num_positions; i++) {
        positions[i] = start_pos[i];
    }
}

Solution::Solution(const Solution &s) {
    num_positions = s.num_positions;
    score = s.score;
    letter_points = s.letter_points;
    word_multiplier = s.word_multiplier;
    length_bonus = s.length_bonus;
    word_length = s.word_length;
    word = s.word;

    positions = new unsigned char[num_positions];
    std::memcpy(positions, s.positions, num_positions);
}

Solution::Solution(Solution &&s) :
    word(std::move(s.word)) {
    num_positions = s.num_positions;
    score = s.score;
    letter_points = s.letter_points;
    word_multiplier = s.word_multiplier;
    length_bonus = s.length_bonus;
    word_length = s.word_length;
    positions=s.positions;
    s.positions = 0;
}

Solution& Solution::operator=(const Solution &s) {
    if (this == &s) return *this;

    num_positions = s.num_positions;
    score = s.score;
    letter_points = s.letter_points;
    word_multiplier = s.word_multiplier;
    length_bonus = s.length_bonus;
    word_length = s.word_length;
    word = std::move(s.word);

    delete [] positions;
    positions = new unsigned char[num_positions];
    std::memcpy(positions, s.positions, num_positions);
    return *this;
}

Solution& Solution::operator=(Solution &&s) {
    if (this == &s) return *this;

    num_positions = s.num_positions;
    score = s.score;
    letter_points = s.letter_points;
    word_multiplier = s.word_multiplier;
    length_bonus = s.length_bonus;
    word_length = s.word_length;
    word = s.word;

    delete [] positions;
    positions = s.positions;
    s.positions = 0;
    return *this;
}

Solution::~Solution() {
    delete [] positions;
}

std::string Solution::format(const std::string &fmt, bool expand_paren) const {
    std::stringstream result;
    for (std::string::const_iterator i = fmt.begin(); i != fmt.end(); ++i) {
        if (*i == '%') {
            if (++i == fmt.end()) break;
            char fmt_specifier = *i;
            switch (fmt_specifier) {
                case 'w':
                    result << word;
                    break;
                case 's':
                    result << score;
                    break;
                case 'b':
                    result << length_bonus;
                    break;
                case 'm':
                    result << word_multiplier;
                    break;
                case 'l':
                    result << letter_points;
                    break;
                case '%':
                    result << '%';
                    break;
                case 'p':
                    {
                        if (++i == fmt.end()) {
                            return result.str();
                        }
                        char separator = *i;
                        for (size_t pos = 0; pos < num_positions; ++pos) {
                            if (pos > 0) {
                                result << separator;
                            }
                            result << positions[pos] + 1 /* 0-base to 1-base */;
                        }
                    }
                    break;
                case '(':
                    {
                        bool in_escape = false;
                        while (++i != fmt.end()) {
                            char c = *i;
                            if (in_escape) {
                                switch (c) {
                                    case 't': c = '\t'; break;
                                    case 'n': c = '\n'; break;
                                    default: c = *i; break;
                                }
                                in_escape = false;
                            } else {
                                if (c == '\\') {
                                    in_escape = true;
                                    continue;
                                }
                                else if (c == ')') {
                                    // Done
                                    break;
                                }
                            }
                            if (expand_paren) {
                                result << c;
                            }
                        }
                    }
                    break;
                default:
                    result << '%' << fmt_specifier;
                    break;
            } // switch (fmt_specifier)
        } // (*i == '%')
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
    }

    return result.str();
}


void Solver::add_word(const char *word) {
    dict.add_word(word);
}

void Solver::solve(const Board *b, const GameScoringRules &sr) {
    if (!b) return;

    solutions.clear();
    cur_len = 0;
    board = b;
    
    size_t board_size = board->get_board_size();
    delete [] used;
    delete [] path;
    delete [] wildcard;
    used = new unsigned char[board_size];
    path = new unsigned char[board_size];
    wildcard = new char[board_size];

    for (size_t i = 0; i < board_size; i++) {
        path[i] = 0;
        used[i] = 0;
        wildcard[i] = '\0';
    }

    for (size_t i = 0; i < board_size; i++) {
        _solve(i, &dict, board->tile(i), sr);
    }
}

void Solver::_solve(size_t pos, const Trie *t, const std::string &tile, const GameScoringRules &sr) {
    if (!t) return;
    if (tile.empty()) return;

    for (std::string::const_iterator i = tile.begin(); i != tile.end(); ++i) {
        if (*i == '?') {
            for (int i = 0; i < ALPHABET_SIZE; ++i) {
                wildcard[pos] = 'A' + i;
                std::string new_tile_value(1, 'A'+i);
                new_tile_value.append(tile.substr(1));
                _solve(pos, t, new_tile_value, sr);
            }
            return;
        }

        t = t->child(std::toupper(*i));
        if (!t) return;

        // if Q, descend to u
        if (sr.qIsQu() && std::toupper(*i) == 'Q') {
            t = t->child('U');
            if (!t) return;
        }
    }

    used[pos] = 1;
    path[cur_len++] = pos;

    if (t->is_a_word()) {
        // Score solution
        Solution new_solution = score_solution(*board, sr, path, path + cur_len);
        if (int(new_solution.get_word_length()) >= sr.minWordLength()) {
            solutions.emplace_back(new_solution);
        }
    }

    size_t board_size = board->get_board_size();
    for (size_t i = 0; i < board_size; i++) {
        if (!used[i] && board->is_adjacent(pos, i)) {
            _solve(i, t, board->tile(i), sr);
        }
    }

    used[pos] = 0;
    --cur_len;
}


Solution Solver::score_solution(const Board &b, const GameScoringRules &s, const unsigned char *start_pos, const unsigned char *stop_pos) const {
    int word_len = 0;
    unsigned score = 0;
    unsigned letter_points = 0;
    unsigned word_multiplier = 1;
    double length_bonus = 0;
    std::string word = "";

    const unsigned char *iter = start_pos;

    while (iter != stop_pos) {
        unsigned tile_value = 0;

        const std::string &tile_letters = b.tile(*iter);
        for (auto i = tile_letters.begin(); i != tile_letters.end(); ++i) {
            bool is_wildcard = false;
            char letter = *i;

            if (letter == '?') {
                letter = wildcard[*iter];
                is_wildcard = true;
            }

            word_len++;
            word += std::toupper(letter);

            if ((letter == 'Q' || letter == 'q') && s.qIsQu()) {
                word += 'U';
                if (s.quLength() == 2) {
                    word_len++;
                }
            }

            if (!is_wildcard || s.wildCardPoints()) {
                tile_value += s.letterValue(letter);
            }
        }

        unsigned letter_multiplier = b.letter_mult(*iter);

        letter_points += tile_value * letter_multiplier;
        word_multiplier *= b.word_mult(*iter);
        iter++;
    }

    if (word_len < s.minWordLength()) {
        return Solution(word, start_pos, stop_pos, word_len, 0, 0, 1, 0);
    }

    if (word_len <= s.shortWordLength()) {
        if (s.shortWordMultiplier()) {
            return Solution(word, start_pos, stop_pos, word_len,
                word_multiplier * s.shortWordPoints(), s.shortWordPoints(),
                word_multiplier, 0);
        }
        else {
            return Solution(word, start_pos, stop_pos, word_len,
                s.shortWordPoints(), s.shortWordPoints(), 1, 0);
        }
    }

    length_bonus = s.lengthBonus(word_len);
    if (s.multiplyLengthBonus()) {
        if (s.roundBonusUp()) {
            score = ceil(letter_points * word_multiplier * length_bonus);
        }
        else {
            score = (letter_points * word_multiplier * length_bonus);
        }
    }
    else {
        if (s.roundBonusUp()) {
            score = ceil(letter_points * word_multiplier + length_bonus);
        }
        else {
            score = letter_points * word_multiplier + length_bonus;
        }
    }

    return Solution(word, start_pos, stop_pos, word_len, score, letter_points,
        word_multiplier, length_bonus);
}
