#include "blackjack_split.h"

int main()
{
    std::cout << std::setprecision(17);
    int hash_exp = 29;
    std::cerr << "\nMemory allocation exponent (29=12GB, 30=24GB, etc.): ";
    std::cin >> hash_exp;
    bool enhc = false;
    std::cerr << "\nEuropean no hole card rule (1=yes, 0=no): ";
    std::cin >> enhc;
    bool h17 = false;
    std::cerr << "\nDealer hits soft 17 (1=yes, 0=no): ";
    std::cin >> h17;
    bool das = true;
    std::cerr << "\nDouble down after split (1=yes, 0=no): ";
    std::cin >> das;
    bool doa = true;
    bool d9 = true;
    if (das)
    {
        std::cerr << "\nDouble down on any two cards (1=yes, 0=no): ";
        std::cin >> doa;
        if (!doa)
        {
            std::cerr << "\nDouble down on 9 (1=yes, 0=no): ";
            std::cin >> d9;
        }
    }
    int spl = 3;
    std::cerr << "\nMaximum number of splits (3=SPL3): ";
    std::cin >> spl;
    bool rsa = true;
    bool force_resplit = false;
    if (spl > 1)
    {
        std::cerr << "\nRe-split aces (1=yes, 0=no): ";
        std::cin >> rsa;
        std::cerr << "\nForce re-splits (1=yes, 0=no): ";
        std::cin >> force_resplit;
    }
    Game game{enhc, h17, das, doa, d9, spl, rsa, force_resplit, hash_exp};
    while (true)
    {
        std::cerr << "\nShoe (ace through ten, -1 to exit): ";
        std::vector<int> shoe = {0};
        for (int card = 1; card <= 10; ++card)
        {
            shoe.push_back(0);
            std::cin >> shoe[card];
            if (shoe[card] < 0)
            {
                return 0;
            }
        }
        int pair_card = 1;
        std::cerr << "\nPair card: ";
        std::cin >> pair_card;
        int up_card = 1;
        std::cerr << "\nDealer up card: ";
        std::cin >> up_card;

        bool count_all = false;
        std::cerr << "\nOutput count of all states (1=yes, 0=no): ";
        std::cin >> count_all;
        bool list_all = false;
        if (count_all)
        {
            std::cerr << "\nOutput list of all states (1=yes, 0=no): ";
            std::cin >> list_all;
        }

        bool count_optimal = false;
        std::cerr << "\nOutput count of optimal states (1=yes, 0=no): ";
        std::cin >> count_optimal;
        bool list_optimal = false;
        if (count_optimal)
        {
            std::cerr << "\nOutput list of optimal states (1=yes, 0=no): ";
            std::cin >> list_optimal;
        }

        std::cerr << "\nSplit " << pair_card << " vs. " << up_card << ":\n";
        double value = game.value_split(shoe, pair_card, up_card,
            count_all, list_all, count_optimal, list_optimal);
        std::cerr << "E(split)= ";
        std::cout << value << std::endl;
        std::cerr << "\n========================================\n";
    }
}
