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

#include <algorithm>
#include <fstream>
#include <iostream>
#include <vector>
#include <set>
#include <string>
#include <sstream>
#include <ctime>
#include <cstdlib>
#include "analyze.h"
#include "dice.h"
#include "scramble.h"
#include "wgs.h"
#include "wgs_json.h"
#include "maker.h"
#include "validate.h"

bool cmp_solutions(const Solution &p1, const Solution &p2) {
    // Compare two solutions based on higher score, then alphabetic order
    if (p1.get_score() > p2.get_score()) {
        return true;
    }
    else if (p1.get_score() < p2.get_score()) {
        return false;
    }
    else {
        return p1.get_word() < p2.get_word();
    }
}

std::string unescape_string(const std::string &s) {
    std::string result;
    bool in_escape = false;

    for (auto c : s) {
        if (in_escape) {
            switch (c) {
                case '\\':
                    c = '\\';
                    break;
                case 't':
                    c = '\t';
                    break;
                case 'n':
                    c = '\n';
                    break;
                default:
                    break;
            }
            in_escape = false;
        }
        else if (c == '\\') {
            in_escape = true;
            continue;
        }
        result.append(1, c);
    }

    return result;
}
            

void do_score_boards(const GameRuleSet &grs);
void do_solve_boards(const GameRuleSet &grs, const std::string fmt, bool solve_dups, std::string solution_prefix, std::string solution_suffix);
void do_generate_simple_boards(const GameRuleSet &grs, size_t boards);
void do_generate_boards(const GameRuleSet &grs, size_t boards, size_t min_words, size_t min_score, bool reverse_target);
void do_analyze_boards(const GameRuleSet &grs, const std::string fmt, bool dump_words);
void do_check_words(const GameRuleSet &grs, int verbosity); 
void do_check_boards(const GameRuleSet &grs, int verbosity); 
std::string analyze_solutions(const std::string fmt, const Board &b, const Solver::SolutionList &solutions);

const char *config_file = NULL;

int main(int argc, char *argv[]) {
    using std::string;
    using std::cout;
    using std::cerr;
    using std::endl;
    srand(time(NULL));

    // Valid commands:
    //
    // The score, solve, check, and analyze commands operate by reading
    // from standard input, one line at a time, until EOF is received.
    // Each line consists of letters representing a game board for the
    // score, solve, and analyze commands.  For the check commands, each
    // line represents a word or board to verify using the specified
    // game rules.  The create command generates one or more game boards
    // and prints these to standard output, one per line.
    //
    // score {game-rules}
    //      Prints the number of words and points for a given board read
    //      from standard input.
    //      Example:
    //          248 2157
    //
    // solve {game-rules} [format]
    //      Solves a game using a board read from standard input according
    //      to the specified game rules.  The provided board is not verified,
    //      a letter combination that is not possible to generate using the
    //      game rules will still be scored according to the corresponding
    //      game scoring rules.  The format specifies what information will
    //      be displayed for each solution found.  Solutions are printed in
    //      order of score and then alphabetically.  The format specifiers
    //      are:
    //      %w  The Word found.
    //      %s  The Score of the word found.  If multiple instances of the
    //          word are found, this value corresponds to the highest-scoring
    //          instance.
    //      %l  The Letter points associated with this word.  Includes letter
    //          multplier bonuses but not word multipliers or length bonuses.
    //      %m  The word Multiplier for this word.  If there are multiple
    //          word multipliers within a word, this is the product of all
    //          word multipliers.
    //      %b  The length Bonus for the found word.
    //      %p  The list of Positions that correspond to the found word.
    //          The character following the %p specifies the character to
    //          use to separate the positions.  For example, %p, will
    //          separate the positions with a comma.
    //      %(...)
    //          Causes the text inside of the parenthesis to be emitted
    //          for each solution except the last.  This is useful to
    //          separate each word by a string.  For example, %( ,) can be
    //          used to create a comma separated list of solutions.
    //          The same escapes below are allows, as is \) if it is desired
    //          to include a closing parenthesis inside of the separation
    //          string.
    //      %%  Literal percent sign.
    //
    //      In addition to the format specifiers, the following character
    //      escapes are recognized:
    //      \t  Horizontal tab
    //      \n  Newlint
    //      \\  Literal backslash
    //
    // solve-dups {game-rules} [format]
    //      Identical to solve except that duplicate solutions are reported.
    //
    // create {game-rules} [boards=1 [min-words=0 [min-points=0 [minimize]]]]
    //      Create one or more random boards.  Each board is output on
    //      separate line and is created according to the letter distribution
    //      rules associated with the specified game rules, i.e. only valid
    //      boards will be generated with this command.
    //      If either min-words or min-points is specified, the
    //      randomly generated board will be subject to a process of
    //      simulated annealing where the board undergoes various
    //      modifications in order to improve its scoring potential.  The
    //      board will be improved until the min-points and min-words
    //      criteria are met or the algorithm determines that further
    //      improvement is not likely.  Ig the minimize option is provided,
    //      the algorithm will attempt to minimize the word and score count.
    //
    // check-word {game-rules} [stats|verbose]
    //      The check-word command determines whether it is possible to spell
    //      a given word using the letter distribution associated with the
    //      specified game rules.  The entered word is echoed back with a
    //      preceding + if it can be spelled and a - if it cannot.
    //
    // analyze {game-rules} [format] [dump-words]
    //      The analyze command prints a number of data related to a board
    //      provided based on the given format string.  If the dump-words
    //      option is specified, a list of the unique words for each board
    //      is maintained and the sum or each word is printed to stderr
    //      along with the number of times each word occurred after all
    //      board have been analyzed, one entry per line.

    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " config-file command options" << endl;
        return EXIT_FAILURE;
    }

    GameConfig config;
    config_file = argv[1];
    
    if (json_read_config(config, config_file) != 0) {
        std::cerr << "Failed to read config file '" << config_file << "'" << std::endl;
        return EXIT_FAILURE;
    }

    string command = argv[2];
    if (command == "score") {
        if (argc != 4) {
            cerr << "Usage: " << argv[0] << " config-file score {game-type}" << endl;
            return EXIT_FAILURE;
        }
        string game_rules = argv[3];
        GameRuleSet grs(config, game_rules);
        do_score_boards(grs);
    }
    else if (command == "solve" || command=="solve-dups") {
        if (argc < 4 || argc > 7) {
            cerr << "Usage: " << argv[0] << " config-file solve {game-type} [format [prefix [suffix]]]" << endl;
            return EXIT_FAILURE;
        }   
        string game_rules = argv[3];
        GameRuleSet grs(config, game_rules);
        std::string fmt = grs.preferences->preference("SolutionFormat");

        if (argc >= 5) {
            fmt = argv[4];
        }

        std::string solution_prefix = grs.preferences->preference("SolutionPrefix");
        std::string solution_suffix = grs.preferences->preference("SolutionSuffix");

        if (argc >= 6) {
            solution_prefix = argv[5];
        }

        if (argc >= 7) {
            solution_suffix = argv[6];
        }

        do_solve_boards(grs, fmt, command == "solve-dups", solution_prefix, solution_suffix);
    }
    else if (command == "analyze") {
        if (argc < 4 || argc > 6) {
            cerr << "Usage: " << argv[0] << " config-file analyze {game-type} [format] [dump-words]" << endl;
            return EXIT_FAILURE;
        }   
        string game_rules = argv[3];
        bool dump_words = false;
        GameRuleSet grs(config, game_rules);

        std::string fmt = grs.preferences->preference("AnalysisFormat");
        if (argc >= 5) {
            fmt = argv[4];
        }
        if (argc >= 6) {
            dump_words = (std::string(argv[5]) == "dump-words");
        }
        do_analyze_boards(grs, fmt, dump_words);
    }
    else if (command == "create") {
        if (argc < 4 || argc > 8) {
            cerr << "Usage: " << argv[0] << " config-file create {game-type} [boards [min-words [min-score [minimize]]]]" << endl;
            return EXIT_FAILURE;
        }

        string game_rules = argv[3];
        GameRuleSet grs(config, game_rules);
        size_t boards = 1;
        size_t min_words = 0;
        size_t min_score = 0;
        bool reverse_target = false;

        if (argc >= 5) {
            boards = std::strtoul(argv[4], NULL, 10);
        }

        if (argc >= 6) {
            min_words = std::strtoul(argv[5], NULL, 10);
        }

        if (argc >= 7) {
            min_score = std::strtoul(argv[6], NULL, 10);
        }

        if (argc >= 8 && strcmp(argv[7], "minimize") == 0) {
            reverse_target = true;
        }

        do_generate_boards(grs, boards, min_words, min_score, reverse_target);
    }
    else if (command == "check-word") {
        if (argc != 4 && argc != 5) {
            cerr << "Usage: " << argv[0] << " config-file check-word {game-type} [stats|verbose]" << endl;
            return EXIT_FAILURE;
        }
        int verbosity = 0;
        if (argc == 5) {
            std::string verbose_option = argv[4];
            if (verbose_option == "stats") {
                verbosity = 1;
            }
            else if (verbose_option == "verbose") {
                verbosity = 2;
            }
            else {
                std::cerr << "Unknown verbosity option '" << verbose_option
                    << "'" << std::endl;
                return EXIT_FAILURE;
            }
        }
        string game_rules = argv[3];
        GameRuleSet grs(config, game_rules);
        do_check_words(grs, verbosity);
    }
    else if (command == "check-board") {
        if (argc != 4 && argc != 5) {
            cerr << "Usage: " << argv[0] << " config-file check-board {game-type} [stats]" << endl;
            return EXIT_FAILURE;
        }
        int verbosity = 0;
        if (argc == 5) {
            std::string verbose_option = argv[4];
            if (verbose_option == "stats") {
                verbosity = 1;
            }
            else {
                std::cerr << "Unknown verbosity option '" << verbose_option
                    << "'" << std::endl;
                return EXIT_FAILURE;
            }
        }
        string game_rules = argv[3];
        GameRuleSet grs(config, game_rules);
        do_check_boards(grs, verbosity);
    }
    else {
        cerr << "'" << command << "' is not a valid command" << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}


void do_solve_boards(const GameRuleSet &grs, const std::string fmt, bool solve_dups, std::string solution_prefix, std::string solution_suffix) {
    // Load dictionary file
    const char *dict_filename = grs.dict->dictFileName().c_str();
    std::ifstream dict_file(dict_filename);

    if (!dict_file) {
        std::cerr << "Failed to open dictionary file '" << dict_filename << "'"
            << std::endl;
        return;
    }

    std::string line;
    Solver s;
    while (dict_file >> line) {
        s.add_word(line.c_str());
    }
    dict_file.close();

    solution_prefix = unescape_string(solution_prefix);
    solution_suffix = unescape_string(solution_suffix);

    std::cout << "Enter letters (empty to quit): ";
    
    while (getline(std::cin, line)) {
        Board b(line.c_str(), grs.grid);
        s.solve(&b, *grs.scoring_rules);

        Solver::SolutionList solutions = s.get_solutions();
        sort(solutions.begin(), solutions.end());
        if (!solve_dups) {
            solutions.erase(unique(solutions.begin(), solutions.end(), equal_words), solutions.end());
        }

        // print the solutions found and the requested information
        std::cout << solution_prefix;
        for (Solver::SolutionList::const_iterator i = solutions.begin(); i != solutions.end(); i++) {
            std::cout << i->format(fmt, i != solutions.end() - 1);
        }
        std::cout << solution_suffix;
    }
}


void do_analyze_boards(const GameRuleSet &grs, const std::string fmt, bool dump_words) {
    // Load dictionary file
    const char *dict_filename = grs.dict->dictFileName().c_str();
    std::ifstream dict_file(dict_filename);

    if (!dict_file) {
        std::cerr << "Failed to open dictionary file '" << dict_filename << "'"
            << std::endl;
        return;
    }

    std::string line;
    Solver s;
    while (dict_file >> line) {
        s.add_word(line.c_str());
    }
    dict_file.close();

    std::cout << "Enter letters (empty to quit): ";

    std::map<std::string, int> word_counts;

    while (getline(std::cin, line)) {
        Board b(line.c_str(), grs.grid);
        s.solve(&b, *grs.scoring_rules);
        Solver::SolutionList solutions = s.get_solutions();
        sort(solutions.begin(), solutions.end());
        SolutionAnalysis sa(b, solutions);
        std::cout << sa.format(fmt);

        if (dump_words) {
            solutions.erase(unique(solutions.begin(), solutions.end(), equal_words), solutions.end());
            for(Solver::SolutionList::iterator i = solutions.begin(); i != solutions.end(); ++i) {
                word_counts[i->get_word()]++;
            }
        }
    }

    if (dump_words) {
        for (auto &i : word_counts) {
            std::cerr << i.first << " " << i.second << std::endl;
        }
    }
}


void do_score_boards(const GameRuleSet &grs) {
    // Load dictionary file
    const char *dict_filename = grs.dict->dictFileName().c_str();
    std::ifstream dict_file(dict_filename);

    if (!dict_file) {
        std::cerr << "Failed to open dictionary file '" << dict_filename << "'"
            << std::endl;
        return;
    }

    std::string line;
    Solver s;
    while (dict_file >> line) {
        s.add_word(line.c_str());
    }
    dict_file.close();

    std::cout << "Enter letters (empty to quit): ";

    while (getline(std::cin, line)) {
        Board b(line.c_str(), grs.grid);
        s.solve(&b, *grs.scoring_rules);
        Solver::SolutionList solutions = s.get_solutions();
        sort(solutions.begin(), solutions.end());
        solutions.erase(unique(solutions.begin(), solutions.end(), equal_words), solutions.end());

        size_t words = solutions.size();
        size_t points = 0;

        for(Solver::SolutionList::iterator i = solutions.begin(); i != solutions.end(); ++i) {
            points += i->get_score();
        }
        std::cout << words << " " << points << std::endl;
    }
}


void do_check_words(const GameRuleSet &grs, int verbosity) {
    std::string line;
    std::cout << "Enter word to check (empty to quit): ";

    Validator v;
    v.setDebug(verbosity == 2);
    while (getline(std::cin, line)) {
        int result = v.validate(grs, line, true);
        std::cout << (result ? "+" : "-") << line << " " << std::endl;
    }
    if (verbosity > 0) {
        v.printStats();
    }
}


void do_check_boards(const GameRuleSet &grs, int verbosity) {
    std::string line;
    std::cout << "Enter word to check (empty to quit): ";

    Validator v;
    v.setDebug(verbosity == 2);
    while (getline(std::cin, line)) {
        int result = v.validate(grs, line, false);
        std::cout << (result ? "+" : "-") << line << " " << std::endl;
    }
    if (verbosity > 0) {
        v.printStats();
    }
}


void do_generate_simple_boards(const GameRuleSet &grs, size_t boards) {
    for (size_t i = 0; i < boards; ++i) {
        std::cout << generate_simple_board(grs) << std::endl;
    }
}


void do_generate_boards(const GameRuleSet &grs, size_t boards, size_t min_words, size_t min_score, bool reverse_target) {
    if (min_words == 0 && min_score == 0 && !reverse_target) {
        // Don't load a dictionary if we don't have too
        return do_generate_simple_boards(grs, boards);
    }

    if (grs.letters->generationMethod() == "WordList") {
        std::cerr << "Minimum word/score board generation not supported for Word List games" << std::endl;
        return;
    }

    // Load dictionary file
    const char *dict_filename = grs.dict->dictFileName().c_str();
    std::ifstream dict_file(dict_filename);

    if (!dict_file) {
        std::cerr << "Failed to open dictionary file '" << dict_filename << "'"
            << std::endl;
        return;
    }

    std::string line;
    Solver s;
    while (dict_file >> line) {
        s.add_word(line.c_str());
    }
    dict_file.close();
    std::string fmt = "%B %W %S";

    for (size_t i = 0; i < boards; ++i) {
        std::string board = generate_board(grs, s, min_words, min_score, reverse_target);
        Board b(board, grs.grid);
        s.solve(&b, *grs.scoring_rules);
        Solver::SolutionList solutions = s.get_solutions();
        sort(solutions.begin(), solutions.end());
        SolutionAnalysis sa(b, solutions);
        std::cout << sa.format(fmt) << std::endl;
    }
} 
