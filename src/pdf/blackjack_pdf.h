///////////////////////////////////////////////////////////////////////////////
//
// blackjack_pdf.h
// Copyright (C) 2016 Eric Farmer (see gpl.txt for details)
//
///////////////////////////////////////////////////////////////////////////////

#ifndef BLACKJACK_PDF_H
#define BLACKJACK_PDF_H

#include "blackjack.h"
#include <map>

// Compute the PDF of outcomes of a unit wager on a single round of blackjack
// with the given shoe, rules, and playing strategy, with two caveats:
//
// 1. Re-splitting pairs is not allowed; i.e., rules.getResplit() is
//      effectively clamped at a maximum of 2.
// 2. Playing strategy must be explicit; i.e., strategy.getOption() must never
//      return BJ_MAX_VALUE.
std::map<double, double> compute_pdf(
    const BJShoe& shoe, BJRules& rules, BJStrategy& strategy);

#endif // BLACKJACK_PDF_H
