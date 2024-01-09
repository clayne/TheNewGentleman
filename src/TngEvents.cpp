#include <TngEvents.h>
#include<TngSizeShape.h>

void TngEvents::CheckForRevealing(RE::TESObjectARMO* aBodyArmor, RE::TESObjectARMO* aPelvisArmor) noexcept {
  if (!aBodyArmor || !aPelvisArmor) return;
  if (aBodyArmor == aPelvisArmor) return;
  if (!aBodyArmor->HasKeyword(fAutoCoverKey) || aBodyArmor->HasKeyword(fRevealingKey) || aPelvisArmor->HasKeyword(fUnderwearKey)) return;
  if (aBodyArmor->GetFile(0)->GetFilename() != aPelvisArmor->GetFile(0)->GetFilename()) return;
  if (aBodyArmor->armorAddons.size() == 0) return;
  for (const auto& lAA : aBodyArmor->armorAddons) {
    if (lAA->HasPartOf(Tng::cSlotGenital) && lAA->HasPartOf(Tng::cSlotBody)) lAA->RemoveSlotFromMask(Tng::cSlotGenital);
    const auto lID = (std::string(aBodyArmor->GetName()).empty()) ? aBodyArmor->GetFormEditorID() : aBodyArmor->GetName();
    Tng::gLogger::info("The armor [0x{:x}:{}] from file [{}] was recognized revealing.", aBodyArmor->GetLocalFormID(), lID, aBodyArmor->GetFile(0)->GetFilename());
    aBodyArmor->RemoveKeyword(fAutoCoverKey);
    aBodyArmor->AddKeyword(fAutoRvealKey);
    return;
  }
}

void TngEvents::CheckForClipping(RE::Actor* aActor, RE::TESObjectARMO* aArmor) noexcept {
  if (!aActor || !aArmor) return;
  fInternal = true;
  fEquipManager->EquipObject(aActor, aArmor, nullptr, 1, nullptr, false, false, false, true);
}

void TngEvents::CheckActor(RE::Actor* aActor, RE::TESObjectARMO* aArmor) noexcept {
  const auto lNPC = aActor ? aActor->GetActorBase() : nullptr;
  if (!aActor || !lNPC) return;
  if (!lNPC->race) return;
  if (!lNPC->race->HasKeyword(fTNGRaceKey)) return;
  TngSizeShape::RandomizeScale(aActor);
  const auto lGArmo = aActor->GetWornArmor(Tng::cSlotGenital);
  if (aArmor && !lGArmo) {
    if (aArmor->HasKeyword(fCoveringKey) || aArmor->HasKeyword(fAutoCoverKey)) CheckForClipping(aActor, aArmor);
    return;
  }
  const auto lBArmo = aActor->GetWornArmor(Tng::cSlotBody);
  CheckForRevealing(lBArmo, lGArmo);
}

RE::BSEventNotifyControl TngEvents::ProcessEvent(const RE::TESEquipEvent* aEvent, RE::BSTEventSource<RE::TESEquipEvent>*) {
  if (!aEvent || fInternal) return RE::BSEventNotifyControl::kContinue;
  const auto lActor = aEvent->actor->As<RE::Actor>();
  auto lArmor = RE::TESForm::LookupByID<RE::TESObjectARMO>(aEvent->baseObject);
  if (!lArmor || !aEvent->equipped) return RE::BSEventNotifyControl::kContinue;
  if (lArmor->HasKeyword(fCoveringKey) || lArmor->HasKeyword(fAutoCoverKey) || lArmor->HasPartOf(Tng::cSlotGenital)) {
    CheckActor(lActor, lArmor);
    fInternal = false;
  }
  return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl TngEvents::ProcessEvent(const RE::TESObjectLoadedEvent* aEvent, RE::BSTEventSource<RE::TESObjectLoadedEvent>*) {
  if (!aEvent || fInternal) return RE::BSEventNotifyControl::kContinue;
  const auto lActor = RE::TESForm::LookupByID<RE::Actor>(aEvent->formID);
  TngSizeShape::RandomizeScale(lActor);
  CheckActor(lActor);
  return RE::BSEventNotifyControl::kContinue;
}

void TngEvents::RegisterEvents() noexcept {
  TngSizeShape::InitSizes();
  const auto lSourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
  fEquipManager = RE::ActorEquipManager::GetSingleton();
  fNPCKey = RE::TESDataHandler::GetSingleton()->LookupForm<RE::BGSKeyword>(Tng::cNPCKeywID, Tng::cSkyrim);
  fAutoRvealKey = RE::TESDataHandler::GetSingleton()->LookupForm<RE::BGSKeyword>(Tng::cAutoRvealKeyID, Tng::cName);
  fRevealingKey = RE::TESDataHandler::GetSingleton()->LookupForm<RE::BGSKeyword>(Tng::cRevealingKeyID, Tng::cName);
  fUnderwearKey = RE::TESDataHandler::GetSingleton()->LookupForm<RE::BGSKeyword>(Tng::cUnderwearKeyID, Tng::cName);
  fAutoCoverKey = RE::TESDataHandler::GetSingleton()->LookupForm<RE::BGSKeyword>(Tng::cAutoCoverKeyID, Tng::cName);
  fCoveringKey = RE::TESDataHandler::GetSingleton()->LookupForm<RE::BGSKeyword>(Tng::cCoveringKeyID, Tng::cName);
  fTNGRaceKey = RE::TESDataHandler::GetSingleton()->LookupForm<RE::BGSKeyword>(Tng::cTNGRaceKeyID, Tng::cName);
    Tng::gLogger::error("Mod cannot find necessary info for events, no events registered!");
    return;
  }
  lSourceHolder->AddEventSink<RE::TESEquipEvent>(GetSingleton());
  lSourceHolder->AddEventSink<RE::TESObjectLoadedEvent>(GetSingleton());
  Tng::gLogger::info("Registered for necessary events.");
}
