#pragma once
#include <vector>
#include <PxPhysicsAPI.h>

// íœ‘Ò‚¿•¨—Actor‚Ìˆê•ÛƒLƒ…[
namespace PhysicsDestroyQueue
{
    inline std::vector<physx::PxActor*>& GetQueue()
    {
        static std::vector<physx::PxActor*> queue;
        return queue;
    }

    // ‰ğ•ú—\–ñ‚·‚é
    inline void Push(physx::PxActor* actor)
    {
        if (actor)
        {
            GetQueue().push_back(actor);
        }
    }
}