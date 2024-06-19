#include "container_reader/ContainerReader.h"

#include <memory>

#include "ll/api/plugin/RegisterHelper.h"

#include "ll/api/event/EventBus.h"
#include "ll/api/event/ListenerBase.h"
#include "ll/api/event/player/PlayerInteractBlockEvent.h"

#include "mc/server/ServerPlayer.h"
#include "mc/world/Container.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/level/block/actor/BlockActor.h"
#include "mc/world/level/block/actor/ChestBlockActor.h"
#include "mc/world/level/block/utils/BlockActorType.h"
#include "mc/world/level/dimension/Dimension.h"

#include <string>

namespace {
ll::event::ListenerPtr evListener;
void                   SendContentToPlayer(ServerPlayer& pl, Container* container) {
    auto        slots = container->getSlots();
    std::string contentStr;
    int         cnt = 0;
    for (auto item : slots) {
        if (!item->isNull()) {
            ++cnt;
            std::string name = item->getName();
            if (name.size() > 50) name = name.substr(0, 50) + "..."; // prevent overflow attack
            contentStr += "§6" + name + "§2(" + std::to_string(item->mCount) + ")§r, ";
        }
    }
    if (cnt == 0) {
        pl.sendMessage("[ContentReader] 点击的容器为空！");
    } else {
        pl.sendMessage(
            "[ContentReader] 点击的容器中共有" + std::to_string(cnt) + "堆物品：\n"
            + contentStr.substr(0, contentStr.size() - 2)
        );
    }
}
} // namespace


namespace container_reader {

static std::unique_ptr<ContainerReader> instance;

ContainerReader& ContainerReader::getInstance() { return *instance; }

bool ContainerReader::load() { return true; }

bool ContainerReader::enable() {
    auto& eventBus = ll::event::EventBus::getInstance();
    evListener =
        eventBus.emplaceListener<ll::event::PlayerInteractBlockEvent>([](ll::event::PlayerInteractBlockEvent& ev) {
            auto&       bs = ev.self().getDimension().getBlockSourceFromMainChunkSource();
            const auto& bl = bs.getBlock(ev.blockPos());
            auto*       ba = bs.getBlockEntity(ev.blockPos());
            if (bl.isContainerBlock() && ba) {
                auto type = ba->getType();
                if (type == BlockActorType::Chest || type == BlockActorType::ShulkerBox) {
                    // chest or shulker_box, check if can be open
                    auto cba = reinterpret_cast<ChestBlockActor*>(ba);
                    if (!cba->canOpen(bs)) {
                        // cannot open, send content to player
                        ::SendContentToPlayer(ev.self(), ba->getContainer());
                    }
                }
            }
        });

    return true;
}

bool ContainerReader::disable() {
    auto& eventBus = ll::event::EventBus::getInstance();
    eventBus.removeListener(evListener);

    return true;
}

} // namespace container_reader

LL_REGISTER_PLUGIN(container_reader::ContainerReader, container_reader::instance);
