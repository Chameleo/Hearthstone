/*
 * Aspire Hearthstone
 * Copyright (C) 2008 AspireDev <http://www.aspiredev.org/>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "StdAfx.h"

void WorldSession::HandleSetVisibleRankOpcode(WorldPacket& recv_data)
{
	CHECK_PACKET_SIZE( recv_data, 4 );
	uint32 ChosenRank;
	recv_data >> ChosenRank; 
	if( ChosenRank == 0xFFFFFFFF )
		_player->SetUInt32Value( PLAYER_CHOSEN_TITLE, 0 );
	else if( _player->HasKnownTitle( static_cast< RankTitles >( ChosenRank ) ) )
	{
		_player->SetUInt32Value( PLAYER_CHOSEN_TITLE, ChosenRank );
		if(ChosenRank <= PVPTITLE_HIGH_WARLORD)
		{
			if(ChosenRank > PVPTITLE_GRAND_MARSHAL)
				ChosenRank -= PVPTITLE_GRAND_MARSHAL;
			_player->SetPVPRank(ChosenRank);
		}
	}
}

void HonorHandler::AddHonorPointsToPlayer(shared_ptr<Player>pPlayer, uint32 uAmount)
{
	pPlayer->m_honorPoints += uAmount;
	pPlayer->m_honorToday += uAmount;
	
	pPlayer->HandleProc(PROC_ON_GAIN_EXPIERIENCE, pPlayer, NULL);
	pPlayer->m_procCounter = 0;

	RecalculateHonorFields(pPlayer);
}

int32 HonorHandler::CalculateHonorPointsForKill( shared_ptr<Player>pPlayer, UnitPointer pVictim )
{
	// this sucks.. ;p
	if( pVictim == NULL )
		return 0;

	// Suicide lol
	if( pVictim == pPlayer )
		return 0;

	if( pVictim->GetTypeId() != TYPEID_PLAYER )
		return 0;

	// How dishonorable, you fiend!
	if( pVictim->HasActiveAura( PLAYER_HONORLESS_TARGET_SPELL ) )
		return 0;

	uint32 k_level = pPlayer->GetUInt32Value( UNIT_FIELD_LEVEL );
	uint32 v_level = pVictim->GetUInt32Value( UNIT_FIELD_LEVEL );

	// formula guessed
	int32 honor_points = 6;
	if(k_level != v_level)
	{
		int32 diff = v_level - k_level;
		honor_points += diff;

		if(honor_points <= 0)
			return 0;

		if(honor_points >= 8)
			honor_points = 8;
	}

	honor_points = float2int32(float(honor_points) * World::getSingleton().getRate( RATE_HONOR ));
	//honor_points *= World::getSingleton().getRate( RATE_HONOR );

	return honor_points;
}

void HonorHandler::OnPlayerKilledUnit( shared_ptr<Player>pPlayer, UnitPointer pVictim )
{
	if( pVictim == NULL || pPlayer == NULL || !pVictim->IsPlayer() || !pPlayer->IsPlayer() )
		return;

	if( TO_PLAYER( pVictim )->m_honorless )
		return;

    if( pVictim->IsPlayer() )
	{
		if( pPlayer->m_bg )
		{
			if( TO_PLAYER( pVictim )->m_bgTeam == pPlayer->m_bgTeam )
				return;

			// patch 2.4, players killed >50 times in battlegrounds won't be worth honor for the rest of that bg
			if( TO_PLAYER(pVictim)->m_bgScore.Deaths >= 50 )
				return;
		}
		else
		{
			if( pPlayer->GetTeam() == TO_PLAYER( pVictim )->GetTeam() )
				return;
		}
	}

	// Calculate points
	int32 Points = CalculateHonorPointsForKill(pPlayer, pVictim);

	if( Points > 0 )
	{
		if( pPlayer->m_bg )
		{
			// hackfix for battlegrounds (since the gorups there are disabled, we need to do this manually)
			vector<shared_ptr<Player> > toadd;
			uint32 t = pPlayer->m_bgTeam;
			toadd.reserve(15);		// shouldnt have more than this
			pPlayer->m_bg->Lock();
			set<shared_ptr<Player> > * s = &pPlayer->m_bg->m_players[t];

			for(set<shared_ptr<Player> >::iterator itr = s->begin(); itr != s->end(); ++itr)
			{
				if((*itr) == pPlayer || (*itr)->isInRange(pPlayer,40.0f))
					toadd.push_back(*itr);
			}

			if( toadd.size() > 0 )
			{
				uint32 pts = Points / (uint32)toadd.size();
				for(vector<shared_ptr<Player> >::iterator vtr = toadd.begin(); vtr != toadd.end(); ++vtr)
				{
					AddHonorPointsToPlayer(*vtr, pts);

					(*vtr)->GetAchievementInterface()->HandleAchievementCriteriaHonorableKill();
					(*vtr)->GetAchievementInterface()->HandleAchievementCriteriaHonorableKillClass( pVictim->getClass() );
					(*vtr)->GetAchievementInterface()->HandleAchievementCriteriaHonorableKillRace( pVictim->getRace() );

					(*vtr)->m_killsToday++;
					(*vtr)->m_killsLifetime++;
					pPlayer->m_bg->HookOnHK(*vtr);
					if(pVictim)
					{
						// Send PVP credit
						WorldPacket data(SMSG_PVP_CREDIT, 12);
						uint32 pvppoints = pts * 10;
						data << pvppoints << pVictim->GetGUID() << uint32(TO_PLAYER(pVictim)->GetPVPRank());
						(*vtr)->GetSession()->SendPacket(&data);
					}
				}
			}

			pPlayer->m_bg->Unlock();
		}
		else if(pPlayer->GetGroup())
        {
            Group *pGroup = pPlayer->GetGroup();
            shared_ptr<Player>gPlayer = NULLPLR;
            int32 GroupPoints;
			pGroup->Lock();
            GroupPoints = (Points / (pGroup->MemberCount() ? pGroup->MemberCount() : 1));
            GroupMembersSet::iterator gitr;
			for(uint32 k = 0; k < pGroup->GetSubGroupCount(); k++)
			{
				for(gitr = pGroup->GetSubGroup(k)->GetGroupMembersBegin(); gitr != pGroup->GetSubGroup(k)->GetGroupMembersEnd(); ++gitr)
				{
					gPlayer = (*gitr)->m_loggedInPlayer;
                    
                    if(gPlayer && (gPlayer == pPlayer || gPlayer->isInRange(pPlayer,100.0f))) // visible range
                    {
						gPlayer->GetAchievementInterface()->HandleAchievementCriteriaHonorableKill();
						gPlayer->GetAchievementInterface()->HandleAchievementCriteriaHonorableKillClass( pVictim->getClass() );
						gPlayer->GetAchievementInterface()->HandleAchievementCriteriaHonorableKillRace( pVictim->getRace() );
                        gPlayer->m_killsToday++;
                        gPlayer->m_killsLifetime++;
						if(gPlayer->m_bg)
							gPlayer->m_bg->HookOnHK(gPlayer);

						sHookInterface.OnHonorableKill(gPlayer, TO_PLAYER(pVictim));
		                AddHonorPointsToPlayer(gPlayer, GroupPoints);
                        if(pVictim)
		                {
			                // Send PVP credit
			                WorldPacket data(SMSG_PVP_CREDIT, 12);
			                uint32 pvppoints = GroupPoints * 10;
			                data << pvppoints << pVictim->GetGUID() << uint32(TO_PLAYER(pVictim)->GetPVPRank());
			                gPlayer->GetSession()->SendPacket(&data);
		                }
						//patch by emsy
                        // If we are in Halaa
                        if(pPlayer->GetZoneId() == 3518)
                        {
                            // Add Halaa Battle Token
                            SpellEntry * pvp_token_spell = dbcSpell.LookupEntry(gPlayer->GetTeam()? 33004 : 33005);
                            gPlayer->CastSpell(gPlayer, pvp_token_spell, true);
                        }
                        // If we are in Hellfire Peninsula
                        if(pPlayer->GetZoneId() == 3483)
                        {
                            // Add Mark of Thrallmar/Honor Hold
                            SpellEntry * pvp_token_spell = dbcSpell.LookupEntry(gPlayer->GetTeam()? 32158 : 32155);
                            gPlayer->CastSpell(gPlayer, pvp_token_spell, true);
                        }
                    }

                }
            }
			pGroup->Unlock();
        
        }
        else
        {
			pPlayer->GetAchievementInterface()->HandleAchievementCriteriaHonorableKill();
			pPlayer->GetAchievementInterface()->HandleAchievementCriteriaHonorableKillClass( pVictim->getClass() );
			pPlayer->GetAchievementInterface()->HandleAchievementCriteriaHonorableKillRace( pVictim->getRace() );
		    pPlayer->m_killsToday++;
		    pPlayer->m_killsLifetime++;
		    AddHonorPointsToPlayer(pPlayer, Points);
    		
			if(pPlayer->m_bg)
				pPlayer->m_bg->HookOnHK(pPlayer);

			sHookInterface.OnHonorableKill(pPlayer, TO_PLAYER(pVictim));
		    if(pVictim)
		    {
			    // Send PVP credit
			    WorldPacket data(SMSG_PVP_CREDIT, 12);
			    uint32 pvppoints = Points * 10;
			    data << pvppoints << pVictim->GetGUID() << uint32(TO_PLAYER(pVictim)->GetPVPRank());
			    pPlayer->GetSession()->SendPacket(&data);
		    }
			//patch by emsy
            // If we are in Halaa
            if(pPlayer->GetZoneId() == 3518)
            {
                // Add Halaa Battle Token
                SpellEntry * halaa_spell = dbcSpell.LookupEntry(pPlayer->GetTeam()? 33004 : 33005);
                pPlayer->CastSpell(pPlayer, halaa_spell, true);
            }
        }
	}
}

void HonorHandler::RecalculateHonorFields(shared_ptr<Player>pPlayer)
{
	pPlayer->SetUInt32Value(PLAYER_FIELD_KILLS, uint16(pPlayer->m_killsToday) | ( pPlayer->m_killsYesterday << 16 ) );
	pPlayer->SetUInt32Value(PLAYER_FIELD_TODAY_CONTRIBUTION, pPlayer->m_honorToday);
	pPlayer->SetUInt32Value(PLAYER_FIELD_YESTERDAY_CONTRIBUTION, pPlayer->m_honorYesterday);
	pPlayer->SetUInt32Value(PLAYER_FIELD_LIFETIME_HONORBALE_KILLS, pPlayer->m_killsLifetime);
	pPlayer->SetUInt32Value(PLAYER_FIELD_HONOR_CURRENCY, pPlayer->m_honorPoints);
	pPlayer->SetUInt32Value(PLAYER_FIELD_ARENA_CURRENCY, pPlayer->m_arenaPoints);
}

bool ChatHandler::HandleAddKillCommand(const char* args, WorldSession* m_session)
{
	uint32 KillAmount = args ? atol(args) : 1;
	shared_ptr<Player>plr = getSelectedChar(m_session, true);
	if(plr == 0)
		return true;

	BlueSystemMessage(m_session, "Adding %u kills to player %s.", KillAmount, plr->GetName());
	GreenSystemMessage(plr->GetSession(), "You have had %u honor kills added to your character.", KillAmount);

	for(uint32 i = 0; i < KillAmount; ++i)
		HonorHandler::OnPlayerKilledUnit(plr, NULLPLR);

	return true;
}

bool ChatHandler::HandleAddHonorCommand(const char* args, WorldSession* m_session)
{
	uint32 HonorAmount = args ? atol(args) : 1;
	shared_ptr<Player>plr = getSelectedChar(m_session, true);
	if(plr == 0)
		return true;

	BlueSystemMessage(m_session, "Adding %u honor to player %s.", HonorAmount, plr->GetName());
	GreenSystemMessage(plr->GetSession(), "You have had %u honor points added to your character.", HonorAmount);

	HonorHandler::AddHonorPointsToPlayer(plr, HonorAmount);
	return true;
}

bool ChatHandler::HandlePVPCreditCommand(const char* args, WorldSession* m_session)
{
	uint32 Rank, Points;
	if(sscanf(args, "%u %u", (unsigned int*)&Rank, (unsigned int*)&Points) != 2)
	{
		RedSystemMessage(m_session, "Command must be in format <rank> <points>.");
		return true;
	}
	Points *= 10;
	uint64 Guid = m_session->GetPlayer()->GetSelection();
	if(Guid == 0)
	{
		RedSystemMessage(m_session, "A selection of a unit or player is required.");
		return true;
	}

	BlueSystemMessage(m_session, "Building packet with Rank %u, Points %u, GUID "I64FMT".", 
		Rank, Points, Guid);

	WorldPacket data(SMSG_PVP_CREDIT, 12);
	data << Points << Guid << Rank;
	m_session->SendPacket(&data);
	return true;
}

bool ChatHandler::HandleGlobalHonorDailyMaintenanceCommand(const char* args, WorldSession* m_session)
{
	return false;
}

bool ChatHandler::HandleNextDayCommand(const char* args, WorldSession* m_session)
{
	return false;
}
