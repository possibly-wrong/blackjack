// Optimal strategy maximizing expected value of splitting pairs in blackjack

#ifndef BLACKJACK_SPLIT_H
#define BLACKJACK_SPLIT_H

#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <set>
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

std::ostream& operator<< (std::ostream& os, const State& s)
{
    os << "cards=[";
    for (int c = 0; c < 11; ++c)
    {
        os << int(s.cards[c]) << (c < 10 ? ", " : "");
    }
    os << "], hands=[";
    for (int h = 0; h < 4; ++h)
    {
        os << int(s.hands[h]) << (h < 3 ? ", " : "");
    }
    os << "], current=" << int(s.current);
    return os;
}

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
        bool force_resplit, int hash_exp) :
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
        force_resplit{force_resplit},
        dealer_hit_hands(11),
        shoe{},
        pair_half{0},
        pair_hand{0},
        up_card{0}
    {
        for (up_card = 1; up_card <= 10; ++up_card)
        {
            std::vector<int> cards(11);
            ++cards[up_card];
            init_dealer_hit_hands(cards, deal(0, up_card));
        }
    }

    // Return expected value of splitting pair vs. dealer up card, from shoe
    // with shoe[rank] cards of each rank 1..10 (including initial pair cards
    // and up card).
    double value_split(const std::vector<int>& shoe,
        int pair_card, int up_card,
        bool count_all, bool list_all, bool count_optimal, bool list_optimal)
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
        if (count_all)
        {
            show_states(s0, false, list_all);
        }
        if (count_optimal)
        {
            show_states(s0, true, list_optimal);
        }
        return lookup(s0)->value;
    }

    // Display states (either count or explicit list) reachable using arbitrary
    // or optimal strategy.
    void show_states(const State& s0, bool optimal, bool list_states)
    {
        std::size_t count = 0;
        std::set<Solved*> level;
        level.insert(lookup(s0));
        while (!level.empty())
        {
            std::set<Solved*> next;
            for (const auto& s_ptr : level)
            {
                State s = s_ptr->state;
                double value = s_ptr->value;
                ++count;
                if (list_states)
                {
                    std::cout << "\n" << s;
                }
                if (s.current == 4 || s.hands[s.current] == 0)
                {
                    if (list_states)
                    {
                        std::cout << ", E(dealer)=" << value;
                    }
                    continue;
                }
                if (s.cards[0] == 2)
                {
                    std::int8_t hand = s.hands[s.current];
                    if (hand == pair_hand && s.hands[SPL] == 0 &&
                        (RSA || pair_half != -11))
                    {
                        Solved* next_ptr = lookup(split(s));
                        if (!optimal && !force_resplit)
                        {
                            next.insert(next_ptr);
                        }
                        if (next_ptr->value == value)
                        {
                            if (list_states)
                            {
                                std::cout << ", E(split)=" << value;
                            }
                            if (optimal || force_resplit)
                            {
                                next.insert(next_ptr);
                                continue;
                            }
                        }
                    }
                    if (DAS && (DOA || hand == 10 || hand == 11 || (D9 && hand == 9))
                        && pair_half != -11)
                    {
                        if (!optimal)
                        {
                            std::vector<State> moves;
                            hit(moves, s, true);
                            for (const auto& m : moves)
                            {
                                next.insert(lookup(m));
                            }
                        }
                        if (value_hit(s, true) == value)
                        {
                            if (list_states)
                            {
                                std::cout << ", E(double)=" << value;
                            }
                            if (optimal)
                            {
                                std::vector<State> moves;
                                hit(moves, s, true);
                                for (const auto& m : moves)
                                {
                                    next.insert(lookup(m));
                                }
                                continue;
                            }
                        }
                    }
                }
                if (s.cards[0] >= 2)
                {
                    Solved* next_ptr = lookup(stand(s));
                    if (!optimal)
                    {
                        next.insert(next_ptr);
                    }
                    if (next_ptr->value == value)
                    {
                        if (list_states)
                        {
                            std::cout << ", E(stand)=" << value;
                        }
                        if (optimal)
                        {
                            next.insert(next_ptr);
                            continue;
                        }
                    }
                }
                if (s.cards[0] < 2 || pair_half != -11)
                {
                    if (!optimal)
                    {
                        std::vector<State> moves;
                        hit(moves, s);
                        for (const auto& m : moves)
                        {
                            next.insert(lookup(m));
                        }
                    }
                    if (value_hit(s) == value)
                    {
                        if (list_states)
                        {
                            std::cout << ", E(hit)=" << value;
                        }
                        if (optimal)
                        {
                            std::vector<State> moves;
                            hit(moves, s);
                            for (const auto& m : moves)
                            {
                                next.insert(lookup(m));
                            }
                            continue;
                        }
                    }
                }
            }
            level = next;
        }
        std::cout << "\n" << count;
        std::cerr << " states reachable using " <<
            (optimal ? "optimal" : "arbitrary") << " strategy.\n";
        std::cout << std::endl;
    }

    // Pre-compute list of depleted shoes where dealer may run out of cards.
    void init_dealer_hit_hands(const std::vector<int>& cards, int hand)
    {
        if (std::abs(hand) < 17 || (H17 && hand == -17))
        {
            dealer_hit_hands[up_card].insert(cards);
            for (int card = 1; card <= 10; ++card)
            {
                std::vector<int> next{cards};
                ++next[card];
                init_dealer_hit_hands(next, deal(hand, card));
            }
        }
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
                        if (num_states * 100 / (HASH_MASK + 1) > 98)
                        {
                            std::cerr << " out of memory." << std::endl;
                            return 0;
                        }
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
    std::size_t solve(const State& s0)
    {
        std::stack<Solved*> depths;
        std::size_t num_states = search(s0, depths);
        if (num_states == 0)
        {
            lookup(s0)->value = std::numeric_limits<double>::quiet_NaN();
            return 0;
        }
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
        return num_states;
    }

    // Return possible next states from standing/hitting/etc.
    std::vector<State> get_moves(const State& s)
    {
        std::vector<State> moves;
        if (s.current == 4 || s.hands[s.current] == 0)
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
            std::int8_t hand = s.hands[s.current];
            if (DAS && (DOA || hand == 10 || hand == 11 || (D9 && hand == 9))
                && pair_half != -11)
            {
                hit(moves, s, true);
            }
            if (hand == pair_hand && s.hands[SPL] == 0 &&
                (RSA || pair_half != -11))
            {
                if (force_resplit)
                {
                    moves.clear();
                }
                moves.push_back(split(s));
            }
        }
        return moves;
    }

    // Return expected value of given state using optimal strategy.
    double get_value(const State& s)
    {
        if (s.current == 4 || s.hands[s.current] == 0)
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
            std::int8_t hand = s.hands[s.current];
            if (DAS && (DOA || hand == 10 || hand == 11 || (D9 && hand == 9))
                && pair_half != -11)
            {
                value = std::max(value, value_hit(s, true));
            }
            if (hand == pair_hand && s.hands[SPL] == 0 &&
                (RSA || pair_half != -11))
            {
                double v_split = lookup(split(s))->value;
                value = force_resplit ? v_split : std::max(value, v_split);
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

        // Return -infinity if dealer may run out of cards.
        const int max_cards[] = {0, 13, 12, 11, 10, 10, 9, 9, 8, 8, 7};
        if (t < max_cards[up_card])
        {
            const std::vector<int> cards = {
                0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10};
            if (dealer_hit_hands[up_card].find(cards) !=
                dealer_hit_hands[up_card].end())
            {
                return -std::numeric_limits<double>::infinity();
            }
        }

        // Compute probabilities of outcomes of dealer's hand.
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
    const bool force_resplit;
    std::vector<std::set<std::vector<int> > > dealer_hit_hands;

    std::vector<int> shoe;
    std::int8_t pair_half;
    std::int8_t pair_hand;
    int up_card;
};

#endif // BLACKJACK_SPLIT_H
