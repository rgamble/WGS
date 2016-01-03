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

#include "maker.h"
#include <algorithm>
#include <fstream>
#include <cstdlib>
#include <vector>
#include <string>
#include <numeric>
#include "dice.h"
#include "wgs_json.h"

static std::string generate_simple_dice_board(const GameRuleSet &grs);
static std::string generate_simple_prop_board(const GameRuleSet &grs);
static std::string generate_simple_list_board(const GameRuleSet &grs);

static std::string generate_dice_board(const GameRuleSet &grs,
    Solver &s, size_t min_words, size_t min_score, bool reverse_target = false);
static std::string generate_prop_board(const GameRuleSet &grs,
    Solver &s, size_t min_words, size_t min_score, bool reverse_target = false);

std::string generate_simple_board(const GameRuleSet &grs) {
    GameLetterDistribution *ld = grs.letters;
    if (!ld) return "";

    if (ld->generationMethod() == "Dice") {
        return generate_simple_dice_board(grs);
    }

    if (ld->generationMethod() == "LetterPropensity") {
        return generate_simple_prop_board(grs);
    }

    if (ld->generationMethod() == "WordList") {
        return generate_simple_list_board(grs);
    }

    return "";
}


std::string generate_board(const GameRuleSet &grs, Solver &s, size_t min_words, size_t min_score, bool reverse_target) {
    GameLetterDistribution *ld = grs.letters;
    if (!ld) return "";

    if (ld->generationMethod() == "Dice") {
        return generate_dice_board(grs, s, min_words, min_score, reverse_target);
    }

    if (ld->generationMethod() == "LetterPropensity") {
        return generate_prop_board(grs, s, min_words, min_score, reverse_target);
    }

    return "";
}


std::string generate_simple_dice_board(const GameRuleSet &grs) {
    GameLetterDistribution *ld = grs.letters;
    size_t max_letters = grs.scoring_rules->randomBoardSize();
    if (max_letters == 0 or grs.grid->tilesSet() < max_letters) {
        max_letters = grs.grid->tilesSet();
    }

    std::vector<std::string> board;
    auto dice = ld->dice;

    if (ld->shuffleDice()) {
        random_shuffle(dice.begin(), dice.end());
    }

    if (dice.size() > max_letters) {
        // Get rid of extra dice
        dice.erase(dice.begin() + max_letters, dice.end());
    }
        
    for (auto iter = dice.begin(); iter != dice.end(); ++iter) {
        board.push_back(iter->at(rand() % iter->size()));
        if (board.size() == max_letters) {
            break;
        }
    }
    return std::accumulate(board.begin(), board.end(), std::string(""));
}


std::string generate_simple_prop_board(const GameRuleSet &grs) {
    GameLetterDistribution *ld = grs.letters;
    size_t max_letters = grs.scoring_rules->randomBoardSize();
    if (max_letters == 0 or grs.grid->tilesSet() < max_letters) {
        max_letters = grs.grid->tilesSet();
    }

    std::vector<std::string> board;
    std::vector<std::string> letters = ld->propensity_list;
    
    if (ld->sampleWithoutReplacement()) {
        for (size_t i = 0; i < max_letters; ++i) {
            if (i == letters.size()) {
                break;
            }
            int j = i + (rand() % (letters.size() - i));
            board.push_back(letters[j]);
            std::swap(letters[i], letters[j]);
        }
    }
    else {
        for (size_t i = 0; i < max_letters; ++i) {
            board.push_back(letters[rand() % letters.size()]);
        }
    }

    return std::accumulate(board.begin(), board.end(), std::string(""));
}


std::string generate_simple_list_board(const GameRuleSet &grs) {
    std::string board;
    std::string line;
    size_t lines = 1;
    GameLetterDistribution *ld = grs.letters;
    std::string word_list_file = ld->wordListFile();

    std::ifstream word_list(word_list_file.c_str());
    if (!word_list) {
        return board;
    }

    while (word_list >> line) {
        if (rand() / (double) RAND_MAX <= 1.0 / lines) {
            board = line;
        }
        ++lines;
    }
    word_list.close();

    if (ld->shuffleLetters()) {
        Board b(board, grs.grid);
        // Get the tiles from the board and shuffle
        std::vector<std::string> board_tiles;
        for (size_t i = 0; i < b.get_board_size(); ++i) {
            std::string letter_mult(b.letter_mult(i) - 1, ':');
            std::string word_mult(b.word_mult(i) - 1, ';');
            board_tiles.push_back(letter_mult + word_mult + b.tile(i) == "" ? "." : b.tile(i));
        }
        random_shuffle(board_tiles.begin(), board_tiles.end());
        board = std::accumulate(board_tiles.begin(), board_tiles.end(),
            std::string(""));
    }
    return board;
}


std::string generate_dice_board(const GameRuleSet &grs, Solver &s, size_t min_words, size_t min_score, bool reverse_target) {
    GameLetterDistribution *ld = grs.letters;
    bool is_anagram = false;
    if (grs.grid->adjacency() == "Full") {
        is_anagram = true;
    }
    size_t max_letters = grs.scoring_rules->randomBoardSize();
    if (max_letters == 0 or grs.grid->tilesSet() < max_letters) {
        max_letters = grs.grid->tilesSet();
    }

    int max_duds = 200;
    size_t best_score = -reverse_target;
    size_t best_points = -reverse_target;
    int duds = 0;
    int changes = 1;
    int iterations = 0;
    auto dice = ld->dice;

    if (ld->shuffleDice()) {
        random_shuffle(dice.begin(), dice.end());
    }

    if (dice.size() > max_letters) {
        // Get rid of extra dice
        dice.erase(dice.begin() + max_letters, dice.end());
    }
        
    int num_dice = dice.size();
    Dice best(dice);
    best.roll();

    do {
        iterations++;
        Dice tmp(best);

        /* Don't do a die swap if anagram, anagrams are already fully connected */
        if (is_anagram || (rand() % 2)) {
            int i = rand() % num_dice;
            tmp.roll(i);
        } 
        else {
            // Swap die
            int i = rand() % num_dice;
            int j = rand() % num_dice;
            tmp.swap_dice(i, j);
        }

        std::string letters = tmp.get_letters();
        Board b(letters, grs.grid);
        s.solve(&b, *grs.scoring_rules);

        Solver::SolutionList solutions = s.get_solutions();
        sort(solutions.begin(), solutions.end());
        solutions.erase(unique(solutions.begin(), solutions.end(), equal_words), solutions.end());

        size_t board_score = solutions.size();
        size_t board_points = 0;    

        for(Solver::SolutionList::iterator i = solutions.begin(); i != solutions.end(); ++i) {
            board_points += i->get_score();
        }

        if (
            (reverse_target && ((board_score < best_score || board_points < best_points) or 
             ( (int)(board_score - best_score) < (250 / (changes) ) ))) ||
            (!reverse_target && ((board_score > best_score || board_points > best_points) or 
             ( (int)(best_score - board_score) < (250 / (changes) ) )))
            ) 
        {
            best = tmp;
            best_score = board_score;
            best_points = board_points;
            duds = 0;
            changes++;
        }
        else {
            duds++;
        }
            
    } while (duds < max_duds and (
        (!reverse_target && (best_score < min_words || best_points < min_score)) ||
        (reverse_target && (best_score > min_words || best_points > min_score)) ));

    std::string result = best.get_letters();
    return result;
} 


std::string generate_prop_board(const GameRuleSet &grs, Solver &s, size_t min_words, size_t min_score, bool reverse_target) {
    GameLetterDistribution *ld = grs.letters;
    bool is_anagram = false;
    if (grs.grid->adjacency() == "Full") {
        is_anagram = true;
    }

    size_t max_letters = grs.scoring_rules->randomBoardSize();
    if (max_letters == 0 or grs.grid->tilesSet() < max_letters) {
        max_letters = grs.grid->tilesSet();
    }

    int max_duds = 200;
    size_t best_score = -reverse_target;
    size_t best_points = -reverse_target;
    int duds = 0;
    int changes = 1;
    int iterations = 0;
    size_t num_letters = max_letters;
    std::vector<std::string> best;
    std::vector<std::string> pool;
    std::vector<std::string> prop_letters(ld->propensity_list);

    if (ld->sampleWithoutReplacement()) {
        size_t i;
        for (i = 0; i < max_letters; ++i) {
            if (i == prop_letters.size()) {
                num_letters = i;
                break;
            }
            int j = i + (rand() % (prop_letters.size() - i));
            best.push_back(prop_letters[j]);
            std::swap(prop_letters[i], prop_letters[j]);
        }
        if (i < prop_letters.size()) {
            pool.clear();
            std::copy(prop_letters.begin() + i, prop_letters.end(), back_inserter(pool));
        }
    }
    else {
        for (size_t i = 0; i < max_letters; ++i) {
            best.push_back(prop_letters[rand() % prop_letters.size()]);
        }
    }

    if (is_anagram && ld->sampleWithoutReplacement() && pool.empty()) {
        /* For anagram games, the letter graph is already fully connected
           so the only thing to do is switch out letters for other letters.
           If we are sampling without replacement and there are no letters
           left to swap in from the pool, there is nothing left to do. */
        return std::accumulate(best.begin(), best.end(), std::string(""));
    }

    do {
        iterations++;
        std::vector<std::string> tmp(best);
        std::vector<std::string> save_pool(pool);

        /* Don't do a die swap if anagram, anagrams are already fully connected */
        if (is_anagram || ((rand() % 2) && !(ld->sampleWithoutReplacement() && pool.size() == 0))) {
            /* Change one of the letters */
            int i = rand() % num_letters;
            if (ld->sampleWithoutReplacement()) {
                // Swap with a remaining pool letter
                int j = rand() % pool.size();
                std::swap(tmp[i], pool[j]);
            }
            else {
                int j = rand() % prop_letters.size();
                tmp[i] = prop_letters[j];
            }
        } 
        else {
            // Swap die
            int i = rand() % num_letters;
            int j = rand() % num_letters;
            std::swap(tmp[i], tmp[j]);
        }

        std::string tmp_board = std::accumulate(tmp.begin(), tmp.end(), std::string(""));
        Board b(tmp_board, grs.grid);
        s.solve(&b, *grs.scoring_rules);
        Solver::SolutionList solutions = s.get_solutions();
        sort(solutions.begin(), solutions.end());
        solutions.erase(unique(solutions.begin(), solutions.end(), equal_words), solutions.end());

        size_t board_score = solutions.size();
        size_t board_points = 0;    

        for(Solver::SolutionList::iterator i = solutions.begin(); i != solutions.end(); ++i) {
            board_points += i->get_score();
        }

        if (
            (reverse_target && ((board_score < best_score || board_points < best_points) or 
             ( (int)(board_score - best_score) < (250 / (changes) ) ))) ||
            (!reverse_target && ((board_score > best_score || board_points > best_points) or 
             ( (int)(best_score - board_score) < (250 / (changes) ) )))
            ) 
        {
            best = tmp;
            best_score = board_score;
            best_points = board_points;
            duds = 0;
            changes++;
        }
        else {
            duds++;
            pool = save_pool;
        }

    } while (duds < max_duds and (
        (!reverse_target && (best_score < min_words || best_points < min_score)) ||
        (reverse_target && (best_score > min_words || best_points > min_score)) ));

    return std::accumulate(best.begin(), best.end(), std::string(""));
} 
