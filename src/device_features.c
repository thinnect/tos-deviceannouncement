/**
 * Device feature storage in a linked-list with user-provided list elements.
 *
 * Copyright Thinnect Inc. 2019
 * @author Raido Pahtma
 * @license MIT
 */

#include "device_features.h"

#include "loglevels.h"
#define __MODUUL__ "DevF"
#define __LOG_LEVEL__ ( LOG_LEVEL_device_features & BASE_LOG_LEVEL )
#include "log.h"

static device_feature_t * mp_features;
static uint8_t m_count;

void devf_init ()
{
	mp_features = NULL;
	m_count = 0;
}

uint8_t devf_count ()
{
	return m_count;
}

uint32_t devf_hash ()
{
	uint32_t hash = 0;  // Really simple for now, just a sum of bytes
	device_feature_t * pf = mp_features;
	while (NULL != pf)
	{
		for (uint8_t i=0; i < sizeof(nx_uuid_t); i++)
		{
			hash += ((uint8_t*)&(pf->uuid))[i];
		}
		pf = pf->next;
	}
	return hash;
}

bool devf_get_feature (uint8_t fnum, nx_uuid_t* pftr)
{
	device_feature_t * pf = mp_features;
	for (uint8_t i=0; i < fnum; i++)
	{
		if  (NULL == pf)
		{
			return false;
		}
		pf = pf->next;
	}
	if (NULL != pf)
	{
		memcpy(pftr, &(pf->uuid), sizeof(nx_uuid_t));
		return true;
	}
	return false;
}

bool devf_add_feature (device_feature_t * pftr, nx_uuid_t * puuid)
{
	device_feature_t * pf = mp_features;
	while (NULL != pf)
	{
		// Features must not be added multiple times
		debug1("%d %p %p %p %p %d", (int)m_count, pftr, puuid, pf, &(pf->uuid), sizeof(nx_uuid_t));
		if ((pf == pftr)||(0 == memcmp(&(pf->uuid), puuid, sizeof(nx_uuid_t))))
		{
			warnb1("dup %p %p", puuid, sizeof(nx_uuid_t), pftr, pf);
			return false;
		}
		if (NULL == pf->next)
		{
			break;
		}
		else
		{
			pf = pf->next;
		}
	}

	if (NULL == pf)
	{
		mp_features = pftr;
	}
	else
	{
		pf->next = pftr; // Append to the end
	}

	memcpy(&(pftr->uuid), puuid, sizeof(nx_uuid_t));
	pftr->next = NULL;
	m_count++;
	return true;
}

bool devf_remove_feature (device_feature_t * pftr)
{
	if (NULL != mp_features)
	{
		if (mp_features == pftr)
		{
			mp_features = mp_features->next;
			m_count--;
			return true;
		}
		else
		{
			device_feature_t * pf = mp_features;
			while (NULL != pf->next)
			{
				if (pf->next == pftr)
				{
					pf->next = pf->next->next;
					m_count--;
					return true;
				}
				pf = pftr->next;
			}
		}
	}
	return false;
}
