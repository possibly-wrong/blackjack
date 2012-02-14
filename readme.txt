Blackjack version 6.5
Copyright (C) 2012 Eric Farmer (erfarmer@gmail.com)

Original card images by Oliver Xymoron
Blackjack game written using the Allegro Game Programming Library
Thanks to London Colin for many improvements and bug fixes

************************************************************************

*** License ***

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Original card images have been modified as follows:

    1. Converted to 16-color Windows bitmap format
    2. Transparent color mask changed to Allegro default

    Source code for the card images may be obtained at:
    http://www.waste.org/~oxymoron/cards/

************************************************************************

*** Documentation ***

    *** Blackjack Game (blackjack.exe) ***

Press Esc at any time to quit.

In Blackjack, you can play any of various styles of casino blackjack,
with a number of decks and rule variations of your choosing.  As you
play, exact probabilities of outcomes of the dealer's hand and exact
expected values for your hand(s) are displayed.

When you run Blackjack, you must first select the file containing the
rule variations for the casino you want.  See the casinos subfolder for
examples, with comments to explain how the rule variations are
specified.

Note that the "counting" practice modes, -1 and -2, will cause the
overall expected value to be recomputed and displayed before each hand.
Practice mode -2 computes the current expected value using "full shoe"
composition-dependent basic strategy; practice mode -1 computes the
expected value using the optimal composition-dependent strategy for the
current shoe.  (More precisely, in all cases, the strategy is CDZ-,
where the optimal strategy for non-split hands is applied to post-split
hands as well.)  In other words, mode -2 allows you to vary your wager
perfectly, and mode -1 allows you to vary both your wager and playing
strategy perfectly.

Once basic strategy for the selected casino has been computed, you are
ready to begin playing.  Press the F1 key at any time to toggle the help
display of each key's function, or press Esc at any time to quit.

To play a hand, use the up and down arrows to set your initial wager.
The table limit is $500.  Your current balance is displayed in the lower
right corner.  This is essentially the "money in your pocket;" it
decreases as you increase your wager, and vice versa.  Press Enter to
deal the hand.

The numbers to the right of the dealer's hand are the exact
probabilities of each possible outcome of the dealer's hand.  These
probabilities take into account all cards dealt from the shoe.  (Tip:
insurance has positive expected return only when the probability of
dealer blackjack is greater than 1/3.)

The numbers to the right of your hand are the exact expected values (in
percent of initial bet) of each available option.  These values take
into account all cards in your current hand and the dealer's up card.
For example, suppose the number displayed for standing is -12.3.  This
means that if you played out the current situation many, many times,
standing each time, with a $10 initial wager, then on average you would
lose $1.23.

Note that if you select an option other than the one which maximizes the
expected value of the hand, the suggested option is displayed below your
hand, and you are given another chance to select the correct option.

Once all hands have been settled, your new balance is displayed in the
lower right corner.  Press Enter to clear the table and play another
hand.  The shoe, displayed in the upper right corner, will be
automatically reshuffled when the "new shoe" card (indicated by the red
line) has been reached.

************************************************************************

    *** Basic Strategy Calculator (strategy.exe) ***

Press Ctrl-C at any time to quit.

When you run the Basic Strategy Calculator, you must first enter the
casino rule variations, similar to those specified in the data files for
the Blackjack game (see above).

In addition to the rule variations available in the game, you may
specify the playing strategy for split hands, which may be any of three
possibilities:

1. CDZ-, or composition-dependent zero-memory, where "pre-split" optimal
   strategy is also applied to all split hands (this is the default used
   in the game);
2. CDP1, where post-split strategy is optimized for the first split hand
   and then applied to all subsequent splits; or
3. CDP, where post-split strategy is allowed to vary as a function of
   the number of additional pair cards removed.

Once basic strategy for the selected rule variations has been computed,
enter the name of a file in which to save the basic strategy table.
This will be a text file, so a '.txt' extension is suggested.  (Tip: the
basic strategy table prints nicely on three pages when opened in Word,
with Courier New 10-point font and 0.75-inch left and right margins.)

In addition to the table of basic strategy for all possible two-card
player hands and all possible dealer up cards, the saved file also
contains the expected values against each dealer up card (and overall),
as well as the probabilities of each possible outcome of the dealer's
hand.

You may then enter individual hands to display the corresponding expected
values.  Enter the dealer's up card, the number of cards in your hand,
and the values of those cards.  For example, suppose you have a 10, 4,
and 2 against a dealer's 7:

Enter dealer up card and player hand (Ctrl-C to exit): 7 3 10 4 2

where:
    7 is the dealer' up card,
    3 is the number of cards in the player's hand,
    10 is the player's first card,
    4 is the player's second card,
    2 is the player's third card.

Note that all tens and face cards are to be entered as 10, and all aces
are to be entered as 1, not 11.

************************************************************************

    *** Card Counting Analyzer (count.exe) ***

Press Ctrl-C at any time to quit.

When you run the Card Counting Analyzer, you must first enter the casino
rule variations, similar to those specified in the Basic Strategy
Calculator (see above).

In addition to the rule variations available in the game, you may
specify whether to use optimal playing strategy, re-optimized for the
current depleted shoe prior to each hand; or "full shoe" composition-
dependent basic strategy.  (In either case, the strategy is CDZ-, where
the optimal strategy for non-split hands is applied to post-split hands
as well.)  In other words, optimal strategy allows you to vary both your
wager and playing strategy perfectly, while basic strategy allows you to
vary only your wager perfectly.

Finally, you may specify a shoe penetration, a random seed, and a
starting and ending index of shoes to simulate.  For each simulated
shoe, a text file with a name of the form shoe#.txt will be created,
containing a row for each round played in the corresponding shoe.  The
first 10 columns indicate the number of cards of each rank remaining in
the shoe prior to the round (ace through ten).  The 11th column
indicates the exact expected value of the round, in fraction of initial
wager.  The 12th column indicates the actual outcome of the simulated
round, in fraction of initial wager.

************************************************************************

*** List Of Files ***

    The package consists of the following files:

    blackjack.exe  Executable for Blackjack game
    strategy.exe   Executable for Basic Strategy Calculator
    count.exe      Executable for Count Analyzer
    readme.txt     This file
    gpl.txt        GNU General Public License
    casinos\       2 casino data files
    images\        55 card and table bitmaps
    src\           8 C++ source files

************************************************************************

*** Revision History ***

6.5  An additional program, count.exe, generates samples of simulated
     depleted shoes with corresponding expected values for analysis of
     card counting systems.  This program, and now also the game, use
     the Mersenne Twister random number generator for deck shuffling.

6.4  The game is now compatible with Windows Vista/7.

6.3  A bug in the recursive base case of the splitting algorithm has
     been fixed.  As a result, the strategy calculator now provides
     accurate results for "small" depleted shoes.

6.2  The game now provides the option to specify the payoff for
     blackjack.

6.1  An additional CDP1 strategy option for pair splitting optimizes for
     the first split hand, then applies that fixed strategy to all
     subsequent splits.

6.0  The algorithm for computing expected values for pair splitting has
     been completely reworked-- again.  Expected values are now exact
     for both CDZ- and CDP for splitting up to a maximum of 4 hands.

5.4  Several minor changes to the computation of dealer probabilities
     yield significant improvement in speed.

5.3  The engine interface now has options for specifying whether post-
     split strategy differs from pre-split (i.e., CDZ- vs. CDP),
     specifying the payoff for blackjack, and specifying "mixed"
     strategies that combine fixed stand/hit/etc. decisions with
     maximizing expected value.  Divide by zero bugs for small shoe
     sizes have also been fixed, as well as some code clean-up to
     improve readability.

5.2  Additional practice modes (-1, -2) play an entire shoe, but
     recompute expected value and/or optimal strategy before each hand.
     The game now runs in windowed mode rather than full screen.  The
     blackjack engine source code has been cleaned up without change to
     functionality.

5.1  The casino settings now include the option 'Practice' with default
     value 0.  Values 1, 2, and 3 indicate a practice mode where only
     hard, soft, and pair hands, respectively, will be dealt.

5.0  The algorithm for computing expected values for pair splitting has
     been completely reworked.  The specification of resplitting rules
     is more general, allowing a different maximum number of split hands
     for each pair (including no splits).  Expected values are exact
     when resplitting is not allowed.  All "private" data is now
     protected.

4.2  A bug in the algorithm for splitting expected values has been fixed.
     The hand data in BJDealer is now protected instead of private, and a
     bug in the game's help screen display has been fixed.

4.1  Expected values may now be computed using a sub-optimal playing
     strategy (e.g., "mimic the dealer").  The playing strategy, as well
     as rule variations and a progress indicator callback, are now
     specified as objects, allowing easier extensibility.  The player
     hand and expected value data in BJPlayer is now protected instead of
     private, and the class and method naming convention has been changed
     in anticipation of a possible port to Java.

4.0  The engine now accepts an arbitrary distribution of cards in the
     shoe, rather than an integral number of decks.  The engine classes
     are now const-correct, eliminating problems with compilers not
     supporting some standard extensions.  Also, the game now displays
     the help screen on startup.

3.1  Additional functions have been added to the engine for computing
     overall expected values; this additional information is included in
     the printable strategy table.  Also, the game now indicates when an
     incorrect play has been selected, giving you the opportunity to
     select the correct play.

3.0  The computational engine has been separated from the user interface.
     In addition to the text strategy calculator, a graphics blackjack
     game now allows you to play blackjack while displaying dealer
     probabilities and player expected values, using the same
     computational engine.

2.0  A completely new computational engine now computes exact
     probabilities and expected values for a given number of decks and
     rule variations.  The text interface now creates a printable table
     of basic strategy for all two-card player hands, as well as expected
     values for any individual player hand and dealer up card.

1.0  This first version uses an infinite deck approximation; i.e., the
     probability distribution of card values is not affected by removing
     cards from the shoe.  A text user interface creates a table of
     total-dependent basic strategy and dealer probabilities.
