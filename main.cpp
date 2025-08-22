#include "wordle-algorithms.cpp"

template <typename Algo>
void runAlgorithm(const words &w, Algo algorithm, std::string firstGuess);
template <typename Algo>
void runAlgorithm(const words &w, Algo algorithm, std::string firstGuess, std::string wrd);
template <typename Algo>
void runAlgorithm_stepthrough(const words &w, Algo algorithm, std::string firstGuess);
template <typename Algo>
void playSecretWord(const words &w, Algo algorithm);
void playSecretWord(const words &w);

void playWordle(const words &w, std::string word);
void autoWordle(const words &w, std::string word, const std::array<std::string, 6> guess_args);
void printTurn(const words &w, const WordleGame &game, const std::string &guess);
void printTurn(const words &w, const WordleGame &game, const std::string &guess, const std::string &recommendation);
void printTurn(const words &w, const WordleGame &game, const std::string &guess, const Status s);

// -------------------------------------------------------------------------------------------------

template <typename Algo>
void runAlgorithm(const words &w, Algo algorithm, std::string firstGuess)
{
    int wins = 0, turns = 0;
    for (const auto &currentWord : w.strings)
    {
        const std::string *guess = &firstGuess;
        Status status;
        WordleState state;
        WordleGame game(currentWord, &state);

        while ((status = game.turn(*guess)) == Status::NEXT_TURN)
        {
            getCandidates(w, state);
            guess = &algorithm(w, state);
        }

        switch (status)
        {
        case Status::WIN:
            wins++;
            turns += game.currentTurn;
            break;

        case Status::LOSS:
            turns += MAX_TURNS;
            break;

        default:
            std::cerr << *guess << " is an invalid word!\n";
            exit(1);
        }
    }

    double Avgwinrate = static_cast<double>(wins) / w.strings.size() * 100.0;
    double Avgturns = static_cast<double>(turns) / w.strings.size();

    std::cout << "\n";
    std::cout << "Winrate: " << Avgwinrate << " %\n";
    std::cout << "Av.turn: " << Avgturns << "\n";
    std::cout << "\n";
}

template <typename Algo>
void runAlgorithm(const words &w, Algo algorithm, std::string firstGuess, std::string wrd)
{
    const std::string *guess = &firstGuess;
    const std::string *suggestion;
    Status status;
    WordleState state;
    WordleGame game(wrd, &state);

    while ((status = game.turn(*guess)) == Status::NEXT_TURN)
    {
        getCandidates(w, state);
        suggestion = &algorithm(w, state);
        printTurn(w, game, *guess, *suggestion);
        guess = suggestion;
    }

    printTurn(w, game, *guess, status);
}

template <typename Algo>
void runAlgorithm_stepthrough(const words &w, Algo algorithm, std::string firstGuess)
{
    int wins = 0, turns = 0;
    for (const auto &currentWord : w.strings)
    {
        const std::string *guess = &firstGuess;
        const std::string *suggestion;
        Status status;
        WordleState state;
        WordleGame game(currentWord, &state);

        while ((status = game.turn(*guess)) == Status::NEXT_TURN)
        {
            getCandidates(w, state);
            suggestion = &algorithm(w, state);
            printTurn(w, game, *guess, *suggestion);
            guess = suggestion;
        }

        printTurn(w, game, *guess, status);
        system("pause");

        switch (status)
        {
        case Status::WIN:
            wins++;
            turns += game.currentTurn;
            break;

        case Status::LOSS:
            turns += MAX_TURNS;
            break;

        default:
            std::cerr << *guess << " is an invalid word!\n";
            exit(1);
        }
    }

    double Avgwinrate = static_cast<double>(wins) / w.strings.size() * 100.0;
    double Avgturns = static_cast<double>(turns) / w.strings.size();

    std::cout << "\n";
    std::cout << "Winrate: " << Avgwinrate << " %\n";
    std::cout << "Av.turn: " << Avgturns << "\n";
    std::cout << "\n";
}

void playWordle(const words &w, std::string word)
{
    if (word.length() == 0)
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(0, 14800);
        int rand = dist(gen);

        // Randomize word
        word = w.strings[rand];
    }

    Status status;
    WordleState state;
    WordleGame game(word, &state);
    std::string guess;
    std::cin >> guess;
    while ((status = game.turn(guess)) == Status::NEXT_TURN)
    {
        getCandidates(w, state);
        printTurn(w, game, guess);

        std::cout << "-------------------------------------------------------------------------------------\n";
        std::cin >> guess;
    }
    printTurn(w, game, guess, status);
}

void autoWordle(const words &w, std::string word, const std::array<std::string, 6> guess_args)
{
    Status status;
    WordleState state;
    WordleGame game(word, &state);

    for (auto &&guess : guess_args)
    {
        status = game.turn(guess);
        getCandidates(w, state);
        printTurn(w, game, guess);
    }
}

template <typename Algo>
void playSecretWord(const words &w, Algo algorithm)
{
    WordleState state;
    WordleGame game("dummy", &state);
    std::string input, guess, result;
    int turn = 1;
    while (turn <= MAX_TURNS)
    {
        if (turn > 1 && state.candidates.size() == 1)
            break;

        // Get input
        std::getline(std::cin, input);
        if (input.length() != WORD_LEN * 2 + 1)
            continue;

        guess = input.substr(0, WORD_LEN);
        result = input.substr(WORD_LEN + 1, WORD_LEN);

        // Input guess
        for (int i = 0; i < WORD_LEN; i++)
        {
            char c = result[i];

            switch (c)
            {
            case 'G':
            case 'g':
                state.green[i] = guess[i];
                state.set_requireMask(i, guess[i] - 'a');
                state.maxSameChar[guess[i] - 'a']++;
                break;

            case 'Y':
            case 'y':
                state.set_yellow(i, guess[i] - 'a');
                state.set_requireMask(i, guess[i] - 'a');
                state.maxSameChar[guess[i] - 'a']++;
                break;

            case 'X':
            case 'x':
                state.set_grey(guess[i] - 'a');
                break;

            default:
                std::cerr << "followup error: " << c << std::endl;
                break;
            }
        }

        game.currentTurn = ++turn;
        getCandidates(w, state);
        result = algorithm(w, state);
        printTurn(w, game, guess, result);
    }
}

void printTurn(const words &w, const WordleGame &game, const std::string &guess)
{
    auto &candidates = game.state->candidates;

    std::cout << "-------------------------------------------------------------------------------------\n";
    std::cout << game.currentTurn - 1 << "/6 - " << guess << " [" << candidates.size() << "]\n\n";

    // At least one element, print it
    std::cout << w.strings[candidates[0]];
    if (candidates.size() == 1)
    {
        std::cout << "\n";
        return;
    }

    // Print the rest
    for (int i = 1; i < candidates.size(); i++)
    {
        std::cout << ", ";
        std::cout << w.strings[candidates[i]];
    }

    std::cout << "\n";
}

void printTurn(const words &w, const WordleGame &game, const std::string &guess, const std::string &recommendation)
{
    printTurn(w, game, guess);

    std::cout << "-------------------------------------------------------------------------------------\n";
    std::cout << "Algorithm suggestion: " << recommendation << "\n";
}

void printTurn(const words &w, const WordleGame &game, const std::string &guess, const Status s)
{
    std::cout << "\n\n";
    std::cout << "-------------------------------------------------------------------------------------\n";
    std::cout << "                              " << game.currentTurn - 1 << "/6 - " << guess << " - ";
    switch (s)
    {
        case Status::WIN:
        std::cout << "Victory!";
        break;
        
        case Status::LOSS:
        std::cout << "Defeat!";
        break;
        
        default:
        std::cerr << guess << " is invalid\n";
        exit(1);
    }
    std::cout << "                                 \n";
    std::cout << "-------------------------------------------------------------------------------------\n";
}

int main(int argc, char const *argv[])
{
    words w;
    loadWords(w);

    // Constants
    const std::array<std::string, 6> GUESS_ARRAY = {"salet", "umiaq", "adorb", "graph", "crank"};
    const auto ALGORITHM = algo_entropy;
    const auto SECRET_WORD = "extol";
    const auto FIRST_GUESS = "tares";
    
    int input;
    std::cin >> input;
    switch (input)
    {
    case 1:
        // "" - Random Word
        playWordle(w, SECRET_WORD);
        break;
    case 2:
        // Find word with an array of guesses
        autoWordle(w, SECRET_WORD, GUESS_ARRAY);
        break;
    case 3:
        /*
            G|g - Green
            Y|g - Yellow
            X|x - Grey

            GUESS|SPACE|RESULT
            ------------------
            words yxgyy
            SALET>GGGGX
            MilKY|xyXxG
        */
        playSecretWord(w, ALGORITHM);
        break;
    case 4:
        // Solve WORD with algorithm and starting word
        runAlgorithm(w, ALGORITHM, FIRST_GUESS, SECRET_WORD);
        break;
    case 5:
        // Solve DICTIONARY with algorithm and starting word
        runAlgorithm(w, ALGORITHM, FIRST_GUESS);
        break;
    case 6:
        // Solve DICTIONARY, but printed one word at a time
        runAlgorithm_stepthrough(w, ALGORITHM, FIRST_GUESS);
        break;
    }
}