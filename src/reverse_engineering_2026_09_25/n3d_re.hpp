#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>
namespace n3d::re {
enum class Evidence:uint8_t{Confirmed,Inferred,Unknown};
inline constexpr std::size_t kMaxVec=1000,kMaxObjects=350,kMaxGuards=100,kMaxDoorControllers=64,kMaxSecretPanels=32,kMaxPushObjects=12,kMaxProjectiles=8,kMaxVisibleSpans=50;
inline constexpr std::size_t kVecStride=28,kObjectStride=28,kGuardStride=26,kDoorStride=22,kPushStride=6,kProjectileStride=42,kVisibleSpanStride=20;
enum InputMask:uint16_t{InputEscape=0x0001,InputForward=0x0002,InputBackward=0x0004,InputTurnLeft=0x0008,InputTurnRight=0x0010,InputUse=0x0200};
enum class ObjectClass:uint8_t{Fire=7,Bat=8,Frankenstein=9,Mummy=10,Skeleton=11,MrsHamerstein=12,Zelda=13,Vampira=14,Baddie1=15,Baddie2=16,Dracula=17,CemeteryGargoyle=18,GardenGargoyle=19,Unknown20=20,Penelope=21,DrHamerstein=22,TallSlimRobot=23,TrashcanRobot=24,Cannon=25,Ghost=26,Goldie=27,Greenie=28,Demon=29,Alien1=30,Alien2=31,Unknown32=32};
enum class GuardState:uint8_t{AnimateLoop=0,Delay=1,Wake=2,Acquire=3,Attack=4,Plan=5,Move=6,WaitForLos=7,WallMove=8,Interact=9,NoDirectBranch0A=0x0A,Disabled=0x0B,Directional0C=0x0C,Directional0D=0x0D,CannonIdle=0x0E,CannonCycle=0x0F,CannonAttack=0x10,RecoverMove=0x11,SequenceOffset=0x12,Strategy3=0x13,GlobalTimed=0x14,HitReaction=0x15};
enum class GuardStrategy:uint8_t{Generic=0,LowHpTargetSearch=1,AlternateMove=2,SpecialWall=3,HitReactionSuppressed=4};
#pragma pack(push,1)
struct GuardRecord{uint8_t unk00[2];uint32_t renderStamp;int16_t timer;uint16_t linkedObjectIndex;uint8_t strategy,state,nextState,underlyingMapObject,areaSelector,directionMode,hp,facing,sequenceDirectionKey;int8_t moveX,moveY;uint8_t unk15,perceptionMode,losResult,proximityResult,unk19;};
static_assert(sizeof(GuardRecord)==26);
struct ObjectRecord{uint8_t unk00[3];uint8_t frame,sequenceSelector,flags,objectClass,unk07;uint32_t animationDeadline;uint8_t unk0C[4];int16_t worldX,worldY;uint8_t type,unk15[3];int16_t projectedBaseRow,verticalOffset;};
static_assert(sizeof(ObjectRecord)==28);
struct DemoEventWin16{uint8_t keyEvent;uint16_t inputMask;uint8_t unused;uint32_t generation;};
static_assert(sizeof(DemoEventWin16)==8);
#pragma pack(pop)
struct ProjectileSlot{std::array<uint8_t,14> motion{};ObjectRecord object{};};static_assert(sizeof(ProjectileSlot)==42);
struct GameEventFlags{uint8_t secretPanelMask=0,cannonEnabled=1,storyLatch=0,oneShotH=0,eventG=0,eventA9=0,eventAA=0,darkEvent=0;};
class N3dRng{public:explicit N3dRng(uint32_t s=1):state_(s){}uint16_t next();private:uint32_t state_;};
int16_t scoreForClass(uint8_t);
struct DamageInput{int16_t projectedBaseRow,viewportCenterY;uint8_t objectClass,weaponSelector,difficulty,episode;uint16_t rngValue;};
struct DamageResult{int rawSeed,transformed,afterDifficulty;uint8_t storedByte;Evidence confidence;};
DamageResult computeDamage(const DamageInput&);
std::optional<uint8_t> fireDamagePerSlowUpdate(uint8_t);
struct Placement{uint8_t episode,level,guard;std::string_view name;uint16_t count;bool playable;};
const std::vector<Placement>& placements();
uint32_t guardCountForLevel(uint8_t,uint8_t);
uint32_t guardCountForEpisode(uint8_t);
uint32_t totalPlayableGuardPlacements();
int64_t oneAwardPerPlacementBaseline(bool excludePenelopePenalty=false);
enum class GuardCategory:uint8_t{Combat,WallEnemy,NPC,PuzzleActor,BossOrScripted,Hazard,Unknown};
GuardCategory categoryForGuard(uint8_t);
} // namespace n3d::re
