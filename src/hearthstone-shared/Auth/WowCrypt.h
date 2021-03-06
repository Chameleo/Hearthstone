/*
 * Aspire Hearthstone
 * Copyright (C) 2008 - 2009 AspireDev <http://www.aspiredev.org/>
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

#ifndef _WOWCRYPT_H
#define _WOWCRYPT_H

#include <stdlib.h>
#include "../Common.h"
#include "AuthPacketKey.h"
#include "SARC4.h"
#include "BigNumber.h"
#include <vector>

class WowCrypt 
{
public:
	WowCrypt();
	~WowCrypt();

	void Init(uint8 *K);
	void DecryptRecv(uint8 * data, size_t len);
	void EncryptSend(uint8 * data, size_t len);

	bool IsInitialized() { return _initialized; }

private:
	bool _initialized;
	SARC4 _Decrypt;
	SARC4 _Encrypt;
};

#endif
