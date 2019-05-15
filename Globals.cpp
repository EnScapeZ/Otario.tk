#include "Globals.h"

namespace Globals
{
	bool bsendpacket = false;
	bool can_shoot_someone = false;
	bool disable_fake_lag = false;
	bool shouldstop = false;
	float flHurtTime = 0.f;
	bool dontdoaa = false;
	bool preservedelete = false;
	bool lbybroken = false;
	int shots_hit[65];
	int shots_fired[65];
	bool hit[65];
	bool shot[65];
	bool isfakeducking = false;
	Vector AngleAA = Vector(0, 0, 0);
	bool playable = false;
	bool breakinglc = false;
	float dormanttime = 0.f;
	int damage = -1;
}