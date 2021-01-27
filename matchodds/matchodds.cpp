#include "pch.h"
#include "matchodds.h"
#include "bakkesmod/wrappers/MMRWrapper.h"
#include "String.h"
#include "RankEnums.h"
#include "PlaylistData.h"
#include <cmath>
#include "bakkesmod/wrappers/GameObject/Stats/StatEventWrapper.h"

#define LINMATH_H
long tmpCount = 0;

BAKKESMOD_PLUGIN(matchodds, "Shows the match favourite + ongoing % chance of winning", "1.3", 0x0)

void matchodds::onLoad()
{
	LoadImgs();
	bEnabled = std::make_shared<bool>(true);
	cvarManager->registerCvar("matchodds_enabled", "1", "Enable MatchOdds", true, true, 0, true, 1).bindTo(bEnabled);
	bCommentaryEnabled = std::make_shared<bool>(true);
	cvarManager->registerCvar("matchodds_commentaryenabled", "1", "Enable Commentary", true, true, 0, true, 1).bindTo(bCommentaryEnabled);
	
	gameWrapper->RegisterDrawable(bind(&matchodds::Render, this, std::placeholders::_1));

	//gameWrapper->HookEvent("Function Engine.Actor.MatchStarting", std::bind(&matchodds::MatchStarting, this, std::placeholders::_1));

	gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.StartNewGame", std::bind(&matchodds::MatchStarted, this, std::placeholders::_1));

	gameWrapper->HookEvent("Function GameEvent_Soccar_TA.Active.StartRound", std::bind(&matchodds::RoundStarted, this, std::placeholders::_1)); // Triggered at the start of the game when the round starts + when a goal is scored + after countdown finishes

	//gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.OnMatchWinnerSet", bind(&matchodds::CalculateStats, this, std::placeholders::_1)); 
	//gameWrapper->HookEvent("Function TAGame.Team_TA.EventScoreUpdated", std::bind(&matchodds::CalculateStats, this, std::placeholders::_1));
	//gameWrapper->HookEvent("Function OnlineGameJoinGame_X.JoiningBase.IsJoiningGame", std::bind(&matchodds::CalculateStats, this, std::placeholders::_1));

	gameWrapper->HookEvent("Function TAGame.GameEvent_TA.EventPlayerAdded", std::bind(&matchodds::CalculateStats, this, std::placeholders::_1)); // Triggers
	gameWrapper->HookEvent("Function TAGame.GameEvent_TA.EventPlayerRemoved", std::bind(&matchodds::CalculateStats, this, std::placeholders::_1)); // Triggers

	gameWrapper->HookEvent("Function GameEvent_TA.Countdown.BeginState", std::bind(&matchodds::CalculateStats, this, std::placeholders::_1));  // Triggers

	gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.OnAllTeamsCreated", std::bind(&matchodds::CalculateStats, this, std::placeholders::_1)); // vs. Bots Triggered before selecting a team to join (very start of the game!)
	gameWrapper->HookEvent("Function TAGame.PRI_TA.PostBeginPlay", std::bind(&matchodds::CalculateStats, this, std::placeholders::_1)); // Triggered when somone joins?

	gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.EndRound", std::bind(&matchodds::RoundEnded, this, std::placeholders::_1)); // Doesn't trigger
	
	gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.EventMatchEnded", std::bind(&matchodds::MatchEnded, this, std::placeholders::_1)); // Triggers at the end of the match 10s before 'Function TAGame.GameEvent_Soccar_TA.BeginHighlightsReplay'
	gameWrapper->HookEvent("Function TAGame.GFxShell_TA.LeaveMatch", std::bind(&matchodds::MatchEnded, this, std::placeholders::_1));

	// Test Triggers:
	//gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.PreMatchLobby.OnAllPlayersReady", std::bind(&matchodds::doesItTrigger, this, std::placeholders::_1)); // Doesn't exist
	//gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.PreMatchLobby.BeginState", std::bind(&matchodds::doesItTrigger, this, std::placeholders::_1)); // Doesn't exist
	//gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.PostGoalScored.BeginState", std::bind(&matchodds::doesItTrigger, this, std::placeholders::_1)); // Doesn't exist
	//gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.PostGoalScored.EndState", std::bind(&matchodds::doesItTrigger, this, std::placeholders::_1)); // Doesn't exist
	//gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.PodiumSpotlight.BeginState", std::bind(&matchodds::doesItTrigger, this, std::placeholders::_1)); // Doesn't exist
	//gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.PodiumSpotlight.EndState", std::bind(&matchodds::doesItTrigger, this, std::placeholders::_1)); // Doesn't exist
	gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.BeginHighlightsReplay", std::bind(&matchodds::EndGameHighlights, this, std::placeholders::_1)); // Very end of game whilst replay is showing behind the scoreboard
	gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.SpawnPodiumCars", std::bind(&matchodds::doesItTrigger, this, std::placeholders::_1));
	//gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.WaitingForPlayers.BeginState", std::bind(&matchodds::doesItTrigger, this, std::placeholders::_1)); // Doesn't exist
	//gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.WaitingForPlayers.EndState", std::bind(&matchodds::doesItTrigger, this, std::placeholders::_1)); // Doesn't exist
	//gameWrapper->HookEvent("Function TAGame.GameEvent_TA.Countdown.EndState", std::bind(&matchodds::doesItTrigger, this, std::placeholders::_1));   // Doesn't exist
	//gameWrapper->HookEvent("Function TAGame.GameEvent_TA.Finished.OnAllPlayersReady", std::bind(&matchodds::doesItTrigger, this, std::placeholders::_1));   // Doesn't exist 
	//gameWrapper->HookEvent("Function TAGame.GameEvent_TA.FinishedBase.IsFinished", std::bind(&matchodds::doesItTrigger, this, std::placeholders::_1));   // Doesn't exist
	gameWrapper->HookEvent("Function Engine.GameReplicationInfo.StartMatch", std::bind(&matchodds::doesItTrigger, this, std::placeholders::_1));  // 
	gameWrapper->HookEvent("Function TAGame.GameEvent_Lobby_TA.StartFirstState", std::bind(&matchodds::doesItTrigger, this, std::placeholders::_1));  // 
	gameWrapper->HookEvent("Function TAGame.GameEvent_TA.StartInitialCountDown", std::bind(&matchodds::doesItTrigger, this, std::placeholders::_1));  // 

	gameWrapper->HookEventWithCallerPost<ServerWrapper>(
		"Function TAGame.GFxHUD_TA.HandleStatTickerMessage",
		std::bind(&matchodds::statTickerEvent, this,
			std::placeholders::_1, std::placeholders::_2));

	gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.EventOvertimeUpdated", std::bind(&matchodds::itsOvertime, this, std::placeholders::_1));
	//gameWrapper->HookEvent("Function TAGame.CrowdSoundManager_TA.Tick", std::bind(&matchodds::GameUpdated, this, std::placeholders::_1));

//Function TAGame.GFxData_DateTime_TA.AddSeconds
//Function Engine.Actor.MatchStarting

}
// The structure of a ticker stat event
struct TickerStruct {
	// person who got a stat
	uintptr_t Receiver;
	// person who is victim of a stat (only exists for demos afaik)
	uintptr_t Victim;
	// wrapper for the stat event
	uintptr_t StatEvent;
};
//const std::map<std::string, int> eventDictionary = {
//	{ "Demolition", demos},
//	{ "Extermination", exterms},
//	{ "Goal", goals},
//	{ "Win", wins},
//	{ "MVP", mvps},
//	{ "Aerial Goal", aerialGoals},
//	{ "Backwards Goal", backwardsGoals},
//	{ "Bicycle Goal", bicycleGoals},
//	{ "Long Goal", longGoals},
//	{ "Turtle Goal", turtleGoals},
//	{ "Pool Shot", poolShots},
//	{ "Overtime Goal", overtimeGoals},
//	{ "Hat Trick", hatTricks},
//	{ "Assist", assists},
//	{ "Playmaker", playmakers},
//	{ "Save", saves},
//	{ "Epic Save", epicSaves},
//	{ "Savior", saviors},
//	{ "Shot on Goal", shots},
//	{ "Center Ball", centers},
//	{ "Clear Ball", clears},
//	{ "First Touch", firstTouchs},
//	{ "Damage", damages},
//	{ "Ultra Damage", ultraDamages},
//	{ "Low Five", lowFives},
//	{ "High Five", highFives},
//	{ "Swish Goal", swishs},
//	{ "Bicycle Hit", bicycleHits}
//};

void matchodds::statTickerEvent(ServerWrapper caller, void* args) {
	auto tArgs = (TickerStruct*)args;
	
	if (!(*bEnabled)) return;
	if (!gameWrapper->IsInOnlineGame()) return;

	// separates the parts of the stat event args
	auto receiver = PriWrapper(tArgs->Receiver);
	auto victim = PriWrapper(tArgs->Victim);
	auto statEvent = StatEventWrapper(tArgs->StatEvent);
	// name of the stat as shown in rocket league 
	//  (Demolition, Extermination, etc.)
	std::string EventName = statEvent.GetLabel().ToString();

	//if (EventName == "Goal") UpdateTeamTotalExtras(receiver.GetTeamNum() + 1, 200); //Goal Scored, give + 200 MMR
	if (EventName == "Goal") {
		lastGoalScoredBy = receiver.GetTeamNum() + 1;
		GetCurrentScore();
		CalculateStats("Goal");
		GetCommentary("Goal");
	} else { // TODO: Find a less hacky way of updating the stats during the game...
		
		GetCurrentScore();
		UpdateTeamTotal(); 
		GetCommentary();
	}

    


}
void::matchodds::GetCommentary(std::string eventName) {

	if (!(*bEnabled)) return;
	if (!gameWrapper->IsInOnlineGame()) return;
	bool isFavourite = false;
	Commentary = "";
	if (PlayerMMRCaptured == PlayerCount) { // Do we have MMR for all players?

		if (LocalTeam123 == 1) {
			if (GetTeamTotal(1) > GetTeamTotal(2)) {
				isFavourite = true;
			}
			else {
				isFavourite = false;
			}
		}
		else {
			if (GetTeamTotal(2) > GetTeamTotal(1)) {
				isFavourite = true;
			}
			else {
				isFavourite = false;
			}
		}
		if (getGameTime() > (getMaxGameTime() - 10)) { // Very Start of the game so store the predicted favourites

			isPredictedFavourite = isFavourite; // Store the predicted favourite somewhere
			if (isFavourite == true) Commentary = "You are the favorites!  Star Player: " + StarPlayerName;
			if (isFavourite == false) Commentary = "You are the underdogs...  Star Player: " + StarPlayerName;
		}
		else if (getGameTime() > 260 && getGameTime() < 291) { // Start of game (if it's the standard 5 min match)
			if (isFavourite && isPredictedFavourite) Commentary = "You are the favorites!  One to watch: " + StarPlayerName;
			if (!isFavourite && isPredictedFavourite) Commentary = "Don't let it slip away...  One to watch: " + StarPlayerName;
			if (isFavourite && !isPredictedFavourite) Commentary = "You're doing great!! One to watch: " + StarPlayerName;
			if (!isFavourite && !isPredictedFavourite) Commentary = "Push a little harder.  One to watch: " + StarPlayerName;

		}
		else if (getGameTime() > 120 && getGameTime() < 261) { // Mid Game
			if (isFavourite == true) Commentary = "...";
			if (isFavourite == false) Commentary = "...";
			
		}
		else if (getGameTime() > 60 && getGameTime() < 121) {
			if (isFavourite && isPredictedFavourite) Commentary = "You are still the favorites!  Current MVP: " + MVPPlayerName;
			if (!isFavourite && isPredictedFavourite) Commentary = "Don't let it slip away...  Current MVP: " + MVPPlayerName;
			if (isFavourite && !isPredictedFavourite) Commentary = "You're doing great!! Current MVP: " + MVPPlayerName;
			if (!isFavourite && !isPredictedFavourite) Commentary = "Push a little harder.  Current MVP: " + MVPPlayerName;

		}
		else if (getGameTime() > 1 && getGameTime() < 61) { // Final minute
			if (isFavourite && isPredictedFavourite) Commentary = "Come on! You're still the favorites!";
			if (!isFavourite && isPredictedFavourite) Commentary = "Nooo, what's going wrong??";
			if (isFavourite && !isPredictedFavourite) Commentary = "Amazing work! You were the underdogs!";
			if (!isFavourite && !isPredictedFavourite) Commentary = "It's the taking part that counts...";

		}
		else if (getGameTime() < 1) { // Basically the end of the match...
			if (isPredictedFavourite && WinningTeam == LocalTeam123) {
				Commentary = "I predicted a win for you and you delivered! Good job!";
			} else if (isPredictedFavourite && WinningTeam != LocalTeam123) {
				Commentary = "What happened?!? I had you down as favorites!";
			}
			else if (isPredictedFavourite == false && WinningTeam == LocalTeam123) {
				Commentary = "Wow! Well done! You defied the odds and won that one!";
			}
			else if (isPredictedFavourite == false && WinningTeam != LocalTeam123) {
				Commentary = "Well.. it's the taking part that counts...";
			}
			
		}


		if (isFavourite == true) CommentaryColour = { 150,200,150,255 }; // Green
		if (isFavourite == false) CommentaryColour = CommentaryColour = { 200,150,150,255 }; // Red?

		if (eventName == "Goal") {
			//Commentary = "Goal, scored by Team: " + std::to_string(lastGoalScoredBy);
			if ((lastGoalScoredBy) == LocalTeam123) { // Your team scored
				if (WinningTeam == LocalTeam123) { // You're winning
					Commentary = "Goooaaaalllll!!";
				}
				else { // You're losing
					Commentary = "You've pulled one back!";
				}
			}
			else { // Oppenent scored
				if (WinningTeam == LocalTeam123) {  // You're still winning though!
					Commentary = "No harm, you're still winning!";
				}
				else { // You're losing...
					Commentary = "FF ? xD";
				}
			}
			
		}

		//if (eventName == "GameUpdated") Commentary = "Debug: getGameTime=" + std::to_string(tmpCount++);


		if (isOvertime) { //TODO: Add # of shots for each team , super handy to see during Overtime in a tournament!
			if (isFavourite == true) Commentary = "It's overtime! Time to focus! You're the favourites here!";
			if (isFavourite == false) Commentary = "Go for the upset! Cmon you underdogs!";
		}
	}
	else {
		Commentary = "Calculating prediction...";
	}
	//if (isMatchEnded == true) Commentary = "..."; // TODO: Actually identify when user is on the post match screen
}
void matchodds::UpdateTeamTotalExtras(int TeamNumber, int Amount) {
	TeamTotalExtras[TeamNumber] += Amount;
}
void matchodds::onUnload() 
{
	gameWrapper->UnhookEvent("Function TAGame.GameEvent_Soccar_TA.EventMatchEnded");
	gameWrapper->UnregisterDrawables();
}

void matchodds::GetCurrentScore() {
	if (!(*bEnabled)) return;
	if (!gameWrapper->IsInOnlineGame() && !gameWrapper->IsInReplay()) return;

	//TODO: GoalMMR should be dynamic based on Team Size "a 3 goal advantage is more significant in 3v3 vs 1v1" thanks DrStein#6280 for the suggestion
	int GoalMMR = 200; // How  much to add on for a goal
	int TimeMMR1 = 0;
	int TimeMMR2 = 0;
	GoalDifference = 0;
	WinningTeam = 0;
	ServerWrapper onlineServer = gameWrapper->GetOnlineGame();
	ArrayWrapper<TeamWrapper> localServerTeams = onlineServer.GetTeams();
	for (TeamWrapper team : localServerTeams) {
		if (team.GetTeamNum() == 0) TeamScore[1] = team.GetScore();
		if (team.GetTeamNum() == 1) TeamScore[2] = team.GetScore();
	}

	if (TeamScore[1] == TeamScore[2]) { //Draw, do nothing...
		
	}
	else if (TeamScore[1] > TeamScore[2]) { //Blue Winning
		GoalDifference = TeamScore[1] - TeamScore[2];
		WinningTeam = 1;
		TimeMMR1 = static_cast<int>((getGameTimeElapsed() * GoalDifference) * 5);
	}
	else if (TeamScore[2] > TeamScore[1]) { 	//Orange Winning
		WinningTeam = 2;
		GoalDifference = TeamScore[2] - TeamScore[1];
		TimeMMR2 = static_cast<int>((getGameTimeElapsed() * GoalDifference) * 5);
	}

	//Goals count for Extra MMR
	TeamTotalExtras[1] = static_cast<int>((TeamScore[1] * GoalMMR) + TimeMMR1);
	TeamTotalExtras[2] = static_cast<int>((TeamScore[2] * GoalMMR) + TimeMMR2);
	//cvarManager->log("TExtras1: " + std::to_string(TeamTotalExtras[1]) + " TExtras2: " + std::to_string(TeamTotalExtras[2]));
}

void matchodds::UpdateTeamTotal() {
	TeamTotal[1] = TeamBaselineMMR[1] + TeamTotalExtras[1];
	TeamTotal[2] = TeamBaselineMMR[2] + TeamTotalExtras[2];
	UpdateTotalMMR(TeamTotal[1], TeamTotal[2]);
	//cvarManager->log("UpdateTeamTotal:  " + std::to_string(Team1) + " | " + std::to_string(Team2));
	//cvarManager->log("UpdateTeamTotal:  " + std::to_string(TeamTotal[0]) + " | " + std::to_string(TeamTotal[1]));
}

int matchodds::GetTeamTotal(int TeamNumber) {
	if (TeamNumber == 1) return TeamTotal[1];
	if (TeamNumber == 2) return TeamTotal[2];
}

void matchodds::UpdateTotalMMR(int Team1, int Team2) {
	TotalMMR = Team1 + Team2;
	//cvarManager->log("UpdateTotalMMR:  " + std::to_string(TotalMMR));
}
void matchodds::doesItTrigger(std::string eventName) {
	cvarManager->log("Triggered: " + eventName);
}
ServerWrapper matchodds::GetCurrentServer()
{
	if (this->gameWrapper->IsInReplay())
		return this->gameWrapper->GetGameEventAsReplay().memory_address;
	else if (this->gameWrapper->IsInOnlineGame())
		return this->gameWrapper->GetOnlineGame();
	else if (this->gameWrapper->IsInFreeplay())
		return this->gameWrapper->GetGameEventAsServer();
	else if (this->gameWrapper->IsInCustomTraining())
		return this->gameWrapper->GetGameEventAsServer();
	else if (this->gameWrapper->IsSpectatingInOnlineGame())
		return this->gameWrapper->GetOnlineGame();
	else
		return NULL;
}

void matchodds::MatchUnload(std::string eventName) {
	OldmatchGUID = matchGUID;
}

void matchodds::MatchEnded(std::string eventName) { // Triggered at the very end of the game
	 MatchState = s_MatchEnd;
	//cvarManager->log("Match Ended " + eventName);
	isMatchEnded = true;
	isOvertime = false;
	GoalDifference = 0;
	WinningTeam = 0;
	isPredictedFavourite = false;
	lastGoalScoredBy = 0;
	randomValue = rand() % 1; // "Random" number between 0 and 1

}
void matchodds::RoundEnded(std::string eventName) { // Triggered after a goal is scored?
	MatchStates MatchState = s_InMatch;
	cvarManager->log("Round Ended: " + eventName);
}
void matchodds::MatchStarted(std::string eventName) { // Triggered at the very start?
	 MatchState = s_PreMatch;
	//cvarManager->log("Match Started " + eventName);
	if (!gameWrapper->IsInOnlineGame()) {
		GetCommentary("GameUpdated");
		return;
	}
}
void matchodds::GameUpdated(std::string eventName) {
	//if (gameWrapper->IsInGame() == 1) {
		//if (isMatchEnded == false) GetCommentary("GameUpdated");
	//}
	//isMatchEnded = false;
	GetCommentary("GameUpdated " + eventName);
}

void matchodds::itsOvertime(std::string eventName) { // Triggered when Overtime is started
	 MatchState = s_Overtime;
	 isOvertime = true;
	GetCommentary();
}
void matchodds::RoundStarted(std::string eventName) { // Triggered when a goal is scored...
	cvarManager->log("Round Started " + eventName);
	 MatchState = s_InMatch;
	isMatchEnded = false;
	TeamTotalExtras[1] = 0;
	TeamTotalExtras[2] = 0;
	GetCommentary();
}
void matchodds::EndGameHighlights(std::string eventName) {
	 MatchState = s_MatchEndReplay;
	 GetCommentary();
	 GoalDifference = 0;
	 WinningTeam = 0;
}




void matchodds::CalculateStats(std::string eventName)
{
	cvarManager->log("Calc Stats: " + eventName);
	if (!(*bEnabled)) return;
	if (!gameWrapper->IsInOnlineGame()) return;
	ServerWrapper server = GetCurrentServer();
	cvarManager->log("Online Game");
	int tmpTeamTotal[2] = {1,1 };
	long tmpHighestMVP = 0;
	tmpHighestMMR = 0;
	StarPlayerName = "";
	tmpTeamTotal[0] = 0;
	tmpTeamTotal[1] = 0;
	LocalTeam123 = 0;
	MMRWrapper mw = gameWrapper->GetMMRWrapper();
	ArrayWrapper<PriWrapper> pris = server.GetPRIs();

	int len = pris.Count();
	PlayerCount = static_cast<int>(server.GetMaxTeamSize() * 2);

	char tmpGetTeamIndex;
	if (len < 1) return;

	PriWrapper localPlayer = GetLocalPlayerPRI();
	if (!localPlayer) return;
	if (localPlayer.GetTeamNum() == 0) LocalTeam123 = 1;
	if (localPlayer.GetTeamNum() == 1) LocalTeam123 = 2;
	PlayerMMRCaptured = 0;
	long tmpMMR;
	int tmpMVP;
	tmpMVP = 0;
	tmpMMR = 0;
	std::string playerName;
	for (int i = 0; i < len; i++)
	{
		PriWrapper player = pris.Get(i);
		tmpGetTeamIndex = player.GetTeamNum();
		playerName = player.GetPlayerName().ToString();
		
		//if (gameWrapper->GetMMRWrapper().IsSynced(uniqueID, userPlaylist) && !gameWrapper->GetMMRWrapper().IsSyncing(uniqueID)) {
			tmpMMR = gameWrapper->GetMMRWrapper().GetPlayerMMR(
				pris.Get(i).GetUniqueIdWrapper(),
				gameWrapper->GetMMRWrapper().GetCurrentPlaylist());
			if (tmpMMR > 1) {
				PlayerMMRCaptured++;
				if (std::to_string(tmpGetTeamIndex) == "0") {
					tmpTeamTotal[0] += tmpMMR;

				}
				if (std::to_string(tmpGetTeamIndex) == "1") {
					tmpTeamTotal[1] += tmpMMR;
				}
			}
			TeamBaselineMMR[1] = tmpTeamTotal[0];
			TeamBaselineMMR[2] = tmpTeamTotal[1];
			// Keep track of highest MMR player (Star Player)
			if (tmpMMR >= tmpHighestMMR)
			{
				tmpHighestMMR = tmpMMR; // # of MMR the player has
				StarPlayerName = playerName; // The name of the player
			}

			
			
		//}
		
		// Keep track of highest Score (MVP)
		tmpMVP = player.GetMatchScore();
		if (tmpMVP >= tmpHighestMVP)
		{
			tmpHighestMVP = tmpMVP; // # of MMR the player has
			MVPPlayerName = playerName; // The name of the player
		}

	}
	GetCurrentScore();
	UpdateTeamTotal();

	//cvarManager->log("Finished Calc Stats");
}

int matchodds::getGameTime()
{
	ServerWrapper onlineServer = gameWrapper->GetOnlineGame();

	if (!gameWrapper->IsInGame())
	{
		if (!gameWrapper->IsInOnlineGame())
			return -1;
		else if (onlineServer.IsNull())
			return -1;
		return onlineServer.GetSecondsRemaining();
	}
	
}
int matchodds::getGameTimeElapsed()
{
	ServerWrapper onlineServer = gameWrapper->GetOnlineGame();

	if (!gameWrapper->IsInGame())
	{
		if (!gameWrapper->IsInOnlineGame())
			return -1;
		else if (onlineServer.IsNull())
			return -1;
		return onlineServer.GetSecondsElapsed();
	}

}
int matchodds::getMaxGameTime()
{
	ServerWrapper onlineServer = gameWrapper->GetOnlineGame();

	if (!gameWrapper->IsInGame())
	{
		if (!gameWrapper->IsInOnlineGame())
			return -1;
		else if (onlineServer.IsNull())
			return -1;
		return onlineServer.GetGameTime();
	}

}

PriWrapper matchodds::GetLocalPlayerPRI()
{
	auto server = GetCurrentServer();
	if (server.IsNull())
		return NULL;
	auto player = server.GetLocalPrimaryPlayer();
	if (player.IsNull())
		return NULL;
	return player.GetPRI();
}

void matchodds::LoadImgs() // Load up the images used to display ingame
{
	//TODO: Make this less hacky
	std::wstring tmpWide(gameWrapper->GetDataFolderW());
	std::string tmpDataDir(tmpWide.begin(), tmpWide.end());

	star = std::make_shared<ImageWrapper>(tmpDataDir + "\\matchodds\\star.png", true);
	dice = std::make_shared<ImageWrapper>(tmpDataDir + "\\matchodds\\dice.png", true);
	percentage = std::make_shared<ImageWrapper>(tmpDataDir + "\\matchodds\\percentage.png", true);
	commentator = std::make_shared<ImageWrapper>(tmpDataDir + "\\matchodds\\commentator.png", true);
	
	star->LoadForCanvas();
	dice->LoadForCanvas();
	percentage->LoadForCanvas();
	commentator->LoadForCanvas();
}

void matchodds::Render(CanvasWrapper canvas)
{
	if (!(*bEnabled)) return; // Don't display if the plugin is disabled...
	if (gameWrapper->IsInGame() == 1 || gameWrapper->IsInOnlineGame() == 1 || gameWrapper->IsInReplay() == 1) { //Display if user is in an online game (Casual, Ranked, Tournament)
		if (StarPlayerName == "") return; //Only render if we have some MMR data
		int ScreenY = gameWrapper->GetScreenSize().Y;
		float ImageScale;
		int YScale = 0;
		int XScale = 0;
		if (ScreenY <= 1000) {
			ImageScale = 0.04f;
			YScale = -5;
			XScale = 0;
		}
		else if (ScreenY >= 1399) {
			ImageScale = 0.06f;
			YScale = 15;
			XScale = 50;
		} else {
			ImageScale = 0.05f;
			YScale = 0;
			XScale = 0;
		}
		float tmpPercentage = 0;
		int tmpPercentage2 = 0;
		std::string tmpCommentary;
		std::string tmpDebugString;
		canvas.SetColor(LinearColor{ 255, 255, 255, 255 }); // White

		if (isMatchEnded == false) { // Render Win %
			//int ScreenWidthX = gameWrapper->GetScreenSize().X;
			
			Vector2 imagePosTeam0 = { (canvas.GetSize().X / 2) - (320 + XScale), 15 }; // Position for the 'dice' image next to the scoreboard
			Vector2 imagePosTeam1 = { (canvas.GetSize().X / 2) + (260 + XScale), 15 };

			Vector2 textPosTeam0 = { (canvas.GetSize().X / 2) - (250 + XScale), 30 }; // Position for the '%' figure next to the scoreboard
			Vector2 textPosTeam1 = { (canvas.GetSize().X / 2) + (200 + XScale), 30 };

			
			canvas.SetPosition(imagePosTeam0);
			canvas.DrawTexture(dice.get(), ImageScale); // Draw the 'dice' image to the screen
			canvas.SetPosition(textPosTeam0);
			if (LocalTeam123 == 1) tmpPercentage = static_cast<float>(GetTeamTotal(1) / static_cast<float>(TotalMMR)); // Check what team you're on and then render the correct % on the correct side of the scoreboard
			if (LocalTeam123 == 2) tmpPercentage = static_cast<float>(GetTeamTotal(2) / static_cast<float>(TotalMMR));
			tmpPercentage2 = static_cast<int>(ceil(tmpPercentage * 100)); // Turn in to a % and round up, this avoids the weirdness of a 49% v 50% prediction...

			canvas.DrawString(std::to_string(tmpPercentage2) + "%", 1.9, 1.9, 0); // Draw the %

			canvas.SetPosition(imagePosTeam1);
			canvas.DrawTexture(dice.get(), ImageScale);
			canvas.SetPosition(textPosTeam1);

			if (LocalTeam123 == 1) tmpPercentage = static_cast<float>(GetTeamTotal(2) / static_cast<float>(TotalMMR));
			if (LocalTeam123 == 2) tmpPercentage = static_cast<float>(GetTeamTotal(1) / static_cast<float>(TotalMMR));
			tmpPercentage2 = static_cast<int>(floor(tmpPercentage * 100)); // Turn in to a % and round down, this avoids the weirdness of a 49% v 50% prediction...

			canvas.DrawString(std::to_string(tmpPercentage2) + "%", 1.9, 1.9, 0);
		}

		if (*bCommentaryEnabled) {

		
		std::string tmpString;
		int tmpCommentatorYPos = 90 + YScale;
		if (isMatchEnded == true) tmpCommentatorYPos = 10;
		Vector2 imagePosCommentator = { (canvas.GetSize().X / 2) - 150 - (Commentary.length() * 4), tmpCommentatorYPos };
		Vector2 textPosCommentator = { (canvas.GetSize().X / 2) - 75 - (Commentary.length() * 4), tmpCommentatorYPos + 15};
		
		if (Commentary != "") {
			canvas.SetColor(CommentaryColour);
			canvas.SetPosition(textPosCommentator);
			canvas.DrawString("\"" + Commentary + "\"", 1.9, 1.9, 1);
			canvas.SetColor(LinearColor{ 255, 255, 255, 255 }); // White
			canvas.SetPosition(imagePosCommentator);
			canvas.DrawTexture(commentator.get(), (ImageScale * 2));
		}

		
		// Debug Text
		Vector2 tmpDebugPosition = { 500, ScreenY - 100 };
		canvas.SetPosition(tmpDebugPosition);
		//tmpDebugString = "Local: " + std::to_string(LocalTeam123) + " T1T " + std::to_string(GetTeamTotal(1)) + " T2T " + std::to_string(GetTeamTotal(2)) + " MMRT " + std::to_string(TotalMMR) + " Score " + std::to_string(TeamScore[1]) + "|" + std::to_string(TeamScore[2]) + " : " + std::to_string(WinningTeam) + "|" + std::to_string(lastGoalScoredBy);
		
		//tmpDebugString = "Lang " + std::to_string(ScreenY);

		//canvas.DrawString("DEBUG: " + tmpDebugString, 1, 1, 1);
		}
	}


}

