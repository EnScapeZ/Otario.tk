#pragma once
#include "..\SDK\CInput.h"
#include "..\SDK\CEntity.h"
#include "..\Utils\GlobalVars.h"
#include "..\SDK\ClientClass.h"
#include "..\Settings.h"
#include "Aimbot.h"

struct lbyRecords
{
	int tick_count;
	float lby;
	Vector headPosition;
};
struct backtrackData
{
	float simtime;
	Vector hitboxPos;
};

class BackTrack
{
public:
	float simtime;
	Vector hitboxPos;
	void legitBackTrack(CUserCmd* cmd, C_BaseEntity* pLocal);
	Vector AngleVector(Vector meme);
	float DistPointToLine(Vector Point, Vector LineOrigin, Vector Dir);
};

inline Vector BackTrack::AngleVector(Vector meme)
{
	auto sy = sin(meme.y / 180.f * static_cast<float>(PI));
	auto cy = cos(meme.y / 180.f * static_cast<float>(PI));

	auto sp = sin(meme.x / 180.f * static_cast<float>(PI));
	auto cp = cos(meme.x / 180.f* static_cast<float>(PI));

	return Vector(cp*cy, cp*sy, -sp);
}

inline float BackTrack::DistPointToLine(Vector Point, Vector LineOrigin, Vector Dir)
{
	auto PointDir = Point - LineOrigin;

	auto TempOffset = PointDir.Dot(Dir) / (Dir.x*Dir.x + Dir.y*Dir.y + Dir.z*Dir.z);
	if (TempOffset < 0.000001f)
		return FLT_MAX;

	auto PerpendicularPoint = LineOrigin + (Dir * TempOffset);

	return (Point - PerpendicularPoint).Length();
}

extern backtrackData headPositions[64][12];

extern BackTrack* backtracking;
