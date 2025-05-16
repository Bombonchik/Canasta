#include "client/game_view.hpp"


GameView::GameView(): console(), screen(ScreenInteractive::TerminalOutput()) {
    console.clear();
}

Element GameView::makeBoard(const BoardState& boardState) {
    const auto myColor = Color::LightSlateBlue;
    const auto oppColor = Color::LightGreenBis; 
    auto myMeldGrid = makeMeldGrid(boardState.getMyTeamMelds(), myColor);
    auto opponentMeldGrid = makeMeldGrid(boardState.getOpponentTeamMelds(), oppColor);
    auto myHandRow = makeHandGrid(boardState.getMyHand());
    auto deckInfo = makeDeckInfo(boardState.getDeckState());
    auto scoreInfo = makeScoreInfo(boardState.getMyTeamTotalScore(),
        boardState.getOpponentTeamTotalScore(),
        boardState.getMyTeamMeldPoints(),
        boardState.getOpponentTeamMeldPoints(),
        myColor, oppColor
    );
    auto myPlayerInfo = makePlayerInfo(boardState.getMyPlayer());
    auto opponentPlayerInfo = makePlayerInfo(boardState.getOppositePlayer());
    auto leftPlayer = boardState.getLeftPlayer();
    auto leftPlayerInfo = leftPlayer.has_value() ?
        makePlayerInfo(leftPlayer.value()) : text(" ");
    auto rightPlayer = boardState.getRightPlayer();
    auto rightPlayerInfo = rightPlayer.has_value() ?
        makePlayerInfo(rightPlayer.value()) : text(" ");

    // 4) Combine in a vertical stack
    return vbox({
        hbox({
            opponentPlayerInfo,
        }) | center,
        hbox({
            hbox({ filler()|flex, myMeldGrid,       filler() |flex }) | flex,
            hbox({ filler()|flex, opponentMeldGrid, filler() |flex }) | flex
        }),
        hbox({
            leftPlayerInfo  | flex,
            scoreInfo       | flex,
            deckInfo        | flex,
            rightPlayerInfo | align_right,
        }),
        hbox({
            myHandRow,
        }) | center,
        hbox({
            myPlayerInfo,
        }) | center
    });
}

Element GameView::makeMeldGrid(const std::vector<MeldView>& melds, Color frameColor) {
    std::vector<MeldView> meldsToPrint;
    for (auto& meld : melds) {
        if (!meld.isInitialized())
            continue;
        meldsToPrint.push_back(meld);
    }
    std::vector<Element> rows;
    constexpr std::size_t maxRows = 8;

    if (meldsToPrint.empty()) {
        return text(" ") | color(frameColor);
    }

    for (std::size_t row = 0; row < maxRows; ++row) {
        std::vector<Element> cells;

        for (auto& meld : meldsToPrint) {
            if (!meld.isInitialized())
                continue;
            // not first meld
            if (meld.getRank() != meldsToPrint.front().getRank())
                cells.push_back(separator() | color(frameColor));
            
            auto& meldCards = meld.getCards();

            // 1) true canasta indicator: if they have ≥7 cards, on the 8th row show 'C'
            if (meldCards.size() >= 7 && row == 7) {
                cells.push_back(text(" C ") | color(frameColor) | flex_grow);
                continue;
            }

            // 2) otherwise if this row falls inside the number of cards
            if (row < meldCards.size()) {
                // a) small meld <7 cards ⇒ show the card at index = row
                if (meldCards.size() < 7) {
                    cells.push_back(makeCardElement(meldCards[row], true));

                } else {
                    if (row == 0) {
                        cells.push_back(makeCardElement(meldCards.front(), true));
                    } else if (row == 1 &&
                            meldCards.front().getRank() != meldCards.back().getRank()) {
                        cells.push_back(makeCardElement(meldCards.back(), true));
                    } else {
                        cells.push_back(text("   ") | flex_grow);
                    }
                }
            }
            // 3) outside the card count ⇒ blank
            else
                cells.push_back(text("   ") | flex_grow);
        }

        rows.push_back(hbox(std::move(cells)));
    }

    // wrap it in a box exactly like before
    return vbox(std::move(rows)) | border | color(frameColor);
}


Element GameView::makeHandGrid(const Hand& hand) {
    auto cards = hand.getCards();
    if (cards.empty())
        return text("Hand is empty");
    std::vector<std::vector<Card>> cardLayout;
    std::vector<Card> cardColumn;
    for (auto& card : cards) {
        if (cardColumn.empty()) {
            cardColumn.push_back(card);
        }
        else if (card.getRank() == cardColumn.back().getRank()) {
            cardColumn.push_back(card);
        } else {
            cardLayout.push_back(cardColumn);
            cardColumn.clear();
            cardColumn.push_back(card);
        }
    }
    cardLayout.push_back(cardColumn);

    std::size_t maxSize = 0;
    for (auto& column : cardLayout) {
        if (column.size() > maxSize) {
            maxSize = column.size();
        }
    }

    std::vector<Element> columns;
    std::vector<Element> cells;
    for (auto& column : cardLayout) {
        for (std::size_t i = 0; i < maxSize; ++i) {
            if (i < column.size()) {
                cells.push_back(makeCardElement(column[i]));
            } else {
                cells.push_back(text("   ") | flex_grow);
            }
        }
        columns.push_back(vbox(std::move(cells)));
        std::vector<Element> delimeterColumn;
        for (std::size_t i = 0; i < maxSize; ++i)
            delimeterColumn.push_back(text("|"));
        //columns.push_back(vbox(std::move(delimeterColumn)));
        columns.push_back(separator());
    }
    columns.pop_back(); // remove the last delimeter
    auto cardsElement = hbox(std::move(columns)) | border;
    
    return cardsElement;
}


// Demo
Element GameView::makeDeckInfo(const ClientDeck& deck) {
    bool hasTop = deck.getTopDiscardCard().has_value();
    auto topCard = hasTop ? deck.getTopDiscardCard().value() : Card();
    return vbox({
        hbox({
            text("Main Deck Size:  ") | bold,
            text(std::to_string(deck.getMainDeckSize())) | bold,
        }),
        hbox({
            text("Top Discard Card:") | bold,
            hasTop ? makeCardElement(topCard) : text(" ") | bold,
        }),
    });
}

CardView GameView::getCardView(const Card& card) {
    auto cardViewColor = card.getColor() == CardColor::RED ? 
    Color::RedLight : Color::White;
    std::string cardToPrint = "";
    auto rank = card.getRank();
    if (rank == Rank::Joker) {
        cardToPrint = "@";
    } else if (rank >= Rank::Two && rank <= Rank::Nine) {
        cardToPrint = std::to_string(static_cast<int>(rank));
    } else if (rank == Rank::Ten) {
        cardToPrint = "X";
    } else if (rank == Rank::Jack) {
        cardToPrint = "J";
    } else if (rank == Rank::Queen) {
        cardToPrint = "Q";
    } else if (rank == Rank::King) {
        cardToPrint = "K";
    } else if (rank == Rank::Ace) {
        cardToPrint = "A";
    }
    return CardView(cardToPrint, cardViewColor);
}

Element GameView::makeCardElement(const Card& card, bool padded) {
    // 1) pull CardView
    CardView cv = getCardView(card);

    // 2) pick whether to pad or not
    std::string label = padded
    ? " " + cv.getLabel() + " "
    : cv.getLabel();

    // 3) build & return the styled element
    return text(label)
        | color(cv.getColor())
        | flex_grow;
}

std::string GameView::promptString(const std::string& question, std::string& placeholder) {
    console.clear();
    // 1) local buffer
    std::string buffer;

    // 2) Configure InputOption to *reference* that buffer
    InputOption option = InputOption::Default();
    option.content     = &buffer;             // <<–– bind to std::string
    option.placeholder = StringRef(placeholder);        // <<–– bind to the caller’s placeholder
    option.on_enter    = [&] { screen.ExitLoopClosure()(); };
    option.multiline   = false;

    // 3) Build & render
    auto input    = Input(option);
    auto renderer = Renderer(input, [&] {
        return vbox({
            text(question),
            input->Render() | frame,
            text("(press Enter)") | dim
        }) | border | center;
    });

    screen.Loop(renderer);
    disableInput();
    // 4) buffer now holds the final text
    return buffer.substr(0, std::min<int>(buffer.size(), 10));
}

void GameView::disableInput() {
    if (!inputGuard)
      inputGuard.emplace();  // now console is in raw/no-echo mode
}

void GameView::restoreInput() {
    inputGuard.reset();     // destructor runs, restoring original mode
}


void GameView::showStaticBoardWithMessages(
    const std::vector<std::string>& messages, const BoardState& boardState) {
    console.clear();
    disableInput();

    auto boardElem = makeBoard(boardState) | flex_grow;

    // 3) Build the message box
    std::vector<Element> lines;
    for (auto& m : messages)
        lines.push_back(text(m));
    auto messagePane =
    vbox(std::move(lines))
    | size(HEIGHT, EQUAL, (int)messages.size() + 2);

    // 4) Compose full layout
    auto document = vbox({
        boardElem,
        separator(),
        messagePane
    });

    // 5) One-shot render into a virtual Screen
    auto screenBuff = Screen::Create(
        Dimension::Full(),        // full terminal width
        Dimension::Fit(document)  // height = content height
    );
    Render(screenBuff, document);

    // 6) Print it
    std::cout
    << screenBuff.ResetPosition()
    << screenBuff.ToString()
    << std::flush;
}

Element GameView::makeScoreInfo(int myTeamTotalScore, int opponentTeamTotalScore,
    int myTeamMeldPoints, int opponentTeamMeldPoints, Color textColor1, Color textColor2) {
    return vbox({
        hbox({
            text("Total Score: ") | bold,
            text(std::to_string(myTeamTotalScore)) | bold | color(textColor1),
            text(" vs ") | bold,
            text(std::to_string(opponentTeamTotalScore)) | bold | color(textColor2),
        }),
        hbox({
            text("Meld Points: ") | bold,
            text(std::to_string(myTeamMeldPoints)) | bold | color(textColor1),
            text(" vs ") | bold,
            text(std::to_string(opponentTeamMeldPoints)) | bold | color(textColor2),
        }),
    });
}

Element GameView::makePlayerInfo(const PlayerPublicInfo& player) {
    return hbox({
        text(player.getName()) | bold | (player.isCurrentPlayer() ? color(Color::Cyan) : color(Color::White)),
        text(", " + std::to_string(player.getHandCardCount())) | bold | (player.isCurrentPlayer() ? color(Color::Cyan) : color(Color::White)),
    });
}


void GameView::showStaticScore(const ScoreState& scoreState) {
    console.clear();
    disableInput();

    // 2) References to your two score‐breakdowns
    const auto& sb  = scoreState.getMyTeamScoreBreakdown();
    const auto& osb = scoreState.getOpponentTeamScoreBreakdown();
    // 3) Build a grid of Elements: each inner vector is a row
    std::vector<Element> columnNames;
    std::vector<Element> team1Scores;
    std::vector<Element> team2Scores;

    // Helper to add a row
    auto addRow = [&](const std::string& label, int left, int right) {
        Color leftColor, rightColor;
        if (left < 0)
            leftColor = Color::Red;
        else 
            leftColor = Color::Default;
        if (right < 0)
            rightColor = Color::Red;
        else
            rightColor = Color::Default;

        // Column 1: label, left‐aligned by default
        columnNames.push_back(text(label) | bold);
        // Column 2: your team score
        team1Scores.push_back(text(std::to_string(left))
        | color(leftColor) | align_right | bold);
        // Column 3: opponent score
        team2Scores.push_back(text(std::to_string(right))
        | color(rightColor) | align_right | bold);
    };

    // 4) Populate all the rows in the exact order you want
    addRow("Natural",     sb.getNaturalCanastaBonus(),   osb.getNaturalCanastaBonus());
    addRow("Mixed",       sb.getMixedCanastaBonus(),     osb.getMixedCanastaBonus());
    addRow("Melded",      sb.getMeldedCardsPoints(),     osb.getMeldedCardsPoints());
    addRow("Red threes",  sb.getRedThreeBonusPoints(),   osb.getRedThreeBonusPoints());
    addRow("On hands",    sb.getHandPenaltyPoints(),     osb.getHandPenaltyPoints());
    addRow("Going out",   sb.getGoingOutBonus(),         osb.getGoingOutBonus());

    // Round total & Game total (bold)
    addRow("Round Total", sb.calculateTotal(), osb.calculateTotal());

    addRow("Game Total",
        scoreState.getMyTeamTotalScore(),
        scoreState.getOpponentTeamTotalScore()
    );

    std::string outcome;
    // Optional outcome line
    auto maybeGameOutcome = scoreState.getGameOutcome();
    if (scoreState.getIsGameOver() && maybeGameOutcome.has_value()) {
        if (scoreState.getPlayersCount() == 2) {
        if (*maybeGameOutcome == ClientGameOutcome::Win)
            outcome = "You win!";
        else if (*maybeGameOutcome == ClientGameOutcome::Lose)
            outcome = "You lose";
        else
            outcome = "Draw";
        } else {
        if (*maybeGameOutcome == ClientGameOutcome::Win)
            outcome = "Your team wins!";
        else if (*maybeGameOutcome == ClientGameOutcome::Lose)
            outcome = "Your team loses";
        else
            outcome = "Draw";
        }
    } else {
        outcome = "Waiting for next round...";
    }
    
    auto table = hbox({
        vbox(std::move(columnNames)),
        filler(),
        vbox(std::move(team1Scores)) | center,
        filler(),
        vbox(std::move(team2Scores)) | align_right,
    })| size(WIDTH, GREATER_THAN, 25);

    auto outcomeLine = text(outcome) | bold | center;
    Element document = vbox({
            table,
            separator(),
            outcomeLine,
    }) | center | border 
    | size(HEIGHT, GREATER_THAN, (int)columnNames.size() + 2)
    | center;

    // 5) One-shot render into a virtual Screen
    auto screenBuff = Screen::Create(
        Dimension::Full(),
        Dimension::Fit(document)
    );
    Render(screenBuff, document);
    std::cout << screenBuff.ResetPosition()
                << screenBuff.ToString()
                << std::flush;
    restoreInput(); // Restore terminal state
}

int GameView::promptChoiceWithBoard(
    const std::string& question,
    const std::vector<std::string>& options,
    const BoardState& boardState,
    std::optional<const std::string> message) {

    console.clear();
    auto localScreen = ScreenInteractive::TerminalOutput();
    localScreen.TrackMouse(false);
    bool shouldUpdate = true;

    // Top: your board, grows to fill
    auto board = makeBoard(boardState) | flex_grow;

    // Selection state
    int selected = 0;
    int scroll   = 0;
    const int paneH = std::min<int>((int)options.size() + 2, 8);
    // show up to 6 options + question/message

    Component component = CatchEvent(
        Renderer([&] {
        // Build prompt lines
        std::vector<Element> lines;

        if (message)
            lines.push_back(text(message.value()));
        lines.push_back(text(question));

        // Visible window
        int vis = paneH - (message ? 2 : 1);
        scroll = std::min<int>(scroll, (int)options.size() - vis);
        scroll = std::max<int>(scroll, 0);

        // Draw options with arrow
        for (int i = 0; i < vis; ++i) {
            int idx = i + scroll;
            if (idx >= (int)options.size()) break;
            std::string prefix = (idx == selected ? "→ " : "  ");
            lines.push_back(text(prefix + options[idx]));
        }

        auto promptBox = vbox(std::move(lines));
        return vbox({board, separator(), promptBox});
        }),
        [&](Event e) {
        int n = options.size();
        int vis = paneH - (message ? 2 : 1);

        if (e == Event::ArrowDown) {
            if (selected + 1 < n) {
            selected++;
            if (selected >= scroll + vis)
                scroll = selected - vis + 1;
            }
            return true;
        }
        if (e == Event::ArrowUp) {
            if (selected > 0) {
            selected--;
            if (selected < scroll)
                scroll = selected;
            }
            return true;
        }
        if (e == Event::Return) {
            // Trigger exit; Loop.RunOnce will see this and quit
            localScreen.ExitLoopClosure()();
            shouldUpdate = false;
            return false;
        }
        return false;
        }
    );

    // Manual event loop: run until Quit()
    auto loop = Loop(&localScreen, component);
    while (!loop.HasQuitted() && shouldUpdate) {
        loop.RunOnce();
    }

    return selected;
}

std::vector<MeldRequest> GameView::runMeldWizard(const BoardState& boardState) {
    console.clear();
    auto localScreen = ScreenInteractive::TerminalOutput();
    localScreen.TrackMouse(false);
    bool shouldUpdate = true;
    // 1) Mutable hand + map for meld‐requests
    Hand working = boardState.getMyHand();
    std::map<Rank, MeldRequest> requestMap;

    // 2) Wizard state
    enum class Mode { PICK_RANK, PICK_CARDS };
    Mode mode = Mode::PICK_RANK;
    const std::vector<Rank> ALL_RANKS = {
        Rank::Three, Rank::Four, Rank::Five, Rank::Six,
        Rank::Seven, Rank::Eight, Rank::Nine, Rank::Ten,
        Rank::Jack,  Rank::Queen, Rank::King, Rank::Ace
    };
    std::size_t rankIdx = 0, rankScroll = 0;
    std::size_t cardIdx = 0, cardScroll = 0;
    std::optional<Rank> currentRank;
    std::vector<bool> cardSelected;
    const int paneH = 6;

    // Helpers
    auto label_for_rank = [&](Rank r) {
    int cnt = 0;
    for (auto& c : working.getCards())
        if (c.getRank() == r) ++cnt;
    Card dummy{r, CardColor::BLACK};
        return getCardView(dummy).getLabel() + " (" + std::to_string(cnt) + ")";
    };
    auto bucket_for = [&](Rank r) {
        std::vector<Card> bucket;
        for (auto& c : working.getCards()) {
            if (c.getRank()==r ||
            c.getRank()==Rank::Joker || c.getRank()==Rank::Two)
            bucket.push_back(c);
        }
        return bucket;
    };

    // 3) Build the UI
    Component component = CatchEvent(
    Renderer([&]{    
        // a) board at top
        auto boardView = makeBoard(boardState) | flex_grow;

        // b) wizard pane
        std::vector<Element> lines;
        if (mode == Mode::PICK_RANK) {
            std::size_t n   = ALL_RANKS.size();
            std::size_t vis = paneH - 2;
            rankIdx    = std::min<std::size_t>(rankIdx, n? n-1:0);
            rankScroll = std::min<std::size_t>(rankScroll, n>vis? n-vis:0);

            lines.push_back(text("Select rank:"));
            for (std::size_t i = 0; i < vis; ++i) {
                std::size_t idx = i + rankScroll;
                if (idx >= n) break;
                    std::string pre = (idx==rankIdx?"→ ":"  ");
                lines.push_back(text(pre + label_for_rank(ALL_RANKS[idx])));
            }
            lines.push_back(text("Enter=Pick  Esc=Finish")|dim);
        } else {
        // PICK_CARDS
        Rank r        = *currentRank;
        auto bucket   = bucket_for(r);
        std::size_t n      = bucket.size();
        std::size_t vis    = paneH - 2;
        if (cardSelected.size()!=n)
            cardSelected.assign(n,false);
        cardIdx    = std::min<std::size_t>(cardIdx, n?n-1:0);
        cardScroll = std::min<std::size_t>(cardScroll, n>vis? n-vis:0);

        Card dummy{r, CardColor::BLACK};
        lines.push_back(text("Pick cards for `"
                        + getCardView(dummy).getLabel() +"`:"));
        for (std::size_t i = 0; i < vis; ++i) {
            std::size_t idx = i + cardScroll;
            if (idx >= n) break;
            std::string prefix = (idx==cardIdx ? "→ " : "  ");
            std::string mark   = cardSelected[idx] ? "[x] " : "[ ] ";
            lines.push_back(
            hbox({
                text(prefix + mark),
                makeCardElement(bucket[idx], /*padded=*/false)
            })
            );
        }
        lines.push_back(text("Space=Toggle  Enter=Add  Esc=Back")|dim);
        }

        // no border/size on wizard
        auto wizard = vbox(std::move(lines));

        return vbox({ boardView, separator(), wizard });
    }),
    [&](Event e){
        if (mode==Mode::PICK_RANK) {
            std::size_t n   = ALL_RANKS.size();
            std::size_t vis = paneH - 2;
            if (e==Event::ArrowDown) {
                if (rankIdx+1 < n) {
                rankIdx++;
                if (rankIdx >= rankScroll + vis)
                    rankScroll = rankIdx - vis + 1;
                }
                return true;
            }
            if (e==Event::ArrowUp) {
                if (rankIdx>0) {
                    rankIdx--;
                    if (rankIdx < rankScroll)
                        rankScroll = rankIdx;
                }
                return true;
            }
            if (e==Event::Return) {
                currentRank = ALL_RANKS[rankIdx];
                mode = Mode::PICK_CARDS;
                cardIdx = cardScroll = 0;
                cardSelected.clear();
                return true;
            }
            if (e==Event::Escape) {
                localScreen.ExitLoopClosure()();
                shouldUpdate = false;
                return false;
            }
        } else {
        // PICK_CARDS
            auto bucket = bucket_for(*currentRank);
            std::size_t n   = bucket.size();
            std::size_t vis = paneH - 2;
            if (e==Event::ArrowDown) {
                if (cardIdx+1 < n) {
                    cardIdx++;
                    if (cardIdx >= cardScroll + vis)
                        cardScroll = cardIdx - vis + 1;
                }
                return true;
            }
            if (e==Event::ArrowUp) {
                if (cardIdx>0) {
                    cardIdx--;
                    if (cardIdx < cardScroll)
                        cardScroll = cardIdx;
                }
                return true;
            }
            if (e==Event::Character(' ')) {
                cardSelected[cardIdx] = !cardSelected[cardIdx];
                return true;
            }
            if (e==Event::Return) {
                // gather picked
                std::vector<Card> picked;
                for (std::size_t i=0; i<bucket.size(); ++i)
                    if (cardSelected[i])
                        picked.push_back(bucket[i]);
                if (!picked.empty()) {
                    Rank natural = *currentRank;
                    auto &mr = requestMap[natural];
                    mr.setRank(natural);
                    mr.appendCards(picked);
                    for (auto& c : picked)
                        working.removeCard(c);
                }
                mode = Mode::PICK_RANK;
                return true;
            }
            if (e==Event::Escape) {
                mode = Mode::PICK_RANK;
                return true;
            }
        }
        return false;
    }
    );

    auto loop = Loop(&localScreen, component);
    while (!loop.HasQuitted() && shouldUpdate) {
        loop.RunOnce();
    }

    // 4) Flatten into vector for server
    std::vector<MeldRequest> result;
    result.reserve(requestMap.size());
    for (auto& kv : requestMap)
        result.push_back(std::move(kv.second));
    return result;
}

Card GameView::runDiscardWizard(const BoardState& boardState) {
    console.clear();
    auto localScreen = ScreenInteractive::TerminalOutput();
    localScreen.TrackMouse(false);
    bool shouldUpdate = true;
    // 1) Working copy of the hand
    Hand working = boardState.getMyHand();

    // 2) Build a dynamic list of ranks present in hand
    std::vector<Rank> ranks;
    {
        std::unordered_set<Rank> seen;
        for (auto& c : working.getCards()) {
            if (seen.insert(c.getRank()).second) {
                ranks.push_back(c.getRank());
            }
        }
    }

    // State
    enum class Mode { PICK_RANK, PICK_CARD };
    Mode mode = Mode::PICK_RANK;
    std::size_t rankIdx = 0, rankScroll = 0;
    std::size_t cardIdx = 0, cardScroll = 0;
    std::optional<Rank> currentRank;
    std::optional<Card> result;  // the selected card

    const int paneH = 6;  // bottom pane height

    // Helper: label for a rank
    auto label_for_rank = [&](Rank r) {
    int cnt = 0;
    for (auto& c : working.getCards())
        if (c.getRank() == r) ++cnt;
    Card dummy{r, CardColor::BLACK};
    return getCardView(dummy).getLabel() + " (" + std::to_string(cnt) + ")";
    };

    // Helper: cards for a rank
    auto bucket_for = [&](Rank r) {
    std::vector<Card> bucket;
    for (auto& c : working.getCards())
        if (c.getRank() == r)
        bucket.push_back(c);
    return bucket;
    };

    // 3) Build and run the UI
    Component component = CatchEvent(
    Renderer([&] {
        auto boardView = makeBoard(boardState) | flex_grow;
        std::vector<Element> lines;

        if (mode == Mode::PICK_RANK) {
        // Rank list
        std::size_t n   = ranks.size();
        std::size_t vis = paneH - 2;
        rankIdx    = std::min<std::size_t>(rankIdx, n ? n - 1 : 0);
        rankScroll = std::min<std::size_t>(rankScroll, n > vis ? n - vis : 0);

        lines.push_back(text("Select rank to discard:"));
        for (std::size_t i = 0; i < vis; ++i) {
            std::size_t idx = i + rankScroll;
            if (idx >= n) break;
            std::string pre = (idx == rankIdx ? "→ " : "  ");
            lines.push_back(text(pre + label_for_rank(ranks[idx])));
        }
        lines.push_back(text("Enter=Pick rank") | dim);
        } else {
        // Card list
        Rank r        = *currentRank;
        auto bucket   = bucket_for(r);
        std::size_t n      = bucket.size();
        std::size_t vis    = paneH - 2;
        cardIdx       = std::min<std::size_t>(cardIdx, n ? n - 1 : 0);
        cardScroll    = std::min<std::size_t>(cardScroll, n > vis ? n - vis : 0);

        Card dummy{r, CardColor::BLACK};
        lines.push_back(text("Pick one `" +
            getCardView(dummy).getLabel() + "` to discard:"));

        for (std::size_t i = 0; i < vis; ++i) {
            std::size_t idx = i + cardScroll;
            if (idx >= n) break;
            std::string pre = (idx == cardIdx ? "→ " : "  ");
            lines.push_back(
                hbox({
                    text(pre),
                    makeCardElement(bucket[idx], /*padded=*/false)
                })
            );
        }
        lines.push_back(text("Enter=Discard") | dim);
        }

        auto wizard = vbox(std::move(lines));
        return vbox({ boardView, separator(), wizard });
    }),
    [&](Event e) {
        if (mode == Mode::PICK_RANK) {
        std::size_t n   = ranks.size();
        std::size_t vis = paneH - 2;
        if (e == Event::ArrowDown) {
            if (rankIdx + 1 < n) {
            rankIdx++;
            if (rankIdx >= rankScroll + vis)
                rankScroll = rankIdx - vis + 1;
            }
            return true;
        }
        if (e == Event::ArrowUp) {
            if (rankIdx > 0) {
            rankIdx--;
            if (rankIdx < rankScroll)
                rankScroll = rankIdx;
            }
            return true;
        }
        if (e == Event::Return) {
            currentRank = ranks[rankIdx];
            mode        = Mode::PICK_CARD;
            cardIdx = cardScroll = 0;
            return true;
        }
        } else {
        // PICK_CARD
        auto bucket = bucket_for(*currentRank);
        std::size_t n   = bucket.size();
        std::size_t vis = paneH - 2;
        if (e == Event::ArrowDown) {
            if (cardIdx + 1 < n) {
            cardIdx++;
            if (cardIdx >= cardScroll + vis)
                cardScroll = cardIdx - vis + 1;
            }
            return true;
        }
        if (e == Event::ArrowUp) {
            if (cardIdx > 0) {
            cardIdx--;
            if (cardIdx < cardScroll)
                cardScroll = cardIdx;
            }
            return true;
        }
        if (e == Event::Return) {
            // finalize
            result = bucket[cardIdx];
            localScreen.ExitLoopClosure()();
            shouldUpdate = false;
            return false;
        }
        }
        return false;
    }
    );

    auto loop = Loop(&localScreen, component);
    while (!loop.HasQuitted() && shouldUpdate) {
        loop.RunOnce();
    }
    return result.value();  // always set by Enter in PICK_CARD
}
