///////////////////////////////////////////////////////////////////////////////
//
// game.cpp
// Copyright (C) 2011 Eric Farmer (see gpl.txt for details)
//
///////////////////////////////////////////////////////////////////////////////

#define ALLEGRO_STATICLINK
#include <allegro.h>

BEGIN_COLOR_DEPTH_LIST
    COLOR_DEPTH_8
END_COLOR_DEPTH_LIST

BEGIN_DIGI_DRIVER_LIST
END_DIGI_DRIVER_LIST

BEGIN_MIDI_DRIVER_LIST
END_MIDI_DRIVER_LIST

BEGIN_JOYSTICK_DRIVER_LIST
END_JOYSTICK_DRIVER_LIST

#include "colors.h"
#include "blackjack.h"
#include <cstdlib>
#include <ctime>
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
            rectfill(screen, 207, 285, 207 + 2*percentComplete, 314, BLUE);
        }
    }

private:
    int last;
};

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
        rect(screen, 549, 11, 600, 220, WHITE);
        srand(time(NULL));
        shuffle(0);
    }

    ~Shoe() {
        delete[] cards;
    }

    void shuffle(int practice) {
		int card, value1, value2, temp;
        for (card = 52*numDecks; card > 1; card--)
            swap(card - 1, rand()%card);
        numCards = 52*numDecks;

        // Practice hard hands.
		if (practice == 1) {
			value1 = rand()%9 + 2;
			value2 = rand()%8 + 2;
			if (value2 >= value1) {
				value2++;
			}

        // Practice soft hands.
		} else if (practice == 2) {
			value1 = 1;
			value2 = rand()%8 + 2;
			if ((rand()&1) != 0) {
				temp = value1;
				value1 = value2;
				value2 = temp;
			}

        // Practice pair hands.
		} else if (practice == 3) {
			value1 = rand()%10 + 1;
			value2 = value1;
		}

        // "Stack" the shoe with a practice hand.
		if (practice > 0) {
			for (card = 0; card < numCards; card++) {
				if (cards[card].value() == value1) {
					swap(card, numCards - 1);
					break;
				}
			}
			for (card = 0; card < numCards; card++) {
				if (cards[card].value() == value2) {
					swap(card, numCards - 3);
					break;
				}
			}
		}
		rectfill(screen, 550, 12, 599, 219, BLUE);
        hline(screen, 550, 219 - 204/numDecks, 599, RED);
    }

    Card deal() {
        numCards--;
        rectfill(screen, 550, 12, 599, 219 - numCards*4/numDecks, DARK_GREEN);
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

// Hand and card offsets.

class Offset {
public:
    int x, y;

    Offset() {
        x = y = 0;
    }

    Offset(int x, int y) {
        this->x = x;
        this->y = y;
    }

    friend Offset operator + (Offset & z1, Offset & z2) {
        return Offset(z1.x + z2.x, z1.y + z2.y);
    }
};

// Pictures of all cards, backs, and the table.

BITMAP *cardBitmaps[13][4],
    *cardBackBitmap,
    *tableBitmap;

// A Hand object represents a drawing of a blackjack hand.

class Hand : public BJHand {
public:
    Card cards[22];
    Offset position;

    int deal(Card & card, bool faceUp = true) {
        cards[getCards()] = card;
        draw(getCards(), faceUp);
        BJHand::deal(card.value());
        return card.value();
    }

    void draw(int card, bool faceUp = true) {
        Offset cardOffsets[22] = {
            Offset(0, 0), Offset(13, -17), Offset(26, -34), Offset(39, -51),
            Offset(52, -68), Offset(65, -85), Offset(78, -102), Offset(13, 0),
            Offset(26, -17), Offset(39, -34), Offset(52, -51), Offset(65, -68),
            Offset(78, -85), Offset(26, 0), Offset(39, -17), Offset(52, -34),
            Offset(65, -51), Offset(78, -68), Offset(39, 0), Offset(52, -17),
            Offset(65, -34), Offset(78, -51)
        },
            z = position + cardOffsets[card];

        draw_sprite(screen,
            faceUp ? cardBitmaps[cards[card].rank - 1][cards[card].suit - 1]
                    : cardBackBitmap,
            z.x, z.y);
    }

    void redraw() {
        for (int card = 0; card < getCards(); card++)
            draw(card);
    }

    void hideCount() {
        text_mode(DARK_GREEN);
        textprintf_centre(screen, font, position.x + 37, position.y + 105,
                YELLOW, "         ");
    }

    // If blackjack is false, then a split hand with a 10-ace is not blackjack,
    // but a soft 21.
    void showCount(bool blackjack = true) {
        hideCount();
        if (getCount() > 21)
            textprintf_centre(screen, font, position.x + 37, position.y + 105,
                    YELLOW, "Bust");
        else if (blackjack && getCards() == 2 && getCount() == 21)
            textprintf_centre(screen, font, position.x + 37, position.y + 105,
                    YELLOW, "Blackjack");
        else
            textprintf_centre(screen, font, position.x + 37, position.y + 105,
                    YELLOW, "%s%d", getSoft() ? "Soft " : "",getCount());
    }
};

// A PlayerHand object represents a particular hand dealt on a single initial
// wager.

class PlayerHand : public Hand {
public:
    int wager;
    PlayerHand *nextHand;

    void showWager() {
        text_mode(DARK_GREEN);
        textprintf_centre(screen, font, position.x + 37, position.y + 121,
                YELLOW, " $%d ", wager);
    }

    void showOption(int option) {
        char *string;

        hideCount();
        switch (option) {
        case KEY_S :    string = "Stand";
                        break;
        case KEY_H :    string = "Hit";
                        break;
        case KEY_D :    string = "Double";
                        break;
        case KEY_P :    string = "Split";
                        break;
        case KEY_R :    string = "Surrender";
        }
        textprintf_centre(screen, font, position.x + 37, position.y + 105, RED,
                "%s", string);
    }
};

// A Player object displays expected values and determines optimal strategy.

class Player : public BJPlayer {
public:
    Player(int numDecks, BJRules *rules, BJStrategy & strategy,
           Progress & progress) :
    BJPlayer(BJShoe(numDecks), *rules, strategy, progress) {
        this->rules = rules;
    }

    void clear() {
        rectfill(screen, 619, 429, 794, 500, BLACK);
    }

    int showOptions(Hand *player, int upCard, int numHands, int options[],
                    int & numOptions) {
        int bestOption;
        double value,
            bestValue;

        clear();
        text_mode(BLACK);
        numOptions = 0;

        // Player can always stand.
        bestValue = value = getValueStand(*player, upCard);
        textprintf(screen, font, 619, 429, WHITE, "Stand : %14.9lf",
                value*100);
        bestOption = options[numOptions++] = KEY_S;

        // Player can't hit split aces.
        if (numHands == 1 || player->cards[0].value() != 1) {
            value = getValueHit(*player, upCard);
            textprintf(screen, font, 619, 429 + 16*numOptions, WHITE,
                    " Hit  : %14.9lf", value*100);
            options[numOptions++] = KEY_H;
            if (value > bestValue) {
                bestValue = value;
                bestOption = KEY_H;
            }

            // Check if player can double down.
            if ((numHands == 1 && rules->getDoubleDown(*player)) ||
                    (numHands > 1 && rules->getDoubleAfterSplit(*player))) {
                value = getValueDoubleDown(*player, upCard);
                textprintf(screen, font, 619, 429 + 16*numOptions, WHITE,
                        "Double: %14.9lf", value*100);
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
            textprintf(screen, font, 619, 429 + 16*numOptions, WHITE,
                    "Split : %14.9lf", value*100);
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
            textprintf(screen, font, 619, 429 + 16*numOptions, WHITE,
                    " Surr.: %14.9lf", value*100);
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

    void reset() {
        rectfill(screen, 619, 88, 794, 191, BLACK);
    }

    // If condition is false, then P(blackjack) may be non-zero.
    void showProbabilities(BJShoe *distribution, int upCard,
                           bool condition = true) {
        computeProbabilities(*distribution);
        double notBlackjack;
        if (condition)
            notBlackjack = (double)1 - getProbabilityBlackjack(upCard);
        else
            notBlackjack = 1;
        reset();
        text_mode(BLACK);
        textprintf(screen, font, 619, 88, WHITE, "  Bust   : %.9lf",
            getProbabilityBust(upCard)/notBlackjack);
        for (int count = 17; count <= 21; count++)
            textprintf(screen, font, 619, 88 + 16*(count - 16), WHITE,
                    "%5d    : %.9lf", count,
                    getProbabilityCount(count, upCard)/notBlackjack);
        textprintf(screen, font, 619, 184, WHITE, "Blackjack: %.9lf",
            condition ? 0.0 : getProbabilityBlackjack(upCard));
    }
};

void showBalance(int balance) {
    text_mode(BLACK);
    textprintf_centre(screen, font, 707, 567, WHITE, "  $%d  ", balance);
}

void helpOn() {
    char *helpKeys[9] = {
        "  Up/Down     Change wager",
        "   Enter      Deal/Continue",
        "     S        Stand",
        "     H        Hit",
        "     D        Double down",
        "     P        Split",
        "     R        Surrender",
        "     F1       Toggle help",
        "    Esc       Quit"
    };

    text_mode(DARK_GREEN);
    for (int helpKey = 0; helpKey < 9; helpKey++)
        textprintf(screen, font, 25, 42 + 16*helpKey, YELLOW,
                helpKeys[helpKey]);
}

int getKey(int options[], int numOptions) {
    int ch;
    static bool help = true;
    bool valid = false;
    while (!valid) {
        ch = readkey() >> 8;
        if (ch == KEY_F1) {
            if (help)
                rectfill(screen, 25, 42, 240, 177, DARK_GREEN);
            else
                helpOn();
            help = !help;
        }
        else if (ch == KEY_ESC) {
            exit(0);
        }
        else
            for (int option = 0; option < numOptions; option++)
                if (ch == options[option])
                    valid = true;
    }
    return ch;
}

// Release all objects and exit.

BJRules *rules;
Player *strategy;
Hand *dealer;
Probabilities *dealerProbabilities;
Shoe *shoe;
BJShoe *distribution;

void beforeExit() {
    for (int rank = 1; rank <= 13; rank++)
        for (int suit = 1; suit <= 4; suit++)
            destroy_bitmap(cardBitmaps[rank - 1][suit - 1]);
    destroy_bitmap(cardBackBitmap);
    destroy_bitmap(tableBitmap);
    delete rules;
    delete strategy;
    delete dealer;
    delete dealerProbabilities;
    delete shoe;
    delete distribution;
    clear_keybuf();
}

///////////////////////////////////////////////////////////////////////////////
//
// Main program
//

int main() {
    char filename[128],
        *cardFilenames[13][4] = {
            {"ac.bmp", "ad.bmp", "ah.bmp", "as.bmp"},
            {"2c.bmp", "2d.bmp", "2h.bmp", "2s.bmp"},
            {"3c.bmp", "3d.bmp", "3h.bmp", "3s.bmp"},
            {"4c.bmp", "4d.bmp", "4h.bmp", "4s.bmp"},
            {"5c.bmp", "5d.bmp", "5h.bmp", "5s.bmp"},
            {"6c.bmp", "6d.bmp", "6h.bmp", "6s.bmp"},
            {"7c.bmp", "7d.bmp", "7h.bmp", "7s.bmp"},
            {"8c.bmp", "8d.bmp", "8h.bmp", "8s.bmp"},
            {"9c.bmp", "9d.bmp", "9h.bmp", "9s.bmp"},
            {"tc.bmp", "td.bmp", "th.bmp", "ts.bmp"},
            {"jc.bmp", "jd.bmp", "jh.bmp", "js.bmp"},
            {"qc.bmp", "qd.bmp", "qh.bmp", "qs.bmp"},
            {"kc.bmp", "kd.bmp", "kh.bmp", "ks.bmp"}
        };

    // Initialize Allegro graphics, mouse, keyboard, and timer drivers.
    allegro_init();
    if (set_gfx_mode(GFX_AUTODETECT_WINDOWED, 800, 600, 0, 0) != 0)
        set_gfx_mode(GFX_AUTODETECT, 800, 600, 0, 0);
    install_mouse();
    install_keyboard();
    install_timer();

    // Load card bitmaps.
    PALETTE palette;
    for (int rank = 1; rank <= 13; rank++)
        for (int suit = 1; suit <= 4; suit++) {
            append_filename(filename, "images/",
                    cardFilenames[rank - 1][suit - 1], 128);
            cardBitmaps[rank - 1][suit - 1] = load_bitmap(filename, palette);
        }
    cardBackBitmap = load_bitmap("images/back.bmp", palette);

    // Display title and license notice.
    clear(screen);
    textprintf_centre(screen, font, 400, 100, WHITE, "Blackjack version 5.5");
    textprintf_centre(screen, font, 400, 108, WHITE, "Copyright (C) 2011 Eric Farmer");
    textprintf_centre(screen, font, 400, 124, WHITE, "Original card images by Oliver Xymoron");
    textprintf_centre(screen, font, 400, 132, WHITE, "Written using the Allegro Game Programming Library");
    textprintf_centre(screen, font, 400, 148, WHITE, "Blackjack comes with ABSOLUTELY NO WARRANTY. This is");
    textprintf_centre(screen, font, 400, 156, WHITE, "free software, and you are welcome to redistribute it");
    textprintf_centre(screen, font, 400, 164, WHITE, "under certain conditions; see gpl.txt for details.");

    // Get casino data file.
    append_filename(filename, "casinos/", "", 128);
    if (!file_select("Select casino data file:", filename, "txt"))
        return 1;
    set_config_file(filename);

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

    numDecks = get_config_int(NULL, "Number_Of_Decks", 6);
    hitSoft17 = (get_config_int(NULL, "Dealer_Hits_Soft_17", 0) != 0);
    doubleAnyTotal = (get_config_int(NULL, "Double_Down_Any_Total", 1) != 0);
    double9 = (get_config_int(NULL, "Double_Down_9", 1) != 0);
    doubleSoft = (get_config_int(NULL, "Double_Down_Soft", 1) != 0);
    doubleAfterHit = (get_config_int(NULL, "Double_Down_After_Hit", 0) != 0);
    doubleAfterSplit =
            (get_config_int(NULL, "Double_Down_After_Split", 1) != 0);
    resplit = (get_config_int(NULL, "Resplit_Allowed", 1) != 0);
    resplitAces = (get_config_int(NULL, "Resplit_Aces_Allowed", 1) != 0);
    lateSurrender = (get_config_int(NULL, "Surrender_Allowed", 1) != 0);
	int practice = get_config_int(NULL, "Practice", 0);
    int dealerSpeed = get_config_int(NULL, "Dealer_Speed", 500);

    // Load table bitmap and color palette.
    append_filename(filename, "images/", hitSoft17 ? "h17.bmp" : "s17.bmp",
            128);
    tableBitmap = load_bitmap(filename, palette);
    set_palette(palette);

    // Clear screen and prepare to compute basic strategy.
    rect(screen, 0, 0, 799, 599, WHITE);
    vline(screen, 615, 1, 598, WHITE);
    rectfill(screen, 1, 1, 614, 598, DARK_GREEN);
    rectfill(screen, 616, 1, 798, 598, BLACK);
    text_mode(DARK_GREEN);
    textprintf_centre(screen, font, 307, 268, YELLOW,
            "Computing basic strategy...");
    rect(screen, 206, 284, 408, 315, WHITE);

    // Compute basic strategy.
    rules = new BJRules(hitSoft17, doubleAnyTotal, double9,
            doubleSoft, doubleAfterHit, doubleAfterSplit, resplit, resplitAces,
            lateSurrender);
    BJStrategy maxValueStrategy;
    Progress progress;
    strategy = new Player(numDecks, rules, maxValueStrategy, progress);

    // Draw table.
    draw_sprite(screen, tableBitmap, 31, 220);
    text_mode(BLACK);
    textprintf_centre(screen, font, 707, 551, WHITE, "Balance:");
    helpOn();

    // Prepare to play blackjack.
    PlayerHand playerHands[4];
    Offset handOffsets[4] = {
            Offset(265, 446), Offset(188, 446),
            Offset(112, 446), Offset(3, 446)};
    dealer = new Hand;
    dealer->position = Offset(265, 105);
    dealerProbabilities = new Probabilities(hitSoft17);
    shoe = new Shoe(numDecks);
    distribution = new BJShoe(numDecks);
    int lastWager = 5,
        balance = 0,
        numOptions,
        options[5];
    Card tempCard;
    PlayerHand *tempHand;
    Offset tempOffset;
    bool firstHand = true;

    ///////////////////////////////////////////////////////////////////////////
    //
    // Play blackjack.
    //

    atexit(beforeExit);
    while (true) {

        // Clear the table.
        int numHands = 1;
        PlayerHand *player = playerHands;
        player->reset();
        player->position = handOffsets[0];
        player->nextHand = NULL;
        rectfill(screen, 3, 344, 612, 574, DARK_GREEN);
        strategy->clear();

        dealer->reset();
        rectfill(screen, 265, 3, 415, 217, DARK_GREEN);
        dealerProbabilities->reset();

        // Reshuffle if necessary.
        if (shoe->numCards < 52 || practice > 0) {
            shoe->shuffle(practice);
            distribution->reset();
        }

        // Recompute expected value and/or strategy for counting modes.
        if (practice < 0) {
            double overallValue = 0;
            if (!firstHand) {
                text_mode(BLACK);
                textprintf(screen, font, 619, 583, WHITE,
                        "  Recomputing edge... ");
                BJProgress silent;
                if (practice == -1) {
                    strategy->reset(*distribution, *rules, maxValueStrategy,
                        silent);
                    overallValue = strategy->getValue()*100;
                } else {
                    BJPlayer *currentBasic = new BJPlayer(*distribution,
                        *rules, *strategy, silent);
                    overallValue = currentBasic->getValue()*100;
                    delete currentBasic;
                }
                clear_keybuf();
            } else {
                overallValue = strategy->getValue()*100;
            }
            firstHand = false;

            // Show overall expected value for counting modes.
            text_mode(BLACK);
            textprintf(screen, font, 619, 583, WHITE,
                    " Edge:  %14.9lf", overallValue);
        }

        // Get player wager.
        player->wager = lastWager;
        balance -= player->wager;
        numOptions = 0;
        options[numOptions++] = KEY_UP;
        options[numOptions++] = KEY_DOWN;
        options[numOptions++] = KEY_ENTER;
        bool done = false;
        while (!done) {
            player->showWager();
            showBalance(balance);
            switch (getKey(options, numOptions)) {
            case KEY_UP     :   if (player->wager < 500) {
                                    player->wager += 5;
                                    balance -= 5;
                                }
                                break;
            case KEY_DOWN   :   if (player->wager > 5) {
                                    player->wager -= 5;
                                    balance += 5;
                                }
                                break;
            case KEY_ENTER  :   done = true;
            }
        }
        lastWager = player->wager;

        // Deal the hand.
        rest(dealerSpeed); distribution->deal(player->deal(shoe->deal()));
        rest(dealerSpeed); dealer->deal(shoe->deal());
        rest(dealerSpeed); distribution->deal(player->deal(shoe->deal()));
        rest(dealerSpeed); dealer->deal(shoe->deal(), false);
        player->showCount();
        bool allSettled = false;

        // Ask for insurance if the up card is an ace.
        bool insurance = false;
        if (dealer->cards[0].value() == 1 || dealer->cards[0].value() == 10) {
            dealerProbabilities->showProbabilities(distribution,
                    dealer->cards[0].value(), false);
            if (dealer->cards[0].value() == 1) {
                text_mode(DARK_GREEN);
                textprintf(screen, font, 391, 477, YELLOW, "Insurance (Y/N)?");
                numOptions = 0;
                options[numOptions++] = KEY_Y;
                options[numOptions++] = KEY_N;
                if (insurance = (getKey(options, numOptions) == KEY_Y)) {
                    textprintf(screen, font, 391, 477, YELLOW,
                            "Insurance: $%d   ", player->wager/2);
                    showBalance(balance -= player->wager/2);
                }
                else
                    textprintf(screen, font, 391, 477, YELLOW, "                ");
            }
            rest(dealerSpeed);
        }

        // Check for dealer blackjack.
        if (dealer->getCards() == 2 && dealer->getCount() == 21) {
            allSettled = true;
            if (insurance)
                balance += player->wager/2 + (player->wager/2)*2;
            if (player->getCards() == 2 && player->getCount() == 21)
                balance += player->wager;
        }

        // Dealer does not have blackjack; collect insurance and check for
        // player blackjack.
        else {
            if (insurance) {
                text_mode(DARK_GREEN);
                textprintf(screen, font, 391, 477, YELLOW, "               ");
            }
            if (player->getCards() == 2 && player->getCount() == 21) {
                allSettled = true;
                balance += player->wager + player->wager*3/2;
            }
        }

        // Finish player hand.
        if (!allSettled) {
            dealerProbabilities->showProbabilities(distribution,
                    dealer->cards[0].value());
            allSettled = true;
            done = false;
            while (!done) {

                // Deal another card to a split hand if necessary.
                if (player->getCards() == 1) {
                    rest(dealerSpeed);
                    distribution->deal(player->deal(shoe->deal()));
                    dealerProbabilities->showProbabilities(distribution,
                        dealer->cards[0].value());
                }
                player->showCount(numHands == 1);
                int ch;

                // Player always stands on 21.
                if (player->getCount() == 21)
                    ch = KEY_S;

                // Get player option, reminding of best option if necessary.
                else {
                    int bestOption = strategy->showOptions(player,
                        dealer->cards[0].value(), numHands, options,
                        numOptions);
                    if ((ch = getKey(options, numOptions)) != bestOption) {
                        player->showOption(bestOption);
                        ch = getKey(options, numOptions);
                        player->showCount(numHands == 1);
                    }
                }

                // Carry out player option.
                switch (ch) {
                case KEY_S :    allSettled = false;
                                if ((player = player->nextHand) == NULL)
                                    done = true;
                                break;
                case KEY_H :    rest(dealerSpeed);
                                distribution->deal(player->deal(shoe->deal()));
                                dealerProbabilities->showProbabilities(
                                        distribution,
                                        dealer->cards[0].value());
                                player->showCount();
                                if (player->getCount() > 21)
                                    if ((player = player->nextHand) == NULL)
                                        done = true;
                                break;
                case KEY_D :    balance -= player->wager;
                                player->wager *= 2;
                                player->showWager();
                                showBalance(balance);
                                rest(dealerSpeed);
                                distribution->deal(player->deal(shoe->deal()));
                                dealerProbabilities->showProbabilities(
                                        distribution,
                                        dealer->cards[0].value());
                                player->showCount();
                                if (player->getCount() <= 21)
                                    allSettled = false;
                                if ((player = player->nextHand) == NULL)
                                    done = true;
                                break;
                case KEY_P :    rest(dealerSpeed);
                                tempCard = player->cards[1];
                                player->undeal(tempCard.value());
                                playerHands[numHands].reset();
                                playerHands[numHands].nextHand =
                                        player->nextHand;
                                player->nextHand = &playerHands[numHands];
                                playerHands[numHands].wager = player->wager;
                                showBalance(balance -= player->wager);
                                rectfill(screen, 3, 344, 612, 574, DARK_GREEN);
                                tempHand = playerHands;
                                tempOffset = handOffsets[numHands];
                                while (tempHand != NULL) {
                                    tempHand->position = tempOffset;
                                    tempHand->redraw();
                                    tempHand->showWager();
                                    if (tempHand->getCards() > 1)
                                        tempHand->showCount(false);
                                    tempOffset.x += 153;
                                    tempHand = tempHand->nextHand;
                                }
                                playerHands[numHands].deal(tempCard);
                                numHands++;
                                break;
                case KEY_R :    showBalance(balance += player->wager/2);
                                done = true;
                }
            }
        }

        // Turn dealer hole card.
        distribution->deal(dealer->cards[0].value());
        distribution->deal(dealer->cards[1].value());
        rest(dealerSpeed); dealer->draw(1);
        dealer->showCount();

        // Finish dealer hand.
        if (!allSettled) {
            while (dealer->getCount() < 17 ||
                (hitSoft17 && dealer->getCount() == 17 && dealer->getSoft())) {
                rest(dealerSpeed);
                distribution->deal(dealer->deal(shoe->deal()));
                dealer->showCount();
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
        showBalance(balance);

        // Prompt user to continue.
        numOptions = 0;
        options[numOptions++] = KEY_ENTER;
        getKey(options,numOptions);
    }
    return 0;
}

END_OF_MAIN();
