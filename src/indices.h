///////////////////////////////////////////////////////////////////////////////
//
// indices.h
// Copyright (C) 2013 Eric Farmer (see gpl.txt for details)
//
///////////////////////////////////////////////////////////////////////////////

#ifndef INDICES_H
#define INDICES_H

#include "blackjack.h"
#include <vector>

// An IndexStrategy is a total-dependent BJStrategy, initially optimized for an
// "infinite" deck, with some number of true count-dependent variations.
class IndexStrategy : public BJStrategy {
public:

    // IndexStrategy() creates a total-dependent BJStrategy optimized for an
    // "infinite" 1024-deck shoe.  Index plays may be specified via setIndex(),
    // where the true count is computed using the given vector of count tags
    // (i.e., tags[card] is the tag for card = 1..10).  The given depleted shoe
    // will be used to compute true counts during evaluation of this strategy.
    IndexStrategy(BJRules& rules, const double tags[], const BJShoe& shoe);

    // setIndex() modifies this playing strategy to use the index specified by
    // the given hand count (hard or soft) and dealer up card, and whether
    // doubling, splitting, or surrendering is allowed.  If the true count is
    // less than the given threshold, play1 is used, otherwise play2
    // (see BJStrategy::getOption).
    void setIndex(int count, bool soft, int upCard,
        bool doubleDown, bool split, bool surrender,
        double trueCount, int play1, int play2);

    // getOption() implements the BJStrategy::getOption() interface.
    virtual int getOption(const BJHand& hand, int upCard, bool doubleDown,
        bool split, bool surrender);

private:
    std::vector<double> tags;
    struct Index {
        double trueCount;
        int play1;
        int play2;
    } indices[22][2][11][2][2][2];
    const BJShoe& shoe;

    double trueCount(const BJHand& hand, int upCard);
};

#endif // INDICES_H
