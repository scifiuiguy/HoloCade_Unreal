// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "VRPlayerTransport/XRReplicatedData.h"
#include "HeadMountedDisplayTypes.h"

const FReplicatedHandKeypoint* FReplicatedHandData::GetKeypoint(EHandKeypoint Keypoint) const
{
	switch (Keypoint)
	{
	case EHandKeypoint::Wrist:
		return &Wrist;
	case EHandKeypoint::MiddleMetacarpal:
		return &HandCenter;
	case EHandKeypoint::ThumbTip:
		return &ThumbTip;
	case EHandKeypoint::IndexTip:
		return &IndexTip;
	case EHandKeypoint::MiddleTip:
		return &MiddleTip;
	case EHandKeypoint::RingTip:
		return &RingTip;
	case EHandKeypoint::LittleTip:
		return &LittleTip;
	default:
		// For keypoints not explicitly stored, return nullptr
		// Future enhancement: Store all keypoints in a map for complete hand skeleton replication
		return nullptr;
	}
}

void FReplicatedHandData::SetKeypoint(EHandKeypoint Keypoint, const FReplicatedHandKeypoint& KeypointData)
{
	switch (Keypoint)
	{
	case EHandKeypoint::Wrist:
		Wrist = KeypointData;
		break;
	case EHandKeypoint::MiddleMetacarpal:
		HandCenter = KeypointData;
		break;
	case EHandKeypoint::ThumbTip:
		ThumbTip = KeypointData;
		break;
	case EHandKeypoint::IndexTip:
		IndexTip = KeypointData;
		break;
	case EHandKeypoint::MiddleTip:
		MiddleTip = KeypointData;
		break;
	case EHandKeypoint::RingTip:
		RingTip = KeypointData;
		break;
	case EHandKeypoint::LittleTip:
		LittleTip = KeypointData;
		break;
	default:
		// For keypoints not explicitly stored, ignore
		// Future enhancement: Store all keypoints in a map for complete hand skeleton replication
		break;
	}
}

