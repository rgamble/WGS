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

#ifndef DLX_H
#define DLX_H

/* This is an implementation of the Dancing Links Algorithm (DLX) as
   described by Donald Knuth in the publication entitled "Dancing Links"
   published 15 Nov 2000
*/

#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <vector>

#define MAX_COLUMNS 200
#define MAX_ROW_BUF 2000

class DLX {
    struct element {
        element *L;  /* Pointer to left element */
        element *R;  /* Pointer to right element */
        element *U;  /* Pointer to above element */
        element *D;  /* Pointer to below element */
        element *C;  /* Pointer to column header */
        unsigned long int S;
        char *name;

        element(): name(0) {}
        ~element() { free(name); }
    };

    typedef element *ELEMENT;

    ELEMENT O[MAX_ROW_BUF];
    ELEMENT root;
    ELEMENT first;
    ELEMENT last;
    ELEMENT last_added;
    ELEMENT column_array[MAX_COLUMNS];
    size_t idx;
    unsigned long long solutions;
    bool all_solutions;

public:
    DLX() : idx(0), solutions(0) {
        //ELEMENT tmp;
        root = new element;
        root->L = root->R = root;
        first = last = 0;
        last_added = root;
    }

    ~DLX() { 
        for (size_t i = 0; i < idx; ++i) {
            ELEMENT tmp = column_array[i]->D;
            while (tmp != column_array[i]) {
                ELEMENT tmp2 = tmp->D;
                delete tmp;
                tmp = tmp2;
            }
            delete tmp;
        }
        delete root;
    }

    void addColumn(const char *name) {
        ELEMENT c = column_array[idx++] = new element;
        last_added->R = c;
        root->L = c;
        c->L = last_added;
        c->R = root;
        c->U = c->D = c;
        c->S = 0;
        c->name = (char *) malloc(strlen(name)+1);
        strcpy(c->name, name);
        last_added = c;
    }


    void addRow(std::vector<size_t> values) {
        ELEMENT tmp = root;
        first = last = 0;

        for (auto k : values) {
            tmp = column_array[k];
            ELEMENT c = new element;
            c->D = tmp;
            (tmp->S)++;
            c->L = c->R = c;
            c->C = tmp;
            c->U = tmp->U;
            tmp->U = c;
            c->U->D = c;
            if (last) {
                c->L = last;
                last->R = c;
            }
            if (first) {
                c->R = first;
                first->L = c;
            }
            if (!first) {
                first = c;
            }
            last = c;
        }
    }


    int solve(bool _all_solutions) {
        all_solutions = _all_solutions;
        solutions = 0;
        _search(0);
        return solutions;
    }


private:
    int _search (size_t k) {
//        printf("DEBUG: entering level %d\n", k);
        if (root->R == root) {
            /* print solution */
//            for (int l = 0; l < k; l++) {
//                ELEMENT h = O[l];
//                do {
//                    printf("%s ", h->C->name);
//                    h = h->R;
//                } while (h != O[l]);
//                puts("");
//            }
            solutions++;
            return 1;
        }

        /* choose column c */
        //ELEMENT c;
        ELEMENT c = root->R;
        unsigned size = (unsigned) -1;
        for (ELEMENT pick = root->R; pick != root; pick = pick->R)
            if (pick->S < size) c = pick, size = pick->S;

        /* Cover column c */
        cover(c);

        /* Other Stuff */
        for (ELEMENT r = c->D; r != c; r = r->D) {
            O[k] = r;
            for (ELEMENT j = r->R; r != j; j = j->R) {
                /* cover column j */
                cover(j->C);
            }
            _search(k + 1);
            r = O[k];
            for (ELEMENT j = r->L; j != r; j = j->L) {
                /* uncover column j */
                uncover(j->C);
            }
            if (solutions && !all_solutions) break;
        }

        /* uncover column c */
        uncover(c);
        return 0;
    }


    void cover (ELEMENT c) {
        c->R->L = c->L;
        c->L->R = c->R;
        for (ELEMENT i = c->D; i != c; i = i->D) {
            for (ELEMENT j = i->R; j != i; j = j->R) {
                j->D->U = j->U;
                j->U->D = j->D;
                (j->C->S)--;
            }
        }
    }


    void uncover (ELEMENT c) {
        for (ELEMENT i = c->U; i != c; i = i->U) {
            for (ELEMENT j = i->L; j != i; j = j->L) {
                (j->C->S)++;
                j->D->U = j;
                j->U->D = j;
            }
        }
        c->R->L = c;
        c->L->R = c;
    }

};

#endif
