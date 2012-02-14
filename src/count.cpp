///////////////////////////////////////////////////////////////////////////////
//
// count.cpp
// Copyright (C) 2012 Eric Farmer (see gpl.txt for details)
//
///////////////////////////////////////////////////////////////////////////////

#include "blackjack.h"
#include "math_Random.h"
#include <cstdio>
#include <cstdlib>
using namespace std;

#define KEY_S   0
#define KEY_H   1
#define KEY_D   2
#define KEY_P   3
#define KEY_R   4

// A Card object includes rank and suit.

class Card {
public:
    int rank,
        suit;

    int value() {
        return(rank < 10 ? rank : 10);
    }
};

// A Shoe object includes arrangement, not just distribution, of cards in the
// shoe.

math::Random rng;

class Shoe {
public:
    Shoe(int numDecks = 1) {
        this->numDecks = numDecks;
        cards = new Card[52*numDecks];
        numCards = 0;
        for (int deck = 0; deck < numDecks; deck++)
            for (int rank = 1; rank <= 13; rank++)
                for (int suit = 1; suit <= 4; suit++) {
                    cards[numCards].rank = rank;
                    cards[numCards].suit = suit;
                    numCards++;
                }
        shuffle();
    }

    ~Shoe() {
        delete[] cards;
    }

    void shuffle() {
        for (int card = 52*numDecks; card > 1; card--)
            swap(card - 1, static_cast<int>(rng.nextInt(card)));
        numCards = 52*numDecks;
    }

    Card deal() {
        numCards--;
        return cards[numCards];
    }

    int numCards;

private:
    int numDecks;
    Card *cards;

    void swap(int card1, int card2) {
        Card temp = cards[card1];
        cards[card1] = cards[card2];
        cards[card2] = temp;
    }
};

// A Hand object represents a drawing of a blackjack hand.

class Hand : public BJHand {
public:
    Card cards[22];

    int deal(Card & card, bool faceUp = true) {
        cards[getCards()] = card;
        BJHand::deal(card.value());
        return card.value();
    }
};

// A PlayerHand object represents a particular hand dealt on a single initial
// wager.

class PlayerHand : public Hand {
public:
    int wager;
    PlayerHand *nextHand;
};

// A Player object displays expected values and determines optimal strategy.

class Player : public BJPlayer {
public:
    Player(int numDecks, BJRules *rules, BJStrategy & strategy,
           BJProgress & progress) :
    BJPlayer(BJShoe(numDecks), *rules, strategy, progress) {
        this->rules = rules;
    }

    int showOptions(Hand *player, int upCard, int numHands, int options[],
                    int & numOptions) {
        int bestOption;
        double value,
            bestValue;

        numOptions = 0;

// Player can always stand.

        bestValue = value = getValueStand(*player, upCard);
        bestOption = options[numOptions++] = KEY_S;

// Player can't hit split aces.

        if (numHands == 1 || player->cards[0].value() != 1) {
            value = getValueHit(*player, upCard);
            options[numOptions++] = KEY_H;
            if (value > bestValue) {
                bestValue = value;
                bestOption = KEY_H;
            }

// Check if player can double down.

            if ((numHands == 1 && rules->getDoubleDown(*player)) ||
                    (numHands > 1 && rules->getDoubleAfterSplit(*player))) {
                value = getValueDoubleDown(*player, upCard);
                options[numOptions++] = KEY_D;
                if (value > bestValue) {
                    bestValue = value;
                    bestOption = KEY_D;
                }
            }
        }

// Check if player can split a pair.

        int card = player->cards[0].value();
        if (player->getCards() == 2 && card == player->cards[1].value()
                && numHands < rules->getResplit(card)) {
            value = getValueSplit(card, upCard);
            options[numOptions++] = KEY_P;
            if (value > bestValue) {
                bestValue = value;
                bestOption = KEY_P;
            }
        }

// Check if player can surrender.

        if (rules->getLateSurrender() && player->getCards() == 2
                && numHands == 1) {
            value = -0.5;
            options[numOptions++] = KEY_R;
            if (value > bestValue) {
                bestValue = value;
                bestOption = KEY_R;
            }
        }

        return bestOption;
    }

private:
    BJRules *rules;
};

// A Probabilities object displays probabilities of outcomes of the dealer's
// hand.

class Probabilities : public BJDealer {
public:
    Probabilities(bool hitSoft17) : BJDealer(hitSoft17) {}
};

// Release all objects and exit.

BJRules *rules;
Player *strategy;
BJPlayer *basic;
Hand *dealer;
Probabilities *dealerProbabilities;
Shoe *shoe;
BJShoe *distribution;

void beforeExit() {
    delete rules;
    delete strategy;
    delete basic;
    delete dealer;
    delete dealerProbabilities;
    delete shoe;
    delete distribution;
}

///////////////////////////////////////////////////////////////////////////////
//
// Main program
//

int main() {

// Display title and license notice.

    printf("Blackjack Card Counting Analyzer version 6.5\n");
    printf("Copyright (C) 2012 Eric Farmer\n");
    printf("\nThanks to London Colin for many improvements and bug fixes.\n");
    printf("\nThis program comes with ABSOLUTELY NO WARRANTY.\n");
    printf("This is free software, and you are welcome to\n");
    printf("redistribute it under certain conditions; see\n");
    printf("gpl.txt for details.\n");

// Get casino rule variations.

    int numDecks;
    bool hitSoft17,
        doubleAnyTotal,
        double9,
        doubleSoft,
        doubleAfterHit,
        doubleAfterSplit,
        resplit,
        resplitAces,
        lateSurrender;
    double bjPayoff;

    printf("\nEnter number of decks: ");
    scanf("%d", &numDecks);

    char input[80];
    printf("Enter 'Y' or 'y' if dealer hits soft 17: ");
    scanf("%1s", input);
    hitSoft17 = (*input == 'Y' || *input == 'y');

    printf("Enter 'Y' or 'y' if doubling down is allowed on any total: ");
    scanf("%1s", input);
    doubleAnyTotal = (*input == 'Y' || *input == 'y');
    double9 = true;
    if (!doubleAnyTotal) {
        printf("Enter 'Y' or 'y' if doubling down is allowed on 9: ");
        scanf("%1s", input);
        double9 = (*input == 'Y' || *input == 'y');
    }

    printf("Enter 'Y' or 'y' if doubling down is allowed on soft hands: ");
    scanf("%1s", input);
    doubleSoft = (*input == 'Y' || *input == 'y');

    printf("Enter 'Y' or 'y' if doubling down is allowed on any number of ");
    printf("cards: ");
    scanf("%1s", input);
    doubleAfterHit = (*input == 'Y' || *input == 'y');

    printf("Enter 'Y' or 'y' if doubling down is allowed after splitting ");
    printf("pairs: ");
    scanf("%1s", input);
    doubleAfterSplit = (*input == 'Y' || *input == 'y');

    printf("Enter 'Y' or 'y' if resplitting pairs is allowed: ");
    scanf("%1s", input);
    resplit = (*input == 'Y' || *input == 'y');
    resplitAces = false;
    if (resplit) {
        printf("Enter 'Y' or 'y' if resplitting aces is allowed: ");
        scanf("%1s", input);
        resplitAces = (*input == 'Y' || *input == 'y');
    }

    printf("Enter 'Y' or 'y' if late surrender is allowed: ");
    scanf("%1s", input);
    lateSurrender = (*input == 'Y' || *input == 'y');

    printf("Enter blackjack payoff (normally 1.5): ");
    scanf("%lf", &bjPayoff);

// Get playing strategy (basic or optimal) and penetration.

    bool optimalPlay;
    printf("\nEnter 'Y' or 'y' if optimal playing strategy is used: ");
    scanf("%1s", input);
    optimalPlay = (*input == 'Y' || *input == 'y');

    int penetration;
    printf("\nEnter shoe penetration (%%): ");
    scanf("%d", &penetration);

// Get random seed and shoe indices to simulate.

    unsigned int seed;
    printf("Enter random seed (e.g. 5489): ");
    scanf("%u", &seed);
    rng.seed(seed);

    int firstShoe, lastShoe;
    printf("Enter index (from 1) of first shoe to simulate: ");
    scanf("%d", &firstShoe);
    printf("Enter index of last shoe to simulate: ");
    scanf("%d", &lastShoe);

// Compute basic strategy.

    rules = new BJRules(hitSoft17, doubleAnyTotal, double9,
            doubleSoft, doubleAfterHit, doubleAfterSplit, resplit, resplitAces,
            lateSurrender, bjPayoff);
    distribution = new BJShoe(numDecks);
    BJStrategy maxValueStrategy;
    BJProgress progress;
    strategy = new Player(numDecks, rules, maxValueStrategy, progress);
    basic = new BJPlayer(*distribution, *rules, maxValueStrategy, progress);

// Prepare to play blackjack.

    PlayerHand playerHands[4];
    dealer = new Hand;
    dealerProbabilities = new Probabilities(hitSoft17);
    shoe = new Shoe(numDecks);
    int lastWager = 10,
        balance = 0,
        numOptions,
        options[5];
    Card tempCard;
    PlayerHand *tempHand;

    int shoesShuffled = 0;
    while (++shoesShuffled < firstShoe) {
        shoe->shuffle();
    }
    sprintf(input, "shoe%d.txt", shoesShuffled);
    printf("Generating file %s...\n", input);
    FILE *file = fopen(input, "w");

///////////////////////////////////////////////////////////////////////////////
//
// Play blackjack.
//

    atexit(beforeExit);
    while (true) {

// Clear the table.

        int numHands = 1;
        PlayerHand *player = playerHands;
        player->reset();
        player->nextHand = NULL;

        dealer->reset();

// Reshuffle if necessary.

        if (shoe->numCards < (100 - penetration)*52*numDecks/100) {
            shoe->shuffle();
            distribution->reset();

            fclose(file);
            if (++shoesShuffled > lastShoe) {
                break;
            } else {
                sprintf(input, "shoe%d.txt", shoesShuffled);
                printf("Generating file %s...\n", input);
                file = fopen(input, "w");
            }
        }

        for (int card = 1; card <= 10; card++)
            fprintf(file, "%d ", distribution->getCards(card));
        if (optimalPlay) {
            strategy->reset(*distribution, *rules, maxValueStrategy, progress);
            fprintf(file, "%.15lf ", strategy->getValue());
        } else {
            basic->reset(*distribution, *rules, *strategy, progress);
            fprintf(file, "%.15lf ", basic->getValue());
        }

// Get player wager.

        player->wager = lastWager;
        double startBalance = balance;
        balance -= player->wager;

// Deal the hand.

        distribution->deal(player->deal(shoe->deal()));
        dealer->deal(shoe->deal());
        distribution->deal(player->deal(shoe->deal()));
        dealer->deal(shoe->deal(), false);
        bool allSettled = false;

// Check for dealer blackjack.

        if (dealer->getCards() == 2 && dealer->getCount() == 21) {
            allSettled = true;
            if (player->getCards() == 2 && player->getCount() == 21)
                balance += player->wager;
        }

// Dealer does not have blackjack; collect insurance and check for player
// blackjack.

        else {
            if (player->getCards() == 2 && player->getCount() == 21) {
                allSettled = true;
                balance += player->wager +
                    static_cast<int>(player->wager*bjPayoff);
            }
        }

// Finish player hand.

        if (!allSettled) {
            allSettled = true;
            bool done = false;
            while (!done) {

// Deal another card to a split hand if necessary.

                if (player->getCards() == 1) {
                    distribution->deal(player->deal(shoe->deal()));
                }
                int ch;

// Player always stands on 21.

                if (player->getCount() == 21)
                    ch = KEY_S;

// Get player option, reminding of best option if necessary.

                else {
                    int bestOption = strategy->showOptions(player,
                        dealer->cards[0].value(), numHands, options,
                        numOptions);
                    ch = bestOption;
                }

// Carry out player option.

                switch (ch) {
                case KEY_S :    allSettled = false;
                                if ((player = player->nextHand) == NULL)
                                    done = true;
                                break;
                case KEY_H :    distribution->deal(player->deal(shoe->deal()));
                                if (player->getCount() > 21)
                                    if ((player = player->nextHand) == NULL)
                                        done = true;
                                break;
                case KEY_D :    balance -= player->wager;
                                player->wager *= 2;
                                distribution->deal(player->deal(shoe->deal()));
                                if (player->getCount() <= 21)
                                    allSettled = false;
                                if ((player = player->nextHand) == NULL)
                                    done = true;
                                break;
                case KEY_P :    tempCard = player->cards[1];
                                player->undeal(tempCard.value());
                                playerHands[numHands].reset();
                                playerHands[numHands].nextHand =
                                        player->nextHand;
                                player->nextHand = &playerHands[numHands];
                                playerHands[numHands].wager = player->wager;
                                balance -= player->wager;
                                tempHand = playerHands;
                                while (tempHand != NULL) {
                                    tempHand = tempHand->nextHand;
                                }
                                playerHands[numHands].deal(tempCard);
                                numHands++;
                                break;
                case KEY_R :    balance += player->wager/2;
                                done = true;
                }
            }
        }

// Turn dealer hole card.

        distribution->deal(dealer->cards[0].value());
        distribution->deal(dealer->cards[1].value());

// Finish dealer hand.

        if (!allSettled) {
            while (dealer->getCount() < 17 ||
                (hitSoft17 && dealer->getCount() == 17 && dealer->getSoft())) {
                distribution->deal(dealer->deal(shoe->deal()));
            }

// Settle remaining wagers.

            int dealer_count;
            if ((dealer_count = dealer->getCount()) > 21)
                dealer_count = 0;
            player = playerHands;
            while (player != NULL) {
                if (player->getCount() <= 21)
                    if (player->getCount() > dealer_count)
                        balance += player->wager + player->wager;
                    else if (player->getCount() == dealer_count)
                        balance += player->wager;
                player = player->nextHand;
            }
        }
        fprintf(file, "%.1lf\n", (balance - startBalance) / lastWager);
    }
    return 0;
}
