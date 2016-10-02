///////////////////////////////////////////////////////////////////////////////
//
// blackjack_pdf.cpp
// Copyright (C) 2016 Eric Farmer (see gpl.txt for details)
//
///////////////////////////////////////////////////////////////////////////////

#include "blackjack_pdf.h"

namespace // unnamed namespace
{
    struct Shoe : public BJShoe
    {
        Shoe(const BJShoe& shoe) : BJShoe(shoe) {}

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

    struct Hand : public BJHand
    {
        Hand() : BJHand(), wager(1), up_card(0) {}

        double wager;
        int up_card;
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
        Context(const BJShoe& shoe, BJRules& rules, BJStrategy& strategy) :
            shoe(shoe),
            rules(rules),
            hit_soft_17(rules.getHitSoft17()),
            surrender(rules.getLateSurrender()),
            blackjack_payoff(rules.getBlackjackPayoff()),
            strategy(strategy),
            dealer_caches(),
            pdf()
        {
            // empty
        }

        Shoe shoe;
        BJRules& rules;
        const bool hit_soft_17;
        const bool surrender;
        const double blackjack_payoff;
        BJStrategy& strategy;
        std::map<Shoe, Dealer> dealer_caches[11];
        std::map<double, Kahan> pdf;
    };

    struct Round
    {
        Round(Context *context) :
            context(context),
            shoe(context->shoe),
            hands(),
            num_hands(1),
            current(0),
            probability(1)
        {
            // empty
        }

        Context *context;
        Shoe shoe;
        Hand hands[5];
        int num_hands;
        int current;
        double probability;

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
                        context->pdf[-1].add(probability * p_bj);
                        context->pdf[-0.5].add(probability * (1 - p_bj));
                        break;
                    }
                }
            }
        }

        void resolve()
        {
            if (current++ < num_hands)
            {
                play();
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
                    context->pdf[-1].add(probability * p_bj);
                    context->pdf[win].add(probability * (1 - p_bj));
                }
                else
                {
                    shoe.undeal(up_card);
                    std::map<Shoe, Dealer>& dealer_cache =
                        context->dealer_caches[up_card];
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
                        context->pdf[0].add(probability * p_bj);
                        context->pdf[context->blackjack_payoff].add(
                            probability * (1 - p_bj));
                    }
                    else
                    {
                        context->pdf[-1].add(probability * p_bj);
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
                            context->pdf[win].add(p);
                        }
                        double win = 0;
                        for (int i = 1; i <= num_hands; ++i)
                        {
                            win += ((hands[i].getCount() > 21) ?
                                -hands[i].wager : hands[i].wager);
                        }
                        context->pdf[win].add(probability * dealer.p_bust);
                    }
                }
            }
        }
    };
} // unnamed namespace

std::map<double, double> compute_pdf(
    const BJShoe& shoe, BJRules& rules, BJStrategy& strategy)
{
    Context context(shoe, rules, strategy);
    Round round(&context);
    round.deal();
    std::map<double, double> pdf;
    for (auto&& q : context.pdf)
    {
        pdf[q.first] = q.second.sum;
    }
    return pdf;
}
