///////////////////////////////////////////////////////////////////////////////
//
// blackjack.cpp
// Copyright (C) 2006 Eric Farmer (see gpl.txt for details)
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
    return cards[card - 1];
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
    for (int card = 1; card <= 10; card++) {
        cards[card - 1] = 0;
    }
    numCards = count = 0;
    soft = false;
}

void BJHand::reset(const int cards[]) {
    numCards = count = 0;
    for (int card = 1; card <= 10; card++) {
        this->cards[card - 1] = cards[card - 1];
        numCards += cards[card - 1];
        count += card*cards[card - 1];
    }
    if (count < 12 && cards[0]) {
        count += 10;
        soft = true;
    } else {
        soft = false;
    }
}

void BJHand::deal(int card) {
    cards[card - 1]++;
    numCards++;
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
    cards[card - 1]--;
    numCards--;
    count -= card;
    if (card == 1 && !cards[0] && soft) {
        count -= 10;
        soft = false;
    } else if (count < 12 && cards[0] && !soft) {
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
    return cards[card - 1];
}

int BJShoe::getCards() const {
    return numCards;
}

double BJShoe::getProbability(int card) const {
    return (double)cards[card - 1]/numCards;
}

void BJShoe::reset() {
    numCards = 0;
    for (int card = 1; card <= 10; card++) {
        cards[card - 1] = totalCards[card - 1];
        numCards += cards[card - 1];
    }
}

void BJShoe::reset(const BJHand & hand) {
    numCards = 0;
    for (int card = 1; card <= 10; card++) {
        cards[card - 1] = totalCards[card - 1] - hand.cards[card - 1];
        numCards += cards[card - 1];
    }
}

void BJShoe::reset(int numDecks) {
    for (int card = 1; card < 10; card++) {
        cards[card - 1] = totalCards[card - 1] = 4*numDecks;
    }
    cards[9] = totalCards[9] = 16*numDecks;
    numCards = 52*numDecks;
}

void BJShoe::reset(const int cards[]) {
    numCards = 0;
    for (int card = 1; card <= 10; card++) {
        this->cards[card - 1] = totalCards[card - 1] = cards[card - 1];
        numCards += cards[card - 1];
    }
}

void BJShoe::deal(int card) {
    cards[card - 1]--;
    numCards--;
}

void BJShoe::undeal(int card) {
    cards[card - 1]++;
    numCards++;
}

///////////////////////////////////////////////////////////////////////////////
//
// BJDealer
//

BJDealer::BJDealer(bool hitSoft17) {
    this->hitSoft17 = hitSoft17;
    for (int count = 17; count <= 21; count++) {
        dealerHandCount[count - 17].numHands = 0;
    }

    // Enumerate all possible dealer hands, counting multiplicities for each up
    // card.
    currentHand.reset();
    for (upCard = 1; upCard <= 10; upCard++) {
        currentHand.deal(upCard);
        countHands();
        currentHand.undeal(upCard);
    }

    for (upCard = 1; upCard <= 10; upCard++) {
        probabilityBlackjack[upCard - 1] = 0;
    }
}

// max*Values[c] are the largest values of s and h for which we need to compute
// lookup[][c][].
//
// lookup[s][c][h] is the probability of h + 1 consecutive cards with value
// c + 1 being dealt from the shoe, given that s other cards have been removed
// from the shoe, and is equal to
// f(shoe.cards[c], h + 1)/f(shoe.numCards - s, h + 1), where f is the falling
// factorial function.
const int BJDealer::maxSvalues[10] = {0, 11, 11, 11, 12, 11, 10, 9, 8, 7};
const int BJDealer::maxHvalues[10] = {11, 8, 5, 4, 3, 2, 2, 1, 1, 1};

void BJDealer::computeProbabilities(const BJShoe & shoe) {

    // Compute lookup[][][].
    for (upCard = 1; upCard <= 10; upCard++) {
        int maxS = maxSvalues[upCard - 1],
            maxH = maxHvalues[upCard - 1],
            numCards = shoe.numCards;
        for (int s = 0; s <= maxS && numCards; s++, numCards--) {
            double *l = lookup[s][upCard - 1];
            int cards = shoe.cards[upCard - 1],
                n = numCards;
            double p = l[0] = (double)cards--/n--;
            for (int h = 1; h <= maxH && cards; h++) {
                p *= (double)cards--/n--;
                l[h] = p;
            }
        }
    }

    // probabilityBust[] will accumulate the probability of NOT busting, so we
    // don't have to actually count those hands.
    for (upCard = 1; upCard <= 10; upCard++) {
        probabilityBust[upCard - 1] = 0;
    }

    // For each (non-bust, non-blackjack) possible outcome, accumulate
    // probability of each hand with that count (that is actually possible in
    // the given shoe).
    for (int count = 17; count <= 21; count++) {
        double *pcount = probabilityCount[count - 17];
        for (upCard = 1; upCard <= 10; upCard++) {
            pcount[upCard - 1] = 0;
        }
        DealerHandCount & list = dealerHandCount[count - 17];
        for (int i = 0; i < list.numHands; i++) {
            DealerHand & hand = list.dealerHands[i];
            bool possible = true;
            for (int card = 1; card <= 10; card++) {
                if (hand.cards[card - 1] > shoe.cards[card - 1]) {
                    possible = false;
                    break;
                }
            }

            // If it is possible (i.e., a subset of the shoe), compute
            // probability of the hand.  Note the initial value of p; we save
            // 10 multiplications in the next step by doing part of the
            // "conditioning" here.
            if (possible) {
                int s = 0;
                double p = shoe.numCards;
                for (int card = 1; card <= 10; card++) {
                    if (hand.cards[card - 1]) {
                        p *= lookup[s][card - 1][hand.cards[card - 1] - 1];
                        s += hand.cards[card - 1];
                    }
                }

                // For each up card, a certain number of permutations of the
                // cards in the hand are possible, each equally likely.  Count
                // these, conditioned on each up card.
                for (upCard = 1; upCard <= 10; upCard++) {
                    if (hand.multiplier[upCard - 1]) {
                        pcount[upCard - 1] += p*hand.multiplier[upCard - 1]
                                /shoe.cards[upCard - 1];
                    }
                }
            }
        }
        for (upCard = 1; upCard <= 10; upCard++) {
            probabilityBust[upCard - 1] += pcount[upCard - 1];
        }
    }

    // Compute P(blackjack).
    if (shoe.cards[0] && shoe.cards[9]) {
        probabilityBlackjack[0] = (double)(shoe.cards[9])/(shoe.numCards - 1);
        probabilityBust[0] += probabilityBlackjack[0];
        probabilityBlackjack[9] = (double)(shoe.cards[0])/(shoe.numCards - 1);
        probabilityBust[9] += probabilityBlackjack[9];
    } else {
        probabilityBlackjack[0] = probabilityBlackjack[9] = 0;
    }

    // Now compute P(bust) easily.
    for (upCard = 1; upCard <= 10; upCard++) {
        probabilityBust[upCard - 1] = (double)1 - probabilityBust[upCard - 1];
        probabilityCard[upCard - 1] = shoe.getProbability(upCard);
    }
}

double BJDealer::getProbabilityBust(int upCard) const {
    return probabilityBust[upCard - 1];
}

double BJDealer::getProbabilityBust() const {
    double p = 0;
    for (int upCard = 1; upCard <= 10; upCard++) {
        p += probabilityBust[upCard - 1]*probabilityCard[upCard - 1];
    }
    return p;
}

double BJDealer::getProbabilityCount(int count, int upCard) const {
    return probabilityCount[count - 17][upCard - 1];
}

double BJDealer::getProbabilityCount(int count) const {
    double p = 0;
    for (int upCard = 1; upCard <= 10; upCard++) {
        p += probabilityCount[count - 17][upCard - 1]
                *probabilityCard[upCard - 1];
    }
    return p;
}

double BJDealer::getProbabilityBlackjack(int upCard) const {
    return probabilityBlackjack[upCard - 1];
}

double BJDealer::getProbabilityBlackjack() const {
    return (probabilityBlackjack[0]*probabilityCard[0] +
            probabilityBlackjack[9]*probabilityCard[9]);
}

void BJDealer::countHands() {

    // If necessary, draw another card.
    if (currentHand.count < 17 || (hitSoft17
            && currentHand.count == 17 && currentHand.soft)) {
        for (int card = 1; card <= 10; card++) {
            currentHand.deal(card);
            countHands();
            currentHand.undeal(card);
        }

    // Otherwise, record all non-bust, non-blackjack hands.
    } else if (currentHand.count <= 21 && (currentHand.numCards != 2
            || currentHand.count != 21)) {
        DealerHandCount & list = dealerHandCount[currentHand.count - 17];
        bool found = false;
        for (int i = 0; i < list.numHands; i++) {
            DealerHand & hand = list.dealerHands[i];
            bool match = true;
            for (int card = 1; card <= 10; card++) {
                if (currentHand.cards[card - 1] != hand.cards[card - 1]) {
                    match = false;
                    break;
                }
            }
            if (match) {
                hand.multiplier[upCard - 1]++;
                found = true;
                break;
            }
        }
        if (!found) {
            DealerHand & hand = list.dealerHands[list.numHands];
            for (int card = 1; card <= 10; card++) {
                hand.cards[card - 1] = currentHand.cards[card - 1];
                hand.multiplier[card - 1] = 0;
            }
            hand.multiplier[upCard - 1]++;
            list.numHands++;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//
// BJRules
//

BJRules::BJRules() {
    hitSoft17 = false;
    doubleAnyTotal = true;
    double9 = true;
    doubleSoft = true;
    doubleAfterHit = false;
    doubleAfterSplit = true;
    resplit = true;
    resplitAces = false;
    lateSurrender = false;
}

BJRules::BJRules(bool hitSoft17, bool doubleAnyTotal, bool double9,
                 bool doubleSoft, bool doubleAfterHit, bool doubleAfterSplit,
                 bool resplit, bool resplitAces, bool lateSurrender) {
    this->hitSoft17 = hitSoft17;
    this->doubleAnyTotal = doubleAnyTotal;
    this->double9 = double9;
    this->doubleSoft = doubleSoft;
    this->doubleAfterHit = doubleAfterHit;
    this->doubleAfterSplit = doubleAfterSplit;
    this->resplit = resplit;
    this->resplitAces = resplitAces;
    this->lateSurrender = lateSurrender;
}

BJRules::~BJRules() {
}

bool BJRules::getHitSoft17() {
    return hitSoft17;
}

bool BJRules::getDoubleDown(const BJHand & hand) {
    return (
        (doubleAnyTotal || hand.getCount() == 10 || hand.getCount() == 11 ||
            (double9 && hand.getCount() == 9)) &&
        (doubleSoft || !hand.getSoft()) &&
        (doubleAfterHit || hand.getCards() == 2));
}

bool BJRules::getDoubleAfterSplit(const BJHand & hand) {
    return (doubleAfterSplit && getDoubleDown(hand));
}

int BJRules::getResplit(int pairCard) {
    return ((resplit && (resplitAces || pairCard != 1)) ? 4 : 2);
}

bool BJRules::getLateSurrender() {
    return lateSurrender;
}

///////////////////////////////////////////////////////////////////////////////
//
// BJStrategy
//

BJStrategy::~BJStrategy() {
}

int BJStrategy::getOption(const BJHand & hand, int upCard, bool doubleDown,
                          bool split, bool surrender) {
    return BJ_MAX_VALUE;
}

///////////////////////////////////////////////////////////////////////////////
//
// BJProgress
//

BJProgress::~BJProgress() {
}

void BJProgress::indicate(int percentComplete) {
}

///////////////////////////////////////////////////////////////////////////////
//
// BJPlayer
//

BJPlayer::BJPlayer(const BJShoe & shoe, BJRules & rules, BJStrategy & strategy,
                   BJProgress & progress) {
    reset(shoe, rules, strategy, progress);
}

void BJPlayer::reset(const BJShoe & shoe, BJRules & rules,
                     BJStrategy & strategy, BJProgress & progress) {

    // Forget about any cards already dealt from the shoe, so shoe.reset(hand)
    // will work.
    this->shoe = shoe;
    numHands = 0;
    for (int card = 1; card <= 10; card++) {
        playerHands[numHands].cards[card - 1] =
        playerHands[numHands].hitHand[card - 1] = 0;
        this->shoe.totalCards[card - 1] = shoe.cards[card - 1];
    }

    // Remember resplit rules when enumerating player hands.
    for (int pairCard = 1; pairCard <= 10; pairCard++) {
        resplit[pairCard - 1] = rules.getResplit(pairCard);
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

    // Compute expected values for splitting pairs.  Re-link original hands by
    // count for future use.
    computeSplit(rules, strategy);
    linkHandCounts();

    // Blackjack pays 3:2, so correct the value for standing on this hand.  We
    // wait to do this until after computing E(split) since a blackjack after
    // splitting a pair only pays even money.
    correctStandBlackjack();

    // Compute overall expected values, condition individual hands on no dealer
    // blackjack, and finalize progress indicator.
    computeOverall(rules, strategy);
    conditionNoBlackjack();
    progress.indicate(100);
}

double BJPlayer::getValueStand(const BJHand & hand, int upCard) const {
    return playerHands[findHand(hand)].valueStand[false][upCard - 1];
}

double BJPlayer::getValueHit(const BJHand & hand, int upCard) const {
    return playerHands[findHand(hand)].valueHit[false][upCard - 1];
}

double BJPlayer::getValueDoubleDown(const BJHand & hand, int upCard) const {
    return playerHands[findHand(hand)].valueDoubleDown[false][upCard - 1];
}

double BJPlayer::getValueSplit(int pairCard, int upCard) const {
    return valueSplit[pairCard - 1][upCard - 1];
}

double BJPlayer::getValue(int upCard) const {
    return overallValues[upCard - 1];
}

double BJPlayer::getValue() const {
    return overallValue;
}

int BJPlayer::getOption(const BJHand & hand, int upCard, bool doubleDown,
                        bool split, bool surrender) {
    PlayerHand & testHand = playerHands[findHand(hand)];
    double value = testHand.valueStand[false][upCard - 1];
    int option = BJ_STAND;
    if (value < testHand.valueHit[false][upCard - 1]) {
        value = testHand.valueHit[false][upCard - 1];
        option = BJ_HIT;
    }
    if (doubleDown) {
        if (value < testHand.valueDoubleDown[false][upCard - 1]) {
            value = testHand.valueDoubleDown[false][upCard - 1];
            option = BJ_DOUBLE_DOWN;
        }
    }
    if (split) {
        int pairCard = 1;
        while (!hand.cards[pairCard - 1]) {
            pairCard++;
        }
        if (value < valueSplit[pairCard - 1][upCard - 1]) {
            value = valueSplit[pairCard - 1][upCard - 1];
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
    for (int card = 1; card <= 10; card++) {
        for (int c = 0; c < hand.cards[card - 1]; c++) {
            i = playerHands[i].hitHand[card - 1];
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

        // Or if it may be a split hand; note that we don't need extra hands for split
        // aces, since hitting split aces is not allowed.
        for (int card = 2; card <= 10; card++) {
            int s = resplit[card - 1];
            if (hand.cards[card - 1] < s) {
                s = hand.cards[card - 1];
            }
            if (hand.count - card*(s - 1) <= 21) {
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
    for (int card = maxCard; card <= 10; card++) {
        if (shoe.cards[card - 1]) {
            shoe.deal(card);
            currentHand.deal(card);

            // If the hand is not busted (or could be a split hand), record it
            // and partially link it up; we'll finish with linkHands() later.
            if (record(currentHand)) {
                playerHands[i].hitHand[card - 1] = numHands;
                for (int c = 1; c <= 10; c++) {
                    playerHands[numHands].cards[c - 1] = currentHand.
                            cards[c - 1];
                    playerHands[numHands].hitHand[c - 1] = 0;
                }
                countHands(numHands++, card);
            }
            currentHand.undeal(card);
            shoe.undeal(card);
        }
    }
}

void BJPlayer::linkHands() {
    for (int i = 0; i < numHands; i++) {
        PlayerHand & hand = playerHands[i];
        for (int card = 1; card <= 10; card++) {
            if (!hand.hitHand[card - 1]
                    && hand.cards[card - 1] < shoe.cards[card - 1]) {
                currentHand.reset(hand.cards);
                currentHand.deal(card);
                if (record(currentHand)) {
                    hand.hitHand[card - 1] = findHand(currentHand);
                }
            }
        }
    }
}

void BJPlayer::computeDealer(BJRules & rules, BJProgress & progress) {
    BJDealer dealer(rules.getHitSoft17());
    for (int i = 0; i < numHands; i++) {
        progress.indicate(100*i/numHands);
        PlayerHand & hand = playerHands[i];
        currentHand.reset(hand.cards);
        shoe.reset(currentHand);
        dealer.computeProbabilities(shoe);
        for (int upCard = 1; upCard <= 10; upCard++) {
            hand.probabilityBust[upCard - 1] = dealer.
                    probabilityBust[upCard - 1];
            for (int count = 17; count <= 21; count++) {
                hand.probabilityCount[count - 17][upCard - 1] = dealer.
                        probabilityCount[count - 17][upCard - 1];
            }
            hand.probabilityBlackjack[upCard - 1] = dealer.
                    probabilityBlackjack[upCard - 1];
        }
    }
}

void BJPlayer::linkHandCounts(bool split, int pairCard, int splitHands) {
    for (int count = 4; count <= 21; count++) {
        playerHandCount[count][false] = playerHandCount[count][true] = 0;
    }
    for (int i = 0; i < numHands; i++) {
        currentHand.reset(playerHands[i].cards);
        bool link;

        // This would be easier if not for the fact that hitting split aces is
        // not allowed.
        if (split) {
            int numCards = currentHand.numCards - (splitHands - 1);
            link = (currentHand.cards[pairCard - 1] >= splitHands)
                    && (currentHand.count - pairCard*(splitHands - 1) <= 21)
                    && (numCards >= 2 && (pairCard != 1 || numCards == 2));
        } else {
            link = (currentHand.count <= 21 && currentHand.numCards >= 2);
        }
        if (link) {
            for (int hands = 1; hands < splitHands; hands++) {
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
    for (count = 21; count >= 11; count--) {
        computeStandCount(count, false, split, pairCard, splitHands);
    }
    for (count = 21; count >= 12; count--) {
        computeStandCount(count, true, split, pairCard, splitHands);
    }
    for (count = 10; count >= 4; count--) {
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
        for (int hands = 1; hands < splitHands; hands++) {
            currentHand.undeal(pairCard);
        }
        for (int upCard = 1; upCard <= 10; upCard++) {
            if (shoe.cards[upCard - 1]) {
                hand.valueStand[split][upCard - 1] =
                        hand.probabilityBust[upCard - 1]
                        - hand.probabilityBlackjack[upCard - 1];
                for (int count = 17; count <= 21; count++) {
                    if (currentHand.count > count) {
                        hand.valueStand[split][upCard - 1] +=
                            hand.probabilityCount[count - 17][upCard - 1];
                    } else if (currentHand.count < count) {
                        hand.valueStand[split][upCard - 1] -=
                            hand.probabilityCount[count - 17][upCard - 1];
                    }
                }
            }
        }
    }
}

void BJPlayer::computeDoubleDown(bool split, int pairCard, int splitHands) {
    int count;

    for (count = 21; count >= 11; count--) {
        computeDoubleDownCount(count, false, split, pairCard, splitHands);
    }
    for (count = 21; count >= 12; count--) {
        computeDoubleDownCount(count, true, split, pairCard, splitHands);
    }
    for (count = 10; count >= 4; count--) {
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
        for (int hands = 1; hands < splitHands; hands++) {
            currentHand.undeal(pairCard);
        }
        for (int upCard = 1; upCard <= 10; upCard++) {
            if (shoe.cards[upCard - 1]) {
                shoe.deal(upCard);

                // We only lose our initial wager if the dealer has blackjack.
                if (upCard == 1) {
                    hand.valueDoubleDown[split][upCard - 1] = shoe.
                            getProbability(10);
                } else if (upCard == 10) {
                    hand.valueDoubleDown[split][upCard - 1] = shoe.
                            getProbability(1);
                } else {
                    hand.valueDoubleDown[split][upCard - 1] = 0;
                }

                for (int card = 1; card <= 10; card++) {
                    if (shoe.cards[card - 1]) {
                        currentHand.deal(card);
                        int j = hand.hitHand[card - 1];
                        double value;
                        if (currentHand.count <= 21) {
                            value = playerHands[j].
                                    valueStand[split][upCard - 1]*2;
                        } else {
                            value = -2;
                        }
                        currentHand.undeal(card);
                        hand.valueDoubleDown[split][upCard - 1] += value
                                *shoe.getProbability(card);
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
    for (count = 21; count >= 11; count--) {
        computeHitCount(count, false, rules, strategy, split, pairCard,
                splitHands);
    }
    for (count = 21; count >= 12; count--) {
        computeHitCount(count, true, rules, strategy, split, pairCard,
                splitHands);
    }
    for (count = 10; count >= 4; count--) {
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
        for (int hands = 1; hands < splitHands; hands++) {
            currentHand.undeal(pairCard);
        }
        for (int upCard = 1; upCard <= 10; upCard++) {
            if (shoe.cards[upCard - 1]) {
                shoe.deal(upCard);
                hand.valueHit[split][upCard - 1] = 0;
                for (int card = 1; card <= 10; card++) {
                    if (shoe.cards[card - 1]) {
                        currentHand.deal(card);
                        int j = hand.hitHand[card - 1];
                        double value, testValue;
                        if (currentHand.count <= 21) {
                            PlayerHand & hitHand = playerHands[j];
                            bool doubleDown;
                            if (split) {
                                doubleDown = rules.
                                        getDoubleAfterSplit(currentHand);
                            } else {
                                doubleDown = rules.getDoubleDown(currentHand);
                            }
                            switch (strategy.getOption(currentHand, upCard,
                                    doubleDown, false, false)) {

                            // To be consistent with a "fixed" playing
                            // strategy, use the option maximizing expected
                            // value for the non-split hand.
                            case BJ_MAX_VALUE :
                                j = findHand(currentHand);
                                testValue = playerHands[j].
                                        valueStand[false][upCard - 1];
                                value = hitHand.valueStand[split][upCard - 1];
                                if (testValue < playerHands[j].
                                        valueHit[false][upCard - 1]) {
                                    testValue = playerHands[j].
                                            valueHit[false][upCard - 1];
                                    value = hitHand.
                                            valueHit[split][upCard - 1];
                                }
                                if (doubleDown) {
                                    if (testValue < playerHands[j].
                                          valueDoubleDown[false][upCard - 1]) {
                                        value = hitHand.
                                            valueDoubleDown[split][upCard - 1];
                                    }
                                }
                                break;
                            case BJ_STAND :
                                value = hitHand.valueStand[split][upCard - 1];
                                break;
                            case BJ_HIT :
                                value = hitHand.valueHit[split][upCard - 1];
                                break;
                            case BJ_DOUBLE_DOWN :
                                value = hitHand.
                                        valueDoubleDown[split][upCard - 1];
                                break;
                            default :
                                value = 0;
                            }
                        } else {
                            value = -1;
                        }
                        currentHand.undeal(card);
                        hand.valueHit[split][upCard - 1] += value
                                *shoe.getProbability(card);
                    }
                }
                shoe.undeal(upCard);
            }
        }
    }
}

void BJPlayer::computeSplit(BJRules & rules, BJStrategy & strategy) {
    for (int pairCard = 1; pairCard <= 10; pairCard++) {
        if (resplit[pairCard - 1] >= 2 && shoe.totalCards[pairCard - 1] >= 2) {

            // Compute maximum number of split hands.
            int maxSplitHands = resplit[pairCard - 1];
            if (shoe.totalCards[pairCard - 1] < maxSplitHands) {
                maxSplitHands = shoe.totalCards[pairCard - 1];
            }

            // Compute probability of splitting exactly 2, 3, and 4 hands.
            double pSplit[5][10];
            shoe.reset();
            shoe.deal(pairCard); shoe.deal(pairCard);
            for (int upCard = 1; upCard <= 10; upCard++) {
                if (shoe.cards[upCard - 1]) {
                    shoe.deal(upCard);
                    double n = shoe.numCards,
                        p = shoe.cards[pairCard - 1];
                    if (maxSplitHands > 2) {
                        pSplit[2][upCard - 1] = (n - p)/n*(n - 1 - p)/(n - 1);
                        if (maxSplitHands > 3) {
                            pSplit[3][upCard - 1] = pSplit[2][upCard - 1]*2
                                    *p/(n - 2)*(n - 2 - p)/(n - 3);
                            pSplit[4][upCard - 1] = (double)1
                                    - pSplit[2][upCard - 1]
                                    - pSplit[3][upCard - 1];
                        } else {
                            pSplit[3][upCard - 1] = (double)1
                                    - pSplit[2][upCard - 1];
                            pSplit[4][upCard - 1] = 0;
                        }
                    } else {
                        pSplit[2][upCard - 1] = 1;
                        pSplit[3][upCard - 1] = pSplit[4][upCard - 1] = 0;
                    }

                    // We only lose our initial wager if the dealer has
                    // blackjack.
                    if (upCard == 1) {
                        valueSplit[pairCard - 1][upCard - 1] = shoe.
                                getProbability(10);
                    } else if (upCard == 10) {
                        valueSplit[pairCard - 1][upCard - 1] = shoe.
                                getProbability(1);
                    } else {
                        valueSplit[pairCard - 1][upCard - 1] = 0;
                    }
                    valueSplit[pairCard - 1][upCard - 1] *=
                            pSplit[2][upCard - 1] + pSplit[3][upCard - 1]*2
                                                  + pSplit[4][upCard - 1]*3;
                    shoe.undeal(upCard);
                }
            }

            // For each possible number of split hands, re-compute expected
            // values with the appropriate number of pair cards removed.
            for (int splitHands = 2; splitHands <= maxSplitHands;
                    splitHands++) {
                linkHandCounts(true, pairCard, splitHands);
                computeStand(true, pairCard, splitHands);
                if (pairCard != 1) {
                    computeDoubleDown(true, pairCard, splitHands);
                    computeHit(rules, strategy, true, pairCard, splitHands);
                }
                currentHand.reset();
                currentHand.deal(pairCard);

                // Remove split pair cards for weighting expected values of
                // possible hands.
                int i = 0, j;
                shoe.reset();
                for (int split = 0; split < splitHands; split++) {
                    shoe.deal(pairCard);
                    i = playerHands[i].hitHand[pairCard - 1];
                }
                for (int upCard = 1; upCard <= 10; upCard++) {
                    if (shoe.cards[upCard - 1]) {
                        shoe.deal(upCard);
                        double valueUpCard = 0,
                            pNoPair = (double)1 - shoe.
                                    getProbability(pairCard);

                        // Evaluate each possible two-card split hand.
                        for (int card = 1; card <= 10; card++) {
                            if (shoe.cards[card - 1]) {
                                currentHand.deal(card);
                                PlayerHand & hand = playerHands[
                                        playerHands[i].hitHand[card - 1]];
                                double value, testValue;
                                if (pairCard == 1) {
                                    value = hand.valueStand[true][upCard - 1];
                                } else {
                                    bool doubleDown = rules.
                                            getDoubleAfterSplit(currentHand);
                                    switch (strategy.getOption(currentHand,
                                            upCard, doubleDown, false, false)){

                                    // Again, use the playing option that
                                    // maximizes expected value for the non-
                                    // split hand.
                                    case BJ_MAX_VALUE :
                                        j = findHand(currentHand);
                                        testValue = playerHands[j].
                                                valueStand[false][upCard - 1];
                                        value = hand.
                                                valueStand[true][upCard - 1];
                                        if (testValue < playerHands[j].
                                                valueHit[false][upCard - 1]) {
                                            testValue = playerHands[j].
                                                   valueHit[false][upCard - 1];
                                            value = hand.
                                                    valueHit[true][upCard - 1];
                                        }
                                        if (doubleDown) {
                                            if (testValue < playerHands[j].
                                                valueDoubleDown[false]
                                                        [upCard - 1]) {
                                                value = hand.
                                                    valueDoubleDown[true]
                                                        [upCard - 1];
                                            }
                                        }
                                        break;
                                    case BJ_STAND :
                                        value = hand.
                                                valueStand[true][upCard - 1];
                                        break;
                                    case BJ_HIT :
                                        value = hand.
                                                valueHit[true][upCard - 1];
                                        break;
                                    case BJ_DOUBLE_DOWN :
                                        value = hand.
                                                valueDoubleDown[true]
                                                    [upCard - 1];
                                        break;
                                    default :
                                        value = 0;
                                    }
                                }

                                // If further resplitting is allowed, condition
                                // on NOT drawing an additional pair.
                                double p = shoe.getProbability(card);
                                if (splitHands < maxSplitHands) {
                                    p /= pNoPair;
                                }
                                if (card != pairCard
                                        || splitHands == maxSplitHands) {
                                    valueUpCard += value*p;
                                }
                                currentHand.undeal(card);
                            }
                        }
                        valueSplit[pairCard - 1][upCard - 1] += valueUpCard
                                *pSplit[splitHands][upCard - 1]*splitHands;
                        shoe.undeal(upCard);
                    }
                }
            }
        }
    }
}

void BJPlayer::correctStandBlackjack() {
    if (shoe.totalCards[0] && shoe.totalCards[9]) {
        currentHand.reset();
        currentHand.deal(1); currentHand.deal(10);
        PlayerHand & hand = playerHands[findHand(currentHand)];
        shoe.reset(currentHand);
        if (shoe.cards[0]) {
            shoe.deal(1);
            hand.valueStand[false][0] = (double)3/2
                    *((double)1 - shoe.getProbability(10));
            shoe.undeal(1);
        }
        for (int upCard = 2; upCard < 10; upCard++) {
            if (shoe.cards[upCard - 1]) {
                hand.valueStand[false][upCard - 1] = (double)3/2;
            }
        }
        if (shoe.cards[9]) {
            shoe.deal(10);
            hand.valueStand[false][9] = (double)3/2
                    *((double)1 - shoe.getProbability(1));
            shoe.undeal(10);
        }
    }
}

void BJPlayer::computeOverall(BJRules & rules, BJStrategy & strategy) {
    overallValue = 0;
    bool surrender = rules.getLateSurrender();
    shoe.reset();
    for (int upCard = 1; upCard <= 10; upCard++) {
        overallValues[upCard - 1] = 0;
        if (shoe.cards[upCard - 1]) {
            shoe.deal(upCard);
            for (int card1 = 1; card1 <= 10; card1++) {
                for (int card2 = 1; card2 <= 10; card2++) {
                    if (shoe.cards[card1 - 1] && shoe.cards[card2 - 1] &&
                            (card1 != card2 || shoe.cards[card1 - 1] >= 2)) {
                        currentHand.reset();
                        double p = shoe.getProbability(card1);
                        shoe.deal(card1); currentHand.deal(card1);
                        p *= shoe.getProbability(card2);
                        shoe.deal(card2); currentHand.deal(card2);
                        PlayerHand & hand = playerHands[findHand(currentHand)];
                        double value;
                        BJHand testHand(hand.cards);
                        bool doubleDown = rules.getDoubleDown(testHand),
                            split = (card1 == card2
                                    && resplit[card1 - 1] >= 2);
                        double valueSurrender;
                        switch (strategy.getOption(testHand, upCard,
                                doubleDown, split, surrender)) {
                        case BJ_MAX_VALUE :
                            value = hand.valueStand[false][upCard - 1];
                            if (value < hand.valueHit[false][upCard - 1]) {
                                value = hand.valueHit[false][upCard - 1];
                            }
                            if (doubleDown) {
                                if (value < hand.
                                        valueDoubleDown[false][upCard - 1]) {
                                    value = hand.
                                        valueDoubleDown[false][upCard - 1];
                                }
                            }
                            if (split) {
                                if (value < valueSplit[card1 - 1]
                                        [upCard - 1]) {
                                    value = valueSplit[card1 - 1][upCard - 1];
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
                            value = hand.valueStand[false][upCard - 1];
                            break;
                        case BJ_HIT :
                            value = hand.valueHit[false][upCard - 1];
                            break;
                        case BJ_DOUBLE_DOWN :
                            value = hand.valueDoubleDown[false][upCard - 1];
                            break;
                        case BJ_SPLIT :
                            value = valueSplit[card1 - 1][upCard - 1];
                            break;
                        case BJ_SURRENDER :
                            value = computeSurrender(upCard);
                            break;
                        }
                        overallValues[upCard - 1] += value*p;
                        shoe.undeal(card2);
                        shoe.undeal(card1);
                    }
                }
            }
            shoe.undeal(upCard);
            overallValue += overallValues[upCard - 1]
                    *shoe.getProbability(upCard);
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
    valueSurrender = -0.5 - valueSurrender/2;
    return valueSurrender;
}

void BJPlayer::conditionNoBlackjack() {
    for (int i = 0; i < numHands; i++) {
        PlayerHand & hand = playerHands[i];
        currentHand.reset(hand.cards);
        if (currentHand.count <= 21) {
            shoe.reset(currentHand);
            if (shoe.cards[0]) {
                shoe.deal(1);
                double p = (double)1 - shoe.getProbability(10);
                if (currentHand.numCards == 2 && currentHand.count == 21) {
                    hand.valueStand[false][0] /= p;
                } else {
                    hand.valueStand[false][0] = (hand.
                            valueStand[false][0] + 1 - p)/p;
                }
                hand.valueHit[false][0] = (hand.valueHit[false][0] + 1 - p)/p;
                hand.valueDoubleDown[false][0] = (hand.
                        valueDoubleDown[false][0] + 1 - p)/p;
                shoe.undeal(1);
            }
            if (shoe.cards[9]) {
                shoe.deal(10);
                double p = (double)1 - shoe.getProbability(1);
                if (currentHand.numCards == 2 && currentHand.count == 21) {
                    hand.valueStand[false][9] /= p;
                } else {
                    hand.valueStand[false][9] = (hand.
                            valueStand[false][9] + 1 - p)/p;
                }
                hand.valueHit[false][9] = (hand.valueHit[false][9] + 1 - p)/p;
                hand.valueDoubleDown[false][9] = (hand.
                        valueDoubleDown[false][9] + 1 - p)/p;
                shoe.undeal(10);
            }
        }
    }
    for (int pairCard = 1; pairCard <= 10; pairCard++) {
        if (resplit[pairCard - 1] >= 2 && shoe.totalCards[pairCard - 1] >= 2) {
            shoe.reset();
            shoe.deal(pairCard); shoe.deal(pairCard);
            if (shoe.cards[0]) {
                shoe.deal(1);
                double p = (double)1 - shoe.getProbability(10);
                valueSplit[pairCard - 1][0] =
                        (valueSplit[pairCard - 1][0] + 1 - p)/p;
                shoe.undeal(1);
            }
            if (shoe.cards[9]) {
                shoe.deal(10);
                double p = (double)1 - shoe.getProbability(1);
                valueSplit[pairCard - 1][9] =
                        (valueSplit[pairCard - 1][9] + 1 - p)/p;
                shoe.undeal(10);
            }
        }
    }
}
