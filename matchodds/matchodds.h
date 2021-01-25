#pragma once
#pragma comment(lib, "PluginSDK.lib")
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "RankEnums.h"
#include "PlaylistData.h"
#include <string>

class matchodds : public BakkesMod::Plugin::BakkesModPlugin
{
private:
	std::shared_ptr<bool> bEnabled;
	int TeamTotal[3] = { };
	int TeamTotalExtras[3] = { };
	int TeamScore[3] = { };
	int TeamBaselineMMR[3] = { };
	int TotalMMR = 0;
	int PlayerCount = 0;
	int PlayerMMRCaptured = 0;
	//int TeamAverage[3] = { };
	std::string TeamName[1] = {};
	long LocalTeam123 = 0;
	long LocalTeam1233 = 0;
	long LocalTeam1234 = 0;
	long tmpHighestMMR;
	std::string tmpHighestMMRName;
	UniqueIDWrapper uniqueID;
	bool drawCanvas, isEnabled, gotNewMMR, isPlacement;
	int userPlaylist, userDiv, userTier, upperTier, lowerTier, upperDiv, lowerDiv, nextLower, beforeUpper;
	float userMMR = 0;
	std::string nameCurrent, nameNext, nameBefore, nextDiff, beforeDiff;
	bool isMatchEnded;
	bool isMatchStarted;
	int initialRand = 1;
	std::string Commentry = "";
	LinearColor CommentryColour = { 255, 255, 255, 255 }; // White

public:
	int getGameTime();
	int getGameTimeElapsed();
	void GetCurrentScore();
	void onLoad() override;
	void onUnload() override;
	void UpdateTeamTotalExtras(int TeamNumber, int Amount);
	void statTickerEvent(ServerWrapper caller, void* args);
	void MatchEnded(std::string eventName);
	void MatchStarted(std::string eventName);
	void LoadImgs();
	std::shared_ptr<ImageWrapper> star;
	std::shared_ptr<ImageWrapper> dice;
	std::shared_ptr<ImageWrapper> percentage;
	std::shared_ptr<ImageWrapper> commentator;
	
	void CalculateStats(std::string eventName);
	void MatchUnload(std::string eventName);
	void Render(CanvasWrapper canvas);
	void UpdateTeamTotal();
	void UpdateTotalMMR(int Team1, int Team2);
	int GetTeamTotal(int TeamNumber);
	void GetCommentry();

	ServerWrapper GetCurrentServer();
	PriWrapper GetLocalPlayerPRI();
	std::string matchGUID;
	std::string OldmatchGUID;
	//int unranker(int mode, int rank, int div, bool upperLimit);
	//DivisionData GetDivisionData(Playlist mode, Rank rank, int div);
	//void DebugGetDivisionData(std::vector<std::string> args);

	//void loadMenu(std::string eventName);
	//void CheckMMR(int retryCount);
};
