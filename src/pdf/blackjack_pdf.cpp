///////////////////////////////////////////////////////////////////////////////
//
// blackjack_pdf.cpp
// Copyright (C) 2017 Eric Farmer (see gpl.txt for details)
//
///////////////////////////////////////////////////////////////////////////////

#include "blackjack_pdf.h"

namespace // unnamed namespace
{
    struct Hand : public BJHand
    {
        Hand() : BJHand(), wager(1), up_card(0) {}

        double wager;
        int up_card;

        friend bool operator< (const Hand& a, const Hand& b)
        {
            if (a.wager != b.wager)
            {
                return a.wager < b.wager;
            }
            int card = 1;
            for (; card < 10 && a.cards[card] == b.cards[card]; ++card);
            return (a.cards[card] < b.cards[card]);
        }
    };

    struct Shoe : public BJShoe
    {
        Shoe(const BJShoe& shoe) : BJShoe(shoe) {}

        // Return true iff this shoe contains at least the given up card and
        // the cards in the given two split hands.
        bool is_valid(int up_card, const Hand& h1, const Hand& h2)
        {
            for (int card = 1; card <= 10; ++card)
            {
                if (cards[card] < ((card == up_card) ? 1 : 0) +
                    h1.getCards(card) + h2.getCards(card))
                {
                    return false;
                }
            }
            return true;
        }

        // Remove the given cards from this shoe, returning the corresponding
        // probability of dealing the cards.
        double deal_split(int up_card, const Hand& h1, const Hand& h2)
        {
            double probability = getProbability(up_card);
            deal(up_card);
            for (int card = 1; card <= 10; ++card)
            {
                int num_cards = h1.getCards(card) + h2.getCards(card);
                for (int i = 0; i < num_cards; ++i)
                {
                    probability *= getProbability(card);
                    deal(card);
                }
            }
            return probability;
        }

        friend bool operator< (const Shoe& a, const Shoe& b)
        {
            int card = 1;
            for (; card < 10 && a.cards[card] == b.cards[card]; ++card);
            return (a.cards[card] < b.cards[card]);
        }
    };

    struct Dealer
    {
        Dealer() : pcount(), p_bust(1) {}

        double pcount[5];
        double p_bust;

        void compute_probabilities(
            const Shoe& shoe, int up_card, bool hit_soft_17)
        {
            const int s1 = shoe.getCards(1);
            const int s2 = shoe.getCards(2);
            const int s3 = shoe.getCards(3);
            const int s4 = shoe.getCards(4);
            const int s5 = shoe.getCards(5);
            const int s6 = shoe.getCards(6);
            const int s7 = shoe.getCards(7);
            const int s8 = shoe.getCards(8);
            const int s9 = shoe.getCards(9);
            const int s10 = shoe.getCards(10);
            const int t = shoe.getCards();
            double& pcount_17 = pcount[0];
            double& pcount_18 = pcount[1];
            double& pcount_19 = pcount[2];
            double& pcount_20 = pcount[3];
            double& pcount_21 = pcount[4];
            if (hit_soft_17)
            {
#include "dealer_up_card_h17.hpp"
            }
            else
            {
#include "dealer_up_card_s17.hpp"
            }
            const int u = shoe.getCards(up_card);
            for (int count = 0; count < 5; ++count)
            {
                pcount[count] /= u;
                p_bust -= pcount[count];
            }
            if (up_card == 1)
            {
                p_bust -= static_cast<double>(s10) / (t - 1);
            }
            else if (up_card == 10)
            {
                p_bust -= static_cast<double>(s1) / (t - 1);
            }
        }
    };

    struct Kahan
    {
        Kahan() : sum(0), correction(0) {}

        double sum;
        double correction;

        void add(double x)
        {
            double y = x - correction;
            double t = sum + y;
            correction = (t - sum) - y;
            sum = t;
        }
    };

    struct Context
    {
        Context(const BJShoe& shoe, BJRules& rules,
                BJStrategy& strategy, IndexStrategy& insurance) :
            shoe(shoe),
            rules(rules),
            hit_soft_17(rules.getHitSoft17()),
            surrender(rules.getLateSurrender()),
            blackjack_payoff(rules.getBlackjackPayoff()),
            strategy(strategy),
            insurance(insurance),
            dealer_caches(),
            pdf(),
            split_hands()
        {
            // empty
        }

        Shoe shoe;
        BJRules& rules;
        const bool hit_soft_17;
        const bool surrender;
        const double blackjack_payoff;
        BJStrategy& strategy;
        IndexStrategy& insurance;
        std::map<Shoe, Dealer> dealer_caches[11];
        std::map<double, Kahan> pdf;
        std::map<Hand, int> split_hands[11][11];
    };

    struct Round
    {
        Round(Context *context) :
            context(context),
            shoe(context->shoe),
            hands(),
            num_hands(1),
            current(0),
            probability(1),
            insurance(false)
        {
            // empty
        }

        Context *context;
        Shoe shoe;
        Hand hands[5];
        int num_hands;
        int current;
        double probability;
        bool insurance;

        void deal()
        {
            for (int card = 1; card <= 10; ++card)
            {
                double p = shoe.getProbability(card);
                if (p != 0)
                {
                    Round round(*this);
                    round.shoe.deal(card);
                    round.hands[current].deal(card);
                    if (hands[current].up_card == 0)
                    {
                        round.hands[current].up_card = card;
                        if (current == 0)
                        {
                            ++round.current;
                        }
                    }
                    round.probability *= p;
                    round.play();
                }
            }
        }

        void play()
        {
            const Hand& hand = hands[current];
            if (hand.getCards() < 2)
            {
                deal();
            }
            else if (hand.getCount() > 21 || hand.wager == 2)
            {
                resolve();
            }
            else
            {
                if (hands[0].up_card == 1 &&
                    hand.getCards() == 2 && num_hands == 1)
                {
                    insurance = context->insurance.insurance(hand);
                }
                bool double_down = false;
                if (num_hands == 1 || hand.up_card != 1)
                {
                    double_down = ((num_hands == 1 &&
                            context->rules.getDoubleDown(hand)) ||
                        (num_hands > 1 &&
                            context->rules.getDoubleAfterSplit(hand)));
                }
                const bool split = (hand.getCards() == 2 &&
                    hand.getCards(hand.up_card) == 2 &&
                    num_hands < context->rules.getResplit(hand.up_card));
                const bool surrender = (context->surrender &&
                    hand.getCards() == 2 && num_hands == 1);
                int option = context->strategy.getOption(hand,
                    hands[0].up_card, double_down, split, surrender);
                if (num_hands > 1 && hand.up_card == 1 && option == BJ_HIT)
                {
                    option = BJ_STAND;
                }
                switch (option)
                {
                case BJ_STAND:
                    {
                        resolve();
                        break;
                    }
                case BJ_HIT:
                    {
                        deal();
                        break;
                    }
                case BJ_DOUBLE_DOWN:
                    {
                        hands[current].wager = 2;
                        deal();
                        break;
                    }
                case BJ_SPLIT:
                    {
                        hands[current].undeal(hand.up_card);
                        hands[++num_hands] = hands[current];
                        deal();
                        break;
                    }
                case BJ_SURRENDER:
                    {
                        double p_bj = 0;
                        if (hands[0].up_card == 1)
                        {
                            p_bj = shoe.getProbability(10);
                        }
                        else if (hands[0].up_card == 10)
                        {
                            p_bj = shoe.getProbability(1);
                        }
                        context->pdf[-1 + (insurance ? 1 : 0)].add(
                            probability * p_bj);
                        context->pdf[-0.5 + (insurance ? -0.5 : 0)].add(
                            probability * (1 - p_bj));
                        break;
                    }
                }
            }
        }

        void resolve()
        {
            if (current < num_hands)
            {
                // We could brute-force all possible ways to resolve all split
                // hands; instead, just record how many times we reach the
                // given subset of cards, *and* the corresponding wager.

                //play();
                context->split_hands[hands[0].up_card][
                    hands[current].up_card][hands[current]]++;
            }
            else
            {
                bool busted = true;
                for (int i = 1; i <= num_hands; ++i)
                {
                    if (hands[i].getCount() <= 21)
                    {
                        busted = false;
                        break;
                    }
                }
                const int up_card = hands[0].up_card;
                double p_bj = 0;
                if (up_card == 1)
                {
                    p_bj = shoe.getProbability(10);
                }
                else if (up_card == 10)
                {
                    p_bj = shoe.getProbability(1);
                }
                if (busted)
                {
                    double win = 0;
                    for (int i = 1; i <= num_hands; ++i)
                    {
                        win -= hands[i].wager;
                    }
                    context->pdf[-1 + (insurance ? 1 : 0)].add(
                        probability * p_bj);
                    context->pdf[win + (insurance ? -0.5 : 0)].add(
                        probability * (1 - p_bj));
                }
                else
                {
                    shoe.undeal(up_card);
                    auto&& dealer_cache = context->dealer_caches[up_card];
                    if (dealer_cache.find(shoe) == dealer_cache.end())
                    {
                        Dealer dealer;
                        dealer.compute_probabilities(shoe, up_card,
                            context->hit_soft_17);
                        dealer_cache[shoe] = dealer;
                    }
                    const Dealer& dealer = dealer_cache[shoe];
                    if (num_hands == 1 && hands[1].getCards() == 2 &&
                        hands[1].getCount() == 21)
                    {
                        context->pdf[0 + (insurance ? 1 : 0)].add(
                            probability * p_bj);
                        context->pdf[context->blackjack_payoff +
                            (insurance ? -0.5 : 0)].add(
                            probability * (1 - p_bj));
                    }
                    else
                    {
                        context->pdf[-1 + (insurance ? 1 : 0)].add(
                            probability * p_bj);
                        for (int dealer_count = 17; dealer_count <= 21;
                            ++dealer_count)
                        {
                            double p = probability *
                                dealer.pcount[dealer_count - 17];
                            double win = 0;
                            for (int i = 1; i <= num_hands; ++i)
                            {
                                int count = hands[i].getCount();
                                win += ((count > 21 || count < dealer_count) ?
                                    -hands[i].wager :
                                    ((count == dealer_count) ?
                                        0 : hands[i].wager));
                            }
                            context->pdf[win + (insurance ? -0.5 : 0)].add(p);
                        }
                        double win = 0;
                        for (int i = 1; i <= num_hands; ++i)
                        {
                            win += ((hands[i].getCount() > 21) ?
                                -hands[i].wager : hands[i].wager);
                        }
                        context->pdf[win + (insurance ? -0.5 : 0)].add(
                            probability * dealer.p_bust);
                    }
                }
            }
        }
    };
} // unnamed namespace

std::map<double, double> compute_pdf(const BJShoe& shoe, BJRules& rules,
    BJStrategy& strategy, IndexStrategy& insurance_strategy)
{
    Context context(shoe, rules, strategy, insurance_strategy);
    Round round(&context);
    round.deal();
    for (int up_card = 1; up_card <= 10; ++up_card)
    {
        for (int pair_card = 1; pair_card <= 10; ++pair_card)
        {
            auto&& split_hands = context.split_hands[up_card][pair_card];
            for (auto&& h1 : split_hands)
            {
                bool insurance = false;
                if (up_card == 1)
                {
                    BJHand pair_hand;
                    pair_hand.deal(pair_card);
                    pair_hand.deal(pair_card);
                    insurance = insurance_strategy.insurance(pair_hand);
                }
                for (auto&& h2 : split_hands)
                {
                    if (!(h1 < h2))
                    {
                        Shoe this_shoe(shoe);
                        if (this_shoe.is_valid(up_card, h1.first, h2.first))
                        {
                            double probability = this_shoe.deal_split(up_card,
                                h1.first, h2.first) * h1.second * h2.second;
                            if (h2 < h1)
                            {
                                probability *= 2;
                            }
                            double p_bj = 0;
                            if (up_card == 1)
                            {
                                p_bj = this_shoe.getProbability(10);
                            }
                            else if (up_card == 10)
                            {
                                p_bj = this_shoe.getProbability(1);
                            }
                            if (h1.first.getCount() > 21 &&
                                h2.first.getCount() > 21)
                            {
                                context.pdf[-1 + (insurance ? 1 : 0)].add(
                                    probability * p_bj);
                                context.pdf[
                                    -(h1.first.wager +
                                    h2.first.wager) +
                                        (insurance ? -0.5 : 0)].add(
                                        probability * (1 - p_bj));
                            }
                            else
                            {
                                this_shoe.undeal(up_card);
                                auto&& dealer_cache =
                                    context.dealer_caches[up_card];
                                if (dealer_cache.find(this_shoe) ==
                                    dealer_cache.end())
                                {
                                    Dealer dealer;
                                    dealer.compute_probabilities(
                                        this_shoe, up_card,
                                        context.hit_soft_17);
                                    dealer_cache[this_shoe] = dealer;
                                }
                                const Dealer& dealer = dealer_cache[this_shoe];
                                context.pdf[-1 + (insurance ? 1 : 0)].add(
                                    probability * p_bj);
                                for (int dealer_count = 17; dealer_count <= 21;
                                    ++dealer_count)
                                {
                                    double p = probability *
                                        dealer.pcount[dealer_count - 17];
                                    double win = 0;
                                    int c1 = h1.first.getCount();
                                    win += ((c1 > 21 || c1 < dealer_count) ?
                                        -h1.first.wager :
                                        ((c1 == dealer_count) ?
                                            0 : h1.first.wager));
                                    int c2 = h2.first.getCount();
                                    win += ((c2 > 21 || c2 < dealer_count) ?
                                        -h2.first.wager :
                                        ((c2 == dealer_count) ?
                                            0 : h2.first.wager));
                                    context.pdf[win +
                                        (insurance ? -0.5 : 0)].add(p);
                                }
                                context.pdf[((h1.first.getCount() > 21) ?
                                    -h1.first.wager : h1.first.wager) +
                                    ((h2.first.getCount() > 21) ?
                                        -h2.first.wager : h2.first.wager) +
                                    (insurance ? -0.5 : 0)].add(
                                            probability * dealer.p_bust);
                            }
                        }
                    }
                }
            }
        }
    }

    // Strip Kahan summation wrapper.
    std::map<double, double> pdf;
    for (auto&& q : context.pdf)
    {
        pdf[q.first] = q.second.sum;
    }
    return pdf;
}
