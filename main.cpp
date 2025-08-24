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
void autoWordle(const words &w, std::string word, const std::vector<std::string> guess_args);
void printTurn(const words &w, const WordleGame &game, const std::string &guess);
void printTurn(const words &w, const WordleGame &game, const std::string &guess, const std::string &recommendation);
void printResult(const words &w, const WordleGame &game, const std::string &guess);
inline void printMainMenu();

// -------------------------------------------------------------------------------------------------

template <typename Algo>
void runAlgorithm(const words &w, Algo algorithm, std::string firstGuess)
{
    int wins = 0, turns = 0;
    for (const auto &currentWord : w.strings)
    {
        const std::string *guess = &firstGuess;
        WordleState state;
        WordleGame game(currentWord, &state);

        while (game.turn(*guess) == Status::NEXT_TURN)
        {
            getCandidates(w, state);
            guess = &algorithm(w, state);
        }

        if (game.status == Status::WIN)
            wins++;
        turns += game.currentTurn;

        // Catches invalid firstGuess
        // Comment out for a very slight performance boost

        // if (game.status == Status::INVALID_TURN)
        //  {
        //  std::cerr << *guess << " is an invalid word!\n";
        //  exit(1);
        //  }
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
    WordleState state;
    WordleGame game(wrd, &state);

    while (game.turn(*guess) == Status::NEXT_TURN)
    {
        getCandidates(w, state);
        suggestion = &algorithm(w, state);
        printTurn(w, game, *guess, *suggestion);
        guess = suggestion;
    }

    printResult(w, game, *guess);
}

template <typename Algo>
void runAlgorithm_stepthrough(const words &w, Algo algorithm, std::string firstGuess)
{
    int wins = 0, turns = 0;
    for (const auto &currentWord : w.strings)
    {
        const std::string *guess = &firstGuess;
        const std::string *suggestion;
        WordleState state;
        WordleGame game(currentWord, &state);

        while (game.turn(*guess) == Status::NEXT_TURN)
        {
            getCandidates(w, state);
            suggestion = &algorithm(w, state);
            printTurn(w, game, *guess, *suggestion);
            guess = suggestion;
        }

        printResult(w, game, *guess);
        system("pause");

        if (game.status == Status::WIN)
            wins++;
        turns += game.currentTurn;

        // Catches invalid firstGuess
        // Comment out for a very slight performance boost

        // if (game.status == Status::INVALID_TURN)
        //  {
        //  std::cerr << *guess << " is an invalid word!\n";
        //  exit(1);
        //  }
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

    WordleState state;
    WordleGame game(word, &state);
    std::string guess;
    std::cout << "Input: ";
    std::cin >> guess;
    while (game.turn(guess) == Status::NEXT_TURN)
    {
        getCandidates(w, state);
        printTurn(w, game, guess);
        std::cout << "-------------------------------------------------------------------------------------\n";
        std::cout << "Input: ";
        std::cin >> guess;
    }
    printResult(w, game, guess);
}

void autoWordle(const words &w, std::string word, const std::vector<std::string> guess_args)
{
    WordleState state;
    WordleGame game(word, &state);

    const auto &lastWord = guess_args[guess_args.size() - 1];
    if (lastWord != word && guess_args.size() != 6)
    {
        std::cerr << "Game cannot be abanoned mid way through";
        exit(1);
    }

    for (auto &&guess : guess_args)
    {
        game.turn(guess);
        getCandidates(w, state);
        printTurn(w, game, guess);
    }

    printResult(w, game, guess_args[guess_args.size() - 1]);
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
        std::cout << "Input: "; // TODO: Flush buffer
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

void printResult(const words &w, const WordleGame &game, const std::string &guess)
{
    std::cout << "\n\n";
    std::cout << "-------------------------------------------------------------------------------------\n";
    std::cout << "                              " << game.currentTurn << "/6 - " << guess << " - ";
    switch (game.status)
    {
    case Status::WIN:
        std::cout << "Victory!";
        break;

    case Status::LOSS:
        std::cout << "Defeat!";
        break;

    default:
        std::cerr << "\n\n\nInvalid word: " << guess;
        exit(1);
    }
    std::cout << "                                 \n";
    std::cout << "-------------------------------------------------------------------------------------\n";
}

inline void printMainMenu()
{
    std::cout << "Select Mode:\n";
    std::cout << "--------------------\n";
    std::cout << "1 - Play Wordle\n";
    std::cout << "2 - Auto Wordle\n";
    std::cout << "3 - Play Unknown Wordle\n";
    std::cout << "4 - Algo Word\n";
    std::cout << "5 - Algo Dictionary\n";
    std::cout << "6 - Algo Dictionary Stepthrough\n";
    std::cout << "--------------------\n";
    std::cout << ">> ";
}

int main(int argc, char const *argv[])
{
    words w;
    loadWords(w);

    // Constants
    const std::vector<std::string> GUESS_ARRAY = {"salet", "gourd", "brunt", "fruit"};
    const auto ALGORITHM = algo_test1;
    const auto SECRET_WORD = "fruit";
    const auto FIRST_GUESS = "salet";

    printMainMenu();
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