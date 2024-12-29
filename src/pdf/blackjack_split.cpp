// Optimal strategy maximizing expected value of splitting pairs in blackjack
//
// NOTE: For depleted shoes where it is possible to run out of cards, assume
// player avoids drawing from empty shoe (i.e., value of hitting or doubling is
// -infinity), and assume dealer busts when drawing from empty shoe.

#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <queue>
#include <stack>
#include <algorithm>
#include <limits>
#include <iostream>
#include <iomanip>
#include <ctime>

// A game state records a decision point during a round:
//   cards[0] = # cards in current hand (0 = empty hash slot, 3 = 3+ cards)
//   cards[1:] = # cards of each rank dealt in the round (including up card)
//   hands[0:current] = sorted set of completed hand totals (<0 = doubled)
//   hands[current] = current hand total (<0 = soft, 0 = completed round)
struct State
{
    std::int8_t cards[11];
    std::int8_t hands[4];
    std::int8_t current;

    bool operator==(const State& rhs) const
    {
        return std::memcmp(this, &rhs, sizeof(State)) == 0;
    }
};

// Store all possible game states using MSI hash map (ref. Chris Wellons
// https://nullprogram.com/blog/2022/08/08/) from state to corresponding
// expected value.
struct Solved
{
    State state;
    union
    {
        // Group states at same depth in decision tree in a linked list, to be
        // evaluated in decreasing order of depth.
        Solved* next;
        double value;
    };
};

struct Game
{
    // Define rule variations:
    Game(bool enhc, bool h17, bool das, bool doa, bool d9, int spl, bool rsa,
        int hash_exp) :
        HASH_EXP{hash_exp}, // allocate 24 * 2^hash_exp bytes for hash map
        HASH_MASK{(1ull << HASH_EXP) - 1},
        hash_map(HASH_MASK + 1),
        ENHC{enhc}, // European no hole card rule
        H17{h17},   // dealer hits soft 17
        DAS{das},   // double down after split
        DOA{doa},   // double down on any two cards (only valid if DAS)
        D9{d9},     // double down on 9 (only valid if !DOA)
        SPL{spl},   // maximum number of splits
        RSA{rsa},   // resplit aces
        shoe{},
        pair_half{0},
        pair_hand{0},
        up_card{0}
    {
        // empty
    }

    // Return expected value of splitting pair vs. dealer up card, from shoe
    // with shoe[rank] cards of each rank 1..10 (including initial pair cards
    // and up card).
    double value_split(const std::vector<int>& shoe,
        int pair_card, int up_card)
    {
        std::clock_t tic = std::clock();
        this->shoe = shoe;
        pair_half = deal(0, pair_card);
        pair_hand = deal(pair_half, pair_card);
        this->up_card = up_card;
        State s0{};
        s0.cards[0] = 1;
        s0.cards[pair_card] += 2;
        s0.cards[up_card] += 1;
        s0.hands[0] = s0.hands[1] = pair_half;
        solve(s0);
        std::cerr << "    Elapsed time " <<
            double(std::clock() - tic) / CLOCKS_PER_SEC << " seconds.\n";
        return lookup(s0)->value;
    }

    // Breadth-first search all states from initial split.
    std::size_t search(const State& s0, std::stack<Solved*>& depths)
    {
        std::fill(hash_map.begin(), hash_map.end(), Solved{});
        Solved* solved = lookup(s0);
        solved->state = s0;
        solved->next = 0;
        std::queue<Solved*> queue;
        queue.push(solved);
        depths.push(solved);
        Solved* head = 0;
        std::size_t num_at_depth = queue.size();
        std::size_t num_states = queue.size();
        while (!queue.empty())
        {
            solved = queue.front();
            queue.pop();
            for (const auto& s : get_moves(solved->state))
            {
                solved = lookup(s);
                if (solved->state.cards[0] == 0)
                {
                    solved->state = s;
                    solved->next = head;
                    head = solved;
                    queue.push(solved);
                    if (((++num_states) & 0xfffff) == 0)
                    {
                        std::cerr << "\r    Searched " << num_states <<
                            " states to depth " << depths.size() << ", " <<
                            num_states * 100 / (HASH_MASK + 1) <<
                            "% capacity...";
                    }
                }
            }
            if (--num_at_depth == 0)
            {
                depths.push(head);
                head = 0;
                num_at_depth = queue.size();
            }
        }
        std::cerr << "\r    Searched " << num_states << " states to depth " <<
            depths.size() - 1 << ", " << num_states * 100 / (HASH_MASK + 1) <<
            "% capacity.  \n";
        return num_states;
    }

    // Work "upward" from states deepest in decision tree, computing expected
    // value either from dealer probabilities for leaf states, or in terms of
    // standing/hitting/etc.
    void solve(const State& s0)
    {
        std::stack<Solved*> depths;
        std::size_t num_states = search(s0, depths);
        std::size_t num_solved = 0;
        for (; !depths.empty(); depths.pop())
        {
            for (Solved* solved = depths.top(); solved != 0;)
            {
                Solved* next = solved->next;
                solved->value = get_value(solved->state);
                solved = next;
                if (((++num_solved) & 0xfffff) == 0)
                {
                    std::cerr << "\r    Solving depth " << depths.size() - 1 <<
                        ", " << double(num_solved * 1000 / num_states) / 10 <<
                        "% complete...    ";
                }
            }
        }
        std::cerr << "\r    Solving depth 0, 100% complete.     \n";
    }

    // Return possible next states from standing/hitting/etc.
    std::vector<State> get_moves(const State& s)
    {
        std::vector<State> moves;
        std::int8_t hand = s.hands[s.current];
        if (s.current == 4 || hand == 0)
        {
            return moves;
        }
        if (s.cards[0] >= 2)
        {
            moves.push_back(stand(s));
        }
        if (s.cards[0] < 2 || pair_half != -11)
        {
            hit(moves, s);
        }
        if (s.cards[0] == 2)
        {
            if (DAS && (DOA || hand == 10 || hand == 11 || (D9 && hand == 9))
                    && pair_half != 11)
            {
                hit(moves, s, true);
            }
            if (hand == pair_hand && s.hands[SPL] == 0 &&
                (RSA || pair_half != -11))
            {
                moves.push_back(split(s));
            }
        }
        return moves;
    }

    // Return expected value of given state using optimal strategy.
    double get_value(const State& s)
    {
        std::int8_t hand = s.hands[s.current];
        if (s.current == 4 || hand == 0)
        {
            return value_round(s);
        }
        double value = s.cards[0] >= 2 ? lookup(stand(s))->value :
            -std::numeric_limits<double>::infinity();
        if (s.cards[0] < 2 || pair_half != -11)
        {
            value = std::max(value, value_hit(s));
        }
        if (s.cards[0] == 2)
        {
            if (DAS && (DOA || hand == 10 || hand == 11 || (D9 && hand == 9))
                && pair_half != 11)
            {
                value = std::max(value, value_hit(s, true));
            }
            if (hand == pair_hand && s.hands[SPL] == 0 &&
                (RSA || pair_half != -11))
            {
                value = std::max(value, lookup(split(s))->value);
            }
        }
        return value;
    }

    // Return new hand total after dealing card (negative total is soft).
    std::int8_t deal(int hand, int card)
    {
        bool soft = hand < 0;
        hand = std::abs(hand) + card;
        if (card == 1 && hand < 12)
        {
            return -(hand + 10);
        }
        else if (hand > 21 && soft)
        {
            return hand - 10;
        }
        return soft ? -hand : std::min(hand, 22);
    }

    // Return new state from standing on current hand (for completed hands,
    // negative total indicates doubled wager).
    State stand(State s, bool doubled = false)
    {
        s.cards[0] = 1;
        std::int8_t& hand = s.hands[s.current];
        hand = std::max(std::abs(hand), 16);
        if (doubled)
        {
            hand = -hand;
        }
        std::sort(&s.hands[0], &s.hands[++s.current]);
        return s;
    }

    // Extend list of moves with all possible drawn cards to current hand.
    void hit(std::vector<State>& moves, State s, bool doubled = false)
    {
        s.cards[0] = std::min(s.cards[0] + 1, 3);
        for (int card = 1; card <= 10; ++card)
        {
            if (s.cards[card] < shoe[card])
            {
                State next{s};
                ++next.cards[card];
                std::int8_t& hand = next.hands[next.current];
                hand = deal(hand, card);
                moves.push_back(
                    doubled || hand > 21 ? stand(next, doubled) : next);
            }
        }
    }

    // Return value of hitting (or doubling down) current hand.
    double value_hit(State s, bool doubled = false)
    {
        s.cards[0] = std::min(s.cards[0] + 1, 3);
        double value = 0;
        int total_cards = 0;
        for (int card = 1; card <= 10; ++card)
        {
            int num_cards = shoe[card] - s.cards[card];
            if (num_cards > 0)
            {
                total_cards += num_cards;
                State next{s};
                ++next.cards[card];
                std::int8_t& hand = next.hands[next.current];
                hand = deal(hand, card);
                value += num_cards * lookup(
                    doubled || hand > 21 ? stand(next, doubled) : next)->value;
            }
        }
        return (total_cards == 0) ? -std::numeric_limits<double>::infinity() :
            (value / total_cards);
    }

    // Return new state from splitting current pair hand.
    State split(State s)
    {
        s.cards[0] = 1;
        s.hands[s.current] = pair_half;
        int num_hands = s.current + 1;
        while (s.hands[num_hands] != 0)
        {
            ++num_hands;
        }
        s.hands[num_hands] = pair_half;
        return s;
    }

    // Return expected value of given state with all hands completed.
    double value_round(State s)
    {
        // Compute probabilities of outcomes of dealer's hand.
        --s.cards[up_card];
        const int s1 = shoe[1] - s.cards[1];
        const int s2 = shoe[2] - s.cards[2];
        const int s3 = shoe[3] - s.cards[3];
        const int s4 = shoe[4] - s.cards[4];
        const int s5 = shoe[5] - s.cards[5];
        const int s6 = shoe[6] - s.cards[6];
        const int s7 = shoe[7] - s.cards[7];
        const int s8 = shoe[8] - s.cards[8];
        const int s9 = shoe[9] - s.cards[9];
        const int s10 = shoe[10] - s.cards[10];
        const int t = s1 + s2 + s3 + s4 + s5 + s6 + s7 + s8 + s9 + s10;
        double pcount_17 = 0;
        double pcount_18 = 0;
        double pcount_19 = 0;
        double pcount_20 = 0;
        double pcount_21 = 0;
        if (H17)
        {
#include "dealer_up_card_h17.hpp"
        }
        else
        {
#include "dealer_up_card_s17.hpp"
        }
        double pcount[23];
        pcount[17] = pcount_17;
        pcount[18] = pcount_18;
        pcount[19] = pcount_19;
        pcount[20] = pcount_20;
        pcount[21] = pcount_21;
        pcount[22] = 1;
        const int u = shoe[up_card] - s.cards[up_card];
        for (int count = 17; count <= 21; ++count)
        {
            pcount[count] /= u;
            pcount[22] -= pcount[count];
        }
        double pbj = (up_card == 1 && t > 1) ? double(s10) / (t - 1) :
                    ((up_card == 10 && t > 1) ? double(s1) / (t - 1) : 0);
        pcount[22] -= pbj;

        // Compute wagers on each hand.
        int wagers[4] = {1, 1, 1, 1};
        int total_wager = s.current;
        for (int h = 0; h < s.current; ++h)
        {
            if (s.hands[h] < 0)
            {
                s.hands[h] = -s.hands[h];
                ++wagers[h];
                ++total_wager;
            }
        }

        // Resolve hands for each possible dealer outcome.
        double value = ENHC ? -pbj * total_wager : -pbj;
        for (int dealer = 17; dealer <= 22; ++dealer)
        {
            int win = 0;
            for (int h = 0; h < s.current; ++h)
            {
                int hand = s.hands[h];
                win += wagers[h] * (hand > 21 ? -1 : (dealer > 21 ? 1 :
                    ((hand > dealer) - (hand < dealer))));
            }
            value += pcount[dealer] * win;
        }
        return value;
    }

    // Return pointer to hash map entry for given game state.
    Solved* lookup(const State& s)
    {
        std::uint64_t h = hash(s);
        std::size_t step = (h >> (64 - HASH_EXP)) | 1;
        for (std::size_t i = h;;)
        {
            i = (i + step) & HASH_MASK;
            if (hash_map[i].state.cards[0] == 0 || hash_map[i].state == s)
            {
                return &hash_map[i];
            }
        }
    }

    std::uint64_t hash(const State& s)
    {
        std::uint64_t h[2];
        std::memcpy(h, &s, sizeof(h));
        return split_mix_64(h[0]) ^ split_mix_64(h[1]);
    }

    std::uint64_t split_mix_64(std::uint64_t h)
    {
        h ^= h >> 30;
        h *= 0xbf58476d1ce4e5b9u;
        h ^= h >> 27;
        h *= 0x94d049bb133111ebu;
        h ^= h >> 31;
        return h;
    }

    const int HASH_EXP;
    const std::size_t HASH_MASK;
    std::vector<Solved> hash_map;

    const bool ENHC;
    const bool H17;
    const bool DAS;
    const bool DOA;
    const bool D9;
    const int SPL;
    const bool RSA;

    std::vector<int> shoe;
    std::int8_t pair_half;
    std::int8_t pair_hand;
    int up_card;
};

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
    if (spl > 1)
    {
        std::cerr << "\nRe-split aces (1=yes, 0=no): ";
        std::cin >> rsa;
    }
    Game game{enhc, h17, das, doa, d9, spl, rsa, hash_exp};
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
        std::cerr << "\nSplit " << pair_card << " vs. " << up_card << ":\n";
        double value = game.value_split(shoe, pair_card, up_card);
        std::cerr << "E(split)= ";
        std::cout << value << std::endl;
        std::cerr << "\n========================================\n";
    }
}
