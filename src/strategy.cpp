///////////////////////////////////////////////////////////////////////////////
//
// strategy.cpp
// Copyright (C) 2017 Eric Farmer (see gpl.txt for details)
//
///////////////////////////////////////////////////////////////////////////////

#include "blackjack_pdf.h"
#include <cstdio>
#include <cmath>
using namespace std;

///////////////////////////////////////////////////////////////////////////////
//
// Implement BJProgress interface.
//

class Progress : public BJProgress {
public:
    Progress() {
        last = -1;
    }

    void indicate(int percentComplete) {
        if (percentComplete != last) {
            last = percentComplete;
            printf("%3d%% complete...\r", percentComplete);
        }
    }

private:
    int last;
};

// getStrategy(...) returns a string representing basic strategy for a given
// two-card player hand and dealer up card.

char *getStrategy(BJRules & rules, BJPlayer *player, int card1, int card2,
                  int upCard) {
    static char options[5];
    BJHand hand; hand.deal(card1); hand.deal(card2);
    double value,
        values[5];
    int numOptions = 0;

// Player can always stand.

    value = player->getValueStand(hand, upCard);
    options[numOptions] = (value > 0 ? 'S' : 's');
    values[numOptions++] = value;

// Player can always hit.

    value = player->getValueHit(hand, upCard);
    options[numOptions] = (value > 0 ? 'H' : 'h');
    values[numOptions++] = value;

// Check if player can double down.

    if (rules.getDoubleDown(hand)) {
        value = player->getValueDoubleDown(hand, upCard);
        options[numOptions] = (value > 0 ? 'D' : 'd');
        values[numOptions++] = value;
    }

// Check if player can split a pair.

    if (card1 == card2) {
        value = player->getValueSplit(card1, upCard);
        options[numOptions] = (value > 0 ? 'P' : 'p');
        values[numOptions++] = value;
    }

// Check for late surrender.

    if (rules.getLateSurrender()) {
        options[numOptions] = 'r';
        values[numOptions++] = -0.5;
    }

// Sort player options by expected value.

    for (int option1 = 0; option1 < numOptions - 1; option1++)
        for (int option2 = option1 + 1; option2 < numOptions; option2++)
            if (values[option1] < values[option2]) {
                double tempValue = values[option1];
                values[option1] = values[option2];
                values[option2] = tempValue;
                char tempOption = options[option1];
                options[option1] = options[option2];
                options[option2] = tempOption;
            }

// Pad option string with extra spaces.

    numOptions = 0;
    while (options[numOptions] != 'S' && options[numOptions] != 's' &&
        options[numOptions] != 'H' && options[numOptions] != 'h')
        numOptions++;
    while (++numOptions < 4)
        options[numOptions] = ' ';
    options[numOptions] = '\0';
    return options;
}

// CDvsTD::printVariations(file) displays a count and list of the composition-
// dependent stand/hit strategy variations.

const char *ranks[] = {"", "A", "2", "3", "4", "5", "6", "7", "8", "9", "T"};

class CDvsTD : public BJPlayer {
public:
    CDvsTD(const BJShoe & shoe, BJRules & rules, BJStrategy & strategy,
        BJProgress & progress) :
        BJPlayer(shoe, rules, strategy, progress) {
        // empty
    }

    void printVariations(FILE *file) {
        int totalChanges = 0;
        for (int soft = 0; soft <= 1; ++soft) {
            for (int count = 21; count >= 4 + 8 * soft; --count) {
                for (int upCard = 1; upCard <= 10; ++upCard) {
                    int numHands = 0;
                    int hitHands = 0;
                    for (int i = playerHandCount[count][soft]; i;
                        i = playerHands[i].nextHand) {
                        ++numHands;
                        BJHand hand(playerHands[i].cards);
                        if (getValueStand(hand, upCard) <
                            getValueHit(hand, upCard)) {
                            ++hitHands;
                        }
                    }
                    if (hitHands > 0 && hitHands < numHands) {
                        bool hit = (2 * hitHands > numHands);
                        int numChanges =
                            (hit ? numHands - hitHands : hitHands);
                        totalChanges += numChanges;
                        fprintf(file, "(%3d) %s %2d vs. %s : %s except",
                            numChanges, soft ? "Soft" : "Hard", count,
                            ranks[upCard], hit ? "hit" : "stand");
                        for (int i = playerHandCount[count][soft]; i;
                            i = playerHands[i].nextHand) {
                            BJHand hand(playerHands[i].cards);
                            bool hitHand = (getValueStand(hand, upCard) <
                                            getValueHit(hand, upCard));
                            if ((hit && !hitHand) || (!hit && hitHand)) {
                                fprintf(file, ", ");
                                for (int card = 1; card <= 10; ++card) {
                                    for (int n = 0; n < hand.getCards(card);
                                        ++n) {
                                        fprintf(file, "%s", ranks[card]);
                                    }
                                }
                            }
                        }
                        fprintf(file, "\n");
                    }
                }
            }
        }
        fprintf(file, "-----\n");
        fprintf(file, "%4d", totalChanges);
    }
};

///////////////////////////////////////////////////////////////////////////////
//
// Main program
//

int main() {
    int count,
        card1, card2,
        upCard,
        card,
        numCards,
        hitCard;

// Display title and license notice.

    printf("Blackjack Basic Strategy Calculator version 7.6\n");
    printf("Copyright (C) 2017 Eric Farmer\n");
    printf("\nThanks to London Colin for many improvements and bug fixes.\n");
    printf("\nThis program comes with ABSOLUTELY NO WARRANTY.\n");
    printf("This is free software, and you are welcome to\n");
    printf("redistribute it under certain conditions; see\n");
    printf("gpl.txt for details.\n");

// Get casino rule variations.

    int numDecks, cards[11];
    bool hitSoft17,
        doubleAnyTotal,
        double9,
        doubleSoft,
        doubleAfterHit,
        doubleAfterSplit,
        resplit,
        resplitAces,
        useCDZ,
        useCDP1,
        lateSurrender;
    double bjPayoff;

    printf("\nEnter number of decks, or 0 to enter shoe: ");
    scanf("%d", &numDecks);

    if (numDecks == 0) {
        printf("Enter number of cards of each rank (1-10): ");
        for (card = 1; card <= 10; card++) {
            scanf("%d", &cards[card]);
        }
    }

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

    printf("Enter 'Y' or 'y' if CDZ- (vs. CDP) post-split strategy is used: ");
    scanf("%1s", input);
    useCDZ = (*input == 'Y' || *input == 'y');
    useCDP1 = true;
    if (!useCDZ) {
        printf("Enter 'Y' or 'y' if CDP1 (vs. CDP) strategy is used: ");
        scanf("%1s", input);
        useCDP1 = (*input == 'Y' || *input == 'y');
    }

    printf("Enter 'Y' or 'y' if late surrender is allowed: ");
    scanf("%1s", input);
    lateSurrender = (*input == 'Y' || *input == 'y');

    printf("Enter blackjack payoff (normally 1.5): ");
    scanf("%lf", &bjPayoff);

// Compute basic strategy.

    printf("\n");
    BJShoe *shoe;
    if (numDecks > 0) {
        shoe = new BJShoe(numDecks);
    } else {
        shoe = new BJShoe(cards);
    }
    BJRules rules(hitSoft17, doubleAnyTotal, double9, doubleSoft,
        doubleAfterHit, doubleAfterSplit, resplit, resplitAces, lateSurrender,
        bjPayoff);
    BJStrategy strategy(useCDZ, useCDP1);
    Progress progress;
    CDvsTD *player = new CDvsTD(*shoe, rules, strategy, progress);
    IndexStrategy no_insurance(rules, std::vector<double>(11), 26, *shoe);
    std::map<double, double> pdf;
    if (!resplit && useCDZ)
    {
        pdf = compute_pdf(*shoe, rules, *player, no_insurance);
    }

// Get output filename and prepare to save basic strategy table.

    printf("\n\nEnter output filename: ");
    scanf("%s", input);
    FILE *file = fopen(input, "w");

///////////////////////////////////////////////////////////////////////////////
//
// Display rule variations.
//

    if (numDecks > 0) {
        fprintf(file, "%d deck%s", numDecks, numDecks > 1 ? "s" : "");
    } else {
        fprintf(file, "Shoe(%d", cards[1]);
        for (card = 2; card <= 10; card++) {
            fprintf(file, ", %d", cards[card]);
        }
        fprintf(file, ")");
    }
    fprintf(file, ", %s, %s",
        hitSoft17 ? "H17" : "S17",
        doubleAnyTotal ? "DOA" : (double9 ? "D9" : "D10"));
    if (!doubleSoft)
        fprintf(file, ", no double on soft");
    if (doubleAfterHit)
        fprintf(file, ", DAN");
    fprintf(file, ", %s", doubleAfterSplit ? "DAS" : "NDAS");
    fprintf(file, ", %s", resplit ? "SPL3" : "SPL1");
    if (resplit)
        fprintf(file, resplitAces ? ", RSA" : ", NRSA");
    if (lateSurrender)
        fprintf(file, ", surrender");
    if (bjPayoff != 1.5)
        fprintf(file, ", BJ pays %lf", bjPayoff);
    fprintf(file, ", %s", useCDZ ? "CDZ-" : (useCDP1 ? "CDP1" : "CDP"));
    fprintf(file, "\n\n");

// Display basic strategy for hard hands, organized by count.

    fprintf(file, "  Hard |                Dealer's up card\n");
    fprintf(file, "  hand | 2    3    4    5    6    7    8    9    10   A\n");
    fprintf(file, "-----------------------------------------------------------\n");
    for (count = 19; count >= 5; count--) {
        card1 = (count > 12 ? 10 : count - 2);
        card2 = count - card1;
        do {
            fprintf(file, "%3d-%2d |", card1, card2);
            for (upCard = 2; upCard <= 10; upCard++)
                fprintf(file, " %s", getStrategy(rules, player, card1, card2,
                        upCard));
            fprintf(file, " %s\n", getStrategy(rules, player, card1, card2,
                    1));
        } while (--card1 > ++card2);
        fprintf(file, "       |\n");
    }

// Display basic strategy for soft hands.

    fprintf(file, "\n  Soft |                Dealer's up card\n");
    fprintf(file, "  hand | 2    3    4    5    6    7    8    9    10   A\n");
    fprintf(file, "-----------------------------------------------------------\n");
    for (card = 9; card >= 2; card--) {
        fprintf(file, "  A-%2d |", card);
        for (upCard = 2; upCard <= 10; upCard++)
            fprintf(file, " %s", getStrategy(rules, player, 1, card, upCard));
        fprintf(file, " %s\n", getStrategy(rules, player, 1, card, 1));
    }

// Display basic strategy for pair hands.

    fprintf(file, "\n  Pair |                Dealer's up card\n");
    fprintf(file, "  hand | 2    3    4    5    6    7    8    9    10   A\n");
    fprintf(file, "-----------------------------------------------------------\n");
    fprintf(file, "  A- A |");
    for (upCard = 2; upCard <= 10; upCard++)
        fprintf(file, " %s", getStrategy(rules, player, 1, 1, upCard));
    fprintf(file, " %s\n", getStrategy(rules, player, 1, 1, 1));
    for (card = 10; card >= 2; card--) {
        fprintf(file, "%3d-%2d |", card, card);
        for (upCard = 2; upCard <= 10; upCard++)
            fprintf(file, " %s", getStrategy(rules, player, card, card,
                    upCard));
        fprintf(file, " %s\n", getStrategy(rules, player, card, card, 1));
    }
    fprintf(file, "-----------------------------------------------------------\n");

// Display strategy codes.

    fprintf(file, "\nS = Stand\n");
    fprintf(file, "H = Hit\n");
    fprintf(file, "D = Double down\n");
    fprintf(file, "P = Split\n");
    if (lateSurrender)
        fprintf(file, "R = Surrender\n");
    fprintf(file, "\nUppercase indicates action is favorable for the player");
    fprintf(file, "\nLowercase indicates action is favorable for the house\n");
    fprintf(file, "\nWhen more than one option is listed, options are ");
    fprintf(file, "listed from left to right in order of preference.\n");

// Display overall expected values.

    fprintf(file, "\n    Up  |\n   card | Overall expected value (%%)\n");
    fprintf(file, "---------------------------------\n");
    for (upCard = 2; upCard <= 10; upCard++)
        fprintf(file, "%6d  | %14.9lf\n", upCard,
                player->getValue(upCard)*100);
    fprintf(file, "     A  | %14.9lf\n", player->getValue(1)*100);
    fprintf(file, "---------------------------------\n");
    double ev = player->getValue();
    fprintf(file, "  Total | %14.9lf", ev*100);
    if (!resplit && useCDZ)
    {
        double variance = 0;
        for (auto&& q : pdf)
        {
            variance += (q.second * pow(q.first - ev, 2));
        }
        fprintf(file, " +/- %.9lf", sqrt(variance)*100);
    }
    fprintf(file, "\n");

// Display probabilities of outcomes of the dealer's hand.

    BJDealer dealer(hitSoft17);
    dealer.computeProbabilities(*shoe);
    fprintf(file, "\n\n\n\n    Up  |              Probability of outcome of dealer's hand\n");
    fprintf(file, "   card |  Bust   |   17    |   18    |   19    |   20    |   21    |");
        fprintf(file, "Blackjack\n");
    fprintf(file, "---------------------------------------------------------------------");
        fprintf(file, "----------\n");
    for (upCard = 2; upCard <= 10; upCard++) {
        fprintf(file, "%6d  | %.5lf", upCard,
                dealer.getProbabilityBust(upCard));
        for (count = 17; count <= 21; count++)
            fprintf(file, " | %.5lf",
                    dealer.getProbabilityCount(count, upCard));
        fprintf(file, " | %.5lf\n", dealer.getProbabilityBlackjack(upCard));
    }
    fprintf(file, "     A  | %.5lf", dealer.getProbabilityBust(1));
    for (count = 17; count <= 21; count++)
        fprintf(file, " | %.5lf", dealer.getProbabilityCount(count,1));
    fprintf(file, " | %.5lf\n", dealer.getProbabilityBlackjack(1));
    fprintf(file, "---------------------------------------------------------");
    fprintf(file, "----------------------\n");
    fprintf(file, "  Total | %.5lf", dealer.getProbabilityBust());
    for (count = 17; count <= 21; count++)
        fprintf(file, " | %.5lf", dealer.getProbabilityCount(count));
    fprintf(file, " | %.5lf\n", dealer.getProbabilityBlackjack());

    if (!resplit && useCDZ)
    {
        fprintf(file, "\n\nOutcome | Probability\n");
        fprintf(file, "---------------------\n");
        for (auto&& q : pdf)
        {
            fprintf(file, "%5.1lf   | %.9lf\n", q.first, q.second);
        }
    }

// Display composition-dependent strategy variations.

    fprintf(file, "\n\nComposition-dependent stand/hit strategy variations:\n");
    fprintf(file, "----------------------------------------------------\n");
    player->printVariations(file);
    fclose(file);

///////////////////////////////////////////////////////////////////////////////
//
// Display statistics for individual player hands.
//

    while (true) {
        printf("\nEnter dealer up card and player hand (Ctrl-C to exit): ");
        scanf("%d%d", &upCard, &numCards);
        if (upCard == 0 && numCards == 0)
            break;
        BJHand hand;
        for (card = 0; card < numCards; card++) {
            scanf("%d", &hitCard);
            hand.deal(hitCard);
        }
        printf("\nStand      E(X) = %14.9lf%%\n",
                player->getValueStand(hand, upCard)*100);
        printf("Hit        E(X) = %14.9lf%%\n",
                player->getValueHit(hand, upCard)*100);
        if (rules.getDoubleDown(hand))
            printf("Double     E(X) = %14.9lf%%\n",
                    player->getValueDoubleDown(hand, upCard)*100);
        if (hand.getCards() == 2)
            for (card = 1; card <= 10; card++)
                if (hand.getCards(card) == 2)
                    printf("Split      E(X) = %14.9lf%%\n",
                        player->getValueSplit(card, upCard)*100);
        if (lateSurrender && hand.getCards() == 2)
            printf("Surrender  E(X) = %14.9lf%%\n", -0.5*100);
    }

    delete player;
}
