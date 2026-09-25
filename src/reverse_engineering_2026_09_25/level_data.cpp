#include "n3d_re.hpp"
namespace n3d::re {
const std::vector<Placement>& placements(){static const std::vector<Placement>p={
{1,1,1,"Bat",12,true},{1,1,2,"Frankenstein",10,true},{1,1,3,"Mummy",9,true},{1,1,8,"Baddie #1",2,true},
{1,2,1,"Bat",5,true},{1,2,2,"Frankenstein",5,true},{1,2,4,"Skeleton",35,true},{1,2,7,"Vampira",11,true},
{1,3,1,"Bat",14,true},{1,3,3,"Mummy",3,true},{1,3,4,"Skeleton",13,true},{1,3,6,"Zelda",8,true},{1,3,7,"Vampira",11,true},
{1,4,1,"Bat",3,true},{1,4,2,"Frankenstein",5,true},{1,4,4,"Skeleton",15,true},{1,4,6,"Zelda",8,true},{1,4,7,"Vampira",12,true},
{1,5,8,"Baddie #1",19,true},{1,5,9,"Baddie #2",20,true},
{1,6,1,"Bat",13,true},{1,6,3,"Mummy",7,true},{1,6,4,"Skeleton",11,true},{1,6,7,"Vampira",5,true},{1,6,11,"Cemetery wall Gargoyle",19,true},
{1,7,1,"Bat",2,true},{1,7,5,"Mrs H.",6,true},{1,7,6,"Zelda",2,true},{1,7,7,"Vampira",6,true},{1,7,18,"Cannon",2,true},
{1,8,4,"Skeleton",17,true},{1,8,5,"Mrs H.",2,true},{1,8,11,"Cemetery wall Gargoyle",26,true},{1,8,12,"Garden wall Gargoyle",20,true},
{1,9,2,"Frankenstein",1,true},{1,9,4,"Skeleton",3,true},{1,9,5,"Mrs H.",8,true},{1,9,26,"Dancers",1,true},
{1,10,1,"Bat",1,true},{1,10,10,"Dracula",1,true},{1,10,14,"Penelope",1,true},{1,10,15,"Dr. Hamerstein",1,true},
{1,11,1,"Bat",14,false},{1,11,4,"Skeleton",13,false},{1,11,6,"Zelda",8,false},{1,11,8,"Baddie #1",15,false},
{2,1,1,"Bat",15,true},{2,1,3,"Mummy",8,true},{2,1,4,"Skeleton",18,true},{2,1,11,"Cemetery wall Gargoyle",10,true},{2,1,12,"Garden wall Gargoyle",29,true},
{2,2,4,"Skeleton",60,true},{2,2,8,"Baddie #1",17,true},{2,2,9,"Baddie #2",22,true},
{2,3,4,"Skeleton",15,true},{2,3,8,"Baddie #1",11,true},{2,3,9,"Baddie #2",13,true},
{2,4,4,"Skeleton",61,true},{2,4,9,"Baddie #2",12,true},{2,4,12,"Garden wall Gargoyle",26,true},
{2,5,4,"Skeleton",26,true},{2,5,8,"Baddie #1",17,true},{2,5,9,"Baddie #2",18,true},
{2,6,8,"Baddie #1",25,true},{2,6,9,"Baddie #2",17,true},
{2,7,16,"Tall slim robot",7,true},{2,7,17,"Trashcan robot",16,true},
{2,8,11,"Cemetery wall Gargoyle",5,true},{2,8,17,"Trashcan robot",19,true},{2,8,18,"Cannon",1,true},
{2,9,16,"Tall slim robot",12,true},{2,9,17,"Trashcan robot",8,true},{2,9,18,"Cannon",8,true},
{2,10,17,"Trashcan robot",5,true},
{3,1,19,"Ghost",18,true},{3,1,20,"Goldie",3,true},{3,1,21,"Greenie",6,true},{3,1,22,"Demon",1,true},
{3,2,19,"Ghost",9,true},{3,2,20,"Goldie",10,true},{3,2,22,"Demon",4,true},{3,2,23,"Alien #1",15,true},
{3,3,19,"Ghost",20,true},{3,3,20,"Goldie",7,true},{3,3,21,"Greenie",12,true},{3,3,24,"Alien #2",11,true},
{3,4,19,"Ghost",19,true},{3,4,20,"Goldie",12,true},{3,4,23,"Alien #1",13,true},
{3,5,19,"Ghost",14,true},{3,5,20,"Goldie",14,true},{3,5,21,"Greenie",11,true},{3,5,22,"Demon",10,true},
{3,6,19,"Ghost",17,true},{3,6,20,"Goldie",8,true},{3,6,24,"Alien #2",36,true},
{3,7,23,"Alien #1",5,true},{3,7,24,"Alien #2",23,true},
{3,8,19,"Ghost",29,true},{3,8,22,"Demon",5,true},{3,8,23,"Alien #1",11,true},
{3,9,19,"Ghost",25,true},{3,9,22,"Demon",7,true},{3,9,23,"Alien #1",13,true},
{3,10,14,"Penelope",1,true},{3,10,15,"Dr. Hamerstein",1,true}};return p;}
uint32_t guardCountForLevel(uint8_t e,uint8_t l){uint32_t t=0;for(auto&p:placements())if(p.playable&&p.episode==e&&p.level==l)t+=p.count;return t;}
uint32_t guardCountForEpisode(uint8_t e){uint32_t t=0;for(auto&p:placements())if(p.playable&&p.episode==e)t+=p.count;return t;}
uint32_t totalPlayableGuardPlacements(){uint32_t t=0;for(auto&p:placements())if(p.playable)t+=p.count;return t;}
int64_t oneAwardPerPlacementBaseline(bool x){int64_t t=0;for(auto&p:placements()){if(!p.playable)continue;if(x&&p.guard==14)continue;uint8_t c=(p.guard>=1&&p.guard<=24)?static_cast<uint8_t>(p.guard+7):0;t+=static_cast<int64_t>(scoreForClass(c))*p.count;}return t;}
} // namespace n3d::re
