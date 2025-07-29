#include <iostream>
#include <random>
#include "wordle-game.cpp"

template <typename Algo>
void runAlgorithm(Algo algorithm, std::string firstGuess);
void playWordle(std::string word);
void autoWordle(std::string word, std::array<std::string, 6> guess_args);

const std::string &algo_idxfirst(const words &w, const WordleState &state);
const std::string &algo_idxmiddle(const words &w, const WordleState &state);
const std::string &algo_idxlast(const words &w, const WordleState &state);
const std::string &algo_rand(const words &w, const WordleState &state);
const std::string &algo_normal(const words &w, const WordleState &state);

// -------------------------------------------------------------------------------------------------
//                                   Algorithm Implementations
// -------------------------------------------------------------------------------------------------
const std::string &algo_idxfirst(const words &w, const WordleState &state)
{
    return w.strings[state.candidates[0]];
}

const std::string &algo_idxmiddle(const words &w, const WordleState &state)
{
    return w.strings[state.candidates.size() / 2];
}

const std::string &algo_idxlast(const words &w, const WordleState &state)
{
    if (state.candidates.size() == 1)
        return w.strings[state.candidates[0]];

    return w.strings[state.candidates.size() - 1];
}

const std::string &algo_rand(const words &w, const WordleState &state)
{
    std::random_device rd;                                                // Non-deterministic random seed
    std::mt19937 gen(rd());                                               // Mersenne Twister engine
    std::uniform_int_distribution<> dist(0, state.candidates.size() - 1); // Range: 0 to N inclusive

    int random_number = dist(gen);
    return w.strings[state.candidates[random_number]];
}

const std::string &algo_normal(const words &w, const WordleState &state)
{
    if (state.candidates.size() == 1)
        return w.strings[state.candidates[0]];

    // Most common letters in valid-wordle-words.txt
    std::array<char, 26> mostUsedLetters = {'e', 'a', 'o', 'r', 'i', 'l', 't', 'n', 's', 'u', 'd', 'p', 'm', 'y', 'c', 'h', 'g', 'b', 'k', 'f', 'w', 'v', 'z', 'j', 'x', 'q'};

    // TEMP RETURN
    return w.strings[state.candidates[0]];
}

// -------------------------------------------------------------------------------------------------

template <typename Algo>
void runAlgorithm(Algo algorithm, std::string firstGuess)
{
    words w;
    loadWords(w);
    int wins = 0;
    int turns = 0;
    for (int i = 0; i < w.strings.size(); ++i)
    {
        WordleState state;
        WordleGame game(w.strings[i], &state);
        Status gameOver = game.turn(firstGuess);
        while ((int)gameOver <= 0)
        {
            ValidWords(w, state);
            /* if (state.candidates.empty())
            {
                std::cout << "ERROR AT: " << game.word << "\n";
                break;
            } */
            const std::string &bestString = algorithm(w, state);
            gameOver = game.turn(bestString);
        }

        if (gameOver == Status::WIN)
            wins++;
        turns += game.currentTurn;
    }

    double Avgwinrate = static_cast<double>(wins) / w.strings.size() * 100.0;
    double Avgturns = static_cast<double>(turns) / w.strings.size();

    std::cout << "\n";
    std::cout << "Winrate: " << Avgwinrate << " %\n";
    std::cout << "Av.turn: " << Avgturns << "\n";
    std::cout << "\n";
}

void playWordle(std::string word)
{
    words w;
    loadWords(w);

    WordleState state;
    WordleGame game1(word, &state);
    std::string guess;
    while (true)
    {
        std::cin >> guess;
        int status = static_cast<int>(game1.turn(guess));
        ValidWords(w, state);
        std::cout << "\n-------------------------------------------------------------------------------------\n";
        for (auto &str : state.candidates)
        {
            std::cout << w.strings[str] << ", ";
        }
        std::cout << "\n-------------------------------------------------------------------------------------\n";

        switch (status)
        {
        case -1:
            std::cout << "invalid" << std::endl;
            break;
        case 0:
            break;
        case 1:
            std::cout << "\n\nWINNER!\n\n"
                      << std::endl;
            return;
        case 2:
            std::cout << "\n\nFUCKING LOSER!\n\n"
                      << std::endl;
            return;
        }
    }
}

void autoWordle(std::string word, std::array<std::string, 6> guess_args)
{
    words w;
    loadWords(w);
    WordleState state;
    WordleGame game1(word, &state);

    std::array<std::string, 6> guesses = guess_args;

    for (auto &&guess : guesses)
    {
        std::cout << "-------------------------------------------------------------------------------------\n";
        std::cout << game1.currentTurn << "/6 - " << guess;
        int status = static_cast<int>(game1.turn(guess));

        switch (status)
        {
        case -1:
            std::cout << "invalid" << std::endl;
            break;
        case 0:
            break;
        case 1:
            std::cout << "\n\nWINNER!\n\n"
                      << std::endl;
            return;
        case 2:
            std::cout << "\n\nFUCKING LOSER!\n\n"
                      << std::endl;
            return;
        }

        ValidWords(w, state);
        std::cout << " - [" << state.candidates.size() << " possible words]\n\n";

        for (auto &str : state.candidates)
        {
            std::cout << w.strings[str] << ", ";
        }
        std::cout << "\n";
    }
}

int main(int argc, char const *argv[])
{
    // playWordle("puers");
    // autoWordle("tophi", {"aahed", "bhoot", "cotch", "holts", "tophi"});
    runAlgorithm(algo_idxfirst, "salet");
}