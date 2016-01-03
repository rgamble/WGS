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

#include <map>
#include <cstring>
#include <cmath>
#include <jansson.h>
#include <iostream>
#include <sstream>
#include "wgs.h"

std::string convertInt(int number) {
   std::stringstream ss;
   ss << number;
   return ss.str();
}

int json_read_grids(json_t *grid_root, std::map<std::string, GameGrid> &grids);
int json_read_dicts(json_t *dict_root, std::map<std::string, GameDictionary> &dicts);
int json_read_scoring_rules(json_t *rules_root, std::map<std::string, GameScoringRules> &rules);
int json_read_game_rules(json_t *rules_root, std::map<std::string, GameRules> &rules);
int json_read_letter_distributions(json_t *letters_root, std::map<std::string, GameLetterDistribution> &letters);
int json_read_preferences(json_t *preferences_root, std::map<std::string, Preferences> &preferences);

int json_read_config(GameConfig &config, const char *filename);
int json_write_config(GameConfig &config, const char *filename);

int json_read_config(GameConfig &config, const char *filename) {
    json_error_t error;
    json_t *json_root = json_load_file(filename, 0, &error);
    if (!json_root) {
        std::cerr << "Error parsing config file: Line " << error.line
                << ": " << error.text << std::endl;
        return -1;
    }

    json_read_grids(json_object_get(json_root, "Grids"), config.grids);
    json_read_dicts(json_object_get(json_root, "Dictionaries"), config.dicts);
    json_read_scoring_rules(json_object_get(json_root, "ScoringRules"), config.score_rules);
    json_read_game_rules(json_object_get(json_root, "GameRules"), config.game_rules);
    json_read_letter_distributions(json_object_get(json_root, "LetterDistributions"), config.letters);
    json_read_preferences(json_object_get(json_root, "Preferences"), config.preferences);

    json_decref(json_root);

    return 0;
}

int json_read_grids(json_t *grid_root, std::map<std::string, GameGrid> &grids) {
    if (!grid_root || !json_is_object(grid_root)) {
        return -1;
    }

    int grids_read = 0;

    const char *grid_name;
    json_t *grid_data;

    json_object_foreach(grid_root, grid_name, grid_data) {
        if (!json_is_object(grid_data)) {
            continue;
        }

        json_t *tile_list;
        const char *tile_adjacency = "";

        json_error_t error;
        int rv = json_unpack_ex(grid_data, &error, 0, "{s?o, s?s}",
            "Tiles", &tile_list, "Adjacency", &tile_adjacency);

        if (rv != 0) {
            std::cerr << "Error processing config file: While processing grid " << grid_name
                << ": " << error.text << std::endl;
            continue;
        }

        GameGrid g;
        g.setAdjacency(tile_adjacency);

        if (json_is_array(tile_list)) {
            for (size_t i = 0; i < json_array_size(tile_list); ++i) {
                json_t *pos = json_array_get(tile_list, i);
                int x, y;
                rv = json_unpack_ex(pos, &error, 0, "[i, i]", &x, &y);
                if (rv == 0) {
                    if (x < 0 || x > 10 || y < 0 || y > 10) {
                        std::cerr << "Error processing config file: While processing tile list for grid "
                            << grid_name << ": Position " << x << "," << y << " is out of range"
                            << " for tile #" << (i+1) << std::endl;
                        continue;
                    }
                    g.setTile(x - 1, y - 1, true);
                }
                else { /* rv != 0 */
                    std::cerr << "Error processing config file: While processing tile list grid "
                        << grid_name << ": Invalid tile specification for tile #" << (i+1)
                        << ": " << error.text << std::endl;
                }
            }
        }
        
        grids[grid_name] = g;
        grids_read++;
    }

    return grids_read;
}

int json_read_dicts(json_t *dict_root, std::map<std::string, GameDictionary> &dicts) {
    if (!dict_root || !json_is_object(dict_root)) {
        return -1;
    }

    int dicts_read = 0;

    const char *dict_name;
    json_t *dict_data;

    json_object_foreach(dict_root, dict_name, dict_data) {
        if (!json_is_string(dict_data)) {
            continue;
        }

        GameDictionary d(json_string_value(dict_data));
        dicts[dict_name] = d;
        dicts_read++;
    }

    return dicts_read;
}

int json_read_scoring_rules(json_t *rules_root, std::map<std::string, GameScoringRules> &rules) {
    if (!rules_root || !json_is_object(rules_root)) {
        return -1;
    }

    int rules_read = 0;

    const char *rules_name;
    json_t *rules_data;
    json_error_t error;

    json_object_foreach(rules_root, rules_name, rules_data) {
        if (!json_is_object(rules_data)) {
            continue;
        }

        int q_is_qu = 1;
        int random_board_size = 0;
        int short_word_length = 0;
        int short_word_points = 0;
        int short_word_multipliers = 0;
        int min_word_length = 1;
        int qu_length = 2;
        int round_bonus_up = 0;
        int multiply_length_bonus = 0;
        int wild_card_points = 0;

        json_t *letter_values = NULL;
        json_t *length_bonuses = NULL;

        int rv = json_unpack_ex(rules_data, &error, 0, "{s?b, s?b, s?b, s?b, s?i, s?i, s?i, s?i, s?b, s?i}",
            "QIsQu", &q_is_qu,
            "ShortWordMultiplier", &short_word_multipliers,
            "RoundBonusUp", &round_bonus_up,
            "MultiplyLengthBonus", &multiply_length_bonus,
            "RandomBoardSize", &random_board_size,
            "ShortWordLength", &short_word_length,
            "ShortWordPoints", &short_word_points,
            "MinWordLength", &min_word_length,
            "WildCardPoints", &wild_card_points,
            "QuLength", &qu_length);

        if (rv != 0) {
            std::cerr << "Error processing config file: While processing scoring rules for " << rules_name
                << ": " << error.text << std::endl;
            continue;
        }

        rv = json_unpack_ex(rules_data, &error, 0, "{s?o, s?o}",
            "LetterValues", &letter_values,
            "LengthBonuses", &length_bonuses);

        if (rv != 0) {
            std::cerr << "Error processing config file: While processing scoring rules for " << rules_name
                << ": " << error.text << std::endl;
            continue;
        }

        GameScoringRules r;

        if (random_board_size < 0) {
            std::cerr << "Error processing config file: While processing scoring rules for " << rules_name
                << ": " << random_board_size << " is not a valid value for RandomBoardSize option" << std::endl;
            random_board_size = 0;
        }

        if (short_word_length < 0) {
            std::cerr << "Error processing config file: While processing scoring rules for " << rules_name
                << ": " << short_word_length << " is not a valid value for ShortWordLength option" << std::endl;
            short_word_length = 0;
        }

        if (short_word_points < 0) {
            std::cerr << "Error processing config file: While processing scoring rules for " << rules_name
                << ": " << short_word_points << " is not a valid value for ShortWordPoints option" << std::endl;
            short_word_points = 0;
        }

        if (min_word_length < 0) {
            std::cerr << "Error processing config file: While processing scoring rules for " << rules_name
                << ": " << min_word_length << " is not a valid value for MinWordLength option" << std::endl;
            min_word_length = 0;
        }

        if (qu_length < 0) {
            std::cerr << "Error processing config file: While processing scoring rules for " << rules_name
                << ": " << qu_length << " is not a valid value for QuLength option" << std::endl;
            qu_length = 0;
        }


        r.setQIsQu((bool) q_is_qu);
        r.setRandomBoardSize(random_board_size);
        r.setShortWordLength(short_word_length);
        r.setShortWordPoints(short_word_points);
        r.setShortWordMultiplier((bool) short_word_multipliers);
        r.setMinWordLength(min_word_length);
        r.setQuLength(qu_length);
        r.setRoundBonusUp((bool) round_bonus_up);
        r.setMultiplyLengthBonus((bool) multiply_length_bonus);
        r.setWildCardPoints((bool) wild_card_points);

        if (json_is_object(letter_values)) {
            const char *val_name;
            json_t *val_data;
        
            json_object_foreach(letter_values, val_name, val_data) {
                if (!json_is_integer(val_data)) {
                    std::cerr << "Error processing config file: While processing scoring rules for " << rules_name
                        << ": Invalid value specified for Letter Value for letter " << val_name << std::endl;
                    continue;
                }
        
                r.setLetterValue(*val_name, json_integer_value(val_data));
            }
        }

        if (json_is_object(length_bonuses)) {
            const char *val_name;
            json_t *val_data;
        
            json_object_foreach(length_bonuses, val_name, val_data) {
                if (!json_is_number(val_data)) {
                    std::cerr << "Error processing config file: While processing scoring rules for " << rules_name
                        << ": Invalid value specified for Length Bonus for length " << val_name << std::endl;
                    continue;
                }
        
                r.setLengthBonus(strtoul(val_name, NULL, 10), json_number_value(val_data));
            }
        }

        rules[rules_name] = r;
        rules_read++;
    }

    return rules_read;
}


int json_read_preferences(json_t *preferences_root, std::map<std::string, Preferences> &preferences) {
    if (!preferences_root || !json_is_object(preferences_root)) {
        return -1;
    }

    int prefs_read = 0;
    const char *pref_name;
    json_t *pref_data;
    const char *pref_key;
    json_t *pref_value;

    json_object_foreach(preferences_root, pref_name, pref_data) {
        if (!json_is_object(pref_data)) {
            continue;
        }

        Preferences p;
        json_object_foreach(pref_data, pref_key, pref_value) {
            if (!json_is_string(pref_value)) {
                // TODO: error message
                continue;
            }
            p.setPreference(pref_key, json_string_value(pref_value));
        }

        preferences[pref_name] = p;
    }

    
    return prefs_read;
}


int json_read_game_rules(json_t *rules_root, std::map<std::string, GameRules> &rules) {
    if (!rules_root || !json_is_object(rules_root)) {
        return -1;
    }

    int rules_read = 0;

    const char *rules_name;
    json_t *rules_data;

    json_object_foreach(rules_root, rules_name, rules_data) {
        if (!json_is_object(rules_data)) {
            continue;
        }

        const char *grid_design = "";
        const char *scoring_rules = "";
        const char *letter_distribution = "";
        const char *preferences = "";
        const char *dictionary = "";

        json_unpack(rules_data, "{s?s, s?s, s?s, s?s, s?s}",
            "GridDesign", &grid_design,
            "ScoringRules", &scoring_rules,
            "LetterDistribution", &letter_distribution,
            "Preferences", &preferences,
            "Dictionary", &dictionary);

        GameRules r;
        r.grid_design = grid_design;
        r.scoring_rules = scoring_rules;
        r.letter_distribution = letter_distribution;
        r.preferences = preferences;
        r.dictionary = dictionary;

        rules[rules_name] = r;
        rules_read++;
    }

    return rules_read;
}

int json_read_letter_distributions(json_t *letters_root, std::map<std::string, GameLetterDistribution> &letters) {
    if (!letters_root || !json_is_object(letters_root)) {
        return -1;
    }

    int letters_read = 0;

    const char *letters_name;
    json_t *letters_data;

    json_object_foreach(letters_root, letters_name, letters_data) {
        if (!json_is_object(letters_data)) {
            continue;
        }

        int shuffle_letters = 1;
        int sample_without_replacement = 1;
        int shuffle_dice = 1;
        const char *generation_method = "";
        const char *word_list_file = "";
        const char *propensity_letters = "";
        const char *dice_letters = "";

        json_unpack(letters_data, "{s?b, s?b, s?b, s?s, s?s, s?s, s?s}",
            "ShuffleLetters", &shuffle_letters,
            "SampleWithoutReplacement", &sample_without_replacement,
            "ShuffleDice", &shuffle_dice,
            "GenerationMethod", &generation_method,
            "WordListFile", &word_list_file,
            "PropensityLetters", &propensity_letters,
            "DiceLetters", &dice_letters);

        GameLetterDistribution l;
        l.setShuffleLetters((bool) shuffle_letters);
        l.setSampleWithoutReplacement((bool) sample_without_replacement);
        l.setShuffleDice((bool) shuffle_dice);
        l.setGenerationMethod(generation_method);
        l.setWordListFile(word_list_file);
        l.setPropensityLetters(propensity_letters);
        l.setDiceLetters(dice_letters);

        letters[letters_name] = l;
        letters_read++;
    }

    return letters_read;
}

int json_write_config(GameConfig &config, const char *filename) {
    json_t *config_root = json_object();
    json_t *grid_root = json_object();
    json_t *dict_root = json_object();
    json_t *sr_root = json_object();
    json_t *gr_root = json_object();
    json_t *letter_root = json_object();
    json_t *pref_root = json_object();

    // build the grids
    for (std::map<std::string, GameGrid>::const_iterator i = config.grids.begin(); i != config.grids.end(); ++i) {
        std::string grid_name = i->first;
        const GameGrid &g = i->second;
        
        json_t *positions = json_array();
        for (int i = 0; i < MAX_GRID_WIDTH; ++i) {
            for (int j = 0; j < MAX_GRID_WIDTH; ++j) {
                if (g.isTileSet(i, j)) {
                    json_t *pos = json_array();
                    json_array_append_new(pos, json_integer(i + 1));
                    json_array_append_new(pos, json_integer(j + 1));
                    json_array_append(positions, pos);
                }
            }
        } 

        json_t *grid = json_object();
        json_object_set(grid, "Tiles", positions);
        json_object_set(grid, "Adjacency", json_string(g.adjacency().c_str()));
        json_object_set(grid_root, grid_name.c_str(), grid);
    }

    // build the dicts
    for (std::map<std::string, GameDictionary>::const_iterator i = config.dicts.begin(); i != config.dicts.end(); ++i) {
        std::string dict_name = i->first;
        const GameDictionary &d = i->second;

        json_t *filename = json_string(d.dictFileName().c_str());
        json_object_set(dict_root, dict_name.c_str(), filename);
    }

    // build the scoring rules
    for (std::map<std::string, GameScoringRules>::const_iterator i = config.score_rules.begin(); i != config.score_rules.end(); ++i) {
        std::string rules_name = i->first;
        const GameScoringRules &r = i->second;

        json_t *rules = json_pack("{s:b, s:i, s:b, s:b, s:b, s:b, s:i, s:i, s:i, s:i}",
            "QIsQu", (int) r.qIsQu(),
            "RandomBoardSize", r.randomBoardSize(),
            "MultiplyLengthBonus", (int) r.multiplyLengthBonus(),
            "WildCardPoints", (int) r.wildCardPoints(),
            "RoundBonusUp", (int) r.roundBonusUp(),
            "ShortWordMultiplier", (int) r.shortWordMultiplier(),
            "ShortWordLength", r.shortWordLength(),
            "ShortWordPoints", r.shortWordPoints(),
            "MinWordLength", r.minWordLength(),
            "QuLength", r.quLength());

        json_t *letters = json_object();
        for (std::map<char, int>::const_iterator j = r.letter_values.begin(); j != r.letter_values.end(); ++j) {
            json_object_set(letters, std::string(1, j->first).c_str(), json_integer(j->second));
        }

        json_t *bonuses = json_object();
        for (std::map<int, double>::const_iterator j = r.length_bonuses.begin(); j != r.length_bonuses.end(); ++j) {
            double ivalue = 0;
            if (modf(j->second, &ivalue) == 0) {
                json_object_set(bonuses, convertInt(j->first).c_str(), json_integer((int)ivalue));
            }
            else {
                json_object_set(bonuses, convertInt(j->first).c_str(), json_real(j->second));
            }
        }
        
        json_object_set(rules, "LetterValues", letters);
        json_object_set(rules, "LengthBonuses", bonuses);
        json_object_set(sr_root, rules_name.c_str(), rules);
    }

    // build the letter distributions
    for (std::map<std::string, GameLetterDistribution>::const_iterator i = config.letters.begin(); i != config.letters.end(); ++i) {
        std::string letters_name = i->first;
        const GameLetterDistribution &l = i->second;

        json_t *letters = json_pack("{s:b, s:b, s:b, s:s, s:s, s:s, s:s}",
            "ShuffleLetters", (int) l.shuffleLetters(),
            "ShuffleDice", (int) l.shuffleDice(),
            "SampleWithoutReplacement", l.sampleWithoutReplacement(),
            "GenerationMethod", l.generationMethod().c_str(),
            "WordListFile", l.wordListFile().c_str(),
            "PropensityLetters", l.propensityLetters().c_str(),
            "DiceLetters", l.diceLetters().c_str());

        json_object_set(letter_root, letters_name.c_str(), letters);
    }

    // build the game rules
    for (std::map<std::string, GameRules>::const_iterator i = config.game_rules.begin(); i != config.game_rules.end(); ++i) {
        std::string rules_name = i->first;
        const GameRules &r = i->second;

        json_t *rules = json_pack("{s:s, s:s, s:s, s:s}",
            "GridDesign", r.grid_design.c_str(),
            "ScoringRules", r.scoring_rules.c_str(),
            "LetterDistribution", r.letter_distribution.c_str(),
            "Dictionary", r.dictionary.c_str());

        json_object_set(gr_root, rules_name.c_str(), rules);
    }

    // build the preferences
    for (auto i = config.preferences.begin(); i != config.preferences.end(); ++i) {
        std::string pref_name = i->first;
        const Preferences &p = i->second;

        json_t *prefs = json_object();
        for (auto j = p.pref_list().begin(); j != p.pref_list().end(); ++j) {
            json_object_set(prefs, j->first.c_str(), json_string(j->second.c_str()));
        }

        json_object_set(pref_root, pref_name.c_str(), prefs);
    }

    json_object_set(config_root, "Grids", grid_root);
    json_object_set(config_root, "Dictionaries", dict_root);
    json_object_set(config_root, "ScoringRules", sr_root);
    json_object_set(config_root, "LetterDistributions", letter_root);
    json_object_set(config_root, "GameRules", gr_root);
    json_object_set(config_root, "Preferences", pref_root);
    return json_dump_file(config_root, filename, JSON_INDENT(4) | JSON_PRESERVE_ORDER);
}

