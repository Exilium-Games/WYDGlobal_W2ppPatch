/*
*   Copyright (C) {2015}  {VK, Charles TheHouse}
*
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program.  If not, see [http://www.gnu.org/licenses/].
*
*/
#include "ProcessClientMessage.h"

void Exec_MSG_TradingItem(int conn, char *pMsg)
{
	MSG_TradingItem *m = (MSG_TradingItem*)pMsg;

	if (pMob[conn].MOB.CurrentScore.Hp == 0 || pUser[conn].Mode != USER_PLAY)
	{
		SendHpMode(conn);
		AddCrackError(conn, 1, 19);
		return;
	}

	if (pUser[conn].Trade.OpponentID)
	{
		RemoveTrade(pUser[conn].Trade.OpponentID);
		RemoveTrade(conn);
		return;
	}

	if (pUser[conn].TradeMode)
	{
		SendClientMessage(conn, g_pMessageStringTable[_NN_CantWhenAutoTrade]);
		return;
	}

	if (m->SrcPlace == m->DestPlace && m->SrcSlot == m->DestSlot)
	{
		Log("err,trading fails.tentativa de dup", pUser[conn].AccountName, pUser[conn].IP);
		return;
	}


	if ((m->DestPlace == ITEM_PLACE_CARGO || m->SrcPlace == ITEM_PLACE_CARGO) && (pMob[conn].MOB.Class != 3 || !(pMob[conn].MOB.LearnedSkill & 0x2000)))
	{
		int WarpID = m->WarpID;

		if (WarpID <= MAX_USER || WarpID >= MAX_MOB)
		{
			SendClientSignal(conn, ESCENE_FIELD, 412);
			return;
		}

		if (pMob[WarpID].Mode != MOB_PEACE || pMob[WarpID].MOB.Merchant != 2)
		{
			SendClientSignal(conn, ESCENE_FIELD, 412);
			return;
		}

		int PosX = pMob[conn].TargetX;
		int PosY = pMob[conn].TargetY;
		int tx = pMob[WarpID].TargetX;
		int ty = pMob[WarpID].TargetY;

		if (PosX < tx - VIEWGRIDX || PosX > tx + VIEWGRIDX || PosY < ty - VIEWGRIDY || PosY > ty + VIEWGRIDY)
		{
			SendClientSignal(conn, ESCENE_FIELD, 412);
			return;
		}
	}
	STRUCT_ITEM *Mount = &pMob[conn].MOB.Equip[14];

	int DestPlace = 0;
	int SrcPlace = 0;

	STRUCT_ITEM *DestItem = GetItemPointer(&pMob[conn].MOB, pUser[conn].Cargo, m->DestPlace, m->DestSlot);
	STRUCT_ITEM *SrcItem = GetItemPointer(&pMob[conn].MOB, pUser[conn].Cargo, m->SrcPlace, m->SrcSlot);

	STRUCT_ITEM *dItem = GetItemPointer(&pMob[conn].MOB, pUser[conn].Cargo, m->DestPlace, m->DestSlot);
	STRUCT_ITEM *sItem = GetItemPointer(&pMob[conn].MOB, pUser[conn].Cargo, m->SrcPlace, m->SrcSlot);


	if (m->DestPlace)
	{
		if (m->DestPlace == ITEM_PLACE_CARRY)
		{
			if (m->DestSlot >= MAX_CARRY-4)
			{
				Log("err,trading fails.SwapItem-Carry", pUser[conn].AccountName, pUser[conn].IP);
				return;
			}
		}
		else
		{
			if (m->DestPlace != ITEM_PLACE_CARGO)
			{
				Log("err,trading fails.SwapItem source type", pUser[conn].AccountName, pUser[conn].IP);
				return;
			}
			if (m->DestSlot > MAX_CARGO)
			{
				Log("err,trading fails.SwapItem-Cargo", pUser[conn].AccountName, pUser[conn].IP);
				return;
			}
		}
	}
	else
	{
		if (m->DestSlot < 0 || m->DestSlot >= MAX_EQUIP)
		{
			Log("err,trading fails.SwapItem-Equip", pUser[conn].AccountName, pUser[conn].IP);
			return;
		}
		DestPlace = 1;
		SrcPlace = 1;
	}
	if (m->SrcPlace)
	{
		if (m->SrcPlace == ITEM_PLACE_CARRY)
		{
			if (m->SrcSlot >= MAX_CARRY-4)
			{
				Log("DEBUG:Trading Fails.Swap-Carr", pUser[conn].AccountName, pUser[conn].IP);
				return;
			}
		}
		else
		{
			if (m->SrcPlace != ITEM_PLACE_CARGO)
			{
				Log("DEBUG:Trading Fails.SwapItem dest type", pUser[conn].AccountName, pUser[conn].IP);
				return;
			}
			if (m->SrcSlot > MAX_CARGO)
			{
				Log("DEBUG:Trading Fails.Swap-Cargo", pUser[conn].AccountName, pUser[conn].IP);
				return;
			}
		}
	}
	else
	{
		if (m->SrcSlot <= 0 || m->SrcSlot >= MAX_EQUIP)
		{
			Log("DEBUG:Trading Fails.Swap-Equip", pUser[conn].AccountName, pUser[conn].IP);
			return;
		}
		DestPlace = 1;
		SrcPlace = 1;
	}

	if (m->DestPlace == ITEM_PLACE_CARRY && m->DestSlot >= pMob[conn].MaxCarry)
		return;

	if (DestItem == NULL || SrcItem == NULL)
	{
		Log("DEBUG:Trading Fails.Swap-Equip", pUser[conn].AccountName, pUser[conn].IP);
		return;
	}

	if (DestItem->sIndex != 747 && SrcItem->sIndex != 747 || m->DestPlace != ITEM_PLACE_CARGO && m->SrcPlace != ITEM_PLACE_CARGO)
	{

		int Can = 1;
		int YouCan = 1;

		if (dItem->sIndex)
		{
			int error = -2;

			if (m->SrcPlace == ITEM_PLACE_EQUIP)
				Can = BASE_CanEquip(dItem, &pMob[conn].MOB.CurrentScore, m->SrcSlot, pMob[conn].MOB.Class, pMob[conn].MOB.Equip, &pMob[conn].extra);

			if (m->SrcPlace == ITEM_PLACE_CARRY)
			{
				if (!Can)
				{
					if (error > 0 && error <= pMob[conn].MaxCarry)
					{
						error--;
						SendItem(conn, ITEM_PLACE_CARRY, error, &pMob[conn].MOB.Carry[error]);
					}
				}
			}
		}

		if (sItem->sIndex)
		{
			int error = -2;

			if (m->DestPlace == ITEM_PLACE_EQUIP)
				YouCan = BASE_CanEquip(sItem, &pMob[conn].MOB.CurrentScore, m->DestSlot, pMob[conn].MOB.Class, pMob[conn].MOB.Equip, &pMob[conn].extra);

			if (m->DestPlace == ITEM_PLACE_CARRY)
			{
				if (!YouCan)
				{
					if (error > 0 && error <= pMob[conn].MaxCarry)
					{
						error--;
						SendItem(conn, ITEM_PLACE_CARRY, error, &pMob[conn].MOB.Carry[error]);
					}
				}
			}
		}
		if (Can && YouCan)
		{
				
			STRUCT_ITEM save1;
			STRUCT_ITEM save2;

			save1.sIndex = SrcItem->sIndex;
			save1.stEffect[0].cEffect = SrcItem->stEffect[0].cEffect;
			save1.stEffect[0].cValue = SrcItem->stEffect[0].cValue;
			save1.stEffect[1].cEffect = SrcItem->stEffect[1].cEffect;
			save1.stEffect[1].cValue = SrcItem->stEffect[1].cValue;
			save1.stEffect[2].cEffect = SrcItem->stEffect[2].cEffect;
			save1.stEffect[2].cValue = SrcItem->stEffect[2].cValue;

			save2.sIndex = DestItem->sIndex;
			save2.stEffect[0].cEffect = DestItem->stEffect[0].cEffect;
			save2.stEffect[0].cValue = DestItem->stEffect[0].cValue;
			save2.stEffect[1].cEffect = DestItem->stEffect[1].cEffect;
			save2.stEffect[1].cValue = DestItem->stEffect[1].cValue;
			save2.stEffect[2].cEffect = DestItem->stEffect[2].cEffect;
			save2.stEffect[2].cValue = DestItem->stEffect[2].cValue;

			if((save1.sIndex == save2.sIndex && save1.sIndex == 413
			|| save1.sIndex == save2.sIndex && save1.sIndex == 412
			|| save1.sIndex == save2.sIndex && save1.sIndex == 419
			|| save1.sIndex == save2.sIndex && save1.sIndex == 420
			|| save1.sIndex == save2.sIndex && save1.sIndex >= 2390 && save1.sIndex <= 2419)
			&& BASE_GetItemAmount(&save1) < 255 && BASE_GetItemAmount(&save2) < 255)
			{
				int amount1 = BASE_GetItemAmount(&save1);
				int amount2 = BASE_GetItemAmount(&save2);
				int tamount = (amount1 <= 0 ? 1 : amount1) + (amount2 <= 0 ? 1 : amount2);

				if(tamount <= 254)
				{
					save1.stEffect[0].cEffect = 61;
					save1.stEffect[0].cValue = tamount;

					BASE_ClearItem(&save2);

					SendItem(conn, m->SrcPlace, m->SrcSlot, &save1);
					SendItem(conn, m->DestPlace, m->DestSlot, &save2);
				}
				else if(tamount >= 255)
				{
					save1.stEffect[0].cEffect = 61;
					save1.stEffect[0].cValue = 255;

					tamount -= 255;

					if(tamount >= 1)
						BASE_SetItemAmount(&save2, tamount > 255 ? 255 : tamount);

					if(tamount <= 0)
						BASE_ClearItem(&save2);

					SendItem(conn, m->SrcPlace, m->SrcSlot, &save1);
					SendItem(conn, m->DestPlace, m->DestSlot, &save2);
				}
			}

			if(save1.EquipCostume() && save1.stEffect[0].cEffect == 0 && save1.stEffect[1].cEffect == 0 && save1.stEffect[2].cEffect == 0)
			{
				BASE_SetItemDate(&save1, 30);
				SendItem(conn, m->SrcPlace, m->SrcSlot, &save1);
			}
			if(save2.EquipCostume() && save2.stEffect[0].cEffect == 0 && save2.stEffect[1].cEffect == 0 && save2.stEffect[2].cEffect == 0)
			{
				BASE_SetItemDate(&save2, 30);
				SendItem(conn, m->DestPlace, m->DestSlot, &save2);
			}
			if (save1.EsferaPet() && BASE_CheckItemDate(&save1) || save1.EquipCostume() && BASE_CheckItemDate(&save1))
			{
				BASE_ClearItem(&save1);
				SendItem(conn, m->SrcPlace, m->SrcSlot, &save1);
			}

			if (save2.EsferaPet() && BASE_CheckItemDate(&save2) || save2.EquipCostume() && BASE_CheckItemDate(&save2))
			{
				BASE_ClearItem(&save2);
				SendItem(conn, m->DestPlace, m->DestSlot, &save2);
			}

			char itemLog[2048];
			char itemLog2[2048];

			BASE_GetItemCode(&save1, itemLog);
			BASE_GetItemCode(&save2, itemLog2);

			sprintf(temp, "tradingitem, SrcType:%d SrcSlot:%d Item:%s DstType:%d DstSlot:%d Item:%s", m->SrcPlace, m->SrcSlot, itemLog, m->DestPlace, m->DestSlot, itemLog2);
			ItemLog(temp, pUser[conn].AccountName, pUser[conn].IP);	

			memcpy(SrcItem, &save2, sizeof(STRUCT_ITEM));
			memcpy(DestItem, &save1, sizeof(STRUCT_ITEM));


			if (save2.sIndex <= 40)
				GetGuild(SrcItem);

			if (save1.sIndex <= 40)
				GetGuild(DestItem);

			pMob[conn].GetCurrentScore(conn);

			pUser[conn].cSock.AddMessage((char*)m, sizeof(MSG_TradingItem));

			if (!pMob[conn].MOB.Equip[6].sIndex)
			{
				if (pMob[conn].MOB.Equip[7].sIndex)
				{
					int hab = BASE_GetItemAbility(&pMob[conn].MOB.Equip[7], EF_POS);

					if (hab != 128)
					{
						m->DestPlace = ITEM_PLACE_EQUIP;
						m->DestSlot = 6;
						m->SrcPlace = ITEM_PLACE_EQUIP;
						m->SrcSlot = 7;

						pUser[conn].cSock.AddMessage((char*)m, sizeof(MSG_TradingItem));

						pMob[conn].MOB.Equip[6] = pMob[conn].MOB.Equip[7];

						memset(&pMob[conn].MOB.Equip[7], 0, sizeof(STRUCT_ITEM));
					}
				}
			}

			if (DestPlace == ITEM_PLACE_CARRY)
				SendEquip(conn, conn);

			if (SrcPlace == ITEM_PLACE_CARRY)
			{
				GetGuild(conn);
				SendScore(conn);
			}

			if(DestItem->sIndex >= 2330 && DestItem->sIndex < 2360 || SrcItem->sIndex >= 2330 && SrcItem->sIndex < 2360)
				MountProcess(conn, Mount);
 
		}
	}
}
