///////////////////////////////////////////////////////////////////////////////
//
// indices.cpp
// Copyright (C) 2013 Eric Farmer (see gpl.txt for details)
//
///////////////////////////////////////////////////////////////////////////////

#include "indices.h"

IndexStrategy::IndexStrategy(BJRules& rules, const double tags[],
                             int resolution, const BJShoe& shoe) :
    BJStrategy(),
    tags(tags, tags + 11),
    resolution(resolution),
    shoe(shoe) {

    // Compute optimal "infinite" deck strategy, so that strategy depends only
    // on hand total, not composition.
    BJShoe infiniteDeck(1024);
    BJStrategy strategy;
    BJProgress progress;
    BJPlayer *tdz = new BJPlayer(infiniteDeck, rules, strategy, progress);

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
                int play = tdz->getOption(hand, upCard,
                    doubleDown != 0, split != 0, surrender != 0);
                setIndex(count, false, upCard, doubleDown != 0,
                    split != 0, surrender != 0, 0, play, play);
            }
        }

        // For completeness, a hard 21 is a special case; use 10-9-2.
        if (split == 0 && surrender == 0) {
            BJHand hand;
            hand.deal(10);
            hand.deal(9);
            hand.deal(2);
            if (rules.getDoubleDown(hand) || doubleDown == 0) {
                int play = tdz->getOption(hand, upCard,
                    doubleDown != 0, false, false);
                setIndex(21, false, upCard,
                    doubleDown != 0, false, false, 0, play, play);
            }
        }

        // Get strategy for soft hands.
        for (int count = 12; count <= 21; ++count) {
            if (count == 12 || split == 0) {
                BJHand hand;
                hand.deal(1);
                hand.deal(count - 11);
                int play = tdz->getOption(hand, upCard,
                    doubleDown != 0, split != 0, surrender != 0);
                setIndex(count, true, upCard, doubleDown != 0,
                    split != 0, surrender != 0, 0, play, play);
            }
        }
    }
    delete tdz;
}

void IndexStrategy::setIndex(int count, bool soft, int upCard,
    bool doubleDown, bool split, bool surrender,
    double trueCount, int play1, int play2) {
    Index& index = indices[count][soft][upCard][doubleDown][split][surrender];
    index.trueCount = trueCount;
    index.play1 = play1;
    index.play2 = play2;
}

int IndexStrategy::getOption(const BJHand& hand, int upCard, bool doubleDown,
    bool split, bool surrender) {
    Index& index = indices[hand.getCount()][hand.getSoft()][upCard]
        [doubleDown][split][surrender];
    return (trueCount(hand, upCard) < index.trueCount ?
        index.play1 : index.play2);
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
