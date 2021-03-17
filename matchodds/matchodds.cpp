#include "pch.h"
#include "matchodds.h"
#include "bakkesmod/wrappers/MMRWrapper.h"
#include "String.h"
#include <cmath>
#include "bakkesmod/wrappers/GameObject/Stats/StatEventWrapper.h"

#define LINMATH_H
long tmpCount = 0;

BAKKESMOD_PLUGIN(matchodds, "Shows win percentages + commentary throughout the game.", "1.7", 0x0);
// ============================================================================
// MatchOdds
// by Civilian360
// Discord: Civilian360#9819
//
// TODO:
// Taking the team size in to account with the odds calculations
//
// ============================================================================
void matchodds::onLoad()
{
	LoadImgs();
	bEnabled = std::make_shared<bool>(true);
	cvarManager->registerCvar("matchodds_enabled", "1", "Enable MatchOdds", true, true, 0, true, 1).bindTo(bEnabled);
	bCommentaryEnabled = std::make_shared<bool>(true);
	cvarManager->registerCvar("matchodds_commentaryenabled", "1", "Enable Commentary", true, true, 0, true, 1).bindTo(bCommentaryEnabled);
	bPercentagesEnabled = std::make_shared<bool>(true);
	cvarManager->registerCvar("matchodds_percentagesenabled", "1", "Enable Percentages", true, true, 0, true, 1).bindTo(bPercentagesEnabled);

	cl_commentator_override = std::make_shared<bool>(false);
	cvarManager->registerCvar("cl_commentator_override", "0", "Override Commentatory Position?", true, true, 0, true, 1).bindTo(cl_commentator_override);
	cl_commentator_x = std::make_shared<int>(500);
	cvarManager->registerCvar("cl_commentator_x", "500", "Commentator X", true, true, 0, true, 3000).bindTo(cl_commentator_x);
	cl_commentator_y = std::make_shared<int>(100);
	cvarManager->registerCvar("cl_commentator_y", "100", "Commentator Y", true, true, 0, true, 2000).bindTo(cl_commentator_y);
	cl_commentator_textscale = std::make_shared<int>(190);
	cvarManager->registerCvar("cl_commentator_textscale", "190", "Text Scale", true, true, 0, true, 200).bindTo(cl_commentator_textscale);
	cl_commentator_imagescale = std::make_shared<int>(50);
	cvarManager->registerCvar("cl_commentator_imagescale", "50", "Image Scale", true, true, 0, true, 100).bindTo(cl_commentator_imagescale);
	cl_dice_imagescale = std::make_shared<int>(50);
	cvarManager->registerCvar("cl_dice_imagescale", "50", "Image Scale", true, true, 0, true, 100).bindTo(cl_dice_imagescale);

	cl_commentarytype = std::make_shared<std::string>("default");
	cvarManager->registerCvar("cl_commentarytype", "default", "Toxic Commentary?", false, false, 0, false, 0, true).bindTo(cl_commentarytype);


	bOnlyOnScoreboard = std::make_shared<bool>(false);
	cvarManager->registerCvar("matchodds_onlyonscoreboard", "0", "Display Only On Scoreboard?", true, true, 0, true, 1).bindTo(bOnlyOnScoreboard);

	gameWrapper->RegisterDrawable(bind(&matchodds::Render, this, std::placeholders::_1));

	gameWrapper->HookEvent("Function Engine.PlayerController.ReceivedGameClass", std::bind(&matchodds::MatchStarted, this, std::placeholders::_1)); // Fires at the start of an online game

	gameWrapper->HookEvent("Function GameEvent_TA.Countdown.BeginState", std::bind(&matchodds::CountdownStarted, this, std::placeholders::_1));  // Start of Countdown
	gameWrapper->HookEvent("Function GameEvent_Soccar_TA.Countdown.EndState", std::bind(&matchodds::CountdownEnded, this, std::placeholders::_1));   // 3 Seconds later

	gameWrapper->HookEvent("Function TAGame.GameEvent_TA.EventPlayerAdded", std::bind(&matchodds::CalculateMMR, this, std::placeholders::_1)); // Triggers
	gameWrapper->HookEvent("Function TAGame.GameEvent_TA.EventPlayerRemoved", std::bind(&matchodds::CalculateMMR, this, std::placeholders::_1)); // Triggers
	gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.OnAllTeamsCreated", std::bind(&matchodds::CalculateMMR, this, std::placeholders::_1)); // vs. Bots Triggered before selecting a team to join (very start of the game!)
	//gameWrapper->HookEvent("Function TAGame.PRI_TA.PostBeginPlay", std::bind(&matchodds::CalculateMMR, this, std::placeholders::_1)); // Triggered when somone joins?

	gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.EventMatchEnded", std::bind(&matchodds::MatchEnded, this, std::placeholders::_1)); // Triggers at the end of the match 10s before 'Function TAGame.GameEvent_Soccar_TA.BeginHighlightsReplay'
	gameWrapper->HookEvent("Function TAGame.GFxShell_TA.LeaveMatch", std::bind(&matchodds::MatchEnded, this, std::placeholders::_1));

	gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.BeginHighlightsReplay", std::bind(&matchodds::EndGameHighlights, this, std::placeholders::_1)); // Very end of game whilst replay is showing behind the scoreboard

	gameWrapper->HookEvent("Function TAGame.Car_TA.EnablePodiumMode", std::bind(&matchodds::PodiumMode, this, std::placeholders::_1)); // End of game before the scoreboard
	gameWrapper->HookEvent("Function TAGame.GFxHUD_TA.OpenScoreboard", std::bind(&matchodds::OpenScoreboard, this, std::placeholders::_1)); // In Game when user views Scoreboard
	gameWrapper->HookEvent("Function TAGame.GFxHUD_TA.CloseScoreboard", std::bind(&matchodds::CloseScoreboard, this, std::placeholders::_1)); // In Game when user stops viewing Scoreboard

	gameWrapper->HookEventWithCallerPost<ServerWrapper>(
		"Function TAGame.GFxHUD_TA.HandleStatTickerMessage",
		std::bind(&matchodds::statTickerEvent, this,
			std::placeholders::_1, std::placeholders::_2));

	gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.EventOvertimeUpdated", std::bind(&matchodds::itsOvertime, this, std::placeholders::_1));

	gameWrapper->HookEvent("Function TAGame.CrowdSoundManager_TA.Tick", std::bind(&matchodds::GameUpdated, this, std::placeholders::_1));

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
	// name of the stat as shown in rocket league  (Demolition, Extermination, etc.)
	std::string EventName = statEvent.GetLabel().ToString();

	if (EventName == "Goal") {
		tmpCounter = 0;
		lastGoalScoredBy = receiver.GetTeamNum() + 1;
		GetCurrentScore("Goal", lastGoalScoredBy);
		CalculateMMR("Goal");
		(*cl_commentarytype == "default") ? GetCommentary("Goal") : GetToxicCommentary("Goal");

	} else if (EventName == "Assist") { //This occurs after the Goal event fires so it would override the commentry... hence we block it.
		return;
	} else { // TODO: Find a less hacky way of updating the stats during the game... (i.e. on a timer every X seconds?)
		tmpCounter = 0;
		//GetCurrentScore();
		UpdateTeamTotal();
		(*cl_commentarytype == "default") ? GetCommentary() : GetToxicCommentary();
	}

    


}

void::matchodds::GetCommentary(std::string eventName) {

	if (!(*bEnabled)) return;
	if (!gameWrapper->IsInOnlineGame()) return;
	if (getGameTime() >= getMaxGameTime())  return; // Very start of game, things are weird here...
	srand(getGameTime());
	rndNumber = (rand() % 2 + 1);
	bool isFavourite = false;
	Commentary = "";
	if (PlayerMMRCaptured == PlayerCount) { // Do we have MMR for all players?  (This of course fails when in competative and 1 person leaves...)
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
		if (LocalTeam123 == 1) {
			if (TeamBaselineMMR[1] > TeamBaselineMMR[2]) {
				isPredictedFavourite = true;
			}
			else {
				isPredictedFavourite = false;
			}
		}
		else {
			if (TeamBaselineMMR[2] > TeamBaselineMMR[1]) {
				isPredictedFavourite = true;
			}
			else {
				isPredictedFavourite = false;
			}
		}
		if (getGameTime() > (getMaxGameTime() - 20)) { // Very Start of the game so store the predicted favourites
			if (isFavourite == true) {
				if (rndNumber == 1) Commentary = "You are the favorites!  Star Player: " + StarPlayerName;
				if (rndNumber == 2) Commentary = "You're the favorites!  Star Player: " + StarPlayerName;
			}
			else if (isFavourite == false) {
				if (rndNumber == 1) Commentary = "You are the underdogs.  Star Player: " + StarPlayerName;
				if (rndNumber == 2) Commentary = "You're the underdogs.  Star Player: " + StarPlayerName;
			}
		}
		else if (getGameTime() > 260 && getGameTime() < 281) { // Start of game (if it's the standard 5 min match)
			Commentary = "...";
			if (isFavourite && isPredictedFavourite && GoalDifference > 0) Commentary = "One to watch: " + StarPlayerName;
			if (!isFavourite && isPredictedFavourite && GoalDifference > 0) Commentary = "Don't let it slip away...  One to watch: " + StarPlayerName;
			if (isFavourite && !isPredictedFavourite && GoalDifference > 0) Commentary = "You're doing great!!  One to watch: " + StarPlayerName;
			if (!isFavourite && !isPredictedFavourite && GoalDifference > 0) Commentary = "Push a little harder.  One to watch: " + StarPlayerName;

		}
		else if (getGameTime() > 120 && getGameTime() < 261) { // Mid Game
			if (isFavourite == true) Commentary = "...";
			if (isFavourite == false) Commentary = "...";
			
		}
		else if (getGameTime() > 60 && getGameTime() < 121) {
			if (isFavourite && isPredictedFavourite) Commentary = "You are still the favorites!  Current MVP: " + MVPPlayerName;
			if (!isFavourite && isPredictedFavourite) Commentary = "Don't let it slip away...  Current MVP: " + MVPPlayerName;
			if (isFavourite && !isPredictedFavourite) Commentary = "You're doing great!!  Current MVP: " + MVPPlayerName;
			if (!isFavourite && !isPredictedFavourite) Commentary = "Push a little harder.  Current MVP: " + MVPPlayerName;

		}
		else if (getGameTime() > 1 && getGameTime() < 61) { // Final minute
			if (isFavourite && isPredictedFavourite) Commentary = "You've got this, You're still the favorites!";
			if (!isFavourite && isPredictedFavourite) Commentary = "Nooo, what's going wrong??";
			if (isFavourite && !isPredictedFavourite) Commentary = "Amazing work! You were the underdogs!";
			if (!isFavourite && !isPredictedFavourite) Commentary = "...";

		}
		//if (MatchState == s_MatchEndPodium) { // Game over, Podium Screen
		//	if (isPredictedFavourite && WinningTeam == LocalTeam123 && GoalDifference > 0) {
		//		if (rndNumber == 1) Commentary = "I predicted a win for you and you delivered! Good job!";
		//		if (rndNumber == 2) Commentary = "I predicted a win for you and you delivered! Well done!";
		//	} else if (isPredictedFavourite && WinningTeam != LocalTeam123 && GoalDifference > 0) { // Losing/lost
		//		if (rndNumber == 1) Commentary = "What happened?!? I had you down as favorites!";
		//		if (rndNumber == 2) Commentary = "I had you down as favorites! Where did it all go wrong?";
		//	} else if (isPredictedFavourite == false && WinningTeam == LocalTeam123 && GoalDifference > 0) {
		//		if (rndNumber == 1) Commentary = "Wow! Well done! You defied the odds and won that one!";
		//		if (rndNumber == 2) Commentary = "Great work! You defied the odds and won that one!";
		//	} else if (isPredictedFavourite == false && WinningTeam != LocalTeam123 && GoalDifference > 0) {
		//		//Commentary = "Well.. it's the taking part that counts...";
		//	}
		//}


		if (isFavourite == true) CommentaryColour = { 150,200,150,255 }; // Green
		if (isFavourite == false) CommentaryColour = CommentaryColour = { 255,150,150,255 }; // Red?
		CommentatorColour = { 255,255,255,255 }; // White
		if (eventName == "Goal") {
			if (lastGoalScoredBy == LocalTeam123) { // Your team scored
				//Commentary = "Goal, Your team scored, Goal Difference: " + std::to_string(GoalDifference) + " Score: " + std::to_string(TeamScore[1]) + "|" + std::to_string(TeamScore[2]);
				if (WinningTeam == LocalTeam123) { // You're winning
					if (GoalDifference < 3) {
						if (rndNumber == 1) Commentary = "Goooaaaalllll!!";
						if (rndNumber == 2) Commentary = "Gooall!!";
					} else if (GoalDifference > 3) { // Winning by a lot!
						if (rndNumber == 1) Commentary = "You're making it look easy!";
						if (rndNumber == 2) Commentary = "Great job, it's looking easy!";
					} 
				}
				else { // You're losing
					if (GoalDifference > 3) {
						if (rndNumber == 1) Commentary = "You've pulled one back.. Keep going!";
						if (rndNumber == 2) Commentary = "Chin up! You can do this!";
					}
					
				}
			}
			else { // Oppenent scored
				//Commentary = "Goal, They scored, Goal Difference: " + std::to_string(GoalDifference) + " Score: " + std::to_string(TeamScore[1]) + "|" + std::to_string(TeamScore[2]);
				if (WinningTeam == LocalTeam123) {  // You're still winning though!
					if (GoalDifference < 1) {
						if (rndNumber == 1) Commentary = "Focus!";
						if (rndNumber == 2) Commentary = "Don't let them get back in this game!";
					}
					else if (GoalDifference > 3) {
						if (rndNumber == 1) Commentary = "No problem...";
						if (rndNumber == 2) Commentary = "It's ok";
					}
				}
				else { // You're losing...
					if (GoalDifference > 1) { 
						if (rndNumber == 1) Commentary = "Don't be disheartened!";
						if (rndNumber == 2) Commentary = "Nooo!";
					}
					if (GoalDifference > 3) { // By a lot!
						if (rndNumber == 1) Commentary = "Well.. it's the taking part that counts";
						if (rndNumber == 2) Commentary = "FF ? xD";
					}
				}
			}
			
		}
		//cvarManager->log(Commentary);
		//if (eventName == "GameUpdated") Commentary = "Debug: getGameTime=" + std::to_string(tmpCount++);


		if (isOvertime) { //TODO: Add # of shots for each team , super handy to see during Overtime in a tournament!
			if (isFavourite == true) Commentary = "You're the favorites, finish them off!";
			if (isFavourite == false) Commentary = "Go for the upset! Cmon you underdogs!!";
		}

	}
	else {
		Commentary = "";
		// Mismatch in captured MMR vs. Playercount so try to get MMR again
		//CalculateMMR("");
	}
	//if (MatchState == s_MatchEndReplay) Commentary = "Wasn't that fun? ^.^"; // TODO: Actually identify when user is on the post match screen
}


void::matchodds::GetToxicCommentary(std::string eventName) {

	if (!(*bEnabled)) return;
	if (!gameWrapper->IsInOnlineGame()) return;
	if (getGameTime() >= getMaxGameTime())  return; // Very start of game, things are weird here...
	srand(getGameTime());
	rndNumber = (rand() % 2 + 1);
	bool isFavourite = false;
	Commentary = "";
	if (PlayerMMRCaptured == PlayerCount) { // Do we have MMR for all players?  (This of course fails when in competative and 1 person leaves...)
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
		if (LocalTeam123 == 1) {
			if (TeamBaselineMMR[1] > TeamBaselineMMR[2]) {
				isPredictedFavourite = true;
			}
			else {
				isPredictedFavourite = false;
			}
		}
		else {
			if (TeamBaselineMMR[2] > TeamBaselineMMR[1]) {
				isPredictedFavourite = true;
			}
			else {
				isPredictedFavourite = false;
			}
		}
		if (getGameTime() > (getMaxGameTime() - 20)) { // Very Start of the game so store the predicted favourites
			if (isPredictedFavourite == true) {
				if (rndNumber == 1) Commentary = "You're favorites, Don't f*ck this up you idiot...";
				if (rndNumber == 2) Commentary = "You're favorites but you'll find a way to screw this up...";
			}
			else if (isPredictedFavourite == false) {
				if (rndNumber == 1) Commentary = "You're underdogs, You look like loser to me, FF now?";
				if (rndNumber == 2) Commentary = "You're underdogs, I mean.. just look at you.. loser...";
			}
		}
		else if (getGameTime() > 260 && getGameTime() < 281) { // Start of game (if it's the standard 5 min match)
			Commentary = "...";
			if (isFavourite && isPredictedFavourite && GoalDifference > 0) Commentary = "I'm surprised...  One to watch: " + StarPlayerName;
			if (!isFavourite && isPredictedFavourite && GoalDifference > 0) Commentary = "Well you've f*cked this up haven't you?";
			if (isFavourite && !isPredictedFavourite && GoalDifference > 0) Commentary = "Maybe you can win back your mothers love by winning at RL?";
			if (!isFavourite && !isPredictedFavourite && GoalDifference > 0) Commentary = "You should have been aborted...";

		}
		else if (getGameTime() > 120 && getGameTime() < 261) { // Mid Game
			if (isFavourite == true) Commentary = "...";
			if (isFavourite == false) Commentary = "...";

		}
		else if (getGameTime() > 60 && getGameTime() < 121) {
			Commentary = "...";
			if (isFavourite && isPredictedFavourite) Commentary = "Somehow you're still favorites, not to your parents though...";
			if (!isFavourite && isPredictedFavourite) Commentary = "Current MVP: " + MVPPlayerName;
			if (isFavourite && !isPredictedFavourite) Commentary = "You're going to choke!  Current MVP: " + MVPPlayerName;
			if (!isFavourite && !isPredictedFavourite) Commentary = "Loser!";

		}
		else if (getGameTime() > 1 && getGameTime() < 61) { // Final minute
			Commentary = "...";
			if (isFavourite && isPredictedFavourite) Commentary = "Choke choke choke choke!";
			if (!isFavourite && isPredictedFavourite) Commentary = "What a dissapointment you've turned out to be";
			if (isFavourite && !isPredictedFavourite) Commentary = "You'll f*ck this up!";
			if (!isFavourite && !isPredictedFavourite) Commentary = "...";

		}
		//if (MatchState == s_MatchEndPodium) { // Game over, Podium Screen
		//	if (isPredictedFavourite && WinningTeam == LocalTeam123 && GoalDifference > 0) {
		//		if (rndNumber == 1) Commentary = "I predicted you to win, somehow you managed it...";
		//		if (rndNumber == 2) Commentary = "I predicted you to win, I still don't understand how?!?";
		//	}
		//	else if (isPredictedFavourite && WinningTeam != LocalTeam123 && GoalDifference > 0) { // Losing/lost
		//		if (rndNumber == 1) Commentary = "You bombed hard!";
		//		if (rndNumber == 2) Commentary = "GG EZ";
		//	}
		//	else if (isPredictedFavourite == false && WinningTeam == LocalTeam123 && GoalDifference > 0) {
		//		if (rndNumber == 1) Commentary = "Wow! The one thing you're good at, hitting a ball with a car... *slow claps*";
		//		if (rndNumber == 2) Commentary = "I feel the other team took pity on you...";
		//	}
		//	else if (isPredictedFavourite == false && WinningTeam != LocalTeam123 && GoalDifference > 0) {
		//		//Commentary = "Well.. it's the taking part that counts...";
		//	}
		//	/*else if (GoalDifference == 0) {
		//		Commentary = "Here comes the overtime...";
		//	}*/

		//}


		if (isFavourite == true) CommentaryColour = { 150,200,150,255 }; // Green
		if (isFavourite == false) CommentaryColour = CommentaryColour = { 255,150,150,255 }; // Red?
		CommentatorColour = { 255,100,100,255 }; // Red?

		if (eventName == "Goal") {
			if (lastGoalScoredBy == LocalTeam123) { // Your team scored
				//Commentary = "Goal, Your team scored, Goal Difference: " + std::to_string(GoalDifference) + " Score: " + std::to_string(TeamScore[1]) + "|" + std::to_string(TeamScore[2]);
				if (WinningTeam == LocalTeam123) { // You're winning
					if (GoalDifference < 3) {
						if (rndNumber == 1) Commentary = "Luck...";
						if (rndNumber == 2) Commentary = "Meh...";
					}
					else if (GoalDifference > 3) { // Winning by a lot!
						if (rndNumber == 1) Commentary = "You'll find a way to ruin this lead";
						if (rndNumber == 2) Commentary = "Don't get too comfy, we know you'll throw it away";
					}
				}
				else { // You're losing
					if (GoalDifference > 3) {
						if (rndNumber == 1) Commentary = "You'll never catch them...";
						if (rndNumber == 2) Commentary = "Why are you even trying?";
					}

				}
			}
			else { // Oppenent scored
				//Commentary = "Goal, They scored, Goal Difference: " + std::to_string(GoalDifference) + " Score: " + std::to_string(TeamScore[1]) + "|" + std::to_string(TeamScore[2]);
				if (WinningTeam == LocalTeam123) {  // You're still winning though!
					if (GoalDifference < 1) {
						if (rndNumber == 1) Commentary = "Loser!";
						if (rndNumber == 2) Commentary = "You're throwing it all away";
					}
					else if (GoalDifference > 3) {
						if (rndNumber == 1) Commentary = "Don't get too comfy...";
						if (rndNumber == 2) Commentary = "Cmon.. stop sucking";
					}
				}
				else { // You're losing...
					if (GoalDifference > 3) { // By a lot!
						if (rndNumber == 1) Commentary = "I'm embarressed for you";
						if (rndNumber == 2) Commentary = "Go ahead, FF, turn off the computer and think about your life choices";
					}
				}
			}

		}
		//cvarManager->log(Commentary);
		//if (eventName == "GameUpdated") Commentary = "Debug: getGameTime=" + std::to_string(tmpCount++);


		if (isOvertime) { //TODO: Add # of shots for each team , super handy to see during Overtime in a tournament!
			if (isFavourite == true) Commentary = "The pressure will break you";
			if (isFavourite == false) Commentary = "Why bother trying? we both know you'll lose.";
		}
	}
	else {
		Commentary = "";
		// Mismatch in captured MMR vs. Playercount so try to get MMR again
		//CalculateMMR("");
	}
	//if (MatchState == s_MatchEndReplay) Commentary = "Wasn't that fun? ^.^"; // TODO: Actually identify when user is on the post match screen
}

void matchodds::UpdateTeamTotalExtras(int TeamNumber, int Amount) {
	TeamTotalExtras[TeamNumber] += Amount;
}
void matchodds::onUnload() 
{
	gameWrapper->UnhookEvent("Function TAGame.GameEvent_Soccar_TA.EventMatchEnded");
	gameWrapper->UnregisterDrawables();
}

void matchodds::GetCurrentScore(std::string eventName, int TeamNum) {
	if (!(*bEnabled)) return;
	if (!gameWrapper->IsInOnlineGame()) return;

	//TODO: GoalMMR should be dynamic based on Team Size "a 3 goal advantage is more significant in 3v3 vs 1v1" thanks DrStein#6280 for the suggestion
	int GoalMMR = 150; // How  much to add on for a goal
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


	MatchGUID = onlineServer.GetMatchGUID();

	if (eventName == "Goal") { // Goal scored, got event from Stat tracker which is updated before the server for some reason?!? #whatisthisbullshit?
		TeamScore[TeamNum]++;
		//cvarManager->log("Manually Added A Goal: Score1: " + std::to_string(TeamScore[1]) + " Score2: " + std::to_string(TeamScore[2]));
	}
	if (TeamScore[1] == TeamScore[2]) { //Draw, do nothing...
		
	}
	else if (TeamScore[1] > TeamScore[2]) { //Blue Winning
		GoalDifference = TeamScore[1] - TeamScore[2];
		WinningTeam = 1;
		TimeMMR1 = static_cast<int>((getGameTimeElapsed() * GoalDifference) * 10);
	}
	else if (TeamScore[2] > TeamScore[1]) { 	//Orange Winning
		WinningTeam = 2;
		GoalDifference = TeamScore[2] - TeamScore[1];
		TimeMMR2 = static_cast<int>((getGameTimeElapsed() * GoalDifference) * 10);
	}

	//Goals count for Extra MMR
	TeamTotalExtras[1] = static_cast<int>((TeamScore[1] * GoalMMR) + TimeMMR1);
	TeamTotalExtras[2] = static_cast<int>((TeamScore[2] * GoalMMR) + TimeMMR2);
	//cvarManager->log("TExtras1: " + std::to_string(TeamTotalExtras[1]) + " TExtras2: " + std::to_string(TeamTotalExtras[2]));
	//cvarManager->log("Score1: " + std::to_string(TeamScore[1]) + " Score2: " + std::to_string(TeamScore[2]));
}

void matchodds::UpdateTeamTotal() {
	TeamTotal[1] = TeamBaselineMMR[1] + TeamTotalExtras[1];
	TeamTotal[2] = TeamBaselineMMR[2] + TeamTotalExtras[2];
	UpdateTotalMMR(TeamTotal[1], TeamTotal[2]);
	//cvarManager->log("UpdateTeamTotal:  " + std::to_string(Team1) + " | " + std::to_string(Team2));
	//cvarManager->log("UpdateTeamTotal:  " + std::to_string(TeamTotal[0]) + " | " + std::to_string(TeamTotal[1]));
}

int matchodds::GetTeamTotal(int TeamNumber) {
	 return TeamTotal[TeamNumber];
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
	MatchState = s_MatchEnd;
	isMatchEnded = true;
	isOvertime = false;
	capturedPredictedFavourite = false;
	ClearStats();
}

void matchodds::MatchEnded(std::string eventName) { // Triggered at the very end of the game
	 MatchState = s_MatchEnd;
	//cvarManager->log("Match Ended " + eventName);
	isMatchEnded = true;
	isOvertime = false;
	capturedPredictedFavourite = false;
	ClearStats();


}
void matchodds::ClearStats() {
	GoalDifference = 0;
	WinningTeam = 0;
	//Commentary = "";
	//capturedPredictedFavourite = false;
	isPredictedFavourite = false;
	lastGoalScoredBy = 0;
	TeamTotal[3] = {};
	TeamTotalExtras[3] = {};
	TeamScore[3] = {};
	TeamBaselineMMR[3] = {};
	TeamTotal[0] = 1;
	TeamTotal[1] = 1;
	TeamTotal[2] = 1;
	TeamBaselineMMR[0] = 1;
	TeamBaselineMMR[1] = 1;
	TeamBaselineMMR[2] = 1;
	//TeamTotalExtras[0] = 1;
	//TeamTotalExtras[1] = 1;
	//TeamTotalExtras[2] = 1;
	TotalMMR = 0;
	PlayerCount = 0;
	PlayerMMRCaptured = 0;
	StarPlayerName = "";
	isScoreboardOpen = false;
}
void matchodds::RoundEnded(std::string eventName) { // Triggered after a goal is scored?
	MatchStates MatchState = s_InMatch;
	
	//cvarManager->log("Round Ended: " + eventName);
}
void matchodds::MatchStarted(std::string eventName) { // Triggered at the very start?
	if (gameWrapper->IsInReplay()) return;
	isMatchEnded = false;
	 MatchState = s_PreMatch;
	 ClearStats();
	//cvarManager->log("Match Started " + eventName);
	if (!gameWrapper->IsInOnlineGame()) {
		GetCurrentScore();
		UpdateTeamTotal();
		(*cl_commentarytype == "default") ? GetCommentary("GameUpdated") : GetToxicCommentary("GameUpdated");
		return;
	}
	
}
void matchodds::GameUpdated(std::string eventName) { // fires every 'tick'
	if (!gameWrapper->IsInOnlineGame()) return;
	tmpCounter++;
	if (tmpCounter == 1000) { // Update % ever 1s
		GetCurrentScore();
		UpdateTeamTotal();
		if (Commentary == "") {
			(*cl_commentarytype == "default") ? GetCommentary() : GetToxicCommentary();
		}
		tmpCounter = 0;
	}
}

void matchodds::itsOvertime(std::string eventName) { // Triggered when Overtime is started
	 MatchState = s_Overtime;
	 isOvertime = true;
	 CalculateMVP();
	 (*cl_commentarytype == "default") ? GetCommentary() : GetToxicCommentary();
}
void matchodds::RoundStarted(std::string eventName) { 
	cvarManager->log("Round Started " + eventName);
	 MatchState = s_InMatch;
	isMatchEnded = false;
	//ClearStats();
	//CalculateMMR("");
	UpdateTeamTotal();
	GetCurrentScore();
	//CalculateMVP();
	(*cl_commentarytype == "default") ? GetCommentary() : GetToxicCommentary();

}void matchodds::BeginState(std::string eventName) {
	cvarManager->log("BeginState " + eventName);
	 MatchState = s_InMatch;
	isMatchEnded = false;
	//ClearStats();
	//CalculateMMR("");
	UpdateTeamTotal();
	GetCurrentScore();
	CalculateMVP();
	(*cl_commentarytype == "default") ? GetCommentary() : GetToxicCommentary();
}
void matchodds::CountdownStarted(std::string eventName) {
	cvarManager->log("CountdownStarted " + eventName);
	MatchState = s_InMatch;
	isMatchEnded = false;
	//ClearStats();
	CalculateMMR("");
	UpdateTeamTotal();
	GetCurrentScore();
	CalculateMVP();
	(*cl_commentarytype == "default") ? GetCommentary() : GetToxicCommentary();
}void matchodds::CountdownEnded(std::string eventName) {
	cvarManager->log("CountdownEnded " + eventName);
	MatchState = s_InMatch;
	isMatchEnded = false;
	//ClearStats();
	CalculateMMR("");
	UpdateTeamTotal();
	GetCurrentScore();
	CalculateMVP();
	(*cl_commentarytype == "default") ? GetCommentary() : GetToxicCommentary();
}
void matchodds::EndGameHighlights(std::string eventName) {
	 MatchState = s_MatchEndReplay;
	 ClearStats();
	 //(*cl_commentarytype == "default") ? GetCommentary() : GetToxicCommentary();
	 GoalDifference = 0;
	 WinningTeam = 0;
}
void matchodds::PodiumMode(std::string eventName) {
	MatchState = s_MatchEndPodium;
	(*cl_commentarytype == "default") ? GetCommentary() : GetToxicCommentary();
}




void matchodds::CalculateMMR(std::string eventName)
{
	if (!(*bEnabled)) return;
	if (!gameWrapper->IsInOnlineGame()) return;
	if (gameWrapper->IsInReplay()) return;
	

		ServerWrapper server = GetCurrentServer();
		//cvarManager->log("Online Game");
		int tmpTeamTotal[2] = { 1,1 };
		tmpHighestMMR = 0;
		StarPlayerName = "";
		//ClearStats();
		tmpTeamTotal[0] = 0;
		tmpTeamTotal[1] = 0;
		//TeamBaselineMMR[1] = 0;
		//TeamBaselineMMR[2] = 0;
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
			if (tmpMMR == 600) return;
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
				tmpHighestMMR = tmpMMR; // # of MMR the player has
				StarPlayerName = playerName; // The name of the player
			}


		}
		TeamBaselineMMR[1] = tmpTeamTotal[0];
		TeamBaselineMMR[2] = tmpTeamTotal[1];
		//GetCurrentScore();
		if (eventName != "") UpdateTeamTotal();
		//UpdateTeamTotal();

		//cvarManager->log(tmpMMRDebugText1 + "  " + tmpMMRDebugText2);
	
}
void matchodds::CalculateMVP()
{
	if (!(*bEnabled)) return;
	if (!gameWrapper->IsInOnlineGame()) return;
	
		ServerWrapper server = GetCurrentServer();
		long tmpHighestMVP = 0;

		ArrayWrapper<PriWrapper> pris = server.GetPRIs();

		int len = pris.Count();
		if (len < 1) return;
		
		int tmpMVP;
		tmpMVP = 0;
		
		std::string playerName;
		for (int i = 0; i < len; i++)
		{
			PriWrapper player = pris.Get(i);
			playerName = player.GetPlayerName().ToString();

			// Keep track of highest Score (MVP)
			tmpMVP = player.GetMatchScore();
			if (tmpMVP >= tmpHighestMVP)
			{
				tmpHighestMVP = tmpMVP; // # of MMR the player has
				MVPPlayerName = playerName; // The name of the player
			}

		}
		
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
	//percentage = std::make_shared<ImageWrapper>(tmpDataDir + "\\matchodds\\percentage.png", true);
	commentator = std::make_shared<ImageWrapper>(tmpDataDir + "\\matchodds\\commentator.png", true);

	star->LoadForCanvas();
	dice->LoadForCanvas();
	//percentage->LoadForCanvas();
	commentator->LoadForCanvas();

}

void matchodds::Render(CanvasWrapper canvas)
{
	if (!(*bEnabled)) return; // Don't display if the plugin is disabled...
	if (gameWrapper->IsInOnlineGame() || gameWrapper->IsInReplay()) { //Display if user is in an online game (Casual, Ranked, Tournament)
		if (TeamBaselineMMR[1] == 1) return;
		if (TeamBaselineMMR[2] == 1) return;
		//if (StarPlayerName == "") return; //Only render if we have some MMR data
		//if (getGameTime() >= getMaxGameTime())  return; // Hotfix: Don't display inaccurate data at the very start of games (if you play multiple games in a row things get messed up...)
		int ScreenY = gameWrapper->GetScreenSize().Y;
		float CommentatorImageScale;
		float DiceImageScale;
		int YScale = 0;
		int XScale = 0;
		if (ScreenY <= 1000) {
			CommentatorImageScale = ((*cl_commentator_imagescale - 100) * 0.001);
			DiceImageScale = ((*cl_dice_imagescale - 100) * 0.001);
			YScale = -5;
			XScale = 0;
		}
		else if (ScreenY >= 1399) {
			CommentatorImageScale = ((*cl_commentator_imagescale + 100) * 0.001);
			DiceImageScale = ((*cl_dice_imagescale + 100) * 0.001);
			YScale = 15;
			XScale = 50;
		} else {
			CommentatorImageScale = (*cl_commentator_imagescale * 0.001);
			DiceImageScale = (*cl_dice_imagescale * 0.001);
			YScale = 0;
			XScale = 0;
		}
		float tmpPercentage = 0;
		int tmpPercentage2 = 0;
		std::string tmpCommentary;
		canvas.SetColor(LinearColor{ 255, 255, 255, 255 }); // White

		if (isMatchEnded == false && *bPercentagesEnabled) { // Render Win %
			if (*bOnlyOnScoreboard  && !isScoreboardOpen) {
			} else {
			
			//int ScreenWidthX = gameWrapper->GetScreenSize().X;
			
			Vector2 imagePosTeam0 = { (canvas.GetSize().X / 2) - (320 + XScale), 15 }; // Position for the 'dice' image next to the scoreboard
			Vector2 imagePosTeam1 = { (canvas.GetSize().X / 2) + (260 + XScale), 15 };

			Vector2 starPosTeam0 = { (canvas.GetSize().X / 2) - (340 + XScale), 15 }; // Position for the 'star' image next to the %
			Vector2 starPosTeam1 = { (canvas.GetSize().X / 2) + (320 + XScale), 15 };

			Vector2 textPosTeam0 = { (canvas.GetSize().X / 2) - (250 + XScale), 30 }; // Position for the '%' figure next to the scoreboard
			Vector2 textPosTeam1 = { (canvas.GetSize().X / 2) + (200 + XScale), 30 };

			
			canvas.SetPosition(imagePosTeam0);
			canvas.DrawTexture(dice.get(), DiceImageScale); // Draw the 'dice' image to the screen
			canvas.SetPosition(textPosTeam0);

			if (LocalTeam123 == 1) 
				tmpPercentage = static_cast<float>(GetTeamTotal(1) / static_cast<float>(TotalMMR)); // Check what team you're on and then render the correct % on the correct side of the scoreboard
			else if (LocalTeam123 == 2) 
				tmpPercentage = static_cast<float>(GetTeamTotal(2) / static_cast<float>(TotalMMR));

			tmpPercentage2 = static_cast<int>(ceil(tmpPercentage * 100)); // Turn in to a % and round up, this avoids the weirdness of a 51% v 50% prediction...

			canvas.DrawString(std::to_string(tmpPercentage2) + "%", 1.9, 1.9, 0); // Draw the %

			canvas.SetPosition(imagePosTeam1);
			canvas.DrawTexture(dice.get(), DiceImageScale);
			canvas.SetPosition(textPosTeam1);

			if (LocalTeam123 == 1) 
				tmpPercentage = static_cast<float>(GetTeamTotal(2) / static_cast<float>(TotalMMR));
			else if (LocalTeam123 == 2) 
				tmpPercentage = static_cast<float>(GetTeamTotal(1) / static_cast<float>(TotalMMR));

			tmpPercentage2 = static_cast<int>(floor(tmpPercentage * 100)); // Turn in to a % and round down, this avoids the weirdness of a 51% v 50% prediction...

			canvas.DrawString(std::to_string(tmpPercentage2) + "%", 1.9, 1.9, 0);


				if (LocalTeam123 == 1) {
					if (TeamBaselineMMR[1] > TeamBaselineMMR[2]) {
						canvas.SetPosition(starPosTeam0);
						canvas.DrawTexture(star.get(), DiceImageScale); // Draw the 'dice' image to the screen
					}
					else {
						canvas.SetPosition(starPosTeam1);
						canvas.DrawTexture(star.get(), DiceImageScale); // Draw the 'dice' image to the screen
					}
				}
				else {
					if (TeamBaselineMMR[2] > TeamBaselineMMR[1]) {
						canvas.SetPosition(starPosTeam0);
						canvas.DrawTexture(star.get(), DiceImageScale); // Draw the 'dice' image to the screen
					}
					else {
						canvas.SetPosition(starPosTeam1);
						canvas.DrawTexture(star.get(), DiceImageScale); // Draw the 'dice' image to the screen
					}
				}
			}
		}

		if (*bCommentaryEnabled) {
		//std::string tmpString;
		
			int tmpPosX = 0;
			int tmpPosY = 0;
		if (*cl_commentator_override) { // Overridden the Commentator position
			tmpPosX = *cl_commentator_x - (Commentary.length() * 4);
			tmpPosY = *cl_commentator_y;

			//Vector2 textPosCommentator = { *cl_commentator_x - 75 - (Commentary.length() * 4), *cl_commentator_y + 15 };
		}
		else { // Default position
			/*int tmpCommentatorYPos = 90 + YScale;
			if (isMatchEnded == true) tmpCommentatorYPos = 10;
			Vector2 imagePosCommentator = { (canvas.GetSize().X / 2) - 150 - (Commentary.length() * 4), tmpCommentatorYPos };
			Vector2 textPosCommentator = { (canvas.GetSize().X / 2) - 75 - (Commentary.length() * 4), tmpCommentatorYPos + 15 };*/

			tmpPosX = (canvas.GetSize().X / 2) - (Commentary.length() * 4);
			tmpPosY = 90 + YScale;
		}
		Vector2 imagePosCommentator = { tmpPosX - 150, tmpPosY };
		Vector2 textPosCommentator = { tmpPosX - 75 , tmpPosY + 15 };
		double tmpFontSize = (*cl_commentator_textscale * 0.01);
		if (Commentary != "") {
			canvas.SetColor(CommentaryColour);
			canvas.SetPosition(textPosCommentator);
			canvas.DrawString("\"" + Commentary + "\"", tmpFontSize, tmpFontSize, 1);
			canvas.SetColor(CommentatorColour); // White
			canvas.SetPosition(imagePosCommentator);
			canvas.DrawTexture(commentator.get(), (CommentatorImageScale * 2));
		}

		
		}

		

	}

	//// Debug Text
	/*Vector2 tmpDebugPosition = { 400, 1000 };
	canvas.SetPosition(tmpDebugPosition);
	std::string tmpDebugString;
	tmpDebugString = "Local: " + std::to_string(LocalTeam123) + " T1T " + std::to_string(TeamTotal[1]) + " T2T " + std::to_string(TeamTotal[2]) + " MMRT " + std::to_string(TotalMMR) + " Score " + std::to_string(TeamScore[1]) + "|" + std::to_string(TeamScore[2]);
	tmpDebugString = tmpDebugString + " TBMMR1: " + std::to_string(TeamBaselineMMR[1]) + " TBMMR2 " + std::to_string(TeamBaselineMMR[2]) + " TTe1 " + std::to_string(TeamTotalExtras[1]) + " TTe2 " + std::to_string(TeamTotalExtras[2]);
	canvas.DrawString("DEBUG: " + tmpDebugString, 1, 1, 1);*/


}

void matchodds::OpenScoreboard(std::string eventName) {
	isScoreboardOpen = true;
}

void matchodds::CloseScoreboard(std::string eventName) {

	isScoreboardOpen = false;
}