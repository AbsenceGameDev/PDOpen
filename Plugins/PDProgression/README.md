This plugin creates a system to grant stats and status effects to players, AI and items. This is done through heavy use of data tables, float curves and UI elements to allow non programmers the freedom to create and modify anything progression related. It also has support for leveling up stats, skills, talent trees and more.

## Table of Contents
* [General Info](#general-information)
* [Features](#features)
* [Screenshots](#screenshots)
* [Setup](#setup)
* [Usage](#usage)
* [Module Status](#module-status)
* [Contact](#contact)


## General Information {#general-information}
This plugin aims to add an easy and extendable way to assign variables to players and objects without needing to write any extra code. Data replicates and is persisted without any programmer input, so anyone from designers, to artists to programmers can add and test progression system functionality.

Another major benefit of this system is that stats are automatically known to status effects, talents and skill trees through the use of gameplay tags. This makes it easy to attach a stat like health to a status like +50% max health by just checking a few boxes. The inventory system also fully supports these stats on equipped items, consumables and damage events like grenade explosions that can leave a poison pool on the ground.


## Features {#features}
The currently implemented features include
* Player Stats that can be applied to a player on spawn
  * This is typically used for health, stamina, damage reduction, movement speed, etc., but can be used for anything and everything that can be modified during gameplay
* Player Skills
  * A player skill is just a stat that can level up through game events. A stat for how long you can hold your breath underwater could be leveled up by spending enough time under water for example
* Talent Trees
  * A talent is any buff or ability that is granted by leveling up skills or experience in general
  * These support multiple trees that a game can allow a player to progress through at their own discretion
  * Talents can be leveled up individually by spending tokens earned by leveling up skills 
* Status Effects
  * A status effect can be applied to an actor automatically or manually with an optional timer
  * These effects can alter stats, like move 10% slower, or deal 10 damage every 1.5 seconds until falling off
  * You can get creative with status effects and have them applied by stepping into a volume, taking the right kind of damage, or by wearing a special piece of body armor
* UI
  * The plugin comes with many widget classes that allow you to define screens for reading stats and progressing through skill trees


## Screenshots {#screenshots}

Stats Data Table Example
<img alt="This is a diagram" src="./Resources/StatsPage.jpg" height=100% width=100%/>


## Setup {#setup}
The initial set up for the player progression system is fairly quick. It basically involves creating a few data tables, a manager blueprint and setting up the character's progression component

* Create a blueprint based off of the PDProgressionManager class or one of its children if modifications need to be made
  * All data tables created in the next steps will need to be added here in the correct array
  * StatCategories = UI categories that are completely optional
  * StatDataTables is a list of all stat data tables that should be used in game. This includes all player types and AI types
* The progression manager needs to be added to the GameState blueprint for the project
	* All manager classes from plugins will be listed here and the game state will handle ticking and initialization
	* Add the progression manager blueprint to the GameState's ManagersToSpawn array
* Create a data table with type PDProgressionTableStatData
  * This is data table for general stats like health and stamina
  * One should exist for the player, and possibly for AI depending on the game
* Create a data table with type PDProgressionTableStatusEffectData
  * This is a data table for all status effects in the game
  * Populate the table with some generic effects like death and winded (out of stamina)
  * See the [Usage](#usage) section for more information
* If needed, create and configure a data table for skill trees using the PDProgressionStatTree type
  * See the [Usage](#usage) section for more information
* The shooter module character class has a progression component set up and ready to go
  * In your character blueprint, configure the progression component as follows
  * @todo Update with Chiba changes for setting the stats data table here
* Create necessary UI files
  * This one is project specific, but the following classes can be used
  * @todo Move the files from FRV to the PDProgression plugin and update this

## Usage {#usage}


### Creating Stats and Status Effects

This section assumes you have already created the necessary data tables for stats and status effects
* In the stats data table, create a new row and name it something descriptive and unique. This value will not be used in code anywhere and should be human readable.
* Set the USID (unique stat id) to a unique number. Every stat is replicated and persisted with this value, so it is important for no two values to be equal
  * These values are verified to be unique and above 0 when running data validation checks
* Set the stat tag to something descriptive that matches the C++ name (see below)
  * This is usually along the lines of PlayerProgress.Stats.Health in format
* Set the StartingValue, MinValue, MaxValue etc to be values that describe your stat
  * It is important to note that all of these values are integers, so for percent values, we usually treat 1000 as 1.0 or 100% in the stat calculations
* Set the RegenValue and RegenInterval if the stat should regenerate over time up to its maximum, or down to its minimum
* The level conditions are only used for stats that should gain experience through performing actions in game and can modify other stats as they level up
  * As an example, FRV has a Rifles stat that gains experience as you kill mutants with rifles. The level curve determines the amount of experience needed to reach each level, and the StatValues curves determine how much a stat will be modified at each level
  * TokensToGrant allows selecting a curve that will grant tokens at different levels. Tokens can be spent in skill trees
* The rest of the stats have tooltips to help explain their functionality

* For status effects, the general rules all still apply
* Creating a row, setting a unique id and creating a unique gameplay tag are all the same as the stats above
* Setting the Triggers array allows a status effect to be granted automatically when another stat reaches a certain threshold
  * This is useful for granting things like a 'Winded' status effect when the player's stamina reaches 0
* StatModifiers is a list of stats that will be modified over the duration of the status effect
  * This can passive things like reducing movement speed, or active stats on a timer like reduce health every second
* ForbidsEffects allows specified effects to not be granted while this effect is active
* Duration is the length of time in seconds before the status effect will be removed
* That's it! Status effects offer an easy way to add buffs and debuffs that alter any stat quickly 

### Referencing Stats in C++

Creating stats is done in data tables within the editor, but we need to create the logic that uses that stats in C++ quite often. This is easy to do with a few simple macros as follows

* Create a generic header in your project, if one does not exist yet, and name it PRJProgressionCommon.h where PRJ is the name of the project
* Add any stats and status effects to this file as a macro with the following format
~~~
#define RPG_STAT_SPRINTINGSTAMINACOST				FGameplayTag::RequestGameplayTag("PlayerProgress.Stats.SprintingStaminaCost", false)
#define RPG_STAT_STAMINAEFFICIENCY				FGameplayTag::RequestGameplayTag("PlayerProgress.Stats.StaminaEffeciency", false)
#define RPG_STAT_JUMPSTAMINACOST				FGameplayTag::RequestGameplayTag("PlayerProgress.Stats.JumpStaminaCost", false)
#define RPG_STAT_HEALTH						FGameplayTag::RequestGameplayTag("PlayerProgress.Stats.Health", false)
#define RPG_STAT_STAMINA					FGameplayTag::RequestGameplayTag("PlayerProgress.Stats.Stamina", false)
~~~
* These values are taken from the PDShooterProgressionCommon.h file 
  * The define at the start is the name of the variable you will want to reference in code
  * The function call at the end takes in a gameplay tag as a string and returns the actual gameplay tag that can be used by all of our functions
~~~
Character->GetPlayerStat(RPG_STAT_HEALTH);
~~~
* The exact same macro can be used to reference status effects or anything else by a gameplay tag


### Retrieving Current Stat Values

Current stat and max stat values can be grabbed for players using the ShooterCharacter functions below
* APDShooterCharacter::GetPlayerStat
  * Returns the current stat value as an integer
* APDShooterCharacter::GetPlayerStatAsModifier
  * Returns the stat value divided by 1000.0f so it is treated like a modifier
  * A value of 1000 will be returned as 1.0f so no change will be made
  * A value of 500 will be returned as 0.5f and will be interpreted as 50%
* APDShooterCharacter::GetPlayerStatPercentage
  * Returns a stat as a percentage of its max value (only for stats that have max value set)
  * Good for stats like health and stamina


### Alter Stat Value

Stats can be altered by using the following functions found in the ShooterCharacter class
* APDShooterCharacter::SetPlayerStatValue
  * Set the stat value directly 
* APDShooterCharacter::AdjustPlayerStatValue
  * Adjusts the stat value by a desired amount. The value will be clamped internally


### Check If Player Has Status Effect

This can be done easily by calling HasStatusEffect on the shooter character or the progression component directly


### Apply Status Effects

The player progression component has a function called AddStatusEffect and RemoveStatusEffect which will take in a gameplay tag and add or remove the status effect for you.


### Grant Stat Experience Points

Stat experience must be granted through the progression manager as follows
~~~
	FPDProgressionStatExp StatExp;
	StatExp.Exp = Exp;
	StatExp.USID = GameState->GetProgressionManager()->GetStatDefaultsByStatTag(SkillTag)->USID;
	
	GameState->GetProgressionManager()->IncrementStatExp(ProgressionComponent, StatExp);
~~~
This can be used to increase or decrease the amount of experience a stat has.


### Create Level Curves

Float curves are used by various effects and leveling systems in the plugin. There is a special type of float curve added named PDProgressionStatLevelCurve that adds curves for additive vs multiplicative curves
<img alt="This is a diagram" src="./Resources/StatsCurve.jpg" height=100% width=100%/>

* Create a curve with the PDProgressionStatLevelCurve type and set it up to be either an additive or multiplicative curve
  * Additive = A percentage value added to the base of 1.0 for a stat
  * Multiplicative = A percentage value multiplied by the current value of a stat
  * DirectValue = This will adjust the base value by the amount specified
* See the UPDProgressionManager::ProcessStat function for more details on how these stats get processed and calculated
* Once a curve is created, it can be used in any of the data tables that support it


### Replication and Persistence

Replication and persistence are handled automatically for new stats. This is a brief explanation of the process for that

* All replication is using push model to be as lightweight as possible
* Every stat, talent, status effect, etc is assigned a unique id in the data table they belong to
* This id is replicated to other clients along with the current and max value
* The exact same data is sent to the persistence system with various override function in the progression manager class
* That's it! Nice and light

## Module Status {#module-status}
Module is: _complete_


## Contact {#contact}
Module maintained by [**Ario Armin**]() and [**Chris Gunderson**](mailto:chris@ironbellystudios.com)
