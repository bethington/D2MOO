
#include <QUESTS/Quests.h>

// 0xF4 was verified on 1.10f; the 1.13c size has not been proven yet (see proof-gated layout adoption).
static_assert(sizeof(D2QuestDataStrc) == 0xF4, "Size of D2QuestDataStrc changed -- verify against 1.13c before trusting");
