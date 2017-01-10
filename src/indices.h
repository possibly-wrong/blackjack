///////////////////////////////////////////////////////////////////////////////
//
// indices.h
// Copyright (C) 2017 Eric Farmer (see gpl.txt for details)
//
///////////////////////////////////////////////////////////////////////////////

#ifndef INDICES_H
#define INDICES_H

#include "blackjack.h"
#include <vector>
#include <limits>

// An IndexStrategy is a total-dependent BJStrategy, initially optimized for an
// "infinite" deck, with some number of true count-dependent variations.
class IndexStrategy : public BJStrategy {
public:

    // IndexStrategy() creates a total-dependent BJStrategy optimized for an
    // "infinite" 1024-deck shoe.  Index plays may be specified via
    // setOption(), where the true count is computed using the given vector of
    // count tags (i.e., tags[card] is the tag for card = 1..10) and resolution
    // for estimating number of decks (1=perfect, 26=half-deck, etc.).  The
    // given depleted shoe will be used to compute true counts during
    // evaluation of this strategy.
    //
    // The insurance index is only used with compute_pdf(), where the default
    // +1000 (effectively +infinity) specifies no insurance for any hand.
    IndexStrategy(BJRules& rules, const std::vector<double>& tags,
        int resolution, const BJShoe& shoe, double insuranceIndex = 1000);

    // setOption() modifies this playing strategy to use the indices specified
    // for the given hand count (hard or soft) and dealer up card, and whether
    // doubling, splitting, or surrendering is allowed.  The indices and plays
    // vectors are of equal positive length; the indices must be strictly
    // increasing, with the last element +infinity, indicating the intervals of
    // true count values corresponding to the given playing strategies (see
    // BJStrategy::getOption).
    void setOption(int count, bool soft, int upCard,
        bool doubleDown, bool split, bool surrender,
        const std::vector<double>& indices, const std::vector<int>& plays);
    
    // getOption() implements the BJStrategy::getOption() interface.
    virtual int getOption(const BJHand& hand, int upCard, bool doubleDown,
        bool split, bool surrender);

    // insurance() returns true iff insurance should be taken with the given
    // player hand (according to the given insurance count tags and index);
    // this function is only used with compute_pdf().
    virtual bool insurance(const BJHand& hand);

    // trueCount() returns the true count for the current depleted shoe, and
    // the given player hand and dealer up card.
    double trueCount(const BJHand& hand, int upCard);

protected:
    std::vector<double> tags;
    const int resolution;
    struct Strategy {
        std::vector<double> indices;
        std::vector<int> plays;
    } strategies[22][2][11][2][2][2];
    const BJShoe& shoe;
    double insuranceIndex;
};

#endif // INDICES_H
