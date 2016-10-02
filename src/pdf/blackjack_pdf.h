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

std::map<double, double> compute_pdf(
    const BJShoe& shoe, BJRules& rules, BJStrategy& strategy);

#endif // BLACKJACK_PDF_H
