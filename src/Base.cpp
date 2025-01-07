#include <Base.h>

void Base::Init() {
  rgInfoList.push_back(Base::RaceGroupInfo{});
  auto &rg0 = rgInfoList[0];
  rg0.idx = 0;
  rg0.name = "TNGRg0";
  rg0.isMain = true;
  rg0.noMCM = true;
  LoadAddons();
}

// Addons loading and handling
void Base::LoadAddons() {
  malAddons.clear();
  femAddons.clear();
  auto &armorlist = Tng::SEDH()->GetFormArray<RE::TESObjectARMO>();
  for (const auto &armor : armorlist) {
    if (FormHasKW(armor, Tng::cMalAddKeyID)) malAddons.push_back({armor, true});
    if (FormHasKW(armor, Tng::cFemAddKeyID)) femAddons.push_back({armor, false});
  }
  for (auto &armorPair : malAddons)
    if (armorPair.first->HasKeyword(Tng::ArmoKey(Tng::akeyIgnored))) armorPair.first->AddKeyword(Tng::ArmoKey(Tng::akeyIgnored));
  for (auto &armorPair : femAddons)
    if (armorPair.first->HasKeyword(Tng::ArmoKey(Tng::akeyIgnored))) armorPair.first->AddKeyword(Tng::ArmoKey(Tng::akeyIgnored));
}

RE::TESObjectARMO *Base::AddonByIdx(bool isFemale, size_t choice, bool onlyActive) {
  auto &list = isFemale ? femAddons : malAddons;
  RE::TESObjectARMO *res = nullptr;
  if (onlyActive) {
    int activeCount = 0;
    for (const auto &element : list) {
      if (element.second) {
        if (activeCount == choice) {
          res = element.first;
          break;
        }
        activeCount++;
      }
    }
  } else {
    res = choice < list.size() ? list[choice].first : nullptr;
  }
  return res;
}

size_t Base::GetAddonCount(bool isFemale, bool onlyActive) {
  auto &list = isFemale ? femAddons : malAddons;
  if (onlyActive) {
    size_t res = 0;
    for (const auto &addonPair : list)
      if (addonPair.second) res++;
    return res;
  } else {
    return list.size();
  }
}

bool Base::GetAddonStatus(const bool isFemale, const size_t addnIdx) {
  auto &list = isFemale ? femAddons : malAddons;
  if (addnIdx >= list.size()) return false;
  return list[addnIdx].second;
}

void Base::SetAddonStatus(const bool isFemale, const size_t addnIdx, const bool aIsActive) {
  auto &list = isFemale ? femAddons : malAddons;
  list[addnIdx].second = aIsActive;
}

// Sizes

float Base::GetGlobalSize(size_t idx) {
  if (idx < 0 || idx >= Tng::cSizeCategories) return 1.0f;
  return Tng::SizeGlb(idx)->value;
}

void Base::SetGlobalSize(size_t idx, float size) {
  if (idx < 0 || idx >= Tng::cSizeCategories) return;
  Tng::SizeGlb(idx)->value = size;
}

// Race handling and info
void Base::AddRace(RE::TESRace *race) {
  int rgCount = static_cast<int>(rgInfoList.size());
  auto rg = GetRg(race, true);
  if (rgInfoList.size() > rgCount)
    Tng::logger::info("The race [0x{:x}: {}] was recognized as a new group {}.", race->GetFormID(), race->GetFormEditorID(), rg->name);
  else
    Tng::logger::info("The race [0x{:x}: {}] was recognized as a member of existing group {}.", race->GetFormID(), race->GetFormEditorID(), rg->name);
}

void Base::UpdateRgSkins() {
  for (auto &rg : rgInfoList) {
    if (rg.addonIdx >= 0) {
      auto skin = GetSkinWithAddonForRg(&rg, rg.ogSkin, rg.addonIdx, false);
      for (auto &race : rg.races) race->skin = skin;
    }
  }
}

RE::TESRace *Base::GetRgRace0(const size_t rgIdx, const bool onlyMCM) {
  auto rg = GetRg(rgIdx, onlyMCM);
  return rg ? rg->races[0] : nullptr;
}

std::vector<std::string> Base::GetRgNames(const bool onlyMCM) {
  std::vector<std::string> res{};
  for (auto &rg : rgInfoList)
    if (!rg.noMCM || !onlyMCM) res.push_back(rg.name);
  return res;
}

std::string Base::GetRgRaceNames(size_t rgChoice, bool onlyMCM) {
  std::string res{""};
  auto rg = GetRg(rgChoice, onlyMCM);
  if (!rg) return res;
  auto &list = rg->races;
  for (auto race : list) {
    res = res + std::string(race->GetFile(0) ? race->GetFile(0)->GetFilename() : "(no mod name)") + " : " + race->GetFormEditorID();
    if (race != list.back()) {
      res = res + Tng::cDelimChar;
    }
  }
  return res;
}

int Base::GetRgAddn(const size_t rgChoice, bool onlyMCM) {
  auto rg = GetRg(rgChoice, onlyMCM);
  if (!rg) return Tng::pgErr;
  return rg->addonIdx;
}

bool Base::SetRgAddn(const size_t rgChoice, const int addnChoice, bool onlyMCM) {
  auto rg = GetRg(rgChoice, onlyMCM);
  if (!rg || addnChoice < -2 || rg->malAddons.find(addnChoice) == rg->malAddons.end()) return false;
  rg->addonIdx = (addnChoice == -1) ? rg->defAddonIdx : addnChoice;
  auto skin = (rg->addonIdx == -2) ? rg->ogSkin : GetSkinWithAddonForRg(rg, rg->ogSkin, rg->addonIdx, false);
  for (auto &race : rg->races) race->skin = skin;
  return true;
}

float Base::GetRgMult(const size_t rgChoice, bool onlyMCM) {
  auto rg = GetRg(rgChoice, onlyMCM);
  if (!rg) return 1.0f;
  return rg->mult;
}

bool Base::SetRgMult(const size_t rgIdx, const float aMult) {
  if (rgInfoList.size() <= rgIdx || aMult < 0.1f || aMult >= 10.0f) {
    Tng::logger::critical("Failure in setting a race mult!");
    return false;
  }
  auto &rg = rgInfoList[rgIdx];
  rg.mult = aMult;
  return true;
}

int Base::GetRaceRgIdx(RE::TESRace *race) { return raceRgs.find(race) != raceRgs.end() ? static_cast<int>(raceRgs[race]) : -1; }

std::vector<size_t> Base::GetRgAddonList(size_t rgChoice, bool isFemale, bool onlyDedicated, bool onlyMCM) {
  std::vector<size_t> res{};
  auto rg = GetRg(rgChoice, onlyMCM);
  if (!rg) return res;
  auto &list = isFemale ? rg->femAddons : rg->malAddons;
  for (auto &addonpair : list)
    if (!onlyDedicated || addonpair.second.first) res.push_back(addonpair.first);
  return res;
}

int Base::GetRgAddn(RE::TESRace *race) {
  if (!race) return Tng::pgErr;
  auto rg = GetRg(race, false);
  if (!rg) return Tng::pgErr;
  return rg->addonIdx;
}

float Base::GetRgMult(RE::TESRace *race) {
  if (!race) return 1.0f;
  return GetRg(race, false)->mult;
}

RE::TESObjectARMO *Base::GetSkinWithAddonForRg(const size_t rgIdx, RE::TESObjectARMO *skin, const size_t addonIdx, const bool isFemale) {
  return GetSkinWithAddonForRg(&rgInfoList[rgIdx], skin, addonIdx, isFemale);
}

Base::RaceGroupInfo *Base::GetRg(const size_t rgChoice, const bool onlyMCM) {
  int idx = -1;
  if (onlyMCM) {
    size_t curr = 0;
    for (int i = 0; i < rgInfoList.size(); i++) {
      auto &rg = rgInfoList[i];
      if (!rg.noMCM) {
        if (curr == rgChoice) {
          idx = i;
          break;
        }
        curr++;
      }
    }
  } else {
    idx = rgChoice < rgInfoList.size() ? static_cast<int>(rgChoice) : -1;
  }
  return idx >= 0 ? &rgInfoList[idx] : nullptr;
}

Base::RaceGroupInfo *Base::GetRg(RE::TESRace *race, const bool allowAdd) {
  if (raceRgs.find(race) != raceRgs.end()) return &rgInfoList[raceRgs[race]];
  if (!allowAdd) return nullptr;
  for (size_t i = 0; i < rgInfoList.size(); i++) {
    auto &rg = rgInfoList[i];
    auto it = std::find(rg.races.begin(), rg.races.end(), race);
    if (it != rg.races.end()) {
      raceRgs.insert({race, i});
      return &rg;
    }
    if (rg.armorRace == race && rg.ogSkin == race->skin) {
      rg.races.push_back(race);
      raceRgs.insert({race, i});
      return &rg;
    }
    if (race->armorParentRace && rg.armorRace == race->armorParentRace && rg.ogSkin == race->skin) {
      rg.races.push_back(race);
      raceRgs.insert({race, i});
      return &rg;
    }
  }
  if (race->armorParentRace && race->armorParentRace->skin == race->skin) {
    auto rg = GetRg(race->armorParentRace, true);
    rg->races.push_back(race);
    raceRgs.insert({race, rg->idx});
    return rg;
  } else {
    auto pRace = race;
    while (pRace->armorParentRace) {
      pRace = pRace->armorParentRace;
    };
    auto filename = race->GetFile(0) ? race->GetFile(0)->GetFilename() : "Unknown";
    auto lName = std::string(filename) + " : " + race->GetFormEditorID();
    rgInfoList.push_back({});
    auto &rg = rgInfoList.back();
    rg.idx = rgInfoList.size() - 1;
    rg.name = lName;
    rg.armorRace = pRace;
    rg.ogSkin = race->skin;
    rg.isMain = (pRace == race);
    rg.races.push_back(race);
    rg.noMCM = false;
    rg.mult = 1.0f;
    rg.defAddonIdx = GetRgDefAddn(rg);
    raceRgs.insert({race, rg.idx});
    UpdateRgAddons(rg);
    return &rg;
  }
}

int Base::GetRgDefAddn(Base::RaceGroupInfo &rg) {
  if (rg.defAddonIdx != -1) return rg.defAddonIdx;
  SEFormLoc defAddon;
  bool defAddonSet = false;
  const auto lRaceDesc = std::string(rg.armorRace->GetFormEditorID()) + std::string(rg.armorRace->GetName());
  for (std::uint32_t i = 0; i < cVanillaRaceTypes; i++) {
    if (lRaceDesc.contains(cRaceNames[i].first[0]) || lRaceDesc.contains(cRaceNames[i].first[1]) && (rg.armorRace->HasKeyword(Tng::RaceKey(Tng::rkeyBeast)) == cRaceNames[i].second)) {
      defAddon = {cRaceDefaults[i], Tng::cName};
      defAddonSet = true;
      break;
    }
  }
  if (!defAddonSet) defAddon = rg.armorRace->HasKeyword(Tng::RaceKey(Tng::rkeyBeast)) ? SEFormLoc({cRaceDefaults[9], Tng::cName}) : SEFormLoc({cRaceDefaults[0], Tng::cName});
  for (auto i = 0; i < malAddons.size(); i++) {
    if (FormToLoc(malAddons[i].first) == defAddon) return i;
  }
  Tng::logger::debug("Addon shouldn't get here!");
  return -2;
}

void Base::UpdateRgAddons(Base::RaceGroupInfo &rg) {
  rg.malAddons.clear();
  rg.femAddons.clear();
  for (int i = 0; i < malAddons.size(); i++) {
    auto &addon = malAddons[i].first;
    bool lSupports = false;
    for (auto &aa : addon->armorAddons) {
      if (aa->IsValidRace(rg.armorRace)) {
        rg.malAddons.insert_or_assign(i, std::pair<bool, RE::TESObjectARMA *>({rg.isMain || AddonHasRace(aa, rg.races[0]), aa}));
        lSupports = true;
        if (rg.malAddons[i].first) break;
      }
    }
    if (!lSupports) continue;
    if (i == rg.defAddonIdx && rg.addonIdx < 0) rg.addonIdx = i;
    if (!rg.isMain && rg.malAddons[i].first && (rg.addonIdx < 0 || !rg.malAddons[rg.addonIdx].first)) {
      rg.defAddonIdx = i;
      rg.addonIdx = i;
    }
    if (rg.malAddons[i].first)
      Tng::logger::info("The addon [0x{:x}] from file [{}] fully supports men in the race group [{}]!", addon->GetFormID(), addon->GetFile(0)->GetFilename(), rg.name);
    else
      Tng::logger::warn("The addon [0x{:x}] from file [{}] can be used for men in the race group [{}]!", addon->GetFormID(), addon->GetFile(0)->GetFilename(), rg.name);
  }
  for (size_t i = 0; i < femAddons.size(); i++) {
    auto &addon = femAddons[i].first;
    bool lSupports = false;
    for (auto &aa : addon->armorAddons) {
      if (aa->IsValidRace(rg.armorRace)) {
        rg.femAddons.insert_or_assign(i, std::pair<bool, RE::TESObjectARMA *>({rg.isMain || AddonHasRace(aa, rg.races[0]), aa}));
        lSupports = true;
        if (rg.femAddons[i].first) break;
      }
    }
    if (!lSupports) continue;
    if (rg.femAddons[i].first)
      Tng::logger::info("The addon [0x{:x}] from file [{}] fully supports women in the race group [{}]!", addon->GetFormID(), addon->GetFile(0)->GetFilename(), rg.name);
    else
      Tng::logger::warn("The addon [0x{:x}] from file [{}] can be used for women in the race group [{}]!", addon->GetFormID(), addon->GetFile(0)->GetFilename(), rg.name);
  }
}

RE::TESObjectARMO *Base::GetSkinWithAddonForRg(RaceGroupInfo *rg, RE::TESObjectARMO *skin, const size_t addonIdx, const bool isFemale) {
  auto &r = rg->isMain ? rgInfoList[0] : *rg;
  auto &skinMap = isFemale ? r.femSkins : r.malSkins;
  auto &ogSkin = skin->HasKeyword(Tng::ArmoKey(Tng::akeyGenSkin)) ? ogSkins[skin] : skin;
  if (!ogSkin->HasKeyword(Tng::ArmoKey(Tng::akeyGenSkin)) && !ogSkin->HasKeyword(Tng::ArmoKey(Tng::akeyIgnored))) ogSkin->AddKeyword(Tng::ArmoKey(Tng::akeyIgnored));
  if (skinMap.find(ogSkin) == skinMap.end()) skinMap.insert({ogSkin, std::map<size_t, RE::TESObjectARMO *>{}});
  RE::TESObjectARMO *resSkin = nullptr;
  if (skinMap[ogSkin].find(addonIdx) == skinMap[ogSkin].end()) {
    resSkin = ogSkin->CreateDuplicateForm(true, (void *)resSkin)->As<RE::TESObjectARMO>();
    resSkin->Copy(ogSkin);
    resSkin->AddKeyword(Tng::ArmoKey(Tng::akeyGenSkin));  // ToDo: If skin doesn't have the Ignored key, it must be added here.
    resSkin->AddSlotToMask(Tng::cSlotGenital);
    ogSkins.insert({resSkin, ogSkin});
  } else {
    resSkin = skinMap[ogSkin][addonIdx];
  }
  auto &reqAA = isFemale ? rg->femAddons[addonIdx].second : rg->malAddons[addonIdx].second;
  if (std::find(resSkin->armorAddons.begin(), resSkin->armorAddons.end(), reqAA) == resSkin->armorAddons.end()) resSkin->armorAddons.push_back(reqAA);
  return resSkin;
}

//  NPC handling and info

Tng::TNGRes Base::CanModifyActor(RE::Actor *actor) {
  const auto npc = actor ? actor->GetActorBase() : nullptr;
  if (!actor || !npc) return Tng::npcErr;
  if (!npc->race) return Tng::raceErr;
  if (npc->race->HasKeyword(Tng::RaceKey(Tng::rkeyProcessed))) return Tng::resOkRaceP;
  if (npc->race->HasKeyword(Tng::RaceKey(Tng::rkeyReady))) return Tng::resOkRaceR;
  return Tng::raceErr;
}

Tng::TNGRes Base::GetActorSizeCat(RE::Actor *actor, int &sizeCat) {
  sizeCat = -1;
  const auto npc = actor ? actor->GetActorBase() : nullptr;
  if (!npc) return Tng::npcErr;
  if (!npc->race) return Tng::raceErr;
  if (!(npc->race->HasKeyword(Tng::RaceKey(Tng::rkeyProcessed)) || npc->race->HasKeyword(Tng::RaceKey(Tng::rkeyReady)))) return Tng::raceErr;
  if (npc->IsPlayer() && Tng::boolSettings[Tng::bsExcludePlayerSize]) return Tng::playerErr;
  for (size_t i = 0; i < Tng::cSizeCategories; i++) {
    if (npc->HasKeyword(Tng::SizeKey(i))) sizeCat = static_cast<int>(i);
  }
  if (sizeCat == -1) sizeCat = npc->formID % Tng::cSizeCategories;
  return Tng::resOkSizable;
}

Tng::TNGRes Base::SetActorSizeCat(RE::Actor *actor, const int sizeCat) {
  auto currCat = -1;
  auto res = GetActorSizeCat(actor, currCat);
  if (res != Tng::resOkSizable) return res;
  const auto npc = actor ? actor->GetActorBase() : nullptr;
  if (!npc) return Tng::npcErr;
  auto cat = (sizeCat == -1) ? currCat : sizeCat;
  if (cat != currCat) {
    npc->RemoveKeywords(Tng::SizeKeys());
    npc->AddKeyword(Tng::SizeKey(cat));
  }
  auto catGlb = Tng::SizeGlb(cat);
  auto mult = GetRgMult(actor->GetRace());
  if (mult < 0.0f) return Tng::rgErr;
  auto lScale = mult * catGlb->value;
  if (lScale < 0.1) lScale = 1;
  RE::NiAVObject *aBaseNode = actor->GetNodeByName(cBaseBone);
  RE::NiAVObject *aScrtNode = actor->GetNodeByName(cScrtBone);
  if (!aBaseNode || !aScrtNode) return Tng::skeletonErr;
  aBaseNode->local.scale = lScale;
  aScrtNode->local.scale = 1.0f / sqrt(lScale);
  return Tng::resOkSizable;
}

void Base::ExcludeNPC(const std::string npcRecord) {
  auto npc = LoadForm<RE::TESNPC>(npcRecord);
  if (!npc || !npc->race || !npc->race->skin) return;
  npc->AddKeyword(Tng::NPCKey(Tng::npckeyExclude));
}

std::pair<bool, int> Base::GetNPCAddn(RE::TESNPC *npc) {
  if (!npc) {
    Tng::logger::critical("Failure in getting a NPC genital!");
    return std::make_pair(false, Tng::pgErr);
  }
  if (npc->keywords) {
    for (std::uint32_t idx = 0; idx < npc->numKeywords; ++idx) {
      if (!npc->keywords[idx] || npc->keywords[idx]->GetFormEditorID() == NULL) continue;
      const std::string lKwStr(npc->keywords[idx]->GetFormEditorID());
      if (lKwStr.starts_with(cNPCAutoAddn)) {
        return std::make_pair(false, std::strtol(lKwStr.substr(strlen(cNPCAutoAddn), 2).data(), nullptr, 0));
      }
      if (lKwStr.starts_with(cNPCUserAddn)) {
        return std::make_pair(true, std::strtol(lKwStr.substr(strlen(cNPCUserAddn), 2).data(), nullptr, 0));
      }
    }
  }
  return std::make_pair(false, -1);
}

Tng::TNGRes Base::SetNPCAddn(RE::TESNPC *npc, int addnIdx, bool isUser) {
  if (addnIdx < -2) return Tng::addonErr;
  if (!npc) {
    Tng::logger::critical("Failure in setting a NPC genital!");
    return Tng::npcErr;
  }
  auto res = (npc->IsPlayer() && Tng::boolSettings[Tng::bsExcludePlayerSize]) ? Tng::resOkFixed : Tng::resOkSizable;
  if (addnIdx == -1 && (npc->IsFemale() || !npc->skin || npc->skin->HasKeyword(Tng::ArmoKey(Tng::akeyGenSkin))))
    return !npc->IsFemale() || (npc->skin && npc->skin->HasKeyword(Tng::ArmoKey(Tng::akeySkinWP))) ? res : Tng::resOkFixed;
  auto &skin = npc->skin ? npc->skin : npc->race->skin;
  auto &ogSkin = skin->HasKeyword(Tng::ArmoKey(Tng::akeyGenSkin)) ? ogSkins[skin] : skin;
  auto rg = GetRg(npc->race, false);
  auto addonChoice = addnIdx == -1 ? rg->addonIdx : addnIdx;
  OrganizeNPCAddonKeywords(npc, addonChoice, isUser);
  auto resSkin = addonChoice == -2 ? ogSkin : GetSkinWithAddonForRg(rg, ogSkin, addonChoice, npc->IsFemale());
  if (resSkin != skin) npc->skin = resSkin == npc->race->skin ? nullptr : resSkin;
  return !npc->IsFemale() || npc->skin->HasKeyword(Tng::ArmoKey(Tng::akeySkinWP)) ? res : Tng::resOkFixed;
}

void Base::OrganizeNPCAddonKeywords(RE::TESNPC *npc, int addnIdx, bool isUser) {
  npc->RemoveKeywords(Tng::NPCKeys());
  npc->ForEachKeyword([&](RE::BGSKeyword *lKw) {
    if (lKw->formEditorID.contains("TNG_ActorAddn")) {
      npc->RemoveKeyword(lKw);
      return RE::BSContainer::ForEachResult::kStop;
    }
    return RE::BSContainer::ForEachResult::kContinue;
  });
  if (addnIdx == -2) {
    if (npc->IsFemale()) npc->AddKeyword(Tng::NPCKey(Tng::npckeyExclude));
    for (RE::BSTArray<RE::TESForm *>::const_iterator it = Tng::GentFml()->forms.begin(); it < Tng::GentFml()->forms.end(); it++) {
      if ((*it)->As<RE::TESNPC>() == npc) Tng::GentFml()->forms.erase(it);
    }
  } else {
    auto lChoice = static_cast<size_t>(addnIdx);
    std::string lReqKw = (isUser ? cNPCUserAddn : cNPCAutoAddn) + (lChoice < 10 ? "0" + std::to_string(lChoice) : std::to_string(lChoice));
    auto lKw = Tng::ProduceOrGetKw(lReqKw);
    npc->AddKeyword(lKw);
    auto &list = npc->IsFemale() ? femAddons : malAddons;
    if (list[addnIdx].first->HasKeyword(Tng::ArmoKey(Tng::akeySkinWP))) {
      Tng::GentFml()->AddForm(npc);
      npc->AddKeyword(Tng::NPCKey(Tng::npckeyGentlewoman));
    }
  }
}

// Loading
bool Base::LoadRgMult(const std::string rgIdRaceRecord, const float size) {
  auto race = LoadForm<RE::TESRace>(rgIdRaceRecord);
  if (!race) {
    Tng::logger::error("A previously saved race cannot be found anymore! It's information is removed from ini file.");
    return false;
  }
  GetRg(race, false)->mult = size;
  return true;
}

bool Base::LoadRgAddn(const std::string rgIdRaceRecord, const std::string addonRecord) {
  auto race = LoadForm<RE::TESRace>(rgIdRaceRecord);
  if (!race) {
    Tng::logger::error("A previously saved race cannot be found anymore! It's information is removed from ini file.");
    return false;
  }
  auto addon = LoadForm<RE::TESObjectARMO>(addonRecord);
  if (!addon) {
    Tng::logger::error("A previously saved addon cannot be found anymore! It's information is removed from ini file.");
    return false;
  }
  auto rg = GetRg(race, false);
  if (!rg) {
    Tng::logger::error("A previously saved race is not categorized anymore! It's information is removed from ini file.");
    return false;
  }
  if (addonRecord == "None") {
    rg->addonIdx = -2;
    return true;
  }
  for (int i = 0; i < malAddons.size(); i++)
    if (malAddons[i].first == addon) {
      rg->addonIdx = i;
      return true;
    }
  return false;
}

bool Base::LoadNPCSize(const std::string npcRecord, const int size) {
  auto npc = LoadForm<RE::TESNPC>(npcRecord);
  if (!npc) return false;
  for (size_t i = 0; i < Tng::cSizeCategories; i++)
    if (npc->HasKeyword(Tng::SizeKey(i))) npc->RemoveKeyword(Tng::SizeKey(i));
  npc->AddKeyword(Tng::SizeKey(size));
  return true;
}

bool Base::LoadNPCAddn(const std::string npcRecord, const std::string aAddonRecord) {
  RE::TESNPC *npc = LoadForm<RE::TESNPC>(npcRecord);
  if (!npc || !npc->race) return false;
  auto addon = LoadForm<RE::TESObjectARMO>(aAddonRecord);
  if (!addon) {
    Tng::logger::error("The addon {} saved for NPC {} cannot be found anymore!", aAddonRecord, npcRecord);
    return false;
  }
  int index = -1;
  const auto &addons = npc->IsFemale() ? femAddons : malAddons;
  for (int i = 0; i < addons.size(); i++)
    if (addons[i].first == addon) {
      index = i;
      break;
    }
  if (index < 0) {
    Tng::logger::error("A previously installed addon {} cannot be loaded anymore! Please report this issue. [0x{:x}] would use original skin.", aAddonRecord, npc->GetFormID());
    return false;
  }
  if (index == GetRgAddn(npc->race) && !npc->IsFemale()) return true;
  auto rg = GetRg(npc->race, false);
  return rg ? SetNPCAddn(npc, index, true) >= 0 : false;  
}