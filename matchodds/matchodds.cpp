#include "pch.h"
#include "matchodds.h"
#include "bakkesmod/wrappers/MMRWrapper.h"
#include "String.h"
#include "RankEnums.h"
#include "PlaylistData.h"
#include <cmath>
#include "bakkesmod/wrappers/GameObject/Stats/StatEventWrapper.h"

#define LINMATH_H


BAKKESMOD_PLUGIN(matchodds, "Shows the match favourite + ongoing % chance of winning", "1.0", 0x0)

void matchodds::onLoad()
{
	LoadImgs();
	bEnabled = std::make_shared<bool>(true);
	cvarManager->registerCvar("MatchOdds_Enable", "1", "Enabled MatchOdds", true, true, 0, true, 1).bindTo(bEnabled);
	
	gameWrapper->RegisterDrawable(bind(&matchodds::Render, this, std::placeholders::_1));

	gameWrapper->HookEvent("Function GameEvent_Soccar_TA.Active.StartRound", std::bind(&matchodds::MatchStarted, this, std::placeholders::_1));

	//gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.OnMatchWinnerSet", bind(&matchodds::CalculateStats, this, std::placeholders::_1)); 
	//gameWrapper->HookEvent("Function TAGame.GameEvent_TA.EventPlayerAdded", std::bind(&matchodds::CalculateStats, this, std::placeholders::_1));
	//gameWrapper->HookEvent("Function TAGame.Team_TA.EventScoreUpdated", std::bind(&matchodds::CalculateStats, this, std::placeholders::_1));
	//gameWrapper->HookEvent("Function OnlineGameJoinGame_X.JoiningBase.IsJoiningGame", std::bind(&matchodds::CalculateStats, this, std::placeholders::_1));

	gameWrapper->HookEvent("Function TAGame.GameEvent_TA.EventPlayerAdded", std::bind(&matchodds::CalculateStats, this, std::placeholders::_1));
	gameWrapper->HookEvent("Function TAGame.GameEvent_TA.EventPlayerRemoved", std::bind(&matchodds::CalculateStats, this, std::placeholders::_1));

	gameWrapper->HookEvent("Function GameEvent_TA.Countdown.BeginState", std::bind(&matchodds::CalculateStats, this, std::placeholders::_1));

	gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.OnAllTeamsCreated", std::bind(&matchodds::CalculateStats, this, std::placeholders::_1));
	gameWrapper->HookEvent("Function TAGame.PRI_TA.PostBeginPlay", std::bind(&matchodds::CalculateStats, this, std::placeholders::_1));

	gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.EventMatchEnded", std::bind(&matchodds::MatchEnded, this, std::placeholders::_1));
	gameWrapper->HookEvent("Function TAGame.GFxShell_TA.LeaveMatch", std::bind(&matchodds::MatchUnload, this, std::placeholders::_1));
	gameWrapper->HookEventWithCallerPost<ServerWrapper>(
		"Function TAGame.GFxHUD_TA.HandleStatTickerMessage",
		std::bind(&matchodds::statTickerEvent, this,
			std::placeholders::_1, std::placeholders::_2));

//TAGame.GameEvent_TA:ReplicatedRoundCountDownNumber

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
	cvarManager->log("stat ticker event!");
	if (!gameWrapper->IsInOnlineGame()) {
		cvarManager->log("not in online game");
		return;
	}

	// separates the parts of the stat event args
	auto receiver = PriWrapper(tArgs->Receiver);
	auto victim = PriWrapper(tArgs->Victim);
	auto statEvent = StatEventWrapper(tArgs->StatEvent);
	// name of the stat as shown in rocket league 
	//  (Demolition, Extermination, etc.)
	std::string EventName = statEvent.GetLabel().ToString();

	//if (EventName == "Goal") UpdateTeamTotalExtras(receiver.GetTeamNum() + 1, 200); //Goal Scored, give + 200 MMR
	if (EventName == "Goal") {
		//cvarManager->log("Event: " + EventName + " Team: " + std::to_string(receiver.GetTeamNum()));
		CalculateStats("FromStats");
	}

    


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
	int GoalMMR = 200;
	int TimeMMR1 = 0;
	int TimeMMR2 = 0;
	int GoalDifference = 0;
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
		TimeMMR1 = static_cast<int>((getGameTimeElapsed() * GoalDifference) * 5);
	}
	else if (TeamScore[2] > TeamScore[1]) { 	//Orange Winning
		GoalDifference = TeamScore[2] - TeamScore[1];
		TimeMMR2 = static_cast<int>((getGameTimeElapsed() * GoalDifference) * 5);
	}

	//Goals count for Extra MMR
	TeamTotalExtras[1] = static_cast<int>((TeamScore[1] * GoalMMR) + TimeMMR1);
	TeamTotalExtras[2] = static_cast<int>((TeamScore[2] * GoalMMR) + TimeMMR2);
	//cvarManager->log("TExtras1: " + std::to_string(TeamTotalExtras[1]) + " TExtras2: " + std::to_string(TeamTotalExtras[2]));
}

void matchodds::UpdateTeamTotal(int Team1, int Team2) {
	TeamTotal[1] = Team1 + TeamTotalExtras[1];
	TeamTotal[2] = Team2 + TeamTotalExtras[2];
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

void matchodds::MatchEnded(std::string eventName) {
	isMatchEnded = true;
}
void matchodds::MatchStarted(std::string eventName) {
	isMatchEnded = false;
	TeamTotalExtras[1] = 0;
	TeamTotalExtras[2] = 0;
	initialRand = (rand() % 3 + 1);
}


void matchodds::CalculateStats(std::string eventName)
{
	if (!(*bEnabled)) return;
	if (!gameWrapper->IsInOnlineGame()) return;
	ServerWrapper server = GetCurrentServer();
	
	int tmpTeamTotal[2] = {1,1 };
	tmpHighestMMR = 0;
	tmpHighestMMRName = "";
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
	if (localPlayer.GetTeamNum() == 0) LocalTeam123 = 0;
	if (localPlayer.GetTeamNum() == 1) LocalTeam123 = 1;
	PlayerMMRCaptured = 0;
	long tmpMMR;
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

			// Keep track of highest MMR player (Star Player)
			if (tmpMMR >= tmpHighestMMR)
			{
				tmpHighestMMR = tmpMMR;
				tmpHighestMMRName = playerName;
			}
			
		//}

	}
	GetCurrentScore();
	UpdateTeamTotal(tmpTeamTotal[0], tmpTeamTotal[1]);

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

void matchodds::LoadImgs()
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

	if (gameWrapper->IsInGame() == 1 || gameWrapper->IsInOnlineGame() == 1) {
		if (tmpHighestMMRName == "") return;
		float tmpPercentage = 0;
		int tmpPercentage2;
		std::string tmpCommentry;
		std::string tmpDebugString;

		if (isMatchEnded == false) { // Render Win %
			Vector2 imagePosTeam0 = { (canvas.GetSize().X / 2) - 320, 15 };
			Vector2 imagePosTeam1 = { (canvas.GetSize().X / 2) + 260, 15 };

			Vector2 textPosTeam0 = { (canvas.GetSize().X / 2) - 250, 30 };
			Vector2 textPosTeam1 = { (canvas.GetSize().X / 2) + 200, 30 };

			canvas.SetColor(LinearColor{ 255, 255, 255, 255 });
			canvas.SetPosition(imagePosTeam0);
			canvas.DrawTexture(dice.get(), 0.05f);
			canvas.SetPosition(textPosTeam0);
			if (LocalTeam123 == 0) tmpPercentage = static_cast<float>(GetTeamTotal(1) / static_cast<float>(TotalMMR));
			if (LocalTeam123 == 1) tmpPercentage = static_cast<float>(GetTeamTotal(2) / static_cast<float>(TotalMMR));
			tmpPercentage2 = tmpPercentage * 100;

			canvas.DrawString(std::to_string(tmpPercentage2) + "%", 1.9, 1.9, 0);

			canvas.SetColor(LinearColor{ 255, 255, 255, 255 });
			canvas.SetPosition(imagePosTeam1);
			canvas.DrawTexture(dice.get(), 0.05f);
			canvas.SetPosition(textPosTeam1);

			if (LocalTeam123 == 0) tmpPercentage = static_cast<float>(GetTeamTotal(2) / static_cast<float>(TotalMMR));
			if (LocalTeam123 == 1) tmpPercentage = static_cast<float>(GetTeamTotal(1) / static_cast<float>(TotalMMR));
			tmpPercentage2 = tmpPercentage * 100;

			canvas.DrawString(std::to_string(tmpPercentage2) + "%", 1.9, 1.9, 0);
		}

		//TODO: Move commentary to seperate function, doesn't need to run this code every frame...
		std::string tmpString;
		LinearColor tmpColour{ 255, 255, 255, 255 }; // White
		if (PlayerMMRCaptured == PlayerCount) { // We have MMR for all players

			if (LocalTeam123 == 0) {
				if (GetTeamTotal(1) > GetTeamTotal(2)) {
					tmpColour = { 150,200,150,255 }; // Green?
					tmpString = "You are the favourites!";
				}
				else {
					tmpColour = { 200,150,150,255 }; // Orange?
					tmpString = "You are the underdogs... ";
				}
			}
			else {
				if (GetTeamTotal(2) > GetTeamTotal(1)) {
					tmpColour = { 150,200,150,255 }; // Green?
					tmpString = "You are the favourites!";
				}
				else {
					tmpColour = { 200,150,150,255 }; // Orange?
					tmpString = "You are the underdogs...";
				}
			}
			if (getGameTime() > 240) {
				switch (1) {
					case 1: tmpCommentry = tmpString + " Star Player: " + tmpHighestMMRName; break;
					//case 2: tmpCommentry = tmpString + " One to watch: " + tmpHighestMMRName; break;
					//case 3: tmpCommentry = tmpString + " Top Rated: " + tmpHighestMMRName; break;
				}
			}
			else if (getGameTime() > 120 && getGameTime() < 241) {
				tmpCommentry = tmpString;
			} else {
				tmpCommentry = tmpString;
			}
			
		}
		else {
			tmpCommentry = "Calculating predictions...";
		}
		if (isMatchEnded == true) tmpCommentry = "...";

		int tmpCommentatorYPos = 90;
		if (isMatchEnded == true) tmpCommentatorYPos = 10;
		Vector2 imagePosCommentator = { (canvas.GetSize().X / 2) - 150 - (tmpCommentry.length() * 4), tmpCommentatorYPos };
		Vector2 textPosCommentator = { (canvas.GetSize().X / 2) - 75 - (tmpCommentry.length() * 4), tmpCommentatorYPos + 15 };
		
		
		canvas.SetColor(tmpColour);
		canvas.SetPosition(textPosCommentator);
		canvas.DrawString("\"" + tmpCommentry + "\"", 1.9, 1.9, 1);
		tmpColour = { 255, 255, 255, 255 };
		canvas.SetColor(tmpColour);
		canvas.SetPosition(imagePosCommentator);
		canvas.DrawTexture(commentator.get(), 0.10f);

		// Debug Text
		//Vector2 tmpDebugPosition = { 500,1060 };
		//canvas.SetPosition(tmpDebugPosition);
		//tmpDebugString = "Local: " + std::to_string(LocalTeam123) + " T0T " + std::to_string(GetTeamTotal(1)) + " T1T " + std::to_string(GetTeamTotal(2)) + " MMRT " + std::to_string(TotalMMR) + " Score " + std::to_string(TeamScore[1]) + "|" + std::to_string(TeamScore[2]) + " : " + std::to_string(PlayerCount) + "|" + std::to_string(PlayerMMRCaptured);
		//canvas.DrawString("DEBUG: " + tmpDebugString, 1, 1, 1);

	}


}

