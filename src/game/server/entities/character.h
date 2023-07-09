/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_CHARACTER_H
#define GAME_SERVER_ENTITIES_CHARACTER_H

#include <game/server/entity.h>
#include <game/generated/server_data.h>
#include <game/generated/protocol.h>

#include <game/gamecore.h>

enum
{
	WEAPON_GAME = -3,  // team switching etc
	WEAPON_SELF = -2,  // console kill command
	WEAPON_WORLD = -1, // death tiles etc
};

class CCharacter : public CEntity
{
	MACRO_ALLOC_POOL_ID()

public:
	// character's size
	static const int ms_PhysSize = 28;

	CCharacter(CGameWorld *pWorld);

	virtual void Reset();
	virtual void Destroy();
	virtual void Tick();
	virtual void TickDefered();
	virtual void TickPaused();
	virtual void Snap(int SnappingClient);

	bool IsGrounded();

	void SetWeapon(int W);
	void HandleWeaponSwitch();
	void DoWeaponSwitch();

	void HandleWeapons();
	void HandleNinja();

	void OnPredictedInput(CNetObj_PlayerInput *pNewInput);
	void OnDirectInput(CNetObj_PlayerInput *pNewInput);
	void ResetInput();
	void FireWeapon();

	void Die(int Killer, int Weapon);
	bool TakeDamage(vec2 Force, int Dmg, int From, int Weapon);

	bool Spawn(class CPlayer *pPlayer, vec2 Pos);
	bool Remove();

	bool IncreaseHealth(int Amount);
	bool IncreaseArmor(int Amount);

	bool GiveWeapon(int Weapon, int Ammo);
	void GiveNinja();

	void SetEmote(int Emote, int Tick);
	void SetEmoteFor(int Emote, int Ticks, int LockEmote = 0, bool UseTime = true);

	bool IsAlive() const { return m_Alive; }
	class CPlayer *GetPlayer() { return m_pPlayer; }

	bool Hooking();
	int HookedPlayer();

	vec2 GetPosition(){ return m_Pos; }
	
	vec2 GetVel(){ return m_Core.m_Vel; }

	void AutoWeaponChange();

	bool m_IsBot;

	int GetActiveWeapon(){ return m_ActiveWeapon; }

	struct WeaponStat
	{
		int m_AmmoRegenStart;
		int m_Ammo;
		int m_Ammocost;
		bool m_Got;

	} m_aWeapons[NUM_WEAPONS];
 
	bool m_InSleep; // AI死亡后不能删除他的Character类，而是将他设置为睡眠，防止线程错误导致崩溃（悲）

	void HandleAI();

	// Need for bot (for now)
	class CCharacterCore *GetCore() { return &m_Core;}
	bool CanFire() { return m_ReloadTimer <= 0; }
	int GetHealth() { return m_Health; }
	int GetArmor() { return m_Armor; }
	int GetAmmoCount(int Weapon) { return m_aWeapons[clamp(Weapon, 0,NUM_WEAPONS-1)].m_Ammo; }

private:   
	// player controlling this character
	class CPlayer *m_pPlayer;

	int m_ActiveWeapon;
	int m_LastWeapon;
	int m_QueuedWeapon;

	bool m_Alive;

	// weapon info
	CEntity *m_apHitObjects[10];
	int m_NumObjectsHit;

	int m_ReloadTimer;
	int m_AttackTick;

	int m_DamageTaken;

	int m_EmoteType;
	int m_EmoteStop;
	int m_EmoteLockStop;

	// last tick that the player took any action ie some input
	int m_LastAction;
	int m_LastNoAmmoSound;

	// these are non-heldback inputs
	CNetObj_PlayerInput m_LatestPrevInput;
	CNetObj_PlayerInput m_LatestInput;

	// input
	CNetObj_PlayerInput m_PrevInput;
	CNetObj_PlayerInput m_Input;
	int m_NumInputs;
	int m_Jumped;

	int m_DamageTakenTick;

	int m_Health;
	int m_Armor;

	// ninja
	struct
	{
		vec2 m_ActivationDir;
		int m_ActivationTick;
		int m_CurrentMoveTime;
		int m_OldVelAmount;
	} m_Ninja;

	// the player core for the physics
	CCharacterCore m_Core;

	// info for dead reckoning
	int m_ReckoningTick;			// tick that we are performing dead reckoning From
	CCharacterCore m_SendCore;		// core that we should send
	CCharacterCore m_ReckoningCore; // the dead reckoning core
};

#endif
