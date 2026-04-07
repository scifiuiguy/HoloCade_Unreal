// Copyright (c) 2025 AJ Campbell. Licensed under the MIT License.

#include "I433MHzReceiver.h"
#include "Generic433MHzReceiver.h"
#include "RTL433MHzReceiver.h"
#include "CC1101433MHzReceiver.h"
#include "RFM69433MHzReceiver.h"
#include "RFM95433MHzReceiver.h"

TUniquePtr<I433MHzReceiver> I433MHzReceiver::CreateReceiver(const FRF433MHzReceiverConfig& Config)
{
	switch (Config.ReceiverType)
	{
	case ERF433MHzReceiverType::RTL_SDR:
		return MakeUnique<FRTL433MHzReceiver>();
		
	case ERF433MHzReceiverType::CC1101:
		return MakeUnique<FCC1101433MHzReceiver>();
		
	case ERF433MHzReceiverType::RFM69:
		return MakeUnique<FRFM69433MHzReceiver>();
		
	case ERF433MHzReceiverType::RFM95:
		return MakeUnique<FRFM95433MHzReceiver>();
		
	case ERF433MHzReceiverType::Generic:
	default:
		return MakeUnique<FGeneric433MHzReceiver>();
	}
}

