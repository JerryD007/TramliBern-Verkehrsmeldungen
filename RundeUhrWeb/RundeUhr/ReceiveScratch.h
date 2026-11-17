#pragma once
#include <new>
#include <type_traits>
// Queue results are processed sequentially on the main task. Only one scratch
// object is alive at a time; placement construction starts its C++ lifetime.
union ReceiveScratch {
 NetResult weather; FinanceResult finance; NewsResult news; LaunchResult launch;
 QuakeResult quake; IssResult iss; AircraftResult aircraft;
 ReceiveScratch() {} ~ReceiveScratch() {}
} receiveScratch;
static_assert(std::is_trivially_destructible<NetResult>::value && std::is_trivially_destructible<FinanceResult>::value && std::is_trivially_destructible<NewsResult>::value && std::is_trivially_destructible<LaunchResult>::value && std::is_trivially_destructible<QuakeResult>::value && std::is_trivially_destructible<IssResult>::value && std::is_trivially_destructible<AircraftResult>::value,"Receive scratch requires trivial destructors");
