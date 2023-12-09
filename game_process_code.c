#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>

int map_size;
int max_rounds = 3;

// Keeps the previous guesses of the players.
// Initialized to -1 for checking to current round.
int player1_previous_guesses[4] = {-1, -1, -1, -1};
int player2_previous_guesses[4] = {-1, -1, -1, -1};

// Keeps the distances between previous guesses to other player.
// Initialized to -1 for checking to current round.
int player1_previous_distances[2] = {-1, -1};
int player2_previous_distances[2] = {-1, -1};

// Keeps x and y values of the guesses.
int guess1[2];
int guess2[2];
int oldguess[2];

// Calculate the manhattan distance between two points
int manhattan_distance(int x1, int y1, int x2, int y2){
    return abs(x1 - x2) + abs(y1 - y2);
}

// If turn 1, make random guess.
// If turn 2, make guess that has the same distance as the distance of 1. turn and the guess of 1. turn.
// If turn 3, make guess that has the same distance as the distance of 1. turn and the guess of 1. turn
// and has the same distance as the distance of 2. turn and the guess of 2. turn.
void guess(int previous_guesses[], int previous_distances[], int guess[]){
    if(previous_guesses[0] == -1){ // Round 1
        guess[0] = rand() % map_size;
        guess[1] = rand() % map_size;
        return;
    } 
    else{
        for(int i = 0; i < map_size; i++){
            for(int j = 0; j < map_size; j++){
                if(previous_guesses[2] == -1){ // Round 2
                    if(manhattan_distance(i, j, previous_guesses[0], previous_guesses[1]) == previous_distances[0]){
                        guess[0] = i;
                        guess[1] = j;
                    }                       
                } 
                else{ // Round 3
                    if(manhattan_distance(i, j, previous_guesses[0], previous_guesses[1]) == previous_distances[0] && manhattan_distance(i, j, previous_guesses[2], previous_guesses[3]) == previous_distances[1]){
                        guess[0] = i;
                        guess[1] = j;
                    } 
                }
            }
        }
    }
}

void print_map(int x1, int y1, int x2, int y2){
    for(int i = 0; i < map_size + 2; i++){
        for(int j = 0; j < map_size + 2; j++){
            if(i == 0 || i == map_size + 1){
                printf("- ");
            } 
            else if (j == 0){
                printf("| ");
            } 
            else if (j == map_size + 1){
                printf("|");
            } 
            else if (i - 1 == x1 && j - 1 == y1){
                printf("1 ");
            } 
            else if (i - 1 == x2 && j - 1 == y2){
                printf("2 ");
            } 
            else{
                printf("  ");
            }
        } 
        printf("\n");
    }
}

int main(int argc, char *argv[]){

    srand(time(NULL));

    if(argc != 2){
        fprintf(stderr, "Argument structure is wrong. Run with 1 argument.\n");
        return 1;
    }

    map_size = atoi(argv[1]);

    printf("%dx%d map is created\n", map_size, map_size);

    // Randomly creating players points.
    int player1_x = rand() % map_size;
    int player1_y = rand() % map_size;

    int player2_x = rand() % map_size;
    int player2_y = rand() % map_size;

    printf("Coordinates of the players are chosen randomly\n");
    printf("player1: [%d,%d] , player2: [%d,%d]\n", player1_x, player1_y, player2_x, player2_y);

    print_map(player1_x, player1_y, player2_x, player2_y);
    printf("Game launches -->\n");

    // Create pipes for communication between parent and child.
    int pipe_fd_parent_to_child[2];
    int pipe_fd_child_to_parent[2];

    if(pipe(pipe_fd_parent_to_child) == -1 || pipe(pipe_fd_child_to_parent) == -1){
        perror("Pipe has failed.");
        exit(1);
    }

    int smallest1 = map_size + map_size; // Smallest guess of player1.
    int smallest2 = map_size + map_size; // Smallest guess of player2.

    // Creates a child process.
    pid_t child_pid = fork();

    if(child_pid == -1){
        perror("Fork has failed.");
        exit(1);
    }

    if(child_pid > 0){  // Parent process
        srand(child_pid*child_pid);  
        close(pipe_fd_parent_to_child[0]);
        close(pipe_fd_child_to_parent[1]);

        printf("A child process created\n");

        // Game loop
        for(int round = 1; round <= max_rounds; ++round){
            printf("---------- Round-%d ----------\n", round);

            // Player 1's turn
            guess(player1_previous_guesses, player1_previous_distances, guess1);
            int guess1_x = guess1[0];
            int guess1_y = guess1[1];
            for(int i = 0; i < 4; i++){
                if(player1_previous_guesses[i] == -1){
                    player1_previous_guesses[i] = guess1_x;
                    player1_previous_guesses[i+1] = guess1_y;
                    break;
                }
            }
            printf("%d.guess of player1: [%d,%d]\n", round, guess1_x, guess1_y);

            // Calculate manhattan distance to player 2 and send it to the child process.
            int distance1 = manhattan_distance(guess1_x, guess1_y, player2_x, player2_y);
            for(int i = 0; i < 3; i++){
                if(player1_previous_distances[i] == -1){
                    player1_previous_distances[i] = distance1;
                    break;
                }
            }
            printf("the distance with player2: %d\n", distance1);
            if(distance1 == 0){
                printf("******************************************\nplayer1 won the game!!!\n******************************************\n");
                return 0;
            }

            write(pipe_fd_parent_to_child[1], &guess1_x, sizeof(int));
            write(pipe_fd_parent_to_child[1], &guess1_y, sizeof(int));

            if(distance1 < smallest1)
                smallest1 = distance1;

            // Player 2's turn
            int guess2_x, guess2_y;
            read(pipe_fd_child_to_parent[0], &guess2_x, sizeof(int));
            read(pipe_fd_child_to_parent[0], &guess2_y, sizeof(int));
            while(oldguess[0] == guess2_x && oldguess[1] == guess2_y){
                read(pipe_fd_child_to_parent[0], &guess2_x, sizeof(int));
                read(pipe_fd_child_to_parent[0], &guess2_y, sizeof(int));
            }

            printf("%d.guess of player2: [%d,%d]\n", round, guess2_x, guess2_y);
            oldguess[0] = guess2_x;
            oldguess[1] = guess2_y;

            // Calculate manhattan distance to player 1 and send it to the parent process.
            int distance2 = manhattan_distance(guess2_x, guess2_y, player1_x, player1_y);
            printf("the distance with player1: %d\n", distance2);
            if(distance2 == 0){
                printf("******************************************\nplayer2 won the game!!!\n******************************************\n");
                return 0;
            }
            if(distance2 < smallest2)
                smallest2 = distance2;
            printf("\n");
        }

        close(pipe_fd_parent_to_child[1]);
        close(pipe_fd_child_to_parent[0]);

        // Wait for the child to finish
        wait(NULL);

        if(smallest1 != 0 && smallest2 != 0){
            printf("The game ends!\n");
            printf("The winner with the closest guess of ");
            if(smallest1 < smallest2){
                printf("%d-distance: \n",smallest1);
                printf("player1\n");
            } 
            else if(smallest1 == smallest2){
                printf("%d-distance: \n",smallest1);
                printf("player1, player2\n");
            }
            else{
                printf("%d-distance: \n",smallest2);
                printf("player2\n");
            }
        }
    } 
    else{  // Child process
        close(pipe_fd_parent_to_child[1]);
        close(pipe_fd_child_to_parent[0]);
        // Game loop
        for(int round = 1; round <= max_rounds; round++){
            // Player 2's turn
            guess(player2_previous_guesses, player2_previous_distances, guess2);
            int guess2_x = guess2[0];
            int guess2_y = guess2[1];
            for (int i = 0; i < 4; i++) {
                if (player2_previous_guesses[i] == -1) {
                    player2_previous_guesses[i] = guess2_x;
                    player2_previous_guesses[i+1] = guess2_y;
                    break;
                }
            }

            write(pipe_fd_child_to_parent[1], &guess2_x, sizeof(int));
            write(pipe_fd_child_to_parent[1], &guess2_y, sizeof(int));

            // Player 1's turn
            int guess1_x, guess1_y;
            read(pipe_fd_parent_to_child[0], &guess1_x, sizeof(int));
            read(pipe_fd_parent_to_child[0], &guess1_y, sizeof(int));

            int distance2 = manhattan_distance(guess2_x, guess2_y, player1_x, player1_y);
            for(int i = 0; i < 3; i++){
                if (player2_previous_distances[i] == -1){
                    player2_previous_distances[i] = distance2;
                    break;
                }
            }

            // Check if player 1 guessed correctly
            if(guess1_x == player2_x && guess1_y == player2_y){
                close(pipe_fd_parent_to_child[0]);
                close(pipe_fd_child_to_parent[1]);
                exit(0);
            }

            // Check if player 2 guessed correctly
            else if(guess2_x == player1_x && guess2_y == player1_y){
                close(pipe_fd_parent_to_child[0]);
                close(pipe_fd_child_to_parent[1]);
                exit(0);
            }
        }

        close(pipe_fd_parent_to_child[0]);
        close(pipe_fd_child_to_parent[1]);  
        exit(0);
    }

    return 0;
}
