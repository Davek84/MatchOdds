#pragma once
#pragma comment(lib, "PluginSDK.lib")
#include "bakkesmod/plugin/bakkesmodplugin.h"
//#include "RankEnums.h"
//*****************#include "PlaylistData.h"
#include <string>

class matchodds : public BakkesMod::Plugin::BakkesModPlugin
{
private:
	std::shared_ptr<bool> bEnabled;
	std::shared_ptr<bool> bCommentaryEnabled;
	std::shared_ptr<bool> bPercentagesEnabled;
	std::shared_ptr<bool> bOnlyOnScoreboard;
	std::shared_ptr<bool> cl_commentator_override;
	std::shared_ptr<int> cl_commentator_x;
	std::shared_ptr<int> cl_commentator_y;
	std::shared_ptr<int> cl_commentator_textscale;
	std::shared_ptr<int> cl_commentator_imagescale;
	std::shared_ptr<bool> cl_percentage_override;
	std::shared_ptr<int> cl_percentageoffset_x;
	std::shared_ptr<int> cl_percentageoffset_y;
	std::shared_ptr<int> cl_percentage_textscale;
	std::shared_ptr<int> cl_dice_imagescale;
	std::shared_ptr<bool> cl_dice_visible;
	std::shared_ptr<std::string> cl_commentarytype;

	int rndNumber = 1;
	int tmpCounter = 0;
	int CommentaryCounter = 0;
	int TeamTotal[3] = { };
	int TeamTotalExtras[3] = { };
	int TeamScore[3] = { };
	int TeamBaselineMMR[3] = { };
	int TotalMMR = 0;
	int PlayerCount = 0;
	int PlayerMMRCaptured = 0;

	std::string MatchGUID;
	std::string OldMatchGUID;
	std::string TeamName[1] = {};
	long LocalTeam123 = 0;
	long tmpHighestMMR;
	std::string StarPlayerName;
	std::string MVPPlayerName;
	UniqueIDWrapper uniqueID;
	bool drawCanvas, isEnabled, gotNewMMR, isPlacement;
	int userPlaylist, userDiv, userTier, upperTier, lowerTier, upperDiv, lowerDiv, nextLower, beforeUpper;
	float userMMR = 0;
	std::string nameCurrent, nameNext, nameBefore, nextDiff, beforeDiff;
	bool isMatchEnded = true;
	bool isRoundStarted = false;
	bool isOvertime = false;
	std::string Commentary = "";
	LinearColor CommentaryColour = { 255, 255, 255, 255 }; // White
	LinearColor CommentatorColour = { 255, 255, 255, 255 }; // White
	int GoalDifference = 0;
	int WinningTeam = 0;
	bool isPredictedFavourite = false;
	bool capturedPredictedFavourite = false;
	int lastGoalScoredBy = 0;
	int randomValue = 0;
	enum  MatchStates
	{
		s_PreMatch,
		s_InMatch,
		s_GoalReplay,
		s_PostGoalCountdown,
		s_MatchEnd,
		s_MatchEndReplay,
		s_MatchEndPodium,
		s_Overtime,
	};
	MatchStates MatchState;
	bool isScoreboardOpen;


public:
	int getGameTime();
	int getMaxGameTime();
	int getGameTimeElapsed();
	std::string trimName(std::string str);
	void GetCurrentScore(std::string eventName = "", int TeamNum = -1);
	void onLoad() override;
	void onUnload() override;
	void UpdateTeamTotalExtras(int TeamNumber, int Amount);
	void statTickerEvent(ServerWrapper caller, void* args);
	void MatchEnded(std::string eventName);
	void MatchStarted(std::string eventName);
	void doesItTrigger(std::string eventName);
	void EndGameHighlights(std::string eventName);
	void PodiumMode(std::string eventName);
	void RoundEnded(std::string eventName);
	void RoundStarted(std::string eventName);
	void CountdownStarted(std::string eventName);
	void GameUpdated(std::string eventName);
	void CountdownEnded(std::string eventName);
	void BeginState(std::string eventName);
	void itsOvertime(std::string eventName);
	void OpenScoreboard(std::string eventName);
	void CloseScoreboard(std::string eventName);
	void LoadImgs();
	void ClearStats();
	std::shared_ptr<ImageWrapper> star;
	std::shared_ptr<ImageWrapper> dice;
	//std::shared_ptr<ImageWrapper> percentage;
	std::shared_ptr<ImageWrapper> commentator;

	void CalculateMMR(std::string eventName);
	void CalculateMVP();
	void MatchUnload(std::string eventName);
	void Render(CanvasWrapper canvas);
	void UpdateTeamTotal();
	void UpdateTotalMMR(int Team1, int Team2);
	int GetTeamTotal(int TeamNumber);
	void GetCommentary(std::string eventName = "", std::string playerName = "");
	void GetToxicCommentary(std::string eventName = "", std::string playerName = "");

	ServerWrapper GetCurrentServer();
	PriWrapper GetLocalPlayerPRI();

	//int unranker(int mode, int rank, int div, bool upperLimit);
	//DivisionData GetDivisionData(Playlist mode, Rank rank, int div);
	//void DebugGetDivisionData(std::vector<std::string> args);

	//void loadMenu(std::string eventName);
	//void CheckMMR(int retryCount);
};
