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

#ifndef WGS_H
#define WGS_H

#include <map>
#include <string>
#include <vector>
#include <cctype>

#define MAX_GRID_WIDTH 10

class GameGrid {
    // Stores the positions that constitute a valid board for grid games.
public:
    GameGrid() : tile_adjacency(), tiles_set(0) {
        clearTiles();
    }

    // Enable a tile for use on the grid
    void setTile(size_t x, size_t y, bool value) {
        if (x < MAX_GRID_WIDTH && y < MAX_GRID_WIDTH) {
            if (grid_tiles[x][y]) {
                return;
            }
            grid_tiles[x][y] = value;
            tiles_set++;
        }
    }

    // Disable all tiles
    void clearTiles() {
        for(int i = 0; i < MAX_GRID_WIDTH; ++i) {
            for (int j = 0; j < MAX_GRID_WIDTH; ++j) {
                grid_tiles[i][j] = false;
            }
        }
        tiles_set = 0;
    }

    // Determine if a tile is used in the grid
    bool isTileSet(size_t x, size_t y) const {
        if (x < MAX_GRID_WIDTH && y < MAX_GRID_WIDTH) {
            return grid_tiles[x][y];
        }
        return false;
    }

    void setAdjacency(std::string d) { tile_adjacency = d; }
    std::string adjacency() const { return tile_adjacency; }
    size_t tilesSet() const { return tiles_set; }

private:
    bool grid_tiles[MAX_GRID_WIDTH][MAX_GRID_WIDTH];
    std::string tile_adjacency;
    size_t tiles_set;
};


class GameDictionary {
    // The name of a dictionary file
public:
    GameDictionary() : dict_file() {}
    GameDictionary(std::string dict_file_) : dict_file(dict_file_) {}

    std::string dictFileName() const { return dict_file; }
    void setDictFileName(std::string name) { dict_file = name; }

private:
    std::string dict_file;
};


class Preferences {
public:
    std::string preference(std::string key) const {
        auto iter = p.find(key);
        if (iter == p.end()) {
            return std::string();
        }
        return iter->second;
    }

    void setPreference(std::string key, std::string value) {
        p[key] = value;
    }

    const std::map<std::string, std::string> &
    pref_list() const { return p; }
private:
    std::map<std::string, std::string> p;
};


class GameScoringRules {
    // The rules that specify how a game is scored.
public:
    bool qIsQu() const { return q_is_qu; }
    bool wildCardPoints() const { return wild_card_points; }
    bool shortWordMultiplier() const { return short_word_multiplier; }
    int randomBoardSize() const { return random_board_size; }
    int quLength() const { return qu_length; }
    int shortWordLength() const { return short_word_length; }
    int shortWordPoints() const { return short_word_points; }
    int minWordLength() const { return min_word_length; }
    bool roundBonusUp() const { return round_bonus_up; }
    bool multiplyLengthBonus() const { return multiply_length_bonus; }

    void setQIsQu(bool value) { q_is_qu = value; }
    void setWildCardPoints(bool value) { wild_card_points = value; }
    void setShortWordMultiplier(bool value) { short_word_multiplier = value; }
    void setRandomBoardSize(int value) { random_board_size = value; }
    void setQuLength(int value) { qu_length = value; }
    void setShortWordLength(int value) { short_word_length = value; }
    void setShortWordPoints(int value) { short_word_points = value; }
    void setMinWordLength(int value) { min_word_length = value; }
    void setRoundBonusUp(bool value) { round_bonus_up = value; }
    void setMultiplyLengthBonus(bool value) { multiply_length_bonus = value; }

    // need to provide const iterator access to these members if we want
    // to make them private
    std::map<char, int> letter_values;
    std::map<int, double> length_bonuses;

    GameScoringRules():
        q_is_qu(true), random_board_size(0),
        short_word_multiplier(false), qu_length(1), short_word_length(0),
        short_word_points(0), min_word_length(0), wild_card_points(false),
        round_bonus_up(false), multiply_length_bonus(false) {}

    void setLetterValue(char letter, int value) {
        letter_values[letter] = value;
    }

    void setLengthBonus(int length, double bonus) {
        length_bonuses[length] = bonus;
    }

    int letterValue(char letter) const {
        std::map<char, int>::const_iterator it = letter_values.find(std::toupper(letter));
        if (it == letter_values.end()) {
            return 0;
        }
        return it->second;
    }

    double lengthBonus(int length) const {
        std::map<int, double>::const_iterator it = length_bonuses.find(length);
        if (it == length_bonuses.end()) {
            return 0;
        }
        return it->second;
    }

private:
    bool q_is_qu;
    int random_board_size;
    bool short_word_multiplier;
    int qu_length;
    int short_word_length;
    int short_word_points;
    int min_word_length;
    bool wild_card_points;
    bool round_bonus_up;
    bool multiply_length_bonus;
};


class GameLetterDistribution {
    // Class that encapsulates the letter distribution for creation of
    // random boards and the validation of entered boards.  Specifies rules
    // for generating random boards.
public:
    // A vector of dice, each of which has a vector of sides
    std::vector<std::vector<std::string> > dice;
    std::vector<std::string> propensity_list;

    GameLetterDistribution():
        shuffle_letters(true), sample_without_replacement(true),
        shuffle_dice(true) {}

    bool shuffleLetters() const { return shuffle_letters; }
    bool sampleWithoutReplacement() const { return sample_without_replacement; }
    bool shuffleDice() const { return shuffle_dice; }
    std::string generationMethod() const { return generation_method; }
    std::string wordListFile() const { return word_list_file; }
    std::string propensityLetters() const { return propensity_letters; }
    std::string diceLetters() const { return dice_letters; }

    void setShuffleLetters(bool value) { shuffle_letters = value; }
    void setSampleWithoutReplacement(bool value) { sample_without_replacement = value; }
    void setShuffleDice(bool value) { shuffle_dice = value; }
    void setGenerationMethod(const std::string &value) { generation_method = value; }
    void setWordListFile(const std::string &value) { word_list_file = value; }

    void setPropensityLetters(const std::string &letters) {
        propensity_letters = letters;
        propensity_list.clear();

        std::string cur_letters;
        for (std::string::const_iterator iter = letters.begin();
                iter != letters.end(); ++iter) {

            char letter = *iter;
            if (letter == ':' || letter == ';') {
                cur_letters += letter;
            }
            else if (std::isupper(letter) || letter == '?' || letter == '.') {
                cur_letters += letter;
                propensity_list.push_back(cur_letters);
                cur_letters.clear();
            }
            else if (std::islower(letter) && !propensity_list.empty()) {
                propensity_list[propensity_list.size() - 1] += letter;
            }
        }
    }


    void setDiceLetters(const std::string &letters) {
        dice_letters = letters;
        dice.clear();

        std::string cur_letters;
        std::vector<std::string> cur_sides;

        for (std::string::const_iterator iter = letters.begin();
                iter != letters.end(); ++iter) {

            char letter = *iter;
            if (letter == ':' || letter == ';') {
                cur_letters += letter;
            }
            else if (std::isupper(letter) || letter == '?' || letter == '.') {
                cur_letters += letter;
                cur_sides.push_back(cur_letters);
                cur_letters.clear();
            }
            else if (std::islower(letter) && !cur_sides.empty()) {
                cur_sides[cur_sides.size() - 1] += letter;
            }
            else if (*iter == ',' && !cur_sides.empty()) {
                dice.push_back(cur_sides);
                cur_sides.clear();
                cur_letters.clear();
            }
        }
        if (!cur_sides.empty()) {
            dice.push_back(cur_sides);
        }

    }

private:
    bool shuffle_letters;
    bool sample_without_replacement;
    bool shuffle_dice;
    std::string generation_method;  // use enum
    std::string word_list_file;
    std::string propensity_letters;
    std::string dice_letters;
};


class GameRules {  // use sub classes
public:
    std::string grid_design;
    std::string scoring_rules;
    std::string letter_distribution;
    std::string dictionary;
    std::string preferences;
};


class GameConfig {
public:
    std::map<std::string, GameGrid> grids;
    std::map<std::string, GameDictionary> dicts;
    std::map<std::string, GameScoringRules> score_rules;
    std::map<std::string, GameLetterDistribution> letters;
    std::map<std::string, GameRules> game_rules;
    std::map<std::string, Preferences> preferences;
};


class GameRuleSet {
public:
    GameGrid *grid;
    GameDictionary *dict;
    GameScoringRules *scoring_rules;
    GameLetterDistribution *letters;
    Preferences *preferences;
    std::string name;

    GameRuleSet(GameConfig &gc, std::string game) :
        grid(0), dict(0), scoring_rules(0), letters(0), preferences(0), name(game) {
        /* FIXME: check all of these before assigning */
        if (gc.grids.count(gc.game_rules[game].grid_design)) {
            grid = &gc.grids[gc.game_rules[game].grid_design];
        }
        dict = &gc.dicts[gc.game_rules[game].dictionary];
        scoring_rules = &gc.score_rules[gc.game_rules[game].scoring_rules];
        letters = &gc.letters[gc.game_rules[game].letter_distribution];
        preferences = &gc.preferences[gc.game_rules[game].preferences];

        if (gc.preferences.find("Default") != gc.preferences.end()) {
            // Add in Default preferences
            for (auto iter = gc.preferences["Default"].pref_list().begin();
                 iter != gc.preferences["Default"].pref_list().end(); ++iter) {
                auto const &key = iter->first;
                auto const &value = iter->second;
                if (preferences->pref_list().find(key) == preferences->pref_list().end()) {
                    preferences->setPreference(key, value);
                }
            }
        }
    }
};

#endif
