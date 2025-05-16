#ifndef GAME_VIEW_HPP
#define GAME_VIEW_HPP

#include <string>
#include <vector>
#include <atomic>
#include <algorithm>
#include <ftxui/dom/elements.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/component/loop.hpp>
#include <optional>
#include <map>
#include "card.hpp"
#include "hand.hpp"
#include "score_details.hpp"
#include "game_state.hpp"
#include "player_public_info.hpp"
#include "meld.hpp"
#include "rule_engine.hpp"
#include "client_deck.hpp"
#include "client/canasta_console.hpp"
#include "client/input_guard.hpp"

using namespace ftxui;

/**
 * @class CardView
 * @brief Class representing a card view for display purposes.
 */
class CardView {
private:
    std::string label;
    ftxui::Color color;
public:
    CardView(const std::string& label, ftxui::Color color)
        : label(label), color(color) {}

    const std::string& getLabel() const { return label; }
    ftxui::Color getColor() const { return color; }
};

/**
 * @class MeldView
 * @brief Class representing a meld view for display purposes.
 */
class MeldView {
private:
    Rank rank;
    std::vector<Card> cards;
    bool isInitializedFlag;
public:
    MeldView(Rank rank, const std::vector<Card>& cards, bool isInitialized)
        : rank(rank), cards(cards), isInitializedFlag(isInitialized) {}

    Rank getRank() const { return rank; }
    const std::vector<Card>& getCards() const { return cards; }
    bool isInitialized() const { return isInitializedFlag; }
};

/**
 * @class BoardState
 * @brief Class representing the game board state for display purposes.
 */
class BoardState {
private:
    std::vector<MeldView> myTeamMelds;
    std::vector<MeldView> opponentTeamMelds;
    Hand myHand;
    ClientDeck deckState;
    PlayerPublicInfo myPlayer;
    PlayerPublicInfo oppositePlayer;
    std::optional<PlayerPublicInfo> leftPlayer;
    std::optional<PlayerPublicInfo> rightPlayer;
    int myTeamTotalScore;
    int opponentTeamTotalScore;
    int myTeamMeldPoints;
    int opponentTeamMeldPoints;
public:

    // Getters
    std::vector<MeldView> getMyTeamMelds() const { return myTeamMelds; }
    std::vector<MeldView> getOpponentTeamMelds() const { return opponentTeamMelds; }
    Hand getMyHand() const { return myHand; }
    ClientDeck getDeckState() const { return deckState; }
    PlayerPublicInfo getMyPlayer() const { return myPlayer; }
    PlayerPublicInfo getOppositePlayer() const { return oppositePlayer; }
    std::optional<PlayerPublicInfo> getLeftPlayer() const { return leftPlayer; }
    std::optional<PlayerPublicInfo> getRightPlayer() const { return rightPlayer; }
    int getMyTeamTotalScore() const { return myTeamTotalScore; }
    int getOpponentTeamTotalScore() const { return opponentTeamTotalScore; }
    int getMyTeamMeldPoints() const { return myTeamMeldPoints; }
    int getOpponentTeamMeldPoints() const { return opponentTeamMeldPoints; }

    // Setters
    void setMyTeamMelds(const std::vector<MeldView>& melds) { myTeamMelds = melds; }
    void setOpponentTeamMelds(const std::vector<MeldView>& melds) { opponentTeamMelds = melds; }
    void setMyHand(const Hand& hand) { myHand = hand; }
    void setDeckState(const ClientDeck& deck) { deckState = deck; }
    void setMyPlayer(const PlayerPublicInfo& player) { myPlayer = player; }
    void setOppositePlayer(const PlayerPublicInfo& player) { oppositePlayer = player; }
    void setLeftPlayer(const PlayerPublicInfo& player) { leftPlayer = player; }
    void setRightPlayer(const PlayerPublicInfo& player) { rightPlayer = player; }
    void setMyTeamTotalScore(int score) { myTeamTotalScore = score; }
    void setOpponentTeamTotalScore(int score) { opponentTeamTotalScore = score; }
    void setMyTeamMeldPoints(int points) { myTeamMeldPoints = points; }
    void setOpponentTeamMeldPoints(int points) { opponentTeamMeldPoints = points; }
};

/**
 * @class ScoreState
 * @brief Class representing the score state for display purposes.
 */
class ScoreState {
private:
    ScoreBreakdown myTeamScoreBreakdown;
    ScoreBreakdown opponentTeamScoreBreakdown;
    std::size_t playersCount;
    int myTeamTotalScore;
    int opponentTeamTotalScore;
    bool isGameOver;
    std::optional<ClientGameOutcome> gameOutcome;
public:

    // Getters
    ScoreBreakdown getMyTeamScoreBreakdown() const { return myTeamScoreBreakdown; }
    ScoreBreakdown getOpponentTeamScoreBreakdown() const { return opponentTeamScoreBreakdown; }
    std::size_t getPlayersCount() const { return playersCount; }
    int getMyTeamTotalScore() const { return myTeamTotalScore; }
    int getOpponentTeamTotalScore() const { return opponentTeamTotalScore; }
    bool getIsGameOver() const { return isGameOver; }
    std::optional<ClientGameOutcome> getGameOutcome() const { return gameOutcome; }

    // Setters
    void setMyTeamScoreBreakdown(const ScoreBreakdown& breakdown) { myTeamScoreBreakdown = breakdown; }
    void setOpponentTeamScoreBreakdown(const ScoreBreakdown& breakdown) { opponentTeamScoreBreakdown = breakdown; }
    void setPlayersCount(std::size_t count) { playersCount = count; }
    void setMyTeamTotalScore(int score) { myTeamTotalScore = score; }
    void setOpponentTeamTotalScore(int score) { opponentTeamTotalScore = score; }
    void setIsGameOver(bool gameOver) { isGameOver = gameOver; }
    void setGameOutcome(std::optional<ClientGameOutcome> outcome) { gameOutcome = outcome; }
};


/**
 * @class GameView
 * @brief Class responsible for displaying the game state and handling user input.
 */
class GameView {
public:
    /**
     * @brief Constructor for GameView.
     */
    GameView();
    /**
     * @brief Prompt the user for a text input.
     * @param question The question to display.
     * @param placeholder The placeholder text to display.
     * @return The user's input as a string.
     */
    std::string promptString(const std::string& question, std::string& placeholder);
    /**
     * @brief Display the game board and prompt the user for input.
     * @param question The question to display.
     * @param options The options for the user to choose from.
     * @param boardState The current state of the game board.
     * @param message Optional message to display.
     * @return The index of the selected option.
     */
    int promptChoiceWithBoard(const std::string& question, const std::vector<std::string>& options,
        const BoardState& boardState, std::optional<const std::string> message = std::nullopt);
    /**
     * @brief Display the game board and get the user's meld requests.
     * @param boardState The current state of the game board.
     * @return A vector of meld requests.
     */
    std::vector<MeldRequest> runMeldWizard(const BoardState& boardState);
    /**
     * @brief Display the game board and get the user's discard card.
     * @param boardState The current state of the game board.
     * @return The selected card to discard.
     */
    Card runDiscardWizard(const BoardState& boardState);
    /**
     * @brief Display the game board with messages.
     * @details This method clears the console, disables input, and shows the game board with messages.
     * @param messages The messages to display.
     * @param boardState The current state of the game board.
     */
    void showStaticBoardWithMessages(
        const std::vector<std::string>& messages, const BoardState& boardState);
    /**
     * @brief Display the game score.
     * @details This method clears the console, disables input, and shows the game score.
     * @param scoreState The current state of the game score.
     */
    void showStaticScore(const ScoreState& scoreState);
    /**
     * @brief Restore the input for the console.
     */
    void restoreInput();

private:
    CanastaConsole console;                 ///< Console for output
    ScreenInteractive screen;               ///< Screen for interactive input/output
    std::optional<InputGuard> inputGuard;   /// Input guard for console state
    
    /**
     * @brief Get the card view for display purposes.
     * @param card The card to display.
     * @return The CardView containing the label and color.
     */
    CardView getCardView(const Card& card);
    /**
     * @brief Create a card element for display.
     * @param card The card to display.
     * @param padded Whether to pad the card label.
     * @return The element representing the card.
     */
    Element makeCardElement(const Card& card, bool padded = true);
    /**
     * @brief Create a board element for display.
     * @param boardState The current state of the game board.
     * @return The element representing the game board.
     */
    Element makeBoard(const BoardState& boardState);
    /**
     * @brief Create a hand grid element for display.
     * @param hand The hand to display.
     * @return The element representing the hand grid.
     */
    Element makeHandGrid(const Hand& hand);
    /**
     * @brief Create a deck info element for display.
     * @param deck The deck to display.
     * @return The element representing the deck info.
     */
    Element makeDeckInfo(const ClientDeck& deck);
    /**
     * @brief Create a score info element for display.
     * @param myTeamTotalScore The total score of the player's team.
     * @param opponentTeamTotalScore The total score of the opponent's team.
     * @param myTeamMeldPoints The meld points of the player's team.
     * @param opponentTeamMeldPoints The meld points of the opponent's team.
     * @param textColor1 The color for the first text.
     * @param textColor2 The color for the second text.
     * @return The element representing the score info.
     */
    Element makeScoreInfo(int myTeamTotalScore, int opponentTeamTotalScore,
        int myTeamMeldPoints, int opponentTeamMeldPoints, Color textColor1, Color textColor2);
    /**
     * @brief Create a meld grid element for display.
     * @param melds The meld views to display.
     * @param frameColor The color for the frame.
     * @return The element representing the meld grid.
     */
    Element makeMeldGrid(const std::vector<MeldView>& melds, Color frameColor);
    /**
     * @brief Create a player info element for display.
     * @param player The player public info to display.
     * @return The element representing the player info.
     */
    Element makePlayerInfo(const PlayerPublicInfo& player);
    /**
     * @brief Disable input for the console.
     */
    void disableInput();
};

#endif // GAME_VIEW_HPP