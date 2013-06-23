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
    IndexStrategy(BJRules& rules, const std::vector<double>& tags,
        int resolution, const BJShoe& shoe);

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

private:
    std::vector<double> tags;
    const int resolution;
    struct Strategy {
        std::vector<double> indices;
        std::vector<int> plays;
    } strategies[22][2][11][2][2][2];
    const BJShoe& shoe;

    double trueCount(const BJHand& hand, int upCard);
};

#endif // INDICES_H
