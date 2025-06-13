#pragma once
#include "framework.h"

namespace Misc {
    void nullFunc() {}

    int True() {
        return 1;
    }

    int False() {
        return 0;
    }

    static inline void (*KickPlayerOG)(AGameSession*, AController*);
    static void KickPlayer(AGameSession*, AController*) {
        return;
    }

    void (*DispatchRequestOG)(__int64 a1, unsigned __int64* a2, int a3);
    void DispatchRequest(__int64 a1, unsigned __int64* a2, int a3)
    {
        return DispatchRequestOG(a1, a2, 3);
    }

    void Hook() {
        MH_CreateHook((LPVOID)(ImageBase + 0x2D95E00), True, nullptr); // collectgarbage
        MH_CreateHook((LPVOID)(ImageBase + 0x4155600), KickPlayer, (LPVOID*)&KickPlayerOG); // Kickplayer
        MH_CreateHook((LPVOID)(ImageBase + 0x1E23840), False, nullptr);// change gamesession id
        MH_CreateHook((LPVOID)(ImageBase + 0x108D740), DispatchRequest, (LPVOID*)&DispatchRequestOG);

        MH_CreateHook((LPVOID)(ImageBase + 0x3ca10c0), nullFunc, nullptr);
        MH_CreateHook((LPVOID)(ImageBase + 0x2d95e00), nullFunc, nullptr);
        MH_CreateHook((LPVOID)(ImageBase + 0x3262100), nullFunc, nullptr);
        MH_CreateHook((LPVOID)(ImageBase + 0x1e23840), nullFunc, nullptr);
        MH_CreateHook((LPVOID)(ImageBase + 0x2d95dc0), nullFunc, nullptr);

        Log("Misc Hooked!");
    }
}