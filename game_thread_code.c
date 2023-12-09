#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <time.h>

#define MAX_PLAYERS 4
#define MAX_ROUNDS 3

int mapSize;
int smallest[MAX_PLAYERS];
int numberOfPlayers;
int currentRound = 1;

typedef struct Point{
    int x;
    int y;
} Point;

typedef struct{
    int id;
    Point position;
} Player;

typedef struct{
    int mapSize;
    int numberOfPlayers;
    Player players[MAX_PLAYERS];
    pthread_mutex_t mutex;
} GameData;

// Keeps minimum distances of previous turns of each player.
int previousDistances[MAX_PLAYERS][MAX_ROUNDS - 1]; 
// Keeps previous guesses of each player.
Point previousGuesses[MAX_PLAYERS][MAX_ROUNDS - 1]; 

void initializeGame(int mapSize, int numberOfPlayers, GameData *game){
    game->mapSize = mapSize;
    game->numberOfPlayers = numberOfPlayers;
    numberOfPlayers = numberOfPlayers;
    pthread_mutex_init(&game->mutex, NULL);

    // For randomizing
    srand(time(NULL));

    // Initialize random positions to the players.
    for(int i = 0; i < numberOfPlayers; ++i){
        game->players[i].id = i + 1;
        game->players[i].position.x = rand() % mapSize;
        game->players[i].position.y = rand() % mapSize;
        smallest[i] = mapSize*mapSize;
    }
}

int calculateDistance(Point p1, Point p2){
    return abs(p1.x - p2.x) + abs(p1.y - p2.y);
}

// If turn 1, make random guess.
// If turn 2, make guess that has the same distance as the distance between 1. turn guess 
//    and minimum distance.
// If turn 3, make guess that has the same distance as the distance between 1. turn guess 
//    and minimum distance of turn 1 + the distance between 2. turn guess 
//    and minimum distance of turn 2 or the distance between 2. turn guess 
//    and minimum distance of turn 2.
// In turn 3 we consider closest players can differ round by round. 
Point guess(void* game, int playerID, Point previousGuesses[4][2], int previousDistances[4][2]){
    Point guess;
    if(currentRound == 1){
        guess.x = rand() % mapSize;
        guess.y = rand() % mapSize;
    } 
    else if(currentRound == 2){
        for(int i = 0; i < mapSize; i++){
            for(int j = 0; j < mapSize; j++){
                Point newPoint = {i,j};
                if(calculateDistance(newPoint, previousGuesses[playerID][0]) == previousDistances[playerID][0]){
                    guess.x = i;
                    guess.y = j;
                }                      
            }
        }
    } 
    else{
        for(int i = 0; i < mapSize; i++){
            for(int j = 0; j < mapSize; j++){
                Point newPoint = {i,j};
                if(calculateDistance(newPoint, previousGuesses[playerID][0]) == previousDistances[playerID][0] && calculateDistance(newPoint, previousGuesses[playerID][1]) == previousDistances[playerID][1]){
                    guess.x = i;
                    guess.y = j;
                } 
                else if(calculateDistance(newPoint, previousGuesses[playerID][1]) == previousDistances[playerID][1]){
                    guess.x = i;
                    guess.y = j;
                }                   
            }
        }
    }

    return guess;
}

void* playRound(void* arg){
    GameData* game = (GameData*)arg;
    int roundDistances[MAX_PLAYERS];
    int x,y;

    pthread_mutex_lock(&game->mutex);

    // 1) Makes a guess for each player 
    // 2) Calculates the distances of each player to the guess point.
    // 3) Prints results and keep minimum distances for the next turns.
    for(int i = 0; i < numberOfPlayers; ++i){
        Point playerGuess = guess((void*)&game, i, previousGuesses, previousDistances);

        // If turn is 1 or 2, guesses are saved. On turn 3, no guesses are saved.
        if(currentRound < 3){
            previousGuesses[i][currentRound - 1] = playerGuess;
        }

        int minimumOfTheTurn = mapSize*2;
        printf("%d.guess of player%d: [%d,%d]\n", currentRound, i + 1, playerGuess.x, playerGuess.y);

        for(int j = 0; j < numberOfPlayers; ++j){
            if (i != j) {
                roundDistances[j] = calculateDistance(playerGuess, game->players[j].position);
                if(roundDistances[j] < minimumOfTheTurn){
                    minimumOfTheTurn = roundDistances[j];
                }
                if(currentRound < 3){
                    previousGuesses[i][currentRound - 1] = playerGuess;
                }
                if(roundDistances[j] < smallest[i]){
                    smallest[i] = roundDistances[j];
                }
                printf("the distance with player%d: %d\n", j + 1, roundDistances[j]);
                if(roundDistances[j] == 0){
                    printf("******************************************\nplayer%d won the game!!!\n******************************************\n",i+1);
                    exit(0);
                }
            }
        }
        if (currentRound < 3) { // Guesses of the next turns based on this distance.
            previousDistances[i][currentRound - 1] = minimumOfTheTurn;
        }
        printf("\n");
    }
    pthread_mutex_unlock(&game->mutex);
    return NULL;
}

void print_map(GameData game) { // Prints the map and positions of the players .
    for(int i = 0; i < mapSize + 2; i++){
        for(int j = 0; j < mapSize + 2; j++){
            if(i == 0 || i == mapSize + 1){
                printf("- ");
            } 
            else if(j == 0){
                printf("| ");
            } 
            else if(j == mapSize + 1){
                printf("|");
            } 
            else if(numberOfPlayers >= 1 && i - 1 == game.players[0].position.x && j - 1 == game.players[0].position.y){
                printf("1 ");
            } 
            else if(numberOfPlayers >= 2 && i - 1 == game.players[1].position.x && j - 1 == game.players[1].position.y){
                printf("2 ");
            } 
            else if(numberOfPlayers >= 3 && i - 1 == game.players[2].position.x && j - 1 == game.players[2].position.y){
                printf("3 ");
            } 
            else if(numberOfPlayers >= 4 && i - 1 == game.players[3].position.x && j - 1 == game.players[3].position.y){
                printf("4 ");
            } 
            else{
                printf("  ");
            }
        } 
        printf("\n");
    }
}

int main(int argc, char *argv[]){
    if(argc != 3){
        fprintf(stderr, "Run with only 2 argument.\n");
        return 1;
    }
  
    mapSize = atoi(argv[1]);
    numberOfPlayers = atoi(argv[2]);

    if(mapSize <= 0 || numberOfPlayers <= 1 || numberOfPlayers > MAX_PLAYERS){
        fprintf(stderr, "Invalid input parameters.\nThere should be maximum %d players.\n", MAX_PLAYERS);
        return 1;
    }

    printf("%dx%d map is created\n", mapSize, mapSize);
    printf("%d threads are created\n", numberOfPlayers);

    srand(time(NULL));

    GameData game;

    Point undefinedPoint = {-1, -1};
    int undefinedDistance = -1;

    //Assaign undefinied check points to previous guess/distance data.
    for(int i = 0; i < 4; i++){
        for(int j = 0; j < 2; j++){
            previousGuesses[i][j] = undefinedPoint;
            previousDistances[i][j] = undefinedDistance;
        }
    }

    initializeGame(mapSize, numberOfPlayers, &game);

    //Prints the positions of players.
    printf("Coordinates of the players are chosen randomly\n");
    for(int i = 0; i < numberOfPlayers; ++i){
        printf("player%d: [%d,%d] ", i + 1, game.players[i].position.x, game.players[i].position.y);
    } 
    
    printf("\n");
    print_map(game);

    printf("\nGame launches -->\n\n");

    pthread_t threads[MAX_PLAYERS];

    //Play multiple rounds.
    for(int round = 1; round <= MAX_ROUNDS; ++round){
        printf("---------- Round-%d ----------\n", round);
        pthread_create(&threads[round-1], NULL, playRound, (void*)&game);
        pthread_join(threads[round-1], NULL);
        currentRound++;
    }

    //Find the minimum distance.
    int minimum = smallest[0];
    for(int i = 0; i < numberOfPlayers; i++){
        if(smallest[i] < minimum)
            minimum = smallest[i];
    }

    printf("The game ends!\nThe winner(s) with the closest guess of %d-distance:\n",minimum);
    
    for(int i = 0; i < numberOfPlayers;i++){
        if(minimum == smallest[i])
            printf("player%d ",i + 1);
    }

    printf("\n");
    return 0;
}
