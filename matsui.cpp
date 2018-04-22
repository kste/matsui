#include <stdio.h>
#include <inttypes.h>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iterator>
#include <iomanip>

#include "lookup_skinny.h"

#define MAX_ROUNDS 40

static int BN = 20;
std::vector<uint8_t> current_perm;

int active_sboxes(uint8_t state[16]) {
    int i, active;
    for(i = 0; i < 16; i++) {
        active += state[i];
    }
    return active;
}

void matsui_algorithm1(int *bound, int iteration, int n, int cur_active, uint8_t diff[16]) {
    uint8_t tmp[16], outdiff[16];
    int i;

    // Apply permutation
    for(i = 0; i < 16; i++) {
       tmp[i] = diff[current_perm[i]];
    }

    // Loop over all possible mixcolumn transitions for each column
    uint8_t col0 = tmp[0] << 3 | tmp[1] << 2 | tmp[2] << 1 | tmp[3];
    uint8_t col1 = tmp[4] << 3 | tmp[5] << 2 | tmp[6] << 1 | tmp[7];
    uint8_t col2 = tmp[8] << 3 | tmp[9] << 2 | tmp[10] << 1 | tmp[11];
    uint8_t col3 = tmp[12] << 3 | tmp[13] << 2 | tmp[14] << 1 | tmp[15];

    for(auto const& col0_outdiff: mixcolumn_lookup[col0]) {
        for(auto const& col1_outdiff: mixcolumn_lookup[col1]) {
            for(auto const& col2_outdiff: mixcolumn_lookup[col2]) {
                for(auto const& col3_outdiff: mixcolumn_lookup[col3]) {
                    for(i = 0; i < 4; i++) {
                        outdiff[i] = col0_outdiff[i];
                    }
                    for(i = 0; i < 4; i++) {
                        outdiff[i + 4] = col1_outdiff[i];
                    }
                    for(i = 0; i < 4; i++) {
                        outdiff[i + 8] = col2_outdiff[i];
                    }
                    for(i = 0; i < 4; i++) {
                        outdiff[i + 12] = col3_outdiff[i];
                    }
                    if ((cur_active + bound[n - iteration -1 ]) < BN) {
                        // continue
                        if (iteration == (n - 1)) {
                            // Reached the end and we found a new best bound
                            bound[n] = cur_active;
                            BN = bound[n];
                        } else {
                            matsui_algorithm1(bound, iteration + 1, n, cur_active + active_sboxes(outdiff), outdiff);
                        }
                    } else {
                        return;
                    }
                }
            }
        }
    }
}

int main(int argc, char* argv[]) {
    // Read all permutations
    std::ifstream file("permutations.txt");
    std::string line;
    std::vector< std::vector<uint8_t> > all_permutations;
    while(std::getline(file, line)) {
        std::istringstream is(line);
        std::vector<uint8_t> temp;
        int n;
        while(is >> n) {
            temp.push_back((uint8_t)n);
        }
        all_permutations.push_back(temp);
    }

    for(int perm = 0; perm < all_permutations.size(); perm++) {
        current_perm = all_permutations[perm];


        // Print permutation
        int k;
        std::cout << "[";
        for(k = 0; k < 15; k++) {
            std::cout << (int)current_perm[k] << " ";
        }
        std::cout << (int)current_perm[15] << "] ";

        // Find bounds
        uint8_t state[16];
        int bound[MAX_ROUNDS + 1];
        int i, j, r;

        for(i = 0; i <= MAX_ROUNDS; i++) {
           bound[i] = 0;
        }

        bound[1] = 1; // 1 round
        bound[2] = 2; // 2 rounds

        for(r = 3; r <= MAX_ROUNDS; r++) {

            // Guess BN
            BN = 6*r;
            // Start First round and loop over all possible differences
            uint8_t X0[16];
            for(i = 0; i < 0xFFFF; i++) {
                uint8_t diff[16];
                for(j = 0; j < 16; j++) {
                    diff[j] = poss_differences[i][j];
                }
                matsui_algorithm1(bound, 0, r, active_sboxes(diff), diff);
            }
        }

        std::cout << "Bounds:";
        // Print bounds
        for(i = 1; i <= MAX_ROUNDS; i++) {
            std::cout << " " << bound[i];
        }
        std::cout << std::endl;
    }
    return 0;
}
