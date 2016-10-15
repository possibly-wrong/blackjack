///////////////////////////////////////////////////////////////////////////////
//
// blackjack.cpp
// Copyright (C) 2016 Eric Farmer (see gpl.txt for details)
//
///////////////////////////////////////////////////////////////////////////////

#include "blackjack.h"

///////////////////////////////////////////////////////////////////////////////
//
// BJHand
//

BJHand::BJHand() {
    reset();
}

BJHand::BJHand(const int cards[]) {
    reset(cards);
}

int BJHand::getCards(int card) const {
    return cards[card];
}

int BJHand::getCards() const {
    return numCards;
}

int BJHand::getCount() const {
    return count;
}

bool BJHand::getSoft() const {
    return soft;
}

void BJHand::reset() {
    for (int card = 1; card <= 10; ++card) {
        cards[card] = 0;
    }
    numCards = count = 0;
    soft = false;
}

void BJHand::reset(const int cards[]) {
    numCards = count = 0;
    for (int card = 1; card <= 10; ++card) {
        this->cards[card] = cards[card];
        numCards += cards[card];
        count += card * cards[card];
    }
    if (count < 12 && cards[1]) {
        count += 10;
        soft = true;
    } else {
        soft = false;
    }
}

void BJHand::deal(int card) {
    ++cards[card];
    ++numCards;
    count += card;
    if (card == 1 && count < 12) {
        count += 10;
        soft = true;
    } else if (count > 21 && soft) {
        count -= 10;
        soft = false;
    }
}

void BJHand::undeal(int card) {
    --cards[card];
    --numCards;
    count -= card;
    if (card == 1 && !cards[1] && soft) {
        count -= 10;
        soft = false;
    } else if (count < 12 && cards[1] && !soft) {
        count += 10;
        soft = true;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
// BJShoe
//

BJShoe::BJShoe(int numDecks) {
    reset(numDecks);
}

BJShoe::BJShoe(const int cards[]) {
    reset(cards);
}

int BJShoe::getCards(int card) const {
    return cards[card];
}

int BJShoe::getCards() const {
    return numCards;
}

double BJShoe::getProbability(int card) const {
    return (double)cards[card] / numCards;
}

void BJShoe::reset() {
    numCards = 0;
    for (int card = 1; card <= 10; ++card) {
        cards[card] = totalCards[card];
        numCards += cards[card];
    }
}

void BJShoe::reset(const BJHand & hand) {
    numCards = 0;
    for (int card = 1; card <= 10; ++card) {
        cards[card] = totalCards[card] - hand.cards[card];
        numCards += cards[card];
    }
}

void BJShoe::reset(int numDecks) {
    for (int card = 1; card < 10; ++card) {
        cards[card] = totalCards[card] = 4 * numDecks;
    }
    cards[10] = totalCards[10] = 16 * numDecks;
    numCards = 52 * numDecks;
}

void BJShoe::reset(const int cards[]) {
    numCards = 0;
    for (int card = 1; card <= 10; ++card) {
        this->cards[card] = totalCards[card] = cards[card];
        numCards += cards[card];
    }
}

void BJShoe::deal(int card) {
    --cards[card];
    --numCards;
}

void BJShoe::undeal(int card) {
    ++cards[card];
    ++numCards;
}

///////////////////////////////////////////////////////////////////////////////
//
// BJDealer
//

BJDealer::BJDealer(bool hitSoft17) {
    this->hitSoft17 = hitSoft17;
    for (int upCard = 1; upCard <= 10; ++upCard) {
        probabilityBlackjack[upCard] = 0;
    }
}

void BJDealer::computeProbabilities(const BJShoe & shoe) {

    // Define constants for auto-generated code.
    const int s1 = shoe.cards[1];
    const int s2 = shoe.cards[2];
    const int s3 = shoe.cards[3];
    const int s4 = shoe.cards[4];
    const int s5 = shoe.cards[5];
    const int s6 = shoe.cards[6];
    const int s7 = shoe.cards[7];
    const int s8 = shoe.cards[8];
    const int s9 = shoe.cards[9];
    const int s10 = shoe.cards[10];
    const int t = shoe.numCards;
    double *pcount_17 = probabilityCount[0];
    double *pcount_18 = probabilityCount[1];
    double *pcount_19 = probabilityCount[2];
    double *pcount_20 = probabilityCount[3];
    double *pcount_21 = probabilityCount[4];

    // Initialize accumulated probabilities.
    for (int upCard = 1; upCard <= 10; ++upCard) {
        for (int count = 17; count <= 21; ++count) {
            probabilityCount[count - 17][upCard] = 0;
        }
        probabilityBust[upCard] = 1;
    }

    // For each (non-bust, non-blackjack) possible outcome, accumulate
    // probability of each hand with that count (that is actually possible in
    // the given shoe).
    if (hitSoft17) {
#include "dealer_h17.hpp"
    } else {
#include "dealer_s17.hpp"
    }

    // probabilityCount[] will accumulate the probability of NOT busting, so we
    // don't have to actually count busted hands.
    for (int upCard = 1; upCard <= 10; ++upCard) {
        if (shoe.cards[upCard]) {
            for (int count = 17; count <= 21; ++count) {
                probabilityCount[count - 17][upCard] /= shoe.cards[upCard];
                probabilityBust[upCard] -=
                    probabilityCount[count - 17][upCard];
            }
        }
    }

    // Compute P(blackjack).
    if (s1 && s10) {
        probabilityBlackjack[1] = (double)s10 / (t - 1);
        probabilityBust[1] -= probabilityBlackjack[1];
        probabilityBlackjack[10] = (double)s1 / (t - 1);
        probabilityBust[10] -= probabilityBlackjack[10];
    } else {
        probabilityBlackjack[1] = probabilityBlackjack[10] = 0;
    }

    // Store probability of each up card for computing overall probabilities.
    for (int upCard = 1; upCard <= 10; ++upCard) {
        probabilityCard[upCard] = shoe.getProbability(upCard);
    }
}

double BJDealer::getProbabilityBust(int upCard) const {
    return probabilityBust[upCard];
}

double BJDealer::getProbabilityBust() const {
    double p = 0;
    for (int upCard = 1; upCard <= 10; ++upCard) {
        p += probabilityBust[upCard] * probabilityCard[upCard];
    }
    return p;
}

double BJDealer::getProbabilityCount(int count, int upCard) const {
    return probabilityCount[count - 17][upCard];
}

double BJDealer::getProbabilityCount(int count) const {
    double p = 0;
    for (int upCard = 1; upCard <= 10; ++upCard) {
        p += probabilityCount[count - 17][upCard] * probabilityCard[upCard];
    }
    return p;
}

double BJDealer::getProbabilityBlackjack(int upCard) const {
    return probabilityBlackjack[upCard];
}

double BJDealer::getProbabilityBlackjack() const {
    return (probabilityBlackjack[1] * probabilityCard[1] +
            probabilityBlackjack[10] * probabilityCard[10]);
}

///////////////////////////////////////////////////////////////////////////////
//
// BJRules
//

BJRules::BJRules(bool hitSoft17, bool doubleAnyTotal, bool double9,
                 bool doubleSoft, bool doubleAfterHit, bool doubleAfterSplit,
                 bool resplit, bool resplitAces, bool lateSurrender,
                 double bjPayoff) :
    hitSoft17(hitSoft17),
    doubleAnyTotal(doubleAnyTotal),
    double9(double9),
    doubleSoft(doubleSoft),
    doubleAfterHit(doubleAfterHit),
    doubleAfterSplit(doubleAfterSplit),
    resplit(resplit),
    resplitAces(resplitAces),
    lateSurrender(lateSurrender),
    bjPayoff(bjPayoff) {
    // empty
}

BJRules::~BJRules() {
    // empty
}

bool BJRules::getHitSoft17() const {
    return hitSoft17;
}

bool BJRules::getDoubleDown(const BJHand & hand) const {
    return (
        (doubleAnyTotal || hand.getCount() == 10 || hand.getCount() == 11 ||
            (double9 && hand.getCount() == 9)) &&
        (doubleSoft || !hand.getSoft()) &&
        (doubleAfterHit || hand.getCards() == 2));
}

bool BJRules::getDoubleAfterSplit(const BJHand & hand) const {
    return (doubleAfterSplit && getDoubleDown(hand));
}

int BJRules::getResplit(int pairCard) const {
    return ((resplit && (resplitAces || pairCard != 1)) ? 4 : 2);
}

bool BJRules::getLateSurrender() const {
    return lateSurrender;
}

double BJRules::getBlackjackPayoff() const {
    return bjPayoff;
}

///////////////////////////////////////////////////////////////////////////////
//
// BJStrategy
//

BJStrategy::BJStrategy(bool useCDZ, bool useCDP1) :
    useCDZ(useCDZ),
    useCDP1(useCDP1) {
    // empty
}

BJStrategy::~BJStrategy() {
    // empty
}

int BJStrategy::getOption(const BJHand & hand, int upCard, bool doubleDown,
                          bool split, bool surrender) {
    return BJ_MAX_VALUE;
}

bool BJStrategy::getUseCDZ() const {
    return useCDZ;
}

bool BJStrategy::getUseCDP1() const {
    return useCDP1;
}

///////////////////////////////////////////////////////////////////////////////
//
// BJProgress
//

BJProgress::~BJProgress() {
    // empty
}

void BJProgress::indicate(int percentComplete) {
    // empty
}

///////////////////////////////////////////////////////////////////////////////
//
// BJPlayer
//

BJPlayer::BJPlayer(const BJShoe & shoe, BJRules & rules, BJStrategy & strategy,
                   BJProgress & progress) :
    BJStrategy(),
    pStrategy(0) {
    reset(shoe, rules, strategy, progress);
}

void BJPlayer::reset(const BJShoe & shoe, BJRules & rules,
                     BJStrategy & strategy, BJProgress & progress) {

    // Forget about any cards already dealt from the shoe, so shoe.reset(hand)
    // will work.
    pStrategy = &strategy;
    useCDZ = strategy.getUseCDZ();
    useCDP1 = strategy.getUseCDP1();
    this->shoe = shoe;
    numHands = 0;
    for (int card = 1; card <= 10; ++card) {
        playerHands[numHands].cards[card] =
        playerHands[numHands].hitHand[card] = 0;
        this->shoe.totalCards[card] = shoe.cards[card];
    }

    // Remember maximum number of additional pair cards to remove when
    // enumerating player hands.
    for (int pairCard = 1; pairCard <= 10; ++pairCard) {
        maxPairCards[pairCard] = (rules.getResplit(pairCard) - 1) * 2;
    }

    // Enumerate all possible player hands.
    currentHand.reset();
    countHands(numHands++, 1);
    linkHands();

    // Compute dealer probabilities for each hand.  This takes the most time,
    // so keep the caller updated on the progress.
    computeDealer(rules, progress);

    // Compute expected values for standing, doubling down, and hitting (in
    // that order, so all required values will be available when needed).
    linkHandCounts();
    computeStand();
    computeDoubleDown();
    computeHit(rules, strategy);

    // Blackjack pays 3:2, so correct the value for standing on this hand.  We
    // do this *before* computing E(split) to ensure consistency of CDZ-
    // strategy when drawing an ace to split tens.
    correctStandBlackjack(rules.getBlackjackPayoff());

    // Compute expected values for splitting pairs.  Re-link original hands by
    // count for future use.
    computeSplit(rules, strategy);
    linkHandCounts();

    // Compute overall expected values, condition individual hands on no dealer
    // blackjack, and finalize progress indicator.
    computeOverall(rules, strategy);
    conditionNoBlackjack(rules);
    progress.indicate(100);
}

double BJPlayer::getValueStand(const BJHand & hand, int upCard) const {
    return playerHands[findHand(hand)].valueStand[false][upCard];
}

double BJPlayer::getValueHit(const BJHand & hand, int upCard) const {
    return playerHands[findHand(hand)].valueHit[false][upCard];
}

double BJPlayer::getValueDoubleDown(const BJHand & hand, int upCard) const {
    return playerHands[findHand(hand)].valueDoubleDown[false][upCard];
}

double BJPlayer::getValueSplit(int pairCard, int upCard) const {
    return valueSplit[pairCard][upCard];
}

double BJPlayer::getValue(int upCard) const {
    return overallValues[upCard];
}

double BJPlayer::getValue() const {
    return overallValue;
}

int BJPlayer::getOption(const BJHand & hand, int upCard, bool doubleDown,
                        bool split, bool surrender) {

    // Use the provided fixed strategy if it is specified.
    int option = pStrategy->getOption(
        hand, upCard, doubleDown, split, surrender);
    if (option != BJ_MAX_VALUE) {
        return option;
    }

    // Otherwise compute the strategy maximizing expected value.
    PlayerHand & testHand = playerHands[findHand(hand)];
    double value = testHand.valueStand[false][upCard];
    option = BJ_STAND;
    if (value < testHand.valueHit[false][upCard]) {
        value = testHand.valueHit[false][upCard];
        option = BJ_HIT;
    }
    if (doubleDown) {
        if (value < testHand.valueDoubleDown[false][upCard]) {
            value = testHand.valueDoubleDown[false][upCard];
            option = BJ_DOUBLE_DOWN;
        }
    }
    if (split) {
        int pairCard = 1;
        while (!hand.cards[pairCard]) {
            ++pairCard;
        }
        if (value < valueSplit[pairCard][upCard]) {
            value = valueSplit[pairCard][upCard];
            option = BJ_SPLIT;
        }
    }
    if (surrender) {
        if (value < -0.5) {
            value = -0.5;
            option = BJ_SURRENDER;
        }
    }
    return option;
}

int BJPlayer::findHand(const BJHand & hand) const {
    int i = 0;
    for (int card = 1; card <= 10; ++card) {
        for (int c = 0; c < hand.cards[card]; ++c) {
            i = playerHands[i].hitHand[card];
        }
    }
    return i;
}

bool BJPlayer::record(const BJHand & hand) {
    bool result = false;

    // A hand is saved if it is not a bust hand...
    if (hand.count <= 21) {
        result = true;
    } else {

        // Or if it may be a split hand; note that we don't need extra hands
        // for split aces, since hitting split aces is not allowed.
        for (int card = 2; card <= 10; ++card) {
            int s = maxPairCards[card];
            if (hand.cards[card] < s) {
                s = hand.cards[card];
            }
            if (hand.count - card * (s - 1) <= 21) {
                result = true;
                break;
            }
        }
    }
    return result;
}

void BJPlayer::countHands(int i, int maxCard) {

    // To only count each hand (subset) once, only draw cards higher than any
    // in the hand.
    for (int card = maxCard; card <= 10; ++card) {
        if (shoe.cards[card]) {
            shoe.deal(card);
            currentHand.deal(card);

            // If the hand is not busted (or could be a split hand), record it
            // and partially link it up; we'll finish with linkHands() later.
            if (record(currentHand)) {
                playerHands[i].hitHand[card] = numHands;
                for (int c = 1; c <= 10; ++c) {
                    playerHands[numHands].cards[c] = currentHand.cards[c];
                    playerHands[numHands].hitHand[c] = 0;
                }
                countHands(numHands++, card);
            }
            currentHand.undeal(card);
            shoe.undeal(card);
        }
    }
}

void BJPlayer::linkHands() {
    for (int i = 0; i < numHands; ++i) {
        PlayerHand & hand = playerHands[i];
        for (int card = 1; card <= 10; ++card) {
            if (!hand.hitHand[card] && hand.cards[card] < shoe.cards[card]) {
                currentHand.reset(hand.cards);
                currentHand.deal(card);
                if (record(currentHand)) {
                    hand.hitHand[card] = findHand(currentHand);
                }
            }
        }
    }
}

void BJPlayer::computeDealer(BJRules & rules, BJProgress & progress) {
    BJDealer dealer(rules.getHitSoft17());
    for (int i = 0; i < numHands; ++i) {
        progress.indicate(100 * i / numHands);
        PlayerHand & hand = playerHands[i];
        currentHand.reset(hand.cards);
        shoe.reset(currentHand);
        dealer.computeProbabilities(shoe);
        for (int upCard = 1; upCard <= 10; ++upCard) {
            hand.probabilityBust[upCard] = dealer.probabilityBust[upCard];
            for (int count = 17; count <= 21; ++count) {
                hand.probabilityCount[count - 17][upCard] =
                    dealer.probabilityCount[count - 17][upCard];
            }
            hand.probabilityBlackjack[upCard] =
                dealer.probabilityBlackjack[upCard];
        }
    }
}

void BJPlayer::linkHandCounts(bool split, int pairCard, int splitHands) {
    for (int count = 4; count <= 21; ++count) {
        playerHandCount[count][false] = playerHandCount[count][true] = 0;
    }
    for (int i = 0; i < numHands; ++i) {
        currentHand.reset(playerHands[i].cards);
        bool link;

        // This would be easier if not for the fact that hitting split aces is
        // not allowed.
        if (split) {
            int numCards = currentHand.numCards - (splitHands - 1);
            link = (currentHand.cards[pairCard] >= splitHands)
                    && (currentHand.count - pairCard * (splitHands - 1) <= 21)
                    && (numCards >= 2 && (pairCard != 1 || numCards == 2));
        } else {
            link = (currentHand.count <= 21 && currentHand.numCards >= 2);
        }
        if (link) {
            for (int hands = 1; hands < splitHands; ++hands) {
                currentHand.undeal(pairCard);
            }
            int j = playerHandCount[currentHand.count][currentHand.soft];
            playerHandCount[currentHand.count][currentHand.soft] = i;
            playerHands[i].nextHand = j;
        }
    }
}

void BJPlayer::computeStand(bool split, int pairCard, int splitHands) {
    int count;

    // This particular traversal of hands isn't really necessary for standing
    // and doubling down; it is simply useful to use the already-identified
    // list of valid hands (depending on whether we are splitting, how many
    // hands, etc.).
    for (count = 21; count >= 11; --count) {
        computeStandCount(count, false, split, pairCard, splitHands);
    }
    for (count = 21; count >= 12; --count) {
        computeStandCount(count, true, split, pairCard, splitHands);
    }
    for (count = 10; count >= 4; --count) {
        computeStandCount(count, false, split, pairCard, splitHands);
    }
}

void BJPlayer::computeStandCount(int count, bool soft, bool split,
                                 int pairCard, int splitHands) {
    for (int i = playerHandCount[count][soft]; i;
            i = playerHands[i].nextHand) {
        PlayerHand & hand = playerHands[i];
        currentHand.reset(hand.cards);
        shoe.reset(currentHand);
        for (int hands = 1; hands < splitHands; ++hands) {
            currentHand.undeal(pairCard);
        }
        for (int upCard = 1; upCard <= 10; ++upCard) {
            if (shoe.cards[upCard]) {
                hand.valueStand[split][upCard] = hand.probabilityBust[upCard] -
                    hand.probabilityBlackjack[upCard];
                for (int count = 17; count <= 21; ++count) {
                    if (currentHand.count > count) {
                        hand.valueStand[split][upCard] +=
                            hand.probabilityCount[count - 17][upCard];
                    } else if (currentHand.count < count) {
                        hand.valueStand[split][upCard] -=
                            hand.probabilityCount[count - 17][upCard];
                    }
                }
            }
        }
    }
}

void BJPlayer::computeDoubleDown(bool split, int pairCard, int splitHands) {
    int count;

    for (count = 21; count >= 11; --count) {
        computeDoubleDownCount(count, false, split, pairCard, splitHands);
    }
    for (count = 21; count >= 12; --count) {
        computeDoubleDownCount(count, true, split, pairCard, splitHands);
    }
    for (count = 10; count >= 4; --count) {
        computeDoubleDownCount(count, false, split, pairCard, splitHands);
    }
}

void BJPlayer::computeDoubleDownCount(int count, bool soft, bool split,
                                      int pairCard, int splitHands) {
    for (int i = playerHandCount[count][soft]; i;
            i = playerHands[i].nextHand) {
        PlayerHand & hand = playerHands[i];
        currentHand.reset(hand.cards);
        shoe.reset(currentHand);
        for (int hands = 1; hands < splitHands; ++hands) {
            currentHand.undeal(pairCard);
        }
        for (int upCard = 1; upCard <= 10; ++upCard) {
            if (shoe.cards[upCard]) {
                shoe.deal(upCard);

                // We only lose our initial wager if the dealer has blackjack.
                if (upCard == 1) {
                    hand.valueDoubleDown[split][upCard] =
                        shoe.getProbability(10);
                } else if (upCard == 10) {
                    hand.valueDoubleDown[split][upCard] =
                        shoe.getProbability(1);
                } else {
                    hand.valueDoubleDown[split][upCard] = 0;
                }

                for (int card = 1; card <= 10; ++card) {
                    if (shoe.cards[card]) {
                        currentHand.deal(card);
                        int j = hand.hitHand[card];
                        double value;
                        if (currentHand.count <= 21) {
                            value =
                                playerHands[j].valueStand[split][upCard] * 2;
                        } else {
                            value = -2;
                        }
                        currentHand.undeal(card);
                        hand.valueDoubleDown[split][upCard] +=
                            value * shoe.getProbability(card);
                    }
                }
                shoe.undeal(upCard);
            }
        }
    }
}

void BJPlayer::computeHit(BJRules & rules, BJStrategy & strategy, bool split,
                          int pairCard, int splitHands) {
    int count;

    // By computing values for E(hit) in the proper order, we guarantee that
    // any values needed for computing the value of a given hand will be
    // available.  We start with the hard hands, but stop at hard 11, since we
    // could draw an ace to a 10, making a soft 21.
    for (count = 21; count >= 11; --count) {
        computeHitCount(count, false, rules, strategy, split, pairCard,
                splitHands);
    }
    for (count = 21; count >= 12; --count) {
        computeHitCount(count, true, rules, strategy, split, pairCard,
                splitHands);
    }
    for (count = 10; count >= 4; --count) {
        computeHitCount(count, false, rules, strategy, split, pairCard,
                splitHands);
    }
}

void BJPlayer::computeHitCount(int count, bool soft, BJRules & rules,
                               BJStrategy & strategy, bool split, int pairCard,
                               int splitHands) {
    for (int i = playerHandCount[count][soft]; i;
            i = playerHands[i].nextHand) {
        PlayerHand & hand = playerHands[i];
        currentHand.reset(hand.cards);
        shoe.reset(currentHand);
        for (int hands = 1; hands < splitHands; ++hands) {
            currentHand.undeal(pairCard);
        }
        for (int upCard = 1; upCard <= 10; ++upCard) {
            if (shoe.cards[upCard]) {
                shoe.deal(upCard);
                hand.valueHit[split][upCard] = 0;
                for (int card = 1; card <= 10; ++card) {
                    if (shoe.cards[card]) {
                        currentHand.deal(card);
                        int j = hand.hitHand[card];
                        double value, testValue;
                        if (currentHand.count <= 21) {
                            PlayerHand & hitHand = playerHands[j];
                            bool doubleDown;
                            if (split) {
                                doubleDown =
                                    rules.getDoubleAfterSplit(currentHand);
                            } else {
                                doubleDown = rules.getDoubleDown(currentHand);
                            }
                            switch (strategy.getOption(currentHand, upCard,
                                    doubleDown, false, false)) {
                            case BJ_MAX_VALUE :
                                j = findHand(currentHand);
                                if (useCDZ) {

                                    // For CDZ-, use the option maximizing
                                    // expected value for the non-split hand.
                                    testValue = playerHands[j].
                                            valueStand[false][upCard];
                                    value = hitHand.valueStand[split][upCard];
                                    if (testValue < playerHands[j].
                                            valueHit[false][upCard]) {
                                        testValue = playerHands[j].
                                                valueHit[false][upCard];
                                        value = hitHand.
                                                valueHit[split][upCard];
                                    }
                                    if (doubleDown) {
                                        if (testValue < playerHands[j].
                                            valueDoubleDown[false][upCard]) {
                                            value = hitHand.
                                                valueDoubleDown[split][upCard];
                                        }
                                    }
                                } else if (useCDP1 && splitHands > 2) {

                                    // For CDP1, use the previously-computed
                                    // optimal strategy with 1 additional pair
                                    // card removed (i.e., splitHands == 2).
                                    switch (playerHands[j].option[upCard]) {
                                    case BJ_STAND :
                                        value = hitHand.valueStand[split][upCard];
                                        break;
                                    case BJ_HIT :
                                        value = hitHand.valueHit[split][upCard];
                                        break;
                                    case BJ_DOUBLE_DOWN :
                                        value = hitHand.valueDoubleDown[split][upCard];
                                        break;
                                    default :
                                        value = 0;
                                    }
                                } else {

                                    // For CDP, use the optimal strategy for
                                    // the current number of additional pair
                                    // cards removed.
                                    value = hitHand.valueStand[split][upCard];
                                    int option = BJ_STAND;
                                    if (value < hitHand.
                                            valueHit[split][upCard]) {
                                        value = hitHand.
                                                valueHit[split][upCard];
                                        option = BJ_HIT;
                                    }
                                    if (doubleDown) {
                                        if (value < hitHand.
                                                valueDoubleDown[split][upCard]) {
                                            value = hitHand.
                                                valueDoubleDown[split][upCard];
                                            option = BJ_DOUBLE_DOWN;
                                        }
                                    }

                                    // Remember this strategy for CDP1.
                                    if (useCDP1 && splitHands == 2) {
                                        playerHands[j].option[upCard] = option;
                                    }
                                }
                                break;
                            case BJ_STAND :
                                value = hitHand.valueStand[split][upCard];
                                break;
                            case BJ_HIT :
                                value = hitHand.valueHit[split][upCard];
                                break;
                            case BJ_DOUBLE_DOWN :
                                value = hitHand.
                                        valueDoubleDown[split][upCard];
                                break;
                            default :
                                value = 0;
                            }
                        } else {
                            value = -1;
                        }
                        currentHand.undeal(card);
                        hand.valueHit[split][upCard] +=
                            value * shoe.getProbability(card);
                    }
                }
                shoe.undeal(upCard);
            }
        }
    }
}

void BJPlayer::computeSplit(BJRules & rules, BJStrategy & strategy) {

    // Compute expected values for splitting each possible pair card. See
    // http://sites.google.com/site/erfarmer/downloads/blackjack_split.pdf for
    // a description of the algorithm.
    for (int pairCard = 1; pairCard <= 10; ++pairCard) {
        int maxSplitHands = rules.getResplit(pairCard);
        if (maxSplitHands > shoe.totalCards[pairCard]) {
            maxSplitHands = shoe.totalCards[pairCard];
        }
        if (maxSplitHands >= 2) {

            // Pre-compute EV[X;a,0] and EV[P;a,0] for all required removals of
            // additional pair cards.
            int maxRemoved = (maxSplitHands - 2) * 2;
            if (maxRemoved > shoe.totalCards[pairCard] - 2) {
                maxRemoved = shoe.totalCards[pairCard] - 2;
            }
            for (int removed = 0; removed <= maxRemoved; ++removed) {
                computeEVx(rules, strategy, pairCard, removed);
            }

            // Accumulate expected value of splitting each possible number of
            // hands.
            for (int upCard = 1; upCard <= 10; ++upCard) {
                valueSplit[pairCard][upCard] = 0;
            }
            shoe.reset();
            shoe.deal(pairCard); shoe.deal(pairCard);

            // Evaluate "stopping early," splitting less than the maximum
            // number of hands.
            for (int h = 2; h < maxSplitHands; ++h) {
                getEVn(q(h, pairCard) * (double)h, h - 2, h - 1, pairCard);
            }

            // Evaluate splitting the maximum number of hands, indexed by the
            // number k of hands completed (i.e., drawing a non-pair card)
            // before drawing additional pair cards to reach the maximum.
            for (int k = 0; k < maxSplitHands - 1; ++k) {
                std::valarray<double> p = r(maxSplitHands, k, pairCard);
                if (k > 0) {
                    getEVn(p * (double)k, maxSplitHands - 2, k - 1, pairCard);
                }
                getEVx(p * (double)(maxSplitHands - k), maxSplitHands - 2, k,
                    pairCard);
            }

            // We only lose our initial wager if the dealer has blackjack.
            for (int upCard = 1; upCard <= 10; upCard *= 10) {
                int holeCard = 11 - upCard;
                if (shoe.cards[upCard] && shoe.cards[holeCard]) {
                    shoe.deal(upCard);
                    double pBlackjack = shoe.getProbability(holeCard);
                    shoe.deal(holeCard);
                    double p2, p3, p4,
                        n = shoe.numCards,
                        p = shoe.cards[pairCard];
                    if (maxSplitHands > 2) {
                        p2 = (n - p) / n * (n - p - 1) / (n - 1);
                        if (maxSplitHands > 3) {
                            p3 = 2 * p / n * (n - p) / (n - 1) * (n - p - 1) /
                                (n - 2) * (n - p - 2) / (n - 3);
                            p4 = 1 - p2 - p3;
                        } else {
                            p3 = 1 - p2;
                            p4 = 0;
                        }
                    } else {
                        p2 = 1;
                        p3 = p4 = 0;
                    }
                    double expectedHands = p2 * 2 + p3 * 3 + p4 * 4;
                    valueSplit[pairCard][upCard] +=
                        (expectedHands - 1) * pBlackjack;
                    shoe.undeal(holeCard);
                    shoe.undeal(upCard);
                }
            }
        }
    }
}

void BJPlayer::computeEVx(BJRules & rules, BJStrategy & strategy, int pairCard,
                          int removed) {

    // Re-compute expected values for all player hands, but with the given
    // number of additional pair cards removed.
    int splitHands = removed + 2;
    linkHandCounts(true, pairCard, splitHands);
    computeStand(true, pairCard, splitHands);
    if (pairCard != 1) {
        computeDoubleDown(true, pairCard, splitHands);
        computeHit(rules, strategy, true, pairCard, splitHands);
    }
    currentHand.reset();
    currentHand.deal(pairCard);

    // Remove split pair cards for weighting expected values of possible hands.
    int i = 0, j;
    shoe.reset();
    for (int split = 0; split < splitHands; ++split) {
        shoe.deal(pairCard);
        i = playerHands[i].hitHand[pairCard];
    }
    for (int upCard = 1; upCard <= 10; ++upCard) {
        valueSplitX[removed][pairCard][upCard] = 0;
        valueSplitP[removed][pairCard][upCard] = 0;
        if (shoe.cards[upCard]) {
            shoe.deal(upCard);

            // Evaluate each possible two-card hand, playing (not resplitting)
            // any new pair.
            for (int card = 1; card <= 10; ++card) {
                if (shoe.cards[card]) {
                    currentHand.deal(card);
                    PlayerHand & hand =
                        playerHands[playerHands[i].hitHand[card]];
                    double value, testValue;
                    if (pairCard == 1) {
                        value = hand.valueStand[true][upCard];
                    } else {
                        bool doubleDown =
                            rules.getDoubleAfterSplit(currentHand);
                        switch (strategy.getOption(currentHand,
                                upCard, doubleDown, false, false)){
                        case BJ_MAX_VALUE :
                            j = findHand(currentHand);
                            if (useCDZ) {

                                // Again, for CDZ-, use the playing option that
                                // maximizes expected value for the non-split
                                // hand.
                                testValue = playerHands[j].
                                        valueStand[false][upCard];
                                value = hand.valueStand[true][upCard];
                                if (testValue < playerHands[j].
                                        valueHit[false][upCard]) {
                                    testValue = playerHands[j].
                                        valueHit[false][upCard];
                                    value = hand.valueHit[true][upCard];
                                }
                                if (doubleDown) {
                                    if (testValue < playerHands[j].
                                        valueDoubleDown[false][upCard]) {
                                        value = hand.
                                            valueDoubleDown[true][upCard];
                                    }
                                }
                            } else if (useCDP1 && removed > 0) {

                                // For CDP1, use the previously-computed
                                // optimal strategy with 1 additional pair
                                // card removed (i.e., removed == 0).
                                switch (playerHands[j].option[upCard]) {
                                case BJ_STAND :
                                    value = hand.valueStand[true][upCard];
                                    break;
                                case BJ_HIT :
                                    value = hand.valueHit[true][upCard];
                                    break;
                                case BJ_DOUBLE_DOWN :
                                    value = hand.valueDoubleDown[true][upCard];
                                    break;
                                default :
                                    value = 0;
                                }
                            } else {

                                // For CDP, use the optimal strategy for
                                // the current number of additional pair
                                // cards removed.
                                value = hand.valueStand[true][upCard];
                                int option = BJ_STAND;
                                if (value < hand.valueHit[true][upCard]) {
                                    value = hand.valueHit[true][upCard];
                                    option = BJ_HIT;
                                }
                                if (doubleDown) {
                                    if (value < hand.
                                        valueDoubleDown[true][upCard]) {
                                        value =
                                            hand.valueDoubleDown[true][upCard];
                                        option = BJ_DOUBLE_DOWN;
                                    }
                                }

                                // Remember this strategy for CDP1.
                                if (removed == 0) {
                                    playerHands[j].option[upCard] = option;
                                }
                            }
                            break;
                        case BJ_STAND :
                            value = hand.valueStand[true][upCard];
                            break;
                        case BJ_HIT :
                            value = hand.valueHit[true][upCard];
                            break;
                        case BJ_DOUBLE_DOWN :
                            value = hand.valueDoubleDown[true][upCard];
                            break;
                        default :
                            value = 0;
                        }
                    }

                    // Accumulate EV[X;a,0] and EV[P;a,0] for this two-card
                    // hand.
                    valueSplitX[removed][pairCard][upCard] +=
                        value * shoe.getProbability(card);
                    if (card == pairCard) {
                        valueSplitP[removed][pairCard][upCard] = value;
                    }
                    currentHand.undeal(card);
                }
            }
            shoe.undeal(upCard);
        }
    }
}

std::valarray<double> BJPlayer::q(int h, int pairCard) {

    // Compute the probability of splitting exactly h hands.  The number of
    // arrangements of cards is the (h - 1)-st Catalan number; this simpler
    // expression C(h - 1) == h - 1 is only valid for 1 < h < 4.
    std::valarray<double> v(h - 1.0, 11);
    for (int upCard = 1; upCard <= 10; ++upCard) {
        if (shoe.cards[upCard]) {
            shoe.deal(upCard);

            // Compute the (constant) probability of each arrangement.
            double n = shoe.numCards,
                p = shoe.cards[pairCard],
                np = n - p;
            for (int i = 0; i < h - 2; ++i, --p, --n) {
                v[upCard] *= p / n;
            }
            for (int i = 0; i < h; ++i, --np, --n) {
                v[upCard] *= np / n;
            }
            shoe.undeal(upCard);
        } else {
            v[upCard] = 0;
        }
    }
    return v;
}

std::valarray<double> BJPlayer::r(int maxSplitHands, int k, int pairCard) {

    // Compute the probability of splitting the maximum number of hands,
    // completing k of them (i.e., drawing a non-pair card) before drawing
    // additional pair cards to reach the maximum.
    //
    // The number m of arrangements of cards is:
    // (1 - k / (n - 1)) * C(n + k - 2, k); this simpler expression for m is
    // only valid for maxSplitHands <= 4.
    double m = 1;
    if (maxSplitHands == 4 && k > 0) {
        m = 2;
    }
    std::valarray<double> v(m, 11);
    for (int upCard = 1; upCard <= 10; ++upCard) {
        if (shoe.cards[upCard]) {
            shoe.deal(upCard);

            // Compute the (constant) probability of each arrangement.
            double n = shoe.numCards,
                p = shoe.cards[pairCard],
                np = n - p;
            for (int i = 0; i < maxSplitHands - 2; ++i, --p, --n) {
                v[upCard] *= p / n;
            }
            for (int i = 0; i < k; ++i, --np, --n) {
                v[upCard] *= np / n;
            }
            shoe.undeal(upCard);
        } else {
            v[upCard] = 0;
        }
    }
    return v;
}

void BJPlayer::getEVx(std::valarray<double> p, int pairRemoved,
                      int nonPairRemoved, int pairCard) {
    if (nonPairRemoved == 0) {
        for (int upCard = 1; upCard <= 10; ++upCard) {
            valueSplit[pairCard][upCard] +=
                p[upCard] * valueSplitX[pairRemoved][pairCard][upCard];
        }
        return;
    }

    // Recursively evaluate EV[X;a,b] in terms of EV[X;a,b-1] and
    // EV[X;a+1,b-1].
    std::valarray<double> v((double)(shoe.cards[pairCard] - pairRemoved), 11);
    v[pairCard] -= 1;
    v /= shoe.numCards - pairRemoved - nonPairRemoved;
    for (int upCard = 1; upCard <= 10; ++upCard) {
        if (v[upCard] >= 1) {
            p[upCard] = v[upCard] = 0;
        }
    }
    getEVx(p / (1.0 - v), pairRemoved, nonPairRemoved - 1, pairCard);
    getEVx(p * v / (v - 1.0), pairRemoved + 1, nonPairRemoved - 1, pairCard);
}

void BJPlayer::getEVn(std::valarray<double> p, int pairRemoved,
                      int nonPairRemoved, int pairCard) {
    std::valarray<double> v((double)(shoe.cards[pairCard] - pairRemoved), 11);
    v[pairCard] -= 1;
    v /= shoe.numCards - 1 - pairRemoved - nonPairRemoved;
    for (int upCard = 1; upCard <= 10; ++upCard) {
        if (v[upCard] >= 1) {
            p[upCard] = v[upCard] = 0;
        }
    }
    if (nonPairRemoved == 0) {

        // Evaluate EV[N;a,0] in terms of EV[X;a,0] and EV[P;a,0].
        p /= 1.0 - v;
        for (int upCard = 1; upCard <= 10; ++upCard) {
            valueSplit[pairCard][upCard] +=
                p[upCard] * (valueSplitX[pairRemoved][pairCard][upCard] -
                    v[upCard] * valueSplitP[pairRemoved][pairCard][upCard]);
        }
        return;
    }

    // Recursively evaluate EV[N;a,b] in terms of EV[N;a,b-1] and
    // EV[N;a+1,b-1].
    getEVn(p / (1.0 - v), pairRemoved, nonPairRemoved - 1, pairCard);
    getEVn(p * v / (v - 1.0), pairRemoved + 1, nonPairRemoved - 1, pairCard);
}

void BJPlayer::correctStandBlackjack(double bjPayoff) {
    if (shoe.totalCards[1] && shoe.totalCards[10]) {
        currentHand.reset();
        currentHand.deal(1); currentHand.deal(10);
        PlayerHand & hand = playerHands[findHand(currentHand)];
        shoe.reset(currentHand);
        if (shoe.cards[1]) {
            shoe.deal(1);
            hand.valueStand[false][1] =
                bjPayoff * (1 - shoe.getProbability(10));
            shoe.undeal(1);
        }
        for (int upCard = 2; upCard < 10; ++upCard) {
            if (shoe.cards[upCard]) {
                hand.valueStand[false][upCard] = bjPayoff;
            }
        }
        if (shoe.cards[10]) {
            shoe.deal(10);
            hand.valueStand[false][10] =
                bjPayoff * (1 - shoe.getProbability(1));
            shoe.undeal(10);
        }
    }
}

void BJPlayer::computeOverall(BJRules & rules, BJStrategy & strategy) {
    overallValue = 0;
    bool surrender = rules.getLateSurrender();
    shoe.reset();
    for (int upCard = 1; upCard <= 10; ++upCard) {
        overallValues[upCard] = 0;
        if (shoe.cards[upCard]) {
            shoe.deal(upCard);
            for (int card1 = 1; card1 <= 10; ++card1) {
                for (int card2 = 1; card2 <= 10; ++card2) {
                    if (shoe.cards[card1] && shoe.cards[card2] &&
                            (card1 != card2 || shoe.cards[card1] >= 2)) {
                        currentHand.reset();
                        double p = shoe.getProbability(card1);
                        shoe.deal(card1); currentHand.deal(card1);
                        p *= shoe.getProbability(card2);
                        shoe.deal(card2); currentHand.deal(card2);
                        PlayerHand & hand = playerHands[findHand(currentHand)];
                        double value;
                        BJHand testHand(hand.cards);
                        bool doubleDown = rules.getDoubleDown(testHand),
                            split = (card1 == card2 &&
                                rules.getResplit(card1) >= 2);
                        double valueSurrender;
                        switch (strategy.getOption(testHand, upCard,
                                doubleDown, split, surrender)) {
                        case BJ_MAX_VALUE :
                            value = hand.valueStand[false][upCard];
                            if (value < hand.valueHit[false][upCard]) {
                                value = hand.valueHit[false][upCard];
                            }
                            if (doubleDown) {
                                if (value < hand.
                                        valueDoubleDown[false][upCard]) {
                                    value = hand.
                                        valueDoubleDown[false][upCard];
                                }
                            }
                            if (split) {
                                if (value < valueSplit[card1][upCard]) {
                                    value = valueSplit[card1][upCard];
                                }
                            }
                            if (surrender) {
                                valueSurrender = computeSurrender(upCard);
                                if (value < valueSurrender) {
                                    value = valueSurrender;
                                }
                            }
                            break;
                        case BJ_STAND :
                            value = hand.valueStand[false][upCard];
                            break;
                        case BJ_HIT :
                            value = hand.valueHit[false][upCard];
                            break;
                        case BJ_DOUBLE_DOWN :
                            value = hand.valueDoubleDown[false][upCard];
                            break;
                        case BJ_SPLIT :
                            value = valueSplit[card1][upCard];
                            break;
                        case BJ_SURRENDER :
                            value = computeSurrender(upCard);
                            break;
                        }
                        overallValues[upCard] += value * p;
                        shoe.undeal(card2);
                        shoe.undeal(card1);
                    }
                }
            }
            shoe.undeal(upCard);
            overallValue +=
                overallValues[upCard] * shoe.getProbability(upCard);
        }
    }
}

double BJPlayer::computeSurrender(int upCard) {
    double valueSurrender;

    if (upCard == 1) {
        valueSurrender = shoe.getProbability(10);
    } else if (upCard == 10) {
        valueSurrender = shoe.getProbability(1);
    } else {
        valueSurrender = 0;
    }
    valueSurrender = -0.5 - valueSurrender / 2;
    return valueSurrender;
}

void BJPlayer::conditionNoBlackjack(BJRules & rules) {
    for (int i = 0; i < numHands; ++i) {
        PlayerHand & hand = playerHands[i];
        currentHand.reset(hand.cards);
        if (currentHand.count <= 21) {
            shoe.reset(currentHand);
            if (shoe.cards[1]) {
                shoe.deal(1);
                double p = 1 - shoe.getProbability(10);
                if (p > 0) {
                    if (currentHand.numCards == 2 && currentHand.count == 21) {
                        hand.valueStand[false][1] /= p;
                    } else {
                        hand.valueStand[false][1] =
                            (hand.valueStand[false][1] + 1 - p) / p;
                    }
                    hand.valueHit[false][1] =
                        (hand.valueHit[false][1] + 1 - p) / p;
                    hand.valueDoubleDown[false][1] =
                        (hand.valueDoubleDown[false][1] + 1 - p) / p;
                }
                shoe.undeal(1);
            }
            if (shoe.cards[10]) {
                shoe.deal(10);
                double p = 1 - shoe.getProbability(1);
                if (p > 0) {
                    if (currentHand.numCards == 2 && currentHand.count == 21) {
                        hand.valueStand[false][10] /= p;
                    } else {
                        hand.valueStand[false][10] =
                            (hand.valueStand[false][10] + 1 - p) / p;
                    }
                    hand.valueHit[false][10] =
                        (hand.valueHit[false][10] + 1 - p) / p;
                    hand.valueDoubleDown[false][10] =
                        (hand.valueDoubleDown[false][10] + 1 - p) / p;
                }
                shoe.undeal(10);
            }
        }
    }
    for (int pairCard = 1; pairCard <= 10; ++pairCard) {
        if (rules.getResplit(pairCard) >= 2 &&
            shoe.totalCards[pairCard] >= 2) {
            shoe.reset();
            shoe.deal(pairCard); shoe.deal(pairCard);
            if (shoe.cards[1]) {
                shoe.deal(1);
                double p = 1 - shoe.getProbability(10);
                if (p > 0) {
                    valueSplit[pairCard][1] =
                            (valueSplit[pairCard][1] + 1 - p) / p;
                }
                shoe.undeal(1);
            }
            if (shoe.cards[10]) {
                shoe.deal(10);
                double p = 1 - shoe.getProbability(1);
                if (p > 0) {
                    valueSplit[pairCard][10] =
                            (valueSplit[pairCard][10] + 1 - p) / p;
                }
                shoe.undeal(10);
            }
        }
    }
}
