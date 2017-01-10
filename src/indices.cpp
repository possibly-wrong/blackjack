///////////////////////////////////////////////////////////////////////////////
//
// indices.cpp
// Copyright (C) 2017 Eric Farmer (see gpl.txt for details)
//
///////////////////////////////////////////////////////////////////////////////

#include "indices.h"

IndexStrategy::IndexStrategy(BJRules& rules, const std::vector<double>& tags,
                             int resolution, const BJShoe& shoe,
                             double insuranceIndex) :
    BJStrategy(),
    tags(tags),
    resolution(resolution),
    shoe(shoe),
    insuranceIndex(insuranceIndex) {

    // Compute optimal "infinite" deck strategy, so that strategy depends only
    // on hand total, not composition.
    BJShoe infiniteDeck(1024);
    BJStrategy strategy;
    BJProgress progress;
    BJPlayer *tdz = new BJPlayer(infiniteDeck, rules, strategy, progress);

    // There are no indices for the default strategy.
    std::vector<double> indices(1, std::numeric_limits<double>::infinity());
    std::vector<int> plays(1);

    // "Copy" TDZ strategy into indices[] array, using a representative 2-card
    // hand for each hard/soft hand total.
    for (int upCard = 1; upCard <= 10; ++upCard)
    for (int doubleDown = 0; doubleDown < 2; ++doubleDown)
    for (int split = 0; split < 2; ++split)
    for (int surrender = 0; surrender < 2; ++surrender) {

        // Get strategy for hard hands.  For each even count, use a pair hand
        // just in case splitting is allowed.
        for (int count = 4; count <= 20; ++count) {
            if (count % 2 == 0 || split == 0) {
                BJHand hand;
                hand.deal(count / 2);
                hand.deal(count - count / 2);
                plays[0] = tdz->getOption(hand, upCard,
                    doubleDown != 0, split != 0, surrender != 0);
                setOption(count, false, upCard, doubleDown != 0,
                    split != 0, surrender != 0, indices, plays);
            }
        }

        // For completeness, a hard 21 is a special case; use 10-9-2.
        if (split == 0 && surrender == 0) {
            BJHand hand;
            hand.deal(10);
            hand.deal(9);
            hand.deal(2);
            if (rules.getDoubleDown(hand) || doubleDown == 0) {
                plays[0] = tdz->getOption(hand, upCard,
                    doubleDown != 0, false, false);
                setOption(21, false, upCard,
                    doubleDown != 0, false, false, indices, plays);
            }
        }

        // Get strategy for soft hands.
        for (int count = 12; count <= 21; ++count) {
            if (count == 12 || split == 0) {
                BJHand hand;
                hand.deal(1);
                hand.deal(count - 11);
                plays[0] = tdz->getOption(hand, upCard,
                    doubleDown != 0, split != 0, surrender != 0);
                setOption(count, true, upCard, doubleDown != 0,
                    split != 0, surrender != 0, indices, plays);
            }
        }
    }
    delete tdz;
}

void IndexStrategy::setOption(int count, bool soft, int upCard,
    bool doubleDown, bool split, bool surrender,
    const std::vector<double>& indices, const std::vector<int>& plays) {
    Strategy& strategy =
        strategies[count][soft][upCard][doubleDown][split][surrender];
    strategy.indices = indices;
    strategy.plays = plays;
}

int IndexStrategy::getOption(const BJHand& hand, int upCard, bool doubleDown,
    bool split, bool surrender) {
    Strategy& strategy = strategies[hand.getCount()][hand.getSoft()][upCard]
        [doubleDown][split][surrender];
    double tc = trueCount(hand, upCard);

    // Return the strategy corresponding to the interval containing the true
    // count.
    for (int i = 0;; ++i) {
        if (tc < strategy.indices[i]) {
            return strategy.plays[i];
        }
    }
}

bool IndexStrategy::insurance(const BJHand& hand) {
    return (trueCount(hand, 1) >= insuranceIndex);
}

double IndexStrategy::trueCount(const BJHand& hand, int upCard) {

    // Compute running count, accounting for the dealer up card and cards in
    // the current hand.  This is a slight approximation in the sense that
    // post-split hands will not correctly account for all cards seen.
    double rc = tags[upCard];
    for (int card = 1; card <= 10; ++card) {
        rc += tags[card] * (hand.getCards(card) - shoe.getCards(card));
    }

    // Divide by number of decks remaining, rounding to the given resolution.
    int numCards = shoe.getCards() - hand.getCards() - 1;
    int numGroups = (numCards + resolution / 2) / resolution;
    return rc / numGroups * 52 / resolution;
}
