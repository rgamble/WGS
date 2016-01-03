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

#include <iostream>
#include <algorithm>
#include <iostream>
#include "validate.h"
#include "scramble.h"
#include "dlx.h"
#include "ford_fulkerson.cpp"

bool Validator::multiLetterDice(std::vector<std::vector<std::string>> dice) {
    // Determine if any of the dice have multiple letters
    for (const auto &die : dice) {
        for (const auto &face : die) {
            if (face.size() > 1) {
                return true;
            }
        }
    }
    return false;
}


bool Validator::multiLetterTiles(std::vector<std::string> tiles) {
    // Determine if any of the tiles have multiple letters
    for (const auto &tile : tiles) {
        if (tile.size() > 1) {
            return true;
        }
    }
    return false;
}


void Validator::debug_log(std::string s) {
    // write debug message if debugging is enabled
    if (debug > 0) {
        std::cerr << s;
    }
}


void Validator::printStats() {
    // write out debugging stats
    std::cerr << "Validator stats: " << "\n"
        << "FF Used:    " << ff_used << "\n"
        << "FF Found:   " << ff_found << "\n"
        << "DLX Used:   " << dlx_used << "\n"
        << "DLX Found:  " << dlx_found << "\n"
        << "Long words: " << long_words << std::endl; 
}


bool Validator::validateDiceWord(const std::vector<std::vector<std::string>> &dice,
    const std::string &word) {

    // Determine if the provided word can be spelled using some arrangements
    // of the provided dice.  The strategy is the first check using FF and
    // then fall back to the slower DLX algorithm if needed.  We only need to
    // fall back if FF returns false and there are multi-letter faces that
    // could appear in the test word.

    bool multiletter_tiles = multiLetterDice(dice);
    // Run FF
    // 0 = super source
    // z = super sink
    // 1 - x = dice
    // x - y = letters

    debug_log("Checking with FF\n");
    ff_used++;

    fordFulkerson ff(dice.size() + word.size() + 2);
    size_t source = 0;
    size_t sink = dice.size() + word.size() + 1;

    // Add paths from source to dice
    for (size_t i = 1; i <= dice.size(); ++i) {
        ff.addEdge(source, i);
    }

    // Add paths from word to sink
    for (size_t i = 1; i <= word.size(); ++i) {
        ff.addEdge(i + dice.size(), sink);
    }

    // Aded paths from dice to word letters
    for (size_t i = 1; i <= dice.size(); i++) {
        for (size_t j = 1; j <= dice[i-1].size(); ++j) {
            if (dice[i-1][j-1].size() > 1) continue;
            for (size_t k = 1; k <= word.size(); k++) {
                if (dice[i-1][j-1][0] == word[k-1] || dice[i-1][j-1][0] == '?') {
                    ff.addEdge(i, k + dice.size());
                }
            }
        }
    }

    size_t flow_result = ff.maxFlow(source, sink);
    if (flow_result == word.size()) {
        debug_log("FF found a solution, done\n");
        ff_found++;
        return true;
    }
    else {
        // Ensure that word length does not exceed dice capacity
        size_t capacity = 0;
        for (size_t i = 1; i <= dice.size(); i++) {
            size_t max_face_len = 0;
            for (size_t j = 1; j <= dice[i-1].size(); ++j) {
                size_t face_len = dice[i-1][j-1].size();
                if (face_len > max_face_len) {
                    max_face_len = face_len;
                }
            }
            capacity += max_face_len;
        }
        if (word.size() > capacity) {
            debug_log("Word is too long to be spelled with candidate dice, done\n");
            long_words++;
            return false;
        }
                    
        // FF did not find a solution
        if (multiletter_tiles) {
            // fall back to DLX now IFF there are multi-letter faces that
            // exist in target word
            for (size_t i = 1; i <= dice.size(); i++) {
                for (size_t j = 1; j <= dice[i-1].size(); ++j) {
                    if (dice[i-1][j-1].size() <= 1) continue;
                    if (word.find(dice[i-1][j-1]) != std::string::npos ||  (dice[i-1][j-1][0] == '?' && word.find(dice[i-1][j-1].substr(1)) != std::string::npos && word.find(dice[i-1][j-1].substr(1)) > 0)) {
                        debug_log("FF returned false but at least one multi-letter face (" + dice[i-1][j-1] + ") exists in word, falling back to DLX\n");
                        goto DLX;
                    }
                }
            }
        }
        debug_log("FF returned false and there are no matching multi-letter tiles, done\n");
        return false;
    }

    DLX:
    debug_log("Using DLX\n");
    // Run DLX
    dlx_used++;
    int cols = word.size() + dice.size();
    DLX dlx;
    for (int i = 0; i < cols; ++i) {
        dlx.addColumn("A");  // The column name is not currently used
    }
    
    std::vector<size_t> positions; // positions to add in each row

    // Columns 0 - n = word letters
    // columns n - x = dice

    int die_offset = word.size();

    for (const auto &die : dice) {
        for (const auto &face : die) {
            if (face == "?") {
                // Handle wildcards
                for (size_t i = 0; i < word.size(); ++i) {
                    positions.clear();
                    positions.push_back(i);
                    positions.push_back(die_offset);
                    dlx.addRow(positions);
                }
            }
            else {
                size_t pos;
                bool using_wildcard = false;
                std::string face_text;

                if (face[0] == '?') {
                    // Handle letters after ?
                    pos = 1;
                    face_text = std::string(face, 1);
                    using_wildcard = true;
                }
                else {
                    pos = 0;
                    face_text = std::string(face);
                }
                    
                // Find every spot in the word where the current die
                // face_text matches and add it to the candidate set
                while ((pos = word.find(face_text, pos)) != std::string::npos) {
                    positions.clear();
                    // Add all letters covered by face_text
                    for (size_t i = 0; i < face_text.size(); ++i) {
                        positions.push_back(pos + i);
                    }
                    if (using_wildcard) {
                        positions.push_back(pos - 1);
                    }
                    positions.push_back(die_offset);
                    dlx.addRow(positions);
                    pos++;
                }
            }
        }

        // Add the die without any positions
        positions.clear();
        positions.push_back(die_offset);
        dlx.addRow(positions);
        die_offset++;
    }
    size_t result = dlx.solve(false);
    if (result) {
        debug_log("DLX found a solution, done\n");
        dlx_found++;
    }
    else {
        debug_log("DLX did not find a solution, done\n");
    }
    return result;
}


bool Validator::validateDiceBoard(const std::vector<std::vector<std::string>> &dice,
    const std::vector<std::string> &board_tiles) {

    // Validate a board.  This means determining if the die faces represented
    // by board_tiles are a valid combination produceable with the provided
    // set of dice.  Bipartite matching can always find a solution quickly,
    // even if die faces contain multiple letters.  Note that a board does
    // not have to use all of the dice or even the number used by the game
    // type to be considered valid as long as the ones that are present are
    // valid.

    // Run FF
    // 0 = super source
    // z = super sink
    // 1 - x = dice
    // x - y = letters

    ff_used++;
    fordFulkerson ff(dice.size() + board_tiles.size() + 2);
    size_t source = 0;
    size_t sink = dice.size() + board_tiles.size() + 1;

    // Add paths from source to dice
    for (size_t i = 1; i <= dice.size(); ++i) {
        ff.addEdge(source, i);
    }

    // Add paths from board to sink
    for (size_t i = 1; i <= board_tiles.size(); ++i) {
        ff.addEdge(i + dice.size(), sink);
    }

    // Add paths from dice to letters
    for (size_t i = 1; i <= dice.size(); i++) {
        for (size_t j = 1; j <= dice[i-1].size(); ++j) {
            for (size_t k = 1; k <= board_tiles.size(); k++) {
                if (dice[i-1][j-1] == board_tiles[k-1]) {
                    ff.addEdge(i, k + dice.size());
                }
            }
        }
    }

    size_t result = (ff.maxFlow(source, sink) == board_tiles.size());
    if (result) {
        ff_found++;
    }
    return result;
}


bool Validator::validatePropensityBoard(const std::vector<std::string> &prop_letters,
    const std::vector<std::string> &board_tiles, bool sample_without_replace) {
    // Validating the board of a propensity based game is quite simple,
    // just verify that each board tile exists in the pool and remove
    // each used tile from the pool if SampleWithoutReplacement is true.

    // Check to see if the board_tiles form a valid board
    debug_log("In validatePropensityBoard()\n");
    auto letters = prop_letters;
    for (auto tile : board_tiles) {
        auto iter = find(letters.begin(), letters.end(), tile);
        if (iter == letters.end()) {
            debug_log("Tile '" + tile + "' does not exist in pool, done\n");
            return false;
        }
        if (sample_without_replace) {
            letters.erase(iter);
        }
    }
    
    return true;
}


bool Validator::validatePropensityWord(const std::vector<std::string> &prop_letters,
    const std::string &word, bool sample_without_replace) {
    // Check to see if word can be spelled using the provided single-letter
    // tiles.  If word cannot be formed, check to see if any multi-letter tiles
    // exist that appear in word.  If no, return false.  Otherwise fall back to DLX.

    bool multiletter_tiles = multiLetterTiles(prop_letters);

    // Check to see if word can be spelled with board tiles
    debug_log("In validatePropensityWord()\n");
    auto letters = prop_letters;

    // Iterate through each letter in word and determine if that letter is
    // available in the letter pool
    for (auto letter : word) {
        auto iter = find(letters.begin(), letters.end(), std::string(1, letter));
        if (iter == letters.end()) {
            // Check to see if a wildcard is available
            iter = find(letters.begin(), letters.end(), std::string("?"));

            if (iter == letters.end()) {
                // Failed to spell word with single-letter tiles, check for multi-
                // letter tiles
                if (multiletter_tiles) {
                    // fall back to DLX now IFF there are multi-letter faces that
                    // exist in target word
                    for (size_t i = 0; i < letters.size(); i++) {
                        if (letters[i].size() <= 1) continue;
                        if (word.find(letters[i]) != std::string::npos ||
                            (letters[i][0] == '?' && word.find(letters[i].substr(1)) != std::string::npos && word.find(letters[i].substr(1)) > 0)) {
                            debug_log("no solution found using single-letter tiles but at least one multi-letter tile (" + letters[i] + ") exists in word, falling back to DLX\n");
                            goto DLX;
                        }
                    }
                    debug_log("no solution found using single-letter tiles and no multi-letter tiles match word, done\n");
                    return false;
                }
                debug_log("Tile '" + std::string(1, letter) + "' does not exist in pool and no multi-letter tiles exist, done\n");
                return false;
            }
        }
        if (sample_without_replace) {
            letters.erase(iter);
        }
    }
    
    return true;

    DLX:
    debug_log("Using DLX\n");
    // Run DLX
    dlx_used++;

    letters.clear();

    // Build a map of prop letters to counts which will be used to constrain
    // the number of times each letter can be used if SampleWithoutReplacement
    // is set.
    std::map<std::string, size_t> prop_counts;
    for (auto const &prop_letter : prop_letters) {
        prop_counts[prop_letter]++;
    }

    // The number of times each tile matches in the target word
    std::map<std::string, size_t> letters_map;
    
    // Go through prop letters, add the number of occurrence of each one needed
    // limited by the number available if SampleWithoutReplacement is false
    for (auto const &map_entry : prop_counts) {
        auto const &letter = map_entry.first;
        auto const &count = map_entry.second;

        if (letter == "?") {
            // Ignore wildcards if SampleWithoutReplacement is false as there
            // would have already been a match in such a case.
            if (sample_without_replace) {
                letters_map[letter] = min(word.size(), count);
            }
        }
        else {
            size_t pos;
            std::string face_text;

            if (letter[0] == '?') {
                // Handle letters after ?
                pos = 1;
                face_text = std::string(letter, 1);
            }
            else {
                pos = 0;
                face_text = std::string(letter);
            }
                    
            // Find every spot in the word where the current die
            // face_text matches and add it to the candidate set
            size_t match_count = 0;
            while ((pos = word.find(face_text, pos)) != std::string::npos) {
                match_count++;
                pos++;
            }
            if (sample_without_replace) {
                letters_map[letter] = min(match_count, count);
            }
            else {
                letters_map[letter] = match_count;
            }
        }
    }

    // Build letters from letters_map
    for (auto const &map_entry : letters_map) {
        auto const &letter = map_entry.first;
        auto const &count = map_entry.second;
        for (size_t i = 0; i < count; ++i) {
            letters.push_back(letter);
        }
    }

    int cols = word.size() + letters.size();
    DLX dlx;
    for (int i = 0; i < cols; ++i) {
        dlx.addColumn("A");  // The column name is not used
    }
    
    std::vector<size_t> positions; // positions to add in each row

    // Columns 0 - n = word letters
    // columns n - x = propensity letter tiles

    int tile_offset = word.size();

    for (const auto &letter : letters) {
        if (letter == "?") {
            // Handle wildcards
            for (size_t i = 0; i < word.size(); ++i) {
                positions.clear();
                positions.push_back(i);
                positions.push_back(tile_offset);
                dlx.addRow(positions);
            }
        }
        else {
            size_t pos;
            std::string face_text;
            bool using_wildcard = false;

            if (letter[0] == '?') {
                // Handle letters after ?
                pos = 1;
                face_text = std::string(letter, 1);
                using_wildcard = true;
            }
            else {
                pos = 0;
                face_text = std::string(letter);
            }
                
            // Find every spot in the word where the current die
            // face_text matches and add it to the candidate set
            while ((pos = word.find(face_text, pos)) != std::string::npos) {
                positions.clear();
                // Add all letters covered by face_text
                for (size_t i = 0; i < face_text.size(); ++i) {
                    positions.push_back(pos + i);
                }
                if (using_wildcard) {
                    positions.push_back(pos - 1);
                }
                positions.push_back(tile_offset);
                dlx.addRow(positions);
                pos++;
            }
        }

        // Add the tile without any positions
        positions.clear();
        positions.push_back(tile_offset);
        dlx.addRow(positions);
        tile_offset++;
    }

    size_t result = dlx.solve(false);
    if (result) {
        dlx_found++;
    }
    return result;
}


bool Validator::validate(const GameRuleSet &grs, std::string to_check,
    bool interpret) {
    // Check a set of tiles (or die faces) and determine if it forms a valid
    // board for the given game type or a word that can be spelled using the
    // available dice/tiles for the game type.  The interpret flag is used
    // for checking whether a word can be spelled, and will expand wildcards
    // and properly match multi-letter tiles to words.  This should not be
    // set if the goal is to validate a board.  This function will return
    // false if the word or board is not valid and true if it is.

    // This function does all the prep work, the actually solving is done in
    // separate functions depending on the letter distribution strategy.
    const std::string &dist_method = grs.letters->generationMethod();
    
    if (dist_method == "Dice") {
        std::vector<std::vector<std::string>> dice = grs.letters->dice;

        // Remove everything except A-Z, a-z, and "?" from dice
        for (auto &die : dice) {
            for (auto &face : die) {
                face.erase(remove_if(face.begin(), face.end(), [](const char &c) -> bool {
                        return (!(std::isalpha(c) || c == '?')); }), face.end());
                // Uppercase letters
                for (auto &c : face) {
                    c = std::toupper(c);
                }
            }
        }

        // Do the same for board/word
        to_check.erase(remove_if(to_check.begin(), to_check.end(), [interpret](const char &c) -> bool {
                return (!(std::isalpha(c) || (!interpret &&c == '?'))); }), to_check.end());

        // Dedup die faces
        for (auto &die : dice) {
            sort(die.begin(), die.end());
            die.erase(unique(die.begin(), die.end()), die.end());
        }

        // Get list of uppercased board letters
        std::vector<std::string> board_tiles;
        Board b(to_check, grs.grid);
        for (size_t i = 0; i < b.get_board_size(); ++i) {
            std::string s = b.tile(i);
            for (auto &c : s) c = std::toupper(c);
            board_tiles.emplace_back(s);
        }
        
        // Upper case word letters
        for (auto &c : to_check) {
            c = std::toupper(c);
        }

        if (interpret) {
            if (grs.scoring_rules->qIsQu()) {
                // Replace all occurrences of "Q" with "QU" in board tiles
                for (auto &die : dice) {
                    for (auto &face : die) {
                        size_t pos = 0;
                        while ((pos = face.find("Q", pos)) != std::string::npos) {
                            face.replace(pos, 1, "QU");
                            pos += 2;
                        }
                    }
                }
            }
            return validateDiceWord(dice, to_check);
        }
        else {
            return validateDiceBoard(dice, board_tiles);
        }
    }
    else if (dist_method == "LetterPropensity") {
        std::vector<std::string> letters = grs.letters->propensity_list;

        // Remove everything except A-Z, a-z, and "?" from letters
        for (auto &tile : letters) {
            tile.erase(remove_if(tile.begin(), tile.end(), [](const char &c) -> bool {
                    return (!(std::isalpha(c) || c == '?')); }), tile.end());
            // Uppercase letters
            for (auto &c : tile) {
                c = std::toupper(c);
            }
        }

        // Do the same for board/word
        to_check.erase(remove_if(to_check.begin(), to_check.end(), [interpret](const char &c) -> bool {
                return (!(std::isalpha(c) || (!interpret && c == '?'))); }), to_check.end());

        // Dedup letters unless SampleWithoutReplacement is set
        bool sample_without_replacement = grs.letters->sampleWithoutReplacement();
        if (!sample_without_replacement) {
            sort(letters.begin(), letters.end());
            letters.erase(unique(letters.begin(), letters.end()), letters.end());
        }

        // Get list of uppercased board letters
        std::vector<std::string> board_tiles;
        Board b(to_check, grs.grid);
        for (size_t i = 0; i < b.get_board_size(); ++i) {
            std::string s = b.tile(i);
            for (auto &c : s) c = std::toupper(c);
            board_tiles.emplace_back(s);
        }
        
        // Upper case word letters
        for (auto &c : to_check) {
            c = std::toupper(c);
        }

        if (interpret) {
            if (grs.scoring_rules->qIsQu()) {
                // Replace all occurrences of "Q" with "QU" in board tiles
                for (auto &tile : letters) {
                    size_t pos = 0;
                    while ((pos = tile.find("Q", pos)) != std::string::npos) {
                        tile.replace(pos, 1, "QU");
                        pos += 2;
                    }
                }
            }
            return validatePropensityWord(letters, to_check, 
                    sample_without_replacement);
        }
        else {
            return validatePropensityBoard(letters, board_tiles,
                    sample_without_replacement);
        }
    }
    else {
        // Unsupported game type
        debug_log("Unsupported game type\n");
        return false;
    }
}
